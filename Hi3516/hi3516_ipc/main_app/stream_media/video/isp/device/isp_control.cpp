/**
 * @FilePath     : isp_control.cpp
 * @Author       : cyc
 * @Date         : 2025-06-05 10:16:15
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-27 20:01:17
 * @Description  : isp参数控制模块
 */

#include <algorithm>
#include <array>
#include <cstdint>
#include <iomanip>
#include <unistd.h>
#include "isp_control.h"
#include "ss_mpi_isp.h"
#include "ot_mpi_ae.h"
#include "ss_mpi_awb.h"
#include "IpcRet.h"
#include "isp_tuning_builder.h"
#include "sample_comm.h"
#include "dlog.h"
#include "ot_mpi_isp.h"
#include "isp_configure.h"
#include "isp_scene.h"
#include "stream_video.h"

using namespace ISP;

namespace
{
/* 网页统一百分比参数下限。 */
constexpr int ISP_USER_VALUE_MIN = 0;
/* 网页统一百分比参数上限。 */
constexpr int ISP_USER_VALUE_MAX = 100;
/* 背光区域未选中时的统计权重。 */
constexpr td_u8 BACKLIGHT_WEIGHT_DEFAULT = 1;
/* 背光区域选中时的统计权重。 */
constexpr td_u8 BACKLIGHT_WEIGHT_VALUE = 3;

/**
 * @brief   : 将网页值按偏移后直接缩放规则映射到MPP参数值
 * @param    {unsigned int} nUserValue：网页值
 * @param    {const ScaledOffsetMapping_S &} stMapping：缩放规则
 * @return   {unsigned int} 保持偏移后直接缩放语义的MPP参数值
 * @note    : 仅限制原网页值，不能在叠加偏移后再次截断，否则TNR=100会从17回退为16。
 */
unsigned int map_user_to_scaled_system(unsigned int nUserValue, const ScaledOffsetMapping_S &stMapping)
{
    const unsigned int nClampedUser = std::min(nUserValue, static_cast<unsigned int>(ISP_USER_VALUE_MAX));
    const uint64_t u64AdjustedUser = static_cast<uint64_t>(nClampedUser + stMapping.nUserOffset);
    return static_cast<unsigned int>((u64AdjustedUser * stMapping.nMultiplier) / stMapping.nDivisor);
}

/**
 * @brief   : 将AWB预设四通道增益写入MPP手动白平衡参数
 * @param    {ot_isp_wb_attr &} stWbAttr：待写入的MPP手动白平衡参数
 * @param    {const AwbPresetGain_S &} stPreset：配置加载并校验后的预设增益
 * @return   {void}
 */
void apply_awb_preset_gain(ot_isp_wb_attr &stWbAttr, const AwbPresetGain_S &stPreset)
{
    stWbAttr.op_type = OT_OP_MODE_MANUAL;
    stWbAttr.manual_attr.r_gain = static_cast<td_u16>(stPreset.nRGain);
    stWbAttr.manual_attr.gr_gain = static_cast<td_u16>(stPreset.nGrGain);
    stWbAttr.manual_attr.gb_gain = static_cast<td_u16>(stPreset.nGbGain);
    stWbAttr.manual_attr.b_gain = static_cast<td_u16>(stPreset.nBGain);
}

/**
 * @brief   : 将网页降噪等级写入同一组MPP 3DNR参数
 * @param    {ot_3dnr_param &} stNrAttr：待写入的MPP 3DNR参数
 * @param    {const DnrAttr_S &} stDnrInfo：网页持久化降噪配置
 * @param    {const NrTuningProfile_S &} stProfile：已校验的降噪映射
 * @return   {void}
 * @note    : 普通和高级模式共用SNR/TNR公式；DNR直写MPP字段fine_g1，不参与缩放。
 */
void apply_nr_levels(ot_3dnr_param &stNrAttr, const DnrAttr_S &stDnrInfo, const NrTuningProfile_S &stProfile)
{
    const td_u8 u8TnrLevel = static_cast<td_u8>(map_user_to_scaled_system(stDnrInfo.nTnrLevel, stProfile.stTnr));
    const td_u8 u8SnrLevel = static_cast<td_u8>(map_user_to_scaled_system(stDnrInfo.nSnrLevel, stProfile.stSnr));
    stNrAttr.nr_norm_param_v2.op_mode = OT_OP_MODE_MANUAL;
    stNrAttr.nr_norm_param_v2.nr_manual.nr_param.iey.fine_g1 = stDnrInfo.nDnrLevel;
    stNrAttr.nr_norm_param_v2.nr_manual.nr_param.tfy[0].tss0 = u8TnrLevel;
    stNrAttr.nr_norm_param_v2.nr_manual.nr_param.tfy[0].tss1 = u8TnrLevel;
    stNrAttr.nr_norm_param_v2.nr_manual.nr_param.tfy[0].tss2 = u8TnrLevel;
    stNrAttr.nr_norm_param_v2.nr_manual.nr_param.sfy[0].sbr1 = u8SnrLevel;
    stNrAttr.nr_norm_param_v2.nr_manual.nr_param.sfy[0].sbr2 = u8SnrLevel;
    stNrAttr.nr_norm_param_v2.nr_manual.nr_param.sfy[0].sbr4 = u8SnrLevel;
}

/**
 * @brief   : 将网页值按配置映射到MPP参数值
 * @param    {unsigned int} nUserValue：网页值
 * @param    {unsigned int} nSystemMin：系统值下限
 * @param    {unsigned int} nSystemMax：系统值上限
 * @param    {int} nUserOffset：网页值偏移
 * @return   {unsigned int} 映射并限制范围后的系统值
 */
unsigned int map_user_to_system(unsigned int nUserValue, unsigned int nSystemMin, unsigned int nSystemMax, int nUserOffset)
{
    const unsigned int nClampedUser = std::min(nUserValue, static_cast<unsigned int>(ISP_USER_VALUE_MAX));
    const int nAdjustedUser = std::clamp(static_cast<int>(nClampedUser) + nUserOffset, ISP_USER_VALUE_MIN, ISP_USER_VALUE_MAX);
    const uint64_t u64Range = static_cast<uint64_t>(nSystemMax - nSystemMin);
    return nSystemMin + static_cast<unsigned int>((static_cast<uint64_t>(nAdjustedUser) * u64Range) / ISP_USER_VALUE_MAX);
}

/**
 * @brief   : 将MPP参数值按配置反映射到网页值
 * @param    {unsigned int} nSystemValue：系统值
 * @param    {unsigned int} nSystemMin：系统值下限
 * @param    {unsigned int} nSystemMax：系统值上限
 * @param    {int} nUserOffset：网页值偏移
 * @return   {unsigned int} 映射并限制到0～100的网页值
 */
unsigned int map_system_to_user(unsigned int nSystemValue, unsigned int nSystemMin, unsigned int nSystemMax, int nUserOffset)
{
    const unsigned int nClampedSystem = std::clamp(nSystemValue, nSystemMin, nSystemMax);
    const uint64_t u64Range = static_cast<uint64_t>(nSystemMax - nSystemMin);
    const uint64_t u64Relative = static_cast<uint64_t>(nClampedSystem - nSystemMin);
    const int nMappedUser = static_cast<int>((u64Relative * ISP_USER_VALUE_MAX) / u64Range) - nUserOffset;
    return static_cast<unsigned int>(std::clamp(nMappedUser, ISP_USER_VALUE_MIN, ISP_USER_VALUE_MAX));
}

/**
 * @brief   : 将内部调参场景转换为画像数组下标
 * @param    {IspTuningScene_E} enScene：已由运行场景上下文校验的调参场景
 * @return   {std::size_t} 对应的场景画像下标
 * @note    : 场景值只能由set_runtime_scene_context写入，非法运行场景会在写入时被拒绝。
 */
std::size_t get_tuning_scene_index(IspTuningScene_E enScene)
{
    return static_cast<std::size_t>(enScene);
}

/**
 * @brief   : 取得当前运行场景对应的网页参数映射
 * @param    {const Hi3516TuningProfile_S &} stTuningProfile：已加载并校验的调参画像
 * @param    {IspTuningScene_E} enScene：当前内部调参场景
 * @return   {const IspSceneParamMapping_S &} 当前场景的只读映射
 * @note    : 调用方必须先确认画像指针有效，避免在初始化前解引用。
 */
const IspSceneParamMapping_S &get_scene_param_mapping(const Hi3516TuningProfile_S &stTuningProfile, IspTuningScene_E enScene)
{
    return stTuningProfile.stSceneParamMappings[get_tuning_scene_index(enScene)];
}

/* 获取ISP启动使用的传感器类型，未注入画像时沿用sample_comm默认选择。 */
sample_sns_type get_sensor_type(const Hi3516TuningProfile_S *pProfile)
{
    if (pProfile != nullptr && pProfile->nSensorType >= 0)
    {
        return static_cast<sample_sns_type>(pProfile->nSensorType);
    }
    return static_cast<sample_sns_type>(IspTuningBuilder_NS::get_sensor_type());
}
} // 匿名命名空间

