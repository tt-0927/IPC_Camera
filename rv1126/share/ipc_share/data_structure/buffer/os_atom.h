

#ifndef _OSA_BASE_CORE_SOURCE_ATOM_INCLUDE_
#define _OSA_BASE_CORE_SOURCE_ATOM_INCLUDE_


typedef          char      os_atomic_char;
typedef   signed char      os_atomic_schar;
typedef unsigned char      os_atomic_uchar;
typedef          short     os_atomic_short;
typedef unsigned short     os_atomic_ushort;
typedef          int       os_atomic_int;
typedef unsigned int       os_atomic_uint;
typedef          long      os_atomic_long;
typedef unsigned long      os_atomic_ulong;
typedef          long long os_atomic_llong;
typedef unsigned long long os_atomic_ullong;


/*
 * 编译器版本
 */
/* gcc version. for example : v4.1.2 is 40102, v3.4.6 is 30406 */
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

/*
 *逻辑跳转优化
 */
#if GCC_VERSION
/*条件大多数为真，与if配合使用，直接执行if中语句*/
  #define likely(x)     __builtin_expect(!!(x), 1)
/*条件大多数为假，与if配合使用，直接执行else中语句*/
  #define unlikely(x)   __builtin_expect(!!(x), 0)
#else
  #define likely(x)     (!!(x))
  #define unlikely(x)   (!!(x))
#endif

/*
 * intel x86 平台
 */
#if (__i386__ || __i386 || __amd64__ || __amd64)
  #ifndef __X86__
    #define __X86__
  #endif
#endif

#ifndef _cpu_pause
  #if defined(__X86__) || defined(__GNUC__)
    #define _cpu_pause()        __asm__("pause")
  #else
    #define _cpu_pause()        ((void)0)
  #endif
#endif

#if (GCC_VERSION >= 40100)

/* 内存访问栅 */
  #define barrier()                     (__sync_synchronize())

/* 原子获取 */
  #define AO_GET(ptr)                   ({ __typeof__(*(ptr)) volatile *_val = (ptr); barrier(); (*_val); })

/* 原子设置 */
  #define AO_SET(ptr, value)            ((void)__sync_lock_test_and_set((ptr), (value)))

/* 原子交换，如果被设置，则返回旧值，否则返回设置值 */
  #define AO_SWAP(ptr, value)           ((__typeof__(*(ptr)))__sync_lock_test_and_set((ptr), (value)))

/* 原子比较交换，如果当前值等于旧值，则新值被设置，返回旧值，否则返回新值*/
  #define AO_CAS(ptr, comp, value)      ((__typeof__(*(ptr)))__sync_val_compare_and_swap((ptr), (comp), (value)))

/* 原子比较交换，如果当前值等于旧指，则新值被设置，返回真值，否则返回假 */
  #define AO_CASB(ptr, comp, value)     (__sync_bool_compare_and_swap((ptr), (comp), (value)) != 0 ? 1 : 0)

/* 原子清零 */
  #define AO_CLEAR(ptr)                 ((void)__sync_lock_release((ptr)))

/* 通过值与旧值进行算术与位操作，返回新值 */
  #define AO_ADD_F(ptr, value)          ((__typeof__(*(ptr)))__sync_add_and_fetch((ptr), (value)))
  #define AO_SUB_F(ptr, value)          ((__typeof__(*(ptr)))__sync_sub_and_fetch((ptr), (value)))
  #define AO_OR_F(ptr, value)           ((__typeof__(*(ptr)))__sync_or_and_fetch((ptr), (value)))
  #define AO_AND_F(ptr, value)          ((__typeof__(*(ptr)))__sync_and_and_fetch((ptr), (value)))
  #define AO_XOR_F(ptr, value)          ((__typeof__(*(ptr)))__sync_xor_and_fetch((ptr), (value)))
/* 通过值与旧值进行算术与位操作，返回旧值 */
  #define AO_F_ADD(ptr, value)          ((__typeof__(*(ptr)))__sync_fetch_and_add((ptr), (value)))
  #define AO_F_SUB(ptr, value)          ((__typeof__(*(ptr)))__sync_fetch_and_sub((ptr), (value)))
  #define AO_F_OR(ptr, value)           ((__typeof__(*(ptr)))__sync_fetch_and_or((ptr), (value)))
  #define AO_F_AND(ptr, value)          ((__typeof__(*(ptr)))__sync_fetch_and_and((ptr), (value)))
  #define AO_F_XOR(ptr, value)          ((__typeof__(*(ptr)))__sync_fetch_and_xor((ptr), (value)))

#else

  //#error "can not supported atomic operation by gcc(v4.0.0+) buildin function."
#endif  /* if (GCC_VERSION >= 40100) */

/* -------------------                  */


/* if (GCC_VERSION >= 40100) */
/* 忽略返回值，算术和位操作 */
#define AO_INC(ptr)                 ((void)AO_ADD_F((ptr), 1))
#define AO_DEC(ptr)                 ((void)AO_SUB_F((ptr), 1))
#define AO_ADD(ptr, val)            ((void)AO_ADD_F((ptr), (val)))
#define AO_SUB(ptr, val)            ((void)AO_SUB_F((ptr), (val)))
#define AO_OR(ptr, val)			 ((void)AO_OR_F((ptr), (val)))
#define AO_AND(ptr, val)			((void)AO_AND_F((ptr), (val)))
#define AO_XOR(ptr, val)			((void)AO_XOR_F((ptr), (val)))
/* 通过掩码，设置某个位为1，并返还新的值 */
#define AO_BIT_ON(ptr, mask)        AO_OR_F((ptr), (mask))
/* 通过掩码，设置某个位为0，并返还新的值 */
#define AO_BIT_OFF(ptr, mask)       AO_AND_F((ptr), ~(mask))
/* 通过掩码，交换某个位，1变0，0变1，并返还新的值 */
#define AO_BIT_XCHG(ptr, mask)      AO_XOR_F((ptr), (mask))



/* 下面的接口是先运算，再返回新的值 */
#define os_atomic_add_fetch(ptr,val) \
		((__typeof__(*(ptr)))__sync_add_and_fetch((ptr), (val)))

#define os_atomic_sub_fetch(ptr,val) \
		((__typeof__(*(ptr)))__sync_sub_and_fetch((ptr), (val)))

#define os_atomic_or_fetch(ptr,val) \
		((__typeof__(*(ptr)))__sync_or_and_fetch((ptr), (val)))

#define os_atomic_and_fetch(ptr,val) \
		((__typeof__(*(ptr)))__sync_and_and_fetch((ptr), (val)))

#define os_atomic_xor_fetch(ptr,val) \
		((__typeof__(*(ptr)))__sync_xor_and_fetch((ptr), (val)))


/* 下面的接口是返回旧值，再运算 */
#define os_atomic_fetch_add(ptr,val) \
		((__typeof__(*(ptr)))__sync_fetch_and_add((ptr), (val)))

#define os_atomic_fetch_sub(ptr,val) \
		((__typeof__(*(ptr)))__sync_fetch_and_sub((ptr), (val)))

#define os_atomic_fetch_or(ptr,val) \
		((__typeof__(*(ptr)))__sync_fetch_and_or((ptr), (val)))

#define os_atomic_fetch_and(ptr,val) \
		((__typeof__(*(ptr)))__sync_fetch_and_and((ptr), (val)))

#define os_atomic_fetch_xor(ptr,val) \
		((__typeof__(*(ptr)))__sync_fetch_and_xor((ptr), (val)))






#endif //_OSA_BASE_CORE_SOURCE_ATOM_INCLUDE_

