#工程名称
project (OcrChinese)
#版本限定
cmake_minimum_required(VERSION 3.10.2)
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
#设置编译链
SET(CMAKE_C_COMPILER "aarch64-buildroot-linux-gnu-gcc")
SET(CMAKE_CXX_COMPILER "aarch64-buildroot-linux-gnu-g++")
SET(CMAKE_AR "aarch64-buildroot-linux-gnu-ar")
SET(CMAKE_LINKER "aarch64-buildroot-linux-gnu-ld")

link_directories(${SYS_LIB_PATH}/link)

#opencv库路径
set(OpenCV_DIR ${CMAKE_CURRENT_LIST_DIR}/source/rk3588_opencv455/lib/cmake/opencv4)
find_package(OpenCV REQUIRED COMPONENTS imgproc core imgcodecs)  # 指定需要的 OpenCV 组件

if (OpenCV_FOUND)
    message("OpenCV found: ${OpenCV_VERSION}")
    include_directories(${OpenCV_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "OpenCV not found")
endif()

# 打印相关信息
if (OpenCV_FOUND)
    message("CMAKE_SOURCE_DIR is ====> ${CMAKE_SOURCE_DIR}")
    message("OpenCV_DIR is --> ${OpenCV_DIR}")
    message(STATUS "include path ${OpenCV_INCLUDE_DIRS}")
    message(STATUS "opencv library status: ")
    message(STATUS "version ${OpenCV_VERSION}")
    message(STATUS "libraries: ${OpenCV_LIBS}")
    message("OpenCV头文件有 --> ${OpenCV_INCLUDE_DIRS}")
else ()
    message("OpenCV_DIR is --> ${OpenCV_DIR}")
    message(FATAL_ERROR "Could not find OpenCV")
endif ()

# rk_mpi api
set(RKNN_API_PATH ${CMAKE_CURRENT_LIST_DIR}/source/librknn_api)
include_directories(${RKNN_API_PATH}/include)
link_directories(${CMAKE_CURRENT_LIST_DIR}/source/librknn_api/lib)

#源文件
set (SRC_PATH
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/OcrDetect/src
    ${CMAKE_CURRENT_LIST_DIR}/OcrRep/src
)
foreach(item ${SRC_PATH})
    aux_source_directory (${item} SRC_LIST)
endforeach()
#头文件
set (STREAM_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}/OcrDetect/include
    ${CMAKE_CURRENT_LIST_DIR}/OcrRep/include
)
foreach(item ${STREAM_INCLUDE})
    include_directories ( ${item} ) 
endforeach()


add_compile_options(-Wall)

#添加库
set (LIBS pthread stdc++)
set (LOCAL_LIBS 
    rknnrt
 )

add_executable(OcrChinese ${SRC_LIST})
target_link_libraries (OcrChinese  ${LIBS} ${LOCAL_LIBS} ${OpenCV_LIBS})