CIspControl::CIspControl()
{
}

CIspControl::~CIspControl()
{
}

int CIspControl::init()
{
    /* nRet统一保留MPP VI/ISP初始化链路的返回码，便于后续扩展错误处理。 */
    int nRet = IpcRet_E::OK;
    m_viPipe = 0;
    /* MPP VI配置快照在启动前同时指定传感器、pipe与ISP旁路状态。 */
    sample_vi_cfg stViCfg;
    sample_sns_type stSnstype = get_sensor_type(m_pTuningProfile);
    /* 获取VI模块默认配置。 */
    /* 根据已注入的调参画像选择传感器，保证 ISP 启动型号与场景调参一致。 */
    sample_comm_vi_get_default_vi_cfg(stSnstype, &stViCfg);
    stViCfg.pipe_info[0].pipe_attr.isp_bypass = TD_FALSE;

    /* 先停止ISP，避免前次未停止导致本次启动失败。 */
    sample_comm_vi_stop_isp(&stViCfg);
    /* 启动ISP。 */
    sample_comm_vi_start_isp(&stViCfg);

    m_isInitialized.store(true);
    dlog_info("ISP控制模块初始化完成");

    return nRet;
}

int CIspControl::deinit()
{
    int nRet = IpcRet_E::OK;
    /* 使用同一传感器配置构造停止参数，确保与 init 的 ISP 实例一致。 */
    sample_vi_cfg stViCfg;
    sample_sns_type stSnstype = get_sensor_type(m_pTuningProfile);

    /* step: 先关闭可见初始化状态，再停止底层 ISP，阻止上层继续下发参数。 */
    m_isInitialized.store(false);

    /* 获取VI模块默认配置。 */
    sample_comm_vi_get_default_vi_cfg(stSnstype, &stViCfg);
    /* 停止ISP。 */
    sample_comm_vi_stop_isp(&stViCfg);
    dlog_info("ISP控制模块去初始化完成");
    return nRet;
}

int CIspControl::set_tuning_profile(const Hi3516TuningProfile_S *pProfile)
{
    if (pProfile == nullptr)
    {
        dlog_error("注入ISP调参画像失败: profile为空");
        return ERR_PARAM_NULL;
    }

    /* memory: 调参画像由业务服务持有，控制模块仅在服务生命周期内借用。 */
    m_pTuningProfile = pProfile;
    return OK;
}

IspTuningScene_E CIspControl::get_tuning_scene() const
{
    return m_enTuningScene.load();
}

int CIspControl::set_runtime_scene_context(ISP::IspRuntimeScene_E enRuntimeScene)
{
    switch (enRuntimeScene)
    {
    case ISP::IspRuntimeScene_E::DAY:
        m_enTuningScene.store(IspTuningScene_E::DAY);
        return OK;
    case ISP::IspRuntimeScene_E::NIGHT_WHITE:
        m_enTuningScene.store(IspTuningScene_E::NIGHT_WHITE);
        return OK;
    case ISP::IspRuntimeScene_E::NIGHT_IR:
    case ISP::IspRuntimeScene_E::NIGHT_SMART:
    case ISP::IspRuntimeScene_E::NIGHT_LIGHT_OFF:
        m_enTuningScene.store(IspTuningScene_E::NIGHT_IR);
        return OK;
    default:
        return ERR_PARAM;
    }
}

/**************************图像参数控制 ************************/

