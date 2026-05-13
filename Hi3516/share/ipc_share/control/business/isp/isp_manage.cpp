/**
 * @FilePath     : isp_manage.cpp
 * @Author       : cyc
 * @Date         : 2025-08-27 09:51:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-29 14:08:06
 * @Description  : 图像管理模块
 */

#include "isp_manage.h"
#include "isp_configure.h"
#include "isp_control.h"
#include "isp_sceneCtrl.h"
#include "dlog.h"
#include "IpcRet.h"
#include "isp_scene.h"
#include "isp_dayNight.h"
#include "isp_light.h"
#include "gpio_ctrl.h"
#include "pwm_ctrl.h"
#include "light_manager.h"
#include <unistd.h>

namespace
{
/**
 * @brief   : 比较两组单灯属性是否完全一致
 * @param    {ISP::Light_S} stLeft：待比较的左侧灯光属性
 * @param    {ISP::Light_S} stRight：待比较的右侧灯光属性
 * @return   {bool} true：一致，false：不一致
 */
bool is_light_attr_same(const ISP::Light_S& stLeft, const ISP::Light_S& stRight)
{
    return stLeft.bEnable == stRight.bEnable && stLeft.nLightLevel == stRight.nLightLevel;
}

/**
 * @brief   : 比较两组补光类型是否一致
 * @param    {ISP::FillLight_S} stLeft：待比较的左侧补光配置
 * @param    {ISP::FillLight_S} stRight：待比较的右侧补光配置
 * @return   {bool} true：一致，false：不一致
 */
bool is_fill_light_type_same(const ISP::FillLight_S& stLeft, const ISP::FillLight_S& stRight)
{
    return stLeft.enLightType == stRight.enLightType;
}

/**
 * @brief   : 比较两组补光亮度参数是否一致
 * @param    {ISP::FillLight_S} stLeft：待比较的左侧补光配置
 * @param    {ISP::FillLight_S} stRight：待比较的右侧补光配置
 * @return   {bool} true：一致，false：不一致
 * @note    : 这里只比较白光和红外的亮度相关参数，不比较补光类型
 */
bool is_fill_light_level_same(const ISP::FillLight_S& stLeft, const ISP::FillLight_S& stRight)
{
    return is_light_attr_same(stLeft.stWhiteAttr, stRight.stWhiteAttr) &&
           is_light_attr_same(stLeft.stRedAttr, stRight.stRedAttr);
}

/**
 * @brief   : 判断夜晚运行态下的补光相关配置是否发生变化
 * @param    {ISP::DayNightAttr_S} stOldDayNightAttr：更新前的日夜配置
 * @param    {ISP::DayNightAttr_S} stNewDayNightAttr：更新后的日夜配置
 * @return   {bool} true：已变化，false：未变化
 * @note    : 自动亮度模式下，用户调整 WhiteAttr/RedAttr 的 LightLevel 不应立即下发到硬件；
 *            只有补光类型变化，或者处于手动亮度模式时亮度参数变化，才需要触发夜晚重同步
 */
bool has_night_light_runtime_changed(const ISP::DayNightAttr_S& stOldDayNightAttr,
                                     const ISP::DayNightAttr_S& stNewDayNightAttr)
{
    if (stOldDayNightAttr.enLightMode != stNewDayNightAttr.enLightMode)
    {
        return true;
    }

    if (!is_fill_light_type_same(stOldDayNightAttr.stFillLight, stNewDayNightAttr.stFillLight))
    {
        return true;
    }

    if (stNewDayNightAttr.enLightMode == ISP::LightBrightMode_E::MANUAL_LIGHT_BRIGHT &&
        !is_fill_light_level_same(stOldDayNightAttr.stFillLight, stNewDayNightAttr.stFillLight))
    {
        return true;
    }

    return false;
}

/**
 * @brief   : IR-CUT 动作前关闭相反补光灯，避免出现Pwm灯光控制异常情况
 * @param    {ISP::LightType_E} enLightType：需要关闭的补光灯类型
 * @param    {const char*} pLightName：日志使用的补光灯名称
 * @return   {void}
 * @note    : 关闭失败不阻断 IR-CUT 动作，避免灯光控制异常导致日夜切换完全失效
 */
void turn_off_light_before_ircut(ISP::LightType_E enLightType, const char* pLightName)
{
    int nRet = CPwmCtrl::instance()->light_turn_off(enLightType);
    if (nRet != OK)
    {
        dlog_warn("IR-CUT动作前关闭%s失败: %d", pLightName, nRet);
        return;
    }

    /* 等待 PWM 关闭写入生效，避免与 IR-CUT 线圈动作抢占电源余量。 */
    usleep(100000);
}
} // namespace

