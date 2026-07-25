#版本限定
cmake_minimum_required(VERSION 3.10.2)
#-------------------------------------------------
#-----------------------rockit--------------------
#-------------------------------------------------
SET(CMAKE_C_COMPILER "aarch64-buildroot-linux-gnu-gcc")
SET(CMAKE_CXX_COMPILER "aarch64-buildroot-linux-gnu-g++")
SET(CMAKE_AR "aarch64-buildroot-linux-gnu-ar")
SET(CMAKE_LINKER "aarch64-buildroot-linux-gnu-ld")

#设置生成文件存放路径
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../output/bin)
message("设置生成文件存放路径:" ${CMAKE_CURRENT_LIST_DIR}/../output/bin)


#opencv库路径
set(OpenCV_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../../../opencv4.5.5/lib/cmake/opencv4)
find_package(OpenCV REQUIRED COMPONENTS imgproc core imgcodecs)
include_directories(${OpenCV_INCLUDE_DIRS})

# rk_mpi api
set(RKNN_API_PATH    ${CMAKE_CURRENT_LIST_DIR}/../../../../..)
include_directories(${RKNN_API_PATH}/rk_include)
link_directories(${RKNN_API_PATH}/lib)


link_directories(${SYS_LIB_PATH}/link)

#源文件
set (SRC_PATH
    ${CMAKE_CURRENT_LIST_DIR}/../
    ${CMAKE_CURRENT_LIST_DIR}/../../Common/ModelOpt
    ${CMAKE_CURRENT_LIST_DIR}/../../../
    ${CMAKE_CURRENT_LIST_DIR}/../../../PostProcess
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../../bl_share/log/dlog
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../../bl_share/system/time
)

foreach(item ${SRC_PATH})
    aux_source_directory (${item} SRC_LIST)
endforeach()

#头文件
set (STREAM_INCLUDE
	${CMAKE_CURRENT_LIST_DIR}/
    ${CMAKE_CURRENT_LIST_DIR}/../
    ${CMAKE_CURRENT_LIST_DIR}/../../Common/ModelOpt
    ${CMAKE_CURRENT_LIST_DIR}/../../../
    ${CMAKE_CURRENT_LIST_DIR}/../../../PostProcess
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../../bl_share/log/dlog
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../../bl_share/system/time
    ${CMAKE_CURRENT_LIST_DIR}/../../../../
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

# 目标检测-人头检测
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/../TargetDetection/HeadDetect HeadDetect_SRC_PATH)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../TargetDetection/HeadDetect)
add_executable (HeadDetect HeadDetect_demo.cpp ${HeadDetect_SRC_PATH} ${SRC_LIST} ${ROCKIT_LIST} ${OSSHARE_SRC_LIST}) 
target_link_libraries (HeadDetect  ${LIBS} ${LOCAL_LIBS} ${OpenCV_LIBS})

# 目标检测-学生行为分析 
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/../TargetDetection/StudentBehavior StudentBehavior_SRC_PATH)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../TargetDetection/StudentBehavior)
add_executable (StudentBehavior StudentBehavior_demo.cpp ${StudentBehavior_SRC_PATH} ${SRC_LIST} ${ROCKIT_LIST} ${OSSHARE_SRC_LIST}) 
target_link_libraries (StudentBehavior  ${LIBS} ${LOCAL_LIBS} ${OpenCV_LIBS})

