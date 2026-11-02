/**
 * @FilePath     : rv1126b_ipc/main_app/ai_app/algorithm_mode/pmnm_detect/pmnm_detect.hpp
 * @Date         : 2026-10-12
 * @Description  : 人车非检测纯组件（不继承 CAlgorithm，不取流，由调用方驱动）
 */

#pragma once

#include <string>
#include <vector>

#include <opencv2/opencv.hpp>

#include "algorithm_define.hpp" /* DetectedTarget_S */

namespace Inference_NS
{
    class CYoloUltralytics;
}

/* 检测类别 ID */
const int PMNM_CLASS_PERSON            = 0; /* 人 */
const int PMNM_CLASS_MOTOR_VEHICLE     = 1; /* 机动车 */
const int PMNM_CLASS_NON_MOTOR_VEHICLE = 2; /* 非机动车 */

/**
 * @brief  : 人车非检测纯组件
 * @note   : 仅做检测（推理 + 前后处理），不含事件业务；输入 RGB 图，输出三类目标框。
 *           类别约定：0-人 1-机动车 2-非机动车（PMNM_CLASS_XXX，与模型 json 的 label_name 顺序一致）。
 *           推理复用 Inference 层 CYoloUltralytics，不依赖 AiModules/Modules。
 */
class CPMNMDetect
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
    explicit CPMNMDetect(const std::string &strJsonConfigPath);
    ~CPMNMDetect();

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

    /**
     * @brief   : 人车非检测同步接口
     * @param    {InData_S} &stInData：输入 RGB 图 + 阈值参数
     * @param    {std::vector<DetectedTarget_S>} &vecResults：输出目标框（坐标为输入图分辨率），无目标时为空
     * @return   {int} 0：成功 -1：模型未初始化 -2：输入非法 -3：缩放失败 -4：推理失败
     * @note    : 非线程安全：并发调用需调用方自行串行化
     */
    int process(const InData_S &stInData, std::vector<DetectedTarget_S> &vecResults);

    /**
     * @brief   : 类别 ID 转名称
     * @param    {int} label：类别 ID（PMNM_CLASS_XXX）
     * @return   {std::string} 类别名称，越界返回"未知"
     * @note    : 静态工具接口，无需实例即可调用
     */
    static std::string labelToString(int label);

private:
    /**
     * @brief   : 等比缩放并居中补黑至模型输入尺寸（letterbox）
     * @param    {cv::Mat} &inputImage：输入图（任意尺寸）
     * @param    {cv::Mat} &outputImage：输出图（模型输入尺寸）
     * @param    {float} &fScale：缩放比例（还原坐标用）
     * @param    {int} &nPadX / &nPadY：居中偏移量（还原坐标用）
     * @return   {bool} true：成功 false：输入为空
     */
    bool resizeAndPad(const cv::Mat &inputImage, cv::Mat &outputImage,
                      float &fScale, int &nPadX, int &nPadY);

    /**
     * @brief   : 将 letterbox 坐标系的检测框还原到原图坐标系
     * @param    {std::vector<DetectedTarget_S>} &vecResults：待还原的检测框（原地修改）
     * @param    {float} fScale / {int} nPadX / {int} nPadY：与 resizeAndPad 同帧记录的缩放与偏移
     * @param    {int} nOriWidth / {int} nOriHeight：原图宽高（还原后夹取边界用）
     */
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
    int m_nLimitHeight = 384;
    int m_nLimitChannel = 3;
};