CIspManage::CIspManage()
{
}

CIspManage::~CIspManage()
{
}

int CIspManage::init()
{
    int nRet = OK;

    /* 初始化场景参数 */
    CSceneParamManager::instance()->scene_init(ISP_CONFIG_PATH);

    /* 初始化日夜切换控制器并设置回调 */
    nRet = init_daynight_controller();
    if (nRet != OK)
    {
        dlog_error("日夜切换控制器初始化失败: %d", nRet);
        return nRet;
    }

    /* 从配置文件加载所有图像参数并应用到硬件 */
    nRet = load_and_apply_all_configs();
    if (nRet != OK)
    {
        dlog_error("加载配置失败: %d", nRet);
        return nRet;
    }

    dlog_info("=== 图像管理模块初始化完成 ===");
    return nRet;
}

int CIspManage::deinit()
{
    int nRet = OK;

    dlog_info("=== 图像管理模块去初始化 ===");

    /* 停止日夜切换控制器 */
    CDayNightController::instance()->stop();

    /* 场景去初始化 */
    CSceneParamManager::instance()->scene_deinit();

    return nRet;
}

int CIspManage::update_config(const ISP::PicConfigureType_E& enConfigType)
{
    int nRet = OK;

    switch (enConfigType)
    {
    case ISP::PicConfigureType_E::DAYNIGHT:
        nRet = apply_daynight_config();
        break;

    case ISP::PicConfigureType_E::IAMGE:
        nRet = apply_image_config();
        break;

    case ISP::PicConfigureType_E::EXPOSURE:
        nRet = apply_exposure_config();
        break;

    case ISP::PicConfigureType_E::BACKLIGHT:
        nRet = apply_backlight_config();
        break;

    case ISP::PicConfigureType_E::AWB:
        nRet = apply_awb_config();
        break;

    case ISP::PicConfigureType_E::NR:
        nRet = apply_dnr_config();
        break;

    case ISP::PicConfigureType_E::MIRROR:
        nRet = apply_mirror_config();
        break;

    case ISP::PicConfigureType_E::SCENE:
        nRet = apply_scene_config();
        break;

    default:
        dlog_error("未知的配置类型: %d", static_cast<int>(enConfigType));
        nRet = ERR;
        break;
    }

    return nRet;
}

int CIspManage::apply_scene_all_params(ISP::SceneType_E enSceneType)
{
    dlog_info("开始加载并应用场景 %d 的所有参数", enSceneType);

    /* 更新当前场景类型到配置文件 */
    ISP::AllSceneParams_S stAllParams;
    int nRet = CIspConfigure::instance()->get_configure(stAllParams);
    if (nRet != OK)
    {
        dlog_error("获取所有场景参数失败");
        return nRet;
    }

    /* 设置新的当前场景 */
    stAllParams.enCurrentScene = enSceneType;
    nRet = CIspConfigure::instance()->set_configure(stAllParams);
    if (nRet != OK)
    {
        dlog_error("设置当前场景失败");
        return nRet;
    }

    nRet = apply_image_config();
    if (nRet != OK)
    {
        dlog_error("应用图像基础参数失败");
        return nRet;
    }

    nRet = apply_awb_config();
    if (nRet != OK)
    {
        dlog_error("应用白平衡参数失败");
        return nRet;
    }

    // todo 暂时不生效曝光属性设置
    nRet = apply_exposure_config();
    if (nRet != OK)
    {
        dlog_error("应用曝光参数失败");
        return nRet;
    }

    nRet = apply_daynight_config();
    if (nRet != OK)
    {
        dlog_error("应用日夜切换参数失败");
        return nRet;
    }

    /* 镜像配置与场景无关，不自动加载 */

    dlog_info("场景 %d 的所有参数加载并应用成功", enSceneType);
    return OK;
}

int CIspManage::init_daynight_controller()
{
    CDayNightController::instance()->setStateChangeCallback(
        [this](bool isNight, DayNightMode_E mode) -> void
        {
            this->onDayNightStateChanged(isNight, mode);
        });

    /* 启动日夜切换控制 */
    if (!CDayNightController::instance()->start())
    {
        dlog_error("Failed to start day night controller");
        return ERR;
    }

    dlog_info("日夜切换控制器初始化成功");
    return OK;
}

