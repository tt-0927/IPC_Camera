ifeq ($(PARAM_FILE), )
    PARAM_FILE:=../../../../Makefile.param
    include $(PARAM_FILE)
endif

ifeq ($(CBB_PARAM_FILE), )
    CBB_PARAM_FILE:=../../Makefile.param
    include $(CBB_PARAM_FILE)
endif

MPI_SRC_DIR += $(PWD)
MPI_SRC_C := mbedtls_harden_adapt.c
MPI_SRC_C += hash_harden_impl.c
MPI_SRC_C += aes_harden_impl.c

MPI_CFLAGS += -I$(PWD)/include
MPI_CFLAGS  += -I$(PWD)/../security_subsys_common/include/uapi_include
MPI_CFLAGS  += -I$(PWD)/../security_subsys_common/include/common_include
MPI_CFLAGS  += -I$(PWD)/../crypto_osal_lib/$(OSTYPE)
MPI_CFLAGS  += -I$(PWD)/..
MPI_CFLAGS  += -I$(MEM_ROOT)/include
MPI_CFLAGS  += -I$(REL_INC)

MPI_CFLAGS  += -DCONFIG_OT_TYPE_SUPPORT -DSOC_ID_CIPHER=0x6C
MPI_CFLAGS  += -include crypto_features.h

#*************************************************************************
# release header
INC_FILE    := $(wildcard mbedtls_harden_adapt.h)

LIB_NAME    := mbedtls_harden_adapt

include $(MAKE_MPI_FILE)