int CIspControl::set_saturation(const unsigned int nSaturation)
{
    /* MPP CSC完整属性快照，仅读改写饱和度以保留其他颜色转换配置。 */
    int nRet = IpcRet_E::OK;
    ot_isp_csc_attr stCscAttr;

    /* note: 必须先读取完整属性再修改单字段，避免覆盖其他模块已下发的 CSC 参数。 */
    nRet = ss_mpi_isp_get_csc_attr(m_viPipe, &stCscAttr);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("获取ISP CSC属性失败, ret:%d", nRet);
        return IpcRet_E::ERR;
    }

    if (m_pTuningProfile == nullptr)
    {
        dlog_error("设置饱和度失败: ISP参数映射未注入");
        return ERR_UNINIT;
    }

    /* 饱和度是CSC快照中唯一由本接口拥有的字段，按当前调参场景选择映射。 */
    const IspSceneParamMapping_S &stSceneMapping = get_scene_param_mapping(*m_pTuningProfile, get_tuning_scene());
    const IspParamMapping_S &stMapping = stSceneMapping.stSaturation;
    stCscAttr.satu = map_user_to_system(nSaturation, stMapping.nSystemMin, stMapping.nSystemMax, stMapping.nUserOffset);

    nRet = ss_mpi_isp_set_csc_attr(m_viPipe, &stCscAttr);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("设置ISP CSC属性失败, ret:%d", nRet);
        return IpcRet_E::ERR;
    }

    return nRet;
}

int CIspControl::get_saturation(unsigned int &pSaturation) const
{
    int nRet = IpcRet_E::OK;
    ot_isp_csc_attr stCscAttr;

    nRet = ss_mpi_isp_get_csc_attr(m_viPipe, &stCscAttr);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("获取ISP CSC属性失败, ret:%d", nRet);
        return IpcRet_E::ERR;
    }

    if (m_pTuningProfile == nullptr)
    {
        dlog_error("读取饱和度失败: ISP参数映射未注入");
        return ERR_UNINIT;
    }

    const IspSceneParamMapping_S &stSceneMapping = get_scene_param_mapping(*m_pTuningProfile, get_tuning_scene());
    const IspParamMapping_S &stMapping = stSceneMapping.stSaturation;
    pSaturation = map_system_to_user(stCscAttr.satu, stMapping.nSystemMin, stMapping.nSystemMax, stMapping.nUserOffset);

    return nRet;
}

int CIspControl::set_brightness(const unsigned int nBrightness)
{
    int nRet = IpcRet_E::OK;
    ot_isp_csc_attr stCscAttr;

    nRet = ss_mpi_isp_get_csc_attr(m_viPipe, &stCscAttr);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("获取ISP CSC属性失败, ret:%d", nRet);
        return IpcRet_E::ERR;
    }

    if (m_pTuningProfile == nullptr)
    {
        dlog_error("设置亮度失败: ISP参数映射未注入");
        return ERR_UNINIT;
    }

    /* 网页亮度按当前日夜运行场景映射到MPP CSC参数范围。 */
    const IspSceneParamMapping_S &stSceneMapping = get_scene_param_mapping(*m_pTuningProfile, get_tuning_scene());
    const BrightnessTuningProfile_S &stMapping = stSceneMapping.stBrightness;
    const unsigned int nMappedBrightness = map_user_to_system(nBrightness,
                                                              stMapping.nSystemMin,
                                                              stMapping.nSystemMax,
                                                              stMapping.nUserOffset);

    /* 整体亮度 */
    stCscAttr.luma = nMappedBrightness;
    nRet = ss_mpi_isp_set_csc_attr(m_viPipe, &stCscAttr);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("设置ISP CSC属性失败, ret:%d", nRet);
        return IpcRet_E::ERR;
    }

    return nRet;
}

int CIspControl::get_brightness(unsigned int &pBrightness) const
{
    int nRet = IpcRet_E::OK;
    ot_isp_csc_attr stCscAttr;

    nRet = ss_mpi_isp_get_csc_attr(m_viPipe, &stCscAttr);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("获取ISP CSC属性失败, ret:%d", nRet);
        return IpcRet_E::ERR;
    }

    if (m_pTuningProfile == nullptr)
    {
        dlog_error("读取亮度失败: ISP参数映射未注入");
        return ERR_UNINIT;
    }

    const IspSceneParamMapping_S &stSceneMapping = get_scene_param_mapping(*m_pTuningProfile, get_tuning_scene());
    const BrightnessTuningProfile_S &stMapping = stSceneMapping.stBrightness;
    pBrightness = map_system_to_user(stCscAttr.luma, stMapping.nSystemMin, stMapping.nSystemMax, stMapping.nUserOffset);

    return nRet;
}

int CIspControl::set_sharpness(const unsigned int nSharpen)
{
    int nRet = IpcRet_E::OK;
    ot_isp_sharpen_attr stSharpenAttr;

    nRet = ss_mpi_isp_get_sharpen_attr(m_viPipe, &stSharpenAttr);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("获取ISP锐度属性失败, ret:%d", nRet);
        return IpcRet_E::ERR;
    }

    if (m_pTuningProfile == nullptr)
    {
        dlog_error("设置锐度失败: ISP参数映射未注入");
        return ERR_UNINIT;
    }

    /* 当前运行场景同时决定锐度映射和手动/自动工作模式。 */
    const IspSceneParamMapping_S &stSceneMapping = get_scene_param_mapping(*m_pTuningProfile, get_tuning_scene());
    const SharpenTuningProfile_S &stProfile = stSceneMapping.stSharpen;
    const unsigned int nMappedSharpen = map_user_to_system(nSharpen, stProfile.nMin, stProfile.nMax, stProfile.nOffset);
    if (stProfile.bUseAutoMode)
    {
        /* 夜晚红外场景沿用原夜间自动锐度策略，限制自动锐度上限。 */
        stSharpenAttr.op_type = OT_OP_MODE_AUTO;
        dlog_info("op_type:AUTO");
        for (td_u8 i = 0; i < OT_ISP_AUTO_ISO_NUM; ++i)
        {
            stSharpenAttr.auto_attr.max_sharp_gain[i] = nMappedSharpen;
            dlog_info("AUTO gain[%d]: %d", i, nMappedSharpen);
        }
    }
    else
    {
        /* 白天与夜晚白光当前均使用手动锐度。 */
        stSharpenAttr.op_type = OT_OP_MODE_MANUAL;
        dlog_info("op_type:MANUAL");
        stSharpenAttr.manual_attr.max_sharp_gain = nMappedSharpen;
        dlog_info("MANUAL gain: %d", nMappedSharpen);
    }

    nRet = ss_mpi_isp_set_sharpen_attr(m_viPipe, &stSharpenAttr);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("设置ISP锐度属性失败, ret:%d", nRet);
        return IpcRet_E::ERR;
    }

    return nRet;
}

