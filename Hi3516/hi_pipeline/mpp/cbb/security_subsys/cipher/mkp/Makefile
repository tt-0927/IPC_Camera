#*************************************************************************
ifeq ($(PARAM_FILE), )
    PARAM_FILE:=../../../../../Makefile.param
    include $(PARAM_FILE)
endif

ifeq ($(CBB_PARAM_FILE), )
    CBB_PARAM_FILE:=../../../Makefile.param
    include $(CBB_PARAM_FILE)
endif

ifdef CONFIG_HI3519DV500
    MKP_CFLAGS  += -DCONFIG_HI3519DV500
else
    MKP_CFLAGS  += -DCONFIG_HI3516CV610
endif

SECURITY_SUBSYS_DIR := $(PWD)

# all source file in this module
SRCS:=

# init
SRCS    += crypto_drv_init.c
INIT_FILE   := $(SECURITY_SUBSYS_DIR)/crypto_drv_init.c
INIT_INC	:= $(SECURITY_SUBSYS_DIR)/crypto_drv_init.h

# IRQ
SRCS    += crypto_drv_irq.c

# Common
SRCS    += ../../security_subsys_common/common/crypto_drv_common.c
SRCS    += ../../security_subsys_common/common/crypto_pke_common.c
SRCS    += ../../security_subsys_common/common/crypto_hash_common.c
SRCS    += ../../security_subsys_common/common/ecc_curve_param.c
SRCS    += ../../security_subsys_common/common/curve_param/brainpool_256r1.c
SRCS    += ../../security_subsys_common/common/curve_param/brainpool_384r1.c
SRCS    += ../../security_subsys_common/common/curve_param/brainpool_512r1.c
SRCS    += ../../security_subsys_common/common/curve_param/nist_p256r.c
SRCS    += ../../security_subsys_common/common/curve_param/nist_p384r.c
SRCS    += ../../security_subsys_common/common/curve_param/nist_p521r.c
SRCS    += ../../security_subsys_common/common/curve_param/sm2.c

# crypto_osal
SRCS    += ../../crypto_osal_lib/linux/crypto_osal_lib.c

# DRV
SRCS    += ../../security_subsys_common/drv_code/trng/drv_trng.c
SRCS    += ../../security_subsys_common/drv_code/symc/drv_symc_ccm.c
SRCS    += ../../security_subsys_common/drv_code/symc/drv_symc_ctr_non_align.c
SRCS    += ../../security_subsys_common/drv_code/symc/drv_symc_gcm.c
SRCS    += ../../security_subsys_common/drv_code/symc/drv_symc_get_config.c
SRCS    += ../../security_subsys_common/drv_code/symc/drv_symc_mac.c
# SRCS    += ../../security_subsys_common/drv_code/symc/drv_symc_mac_simple.c
SRCS    += ../../security_subsys_common/drv_code/symc/drv_symc.c
SRCS    += ../../security_subsys_common/drv_code/symc/drv_symc_proc.c

# KAPI
SRCS    += ../../security_subsys_common/kapi_code/kapi_init.c
SRCS    += ../../security_subsys_common/kapi_code/kapi_trng.c
# SRCS    += ../../security_subsys_common/kapi_code/kapi_symc_simple.c
SRCS    += ../../security_subsys_common/kapi_code/kapi_symc.c
SRCS    += ../../security_subsys_common/kapi_code/kapi_symc_mac.c
SRCS    += ../../security_subsys_common/kapi_code/kapi_symc_long_term.c
SRCS    += ../../security_subsys_common/kapi_code/kapi_symc_short_term.c

SRCS    += ../../security_subsys_common/kapi_code/kapi_hash_simple.c
SRCS    += ../../security_subsys_common/kapi_code/harden_support_longterm/kapi_hash_impl.c
SRCS    += ../../security_subsys_common/kapi_code/harden_support_longterm/kapi_symc_impl.c

