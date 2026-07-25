/*
 * @Author: lianghy lianghy@kfb.cn
 * @Date: 2026-01-08 20:27:20
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-01-17 09:37:04
 * @FilePath: /1126/share/ai_share/AiModules/Modules/Group4Detect/V1_0/Group41DetectV1_0.hpp
 * @Description: cigarette(香烟)、sleep(睡觉)、phone(玩手机)、fall(摔跤)、falling(摔跤中)
 */
#pragma once

#include <unordered_map>

#include "Group4DetectExt.hpp"
#include "YoloUltralytics.hpp"

#define Group4Detect 0

namespace Group4Detect_NS {
class CGroup4DetectV1_0 {
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

    CGroup4DetectV1_0(InParam_S stInParam);
    ~CGroup4DetectV1_0();

    /**
     * @brief 初始化
     * @return [*]
     * @note
     */
    bool init();

    /**
     * @brief 反初始化
     * @return [*]
     * @note
     */
    bool unInit();

    /**
     * @brief 处理数据
     * @param [cv::Mat] inMat: 传入的视频数据
     * @param [AnalyseParam_S] stParam: 分析的参数
     * @param [std::vector<Result_S>&] vecResult: 输出的处理结果
     * @return [*]
     * @note
     */
    bool process(bool isDetectPerson, InData_S stInData, std::vector<Result_S> &vecResult, OutData_S *stOutData = nullptr);

    // bool push_back_unique(std::vector<Result_S> &vstResult, const Result_S &stResult);
    bool resizeAndPadImage(cv::Mat inputImage, cv::Mat &outputImage);
  private:

    /* 算法输入参数限制 */
    int m_nLimitWidth   = 640;
    int m_nLimitHeight  = 384;

    /* 缩放填充后左上角的坐标 */
    int m_nXOffset = 0;
    int m_nYOffset = 0;
    /* 缩放比例 */
    float m_fResizeScale = 1.0;

    /* 初始化参数 */
    InParam_S m_stInParam;

    Inference_NS::CYoloUltralytics *m_pYoloUltralytics = nullptr;

    int64_t m_llCigaretteTimestamp = 0;  // 香烟 检测时间戳
    int64_t m_llPhoneTimestamp     = 0;  // 玩手机 检测时间戳
    int m_nSleepFrameCount     = 0;  // 睡觉 连续检测帧数
    int m_nFallFrameCount      = 0;  // 摔跤 连续检测帧数
};

}  // namespace Group4Detect_NS