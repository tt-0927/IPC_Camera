/**
 * @FilePath     : isp_scene_provider_hi3516.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-13 14:13:58
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-15 15:49:12
 * @Description  : Hi3516 ISP场景适配端口实现
 */

#include "isp_scene_provider_hi3516.h"

#include "IpcRet.h"
#include "dlog.h"
#include "isp_scene.h"
#include "path_define.h"

CIspSceneProviderHi3516::CIspSceneProviderHi3516(const Hi3516TuningProfile_S &stTuningProfile)
    : m_rstTuningProfile(stTuningProfile), m_bInitialized(false)
{
}

int CIspSceneProviderHi3516::init()
{
    if (m_bInitialized)
    {
        return OK;
    }

    /* ISP_CONFIG_PATH 是平台配置根目录；共享层不感知文件布局。 */
    int nRet = CSceneParamManager::instance()->scene_init(ISP_CONFIG_PATH);
    if (nRet != OK)
    {
        dlog_error("Hi3516场景初始化失败: %d", nRet);
        return nRet;
    }

    m_bInitialized = true;
    return OK;
}

int CIspSceneProviderHi3516::deinit()
{
    if (!m_bInitialized)
    {
        return OK;
    }

    int nRet = CSceneParamManager::instance()->scene_deinit();
    m_bInitialized = false;
    return nRet;
}

int CIspSceneProviderHi3516::apply_scene(ISP::IspRuntimeScene_E enRuntimeScene)
{
    /* 调参画像决定传感器相关场景映射，避免 CSceneParamManager 再读取机型宏。 */
    return CSceneParamManager::instance()->scene_set_mode(enRuntimeScene, m_rstTuningProfile);
}