int CIspControl::get_sharpness(unsigned int &pSharpen) const
{
    int nRet = IpcRet_E::OK;
    ot_isp_sharpen_attr stSharpenAttr;

    nRet = ss_mpi_isp_get_sharpen_attr(m_viPipe, &stSharpenAttr);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("获取ISP锐度属性失败, ret:%d", nRet);
        return IpcRet_E::ERR;
    }

    if (m_pTuningProfile == nullptr)
    {
        dlog_error("读取锐度失败: ISP参数映射未注入");
        return ERR_UNINIT;
    }

    const IspSceneParamMapping_S &stSceneMapping = get_scene_param_mapping(*m_pTuningProfile, get_tuning_scene());
    const SharpenTuningProfile_S &stProfile = stSceneMapping.stSharpen;
    /* set_sharpness会把所有自动ISO档位设为相同上限，读取首档即可还原网页值。 */
    const unsigned int nSystemSharpen = stProfile.bUseAutoMode ? stSharpenAttr.auto_attr.max_sharp_gain[0]
                                                               : stSharpenAttr.manual_attr.max_sharp_gain;
    pSharpen = map_system_to_user(nSystemSharpen, stProfile.nMin, stProfile.nMax, stProfile.nOffset);
    return nRet;
}

int CIspControl::set_contrast(const unsigned int nContrast)
{
    int nRet = IpcRet_E::OK;
    ot_isp_csc_attr stCscAttr;

    nRet = ss_mpi_isp_get_csc_attr(m_viPipe, &stCscAttr);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("获取ISP CSC属性失败, ret:%d", nRet);
        return IpcRet_E::ERR;
    }

    if (m_pTuningProfile == nullptr)
    {
        dlog_error("设置对比度失败: ISP参数映射未注入");
        return ERR_UNINIT;
    }

    /* 对比度按当前调参场景映射到MPP CSC字段。 */
    const IspSceneParamMapping_S &stSceneMapping = get_scene_param_mapping(*m_pTuningProfile, get_tuning_scene());
    const IspParamMapping_S &stMapping = stSceneMapping.stContrast;
    stCscAttr.contr = map_user_to_system(nContrast, stMapping.nSystemMin, stMapping.nSystemMax, stMapping.nUserOffset);

    nRet = ss_mpi_isp_set_csc_attr(m_viPipe, &stCscAttr);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("设置ISP CSC属性失败, ret:%d", nRet);
        return IpcRet_E::ERR;
    }

    return nRet;
}

int CIspControl::get_contrast(unsigned int &pContrast) const
{
    int nRet = IpcRet_E::OK;
    ot_isp_csc_attr stCscAttr;

    nRet = ss_mpi_isp_get_csc_attr(m_viPipe, &stCscAttr);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("获取ISP CSC属性失败, ret:%d", nRet);
        return IpcRet_E::ERR;
    }

    if (m_pTuningProfile == nullptr)
    {
        dlog_error("读取对比度失败: ISP参数映射未注入");
        return ERR_UNINIT;
    }

    const IspSceneParamMapping_S &stSceneMapping = get_scene_param_mapping(*m_pTuningProfile, get_tuning_scene());
    const IspParamMapping_S &stMapping = stSceneMapping.stContrast;
    pContrast = map_system_to_user(stCscAttr.contr, stMapping.nSystemMin, stMapping.nSystemMax, stMapping.nUserOffset);
    return nRet;
}

int CIspControl::get_imageParam_attr(ImageParam_S &stImage) const
{
    int nRet = IpcRet_E::OK;
    /* 对比度 */
    nRet = get_contrast(stImage.nContrast);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("获取对比度失败, 对比度:%u, ret:%d", stImage.nContrast, nRet);
        return IpcRet_E::ERR;
    }
    /* 锐度 */
    nRet = get_sharpness(stImage.nSharpness);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("获取锐度失败, 锐度:%u, ret:%d", stImage.nSharpness, nRet);
        return IpcRet_E::ERR;
    }
    /* 亮度 */
    nRet = get_brightness(stImage.nBrightness);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("获取亮度失败, 亮度:%u, ret:%d", stImage.nBrightness, nRet);
        return IpcRet_E::ERR;
    }
    /* 饱和度 */
    nRet = get_saturation(stImage.nSaturation);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("获取饱和度失败, 饱和度:%u, ret:%d", stImage.nSaturation, nRet);
        return IpcRet_E::ERR;
    }
    return nRet;
}

int CIspControl::set_imageParam_attr(const ImageParam_S &stImage)
{
    int nRet = IpcRet_E::OK;
    /* 对比度 */
    /* step: 按颜色属性顺序逐项下发，任一项失败立即返回以保留可诊断的失败点。 */
    nRet = set_contrast(stImage.nContrast);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("设置对比度失败, 对比度:%u, ret:%d", stImage.nContrast, nRet);
        return IpcRet_E::ERR;
    }
    /* 锐度 */
    nRet = set_sharpness(stImage.nSharpness);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("设置锐度失败, 锐度:%u, ret:%d", stImage.nSharpness, nRet);
        return IpcRet_E::ERR;
    }
    /* 亮度 */
    nRet = set_brightness(stImage.nBrightness);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("设置亮度失败, 亮度:%u, ret:%d", stImage.nBrightness, nRet);
        return IpcRet_E::ERR;
    }
    /* 饱和度 */
    nRet = set_saturation(stImage.nSaturation);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("设置饱和度失败, 饱和度:%u, ret:%d", stImage.nSaturation, nRet);
        return IpcRet_E::ERR;
    }
    return nRet;
}

int CIspControl::set_wdr_attr(const WdrAttr_S &stWdrAttr)
{
    int nRet = IpcRet_E::OK;
    ot_isp_drc_attr stDrcInfo;
    /* 宽动态 */
    /* 读取完整 DRC 属性，仅修改宽动态启用和强度字段。 */
    nRet = ot_mpi_isp_get_drc_attr(m_viPipe, &stDrcInfo);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("获取ISP DRC属性失败, ret:%d", nRet);
        return IpcRet_E::ERR;
    }

    if (stWdrAttr.bEnable)
    {
        if (m_pTuningProfile == nullptr)
        {
            dlog_error("设置WDR失败: ISP参数映射未注入");
            return ERR_UNINIT;
        }

        const IspSceneParamMapping_S &stSceneMapping = get_scene_param_mapping(*m_pTuningProfile, get_tuning_scene());
        const WdrTuningProfile_S &stMapping = stSceneMapping.stWdr;
        const unsigned int nMappedWdr = map_user_to_system(stWdrAttr.nWdrLevel,
                                                           stMapping.nSystemMin,
                                                           stMapping.nSystemMax,
                                                           stMapping.nUserOffset);
        stDrcInfo.enable = TD_TRUE;
        stDrcInfo.op_type = OT_OP_MODE_MANUAL;
        stDrcInfo.manual_attr.strength = nMappedWdr;
    }
    else
    {
        stDrcInfo.enable = TD_FALSE;
    }

    nRet = ot_mpi_isp_set_drc_attr(m_viPipe, &stDrcInfo);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("设置ISP DRC属性失败, ret:%d", nRet);
        return IpcRet_E::ERR;
    }
    return nRet;
}

