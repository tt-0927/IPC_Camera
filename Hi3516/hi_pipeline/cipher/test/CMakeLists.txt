cmake_minimum_required(VERSION 3.12)

# 独立验证程序固定面向 Hi3516CV610 交叉编译；如调用方显式传入编译器，则保留调用方配置。
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
if(NOT CMAKE_C_COMPILER)
    set(CMAKE_C_COMPILER arm-v01c02-linux-musleabi-gcc)
endif()

project(security_subsys_selftest C)

set(REPO_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../..)
set(HI_PIPELINE_ROOT ${CMAKE_CURRENT_LIST_DIR}/../..)
set(CIPHER_ROOT ${HI_PIPELINE_ROOT}/cipher)
set(MPP_SECURITY_ROOT ${HI_PIPELINE_ROOT}/mpp/cbb/security_subsys)
set(IPC_PLATFORM_LIB_ROOT ${REPO_ROOT}/ipc_platform/lib)
set(SS_MPI_LIB_DIR ${IPC_PLATFORM_LIB_ROOT}/ss_mpi/lib)
set(OPENSSL_ROOT ${IPC_PLATFORM_LIB_ROOT}/openssl)
set(OPENSSL_INCLUDE_DIR ${OPENSSL_ROOT}/include)
set(OPENSSL_LIB_DIR ${OPENSSL_ROOT}/lib)

link_directories(
    ${SS_MPI_LIB_DIR}
    ${OPENSSL_LIB_DIR}
)

set(CMAKE_EXE_LINKER_FLAGS
    "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath-link=${SS_MPI_LIB_DIR} -Wl,-rpath-link=${OPENSSL_LIB_DIR}"
)

include_directories(
    ${CIPHER_ROOT}/cipher
    ${CIPHER_ROOT}/km
    ${CIPHER_ROOT}/otp
    ${HI_PIPELINE_ROOT}/common
    ${HI_PIPELINE_ROOT}/mpp/include
    ${OPENSSL_INCLUDE_DIR}
    ${MPP_SECURITY_ROOT}/security_subsys_common/ot_mpi
    ${MPP_SECURITY_ROOT}/security_subsys_common/ot_mpi/linux
    ${MPP_SECURITY_ROOT}/security_subsys_common/ot_mpi_api
    ${MPP_SECURITY_ROOT}/security_subsys_common/include/common_include
    ${MPP_SECURITY_ROOT}/security_subsys_common/include/uapi_include
    ${MPP_SECURITY_ROOT}/security_subsys_common/include/ioctl_include
    ${MPP_SECURITY_ROOT}/crypto_osal_lib/linux
)

add_executable(security_subsys_selftest
    security_subsys_selftest.c
    ${CIPHER_ROOT}/cipher/cipher_context.c
    ${CIPHER_ROOT}/cipher/cipher_hash.c
    ${CIPHER_ROOT}/cipher/cipher_pke.c
    ${CIPHER_ROOT}/cipher/cipher_symc.c
    ${CIPHER_ROOT}/cipher/cipher_trng.c
    ${CIPHER_ROOT}/km/cipher_km.c
    ${CIPHER_ROOT}/otp/cipher_otp.c
    ${HI_PIPELINE_ROOT}/common/mpi_buf.c
)

target_link_libraries(security_subsys_selftest
    atomic
    crypto
    securec
    ss_mpi_cipher
    ss_mpi_km
    ss_mpi_sysmem
    ot_mpi_otp
)
