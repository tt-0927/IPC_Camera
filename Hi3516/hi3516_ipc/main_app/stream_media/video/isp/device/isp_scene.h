/**
 * @FilePath     : isp_scene.h
 * @Author       : cyc
 * @Date         : 2025-08-08 15:44:01
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-27 15:35:33
 * @Description  : isp场景模块
 */

#pragma once
#include <string>
#include "Singleton.h"
#include "isp_tuning_profile.h"
#include "isp_runtime_scene.h"

extern "C"
{
#include "scene_loadparam.h"
#include "ot_scene.h"
}

struct SceneConfig_S
{
    std::string strConfigPath;       /* ISP配置路径 */
    ot_scene_param stSceneParam;     /* 场景参数结构体 */
    ot_scene_video_mode stSceneMode; /* 场景模式 */
};

class CSceneParamManager : public CSingleton<CSceneParamManager>
{
public:
    friend class CSingleton<CSceneParamManager>;
    /**
     * @brief   : 构造场景参数管理器并清零MPP Scene结构。
     * @return   {void}
     */
    CSceneParamManager();
    /**
     * @brief   : 销毁管理器并在必要时释放MPP Scene资源。
     * @return   {void}
     */
    ~CSceneParamManager();

    /**
     * @brief   : 初始化 ISP 场景参数
     * @param    {std::string} stConfigDir：场景配置目录
     * @return   {int} OK：成功，ERR/ERR_PARAM：失败
     */
    int scene_init(const std::string &stConfigDir);

    /**
     * @brief   : 设置 ISP 场景模式
     * @param    {ISP::IspRuntimeScene_E} enRuntimeScene：内部日夜运行场景
     * @param    {Hi3516TuningProfile_S} stProfile：机型调参画像
     * @return   {int} OK：成功，ERR：失败
     */
    int scene_set_mode(ISP::IspRuntimeScene_E enRuntimeScene, const Hi3516TuningProfile_S &stProfile);

    /**
     * @brief   : 暂停或恢复 ISP 场景算法
     * @param    {bool} bIsPause：true 暂停，false 恢复
     * @return   {int} OK：成功，ERR/ERR_UNINIT：失败
     */
    int scene_pause(bool bIsPause);

    /**
     * @brief   : 去初始化 ISP 场景模块
     * @param    {void}
     * @return   {int} OK：成功，ERR/ERR_UNINIT：失败
     */
    int scene_deinit();

    /**
     * @brief   : 获取当前 ISP 场景模式
     * @param    {void}
     * @return   {ISP::IspRuntimeScene_E} 当前内部运行场景
     */
    ISP::IspRuntimeScene_E scene_get_mode();

private:
    /* memory: 场景参数和模式表由本管理器拥有，scene_deinit后不得继续传给MPP Scene接口。 */
    SceneConfig_S m_stSceneConfig;
    /* 场景初始化标志 */
    bool m_bInit = false;
    /* 场景暂停标志 */
    bool m_bPaused = true;
    /* 当前场景模式 */
    ISP::IspRuntimeScene_E m_enCurrentRuntimeScene = ISP::IspRuntimeScene_E::DAY;
};
