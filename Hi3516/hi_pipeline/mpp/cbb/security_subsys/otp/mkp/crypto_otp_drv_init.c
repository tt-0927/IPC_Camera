/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include <asm/io.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/slab.h>

#include <securec.h>

#include "ot_osal.h"
#include "mm_ext.h"
#include "crypto_common_macro.h"
#include "crypto_drv_common.h"
#include "init_otp.h"

#ifdef MODULE
static td_void *g_otp_reg_virt = TD_NULL;
static td_void *g_ca_misc_reg_virt = TD_NULL;

td_u32 crypto_ex_reg_read(reg_region_e region, td_u32 offset)
{
    td_u32 value;
    if (region == REG_REGION_CA_MISC) {
        value = crypto_reg_read((volatile td_void *)(g_ca_misc_reg_virt + offset));
    } else {
        value = crypto_reg_read((volatile td_void *)(g_otp_reg_virt + offset));
#if defined(CRYPTO_REG_DEBUG)
        crypto_print("OTP read 0x%x from 0x%x\n", value, OTPC_BASE_ADDR + offset);
#endif
    }
    return value;
}

td_void crypto_ex_reg_write(reg_region_e region, td_u32 offset, td_u32 value)
{
    if (region == REG_REGION_CA_MISC) {
        crypto_reg_write((volatile td_void *)(g_ca_misc_reg_virt + offset), value);
        return;
    }
    crypto_reg_write((volatile td_void *)(g_otp_reg_virt + offset), value);
#if defined(CRYPTO_REG_DEBUG)
    crypto_print("write 0x%x to 0x%x\n", value, OTPC_BASE_ADDR + offset);
#endif
}
#else
static int crypto_reg_map(td_void)
{
    td_s32 ret;

    /* CS MISC Region. */
    ret = crypto_register_reg_map_region(REG_REGION_CA_MISC);
    crypto_chk_return(ret != TD_SUCCESS, ret, "crypto_register_reg_map_region CA_MISC failed, ret is 0x%x\n", ret);

    /* OTPC Region. */
    ret = crypto_register_reg_map_region(REG_REGION_OTPC);
    crypto_chk_goto(ret != TD_SUCCESS, error_ca_misc_unmap, "crypto_register_reg_map_region OTPC failed,  \
        ret is 0x%x\n", ret);

    return TD_SUCCESS;
error_ca_misc_unmap:
    crypto_unregister_reg_map_region(REG_REGION_CA_MISC);
    return TD_FAILURE;
}

static void crypto_reg_unmap(td_void)
{
    crypto_unregister_reg_map_region(REG_REGION_OTPC);
    crypto_unregister_reg_map_region(REG_REGION_CA_MISC);
}
#endif

static int ot_otp_probe(struct platform_device *pdev)
{
    td_s32 ret = TD_SUCCESS;

#ifdef MODULE
    g_otp_reg_virt = crypto_ioremap_nocache(OTPC_BASE_ADDR, OTPC_ADDR_SIZE);
    if (g_otp_reg_virt == TD_NULL) {
        crypto_log_err("crypto_ioremap_nocache failed for otp_reg\n");
        return TD_FAILURE;
    }

    g_ca_misc_reg_virt = crypto_ioremap_nocache(CA_MISC_REG_BASE_ADDR, CA_MISC_REG_SIZE);
    if (g_ca_misc_reg_virt == TD_NULL) {
        crypto_log_err("crypto_ioremap_nocache failed for ca_misc_reg\n");
        crypto_iounmap(g_otp_reg_virt, OTPC_ADDR_SIZE);
        return TD_FAILURE;
    }
#else
    ret = crypto_reg_map();
    crypto_chk_return(ret != TD_SUCCESS, TD_FAILURE, "crypto_reg_map failed, ret is 0x%x\n", ret);
#endif

    ret = crypto_otp_init();
    crypto_chk_goto(ret != TD_SUCCESS, reg_unmap_error, "crypto_otp_init failed, ret is 0x%x\n", ret);

    crypto_print("load otp.ko success!\n");

    return TD_SUCCESS;

reg_unmap_error:
#ifdef MODULE
    crypto_iounmap(g_ca_misc_reg_virt, CA_MISC_REG_SIZE);
    crypto_iounmap(g_otp_reg_virt, OTPC_ADDR_SIZE);
#else
    crypto_reg_unmap();
#endif
    return TD_FAILURE;
}

static int ot_otp_remove(struct platform_device *pdev)
{
    crypto_unused(pdev);
    crypto_otp_deinit();
#ifdef MODULE
    crypto_iounmap(g_ca_misc_reg_virt, CA_MISC_REG_SIZE);
    crypto_iounmap(g_otp_reg_virt, OTPC_ADDR_SIZE);
#else
    crypto_reg_unmap();
#endif
    crypto_print("unload otp.ko success!\n");
    return TD_SUCCESS;
}

#ifdef OT_PLATFORM_REG
static const struct of_device_id g_ot_otp_match[] = {
    { .compatible = "vendor,otp" },
    { },
};
MODULE_DEVICE_TABLE(of, g_ot_otp_match);

static struct platform_driver g_ot_otp_driver = {
    .probe          = ot_otp_probe,
    .remove         = ot_otp_remove,
    .driver         = {
        .name   = "ot_otp",
        .of_match_table = g_ot_otp_match,
    },
};

osal_module_platform_driver(g_ot_otp_driver);
#else

static int __init otp_init(void)
{
    return ot_otp_probe(NULL);
}

static void __exit otp_exit(void)
{
    ot_otp_remove(NULL);
}

module_init(otp_init);
module_exit(otp_exit);
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
#endif