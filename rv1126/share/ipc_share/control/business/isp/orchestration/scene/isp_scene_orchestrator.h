/**
 * @FilePath     : isp_scene_orchestrator.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-13 14:13:58
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-15 16:03:11
 * @Description  : 共享ISP平台场景资源生命周期管理器声明
 */

#pragma once

#include "isp_platform_adapters.h"

/**
 * @brief 共享ISP平台场景资源生命周期管理器。
 * @note  运行场景切换和网页参数重放统一由reconciler串行执行。
 */
class CIspSceneOrchestrator
{
public:
    /**
     * @brief   : 构造场景编排器
     * @param    {IIspSceneProvider&} stProvider：场景适配端口
     */
    explicit CIspSceneOrchestrator(IIspSceneProvider &stProvider);

    /**
     * @brief   : 初始化场景资源（幂等）
     * @return   {int} OK：成功，非OK：失败
     */
    int init();

    /**
     * @brief   : 释放场景资源（幂等）
     * @return   {int} OK：成功，非OK：失败
     */
    int deinit();

private:
    /* 场景适配端口引用 */
    IIspSceneProvider &m_rstProvider;
    /* 初始化状态 */
    bool m_bInitialized;
};
