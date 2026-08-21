/**
 * @FilePath     : isp_parameter_applier_hi3516.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-13 14:11:02
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-17 13:40:47
 * @Description  : Hi3516 ISP参数适配端口实现
 */

#include "isp_parameter_applier_hi3516.h"

#include "isp_control.h"

int CIspParameterApplierHi3516::set_runtime_scene_context(ISP::IspRuntimeScene_E enRuntimeScene)
{
    return CIspControl::instance()->set_runtime_scene_context(enRuntimeScene);
}

int CIspParameterApplierHi3516::apply_image(const ISP::ImageParam_S &stConfig)
{
    return CIspControl::instance()->set_imageParam_attr(stConfig);
}

int CIspParameterApplierHi3516::apply_exposure(const ISP::ExposureAttr_S &stConfig)
{
    return CIspControl::instance()->set_exposure_attr(stConfig);
}

int CIspParameterApplierHi3516::apply_backlight(const ISP::BackLightArrt_S &stConfig)
{
    return CIspControl::instance()->set_backLight_attr(stConfig);
}

int CIspParameterApplierHi3516::apply_awb(const ISP::AwbAttr_S &stConfig)
{
    return CIspControl::instance()->set_awb_attr(stConfig);
}

int CIspParameterApplierHi3516::apply_nr(const ISP::DnrAttr_S &stConfig)
{
    return CIspControl::instance()->set_nr_attr(stConfig);
}

int CIspParameterApplierHi3516::apply_mirror(const ISP::VideoAdjust_S &stConfig)
{
    return CIspControl::instance()->set_videoMirror_attr(stConfig);
}

int CIspParameterApplierHi3516::on_scene_applied(ISP::IspRuntimeScene_E enRuntimeScene)
{
    /* CIspControl::apply_gamma_attr内部根据tuning profile决定是否执行Gamma。
     * scene参数预留给未来场景特定后处理扩展。 */
    (void) enRuntimeScene;
    return CIspControl::instance()->apply_gamma_attr();
}
