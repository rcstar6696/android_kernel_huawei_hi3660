/*
 * oases_signing.c - init key and verify signature
 *
 * Copyright (C) 2016 Baidu, Inc. All Rights Reserved.
 *
 * You should have received a copy of license along with this program;
 * if not, ask for it from Baidu, Inc.
 *
 */
#include <crypto/public_key.h>
#include <crypto/hash.h>
#include <keys/asymmetric-type.h>
#include <linux/cred.h>
#include <linux/key.h>
#include <linux/version.h>
#include <linux/err.h>

#include "util.h"
#include "oases_signing.h"

struct patch_signature {
	u8	algo;		/* Public-key crypto algorithm [enum pkey_algo] */
	u8	hash;		/* Digest algorithm [enum pkey_hash_algo] */
	u8	id_type;	/* Key identifier type [enum pkey_id_type] */
	u8	signer_len;	/* Length of signer's name */
	u8	key_id_len;	/* Length of key identifier */
	u8	__pad[3];
	__be32	sig_len;	/* Length of signature data */
};

static struct cred *oases_cred = NULL;
static struct key *oases_sign_keyring = NULL;
static struct key *oases_pub_key = NULL;
static struct key *vendor_pub_key = NULL;

extern __initdata const u8 oases_sign_certificate_list[];
extern __initdata const u8 oases_sign_certificate_list_end[];

extern __initdata const u8 vendor_sign_certificate_list[];
extern __initdata const u8 vendor_sign_certificate_list_end[];


static struct key *oases_key_create(const u8 *liststart, const u8 *listend)
{
	key_ref_t key;
	const u8 *p, *end;
	size_t plen;

	p = liststart;
	end = listend;
	while (p < end) {
		/* Each cert begins with an ASN.1 SEQUENCE tag and must be more
		 * than 256 bytes in size.
		 */
		if (end - p < 4)
			goto fail;
		if (p[0] != 0x30 &&
		    p[1] != 0x82)
			goto fail;
		plen = (p[2] << 8) | p[3];
		plen += 4;
		if (plen > end - p)
			goto fail;

		key = key_create_or_update(make_key_ref(oases_sign_keyring, 1),
					   "asymmetric",
					   NULL,
					   p,
					   plen,
					   (KEY_POS_ALL & ~KEY_POS_SETATTR) |
					   KEY_USR_VIEW,
					   KEY_ALLOC_NOT_IN_QUOTA);
		if (IS_ERR(key)) {
			oases_error("loading X.509 certificate (%ld) fail\n",
			       PTR_ERR(key));
			goto fail;
		} else {
			return key_ref_to_ptr(key);
		}
		p += plen;
	}

fail:
	return ERR_PTR(-ENOKEY);

}

int __init oases_init_signing_keys(void)
{
	struct key *key;

	oases_debug("init oases keys\n");
	oases_cred = prepare_kernel_cred(NULL);
	if (!oases_cred)
		return -ENOMEM;

	oases_sign_keyring = keyring_alloc(".oases_sign",
					KUIDT_INIT(0), KGIDT_INIT(0),
					oases_cred,
					((KEY_POS_ALL & ~KEY_POS_SETATTR) |
					 KEY_USR_VIEW | KEY_USR_READ),
					KEY_ALLOC_NOT_IN_QUOTA, NULL);
	if (IS_ERR(oases_sign_keyring)) {
		oases_error("can't allocate oases signing keyring\n");
		goto failed_put_cred;
	}

	key = oases_key_create(oases_sign_certificate_list,
				oases_sign_certificate_list_end);
	if (IS_ERR(key)) {
		oases_debug("loaded oases cert fail\n");
		goto failed_put_keyring;
	} else {
		oases_pub_key = key;
	}

	key = oases_key_create(vendor_sign_certificate_list,
				vendor_sign_certificate_list_end);
	if (IS_ERR(key)) {
		oases_debug("loaded vendor cert fail\n");
		goto failed_put_key;
	} else {
		vendor_pub_key = key;
	}

	return 0;

failed_put_key:
	key_put(oases_pub_key);
failed_put_keyring:
	key_put(oases_sign_keyring);
failed_put_cred:
	put_cred(oases_cred);
	return -ENOKEY;
}

void oases_destroy_signing_keys(void)
{
	if (vendor_pub_key) {
		key_put(oases_pub_key);
		key_put(vendor_pub_key);
		key_put(oases_sign_keyring);
		put_cred(oases_cred);
	}
}