int CIspControl::get_wdr_attr(WdrAttr_S &stWdrAttr) const
{
    int nRet = IpcRet_E::OK;
    ot_isp_drc_attr stDrcInfo;
    /* 宽动态 */
    nRet = ot_mpi_isp_get_drc_attr(m_viPipe, &stDrcInfo);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("获取ISP DRC属性失败, ret:%d", nRet);
        return IpcRet_E::ERR;
    }

    if (m_pTuningProfile == nullptr)
    {
        dlog_error("读取WDR失败: ISP参数映射未注入");
        return ERR_UNINIT;
    }

    stWdrAttr.bEnable = (stDrcInfo.enable == TD_TRUE);
    const IspSceneParamMapping_S &stSceneMapping = get_scene_param_mapping(*m_pTuningProfile, get_tuning_scene());
    const WdrTuningProfile_S &stMapping = stSceneMapping.stWdr;
    stWdrAttr.nWdrLevel = map_system_to_user(stDrcInfo.manual_attr.strength,
                                             stMapping.nSystemMin,
                                             stMapping.nSystemMax,
                                             stMapping.nUserOffset);
    return OK;
}

int CIspControl::set_hls_attr(const HlsAttr_S &stHlsAttr)
{
    int nRet = IpcRet_E::OK;
    ot_isp_exposure_attr exp_attr;

    if (m_pTuningProfile == nullptr)
    {
        dlog_error("设置强光抑制失败: ISP参数映射未注入");
        return ERR_UNINIT;
    }

    /* 读取完整曝光属性，避免覆盖其他设置。 */
    nRet = ot_mpi_isp_get_exposure_attr(m_viPipe, &exp_attr);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("获取ISP曝光属性失败, ret:%d", nRet);
        return IpcRet_E::ERR;
    }

    const IspSceneParamMapping_S &stSceneMapping = get_scene_param_mapping(*m_pTuningProfile, get_tuning_scene());
    const HlcTuningProfile_S &stProfile = stSceneMapping.stHlc;
    /* 强光抑制功能控制MPP曝光tolerance字段；配置表达网页100对应的MPP下限。 */
    unsigned int tolerance = stProfile.nDisabledValue;
    if (stHlsAttr.bEnable)
    {
        /* 网页等级为有符号值，必须先限制范围，避免负值转换为无符号大数。 */
        const unsigned int nUserLevel = static_cast<unsigned int>(
            std::clamp(stHlsAttr.nHlsLevel, ISP_USER_VALUE_MIN, ISP_USER_VALUE_MAX));
        const unsigned int nRange = stProfile.nSystemMax - stProfile.nSystemMin;
        tolerance = stProfile.nSystemMax - (nUserLevel * nRange) / ISP_USER_VALUE_MAX;
    }

    /* 设置曝光tolerance参数 */
    exp_attr.auto_attr.tolerance = tolerance;

    nRet = ot_mpi_isp_set_exposure_attr(m_viPipe, &exp_attr);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("设置ISP曝光属性失败, ret:%d", nRet);
        return IpcRet_E::ERR;
    }

    return nRet;
}

int CIspControl::get_hls_attr(HlsAttr_S &stHlsAttr) const
{
    /* HLC关闭值可能与开启状态的有效等级重合，不能由MPP tolerance反推网页开关。 */
    BackLightArrt_S stStoredBacklight;
    const int nRet = CIspConfigure::instance()->get_configure(stStoredBacklight);
    if (nRet != OK)
    {
        dlog_error("读取持久化强光抑制配置失败, ret:%d", nRet);
        return nRet;
    }

    /* 配置存储是网页参数的唯一事实来源，保留用户明确保存的开关和等级。 */
    stHlsAttr = stStoredBacklight.stHlsAttr;
    return OK;
}

static void configure_backlight_weights(BackLightArea_E enBackLightArea, ot_isp_ae_stats_cfg &ae_stats_cfg)
{
    /* 初始化所有测光区域权重，避免上一次背光区域配置残留。 */
    for (td_u8 row = 0; row < OT_ISP_AE_ZONE_ROW; row++)
    {
        for (td_u8 col = 0; col < OT_ISP_AE_ZONE_COLUMN; col++)
        {
            ae_stats_cfg.weight[row][col] = BACKLIGHT_WEIGHT_DEFAULT;
        }
    }

    /* 根据用户选择的背光区域提高对应测光权重。 */
    /* 区域坐标以MPP AE网格为准；加权区域必须在行列上对称保持测光稳定。 */
    switch (enBackLightArea)
    {
    case UP: /* 上区域：前 6 行 */
        for (td_u8 row = 0; row < 6; row++)
        {
            for (td_u8 col = 0; col < OT_ISP_AE_ZONE_COLUMN; col++)
            {
                ae_stats_cfg.weight[row][col] = BACKLIGHT_WEIGHT_VALUE;
            }
        }
        break;

    case DOWN: /* 下区域：后 6行 */
        for (td_u8 row = OT_ISP_AE_ZONE_ROW - 6; row < OT_ISP_AE_ZONE_ROW; row++)
        {
            for (td_u8 col = 0; col < OT_ISP_AE_ZONE_COLUMN; col++)
            {
                ae_stats_cfg.weight[row][col] = BACKLIGHT_WEIGHT_VALUE;
            }
        }
        break;

    case LEFT: /* 左区域：前 6 列 */
        for (td_u8 col = 0; col < 6; col++)
        {
            for (td_u8 row = 0; row < OT_ISP_AE_ZONE_ROW; row++)
            {
                ae_stats_cfg.weight[row][col] = BACKLIGHT_WEIGHT_VALUE;
            }
        }
        break;

    case RIGHT: /* 右区域：后 6 列 */
        for (td_u8 col = OT_ISP_AE_ZONE_COLUMN - 6; col < OT_ISP_AE_ZONE_COLUMN; col++)
        {
            for (td_u8 row = 0; row < OT_ISP_AE_ZONE_ROW; row++)
            {
                ae_stats_cfg.weight[row][col] = BACKLIGHT_WEIGHT_VALUE;
            }
        }
        break;

    case CENTER_BACKLIGHT: /* 中心区域：中间 3 行 5 列 */
        for (td_u8 row = (OT_ISP_AE_ZONE_ROW / 2) - 1; row <= (OT_ISP_AE_ZONE_ROW / 2) + 1; row++)
        {
            for (td_u8 col = (OT_ISP_AE_ZONE_COLUMN / 2) - 2; col <= (OT_ISP_AE_ZONE_COLUMN / 2) + 2; col++)
            {
                ae_stats_cfg.weight[row][col] = BACKLIGHT_WEIGHT_VALUE;
            }
        }
        break;

    default:
        break;
    }
}

