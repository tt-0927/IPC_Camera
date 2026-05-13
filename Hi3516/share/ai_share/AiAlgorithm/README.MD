# 零、文件结构

- **AIApply文件（算法的应用）**：多个AI模型组合而成的功能
- **BaseAIModel**：单个AI模型实现的功能
- **Other**：其他的一些不与AI模型相关的算法

# 一、算法业务应用

## 1、画面变化检测算法（PPT）

（1）算法1：Other/Opencv/image_diff

- 功能：对比前后帧，或者多帧画面的像素差异

## 2、人数统计算法

（1）算法1：BaseAIModel/Segmentation/RK_HumanCount

- 功能：基于热力图的分割识别，耗时800ms，比较准确

（2）算法2：BaseAIModel/TargetDetection/RK_HeadDetect

- 功能：基于热力图的分割识别，耗时20-25ms，准确度没有算法1准

## 3、人脸识别算法

（1）算法1：AIApply/RK_RetinafaceFacenet

- 功能：人脸识别+人脸特征点比对

## 4、跟踪算法

（1）算法1：AIApply/RK_CameraTrack

- 功能：云台控制跟踪算法，特写画面跟踪

（2）算法2：AIApply/RK_SlideWindowTrack

- 功能：全景静态跟踪算法，全景画面跟踪

（3）算法3：AIApply/RK_HeadSort

- 功能：站立跟踪

## 5、文字识别算法

（1）算法1：AIApply/RK_Ocr

- 功能：文字检测+文字识别算法

# 二、基础算法

## 1、人体17个关键点检测

（1）算法1：BaseAIModel/KeyPoint/RK_KeypointsDetect

- 功能：可以获取教师的身体17个关键点，根据关键点相关的逻辑进行行为判断
- 应用：教师板书检测

## 2、文本检测

（1）算法1：BaseAIModel/Ocr/RK_OcrDetect

- 功能：将图片中的文字检测成坐标

## 3、文本识别

（1）算法1：BaseAIModel/Ocr/RK_OcrRep

- 功能：将图片中的文字识别成文本

## 4、人脸检测

（1）算法1：BaseAIModel/FaceRecognition/RK_Retinaface

- 功能：将图片中的人脸检测成坐标

## 5、人脸识别

（1）算法1：BaseAIModel/FaceRecognition/RK_FaceNet

- 功能：将图片中的人脸提取128个特征点

## 6、火焰和烟雾识别

（1）算法1：BaseAIModel/TargetDetection/RK_FireDetect

## 7、人头检测

（1）算法1：BaseAIModel/TargetDetection/RK_HeadDetect

## 8、行人检测

（1）算法1：BaseAIModel/TargetDetection/RK_HumanDetect

## 9、吸烟检测

（1）算法1：BaseAIModel/TargetDetection/RK_MaskDetect

## 10、玩手机检测

（1）算法1：BaseAIModel/TargetDetection/RK_PlayPhone

## 11、学生行为分析

（1）算法1：BaseAIModel/TargetDetection/RK_StudentBehaviorV1

- 功能：8个行为检测(低头写字，低头看书，抬头听课，转头，举手，站立，小组讨论，教师指导)
（2）算法2：BaseAIModel/TargetDetection/RK_StudentBehaviorV2
- 功能：4个行为检测(低头、抬头、转头、举手)