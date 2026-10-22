#工程名称
project (AI)
#版本限定
cmake_minimum_required(VERSION 3.10.2)
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
#设置编译链
SET(CMAKE_C_COMPILER "aarch64-buildroot-linux-gnu-gcc")
SET(CMAKE_CXX_COMPILER "aarch64-buildroot-linux-gnu-g++")
SET(CMAKE_AR "aarch64-buildroot-linux-gnu-ar")
SET(CMAKE_LINKER "aarch64-buildroot-linux-gnu-ld")

#opencv库路径
set(OpenCV_DIR ${CMAKE_CURRENT_LIST_DIR}/../opencv4.5.5/lib/cmake/opencv4)
find_package(
    OpenCV 
    REQUIRED 
    COMPONENTS 
    imgproc 
    core 
    imgcodecs #libopencv_imgcodecs
    highgui   #libopencv_highgui
    videoio #libopencv_videoio
    )  # 指定需要的 OpenCV 组件和库名
include_directories(${OpenCV_INCLUDE_DIRS})

#共享库路径
set(OS_SHARE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../)

#源文件
set(SRC_PATH
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../image_diff
    ${CMAKE_CURRENT_LIST_DIR}/../face_analyse

    ${OS_SHARE_PATH}/common/
    ${OS_SHARE_PATH}/system/
	${OS_SHARE_PATH}/system/time/
    ${OS_SHARE_PATH}/data_structure/que/
    ${OS_SHARE_PATH}/standard/
)
foreach(item ${SRC_PATH})
    aux_source_directory (${item} SRC_LIST)
endforeach()

#头文件
set (SRC_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../image_diff/
    ${CMAKE_CURRENT_LIST_DIR}/../image_diff/include
    ${CMAKE_CURRENT_LIST_DIR}/../face_analyse
    ${CMAKE_CURRENT_LIST_DIR}/../face_analyse/include

    ${OS_SHARE_PATH}/common/
    ${OS_SHARE_PATH}/system/
	${OS_SHARE_PATH}/system/time/
    ${OS_SHARE_PATH}/data_structure/que/
    ${OS_SHARE_PATH}/standard/
)

foreach(item ${SRC_INCLUDE})
    include_directories ( ${item} ) 
endforeach()

#编译选项
add_compile_options(-Wall)

#库路径
set(SRC_LIB_PATH
    ${CMAKE_CURRENT_LIST_DIR}/
    ${CMAKE_CURRENT_LIST_DIR}/../lib
    ${CMAKE_CURRENT_LIST_DIR}/../image_diff/include
)
foreach(item ${SRC_LIB_PATH})
    link_directories( ${item} )
endforeach()
#添加库
#set (LIBS pthread stdc++)
set (LOCAL_LIBS 
    -lrknnrt
    -lfacenet128
    -lretinaface
    -lhumancount
    -lframechanged
)
 
# 添加可执行目标或库，并链接库
add_executable(AI ${SRC_LIST})
target_link_libraries (AI  ${LIBS} ${LOCAL_LIBS} ${OpenCV_LIBS})


