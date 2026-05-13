/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include "crypto_drv_init.h"

#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <securec.h>

#include "ot_osal.h"
#include "ot_common.h"

#include "mm_ext.h"

#include "crypto_common_macro.h"
#include "crypto_pke_common.h"
#include "crypto_dispatch.h"
#include "kapi_inner.h"
#include "kapi_hash.h"
#include "kapi_symc.h"
#include "drv_symc.h"
#include "crypto_drv_irq.h"
#if defined(CONFIG_SYMC_HARDEN_INTERFACE_SUPPORT)
#include "kapi_symc_impl.h"
#endif
#if defined(CONFIG_HASH_HARDEN_INTERFACE_SUPPORT)
#include "kapi_hash_impl.h"
#endif

#if defined(CRYPTO_UT_TEST_ENABLE)
#include "hal_test.h"
#include "drv_test.h"
#include "kapi_test.h"
#endif

#define CRYPTO_MAX_MEMORY_POOL_NUM  100
#define CRYPTO_DEV_CIPHER_NAME  "soc_cipher"

td_s32 g_spacc_tee_irq_num;
td_s32 g_spacc_ree_irq_num;
td_s32 g_pke_tee_irq_num;
td_s32 g_pke_ree_irq_num;

#ifdef CONFIG_SYMC_PROC_SUPPORT
static const char *alg_str[] = { "TDES", "AES", "SM4", "LEA", "DMA" };
static const char *mode_str[] = { "ECB", "CBC", "CTR", "OFB", "CFB", "CCM", "GCM",
    "CBC-MAC", "CMAC"};
static const char *key_len_str[] = { "64", "128", "192", "256" };
#endif

static crypto_mutex g_mem_pool_lock;

static osal_dev g_cipher_dev = {{0}};
static crypto_cpu_type g_cpu_type = CRYPTO_CPU_TYPE_INVALID;

/* ****** proc function begin ******* */
#ifdef CONFIG_SYMC_PROC_SUPPORT
static td_void hex2str(td_u8 *buf, td_u32 buf_len)
{
    td_s32 i;

    if (buf == TD_NULL) {
        return;
    }

    for (i = 0; i < buf_len; i++) {
        if (buf[i] <= 9) { /* 0~9 */
            buf[i] = buf[i] + '0';
        } else {
            buf[i] = (buf[i] - 0x0A) + 'A'; /* 0x0A~0x0F */
        }
    }

    return;
}

td_s32 symc_proc_read(osal_proc_entry *s)
{
    td_u32 i;
    crypto_symc_proc_info proc_symc_info = {0};

    if (s == TD_NULL) {
        crypto_log_err("Invalid pointer!\n");
        return TD_FAILURE;
    }

    osal_seq_printf(s->seqfile, "\n[CIPHER] Version: [%s], Build Time[%s, %s]\n\n", OT_MPP_VERSION, __DATE__, __TIME__);
    osal_seq_printf(s->seqfile, "\n---------------------------------------- cipher status ---------------------------"
        "--------------------------------------\n");
    osal_seq_printf(s->seqfile, "chn_id   status   decrypt   alg   mode   key_len     addr in/out    key_from   "
        "int_raw  int_en   int_status   iv_out\n");

    for (i = 0; i < CONFIG_SYMC_HARD_CHN_CNT; i++) {
        if (((0x1 << i) & CONFIG_SYMC_HARD_CHANNEL_MASK) == 0x00) {
            continue;
        }
        (td_void)memset_s(&proc_symc_info, sizeof(proc_symc_info), 0, sizeof(proc_symc_info));
        drv_cipher_symc_get_proc_info(i, &proc_symc_info);
        hex2str(proc_symc_info.iv, CRYPTO_AES_IV_SIZE + CRYPTO_AES_IV_SIZE);

        osal_seq_printf(s->seqfile, "  %02u     %s      %s     %s  %s      %s    %08x/%08x    "
                        "%s          %u        %u         %u       %s\n",
                        i,
                        proc_symc_info.open == 0 ? "close" : " open ",      proc_symc_info.is_decrypt ? "DEC" : "ENC",
                        alg_str[proc_symc_info.alg],                        mode_str[proc_symc_info.mode],
                        key_len_str[proc_symc_info.key_len],                proc_symc_info.in_node_head,
                        proc_symc_info.out_node_head,                       proc_symc_info.key_source ? "HW" : "SW",
                        proc_symc_info.int_raw,                             proc_symc_info.int_en,
                        proc_symc_info.int_status,                          proc_symc_info.iv);
    }

    return TD_SUCCESS;
}