SRCS    += ../../security_subsys_common/kapi_code/kapi_pke.c
SRCS    += ../../security_subsys_common/kapi_code/kapi_pke_cal.c

# Dispatch
SRCS    += ../../security_subsys_common/dispatch_code/crypto_dispatch.c

# TRNG HAL
SRCS    += ../../security_subsys_common/hal_code/trng_v4/hal_trng.c

# SPACC HAL
SRCS    += ../../security_subsys_common/hal_code/spacc_v4/hal_symc.c
SRCS    += ../../security_subsys_common/hal_code/spacc_v4/hal_symc_proc.c
SRCS    += ../../security_subsys_common/hal_code/spacc_v4/hal_symc_cbc_mac.c
SRCS    += ../../security_subsys_common/hal_code/spacc_v4/hal_symc_ccm.c
SRCS    += ../../security_subsys_common/hal_code/spacc_v4/hal_symc_cmac.c
SRCS    += ../../security_subsys_common/hal_code/spacc_v4/hal_symc_gcm.c

SRCS    += ../../security_subsys_common/hal_code/spacc_v4/hal_hash.c

# PKE HAL
ifdef CONFIG_HI3519DV500
SRCS    += ../../security_subsys_common/hal_code/pke_v4/hal_pke.c
endif
ifdef CONFIG_HI3516CV610
SRCS    += ../../security_subsys_common/hal_code/pke_v5/ecc_data_ram_addr.c
SRCS    += ../../security_subsys_common/hal_code/pke_v5/rsa_data_ram_addr.c
SRCS    += ../../security_subsys_common/hal_code/pke_v5/hal_pke.c
SRCS    += ../../security_subsys_common/hal_code/pke_v5/rom_lib.c
endif

# drv_pke
ifdef CONFIG_HI3519DV500
SRCS    += ../../security_subsys_common/drv_code/pke/v4/drv_pke_ecc_curve.c
SRCS    += ../../security_subsys_common/drv_code/pke/v4/drv_pke_ecc.c
SRCS    += ../../security_subsys_common/drv_code/pke/v4/drv_pke.c
endif

ifdef CONFIG_HI3516CV610
SRCS    += ../../security_subsys_common/drv_code/pke/ecc/drv_pke_ecdsa_check_dot_on_curve.c
SRCS    += ../../security_subsys_common/drv_code/pke/ecc/drv_pke_ecdsa_sign.c
SRCS    += ../../security_subsys_common/drv_code/pke/ecc/drv_pke_sm2_sign.c
SRCS    += ../../security_subsys_common/drv_code/pke/ecc/drv_pke_sm2_dsa_hash.c
SRCS    += ../../security_subsys_common/drv_code/pke/ecc/drv_pke_ecdsa_verify.c
SRCS    += ../../security_subsys_common/drv_code/pke/ecc/drv_pke_sm2_verify.c
SRCS    += ../../security_subsys_common/drv_code/pke/ecc/drv_pke_ecdsa_ecdh.c
SRCS    += ../../security_subsys_common/drv_code/pke/ecc/drv_pke_ecdsa_gen_key.c
SRCS    += ../../security_subsys_common/drv_code/pke/ecc/drv_pke_sm2_pub_enc.c
SRCS    += ../../security_subsys_common/drv_code/pke/ecc/drv_pke_sm2_priv_dec.c
SRCS    += ../../security_subsys_common/drv_code/pke/drv_pke_inner.c
SRCS    += ../../security_subsys_common/drv_code/pke/drv_pke_api.c
SRCS    += ../../security_subsys_common/drv_code/pke/drv_pke_exp_mod.c
SRCS    += ../../security_subsys_common/drv_code/pke/drv_pke_mod.c
endif