/**************************图像参数控制结束 ************************/

/**************************高级参数控制 ************************/
int CIspControl::get_exposure_attr(ExposureAttr_S &stExpAttr) const
{
    int nRet = IpcRet_E::OK;
    ot_isp_exposure_attr exp_attr;

    nRet = ot_mpi_isp_get_exposure_attr(m_viPipe, &exp_attr);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("获取ISP曝光属性失败, ret:%d", nRet);
        return IpcRet_E::ERR;
    }

    switch (exp_attr.op_type)
    {
    case OT_OP_MODE_AUTO:
    {
        /* 获取防横纹开关状态 */
        stExpAttr.bAntiBanding = (exp_attr.auto_attr.antiflicker.enable == TD_TRUE);
        break;
    }
    case OT_OP_MODE_MANUAL:
    {
        exp_attr.op_type = OT_OP_MODE_MANUAL;
        /* 曝光时长设置为手动 */
        exp_attr.manual_attr.exp_time_op_type = OT_OP_MODE_MANUAL;
        /* 曝光时间 */
        exp_attr.manual_attr.exp_time = exposureTimeMapping[static_cast<uint8_t>(stExpAttr.enExpTime)];
        exp_attr.auto_attr.antiflicker.enable = TD_FALSE;
        break;
    }
    default:
        dlog_error("不支持的曝光工作模式: %d", exp_attr.op_type);
        return IpcRet_E::ERR;
    }

    return nRet;
}

int CIspControl::set_exposure_attr(const ExposureAttr_S &stExpAttr)
{
    int nRet = IpcRet_E::OK;
    ot_isp_exposure_attr exp_attr;

    /* 曝光完整属性快照保留 AE 的其他控制字段，仅更新防闪烁或补偿档位。 */
    nRet = ot_mpi_isp_get_exposure_attr(m_viPipe, &exp_attr);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("获取ISP曝光属性失败, ret:%d", nRet);
        return IpcRet_E::ERR;
    }

    if (stExpAttr.bAntiBanding)
    {
        exp_attr.op_type = OT_OP_MODE_AUTO;
        /* 防横纹开关,即抗闪烁功能 */
        exp_attr.auto_attr.antiflicker.enable = TD_TRUE;
        exp_attr.auto_attr.antiflicker.frequency = 50;
        exp_attr.auto_attr.antiflicker.mode = OT_ISP_ANTIFLICKER_NORMAL_MODE;
    }
    else
    {
        /* 关闭防横纹功能，关闭antiflicker使能 */
        exp_attr.auto_attr.antiflicker.enable = TD_FALSE;

        if (m_pTuningProfile == nullptr)
        {
            dlog_error("设置曝光补偿失败: ISP参数映射未注入");
            return ERR_UNINIT;
        }

        /* memory: 指针仅借用只读画像内数组，set调用结束前保持有效。 */
        const std::array<int, ISP_EXPOSURE_COMPENSATION_LEVEL_COUNT>
            &stExposureCompensation = m_pTuningProfile->stExposureCompensation[get_tuning_scene_index(get_tuning_scene())];
        /* 根据白天/夜晚白光/夜晚红外场景设置曝光补偿 */
        exp_attr.auto_attr.compensation = stExposureCompensation[static_cast<uint8_t>(stExpAttr.enExpTime)];
    }

    nRet = ot_mpi_isp_set_exposure_attr(m_viPipe, &exp_attr);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("设置ISP曝光属性失败, ret:%d", nRet);
        return IpcRet_E::ERR;
    }

    return nRet;
}

int CIspControl::apply_gamma_attr()
{
    if (m_pTuningProfile == nullptr || !m_pTuningProfile->bUseCmosGamma)
    {
        return IpcRet_E::OK;
    }

    /* Gamma 属性快照用于校验模块可读；最终整体替换为当前场景的目标曲线。 */
    ot_isp_gamma_attr stGammaAttr;
    int nRet = ot_mpi_isp_get_gamma_attr(m_viPipe, &stGammaAttr);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("获取ISP Gamma属性失败, ret:%d", nRet);
        return IpcRet_E::ERR;
    }

    /* 当前调参场景决定白天或红外 Gamma 曲线，夜白沿用白天曲线。 */
    const IspTuningScene_E enScene = get_tuning_scene();
    const ot_isp_gamma_attr *pTargetGammaAttr = (enScene == IspTuningScene_E::NIGHT_IR) ? m_pTuningProfile->pGammaIr
                                                                                        : m_pTuningProfile->pGammaDay;
    if (pTargetGammaAttr == nullptr)
    {
        return IpcRet_E::OK;
    }

    /* 保留 get 接口校验 ISP Gamma 模块可读后，整体覆盖为当前场景目标曲线。 */
    stGammaAttr = *pTargetGammaAttr;
    nRet = ot_mpi_isp_set_gamma_attr(m_viPipe, &stGammaAttr);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("设置ISP Gamma属性失败, ret:%d", nRet);
        return IpcRet_E::ERR;
    }

    dlog_info("Gamma参数应用成功，调参场景: %d", static_cast<int>(enScene));
    return nRet;
}

int CIspControl::get_awb_attr(AwbAttr_S &stAwbInfo) const
{
    int nRet = IpcRet_E::OK;
    ot_isp_wb_attr wb_attr;
    nRet = ss_mpi_isp_get_wb_attr(m_viPipe, &wb_attr);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("获取ISP白平衡属性失败, ret:%d", nRet);
        return IpcRet_E::ERR;
    }

    switch (wb_attr.op_type)
    {
    /* 自动白平衡 */
    case OT_OP_MODE_AUTO:
        stAwbInfo.enAwbMode = AUTO_AWB_MODE;
        break;
    /* 手动白平衡 */
    case OT_OP_MODE_MANUAL:
    {
        stAwbInfo.enAwbMode = MANUAL_AWB_MODE;
        stAwbInfo.nRGain = wb_attr.manual_attr.r_gain;
        stAwbInfo.nBGain = wb_attr.manual_attr.b_gain;
        break;
    }
    default:
        break;
    }

    return nRet;
}

