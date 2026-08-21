# cipher.cmake

add_compile_definitions(SOC_ID_CIPHER=0x6C)

# 引用cmake
include(${CMAKE_CURRENT_LIST_DIR}/cipher/cipher.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/km/km.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/otp/otp.cmake)

# 头文件
set (INCLUDE_PATH
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../mpp/cbb/security_subsys/security_subsys_common/ot_mpi
    ${CMAKE_CURRENT_LIST_DIR}/../mpp/cbb/security_subsys/security_subsys_common/ot_mpi/linux
    ${CMAKE_CURRENT_LIST_DIR}/../mpp/cbb/security_subsys/security_subsys_common/ot_mpi_api
    ${CMAKE_CURRENT_LIST_DIR}/../mpp/cbb/security_subsys/security_subsys_common/include/common_include/
    ${CMAKE_CURRENT_LIST_DIR}/../mpp/cbb/security_subsys/security_subsys_common/include/uapi_include
    ${CMAKE_CURRENT_LIST_DIR}/../mpp/cbb/security_subsys/crypto_osal_lib/linux
    ${CMAKE_CURRENT_LIST_DIR}/../mpp/cbb/security_subsys/security_subsys_common/common
    ${CMAKE_CURRENT_LIST_DIR}/../mpp/cbb/security_subsys
    ${CMAKE_CURRENT_LIST_DIR}/../mpp/include/exp_inc
    ${CMAKE_CURRENT_LIST_DIR}/../mpp/cbb/security_subsys/security_subsys_common/uapi_code
    ${CMAKE_CURRENT_LIST_DIR}/../mpp/cbb/security_subsys/security_subsys_common/include/ioctl_include
)

# 源文件
set (SOURCE_PATH
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../mpp/cbb/security_subsys/security_subsys_common/ot_mpi
    # crypto_osal_lib/linux 和 security_subsys_common/common 是内核态驱动源码，
    # 使用 osal_printk / osal_ioremap_nocache 等内核 API，不应编入用户态程序。
    # 用户态 crypto 功能由 -lss_mpi_cipher / -lss_mpi_km / -lot_mpi_otp 提供。
    ${CMAKE_CURRENT_LIST_DIR}/../mpp/cbb/security_subsys
    ${CMAKE_CURRENT_LIST_DIR}/../mpp/cbb/security_subsys/security_subsys_common/uapi_code
)

# 添加头文件
foreach(item ${INCLUDE_PATH})
    include_directories (${item}) 
endforeach()

# 添加源文件
foreach(item ${SOURCE_PATH})
    aux_source_directory (${item} SOURCE_LIST)
endforeach()

list(APPEND HI_PIPELINE_LIST ${SOURCE_LIST})