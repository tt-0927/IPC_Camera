/**
 * @FilePath     : isp_scene.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-22 15:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-11 13:43:07
 * @Description  : RV1126B运行场景资源控制实现
 */

#include "isp_scene.h"

#include "IpcRet.h"
#include "dlog.h"
#include "isp_control.h"

namespace
{
constexpr const char *RK_SCENE_MAIN = "normal";
constexpr const char *RK_SCENE_FULL_COLOR = "day";

/**
 * @brief   : 判断是否为RV1126B产品允许的共享运行场景
 * @param    {ISP::IspRuntimeScene_E} enRuntimeScene：待校验场景
 * @return   {bool} true：支持，false：不支持
 */
bool is_supported_runtime_scene(ISP::IspRuntimeScene_E enRuntimeScene)
{
    return enRuntimeScene == ISP::IspRuntimeScene_E::DAY || enRuntimeScene == ISP::IspRuntimeScene_E::NIGHT_WHITE ||
           enRuntimeScene == ISP::IspRuntimeScene_E::NIGHT_LIGHT_OFF;
}
} // namespace

CSceneParamManager::~CSceneParamManager()
{
    scene_deinit();
}

bool CSceneParamManager::scene_init(const std::string &strConfigDir)
{
    if (m_bInitialized)
    {
        return true;
    }

    if (CIspControl::instance()->get_aiq_ctx() == nullptr)
    {
        dlog_error("RV1126B场景初始化失败：RK AIQ上下文未就绪, iq_dir:%s", strConfigDir.c_str());
        return false;
    }

    m_bInitialized = true;
    return true;
}

int CSceneParamManager::scene_set_mode(ISP::IspRuntimeScene_E enRuntimeScene)
{
    if (!m_bInitialized)
    {
        return ERR_UNINIT;
    }
    if (!is_supported_runtime_scene(enRuntimeScene))
    {
        return ERR_UNSUPPORT;
    }

    /* memory: 只借用CIspControl的AIQ上下文，场景管理器不负责释放。 */
    rk_aiq_sys_ctx_t *pstAiqContext = CIspControl::instance()->get_aiq_ctx();
    if (pstAiqContext == nullptr)
    {
        return ERR_UNINIT;
    }

    /* step: 即使三个运行态共用IQ，也必须真实调用RK API，禁止以无动作OK掩盖场景资源失败。 */
    const int nRet = rk_aiq_uapi2_sysctl_switch_scene(pstAiqContext, RK_SCENE_MAIN, RK_SCENE_FULL_COLOR);
    if (nRet != XCAM_RETURN_NO_ERROR)
    {
        dlog_error("RV1126B切换全彩IQ场景失败, runtime_scene:%d, ret:%d", static_cast<int>(enRuntimeScene), nRet);
        return static_cast<int>(nRet);
    }

    m_enCurrentRuntimeScene = enRuntimeScene;
    return OK;
}

ISP::IspRuntimeScene_E CSceneParamManager::scene_get_mode() const
{
    return m_enCurrentRuntimeScene;
}

bool CSceneParamManager::scene_deinit()
{
    m_bInitialized = false;
    m_enCurrentRuntimeScene = ISP::IspRuntimeScene_E::DAY;
    return true;
}
