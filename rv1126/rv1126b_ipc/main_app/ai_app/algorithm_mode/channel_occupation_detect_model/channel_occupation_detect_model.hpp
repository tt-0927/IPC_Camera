/**
 * @FilePath     : rv1126b_ipc/main_app/ai_app/algorithm_mode/channel_occupation_detect_model/channel_occupation_detect_model.hpp
 * @Date         : 2026-10-23
 * @Description  : 通道占用检测纯组件（不继承 CAlgorithm，不取流，由调用方驱动）
 */

#pragma once

#include <vector>

#include <opencv2/opencv.hpp>

namespace Inference_NS
{
    class CChannelOccupationDetect;
}

/* 灵敏度档位（算法预设） */
const unsigned int CHANNEL_OCCUPANCY_SENSITIVITY_LOW  = 0; /* 低：只报明显占用 */
const unsigned int CHANNEL_OCCUPANCY_SENSITIVITY_MID  = 1; /* 中：漏报误报均衡 */
const unsigned int CHANNEL_OCCUPANCY_SENSITIVITY_HIGH = 2; /* 高：抓小目标与细微变化 */

/**
 * @brief  : 通道占用检测纯组件
 * @note   : 仅做单帧分析（背景差分 + 连通域 + 帧间运动），不含使能判断与事件业务；
 *           输入 CV_8UC3 RGB 图，输出是否占用、目标框、面积占比与运动标志。
 *           推理复用 Inference 层 CChannelOccupationDetect，不依赖 AiModules/Modules。
 *           移植自 share/ai_share/AiModules/Modules/ChannelOccupationDetect（CChannelOccupationDetectV1_0）。
 */
class CChannelOccupationDetectModel
{
public:
    /* 初始化参数：对比帧/背景帧需同尺寸；vecPoints 为空表示整幅画面参与检测 */
    typedef struct _InParam_
    {
        cv::Mat                prevMat;   /* 对比帧 */
        cv::Mat                bgMat;     /* 背景帧 */
        std::vector<cv::Point> vecPoints; /* 检测区域多边形顶点 */
    } InParam_S;

    /* 单帧分析参数 */
    typedef struct _AnalyseParam_
    {
        unsigned int nSensitivity = CHANNEL_OCCUPANCY_SENSITIVITY_MID; /* 灵敏度档位 */
    } AnalyseParam_S;

    /* process 输入：RGB 图 + 参数 */
    typedef struct _InData_
    {
        cv::Mat        inMat;   /* CV_8UC3 RGB，尺寸须与初始化帧一致 */
        AnalyseParam_S stParam;
    } InData_S;

    /* 检测结果：坐标为输入图坐标系 */
    typedef struct _OutData_
    {
        bool     bOccupied    = false; /* 本帧是否占用 */
        cv::Rect stBbox;               /* 主要目标框，无目标时为空 */
        float    fAreaRatio   = 0.0f;  /* 最大连通域面积 / ROI 面积 */
        bool     bMoving      = false; /* 目标是否在运动 */
        float    fMotionRatio = 0.0f;  /* 目标框内运动像素比例 */
    } OutData_S;

public:
    explicit CChannelOccupationDetectModel(const InParam_S &stInParam);
    ~CChannelOccupationDetectModel();

    /**
     * @brief   : 建立检测基准
     * @return   {bool} true：成功 false：失败（可修复后重新调用）
     * @note    : 校验基准帧与检测区域后才创建算法句柄；重复调用安全
     */
    bool init();

    /**
     * @brief   : 释放算法句柄
     * @return   {bool} true：成功
     * @note    : 重复调用安全；调用方需保证 unInit 后不再调用 process
     */
    bool unInit();

    /**
     * @brief   : 通道占用检测同步接口
     * @param    {InData_S} &stInData：输入 RGB 图 + 灵敏度档位
     * @param    {OutData_S} &stOutData：输出占用标志、目标框与运动信息
     * @return   {int} 0：成功 -1：未初始化 -2：输入非法 -3：推理失败
     * @note    : 非线程安全：并发调用需调用方自行串行化；灵敏度变化时热切换参数
     */
    int process(const InData_S &stInData, OutData_S &stOutData);

private:
    /**
     * @brief   : 按灵敏度档位设置算法参数
     * @param    {unsigned int} nSensitivity：灵敏度档位（非法值按中档处理）
     * @note    : 只改阈值类参数，不改 process_width，避免内部坐标比例失配
     */
    void applySensitivity(unsigned int nSensitivity);

private:
    /* 初始化参数 */
    InParam_S m_stInParam;

    /* 算法句柄 */
    Inference_NS::CChannelOccupationDetect *m_pDetector = nullptr;

    /* 当前灵敏度档位 避免每帧重复设置参数 */
    unsigned int m_nSensitivity = CHANNEL_OCCUPANCY_SENSITIVITY_MID;
};
