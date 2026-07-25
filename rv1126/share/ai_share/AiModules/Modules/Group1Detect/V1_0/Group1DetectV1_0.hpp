/*
 * @Author: 梁浩尧 lianghaoyao@kfb.cn
 * @Date: 2025-12-24 16:23:34
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-01-20 11:22:09
 * @FilePath: /1126/share/ai_share/AiModules/Modules/Group1Detect/V1_0/Group1DetectV1_0.hpp
 * @Description: notHelmet(未戴安全帽)、helmet(安全帽)、reflective(反光衣)、safetyRope(安全绳)、exposedSoil(泥土裸露)、person(人)
 */

#pragma once

#include <unordered_map>

#include "Group1DetectExt.hpp"
#include "YoloUltralytics.hpp"

#define Group1Detect 0

namespace Group1Detect_NS {
class CGroup1DetectV1_0 {
  public:
    /* 检测类别 */
    enum FireClass
    {
        /* 未戴安全帽 */
        NOTHELMET,
        /* 戴安全帽 */
        HELMET,
        /* 反光衣 */
        REFLECTIVE,
        /* 安全绳 */
        SAFETYROPE,
        /* 泥土裸露 */
        EXPOSEDSOIL,
        /* 人 */
        PERSON,
        /* 未知类型 */
        UNKOWN
    };

    CGroup1DetectV1_0(InParam_S stInParam);
    ~CGroup1DetectV1_0();

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
    bool process(InData_S stInData, std::vector<Result_S> &vecResult, OutData_S *stOutData = nullptr);

    bool resizeAndPadImage(cv::Mat inputImage, cv::Mat &outputImage);
  private:
    /* 初始化参数 */
    InParam_S m_stInParam;

    Inference_NS::CYoloUltralytics *m_pYoloUltralytics = nullptr;


    /* 算法输入参数限制 */
    int m_nLimitWidth   = 640;
    int m_nLimitHeight  = 384;

    /* 缩放填充后左上角的坐标 */
    int m_nXOffset = 0;
    int m_nYOffset = 0;
    /* 缩放比例 */
    float m_fResizeScale = 1.0;


    int m_nSafetyHelmetDetectFrameCount   = 0;  // 安全帽识别 连续检测帧数
    int m_nReflectiveClothingFrameCount   = 0;  // 反光衣识别 连续检测帧数
    int m_nHighAltitudeSeatbeltFrameCount = 0;  // 高空安全带识别 连续检测帧数
    int m_nBareSoiletFrameCount           = 0;  // 泥土裸露 连续检测帧数
};

}  // namespace Group1Detect_NS