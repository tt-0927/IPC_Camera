/**
 * @FilePath     : isp_scene.cpp
 * @Author       : cyc
 * @Date         : 2025-08-08 15:43:47
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-27 15:35:33
 * @Description  : isp场景模块
 */

#include "isp_scene.h"
#include "IpcRet.h"
#include "dlog.h"
#include "isp_define.h"
#include "ss_mpi_isp.h"
#include "ot_mpi_ae.h"
#include "ot_scene.h"
#include <cstring>

namespace
{
/* 场景模块当前固定使用 VI pipe 0。 */
constexpr int ISP_SCENE_VI_PIPE = 0;

/**
 * @brief   : 按运行场景应用配置化DRC覆盖策略
 * @param    {ISP::IspRuntimeScene_E} enRuntimeScene：内部运行场景
 * @param    {const SceneDrcPolicy_S &} stPolicy：三场景DRC策略
 * @return   {int} OK：成功，ERR：失败
 */
int apply_scene_drc_adjustment(ISP::IspRuntimeScene_E enRuntimeScene, const SceneDrcPolicy_S &stPolicy)
{
    const SceneDrcAdjustment_S *pAdjustment = nullptr;
    switch (enRuntimeScene)
    {
    case ISP::IspRuntimeScene_E::DAY:
        pAdjustment = &stPolicy.stDay;
        break;
    case ISP::IspRuntimeScene_E::NIGHT_WHITE:
        pAdjustment = &stPolicy.stNightWhite;
        break;
    case ISP::IspRuntimeScene_E::NIGHT_IR:
    case ISP::IspRuntimeScene_E::NIGHT_LIGHT_OFF:
    case ISP::IspRuntimeScene_E::NIGHT_SMART:
        pAdjustment = &stPolicy.stNightIr;
        break;
    default:
        return ERR_PARAM;
    }

    if (pAdjustment == nullptr || !pAdjustment->bOverride)
    {
        return OK;
    }

    /* 先读取完整MPP属性，仅覆盖策略明确拥有的字段。 */
    ot_isp_drc_attr stDrcAttr;
    int nRet = ss_mpi_isp_get_drc_attr(ISP_SCENE_VI_PIPE, &stDrcAttr);
    if (nRet != OK)
    {
        dlog_error("获取ISP DRC属性失败, ret:%d", nRet);
        return ERR;
    }

    stDrcAttr.enable = pAdjustment->bEnable ? TD_TRUE : TD_FALSE;
    if (pAdjustment->bUseManualStrength)
    {
        stDrcAttr.op_type = OT_OP_MODE_MANUAL;
        stDrcAttr.manual_attr.strength = pAdjustment->nStrength;
    }

    nRet = ss_mpi_isp_set_drc_attr(ISP_SCENE_VI_PIPE, &stDrcAttr);
    if (nRet != OK)
    {
        dlog_error("设置ISP DRC覆盖策略失败, 场景:%d, ret:%d", static_cast<int>(enRuntimeScene), nRet);
        return ERR;
    }
    return OK;
}
} // 匿名命名空间

CSceneParamManager::CSceneParamManager()
{
    memset(&m_stSceneConfig.stSceneParam, 0, sizeof(ot_scene_param));
    memset(&m_stSceneConfig.stSceneMode, 0, sizeof(ot_scene_video_mode));
}

CSceneParamManager::~CSceneParamManager()
{
    if (m_bInit)
    {
        scene_deinit();
    }
}

int CSceneParamManager::scene_init(const std::string &stConfigDir)
{
    if (stConfigDir.empty())
    {
        dlog_error("ISP场景配置目录为空");
        return ERR_PARAM;
    }

    /* 保存已验证的配置目录，供后续故障日志和场景资源生命周期追踪。 */
    m_stSceneConfig.strConfigPath = stConfigDir;

    /* 加载 ISP 场景参数。 */
    /* step: 先从ini解析场景参数和模式表，再初始化MPP Scene运行模块。 */
    int nRet = ot_scene_create_param(stConfigDir.c_str(), &m_stSceneConfig.stSceneParam, &m_stSceneConfig.stSceneMode);

    if (nRet != OK)
    {
        dlog_error("加载ISP场景参数失败, 配置路径:%s, ret:%d", stConfigDir.c_str(), nRet);
        return ERR;
    }

    nRet = ot_scene_init(&m_stSceneConfig.stSceneParam);
    if (nRet != OK)
    {
        dlog_error("初始化ISP场景模块失败, ret:%d", nRet);
        return ERR;
    }

    dlog_info("ISP场景模块初始化完成, 配置路径:%s", stConfigDir.c_str());
    m_bInit = true;

    return OK;
}

