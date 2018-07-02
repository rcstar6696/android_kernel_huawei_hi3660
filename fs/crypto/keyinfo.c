/*
 * key management facility for FS encryption support.
 *
 * Copyright (C) 2015, Google, Inc.
 *
 * This contains encryption key functions.
 *
 * Written by Michael Halcrow, Ildar Muslukhov, and Uday Savagaonkar, 2015.
 */

#include <keys/user-type.h>
#include <linux/scatterlist.h>
#include <uapi/linux/keyctl.h>
#include <crypto/skcipher.h>
#include <crypto/hash.h>
#include <linux/fscrypto.h>

static void derive_crypt_complete(struct crypto_async_request *req, int rc)
{
	struct fscrypt_completion_result *ecr = req->data;

	if (rc == -EINPROGRESS)
		return;

	ecr->res = rc;
	complete(&ecr->completion);
}

/**
 * derive_key_aes() - Derive a key using AES-128-ECB
 * @deriving_key: Encryption key used for derivation.
 * @source_key:   Source key to which to apply derivation.
 * @derived_key:  Derived key.
 *
 * Return: Zero on success; non-zero otherwise.
 */
static int derive_key_aes(u8 deriving_key[FS_AES_128_ECB_KEY_SIZE],
				u8 source_key[FS_AES_256_XTS_KEY_SIZE],
				u8 derived_key[FS_AES_256_XTS_KEY_SIZE])
{
	int res = 0;
	struct skcipher_request *req = NULL;
	DECLARE_FS_COMPLETION_RESULT(ecr);
	struct scatterlist src_sg, dst_sg;
	struct crypto_skcipher *tfm = crypto_alloc_skcipher("ecb(aes)", 0, 0);

	if (IS_ERR(tfm)) {
		res = PTR_ERR(tfm);
		tfm = NULL;
		goto out;
	}
	crypto_skcipher_set_flags(tfm, CRYPTO_TFM_REQ_WEAK_KEY);
	req = skcipher_request_alloc(tfm, GFP_NOFS);
	if (!req) {
		res = -ENOMEM;
		goto out;
	}
	skcipher_request_set_callback(req,
			CRYPTO_TFM_REQ_MAY_BACKLOG | CRYPTO_TFM_REQ_MAY_SLEEP,
			derive_crypt_complete, &ecr);
	res = crypto_skcipher_setkey(tfm, deriving_key,
					FS_AES_128_ECB_KEY_SIZE);
	if (res < 0)
		goto out;

	sg_init_one(&src_sg, source_key, FS_AES_256_XTS_KEY_SIZE);
	sg_init_one(&dst_sg, derived_key, FS_AES_256_XTS_KEY_SIZE);
	skcipher_request_set_crypt(req, &src_sg, &dst_sg,
					FS_AES_256_XTS_KEY_SIZE, NULL);
	res = crypto_skcipher_encrypt(req);
	if (res == -EINPROGRESS || res == -EBUSY) {
		wait_for_completion(&ecr.completion);
		res = ecr.res;
	}
out:
	skcipher_request_free(req);
	crypto_free_skcipher(tfm);
	return res;
}

int fscrypt_set_gcm_key(struct crypto_aead *tfm,
			u8 deriving_key[FS_AES_256_GCM_KEY_SIZE])
{
	int res = 0;
	unsigned int iv_len;

	crypto_aead_set_flags(tfm, CRYPTO_TFM_REQ_WEAK_KEY);

	iv_len = crypto_aead_ivsize(tfm);
	if (iv_len > FS_KEY_DERIVATION_IV_SIZE) {
		res = -EINVAL;
		pr_err("fscrypt %s : IV length is incompatible\n", __func__);
		goto out;
	}

	res = crypto_aead_setauthsize(tfm, FS_KEY_DERIVATION_TAG_SIZE);
	if (res < 0) {
		pr_err("fscrypt %s : Failed to set authsize\n", __func__);
		goto out;
	}

	res = crypto_aead_setkey(tfm, deriving_key,
					FS_AES_256_GCM_KEY_SIZE);
	if (res < 0)
		pr_err("fscrypt %s : Failed to set deriving key\n", __func__);
out:
	return res;
}

