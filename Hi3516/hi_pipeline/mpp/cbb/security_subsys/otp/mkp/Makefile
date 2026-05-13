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

OTP_MKP_DIR := $(PWD)

# all source file in this module
SRCS:=
MKP_CFLAGS  += -include crypto_features.h
MKP_CFLAGS  += -DSOC_ID_CIPHER=0x6C

# add include files
MKP_CFLAGS  +=  -I$(OTP_MKP_DIR) -I$(OTP_MKP_DIR)/../../crypto_osal_lib/$(OSTYPE) \
    -I$(OTP_MKP_DIR)/../../security_subsys_common/include/common_include \
    -I$(OTP_MKP_DIR)/../../security_subsys_common/include/drv_include \
    -I$(OTP_MKP_DIR)/../../security_subsys_common/include/ioctl_include \
    -I$(OTP_MKP_DIR)/../../security_subsys_common/include/kapi_include \
    -I$(OTP_MKP_DIR)/../../security_subsys_common/include/hal_include \
    -I$(OTP_MKP_DIR)/../../security_subsys_common/dispatch_code \
    -I$(OTP_MKP_DIR)/../../security_subsys_common/drv_code \
    -I$(OTP_MKP_DIR)/../../security_subsys_common/common \
    -I$(OTP_MKP_DIR)/../../cipher/mkp -I$(OTP_MKP_DIR)/../../

# add compiling macros
INIT_FILE   := $(OTP_MKP_DIR)/crypto_otp_drv_init.c
INIT_INC	:= $(OTP_MKP_DIR)/init_otp.h

MKP_SRCS	:= crypto_otp_drv_init.c init_otp.c ../../security_subsys_common/dispatch_code/dispatch_otp.c \
                ../../security_subsys_common/kapi_code/kapi_otp.c ../../security_subsys_common/drv_code/otp/drv_otp.c

DRIVER_OBJS-y	+= $(patsubst %.c, %.o, $(MKP_SRCS))

SRCS	    += $(MKP_SRCS)

# Debug
# MKP_CFLAGS  += -DCRYPTO_REG_DEBUG
# MKP_CFLAGS      += -DCRYPTO_HAL_FUNC_TRACE_ENABLE
# MKP_CFLAGS      += -DCRYPTO_KAPI_FUNC_TRACE_ENABLE
# MKP_CFLAGS      += -DCRYPTO_DRV_FUNC_TRACE_ENABLE
# MKP_CFLAGS      += -DCRYPTO_DISPATCH_FUNC_TRACE_ENABLE

SRCS_O := $(SRCS)

#*************************************************************************
TARGET := $(KO_PREFIX)_otp

#*************************************************************************

include $(MAKE_DRV_FILE)