static td_u32 symc_proc_init(td_void)
{
    osal_proc_entry *proc_entry = TD_NULL;

    proc_entry = osal_create_proc_entry("cipher", TD_NULL);
    if (proc_entry == TD_NULL) {
        crypto_print("cipher: can't create proc.\n");
        return TD_FAILURE;
    }
    proc_entry->read = symc_proc_read;
    return TD_SUCCESS;
}

static td_void symc_proc_deinit(td_void)
{
    osal_remove_proc_entry("cipher", TD_NULL);
}
#endif /* ****** proc function end ******* */

static int crypto_reg_map(td_void)
{
    td_s32 ret;

    /* SPACC Region. */
    ret = crypto_register_reg_map_region(REG_REGION_SPACC);
    crypto_chk_return(ret != TD_SUCCESS, ret, "crypto_register_reg_map_region failed, ret is 0x%x\n", ret);

    /* CS MISC Region. */
    ret = crypto_register_reg_map_region(REG_REGION_CA_MISC);
    crypto_chk_goto(ret != TD_SUCCESS, error_spacc_unmap, "crypto_register_reg_map_region failed, ret is 0x%x\n", ret);

    /* PKE Region. */
    ret = crypto_register_reg_map_region(REG_REGION_PKE);
    crypto_chk_goto(ret != TD_SUCCESS, error_ca_misc_unmap,
        "crypto_register_reg_map_region failed, ret is 0x%x\n", ret);

    /* TRNG Region. */
    ret = crypto_register_reg_map_region(REG_REGION_TRNG);
    crypto_chk_goto(ret != TD_SUCCESS, error_pke_unmap, "crypto_register_reg_map_region failed, ret is 0x%x\n", ret);

    /* KM Region. */
    ret = crypto_register_reg_map_region(REG_REGION_KM);
    crypto_chk_goto(ret != TD_SUCCESS, error_trng_unmap, "crypto_register_reg_map_region failed, ret is 0x%x\n", ret);

    return TD_SUCCESS;

error_trng_unmap:
    crypto_unregister_reg_map_region(REG_REGION_TRNG);
error_pke_unmap:
    crypto_unregister_reg_map_region(REG_REGION_PKE);
error_ca_misc_unmap:
    crypto_unregister_reg_map_region(REG_REGION_CA_MISC);
error_spacc_unmap:
    crypto_unregister_reg_map_region(REG_REGION_SPACC);
    return TD_FAILURE;
}

static void crypto_reg_unmap(td_void)
{
    crypto_unregister_reg_map_region(REG_REGION_KM);
    crypto_unregister_reg_map_region(REG_REGION_TRNG);
    crypto_unregister_reg_map_region(REG_REGION_PKE);
    crypto_unregister_reg_map_region(REG_REGION_CA_MISC);
    crypto_unregister_reg_map_region(REG_REGION_SPACC);
}

typedef struct crypto_compat_addr {
    td_phys_addr_t phys_addr;
    td_void *virt_addr;
    td_u32 length;
    td_bool is_used;
    const char *name;
} crypto_compat_addr_t;

static crypto_compat_addr_t g_crypto_mem_list[CRYPTO_MAX_MEMORY_POOL_NUM];

td_void *crypto_malloc_coherent(td_u32 size, const td_char *name)
{
    td_u32 i;
    td_s32 ret = TD_SUCCESS;
    crypto_compat_addr_t *compat_addr = TD_NULL;
    mm_malloc_param malloc_param = {0};

    crypto_mutex_lock(&g_mem_pool_lock);
    for (i = 0; i < CRYPTO_MAX_MEMORY_POOL_NUM; i++) {
        if (g_crypto_mem_list[i].is_used == TD_FALSE) {
            compat_addr = &g_crypto_mem_list[i];
            break;
        }
    }
    if (compat_addr == TD_NULL) {
        crypto_print("mem pool is Full!\n");
        goto exit_unlock;
    }
    malloc_param.buf_name = name;
    malloc_param.size = size;
    malloc_param.kernel_only = TD_TRUE;
    ret = cmpi_mmz_malloc_nocache(&malloc_param, &compat_addr->phys_addr, &compat_addr->virt_addr);
    if (ret != TD_SUCCESS) {
        crypto_print("cmpi_mmz_malloc_nocache failed, ret is 0x%x\n", ret);
        goto exit_unlock;
    }
#if defined(CONFIG_MALLOC_DEBUG)
    crypto_print("mmz_malloc virt_addr is 0x%lx, phys_addr is 0x%llx\n", (td_uintptr_t)compat_addr->virt_addr,
        compat_addr->phys_addr);
#endif
    compat_addr->is_used = TD_TRUE;
    compat_addr->length = size;
    compat_addr->name = name;
    crypto_mutex_unlock(&g_mem_pool_lock);
    return compat_addr->virt_addr;
exit_unlock:
    crypto_mutex_unlock(&g_mem_pool_lock);
    return TD_NULL;
}

