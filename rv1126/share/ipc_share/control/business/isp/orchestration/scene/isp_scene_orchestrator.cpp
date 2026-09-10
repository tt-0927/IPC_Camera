/**
 * @FilePath     : isp_scene_orchestrator.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-13 14:13:58
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-15 16:03:11
 * @Description  : 共享ISP平台场景资源生命周期管理器实现
 */

#include "isp_scene_orchestrator.h"

#include "IpcRet.h"
#include "dlog.h"

CIspSceneOrchestrator::CIspSceneOrchestrator(IIspSceneProvider &stProvider)
    : m_rstProvider(stProvider), m_bInitialized(false)
{
}

int CIspSceneOrchestrator::init()
{
    if (m_bInitialized)
    {
        return OK;
    }

    int nRet = m_rstProvider.init();
    if (nRet != OK)
    {
        dlog_error("场景provider初始化失败: %d", nRet);
        return nRet;
    }

    m_bInitialized = true;
    return OK;
}

int CIspSceneOrchestrator::deinit()
{
    if (!m_bInitialized)
    {
        return OK;
    }

    int nRet = m_rstProvider.deinit();
    m_bInitialized = false;
    return nRet;
}
