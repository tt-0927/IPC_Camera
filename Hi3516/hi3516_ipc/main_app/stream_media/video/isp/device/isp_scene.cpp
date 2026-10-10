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
#include "ss_mpi_vi.h"
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

/**
 * @brief   : 按运行场景覆盖3DNR mdy[]/tfy[]/nrc0_mode参数
 * @param    {ISP::IspRuntimeScene_E} enRuntimeScene：内部运行场景
 * @param    {const NrxPolicy_S &} stNrx：三场景NRX覆盖策略
 * @return   {int} OK：成功，ERR：失败
 * @note    : 仅当配置启用覆盖时生效；未配置覆盖的场景直接返回OK
 */
int apply_scene_nrx_adjustment(ISP::IspRuntimeScene_E enRuntimeScene, const NrxPolicy_S &stNrx)
{
    const NrxAdjustment_S *pAdjustment = nullptr;
    switch (enRuntimeScene)
    {
    case ISP::IspRuntimeScene_E::DAY:
        pAdjustment = &stNrx.stDay;
        break;
    case ISP::IspRuntimeScene_E::NIGHT_WHITE:
        pAdjustment = &stNrx.stNightWhite;
        break;
    case ISP::IspRuntimeScene_E::NIGHT_IR:
    case ISP::IspRuntimeScene_E::NIGHT_LIGHT_OFF:
    case ISP::IspRuntimeScene_E::NIGHT_SMART:
        pAdjustment = &stNrx.stNightIr;
        break;
    default:
        return OK;
    }

    if (pAdjustment == nullptr || !pAdjustment->bOverride)
    {
        return OK;
    }

    /* 读取当前3DNR参数，修改mdy[]/tfy[]/nrc0_mode后回写。 */
    ot_3dnr_param stNrxAttr;
    stNrxAttr.nr_version = OT_NR_V2;
    stNrxAttr.nr_norm_param_v2.op_mode = OT_OP_MODE_MANUAL;
    int nRet = ss_mpi_vi_get_pipe_3dnr_param(ISP_SCENE_VI_PIPE, &stNrxAttr);
    if (nRet != OK)
    {
        dlog_error("获取VI pipe 3DNR参数失败, ret:%d", nRet);
        return ERR;
    }

    /* mdy[] */
    stNrxAttr.nr_norm_param_v2.nr_manual.nr_param.mdy[0].math0 = pAdjustment->nMdy0Math0;
    stNrxAttr.nr_norm_param_v2.nr_manual.nr_param.mdy[0].math1 = pAdjustment->nMdy0Math1;
    stNrxAttr.nr_norm_param_v2.nr_manual.nr_param.mdy[1].math0 = pAdjustment->nMdy1Math0;
    stNrxAttr.nr_norm_param_v2.nr_manual.nr_param.mdy[1].math1 = pAdjustment->nMdy1Math1;

    /* tfy[0] */
    stNrxAttr.nr_norm_param_v2.nr_manual.nr_param.tfy[0].tfs0 = pAdjustment->nTfy0Tfs0;
    stNrxAttr.nr_norm_param_v2.nr_manual.nr_param.tfy[0].tfs1 = pAdjustment->nTfy0Tfs1;
    stNrxAttr.nr_norm_param_v2.nr_manual.nr_param.tfy[0].tfs2 = pAdjustment->nTfy0Tfs2;
    for (int i = 0; i < 6; ++i)
    {
        stNrxAttr.nr_norm_param_v2.nr_manual.nr_param.tfy[0].tfr0[i] = pAdjustment->nTfy0Tfr0[i];
    }

    /* tfy[1] */
    stNrxAttr.nr_norm_param_v2.nr_manual.nr_param.tfy[1].tfs0 = pAdjustment->nTfy1Tfs0;
    stNrxAttr.nr_norm_param_v2.nr_manual.nr_param.tfy[1].tfs1 = pAdjustment->nTfy1Tfs1;
    stNrxAttr.nr_norm_param_v2.nr_manual.nr_param.tfy[1].tfs2 = pAdjustment->nTfy1Tfs2;
    for (int i = 0; i < 6; ++i)
    {
        stNrxAttr.nr_norm_param_v2.nr_manual.nr_param.tfy[1].tfr0[i] = pAdjustment->nTfy1Tfr0[i];
    }

    /* nrc0_mode */
    stNrxAttr.nr_norm_param_v2.nr_manual.nr_param.nrc0_mode = pAdjustment->nNrc0Mode;

    nRet = ss_mpi_vi_set_pipe_3dnr_param(ISP_SCENE_VI_PIPE, &stNrxAttr);
    if (nRet != OK)
    {
        dlog_error("设置VI pipe 3DNR参数失败, 场景:%d, ret:%d", static_cast<int>(enRuntimeScene), nRet);
        return ERR;
    }

    dlog_info("场景%d 3DNR已覆盖: mdy[0].math0=%u math1=%u, mdy[1].math0=%u math1=%u, "
              "tfy[0].tfs0=%u tfs1=%u tfs2=%u, tfy[1].tfs0=%u tfs1=%u tfs2=%u, nrc0_mode=%u",
              static_cast<int>(enRuntimeScene),
              pAdjustment->nMdy0Math0, pAdjustment->nMdy0Math1, pAdjustment->nMdy1Math0, pAdjustment->nMdy1Math1,
              pAdjustment->nTfy0Tfs0, pAdjustment->nTfy0Tfs1, pAdjustment->nTfy0Tfs2,
              pAdjustment->nTfy1Tfs0, pAdjustment->nTfy1Tfs1, pAdjustment->nTfy1Tfs2,
              pAdjustment->nNrc0Mode);
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

    nRet = apply_scene_nrx_adjustment(enRuntimeScene, stProfile.stNrx);
    if (nRet != OK)
    {
        dlog_error("应用ISP场景NRX修正失败, runtime_scene:%d, ret:%d", static_cast<int>(enRuntimeScene), nRet);
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