td_void *crypto_malloc_mmz(td_u32 size, const td_char *name)
{
    return crypto_malloc_coherent(size, name);
}

td_void crypto_free_coherent(const td_void *ptr)
{
    td_u32 i;
    crypto_compat_addr_t *compat_addr = TD_NULL;
    crypto_mutex_lock(&g_mem_pool_lock);
    for (i = 0; i < CRYPTO_MAX_MEMORY_POOL_NUM; i++) {
        if (g_crypto_mem_list[i].virt_addr == ptr && g_crypto_mem_list[i].is_used) {
            compat_addr = &g_crypto_mem_list[i];
            break;
        }
    }
    if (compat_addr == TD_NULL) {
        goto exit_unlock;
    }
#if defined(CONFIG_MALLOC_DEBUG)
    crypto_print("mmz_free virt_addr is 0x%lx, phys_addr is 0x%llx\n", (td_uintptr_t)compat_addr->virt_addr,
        compat_addr->phys_addr);
#endif
    cmpi_mmz_free(compat_addr->phys_addr, compat_addr->virt_addr);
    (td_void)memset_s(compat_addr, sizeof(crypto_compat_addr_t), 0, sizeof(crypto_compat_addr_t));

    compat_addr->is_used = TD_FALSE;
exit_unlock:
    crypto_mutex_unlock(&g_mem_pool_lock);
}

static td_void crypto_mem_pool_release(td_void)
{
    td_u32 i;
    for (i = 0; i < CRYPTO_MAX_MEMORY_POOL_NUM; i++) {
        if (g_crypto_mem_list[i].is_used) {
            crypto_print("mem pool idx[%s] is not free!\n", g_crypto_mem_list[i].name);
            crypto_free_coherent(g_crypto_mem_list[i].virt_addr);
        }
    }
}

td_u64 crypto_get_phys_addr(const td_void *ptr)
{
    td_u32 i;
    crypto_compat_addr_t *compat_addr = TD_NULL;
    for (i = 0; i < CRYPTO_MAX_MEMORY_POOL_NUM; i++) {
        compat_addr = &g_crypto_mem_list[i];
        if (ptr >= compat_addr->virt_addr && ptr < compat_addr->virt_addr + compat_addr->length) {
            return compat_addr->phys_addr + (ptr - compat_addr->virt_addr);
        }
    }
    return 0;
}

static td_s32 crypto_drv_release(td_void *private_data)
{
    kapi_cipher_symc_process_release();
    kapi_cipher_hash_process_release();
    return TD_SUCCESS;
}

static osal_fileops g_cipher_fops = {
    .open   = TD_NULL,
    .write  = TD_NULL,
    .llseek = TD_NULL,
    .release = crypto_drv_release,
    .cmd_list = TD_NULL,
    .cmd_cnt = 0,
};

#define CPU_ID_STAT                     (0x0018)
#define CRYPTO_CPU_ID_SCPU              (0xa5)
#define CRYPTO_CPU_ID_ACPU              (0xaa)

crypto_cpu_type crypto_get_cpu_type(td_void)
{
    td_u32 cpu_id;
    if (g_cpu_type != CRYPTO_CPU_TYPE_INVALID) {
        return g_cpu_type;
    }

    cpu_id = ca_misc_reg_read(CPU_ID_STAT) & 0x00ff;
    if (cpu_id == CRYPTO_CPU_ID_SCPU) {
        g_cpu_type = CRYPTO_CPU_TYPE_SCPU;
    } else if (cpu_id == CRYPTO_CPU_ID_ACPU) {
        g_cpu_type = CRYPTO_CPU_TYPE_ACPU;
    }
    return g_cpu_type;
}