# drv_hash
SRCS    += ../../security_subsys_common/drv_code/hash/drv_hash_inner.c
SRCS    += ../../security_subsys_common/drv_code/hash/drv_hash_simple.c
SRCS    += ../../security_subsys_common/drv_code/hash/drv_hash_clone.c
SRCS    += ../../security_subsys_common/drv_code/hash/drv_pbkdf2_soft.c
SRCS    += ../../security_subsys_common/drv_code/hash/drv_pbkdf2_hard.c
SRCS    += ../../security_subsys_common/drv_code/hash/drv_hash_soft_hmac_simple.c

# ECC
ifdef CONFIG_HI3516CV610
SRCS    += ../../security_subsys_common/hal_code/pke_alg/calc_hash.c
SRCS    += ../../security_subsys_common/hal_code/pke_alg/ecc_check_dot_on_curve.c
SRCS    += ../../security_subsys_common/hal_code/pke_alg/ecc_common.c
SRCS    += ../../security_subsys_common/hal_code/pke_alg/ecc_compute_s.c
SRCS    += ../../security_subsys_common/hal_code/pke_alg/ecc_compute_u1_and_u2.c
SRCS    += ../../security_subsys_common/hal_code/pke_alg/ecc_compute_x.c
SRCS    += ../../security_subsys_common/hal_code/pke_alg/ecc_inner.c
SRCS    += ../../security_subsys_common/hal_code/pke_alg/ecc_inv_mod.c
SRCS    += ../../security_subsys_common/hal_code/pke_alg/ecc_mul_naf_inner.c
SRCS    += ../../security_subsys_common/hal_code/pke_alg/ecc_point_mul.c
SRCS    += ../../security_subsys_common/hal_code/pke_alg/get_multi_random.c
SRCS    += ../../security_subsys_common/hal_code/pke_alg/pke_alg_inner.c
SRCS    += ../../security_subsys_common/hal_code/pke_alg/rsa_exp_mod.c
SRCS    += ../../security_subsys_common/hal_code/pke_alg/rsa_mod.c
SRCS    += ../../security_subsys_common/hal_code/pke_alg/rsa_inner.c
SRCS    += ../../security_subsys_common/hal_code/pke_alg/sm2_add_mod.c
SRCS    += ../../security_subsys_common/hal_code/pke_alg/sm2_compute_s.c
endif

# RSA
ifdef CONFIG_HI3519DV500
SRCS    += ../../security_subsys_common/drv_code/pke/v4/drv_pke_rsa.c
endif

ifdef CONFIG_HI3516CV610
SRCS    += ../../security_subsys_common/drv_code/pke/rsa/drv_pke_rsa_priv_dec.c
SRCS    += ../../security_subsys_common/drv_code/pke/rsa/drv_pke_rsa_pub_enc.c
SRCS    += ../../security_subsys_common/drv_code/pke/rsa/drv_pke_rsa_sign.c
SRCS    += ../../security_subsys_common/drv_code/pke/rsa/drv_pke_rsa_v15.c
SRCS    += ../../security_subsys_common/drv_code/pke/rsa/drv_pke_rsa_v21.c
SRCS    += ../../security_subsys_common/drv_code/pke/rsa/drv_pke_rsa_verify.c
endif

# include
MKP_CFLAGS  += -I$(SECURITY_SUBSYS_DIR)/../../security_subsys_common/hal_code/pke_alg