int CIspManage::apply_daynight_config()
{
    ISP::DayNightAttr_S stDayNightAttr;
    int nRet = CIspConfigure::instance()->get_configure(stDayNightAttr);
    if (nRet != OK)
    {
        dlog_error("获取日夜切换配置失败");
        return nRet;
    }

    nRet = CIspControl::instance()->set_dayNight_attr(stDayNightAttr);
    if (nRet != OK)
    {
        dlog_error("设置日夜切换配置失败");
        return nRet;
    }

    dlog_info("日夜切换配置应用成功，模式: %d", stDayNightAttr.enDayNightMode);
    return OK;
}

int CIspManage::update_daynight(const ISP::DayNightAttr_S& stOldDayNightAttr,
                                const ISP::DayNightAttr_S& stNewDayNightAttr)
{
    /* 更新前的夜晚运行态 */
    bool bWasNight = CDayNightController::instance()->isNightMode();

    /* 日夜配置应用结果 */
    int nRet = apply_daynight_config();
    if (nRet != OK)
    {
        return nRet;
    }

    /* 更新后的夜晚运行态 */
    bool bIsNight = CDayNightController::instance()->isNightMode();

    /* 当前后都处于夜晚且补光配置变化时，主动补做一次硬件同步。 */
    if (bWasNight && bIsNight && has_night_light_runtime_changed(stOldDayNightAttr, stNewDayNightAttr))
    {
        dlog_info("夜晚状态未变化，但补光配置已更新，重新同步IR-CUT和补光灯硬件状态");
        return sync_night_fill_light(stNewDayNightAttr);
    }

    return OK;
}

int CIspManage::apply_image_config()
{
    ISP::ImageParam_S stImageParam;
    int nRet = CIspConfigure::instance()->get_configure(stImageParam);
    if (nRet != OK)
    {
        dlog_error("获取图像参数配置失败");
        return nRet;
    }

    nRet = CIspControl::instance()->set_imageParam_attr(stImageParam);
    if (nRet != OK)
    {
        dlog_error("应用图像参数到硬件失败");
        return nRet;
    }

    dlog_info("图像参数配置应用成功，亮度: %d, 对比度: %d, 饱和度: %d, 锐度: %d",
              stImageParam.nBrightness,
              stImageParam.nContrast,
              stImageParam.nSaturation,
              stImageParam.nSharpness);

    return OK;
}

int CIspManage::apply_exposure_config()
{
    ISP::ExposureAttr_S stExpAttr;
    int nRet = CIspConfigure::instance()->get_configure(stExpAttr);
    if (nRet != OK)
    {
        dlog_error("获取曝光参数配置失败");
        return nRet;
    }

    nRet = CIspControl::instance()->set_exposure_attr(stExpAttr);
    if (nRet != OK)
    {
        dlog_error("应用曝光参数到硬件失败");
        return nRet;
    }

    dlog_info("曝光参数配置应用成功，防横纹: %s, 曝光时间: %d",
              stExpAttr.bAntiBanding ? "开启" : "关闭",
              stExpAttr.enExpTime);

    return OK;
}
int CIspManage::apply_gamma_config()
{
#if CAP_ISP_DAY_NIGHT_DIFFERENT_TUNING
    int nRet = CIspControl::instance()->apply_gamma_attr();
    if (nRet != OK)
    {
        dlog_error("应用Gamma参数到硬件失败");
        return nRet;
    }

    dlog_info("Gamma参数配置应用成功");
#endif
    return OK;
}

int CIspManage::apply_backlight_config()
{
    ISP::BackLightArrt_S stBackLightAttr;
    int nRet = CIspConfigure::instance()->get_configure(stBackLightAttr);
    if (nRet != OK)
    {
        dlog_error("获取背光参数配置失败");
        return nRet;
    }

    nRet = CIspControl::instance()->set_backLight_attr(stBackLightAttr);
    if (nRet != OK)
    {
        dlog_error("应用背光参数到硬件失败");
        return nRet;
    }

    dlog_info("背光参数配置应用成功，背光区域: %d, 宽动态: %s",
              stBackLightAttr.enBackLightArea,
              stBackLightAttr.stWdrAttr.bEnable ? "开启" : "关闭");

    return OK;
}

int CIspManage::apply_awb_config()
{
    ISP::AwbAttr_S stAwbAttr;
    int nRet = CIspConfigure::instance()->get_configure(stAwbAttr);
    if (nRet != OK)
    {
        dlog_error("获取白平衡参数配置失败");
        return nRet;
    }

    nRet = CIspControl::instance()->set_awb_attr(stAwbAttr);
    if (nRet != OK)
    {
        dlog_error("应用白平衡参数到硬件失败");
        return nRet;
    }

    dlog_info("白平衡参数配置应用成功，模式: %d", stAwbAttr.enAwbMode);
    return OK;
}

