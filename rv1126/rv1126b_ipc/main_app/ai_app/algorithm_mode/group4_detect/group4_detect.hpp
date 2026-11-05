/*
 * @FilePath: rv1126b_ipc/main_app/ai_app/algorithm_mode/group4_detect/group4_detect.hpp
 * @Description: cigarette(香烟)、sleep(睡觉)、phone(玩手机)、fall(摔跤)、falling(摔跤中)
 *
 * 全盘移植自 share/ai_share/AiModules/Modules/Group4Detect/V3_0/Group4DetectV3_0.hpp
 * + share/ai_share/AiModules/Modules/Group4Detect/Group4DetectExt.hpp
 */
#pragma once

#include <string>
#include <unordered_map>

#include "BYTETracker.h"
#include "YoloUltralytics.hpp"

#include <opencv2/opencv.hpp>

#define Group4Detect 0

namespace Group4Detect_NS {

/* 识别参数 */
typedef struct _Group4DetectParam_
{
    bool         bEnable      = false; /* 是否分析 */
    float        fConfidence  = 0.5;   /* 置信度 */
    unsigned int nDetectFrame = 1;     /* 检测多少帧才触发 */
    unsigned int nDetectDuration = 1;     /* 检测多长时间才触发，默认1s */
} Group4DetectParam_S;

typedef struct _Group4DetectPersonParam_
{
    bool         bEnable         = false; /* 是否分析 */
    float        fConfidence     = 0.5;   /* 置信度 */
    unsigned int nDetectDuration = 1;     /* 检测多长时间才触发，默认1s */
} Group4DetectPersonParam;

/* 检测分析参数 */
typedef struct _AnalyseParam_
{
    Group4DetectPersonParam stCigaretteDetectParam; /* 香烟 识别 */
    Group4DetectParam_S     stSleepParam;           /* 睡觉 识别 */
    Group4DetectPersonParam stPhoneParam;           /* 玩手机 识别 */
    Group4DetectParam_S     stFallParam;            /* 摔倒 识别 */

    float fBoxThreshold = 0.25; /* 框的置信度阈值 */
    float fNmsThreshold = -1;   /* 两个框的重叠程度 */
} AnalyseParam_S;

/* =========================================================================== */

/* 检测结果 */
typedef struct _OutData_
{
    std::string savedFileName; /* 分析后图片路径 */

    bool bCigarette = false; /* 是否 香烟 触发 */
    bool bSleep     = false; /* 是否 睡觉 触发 */
    bool bPhone     = false; /* 是否 玩手机 触发 */
    bool bFall      = false; /* 是否 摔跤 触发 */
} OutData_S;

/* 初始化参数 */
typedef struct _InParam_
{
    std::string strModelPath; /* 数字识别模型路径 */
    std::string strModelPath1; /* 数字识别模型路径 */

    /* 调试功能 */
    bool        bDebug = false;      /* 是否开启调试功能 */
    std::string strAnalyzeDataPath;  /* 设置分析后数据的保存路径, 文件夹路径 */
    std::string strOriginalDataPath; /* 原始数据保存路径, 文件夹路径 */
} InParam_S;


/* 结果 */
typedef struct _Result_
{
    float fX1 = 0;        /* 左上角x坐标 */
    float fY1 = 0;        /* 左上角y坐标 */
    float fX2 = 0;        /* 右下角x坐标 */
    float fY2 = 0;        /* 右下角y坐标 */
    float fBoxConfidence; /* 置信度 */
    int   nClassId = -1;  /* 种类ID */
} Result_S;

/* 输入数据 */
typedef struct _InData_
{
    int            nChnId;  /* 通道 */
    cv::Mat        inMat;   /* 图片 */
    AnalyseParam_S stParam; /* 参数 */
    std::vector<Result_S> vecArea; /* 人物坐标 */
} InData_S;

class CGroup4Detect {
  public:
    /* 检测类别 */
    enum FireClass
    {
        /* 香烟 */
        CIGARETTE,
        /* 睡觉 */
        SLEEP,
        /* 玩手机 */
        PHONE,
        /* 摔跤 */
        FALL,
        /* 摔跤中 */
        FALLING,
        /* 未知类型 */
        UNKOWN
    };

    /* 目标数据 */
    typedef struct _Target_
    {
        int       nId;                 /* 目标的ID */
        int64_t   nFirstTime = 0;          /* 首次跟踪时间 */
        bool      isUsed;              /* 当前数据是否使用 */
        int64_t   ndwellTime = 0;
    } Target_S;

    CGroup4Detect(InParam_S stInParam);
    ~CGroup4Detect();

    bool init();
    bool unInit();
    bool process(InData_S stInData, std::vector<Result_S> &vecResult, OutData_S *stOutData = nullptr);
    bool resizeAndPadImage(cv::Mat inputImage, cv::Mat &outputImage);

  private:
    /* 算法输入参数限制 */
    int m_nLimitWidth   = 640;
    int m_nLimitHeight  = 384;
    int m_nLimitChannel = 3;

    /* 缩放填充后左上角的坐标 */
    int m_nXOffset = 0;
    int m_nYOffset = 0;
    /* 缩放比例 */
    float m_fResizeScale = 1.0;

    /* 初始化参数 */
    InParam_S m_stInParam;

    Inference_NS::CYoloUltralytics *m_pYoloUltralytics = nullptr;

    float m_fTrackThresh = 0.5f;
    float m_fHighThresh = 0.6f;
    float m_fMatchThresh = 0.8f;
    float m_nFrameId = 0;
    int m_nMaxTimeLost = 30;
    Inference_NS::cBYTETracker *m_pByteTracker = nullptr;

    int64_t m_llCigaretteTimestamp = 0;  // 香烟 检测时间戳
    int64_t m_llPhoneTimestamp     = 0;  // 玩手机 检测时间戳
    int64_t m_llSleepTimestamp     = 0;  // 睡觉 连续检测帧数
    int64_t m_llFallTimestamp      = 0;  // 摔跤 连续检测帧数

    std::unordered_map<int, Target_S> m_mapPenson;
};

}  // namespace Group4Detect_NS
