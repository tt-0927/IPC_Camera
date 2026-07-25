# 一、图像分类模块

> Classification 文件夹

| 序号 | 模型                 | 位置                  | 图片输入格式 | 输出                                         |
| ---- | -------------------- | --------------------- | ------------ | -------------------------------------------- |
| 1    | 车牌识别             | LicensePlateRec       | RGB          | 第一个输出: 车牌号 <br> 第二个输出: 车牌颜色 |
| 2    | 人脸属性识别         | FaceAttribute         | RGB          | 性别、年龄、眼镜、胡子、口罩、表情           |
| 3    | 人脸特征提取         | FaceFeature           | RGB          |                                              |
| 4    | 非机动车属性识别     | NonMotorizedAttribute | RGB          | 类型、颜色                                   |
| 5    | 数字识别             | NumberOcr             | RGB          |                                              |
| 6    | 行人属性识别         | PresonAttribute       | RGB          |                                              |
| 7    | 车辆属性识别         | VehicleAttribute      | RGB          |                                              |
| 8    | Clip-图像特征提取    | ClipImage             | RGB          |                                              |
| 9    | Clip-文本特征提取    | ClipText              | 文本         |                                              |
| 10   | 人脸1000个特征点提取 | FaceLandmark1000      | RGB          |                                              |
| 11   | 人脸质量评估         | FaceQualityAssessment | RGB          |                                              |

# 二、目标检测模块

> TargetDetection文件夹

| 序号 | 模型 | 位置 | 图片输入格式 | 输出 |
|:--------:|:--------:| :---------:|:--------:|:--------:|
| 1 | 行人检测 | HumanDetect | RGB | {0: 行人} |
| 2 | 车牌检测 | LicensePlateDetect | RGB | {0: 车牌} |
| 3 | 人脸检测算法 | Yolov5Face | RGB | {0: 人脸} |
| 4 | 人脸检测算法（带5个关键点） | Retinaface、Yolov5Face | RGB |  |
| 5 | 人、机动车、非机动车检测 | PMNMDetect | RGB | {0: 人, 1: 机动车, 2: 非机动车} |


# 三、关键点检测模块

> KeyPoint文件夹

| 序号 | 模型 | 位置 | 图片输入格式 | 输出 |
|:--------:|:--------:| :---------:|:--------:|:--------:|
| 1 | 车牌关键点检测 | LicensePlateDetect | RGB | 车牌目标框、车牌四个角的坐标 |