int CIspControl::set_awb_attr(const AwbAttr_S &stAwbInfo)
{
    int nRet = IpcRet_E::OK;
    ot_isp_wb_attr wb_attr;

    if (m_pTuningProfile == nullptr)
    {
        dlog_error("设置白平衡失败: ISP参数映射未注入");
        return ERR_UNINIT;
    }

    /* 读取白平衡属性后仅按用户模式更新控制方式和相关增益。 */
    nRet = ss_mpi_isp_get_wb_attr(m_viPipe, &wb_attr);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("获取ISP白平衡属性失败, ret:%d", nRet);
        return IpcRet_E::ERR;
    }

    const IspSceneParamMapping_S &stSceneMapping = get_scene_param_mapping(*m_pTuningProfile, get_tuning_scene());
    const AwbTuningProfile_S &stProfile = stSceneMapping.stAwb;
    /* 预设色温模式写入配置中的标定增益，手动模式使用网页连续增益映射。 */
    switch (stAwbInfo.enAwbMode)
    {
    /* 自动白平衡 */
    case AUTO_AWB_MODE:
        wb_attr.op_type = OT_OP_MODE_AUTO;
        break;
    /* 手动白平衡 */
    case MANUAL_AWB_MODE:
        wb_attr.op_type = OT_OP_MODE_MANUAL;
        wb_attr.manual_attr.r_gain = static_cast<td_u16>(map_user_to_system(stAwbInfo.nRGain,
                                                                            stProfile.stManualGain.nSystemMin,
                                                                            stProfile.stManualGain.nSystemMax,
                                                                            stProfile.stManualGain.nUserOffset));
        wb_attr.manual_attr.b_gain = static_cast<td_u16>(map_user_to_system(stAwbInfo.nBGain,
                                                                            stProfile.stManualGain.nSystemMin,
                                                                            stProfile.stManualGain.nSystemMax,
                                                                            stProfile.stManualGain.nUserOffset));
        break;
    /* 锁定白平衡 */
    case LOCK_AWB_MODE:
        apply_awb_preset_gain(wb_attr, stProfile.stLock);
        break;
    /* 白炽灯 */
    case INCANDESCENT_MODE:
        apply_awb_preset_gain(wb_attr, stProfile.stIncandescent);
        break;
    /* 暖光灯 */
    case WARM_MODE:
        apply_awb_preset_gain(wb_attr, stProfile.stWarm);
        break;
    /* 日光灯 */
    case FLUORESCENT_MODE:
        apply_awb_preset_gain(wb_attr, stProfile.stFluorescent);
        break;
    /* 自然灯 */
    case DAY_LIGHT_MODE:
        apply_awb_preset_gain(wb_attr, stProfile.stDaylight);
        break;
    default:
        break;
    }

    nRet = ss_mpi_isp_set_wb_attr(m_viPipe, &wb_attr);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("设置ISP白平衡属性失败, ret:%d", nRet);
        return IpcRet_E::ERR;
    }

    return nRet;
}

int CIspControl::get_nr_attr(DnrAttr_S &stDnrInfo) const
{
    int nRet = IpcRet_E::OK;
    ot_3dnr_param nr_attr;
    /* 数字降噪 */
    nRet = ss_mpi_vpss_get_grp_3dnr_param(m_viPipe, &nr_attr);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("获取VPSS 3D降噪参数失败, ret:%d", nRet);
        return IpcRet_E::ERR;
    }

    return nRet;
}

int CIspControl::set_nr_attr(const DnrAttr_S &stDnrInfo)
{
    int nRet = IpcRet_E::OK;
    /* VI 3D 降噪参数快照承载强度曲线，避免设置使能状态时清除其他降噪字段。 */
    ot_3dnr_param nr_attr;
    ot_3dnr_attr attr;

    if (m_pTuningProfile == nullptr)
    {
        dlog_error("设置降噪失败: ISP参数映射未注入");
        return ERR_UNINIT;
    }

    /* 数字降噪 */
    nRet = ss_mpi_vi_get_pipe_3dnr_param(m_viPipe, &nr_attr);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("获取VI 3D降噪参数失败, ret:%d", nRet);
        return IpcRet_E::ERR;
    }

    nRet = ss_mpi_vi_get_pipe_3dnr_attr(m_viPipe, &attr);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("获取VI 3D降噪属性失败, ret:%d", nRet);
        return IpcRet_E::ERR;
    }

    const IspSceneParamMapping_S &stSceneMapping = get_scene_param_mapping(*m_pTuningProfile, get_tuning_scene());
    const NrTuningProfile_S &stProfile = stSceneMapping.stNr;
    /* 普通和高级模式共用配置中的SNR/TNR转换，DNR仍按历史行为直写。 */
    switch (stDnrInfo.enDnrMode)
    {
    /* 关闭 */
    case CLOSE_MODE:
        attr.enable = TD_FALSE;
        break;
    /* 普通模式 */
    case NORMAL_MODE:
        attr.enable = TD_TRUE;
        apply_nr_levels(nr_attr, stDnrInfo, stProfile);
        break;
    /* 专家模式 */
    case ADVANCED_MODE:
        attr.enable = TD_TRUE;
        apply_nr_levels(nr_attr, stDnrInfo, stProfile);
        break;
    default:
        break;
    }

    /* step: 先更新降噪开关，再下发关联的参数表，保持 VI 模块状态与参数一致。 */
    nRet = ss_mpi_vi_set_pipe_3dnr_attr(m_viPipe, &attr);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("设置VI 3D降噪属性失败, ret:%d", nRet);
        return IpcRet_E::ERR;
    }

    nRet = ss_mpi_vi_set_pipe_3dnr_param(m_viPipe, &nr_attr);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("设置VI 3D降噪参数失败, ret:%d", nRet);
        return IpcRet_E::ERR;
    }

    return nRet;
}

int CIspControl::get_backLight_attr(BackLightArrt_S &stBackAttr) const
{
    int nRet = IpcRet_E::OK;

    /* 背光区域 */

    /* 宽动态 */
    nRet = get_wdr_attr(stBackAttr.stWdrAttr);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("获取宽动态配置失败, ret:%d", nRet);
        return nRet;
    }

    /* 强光抑制 */
    nRet = get_hls_attr(stBackAttr.stHlsAttr);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("获取强光抑制配置失败, ret:%d", nRet);
        return nRet;
    }

    return nRet;
}

