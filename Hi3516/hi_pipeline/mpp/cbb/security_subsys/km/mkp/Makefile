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

KM_MKP_DIR := $(PWD)
export CONFIG_DRIVER_KM_SUPPORT=y

# all source file in this module
SRCS:=
MKP_CFLAGS  += -include crypto_features.h
MKP_CFLAGS  += -DSOC_ID_CIPHER=0x6C
# add include files
MKP_CFLAGS  +=  -I$(KM_MKP_DIR) -I$(KM_MKP_DIR)/../../crypto_osal_lib/$(OSTYPE) \
    -I$(KM_MKP_DIR)/../../security_subsys_common/include/common_include \
    -I$(KM_MKP_DIR)/../../security_subsys_common/include/drv_include \
    -I$(KM_MKP_DIR)/../../security_subsys_common/include/ioctl_include \
    -I$(KM_MKP_DIR)/../../security_subsys_common/include/kapi_include \
    -I$(KM_MKP_DIR)/../../security_subsys_common/include/hal_include \
    -I$(KM_MKP_DIR)/../../security_subsys_common/hal_code/km_v4 \
    -I$(KM_MKP_DIR)/../../security_subsys_common/hal_code/trng_v4 \
    -I$(KM_MKP_DIR)/../../security_subsys_common/dispatch_code \
    -I$(KM_MKP_DIR)/../../security_subsys_common/drv_code \
    -I$(KM_MKP_DIR)/../../security_subsys_common/common \
    -I$(KM_MKP_DIR)/../../cipher/mkp -I$(KM_MKP_DIR)/../../

# add compiling macros
INIT_FILE   := $(KM_MKP_DIR)/crypto_km_drv_init.c
INIT_INC	:= $(KM_MKP_DIR)/init_km.h

MKP_SRCS	:= crypto_km_drv_init.c init_km.c \
                ../../security_subsys_common/dispatch_code/dispatch_km.c \
                ../../security_subsys_common/drv_code/km/drv_klad.c \
                ../../security_subsys_common/drv_code/km/drv_klad_clear_key.c \
                ../../security_subsys_common/drv_code/km/drv_klad_effective_key.c \
                ../../security_subsys_common/drv_code/km/drv_klad_session_key.c \
                ../../security_subsys_common/drv_code/km/drv_klad_content_key.c \
                ../../security_subsys_common/drv_code/km/drv_keyslot.c \
                ../../security_subsys_common/hal_code/km_v4/hal_rkp.c \
                ../../security_subsys_common/hal_code/km_v4/hal_klad.c \
                ../../security_subsys_common/hal_code/km_v4/hal_keyslot_hard.c \
                ../../security_subsys_common/hal_code/km_v4/hal_keyslot_soft.c \
                ../../security_subsys_common/hal_code/km_v4/hal_klad_clear_key.c \
                ../../security_subsys_common/kapi_code/kapi_km.c    \
                ../../security_subsys_common/kapi_code/harden_support_longterm/kapi_km_impl.c

DRIVER_OBJS-y	+= $(patsubst %.c, %.o, $(MKP_SRCS))

SRCS	    += $(MKP_SRCS)

# Debug
# MKP_CFLAGS      += -DCRYPTO_HAL_FUNC_TRACE_ENABLE
# MKP_CFLAGS      += -DCRYPTO_KAPI_FUNC_TRACE_ENABLE
# MKP_CFLAGS      += -DCRYPTO_DRV_FUNC_TRACE_ENABLE
# MKP_CFLAGS      += -DCRYPTO_DISPATCH_FUNC_TRACE_ENABLE
# MKP_CFLAGS      += -DCRYPTO_LOG_LEVEL=4
# MKP_CFLAGS      += -DCONFIG_KAPI_KM_PARAM_TRACE_ENABLE

SRCS_O := $(SRCS)

#*************************************************************************
TARGET := $(KO_PREFIX)_km

#*************************************************************************

include $(MAKE_DRV_FILE)