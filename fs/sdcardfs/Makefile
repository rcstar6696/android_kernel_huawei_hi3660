SDCARDFS_VERSION="1.0"

ccflags-y += -DSDCARDFS_VERSION=\"$(SDCARDFS_VERSION)\" -DSDCARDFS_CASE_INSENSITIVE -DSDCARDFS_SUPPORT_RESERVED_SPACE \
	-DSDCARDFS_UNDERLAY_MULTI_ALIASES -DSDCARDFS_SYSFS_FEATURE -DSDCARDFS_PLUGIN_PRIVACY_SPACE -g -Wall

obj-$(CONFIG_SDCARD_FS) += sdcardfs.o
sdcardfs-objs := dentry.o file.o inode.o configfs.o lookup_ci.o main.o super.o namei.o \
	mmap.o packagelist.o derived_perm.o tree.o xattr.o sysfs.o sdcardd.o