# 目标检测-火焰检测
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/../TargetDetection/FireDetect FireDetect_SRC_PATH)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../TargetDetection/FireDetect)
add_executable (FireDetect FireDetect_demo.cpp ${FireDetect_SRC_PATH} ${SRC_LIST} ${ROCKIT_LIST} ${OSSHARE_SRC_LIST}) 
target_link_libraries (FireDetect  ${LIBS} ${LOCAL_LIBS} ${OpenCV_LIBS})
# 目标检测-行人检测
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/../TargetDetection/HumanDetect HumanDetect_SRC_PATH)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../TargetDetection/HumanDetect)
add_executable (HumanDetect HumanDetect_demo.cpp ${HumanDetect_SRC_PATH} ${SRC_LIST} ${ROCKIT_LIST} ${OSSHARE_SRC_LIST}) 
target_link_libraries (HumanDetect  ${LIBS} ${LOCAL_LIBS} ${OpenCV_LIBS})
# 目标检测-口罩检测
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/../TargetDetection/MaskDetect MaskDetect_SRC_PATH)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../TargetDetection/MaskDetect)
add_executable (MaskDetect MaskDetect_demo.cpp ${MaskDetect_SRC_PATH} ${SRC_LIST} ${ROCKIT_LIST} ${OSSHARE_SRC_LIST}) 
target_link_libraries (MaskDetect  ${LIBS} ${LOCAL_LIBS} ${OpenCV_LIBS})
# 目标检测-玩手机检测
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/../TargetDetection/PlayPhone PlayPhone_SRC_PATH)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../TargetDetection/PlayPhone)
add_executable (PlayPhone PlayPhone_demo.cpp ${PlayPhone_SRC_PATH} ${SRC_LIST} ${ROCKIT_LIST} ${OSSHARE_SRC_LIST}) 
target_link_libraries (PlayPhone  ${LIBS} ${LOCAL_LIBS} ${OpenCV_LIBS})
# 目标检测-船只检测
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/../TargetDetection/ShipDetect ShipDetect_SRC_PATH)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../TargetDetection/ShipDetect)
add_executable (ShipDetect ShipDetect_demo.cpp ${ShipDetect_SRC_PATH} ${SRC_LIST} ${ROCKIT_LIST} ${OSSHARE_SRC_LIST}) 
target_link_libraries (ShipDetect  ${LIBS} ${LOCAL_LIBS} ${OpenCV_LIBS})
# 目标检测-摔跤检测
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/../TargetDetection/FallDetect FallDetect_SRC_PATH)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../TargetDetection/FallDetect)
add_executable (FallDetect FallDetect_demo.cpp ${FallDetect_SRC_PATH} ${SRC_LIST} ${ROCKIT_LIST} ${OSSHARE_SRC_LIST}) 
target_link_libraries (FallDetect  ${LIBS} ${LOCAL_LIBS} ${OpenCV_LIBS})
# 目标检测-抽烟检测
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/../TargetDetection/SmokingDetect SmokingDetect_SRC_PATH)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../TargetDetection/SmokingDetect)
add_executable (SmokingDetect SmokingDetect_demo.cpp ${SmokingDetect_SRC_PATH} ${SRC_LIST} ${ROCKIT_LIST} ${OSSHARE_SRC_LIST}) 
target_link_libraries (SmokingDetect  ${LIBS} ${LOCAL_LIBS} ${OpenCV_LIBS})
# 目标检测-危险物品检测
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/../TargetDetection/KnifeHammerAxe KnifeHammerAxe_SRC_PATH)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../TargetDetection/KnifeHammerAxe)
add_executable (KnifeHammerAxe KnifeHammerAxe_demo.cpp ${KnifeHammerAxe_SRC_PATH} ${SRC_LIST} ${ROCKIT_LIST} ${OSSHARE_SRC_LIST}) 
target_link_libraries (KnifeHammerAxe  ${LIBS} ${LOCAL_LIBS} ${OpenCV_LIBS})

# 目标检测-电动车检测
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/../TargetDetection/ElectricBikeDetect ElectricBikeDetect_SRC_PATH)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../TargetDetection/ElectricBikeDetect)
add_executable (ElectricBikeDetect ElectricBikeDetect_demo.cpp ${ElectricBikeDetect_SRC_PATH} ${SRC_LIST} ${ROCKIT_LIST} ${OSSHARE_SRC_LIST}) 
target_link_libraries (ElectricBikeDetect  ${LIBS} ${LOCAL_LIBS} ${OpenCV_LIBS})

