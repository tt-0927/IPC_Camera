/**
 * @FilePath     : isp_scene.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-22 15:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-11 13:39:13
 * @Description  : RV1126B运行场景资源控制声明
 */

#pragma once

#include <string>

#include "Singleton.h"
#include "isp_runtime_scene.h"

/**
 * @brief RV1126B运行场景资源管理器。
 * @note 三种可达运行场景均调用RK normal/day IQ资源，以保持产品全彩策略。
 */
class CSceneParamManager : public CSingleton<CSceneParamManager>
{
public:
    /**
     * @brief   : 构造运行场景资源管理器
     * @return  {void}
     */
    CSceneParamManager() = default;
    /**
     * @brief   : 释放运行场景资源管理器
     * @return  {void}
     */
    ~CSceneParamManager();
    friend class CSingleton<CSceneParamManager>;

    /**
     * @brief   : 初始化运行场景资源
     * @param    {const std::string&} strConfigDir：IQ配置目录，仅保留用于接口兼容和日志上下文
     * @return   {bool} true：成功，false：失败
     */
    bool scene_init(const std::string &strConfigDir);
    /**
     * @brief   : 将共享运行场景应用为RK normal/day IQ场景
     * @param    {ISP::IspRuntimeScene_E} enRuntimeScene：共享裁决后的运行场景
     * @return   {int} OK：成功，ERR_UNSUPPORT：运行场景不支持，其他：RK调用失败
     * @note    : NIGHT_WHITE和NIGHT_LIGHT_OFF均保持全彩，不调用normal/night。
     */
    int scene_set_mode(ISP::IspRuntimeScene_E enRuntimeScene);
    /**
     * @brief   : 获取最后一次成功下发的共享运行场景
     * @return   {ISP::IspRuntimeScene_E} 当前运行场景
     */
    ISP::IspRuntimeScene_E scene_get_mode() const;
    /**
     * @brief   : 释放运行场景资源
     * @return   {bool} true：成功，false：失败
     */
    bool scene_deinit();

private:
    /* 当前资源生命周期状态；只有true时才允许调用scene_set_mode。 */
    bool m_bInitialized{ false };
    /* 最后成功下发的共享运行场景；RK三个场景实际都使用normal/day。 */
    ISP::IspRuntimeScene_E m_enCurrentRuntimeScene{ ISP::IspRuntimeScene_E::DAY };
};
