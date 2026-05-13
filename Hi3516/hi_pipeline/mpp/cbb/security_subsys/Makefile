ifeq ($(PARAM_FILE), )
    PARAM_FILE:=../../../Makefile.param
    include $(PARAM_FILE)
endif

export CONFIG_SECURITY_SUBSYS_BUILD_ENV=sdk
export SECURITY_SUBSYS_WORK_ROOT_DIR=$(PWD)

include $(OSTYPE).mak