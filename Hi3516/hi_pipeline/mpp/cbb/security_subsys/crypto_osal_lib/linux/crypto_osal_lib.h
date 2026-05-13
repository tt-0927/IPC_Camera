/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#ifndef CRYPTO_OSAL_LIB_H
#define CRYPTO_OSAL_LIB_H

#include "ot_osal.h"
#include "crypto_type.h"
#include "crypto_platform.h"
#include "crypto_common_struct.h"
#include "crypto_security.h"
#include "mm_ext.h"
#include "crypto_common_macro.h"

#include <securec.h>

/* Timer. */
#define crypto_msleep           osal_msleep
#define crypto_udelay           osal_udelay

/* map. */
#define crypto_ioremap_nocache  osal_ioremap_nocache
#define crypto_iounmap          osal_iounmap

#define crypto_cache_all(...)
#define crypto_cache_flush(...)

/* Mutex. */
#define crypto_mutex            osal_mutex
#define crypto_mutex_init       osal_mutex_init
#define crypto_mutex_destroy    osal_mutex_destroy
#define crypto_mutex_lock       osal_mutex_lock
#define crypto_mutex_unlock     osal_mutex_unlock

/* Log. */
#if defined(CONFIG_CRYPTO_LOG_DISABLE)
#define crypto_print(...)
#else
#define crypto_print            osal_printk
#endif

/* Malloc. */
#define crypto_malloc(x)        (((x) > 0) ? osal_kmalloc((x), OSAL_GFP_KERNEL) : TD_NULL)
#define crypto_free(x)          {if (((x) != TD_NULL)) osal_kfree(x);}

/* ioctl_cmd. */
#define crypto_ioctl_cmd        osal_ioctl_cmd

/* thread. */
#define crypto_getpid()                         osal_get_current_tgid()
#define crypto_gettid()                         osal_get_current_tid()

/* memory. */
td_void *crypto_malloc_coherent(td_u32 size, const td_char *name);

td_void *crypto_malloc_mmz(td_u32 size, const td_char *name);

td_void crypto_free_coherent(const td_void *ptr);

td_u64 crypto_get_phys_addr(const td_void *ptr);

#define crypto_symc_support(...)            TD_TRUE
#define crypto_rsa_support(...)             TD_TRUE
#define crypto_sm3_support(...)             TD_TRUE
td_bool crypto_data_buf_check(const crypto_buf_attr *buf_attr, td_u32 length);
crypto_cpu_type crypto_get_cpu_type(td_void);

/* Crypto Owner Operation. */
#define crypto_owner                        td_u32
#define crypto_owner_dump(owner)            crypto_log_dbg("Owner's pid is 0x%x\n", owner)
static inline td_s32 crypto_get_owner(crypto_owner *owner)
{
    if (owner == TD_NULL) {
        return CRYPTO_FAILURE;
    }
    *owner = crypto_getpid();
    return CRYPTO_SUCCESS;
}

unsigned long crypto_osal_get_phys_addr(const crypto_buf_attr *buf);
#define crypto_osal_put_phys_addr(...)
#define crypto_osal_mem_handle_get(fd, module_id)       TD_SUCCESS
#define crypto_osal_mem_handle_put(...)


/* Register Read&Write. */
typedef enum {
    REG_REGION_SPACC = 0x0,
    REG_REGION_TRNG,
    REG_REGION_PKE,
    REG_REGION_CA_MISC,
    REG_REGION_KM,
    REG_REGION_OTPC,
    REG_REGION_NUM
} reg_region_e;

td_s32 crypto_get_random(td_u32 *randnum);

td_s32 crypto_register_reg_map_region(reg_region_e region);

void crypto_unregister_reg_map_region(reg_region_e region);

td_u32 crypto_ex_reg_read(reg_region_e region, td_u32 offset);

td_void crypto_ex_reg_write(reg_region_e region, td_u32 offset, td_u32 value);

#define spacc_reg_read(offset)          crypto_ex_reg_read(REG_REGION_SPACC, offset)
#define spacc_reg_write(offset, value)  crypto_ex_reg_write(REG_REGION_SPACC, offset, value)

#define trng_reg_read(offset)           crypto_ex_reg_read(REG_REGION_TRNG, offset)
#define trng_reg_write(offset, value)   crypto_ex_reg_write(REG_REGION_TRNG, offset, value)

#define pke_reg_read(offset)            crypto_ex_reg_read(REG_REGION_PKE, offset)
#define pke_reg_write(offset, value)    crypto_ex_reg_write(REG_REGION_PKE, offset, value)

#define km_reg_read(offset)             crypto_ex_reg_read(REG_REGION_KM, offset)
#define km_reg_write(offset, value)     crypto_ex_reg_write(REG_REGION_KM, offset, value)

#define otpc_reg_read(offset)              crypto_ex_reg_read(REG_REGION_OTPC, offset)
#define otpc_reg_write(offset, value)      crypto_ex_reg_write(REG_REGION_OTPC, offset, value)

#define ca_misc_reg_read(offset)             crypto_ex_reg_read(REG_REGION_CA_MISC, offset)
#define ca_misc_reg_write(offset, value)     crypto_ex_reg_write(REG_REGION_CA_MISC, offset, value)

#define CRYPTO_EXPORT_SYMBOL        EXPORT_SYMBOL

typedef enum {
    CRYPTO_OSAL_MMZ_TYPE,
    CRYPTO_OSAL_NSSMMU_TYPE,
    CRYPTO_OSAL_ERROR_TYPE,
} crypto_osal_mem_type;

crypto_osal_mem_type crypto_osal_get_mem_type(td_void *mem_handle);

#define crypto_osal_mem_phys(mem_handle)                        osal_mem_phys(mem_handle)
#define crypto_osal_mem_nssmmu_map(mem_handle)                  osal_mem_nssmmu_map(mem_handle, 0)
#define crypto_osal_mem_nssmmu_unmap(mem_handle, smmu_addr)     osal_mem_nssmmu_unmap(mem_handle, smmu_addr, 0)

#define crypto_memory_barrier() do { osal_mb(); osal_isb(); osal_dsb(); } while (0)

#define CRYPTO_ERROR_ENV                            ERROR_ENV_LINUX

#define crypto_sm4_support()        TD_TRUE

td_s32 crypto_copy_from_user(td_void *to, unsigned long to_len, const td_void *from, unsigned long from_len);
td_s32 crypto_copy_to_user(td_void  *to, unsigned long to_len, const td_void *from, unsigned long from_len);

#endif