int fscrypt_derive_gcm_key(struct crypto_aead *tfm,
				u8 source_key[FS_KEY_DERIVATION_CIPHER_SIZE],
				u8 derived_key[FS_KEY_DERIVATION_CIPHER_SIZE],
				u8 iv[FS_KEY_DERIVATION_IV_SIZE],
				int enc)
{
	int res = 0;
	struct aead_request *req = NULL;
	DECLARE_FS_COMPLETION_RESULT(ecr);
	struct scatterlist src_sg, dst_sg;
	unsigned int ilen;

	if (!tfm) {
		res = -EINVAL;
		goto out;
	}

	if (IS_ERR(tfm)) {
		res = PTR_ERR(tfm);
		goto out;
	}

	req = aead_request_alloc(tfm, GFP_NOFS);
	if (!req) {
		res = -ENOMEM;
		goto out;
	}

	aead_request_set_callback(req,
			CRYPTO_TFM_REQ_MAY_BACKLOG | CRYPTO_TFM_REQ_MAY_SLEEP,
			derive_crypt_complete, &ecr);

	ilen = enc ? FS_KEY_DERIVATION_NONCE_SIZE :
			FS_KEY_DERIVATION_CIPHER_SIZE;

	sg_init_one(&src_sg, source_key, FS_KEY_DERIVATION_CIPHER_SIZE);
	sg_init_one(&dst_sg, derived_key, FS_KEY_DERIVATION_CIPHER_SIZE);

	aead_request_set_ad(req, 0);

	aead_request_set_crypt(req, &src_sg, &dst_sg, ilen, iv);

	res = enc ? crypto_aead_encrypt(req) : crypto_aead_decrypt(req);
	if (res == -EINPROGRESS || res == -EBUSY) {
		wait_for_completion(&ecr.completion);
		res = ecr.res;
	}

out:
	if (req)
		aead_request_free(req);
	return res;
}

struct key *fscrypt_request_key(u8 *descriptor, u8 *prefix,
				int prefix_size)
{
	u8 *full_key_descriptor;
	struct key *keyring_key;
	int full_key_len = prefix_size + (FS_KEY_DESCRIPTOR_SIZE * 2) + 1;

	full_key_descriptor = kmalloc(full_key_len, GFP_NOFS);
	if (!full_key_descriptor)
		return (struct key *)ERR_PTR(-ENOMEM);

	memcpy(full_key_descriptor, prefix, prefix_size);
	sprintf(full_key_descriptor + prefix_size,
			"%*phN", FS_KEY_DESCRIPTOR_SIZE,
			descriptor);
	full_key_descriptor[full_key_len - 1] = '\0';
	keyring_key = request_key(&key_type_logon, full_key_descriptor, NULL);
	kfree(full_key_descriptor);

	return keyring_key;
}

