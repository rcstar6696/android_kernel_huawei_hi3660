#undef TRACE_SYSTEM
#define TRACE_SYSTEM sdcardfs

#if !defined(__SDCARDFS_TRACE_EVENTS_H) || defined(TRACE_HEADER_MULTI_READ)
#define __SDCARDFS_TRACE_EVENTS_H

#include <linux/tracepoint.h>

TRACE_EVENT(sdcardfs_ialloc,
	TP_PROTO(struct inode *inode),

	TP_ARGS(inode),

	TP_STRUCT__entry(
		__field(struct inode *,    inode)
		__field(ino_t,              ino)
		__field(mode_t,             mode)
	),

	TP_fast_assign(
		__entry->inode	= inode;
		__entry->ino	= inode->i_ino;
		__entry->mode	= inode->i_mode;
	),

	TP_printk("(inode 0x%p) ino %lu mode 0%o",
		__entry->inode, (unsigned long) __entry->ino, __entry->mode)
);

TRACE_EVENT(sdcardfs_lookup,
	TP_PROTO(struct inode *dir,
		struct dentry *dentry,
		unsigned int flags),
	TP_ARGS(dir, dentry, flags),

	TP_STRUCT__entry(
		__field(struct inode *,    dir)
		__field(struct dentry *,   dentry)
		__field(unsigned int,      flags)
		__string(name,              dentry->d_name.name)
	),

	TP_fast_assign(
		__entry->dir = dir;
		__entry->dentry = dentry;
		__entry->flags = flags;
		__assign_str(name, dentry->d_name.name);
	),

	TP_printk("(dentry 0x%p) name %s dir 0x%p flags %x",
		__entry->dentry, __get_str(name),
		__entry->dir, __entry->flags)
);

#define trace_sdcardfs_d_delete_enter(a)
#define trace_sdcardfs_d_delete_exit(a, b)

#define trace_sdcardfs_d_revalidate_fast_enter(a, b)
#define trace_sdcardfs_d_revalidate_fast_refwalk(a, b)
#define trace_sdcardfs_d_revalidate_fast_exit(a, b, c)

#define trace_sdcardfs_d_revalidate_slow_enter(a, b)
#define trace_sdcardfs_d_revalidate_slow_miss(a, b)
#define trace_sdcardfs_d_revalidate_slow_exit(a, b, c)

#define trace_sdcardfs_create_enter(a, b, c, d)
#define trace_sdcardfs_create_exit(a, b, c, d, e)

#define trace_sdcardfs_mkdir_enter(a, b, c)
#define trace_sdcardfs_mkdir_exit(a, b, c, d)

#define trace_sdcardfs_unlink_enter(a, b)
#define trace_sdcardfs_unlink_exit(a, b, c)

#define trace_sdcardfs_rmdir_enter(a, b)
#define trace_sdcardfs_rmdir_exit(a, b, c)

#define trace_sdcardfs_rename_enter(a, b, c, d)
#define trace_sdcardfs_rename_exit(a, b, c, d, e)

#endif
/***** NOTICE! The #if protection ends here. *****/

#undef TRACE_INCLUDE_PATH
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_PATH .

#define TRACE_INCLUDE_FILE trace-events

/* This part must be outside protection */
#include <trace/define_trace.h>
