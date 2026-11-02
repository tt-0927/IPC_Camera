/**
 * @FilePath     : rv1126b_ipc/main_app/ai_app/algorithm_mode/fight_detect/fight_detect.hpp
 * @Date         : 2026-10-20
 * @Description  : 打架斗殴检测纯组件实现
 */

#pragma once

#include <string>
#include <vector>

#include <opencv2/opencv.hpp>
#include "algorithm_define.hpp"

namespace Inference_NS
{
    class CYoloUltralytics;
}

/* 检测类别 ID */
const int FIGHT_CLASS_FIGHT = 0; /* 打架 */

class FightDetect
{
public:
    /* 检测参数：阈值 <=0 时由推理库回落到 json 配置的默认值（预留灵敏度映射入口） */
    typedef struct _AnalyseParam_
    {
        float fBoxThreshold = -1.0f;
        float fNmsThreshold = -1.0f;
    } AnalyseParam_S;

    /* process 输入：RGB 图 + 参数 */
    typedef struct _InData_
    {
        cv::Mat        inMat; /* CV_8UC3 RGB，任意分辨率 */
        AnalyseParam_S stParam;
    } InData_S;

    /* 检测结果：DetectedTarget_S（algorithm_define.hpp） */

public:
    explicit FightDetect(const std::string &strJsonConfigPath);
    ~FightDetect();
    /**
     * @brief   : 加载模型
     * @return   {bool} true：成功 false：失败（可修复后重新调用）
     * @note    : 只读取 json 配置加载推理句柄；失败时内部会清理半初始化状态
     */
    bool init();
    /**
     * @brief   : 释放模型
     * @return   {bool} true：成功
     * @note    : 重复调用安全；调用方需保证 unInit 后不再调用 process
     */
    bool unInit();
    int process(const InData_S &stInData, std::vector<DetectedTarget_S> &vecResults);
    static std::string labelToString(int label);

private:
    bool resizeAndPad(const cv::Mat &inputImage, cv::Mat &outputImage,
                               float &fScale, int &nPadX, int &nPadY);

    void restoreCoords(std::vector<DetectedTarget_S> &vecResults,
                       float fScale, int nPadX, int nPadY,
                       int nOriWidth, int nOriHeight);

private:
    /* 推理句柄 */
    Inference_NS::CYoloUltralytics *m_pYoloHandle = nullptr;

    /* 模型配置文件路径 */
    std::string m_strJsonConfigPath;

    /* 模型输入尺寸（init 后以 getSizeLimit 实测为准，不写死） */
    int m_nLimitWidth  = 640;
    int m_nLimitHeight = 640;
    int m_nLimitChannel = 3;
};