# Include
MKP_CFLAGS  += -I$(SECURITY_SUBSYS_DIR)/../../security_subsys_common/include/common_include
MKP_CFLAGS  += -I$(SECURITY_SUBSYS_DIR)/../../security_subsys_common/include/drv_include
MKP_CFLAGS  += -I$(SECURITY_SUBSYS_DIR)/../../security_subsys_common/include/hal_include
MKP_CFLAGS  += -I$(SECURITY_SUBSYS_DIR)/../../security_subsys_common/include/ioctl_include
MKP_CFLAGS  += -I$(SECURITY_SUBSYS_DIR)/../../security_subsys_common/include/kapi_include
MKP_CFLAGS  += -I$(SECURITY_SUBSYS_DIR)
MKP_CFLAGS  += -I$(SECURITY_SUBSYS_DIR)/../..
MKP_CFLAGS  += -I$(SECURITY_SUBSYS_DIR)/../../crypto_osal_lib/linux
MKP_CFLAGS  += -I$(SECURITY_SUBSYS_DIR)/../../security_subsys_common/common
MKP_CFLAGS  += -I$(SECURITY_SUBSYS_DIR)/../../security_subsys_common/dispatch_code
MKP_CFLAGS  += -I$(SECURITY_SUBSYS_DIR)/../../security_subsys_common/drv_code
MKP_CFLAGS  += -I$(SECURITY_SUBSYS_DIR)/../../security_subsys_common/drv_code/hash
MKP_CFLAGS  += -I$(SECURITY_SUBSYS_DIR)/../../security_subsys_common/drv_code/symc
MKP_CFLAGS  += -I$(SECURITY_SUBSYS_DIR)/../../security_subsys_common/kapi_code

ifdef CONFIG_HI3519DV500
MKP_CFLAGS  += -I$(SECURITY_SUBSYS_DIR)/../../security_subsys_common/drv_code/pke/v4
endif
ifdef CONFIG_HI3516CV610
MKP_CFLAGS  += -I$(SECURITY_SUBSYS_DIR)/../../security_subsys_common/drv_code/pke
MKP_CFLAGS  += -I$(SECURITY_SUBSYS_DIR)/../../security_subsys_common/drv_code/pke/ecc
MKP_CFLAGS  += -I$(SECURITY_SUBSYS_DIR)/../../security_subsys_common/hal_code/pke_v5
MKP_CFLAGS  += -I$(SECURITY_SUBSYS_DIR)/../../security_subsys_common/hal_code/pke_v5/alg
endif

MKP_CFLAGS  += -include crypto_features.h
MKP_CFLAGS  += -DSOC_ID_CIPHER=0x6C

# Debug
# MKP_CFLAGS  += -DCONFIG_HASH_TIMEOUT_DEBUG
# MKP_CFLAGS  += -DCRYPTO_REG_DEBUG
# MKP_CFLAGS  += -DCRYPTO_LOG_LEVEL=4
# MKP_CFLAGS  += -DCRYPTO_CTR_TRACE_ENABLE
# MKP_CFLAGS  += -DCRYPTO_NORMAL_TRACE_ENABLE
# MKP_CFLAGS  += -DCRYPTO_CCM_TRACE_ENABLE
# MKP_CFLAGS  += -DCRYPTO_GCM_TRACE_ENABLE
# MKP_CFLAGS	+= -DCRYPTO_DISPATCH_FUNC_TRACE_ENABLE
# MKP_CFLAGS	+= -DCRYPTO_KAPI_FUNC_TRACE_ENABLE
# MKP_CFLAGS	+= -DCRYPTO_DRV_FUNC_TRACE_ENABLE
# MKP_CFLAGS	+= -DCRYPTO_HAL_FUNC_TRACE_ENABLE
# MKP_CFLAGS	+= -DCRYPTO_SYMC_HAL_NODE_DEBUG_ENABLE
# MKP_CFLAGS  += -DCRYPTO_MAC_TRACE_ENABLE
# MKP_CFLAGS  += -DCONFIG_DRV_HASH_PARAM_TRACE_ENABLE
# MKP_CFLAGS  += -DCONFIG_DRV_SYMC_PARAM_TRACE_ENABLE
# MKP_CFLAGS  += -DCONFIG_SYMC_CMAC_TRACE_ENABLE
# MKP_CFLAGS  += -DCONFIG_SYMC_HAL_DEBUG_ENABLE

SRCS_O := $(SRCS)
#*************************************************************************
# release header
INC_FILE    := ../../security_subsys_common/include/common_include/ot_common_crypto.h
#*************************************************************************
TARGET := $(KO_PREFIX)_cipher

#*************************************************************************
# compile linux or liteos
include $(MAKE_DRV_FILE)