/**
 * @FilePath     : isp_parameter_applier_rv1126b.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-22 15:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-11 13:51:17
 * @Description  : RV1126B ISP参数适配端口实现
 */

#include "isp_parameter_applier_rv1126b.h"

#include "IpcRet.h"
#include "isp_control.h"

int CIspParameterApplierRv1126b::set_runtime_scene_context(ISP::IspRuntimeScene_E enRuntimeScene)
{
    /* step: 先保存共享层明确的运行场景，供参数重放完成钩子校验时序一致性。 */
    switch (enRuntimeScene)
    {
    case ISP::IspRuntimeScene_E::DAY:
    case ISP::IspRuntimeScene_E::NIGHT_WHITE:
    case ISP::IspRuntimeScene_E::NIGHT_LIGHT_OFF:
        m_enRuntimeScene = enRuntimeScene;
        return OK;
    default:
        return ERR_UNSUPPORT;
    }
}

int CIspParameterApplierRv1126b::apply_image(const ISP::ImageParam_S &stConfig)
{
    /* note: 参数校验和RK AIQ错误码由共享层/CIspControl负责，本类不复制配置。 */
    return CIspControl::instance()->set_imageParam_attr(stConfig);
}

int CIspParameterApplierRv1126b::apply_exposure(const ISP::ExposureAttr_S &stConfig)
{
    return CIspControl::instance()->set_exposure_attr(stConfig);
}

int CIspParameterApplierRv1126b::apply_backlight(const ISP::BackLightArrt_S &stConfig)
{
    return CIspControl::instance()->set_backLight_attr(stConfig);
}

int CIspParameterApplierRv1126b::apply_awb(const ISP::AwbAttr_S &stConfig)
{
    return CIspControl::instance()->set_awb_attr(stConfig);
}

int CIspParameterApplierRv1126b::apply_nr(const ISP::DnrAttr_S &stConfig)
{
    return CIspControl::instance()->set_nr_attr(stConfig);
}

int CIspParameterApplierRv1126b::apply_mirror(const ISP::VideoAdjust_S &stConfig)
{
    return CIspControl::instance()->set_videoMirror_attr(stConfig);
}

int CIspParameterApplierRv1126b::on_scene_applied(ISP::IspRuntimeScene_E enRuntimeScene)
{
    /* note: RV三个运行场景共用normal/day，但仍必须校验共享层的重放上下文。 */
    switch (enRuntimeScene)
    {
    case ISP::IspRuntimeScene_E::DAY:
    case ISP::IspRuntimeScene_E::NIGHT_WHITE:
    case ISP::IspRuntimeScene_E::NIGHT_LIGHT_OFF:
        break;
    default:
        /* RV1126B没有红外或独立低照IQ，适配器边界也必须拒绝不可达场景。 */
        return ERR_UNSUPPORT;
    }

    /* RV1126B三个可达运行场景共用normal/day IQ，不存在Gamma等场景后处理。 */
    return enRuntimeScene == m_enRuntimeScene ? OK : ERR_PARAM;
}
