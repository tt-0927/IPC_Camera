#版本限定
cmake_minimum_required(VERSION 3.10.2)
#-------------------------------------------------
#-----------------------rockit--------------------
#-------------------------------------------------
SET(CMAKE_C_COMPILER "aarch64-buildroot-linux-gnu-gcc")
SET(CMAKE_CXX_COMPILER "aarch64-buildroot-linux-gnu-g++")
SET(CMAKE_AR "aarch64-buildroot-linux-gnu-ar")
SET(CMAKE_LINKER "aarch64-buildroot-linux-gnu-ld")

#opencv库路径
set(OpenCV_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../../../opencv4.5.5/lib/cmake/opencv4)
find_package(OpenCV REQUIRED COMPONENTS imgproc core imgcodecs)
include_directories(${OpenCV_INCLUDE_DIRS})

link_directories(${SYS_LIB_PATH}/link)

#源文件
set (SRC_PATH
    ${CMAKE_CURRENT_LIST_DIR}/../Tracker/ByteTrack
    ${CMAKE_CURRENT_LIST_DIR}/../Segmentation/ColorCutout
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../../bl_share/log/dlog
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../../bl_share/system/time
)

foreach(item ${SRC_PATH})
    aux_source_directory (${item} SRC_LIST)
endforeach()

#头文件
set (STREAM_INCLUDE
	${CMAKE_CURRENT_LIST_DIR}/../Tracker/ByteTrack
	${CMAKE_CURRENT_LIST_DIR}/../common/ByteTrack
	${CMAKE_CURRENT_LIST_DIR}/../Segmentation/ColorCutout
	${CMAKE_CURRENT_LIST_DIR}/../../Common/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../../bl_share/log/dlog
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../../bl_share/system/time
)

foreach(item ${STREAM_INCLUDE})
    include_directories ( ${item} ) 
endforeach()

add_compile_options(-Wall)

#添加库
set (LIBS pthread stdc++)


# 分割网络-颜色人像抠图
add_executable (ColorCutoutDemo ColorCutoutDemo.cpp ${SRC_LIST} ${ROCKIT_LIST} ) 
target_link_libraries (ColorCutoutDemo  ${LIBS} ${OpenCV_LIBS})

# ByteTrack跟踪算法
add_executable (ByteTrackDemo ByteTrackDemo.cpp ${SRC_LIST} ${ROCKIT_LIST} ) 
target_link_libraries (ByteTrackDemo  ${LIBS} ${OpenCV_LIBS})