/*
 * Digest the oases patch contents.
 *
 * 3.13+, hash type changed
 */
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3, 12, 255)
static struct public_key_signature *oases_make_digest(enum pkey_hash_algo hash,
#else
static struct public_key_signature *oases_make_digest(enum hash_algo hash,
#endif
						    const void *patch,
						    unsigned long patchlen)
{
	struct public_key_signature *pks;
	struct crypto_shash *tfm;
	struct shash_desc *desc;
	size_t digest_size, desc_size;
	int ret;

	/* Allocate the hashing algorithm we're going to need and find out how
	 * big the hash operational data will be.
	 */

#if LINUX_VERSION_CODE <= KERNEL_VERSION(3, 12, 255)
	tfm = crypto_alloc_shash(pkey_hash_algo[hash], 0, 0);
#else
	tfm = crypto_alloc_shash(hash_algo_name[hash], 0, 0);
#endif

	if (IS_ERR(tfm))
		return (PTR_ERR(tfm) == -ENOENT) ? ERR_PTR(-ENOPKG) : ERR_CAST(tfm);

	desc_size = crypto_shash_descsize(tfm) + sizeof(*desc);
	digest_size = crypto_shash_digestsize(tfm);

	/* We allocate the hash operational data storage on the end of our
	 * context data and the digest output buffer on the end of that.
	 */
	ret = -ENOMEM;
	pks = kzalloc(digest_size + sizeof(*pks) + desc_size, GFP_KERNEL);
	if (!pks)
		goto error_no_pks;

	pks->pkey_hash_algo	= hash;
	pks->digest		= (u8 *)pks + sizeof(*pks) + desc_size;
	pks->digest_size	= digest_size;

	desc = (void *)pks + sizeof(*pks);
	desc->tfm   = tfm;
	desc->flags = CRYPTO_TFM_REQ_MAY_SLEEP;

	ret = crypto_shash_init(desc);
	if (ret < 0)
		goto error;

	ret = crypto_shash_finup(desc, patch, patchlen, pks->digest);
	if (ret < 0)
		goto error;

	crypto_free_shash(tfm);
	return pks;

error:
	kfree(pks);
error_no_pks:
	crypto_free_shash(tfm);
	oases_debug("make digest fail, ret:%d\n", ret);
	return ERR_PTR(ret);
}

/*
 * Extract an MPI array from the signature data.  This represents the actual
 * signature.  Each raw MPI is prefaced by a BE 2-byte value indicating the
 * size of the MPI in bytes.
 *
 * RSA signatures only have one MPI, so currently we only read one.
 */
static int oases_extract_mpi_array(struct public_key_signature *pks,
				 const void *data, size_t len)
{
	size_t nbytes;
	MPI mpi;

	if (len < 3)
		return -EBADMSG;
	nbytes = ((const u8 *)data)[0] << 8 | ((const u8 *)data)[1];
	data += 2;
	len -= 2;
	if (len != nbytes)
		return -EBADMSG;

	mpi = mpi_read_raw_data(data, nbytes);
	if (!mpi)
		return -ENOMEM;
	pks->mpi[0] = mpi;
	pks->nr_mpi = 1;
	return 0;
}

int oases_verify_sig(char *data, unsigned long *_patchlen, int sig_type)
{
	struct public_key_signature *pks;
	struct patch_signature ps;
	const void *sig;
	size_t patchlen = *_patchlen;
	size_t sig_len;
	int ret;
	struct key *key;
	if (patchlen <= sizeof(ps))
		return -EBADMSG;

	memcpy(&ps, data + (patchlen - sizeof(ps)), sizeof(ps));
	patchlen -= sizeof(ps);

	sig_len = be32_to_cpu(ps.sig_len);
	if (sig_len >= patchlen)
		return -EBADMSG;
	patchlen -= sig_len;
	if (((size_t)ps.signer_len + (size_t)ps.key_id_len) >= patchlen)
		return -EBADMSG;
	patchlen -= (size_t)ps.signer_len + (size_t)ps.key_id_len;
	*_patchlen = patchlen;

	sig = data + patchlen;
	if (ps.algo != PKEY_ALGO_RSA ||
	    ps.id_type != PKEY_ID_X509)
		return -ENOPKG;

	if (ps.hash >= PKEY_HASH__LAST
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3, 12, 255)
			|| !pkey_hash_algo[ps.hash])
#else
			|| !hash_algo_name[ps.hash])
#endif
		return -ENOPKG;

	pks = oases_make_digest(ps.hash, data, patchlen);
	if (IS_ERR(pks)) {
		ret = PTR_ERR(pks);
		goto out;
	}
	ret = oases_extract_mpi_array(pks, sig + ps.signer_len + ps.key_id_len,
					sig_len);
	if (ret < 0)
		goto error_free_pks;

	if (sig_type == SIG_TYPE_SYSTEM)
		key = vendor_pub_key;
	else
		key = oases_pub_key;

	ret = verify_signature(key, pks);
	if (!ret)
		oases_debug("verify_signature sucess\n");

error_free_pks:
	mpi_free(pks->rsa.s);
	kfree(pks);
out:
	return ret;
}