static int validate_user_key(struct fscrypt_info *crypt_info,
			struct fscrypt_context *ctx, u8 *raw_key,
			u8 *prefix, int prefix_size)
{
	struct key *keyring_key;
	struct fscrypt_key *master_key;
	const struct user_key_payload *ukp;
	int res;
	u8 plain_text[FS_KEY_DERIVATION_CIPHER_SIZE] = {0};
	struct crypto_aead *tfm = NULL;

	keyring_key = fscrypt_request_key(ctx->master_key_descriptor,
				prefix, prefix_size);
	if (IS_ERR(keyring_key))
		return PTR_ERR(keyring_key);
	down_read(&keyring_key->sem);

	if (keyring_key->type != &key_type_logon) {
		printk_once(KERN_WARNING
				"%s: key type must be logon\n", __func__);
		res = -ENOKEY;
		goto out;
	}
	ukp = user_key_payload(keyring_key);
	if (ukp->datalen != sizeof(struct fscrypt_key)) {
		res = -EINVAL;
		goto out;
	}
	master_key = (struct fscrypt_key *)ukp->data;
	BUILD_BUG_ON(FS_AES_256_XTS_KEY_SIZE != FS_KEY_DERIVATION_NONCE_SIZE);

	//force the size equal to FS_AES_256_GCM_KEY_SIZE since user space might pass FS_AES_256_XTS_KEY_SIZE
	master_key->size = FS_AES_256_GCM_KEY_SIZE;
	if (master_key->size != FS_AES_256_GCM_KEY_SIZE) {
		printk_once(KERN_WARNING
				"%s: key size incorrect: %d\n",
				__func__, master_key->size);
		res = -ENOKEY;
		goto out;
	}

	tfm = (struct crypto_aead *)crypto_alloc_aead("gcm(aes)", 0, 0);
	if (IS_ERR(tfm)) {
		up_read(&keyring_key->sem);
		res = (int)PTR_ERR(tfm);
		tfm = NULL;
		pr_err("fscrypt %s : tfm allocation failed!\n", __func__);
		goto out;
	}

	res = fscrypt_set_gcm_key(tfm, master_key->raw);
	if (res)
		goto out;
	res = fscrypt_derive_gcm_key(tfm, ctx->nonce, plain_text, ctx->iv, 0);
	if (res)
		goto out;

	memcpy(raw_key, plain_text, FS_KEY_DERIVATION_NONCE_SIZE);

	crypt_info->ci_gtfm = tfm;
	up_read(&keyring_key->sem);
	key_put(keyring_key);
	return 0;
out:
	if (tfm)
		crypto_free_aead(tfm);
	up_read(&keyring_key->sem);
	key_put(keyring_key);
	return res;
}

static int determine_cipher_type(struct fscrypt_info *ci, struct inode *inode,
				 const char **cipher_str_ret, int *keysize_ret)
{
	if (S_ISREG(inode->i_mode)) {
		if (ci->ci_data_mode == FS_ENCRYPTION_MODE_AES_256_XTS) {
			*cipher_str_ret = "xts(aes)";
			*keysize_ret = FS_AES_256_XTS_KEY_SIZE;
			return 0;
		}
		pr_warn_once("fscrypto: unsupported contents encryption mode "
			     "%d for inode %lu\n",
			     ci->ci_data_mode, inode->i_ino);
		return -ENOKEY;
	}

	if (S_ISDIR(inode->i_mode) || S_ISLNK(inode->i_mode)) {
		if (ci->ci_filename_mode == FS_ENCRYPTION_MODE_AES_256_CTS) {
			*cipher_str_ret = "cts(cbc(aes))";
			*keysize_ret = FS_AES_256_CTS_KEY_SIZE;
			return 0;
		}
		pr_warn_once("fscrypto: unsupported filenames encryption mode "
			     "%d for inode %lu\n",
			     ci->ci_filename_mode, inode->i_ino);
		return -ENOKEY;
	}

	pr_warn_once("fscrypto: unsupported file type %d for inode %lu\n",
		     (inode->i_mode & S_IFMT), inode->i_ino);
	return -ENOKEY;
}

static void put_crypt_info(struct fscrypt_info *ci)
{
	void *prev;
	void *key;

	if (!ci)
		return;

	/*lint -save -e529 -e438*/
	key = ACCESS_ONCE(ci->ci_key);
	/*lint -restore*/
	/*lint -save -e1072 -e747 -e50*/
	prev = cmpxchg(&ci->ci_key, key, NULL);
	/*lint -restore*/
	if (prev == key && key) {
		memzero_explicit(key, (size_t)FS_MAX_KEY_SIZE);
		kfree(key);
		ci->ci_key_len = 0;
	}
	crypto_free_skcipher(ci->ci_ctfm);
	if (ci->ci_gtfm)
		crypto_free_aead(ci->ci_gtfm);
	kmem_cache_free(fscrypt_info_cachep, ci);
}

