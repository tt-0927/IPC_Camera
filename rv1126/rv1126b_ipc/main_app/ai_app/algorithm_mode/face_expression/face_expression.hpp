/*
 * @Author: zhengxh
 * @Date: 2026-10-19 11:10:51
 * @LastEditors: zhengxh
 * @LastEditTime: 2026-10-19 11:32:09
 * @Description: 
 */
/**
 * @FilePath     : rv1126b_ipc/main_app/ai_app/algorithm_mode/face_expression/face_expression.hpp
 * @Date         : 2026-10-19
 * @Description  : 人脸表情识别纯组件（不继承 CAlgorithm，不取流，由调用方驱动）
 */

#pragma once

#include <string>

#include <opencv2/opencv.hpp>

#include "algorithm_define.hpp" /* FACE_EXPRESSION_MODEL_CONFIG_PATH */

namespace Inference_NS
{
    class CImageFeature;
}

/* 表情类别 ID */
const int FACE_EXPRESSION_ANGER = 0;    /* 生气 */
const int FACE_EXPRESSION_DISGUST = 1;  /* 厌恶 */
const int FACE_EXPRESSION_FEAR = 2;     /* 恐惧 */
const int FACE_EXPRESSION_HAPPY = 3;    /* 高兴 */
const int FACE_EXPRESSION_SAD = 4;      /* 难过 */
const int FACE_EXPRESSION_SURPRISE = 5; /* 惊讶 */
const int FACE_EXPRESSION_NEUTRAL = 6;  /* 中立 */

/**
 * @brief  : 人脸表情识别纯组件
 * @note   : 仅做分类（前处理 + 推理 + 后处理），不含事件业务；输入人脸 RGB 图，
 *           输出为表情类别 ID（int，FACE_EXPRESSION_XXX）。
 *           类别约定：0-生气 1-厌恶 2-恐惧 3-高兴 4-难过 5-惊讶 6-中立（FACE_EXPRESSION_XXX，名称经 labelToString() 获取）。
 *           推理复用 Inference 层 CImageFeature，不依赖 AiModules/Modules。
 *           移植自 share/ai_share/AiModules/Modules/FaceExpression（CFaceExpressionV1_0）。
 */
class CFaceExpression
{
public:
    /* 分析参数（预留灵敏度映射入口） */
    typedef struct _AnalyseParam_
    {
    } AnalyseParam_S;

    /* process 输入：人脸 RGB 图 + 参数 */
    typedef struct _InData_
    {
        cv::Mat        inMat; /* CV_8UC3 RGB 人脸图，任意分辨率 */
        AnalyseParam_S stParam;
    } InData_S;

    /* process 输出：表情类别 ID（int，见 process 返回值） */

public:
    explicit CFaceExpression(const std::string &strJsonConfigPath);
    ~CFaceExpression();

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
     * @brief   : 人脸表情识别同步接口
     * @param    {InData_S} &stInData：输入人脸 RGB 图
     * @return   {int} 表情类别 0~6（FACE_EXPRESSION_XXX），失败返回 -1
     * @note    : 非线程安全：并发调用需调用方自行串行化
     */
    int process(const InData_S &stInData);

    /**
     * @brief   : 类别 ID 转名称
     * @param    {int} label：类别 ID（FACE_EXPRESSION_XXX）
     * @return   {std::string} 类别名称，越界返回"未知"
     * @note    : 静态工具接口，无需实例即可调用
     */
    static std::string labelToString(int label);

private:
    /* 推理句柄 */
    Inference_NS::CImageFeature *m_pImageFeature = nullptr;

    /* 模型配置文件路径 */
    std::string m_strJsonConfigPath;

    /* 模型输入尺寸（init 后以 getSizeLimit 实测为准，不写死） */
    int m_nLimitWidth  = 224;
    int m_nLimitHeight = 224;
    int m_nLimitChannel = 0;
};
