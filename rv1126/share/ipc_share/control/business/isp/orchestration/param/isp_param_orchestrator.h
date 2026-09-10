/**
 * @FilePath     : isp_param_orchestrator.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-13 14:11:02
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-22 10:20:06
 * @Description  : 共享ISP参数应用服务声明
 */

#pragma once

#include "isp_define.h"
#include "isp_config_repository.h"
#include "isp_platform_adapters.h"

/**
 * @brief 共享 ISP 参数应用服务。
 * @note  从配置仓储读取参数，再通过平台适配接口写入硬件。
 *        禁止使用 CIspControl::instance()；构造时传入仓储和参数适配接口。
 */
class CIspParamOrchestrator
{
public:
    /**
     * @brief   : 构造参数应用服务
     * @param    {IIspConfigRepository&} stRepository：配置仓储
     * @param    {IIspParameterApplier&} stApplier：参数适配端口
     */
    CIspParamOrchestrator(IIspConfigRepository &stRepository, IIspParameterApplier &stApplier);

    /**
     * @brief   : 按配置类型读取持久化参数并下发到平台适配端口
     * @param    {ISP::PicConfigureType_E} enType：配置类型
     * @return   {int} OK：成功，ERR_UNSUPPORT：DAYNIGHT/SCENE不属于param域，非OK：失败
     */
    int apply_by_type(ISP::PicConfigureType_E enType);

    /**
     * @brief   : 按固定顺序再次应用网页参数，再调用平台后处理
     * @param    {ISP::SceneType_E} enConfigScene：要再次应用的网页配置场景
     * @param    {ISP::IspRuntimeScene_E} enRuntimeScene：已应用的内部日夜场景
     * @return   {int} OK：成功，非OK：失败
     * @note    : 应用顺序：image→exposure→backlight→AWB→NR→mirror→on_scene_applied。
     */
    int replay_web_params(ISP::SceneType_E enConfigScene, ISP::IspRuntimeScene_E enRuntimeScene);

private:
    /**
     * @brief   : 从指定场景参数副本下发一种网页参数
     * @param    {ISP::PicConfigureType_E} enType：配置类型
     * @param    {const ISP::SceneParams_S&} stSceneParams：指定场景的参数副本
     * @return   {int} OK：成功，非OK：失败
     */
    int apply_scene_param_by_type(ISP::PicConfigureType_E enType, const ISP::SceneParams_S &stSceneParams);

    /* 配置仓储引用 */
    IIspConfigRepository &m_rstRepository;
    /* 参数适配端口引用 */
    IIspParameterApplier &m_rstApplier;
};