static int fscrypt_verify_ctx(struct fscrypt_context *ctx)
{
	if ((u32)(ctx->format) != FS_ENCRYPTION_CONTEXT_FORMAT_V2)
		return -EINVAL;

	if (!fscrypt_valid_contents_enc_mode(
			(u32)(ctx->contents_encryption_mode)))
		return -EINVAL;

	if (!fscrypt_valid_filenames_enc_mode(
			(u32)(ctx->filenames_encryption_mode)))
		return -EINVAL;

	if ((u32)(ctx->flags) & ~FS_POLICY_FLAGS_VALID)
		return -EINVAL;

	return 0;
}

/*
 * When we cannot determine if original or backup ctx is valid,
 * trust original if @verify is 0, or backup if it is 1.
 */
int fscrypt_get_verify_context(struct inode *inode, void *ctx, size_t len)
{
	if (!inode->i_sb->s_cop->get_verify_context)
		return 0;

	return inode->i_sb->s_cop->get_verify_context(inode, ctx, len);
}

int fscrypt_set_verify_context(struct inode *inode, const void *ctx,
			size_t len, void *fs_data, int create_crc)
{
	if (!inode->i_sb->s_cop->set_verify_context)
		return 0;

	return inode->i_sb->s_cop->set_verify_context(inode,
				ctx, len, fs_data, create_crc);
}

