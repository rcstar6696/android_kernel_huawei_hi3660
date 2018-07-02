#ifndef __LINUX_MACROS_H
#define __LINUX_MACROS_H

#ifndef d_inode
#define d_inode(x)  ((x)->d_inode)
#endif

#ifndef inode_lock_nested
#define inode_lock_nested(x, y) mutex_lock_nested(&(x)->i_mutex, (y))
#endif

#ifndef inode_lock
#define inode_lock(x) mutex_lock(&(x)->i_mutex)
#endif

#ifndef inode_unlock
#define inode_unlock(x) mutex_unlock(&(x)->i_mutex)
#endif

#ifndef lockless_dereference
/**
 * lockless_dereference() - safely load a pointer for later dereference
 * @p: The pointer to load
 *
 * Similar to rcu_dereference(), but for situations where the pointed-to
 * object's lifetime is managed by something other than RCU.  That
 * "something other" might be reference counting or simple immortality.
 *
 * The seemingly unused variable ___typecheck_p validates that @p is
 * indeed a pointer type by using a pointer to typeof(*p) as the type.
 * Taking a pointer to typeof(*p) again is needed in case p is void *.
 */
#define lockless_dereference(p) \
({ \
	typeof(p) _________p1 = ACCESS_ONCE(p); \
	typeof(*(p)) *___typecheck_p __maybe_unused; \
	smp_read_barrier_depends(); /* Dependency order vs. p above. */ \
	(_________p1); \
})
#endif

#ifndef d_really_is_negative
#define d_really_is_negative(dentry) ((dentry)->d_inode == NULL)
#endif

#endif