int CIspManage::apply_dnr_config()
{

    ISP::DnrAttr_S stDnrAttr;
    int nRet = CIspConfigure::instance()->get_configure(stDnrAttr);
    if (nRet != OK)
    {
        dlog_error("获取降噪参数配置失败");
        return nRet;
    }

    nRet = CIspControl::instance()->set_nr_attr(stDnrAttr);
    if (nRet != OK)
    {
        dlog_error("应用降噪参数到硬件失败");
        return nRet;
    }

    dlog_info("降噪参数配置应用成功，模式: %d, 等级: %d", stDnrAttr.enDnrMode, stDnrAttr.nDnrLevel);

    return OK;
}

int CIspManage::apply_mirror_config()
{

    ISP::VideoAdjust_S stVideoAdjust;
    int nRet = CIspConfigure::instance()->get_configure(stVideoAdjust);
    if (nRet != OK)
    {
        dlog_error("获取镜像参数配置失败");
        return nRet;
    }

    nRet = CIspControl::instance()->set_videoMirror_attr(stVideoAdjust);
    if (nRet != OK)
    {
        dlog_error("应用镜像参数到硬件失败");
        return nRet;
    }

    dlog_info("镜像参数配置应用成功，镜像模式: %d", stVideoAdjust.enMirrorMode);
    return OK;
}

int CIspManage::apply_scene_config()
{
#if 0 /* 加载参数即可 */
    ISP::SceneType_E enSceneType;
    int nRet = CIspConfigure::instance()->get_configure(enSceneType);
    if (nRet != OK) 
    {
        dlog_error("获取场景参数配置失败");
        return nRet;
    }
    
    nRet = CSceneParamManager::instance()->scene_set_mode(enSceneType);
    if (nRet != OK) 
    {
        dlog_error("应用场景参数到硬件失败,enSceneType:%u",enSceneType);
        return nRet;
    }
#endif

    apply_image_config();

    // dlog_info("场景参数配置应用成功，当前场景: %d", enSceneType);
    return OK;
}

int CIspManage::apply_schedule_config()
{

    ISP::SceneSchedule_S stSchedule;
    int nRet = CIspConfigure::instance()->get_configure(stSchedule);
    if (nRet != OK)
    {
        dlog_error("获取图像计划配置失败");
        return nRet;
    }

    /* 更新图像计划控制器 */
    CSceneCtrl::instance()->update();

    dlog_info("图像计划配置应用成功，启用状态: %s", stSchedule.bEnable ? "开启" : "关闭");
    return OK;
}

int CIspManage::load_and_apply_all_configs()
{
    int nRet = OK;

    dlog_info("开始从配置文件加载并应用所有图像参数");

    /* 按顺序应用各种配置 */
    ISP::PicConfigureType_E configs[] = {
        // ISP::PicConfigureType_E::SCENE,    // 加载场景
        ISP::PicConfigureType_E::IAMGE, // 图像基础参数
        ISP::PicConfigureType_E::EXPOSURE, // 曝光参数
        ISP::PicConfigureType_E::BACKLIGHT,    // 背光参数
        ISP::PicConfigureType_E::AWB,      // 白平衡参数
        ISP::PicConfigureType_E::NR,       // 降噪参数
        ISP::PicConfigureType_E::MIRROR,   // 镜像参数
        ISP::PicConfigureType_E::DAYNIGHT, // 加载日夜切换
    };

    for (auto configType : configs)
    {
        nRet = update_config(configType);
        if (nRet != OK)
        {
            dlog_error("加载配置类型 %d 失败", static_cast<int>(configType));
            return nRet;
        }
    }

    nRet = apply_schedule_config();
    if (nRet != OK)
    {
        dlog_error("加载图像计划配置失败");
        return nRet;
    }

    dlog_info("所有图像参数加载并应用完成");
    return OK;
}

void CIspManage::update(const ISP::PicConfigureType_E& enPicConfigureInfo)
{
    update_config(enPicConfigureInfo);
}

void CIspManage::onDayNightStateChanged(bool isNight, ISP::DayNightMode_E mode)
{
    dlog_info("日夜状态变化: %s, 模式: %d", isNight ? "夜晚" : "白天", static_cast<int>(mode));

    /* 获取当前日夜切换配置 */
    ISP::DayNightAttr_S stDayNightAttr;
    if (CIspConfigure::instance()->get_configure(stDayNightAttr) != OK)
    {
        dlog_error("Failed to get daynight config");
        return;
    }
#if CAP_ISP_IR_SWITCH // ISP 红外切换
    /* 执行IR切换 */
    if (performIrSwitch(isNight, stDayNightAttr.stFillLight.enLightType) != OK)
    {
        dlog_error("IR-CUT硬件切换失败");
        return;
    }
#endif

    /* 灯光管理器控制补光灯 */
    CLightManager::instance()->on_dayNight_changed(isNight, stDayNightAttr.stFillLight);

    // if(!isNight)
    // {
    //     /* 再加载一下场景参数 */
    //     apply_image_config();
    // }

    dlog_info("日夜状态切换处理完成");
}

