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

MPI_SRC_DIR += $(PWD)/../../security_subsys_common/ot_mpi
MPI_SRC_DIR += $(PWD)/../../security_subsys_common/uapi_code
MPI_SRC_C := ot_mpi_km.c uapi_km.c
MPI_CFLAGS  += -I$(PWD)/../../crypto_osal_lib/$(OSTYPE) \
                -I$(PWD)/../../security_subsys_common/include/uapi_include \
                -I$(PWD)/../../security_subsys_common/include/common_include \
                -I$(PWD)/../../security_subsys_common/include/ioctl_include \
                -I$(PWD)/../../security_subsys_common/ot_mpi_api \
                -I$(PWD)/../../security_subsys_common/ot_mpi/linux

# Debug
# MPI_CFLAGS  += -DCRYPTO_UAPI_TRNG_DEBUG_ENABLE
MPI_CFLAGS  += -DCONFIG_OT_TYPE_SUPPORT -DSOC_ID_CIPHER=0x6C
MPI_CFLAGS  += -include $(PWD)/../../crypto_features.h

#*************************************************************************
# release header
INC_FILE    := $(wildcard ../../security_subsys_common/ot_mpi_api/ot_mpi_km.h)

LIB_NAME    := ot_mpi_km

include $(MAKE_MPI_FILE)