int CSceneParamManager::scene_set_mode(ISP::IspRuntimeScene_E enRuntimeScene, const Hi3516TuningProfile_S &stProfile)
{
    if (!m_bInit)
    {
        dlog_error("ISP场景模块未初始化，无法设置场景模式");
        return ERR;
    }

    /* 内部运行场景只映射MPP调参索引，不再接收网页配置场景。 */
    int nIndex = 0;
    switch (enRuntimeScene)
    {
    case ISP::IspRuntimeScene_E::DAY:
        nIndex = stProfile.nDaySceneIndex;
        break;
    case ISP::IspRuntimeScene_E::NIGHT_WHITE:
        /* 夜白能力未提供独立索引时兼容回退到白天全彩调参。 */
        nIndex = stProfile.nNightWhiteSceneIndex;
        break;
    case ISP::IspRuntimeScene_E::NIGHT_IR:
    case ISP::IspRuntimeScene_E::NIGHT_LIGHT_OFF:
    case ISP::IspRuntimeScene_E::NIGHT_SMART:
        nIndex = stProfile.nNightIrSceneIndex;
        break;
    default:
        dlog_error("未知ISP内部运行场景: %d", static_cast<int>(enRuntimeScene));
        return ERR_PARAM;
    }

    /* 将解析出的场景槽位下发给MPP Scene模块，随后按画像补充DRC差异。 */
    int nRet = ot_scene_set_scene_mode(&m_stSceneConfig.stSceneMode.video_mode[nIndex]);
    if (nRet != OK)
    {
        dlog_error("设置ISP场景模式失败, runtime_scene:%d, 索引:%d, ret:%d", static_cast<int>(enRuntimeScene), nIndex, nRet);
        return ERR;
    }

    nRet = apply_scene_drc_adjustment(enRuntimeScene, stProfile.stSceneDrc);
    if (nRet != OK)
    {
        dlog_error("应用ISP场景DRC修正失败, runtime_scene:%d, 索引:%d, ret:%d", static_cast<int>(enRuntimeScene), nIndex, nRet);
        return ERR;
    }

    m_enCurrentRuntimeScene = enRuntimeScene;
    return OK;
}

ISP::IspRuntimeScene_E CSceneParamManager::scene_get_mode()
{
    if (!m_bInit)
    {
        dlog_error("ISP场景模块未初始化，返回缓存场景模式");
        return m_enCurrentRuntimeScene;
    }

    return m_enCurrentRuntimeScene;
}

int CSceneParamManager::scene_pause(bool bIsPause)
{
    if (!m_bInit)
    {
        dlog_error("ISP场景模块未初始化，无法暂停或恢复场景算法");
        return ERR_UNINIT;
    }

    /* 将暂停状态同步到MPP Scene算法，避免仅更新本地标记导致状态分离。 */
    int nRet = ot_scene_pause(static_cast<td_bool>(bIsPause));
    if (nRet != OK)
    {
        dlog_error("%sISP场景算法失败, ret:%d", (bIsPause ? "暂停" : "恢复"), nRet);
        return ERR;
    }
    m_bPaused = bIsPause;
    return OK;
}

int CSceneParamManager::scene_deinit()
{
    if (!m_bInit)
    {
        return OK;
    }

    /* step: 先释放MPP Scene资源，再更新本地状态，失败时保留已初始化标记便于重试。 */
    int nRet = ot_scene_deinit();
    if (nRet != OK)
    {
        dlog_error("去初始化ISP场景模块失败, ret:%d", nRet);
        return ERR;
    }

    m_bInit = false;
    m_bPaused = true;
    return OK;
}