int fscrypt_get_encryption_info(struct inode *inode)
{
	struct fscrypt_info *crypt_info;
	struct fscrypt_context ctx;
	struct crypto_skcipher *ctfm;
	const char *cipher_str;
	int keysize;
	u8 *raw_key = NULL;
	int res;
	int has_crc = 0;
	int verify = 0;

	if (inode->i_crypt_info)
		return 0;

	res = fscrypt_initialize();
	if (res)
		return res;

	if (!inode->i_sb->s_cop->get_context)
		return -EOPNOTSUPP;

	res = inode->i_sb->s_cop->get_context(inode, &ctx, sizeof(ctx), &has_crc);
	if (res < 0) {
		if (!fscrypt_dummy_context_enabled(inode)) {
			verify = fscrypt_get_verify_context(inode, &ctx,
							    sizeof(ctx));
			if (verify < 0)
				inode->i_sb->s_cop->set_encrypted_corrupt(inode);
			return res;
		}
		ctx.format = FS_ENCRYPTION_CONTEXT_FORMAT_V2;
		ctx.contents_encryption_mode = FS_ENCRYPTION_MODE_AES_256_XTS;
		ctx.filenames_encryption_mode = FS_ENCRYPTION_MODE_AES_256_CTS;
		ctx.flags = 0;
	} else if (res != sizeof(ctx)) {
		pr_err("%s: inode %lu incorrect ctx size [%u : %lu]\n",
			inode->i_sb->s_type->name, inode->i_ino, res, sizeof(ctx));
		inode->i_sb->s_cop->set_encrypted_corrupt(inode);
		return -EINVAL;
	}

	if (fscrypt_verify_ctx(&ctx)) {
		pr_err("%s: inode %lu verify ctx failed\n",
			inode->i_sb->s_type->name, inode->i_ino);
		inode->i_sb->s_cop->set_encrypted_corrupt(inode);
		return -EINVAL;
	}

	crypt_info = kmem_cache_alloc(fscrypt_info_cachep, GFP_NOFS);
	if (!crypt_info)
		return -ENOMEM;

	crypt_info->ci_flags = ctx.flags;
	crypt_info->ci_data_mode = ctx.contents_encryption_mode;
	crypt_info->ci_filename_mode = ctx.filenames_encryption_mode;
	crypt_info->ci_ctfm = NULL;
	crypt_info->ci_gtfm = NULL;
	crypt_info->ci_key = NULL;
	crypt_info->ci_key_len = 0;
	memcpy(crypt_info->ci_master_key, ctx.master_key_descriptor,
				sizeof(crypt_info->ci_master_key));

	res = determine_cipher_type(crypt_info, inode, &cipher_str, &keysize);
	if (res)
		goto out;

	/*
	 * This cannot be a stack buffer because it is passed to the scatterlist
	 * crypto API as part of key derivation.
	 */
	res = -ENOMEM;
	raw_key = kmalloc(FS_MAX_KEY_SIZE, GFP_NOFS);
	if (!raw_key)
		goto out;

	if (fscrypt_dummy_context_enabled(inode)) {
		memset(raw_key, 0x42, FS_AES_256_XTS_KEY_SIZE);
		goto got_key;
	}

	res = validate_user_key(crypt_info, &ctx, raw_key,
			FS_KEY_DESC_PREFIX, FS_KEY_DESC_PREFIX_SIZE);
	if (res && inode->i_sb->s_cop->key_prefix) {
		u8 *prefix = NULL;
		int prefix_size, res2;

		prefix_size = inode->i_sb->s_cop->key_prefix(inode, &prefix);
		res2 = validate_user_key(crypt_info, &ctx, raw_key,
							prefix, prefix_size);
		if (res2) {
			verify = fscrypt_get_verify_context(inode, &ctx,
							    sizeof(ctx));
			if (verify < 0 || res2 == -EBADMSG || res == -EBADMSG)
				inode->i_sb->s_cop->set_encrypted_corrupt(inode);
			if (res2 == -ENOKEY)
				res = -ENOKEY;
			goto out;
		}
	} else if (res) {
		verify = fscrypt_get_verify_context(inode, &ctx, sizeof(ctx));
		if (verify < 0 || res == -EBADMSG)
			inode->i_sb->s_cop->set_encrypted_corrupt(inode);
		goto out;
	}
got_key:
	ctfm = crypto_alloc_skcipher(cipher_str, 0, 0);
	if (!ctfm || IS_ERR(ctfm)) {
		res = ctfm ? PTR_ERR(ctfm) : -ENOMEM;
		printk(KERN_DEBUG
		       "%s: error %d (inode %u) allocating crypto tfm\n",
		       __func__, res, (unsigned) inode->i_ino);
		goto out;
	}
	crypt_info->ci_ctfm = ctfm;
	crypto_skcipher_clear_flags(ctfm, ~0);
	crypto_skcipher_set_flags(ctfm, CRYPTO_TFM_REQ_WEAK_KEY);
	res = crypto_skcipher_setkey(ctfm, raw_key, keysize);
	if (res)
		goto out;

	if (S_ISREG(inode->i_mode) &&
			inode->i_sb->s_cop->is_inline_encrypted &&
			inode->i_sb->s_cop->is_inline_encrypted(inode)) {
		crypt_info->ci_key = kzalloc((size_t)FS_MAX_KEY_SIZE, GFP_NOFS);
		if (!crypt_info->ci_key) {
			res = -ENOMEM;
			goto out;
		}
		crypt_info->ci_key_len = keysize;
		/*lint -save -e732 -e747*/
		memcpy(crypt_info->ci_key, raw_key, crypt_info->ci_key_len);
		/*lint -restore*/
	}

	if (cmpxchg(&inode->i_crypt_info, NULL, crypt_info) == NULL)
		crypt_info = NULL;
	fscrypt_set_verify_context(inode, &ctx, sizeof(ctx), NULL, !has_crc);

out:
	if (res == -ENOKEY)
		res = 0;
	put_crypt_info(crypt_info);
	kzfree(raw_key);
	return res;
}
EXPORT_SYMBOL(fscrypt_get_encryption_info);

void fscrypt_put_encryption_info(struct inode *inode, struct fscrypt_info *ci)
{
	struct fscrypt_info *prev;

	if (ci == NULL)
		ci = ACCESS_ONCE(inode->i_crypt_info);
	if (ci == NULL)
		return;

	prev = cmpxchg(&inode->i_crypt_info, ci, NULL);
	if (prev != ci)
		return;

	put_crypt_info(ci);
}
EXPORT_SYMBOL(fscrypt_put_encryption_info);
