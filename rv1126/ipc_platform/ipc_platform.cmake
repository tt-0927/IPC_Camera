# ipc_platform.cmake
set(PLATFORM "RV1126B_IPC")
set(PLATFORM_SOC "rv1126b")

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

# 本机以 Code/tools 下的工具链副本替代编译机 SDK 路径，仅需 tools/linux/toolchain 子目录
set(SDK_PATH           /home/jatin/Code/tools/rk-sdk)
#set(SDK_LIBRARY_DIR    ${SDK_PATH}/output/out/media_out/lib)
set(SDK_LIBRARY_DIR    ${SDK_PATH}/project/app/out/lib)
set(TOOLCHAIN_DIR      ${SDK_PATH}/tools/linux/toolchain/arm-rockchip1240-linux-gnueabihf)
set(CMAKE_C_COMPILER   ${TOOLCHAIN_DIR}/bin/arm-rockchip1240-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_DIR}/bin/arm-rockchip1240-linux-gnueabihf-g++)
set(CMAKE_AR           ${TOOLCHAIN_DIR}/bin/arm-rockchip1240-linux-gnueabihf-ar)
set(CMAKE_RANLIB       ${TOOLCHAIN_DIR}/bin/arm-rockchip1240-linux-gnueabihf-ranlib)
set(CMAKE_LD           ${TOOLCHAIN_DIR}/bin/arm-rockchip1240-linux-gnueabihf-ld)