int CIspControl::set_backLight_attr(const BackLightArrt_S &stBackAttr)
{
    int nRet = IpcRet_E::OK;

    if (m_pTuningProfile == nullptr)
    {
        dlog_error("设置背光配置失败: ISP参数映射未注入");
        return ERR_UNINIT;
    }

    /* AE 统计配置快照用于调整背光测光权重，其他测光设置必须保留。 */
    ot_isp_stats_cfg stat_cfg;

    nRet = ot_mpi_isp_get_stats_cfg(m_viPipe, &stat_cfg);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("获取ISP统计配置失败, ret:%d", nRet);
        return IpcRet_E::ERR;
    }
    /* 宽动态和背光补偿同时启用时，仅宽动态生效 */
    /* WDR 和区域背光均影响曝光统计，策略规定 WDR 开启时不叠加区域权重。 */
    if (!stBackAttr.stWdrAttr.bEnable && stBackAttr.enBackLightArea != CLOSE)
    {
        /* 背光区域 */
        /* 仅在未启用宽动态时改变区域权重，避免两个曝光策略同时竞争。 */
        configure_backlight_weights(stBackAttr.enBackLightArea, stat_cfg.ae_cfg);
        /* 背光区域设置 */
        nRet = ot_mpi_isp_set_stats_cfg(m_viPipe, &stat_cfg);
        if (nRet != IpcRet_E::OK)
        {
            dlog_error("设置ISP统计配置失败, ret:%d", nRet);
            return IpcRet_E::ERR;
        }
    }
    else
    {
        /* 宽动态 */
        nRet = set_wdr_attr(stBackAttr.stWdrAttr);
        if (nRet != IpcRet_E::OK)
        {
            dlog_error("设置宽动态配置失败, ret:%d", nRet);
            return nRet;
        }

        /* 关闭背光补偿区域后，重新写入一次当前场景的ini文件 */
        /* 获取当前运行场景并重新应用场景配置 */
        ISP::IspRuntimeScene_E enCurrentScene = CSceneParamManager::instance()->scene_get_mode();
        nRet = CSceneParamManager::instance()->scene_set_mode(enCurrentScene, *m_pTuningProfile);
        if (nRet != IpcRet_E::OK)
        {
            dlog_error("重新应用场景配置失败, runtime_scene:%d, ret:%d", static_cast<int>(enCurrentScene), nRet);
            return nRet;
        }
    }

    /* 强光抑制 */
    nRet = set_hls_attr(stBackAttr.stHlsAttr);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("设置强光抑制配置失败, ret:%d", nRet);
        return nRet;
    }

    return nRet;
}

int CIspControl::get_videoMirror_attr(VideoAdjust_S &stVideoAdjust) const
{
    int nRet = OK;
    /* memory: viHandle 归视频流模块所有，只在本函数内读取其扩展参数。 */
    auto viHandle = CStreamVideo::instance()->get_viHandle();
    if (viHandle == nullptr)
    {
        dlog_error("获取VI句柄失败");
        return ERR;
    }
    auto &stExParam = viHandle->stExParam;

    /* 根据底层镜像和翻转开关判断当前镜像模式。 */
    if (!stExParam.bMirror && !stExParam.bFlip)
    {
        /* 关闭镜像翻转 */
        stVideoAdjust.enMirrorMode = DISABLE;
    }
    else if (stExParam.bMirror && !stExParam.bFlip)
    {
        /* 镜像左右翻转 */
        stVideoAdjust.enMirrorMode = HORIZONTAL;
    }
    else if (!stExParam.bMirror && stExParam.bFlip)
    {
        /* 镜像上下翻转 */
        stVideoAdjust.enMirrorMode = VERTICAL;
    }
    else if (stExParam.bMirror && stExParam.bFlip)
    {
        /* 镜像中心翻转 */
        stVideoAdjust.enMirrorMode = CENTER;
    }
    else
    {
        dlog_error("无效的镜像状态组合, 镜像:%d, 翻转:%d", stExParam.bMirror, stExParam.bFlip);
        return ERR;
    }

    return nRet;
}

int CIspControl::set_videoMirror_attr(const VideoAdjust_S &stVideoAdjust)
{
    int nRet = OK;
    /* memory: VI 句柄归视频模块所有，此处只借用以更新传感器镜像/翻转扩展参数。 */
    auto viHandle = CStreamVideo::instance()->get_viHandle();
    if (viHandle == nullptr)
    {
        dlog_error("获取VI句柄失败");
        return ERR;
    }
    auto &stExParam = viHandle->stExParam;
    /* 吊装机型需要反转用户镜像语义，使最终画面方向与安装姿态一致。 */
    const bool bCeilingMount = (m_pTuningProfile != nullptr && m_pTuningProfile->stMirror.bCeilingMount);

    switch (stVideoAdjust.enMirrorMode)
    {
    /* 关闭 */
    case DISABLE:
        dlog_info("关闭镜像翻转");
        stExParam.bMirror = bCeilingMount ? TD_TRUE : TD_FALSE;
        stExParam.bFlip = bCeilingMount ? TD_TRUE : TD_FALSE;
        break;
    /* 左右 */
    case HORIZONTAL:
        dlog_info("镜像左右翻转");
        stExParam.bMirror = bCeilingMount ? TD_FALSE : TD_TRUE;
        stExParam.bFlip = bCeilingMount ? TD_TRUE : TD_FALSE;
        break;
    /* 上下 */
    case VERTICAL:
        dlog_info("镜像上下翻转");
        stExParam.bMirror = bCeilingMount ? TD_TRUE : TD_FALSE;
        stExParam.bFlip = bCeilingMount ? TD_FALSE : TD_TRUE;
        break;
    /* 中心 */
    case CENTER:
        dlog_info("镜像中心翻转");
        stExParam.bMirror = bCeilingMount ? TD_FALSE : TD_TRUE;
        stExParam.bFlip = bCeilingMount ? TD_FALSE : TD_TRUE;
        break;
    default:
        dlog_error("无效的镜像模式: %u", stVideoAdjust.enMirrorMode);
        return ERR;
    }

    /* 将修改后的扩展参数一次性提交给传感器，避免镜像与翻转分别生效造成画面跳变。 */
    nRet = viHandle->mppVi_set_sensor_mirror_flip(viHandle);
    if (nRet != OK)
    {
        dlog_error("设置传感器镜像翻转失败, ret:%d", nRet);
        return ERR;
    }

    return OK;
}

int CIspControl::get_dayNight_attr(DayNightAttr_S &stDayNightAttr) const
{
    return CIspConfigure::instance()->get_configure(stDayNightAttr);
}

int CIspControl::set_dayNight_attr(const DayNightAttr_S &stDayNightAttr)
{
    /* note: mode/time/filter和运行场景由共享核心负责，此处只更新AUTO观测灵敏度。 */
    CDayNightController::instance()->setSensitivity(stDayNightAttr.nSensitivityLevel);
    return IpcRet_E::OK;
}

/**************************高级参数控制结束 ************************/