int CIspManage::performIrSwitch(bool bNight, ISP::LightType_E enLightType)
{
    dlog_info("执行IR切换: %s, 灯光类型: %d", bNight ? "夜晚" : "白天", static_cast<int>(enLightType));
    int nRet = OK;

    if (bNight)
    {
        if (enLightType == ISP::LIGHT_TYPE_RED || enLightType == ISP::LIGHT_TYPE_SMART)
        {
            /* 红外模式：切换到黑白 */
            dlog_debug("切换到红外黑白模式");
            turn_off_light_before_ircut(ISP::LIGHT_TYPE_WHITE, "白光灯");
            CSceneParamManager::instance()->scene_set_mode(ISP::SCENE_NIGHT);
            usleep(1000);
            nRet = CGpioCtrl::instance()->ir_cut_switch_night();
        }
        else if (enLightType == ISP::LIGHT_TYPE_WHITE || enLightType == ISP::LIGHT_TYPE_WHITE_ON_RED_OFF)
        {
            /* 白光模式：保持彩色 */
            dlog_debug("切换到夜间全彩模式");
            turn_off_light_before_ircut(ISP::LIGHT_TYPE_RED, "红外灯");
#if CAP_ISP_SCENE_LIGHT_PARAM
            /* TV-3852H* 系列使用独立 light 场景参数，保证白光夜视与白天参数解耦。 */
            CSceneParamManager::instance()->scene_set_mode(ISP::SCENE_NIGHT_LIGHT);
#else
            CSceneParamManager::instance()->scene_set_mode(ISP::SCENE_NORMAL);
#endif
            usleep(1000);
            nRet = CGpioCtrl::instance()->ir_cut_switch_day();
        }
        else
        {
            /* 其他情况默认处理 */
            dlog_debug("夜晚模式默认处理");
            turn_off_light_before_ircut(ISP::LIGHT_TYPE_WHITE, "白光灯");
            CSceneParamManager::instance()->scene_set_mode(ISP::SCENE_NIGHT);
            usleep(1000);
            nRet = CGpioCtrl::instance()->ir_cut_switch_night();
        }
    }
    else
    {
        /* 白天模式：恢复正常彩色模式 */
        dlog_debug("切换到白天模式");
        turn_off_light_before_ircut(ISP::LIGHT_TYPE_RED, "红外灯");
        CSceneParamManager::instance()->scene_set_mode(ISP::SCENE_NORMAL);
        usleep(1000);
        nRet = CGpioCtrl::instance()->ir_cut_switch_day();
    }

    if (nRet != OK)
    {
        dlog_error("IR-CUT GPIO切换失败: %d", nRet);
        return nRet;
    }

    // note 设置场景模式后，需要重新应用图像参数、曝光，否则图像将恢复默认
    nRet = apply_image_config();
    if (nRet != OK)
    {
        dlog_warn("IR-CUT已切换，重新应用图像参数失败但继续同步补光灯: %d", nRet);
    }

    nRet = apply_exposure_config();
    if (nRet != OK)
    {
        dlog_warn("IR-CUT已切换，重新应用曝光参数失败但继续同步补光灯: %d", nRet);
    }

    nRet = apply_gamma_config();
    if (nRet != OK)
    {
        dlog_warn("IR-CUT已切换，重新应用Gamma参数失败但继续同步补光灯: %d", nRet);
    }

    return OK;
}

int CIspManage::sync_night_fill_light(const ISP::DayNightAttr_S& stDayNightAttr)
{
#if CAP_ISP_IR_SWITCH // ISP 红外切换
    /* 夜晚下切换补光类型时，除了灯光本身，还必须同步IR-CUT和夜景场景。 */
    int nRet = performIrSwitch(true, stDayNightAttr.stFillLight.enLightType);
    if (nRet != OK)
    {
        return nRet;
    }
#endif

    /* 灯光管理器负责实际灯珠开关和亮度，因此需要把最新补光配置再次下发。 */
    CLightManager::instance()->on_dayNight_changed(true, stDayNightAttr.stFillLight);
    return OK;
}
