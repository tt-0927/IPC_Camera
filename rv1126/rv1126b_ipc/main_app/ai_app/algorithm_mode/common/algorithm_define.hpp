/*
 * @Author: zhengxh
 * @Date: 2026-10-12 15:11:09
 * @LastEditors: zhengxh
 * @LastEditTime: 2026-10-22 09:07:00
 * @Description: 
 */

#pragma once

/* ===================== 模型与配置路径 ===================== */

/* 算法模型/配置统一部署目录（与设备 /opt/cam/model/ 一致） */
#define ALGORITHM_MODEL_DIR "/opt/cam/model/"

/* 人车非检测模型配置文件 */
#define PMNM_DETECT_MODEL_CONFIG_PATH (ALGORITHM_MODEL_DIR "group2.json")
/* 烟火行人检测模型配置文件 */
#define FIRE_PERSON_DETECT_MODEL_CONFIG_PATH (ALGORITHM_MODEL_DIR "FirePersonDetect.json")
/* 厨房14类检测模型配置文件 */
#define KITCHEN_DETECT_MODEL_CONFIG_PATH (ALGORITHM_MODEL_DIR "KitchenClass14Detect.json")
/* 人脸表情识别模型配置文件 */
#define FACE_EXPRESSION_MODEL_CONFIG_PATH (ALGORITHM_MODEL_DIR "FaceExpressionDetect.json")
/* 人脸检测模型配置文件 */
#define FACE_DETECT_MODEL_CONFIG_PATH (ALGORITHM_MODEL_DIR "FaceDetect.json")
/* 人脸质量评估模型配置文件 */
#define FACE_QUALITY_MODEL_CONFIG_PATH (ALGORITHM_MODEL_DIR "FaceEvaluate.json")
/* 人脸属性分析模型配置文件 */
#define FACE_ATTRIBUTE_MODEL_CONFIG_PATH (ALGORITHM_MODEL_DIR "FaceAttribute.json")
/* 攀爬检测模型配置文件 */
#define CLIMB_DETECT_MODEL_CONFIG_PATH (ALGORITHM_MODEL_DIR "ClimbingDetect.json")
/* 打架斗殴分析模型配置文件 */
#define FIGHT_MODEL_CONFIG_PATH (ALGORITHM_MODEL_DIR "FightDetection.json")
/* 鼠患识别模型配置文件 */
#define MOUSE_DETECT_MODEL_CONFIG_PATH (ALGORITHM_MODEL_DIR "Mouse.json")
/* ===================== 公共数据契约 ===================== */
/* 检测目标 */
typedef struct _DetectedTarget_S_
{
    float fX1, fY1, fX2, fY2; /* 左上/右下坐标 */
    float fConfidence;
    int   nLabel;             /* 类别 ID */
} DetectedTarget_S;

/* 跟踪目标：跟踪→判定（类别由外层向量下标承载） */
typedef struct _TrackedTarget_S_
{
    int   nTrackId = -1;                                  /* per-class 原始 ID，实例内唯一 */
    float fConfidence = 0.0f;
    float fX1 = 0.0f, fY1 = 0.0f, fX2 = 0.0f, fY2 = 0.0f; /* 左上/右下坐标 */
} TrackedTarget_S;