# 目标检测-人脸检测
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/../TargetDetection/FaceDetect FaceDetectV1_SRC_PATH)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../TargetDetection/FaceDetect)
add_executable (FaceDetectV1 FaceDetect_demoV1.cpp ${FaceDetectV1_SRC_PATH} ${SRC_LIST} ${ROCKIT_LIST} ${OSSHARE_SRC_LIST}) 
target_link_libraries (FaceDetectV1  ${LIBS} ${LOCAL_LIBS} ${OpenCV_LIBS})

# 目标检测-人脸检测(带有人脸5个点)
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/../FaceRec/FaceDetect FaceDetectV2_SRC_PATH)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../FaceRec/FaceDetect)
add_executable (FaceDetectV2 FaceDetect_demoV2.cpp ${FaceDetectV2_SRC_PATH} ${SRC_LIST} ${ROCKIT_LIST} ${OSSHARE_SRC_LIST}) 
target_link_libraries (FaceDetectV2  ${LIBS} ${LOCAL_LIBS} ${OpenCV_LIBS})

# 人脸特征提取
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/../FaceRec/FaceFeature FaceFeature_SRC_PATH)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../FaceRec/FaceFeature)
add_executable (FaceFeature FaceFeature_demo.cpp ${FaceFeature_SRC_PATH} ${SRC_LIST} ${ROCKIT_LIST} ${OSSHARE_SRC_LIST}) 
target_link_libraries (FaceFeature  ${LIBS} ${LOCAL_LIBS} ${OpenCV_LIBS})

# IIM人数统计算法
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/../Segmentation/HumanCount HumanCount_SRC_PATH)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../Segmentation/HumanCount)
add_executable (HumanCount HumanCount_demo.cpp ${HumanCount_SRC_PATH} ${SRC_LIST} ${ROCKIT_LIST} ${OSSHARE_SRC_LIST}) 
target_link_libraries (HumanCount  ${LIBS} ${LOCAL_LIBS} ${OpenCV_LIBS})

# 人体特征提取
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/../Classification/BodyFeature BodyFeature_SRC_PATH)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../Classification/BodyFeature)
add_executable (BodyFeature BodyFeature_demo.cpp ${BodyFeature_SRC_PATH} ${SRC_LIST} ${ROCKIT_LIST} ${OSSHARE_SRC_LIST}) 
target_link_libraries (BodyFeature  ${LIBS} ${LOCAL_LIBS} ${OpenCV_LIBS})

# 人脸表情识别
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/../Classification/Expression Expression_SRC_PATH)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../Classification/Expression)
add_executable (Expression Expression_demo.cpp ${Expression_SRC_PATH} ${SRC_LIST} ${ROCKIT_LIST} ${OSSHARE_SRC_LIST}) 
target_link_libraries (Expression  ${LIBS} ${LOCAL_LIBS} ${OpenCV_LIBS})

# 打架识别
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/../Classification/FightClassify FightClassify_SRC_PATH)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../Classification/FightClassify)
add_executable (FightClassify FightClassify_demo.cpp ${FightClassify_SRC_PATH} ${SRC_LIST} ${ROCKIT_LIST} ${OSSHARE_SRC_LIST}) 
target_link_libraries (FightClassify  ${LIBS} ${LOCAL_LIBS} ${OpenCV_LIBS})

# 黑底白数字识别
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/../Classification/NumberOcr NumberOcr_SRC_PATH)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../Classification/NumberOcr)
add_executable (NumberOcr NumberOcr_demo.cpp ${NumberOcr_SRC_PATH} ${SRC_LIST} ${ROCKIT_LIST} ${OSSHARE_SRC_LIST}) 
target_link_libraries (NumberOcr  ${LIBS} ${LOCAL_LIBS} ${OpenCV_LIBS})


