#版本限定
#cmake_minimum_required(VERSION 3.10.2)
#-------------------------------------------------
#-----------------------rockit--------------------
#-------------------------------------------------
#SET(CMAKE_C_COMPILER "aarch64-buildroot-linux-gnu-gcc")
#SET(CMAKE_CXX_COMPILER "aarch64-buildroot-linux-gnu-g++")
#SET(CMAKE_AR "aarch64-buildroot-linux-gnu-ar")
#SET(CMAKE_LINKER "aarch64-buildroot-linux-gnu-ld")


#opencv库路径
set(OpenCV_DIR ${CMAKE_CURRENT_LIST_DIR}/../source/rk3588_opencv455/lib/cmake/opencv4)
find_package(OpenCV REQUIRED COMPONENTS imgproc core imgcodecs)  # 指定需要的 OpenCV 组件
include_directories(${OpenCV_INCLUDE_DIRS})

# rk_mpi api
set(RKNN_API_PATH ${CMAKE_CURRENT_LIST_DIR}/../source/librknn_api)
include_directories(${RKNN_API_PATH}/include)
link_directories(${RKNN_API_PATH}/lib)

# rk_rga api
set(RKNN_API_PATH ${CMAKE_CURRENT_LIST_DIR}/../source/librga)
include_directories(${RKNN_API_PATH}/include)
link_directories(${RKNN_API_PATH}/lib)

#rockit源文件
foreach(item ${ROCKIT_SRC_PATH})
    aux_source_directory (${item} ROCKIT_LIST)
endforeach()
#rockit头文件
foreach(item ${ROCKIT_INCLUDE_PATH})
    include_directories (${item})
endforeach()

#rockit库路径
foreach(item ${ROCKIT_LIB_PATH})
    link_directories (${item})
endforeach()


link_directories(${SYS_LIB_PATH}/link)

#源文件
set (SRC_PATH
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/src
)

foreach(item ${SRC_PATH})
    aux_source_directory (${item} SRC_LIST)
endforeach()

#头文件
set (STREAM_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}/include
)

foreach(item ${STREAM_INCLUDE})
    include_directories ( ${item} ) 
endforeach()

add_compile_options(-Wall)

#添加库
set (LIBS pthread stdc++)
set (LOCAL_LIBS 
    asound
    drm_rockchip
    drm
    rockchip_mpp
    rockit
    avcodec        
    avdevice        
    avfilter        
    avformat        
    avutil       
    swscale         
    swresample
    rga
    zlog
    xml2
    MemoryCheck
    freetype
    iconv
    x264
    fdk-aac
    z
    
    rknnrt 
 )

add_executable (Fastpose ${SRC_LIST} ${ROCKIT_LIST} ${OSSHARE_SRC_LIST}) 
target_link_libraries (Fastpose  ${LIBS} ${LOCAL_LIBS} ${OpenCV_LIBS})