static int ot_cipher_probe(struct platform_device *pdev)
{
    td_s32 ret = TD_SUCCESS;

    ret = crypto_mutex_init(&g_mem_pool_lock);
    crypto_chk_return(ret != TD_SUCCESS, TD_FAILURE, "crypto_mutex_init failed\n");

#ifdef OT_PLATFORM_REG
    g_spacc_tee_irq_num = osal_platform_get_irq_byname(pdev, "spacc_tee");
    g_spacc_ree_irq_num = osal_platform_get_irq_byname(pdev, "spacc_ree");
    g_pke_tee_irq_num = osal_platform_get_irq_byname(pdev, "pke_tee");
    g_pke_ree_irq_num = osal_platform_get_irq_byname(pdev, "pke_ree");
#endif

    ret = crypto_reg_map();
    crypto_chk_goto(ret != TD_SUCCESS, error_destroy_mutex, "crypto_reg_map failed.\n");

    /* Create Device Node. */
    ret = strncpy_s(g_cipher_dev.name, sizeof(g_cipher_dev.name), CRYPTO_DEV_CIPHER_NAME,
        sizeof(CRYPTO_DEV_CIPHER_NAME));
    crypto_chk_goto(ret != EOK, error_unmap_reg, "strncpy failed. ret(%d).\n", ret);

    g_cipher_fops.cmd_list = crypto_get_ioctl_cmd_list();
    g_cipher_fops.cmd_cnt = crypto_get_ioctl_cmd_cnt();
    g_cipher_dev.fops = &g_cipher_fops;

    ret = osal_dev_register(&g_cipher_dev);
    crypto_chk_goto(ret != TD_SUCCESS, error_unmap_reg, "osal_dev_register failed. ret is 0x%x.\n", ret);

    /* KAPI Init. */
    ret = kapi_env_init();
    crypto_chk_goto(ret != TD_SUCCESS, error_unregister_dev, "kapi_env_init failed. ret is 0x%x.\n", ret);

#ifdef CONFIG_SYMC_PROC_SUPPORT
    ret = symc_proc_init();
    if (ret != TD_SUCCESS) {
        kapi_env_deinit();
        crypto_print("symc_proc_init failed\n");
        goto error_unregister_dev;
    }
#endif

#if defined(CRYPTO_OS_INT_SUPPORT)
    ret = crypto_irq_init();
    if (ret != TD_SUCCESS) {
        symc_proc_deinit();
        kapi_env_deinit();
        crypto_print("crypto_irq_init failed\n");
        goto error_unregister_dev;
    }
#endif

#if defined(CONFIG_SYMC_HARDEN_INTERFACE_SUPPORT)
    kapi_symc_init_impl();
#endif
#if defined(CONFIG_HASH_HARDEN_INTERFACE_SUPPORT)
    kapi_hash_init_impl();
#endif
    crypto_curve_param_init();

    crypto_print("load cipher.ko success!\n");
    return TD_SUCCESS;

error_unregister_dev:
    osal_dev_unregister(&g_cipher_dev);
error_unmap_reg:
    crypto_reg_unmap();
error_destroy_mutex:
    crypto_mutex_destroy(&g_mem_pool_lock);
    crypto_print("load cipher.ko failed!\n");
    return TD_FAILURE;
}

static int ot_cipher_remove(struct platform_device *pdev)
{
    /* ****** proc function begin ******* */
#ifdef CONFIG_SYMC_PROC_SUPPORT
    symc_proc_deinit();
#endif /* ****** proc function end ******* */
#if defined(CONFIG_HASH_HARDEN_INTERFACE_SUPPORT)
    kapi_hash_deinit_impl();
#endif
#if defined(CONFIG_SYMC_HARDEN_INTERFACE_SUPPORT)
    kapi_symc_destroy_impl();
#endif
    kapi_env_deinit();
#if defined(CRYPTO_OS_INT_SUPPORT)
    crypto_irq_deinit();
#endif
    crypto_unused(pdev);
    osal_dev_unregister(&g_cipher_dev);
    crypto_reg_unmap();
    crypto_mem_pool_release();
    crypto_mutex_destroy(&g_mem_pool_lock);
    crypto_print("unload cipher.ko success!\n");
    return TD_SUCCESS;
}

#ifdef OT_PLATFORM_REG
static const struct of_device_id g_ot_cipher_match[] = {
    { .compatible = "vendor,cipher" },
    { },
};
MODULE_DEVICE_TABLE(of, g_ot_cipher_match);

static struct platform_driver g_ot_cipher_driver = {
    .probe          = ot_cipher_probe,
    .remove         = ot_cipher_remove,
    .driver         = {
        .name   = "ot_cipher",
        .of_match_table = g_ot_cipher_match,
    },
};

osal_module_platform_driver(g_ot_cipher_driver);
MODULE_LICENSE("GPL");
#else

static int __init crypto_init(void)
{
    return ot_cipher_probe(NULL);
}

static void __exit crypto_exit(void)
{
    ot_cipher_remove(NULL);
}

module_init(crypto_init);
module_exit(crypto_exit);
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
#endif