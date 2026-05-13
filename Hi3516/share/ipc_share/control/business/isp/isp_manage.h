/**
 * @FilePath     : isp_manage.h
 * @Author       : cyc
 * @Date         : 2025-08-27 09:51:40
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-29 09:48:13
 * @Description  : 图像管理模块
 */

#pragma once
#include "Singleton.h"
#include "isp_define.h"

class CIspManage : public CSingleton<CIspManage>
{

public:
    CIspManage();
    ~CIspManage();
    friend class CSingleton<CIspManage>;

    /*** 
    * @description : 图像管理初始化
    * @author      : cyc
    * @return       {*}
    */    
    int init();

    /*** 
    * @description : 图像管理去初始化
    * @author      : cyc
    * @return       {*}
    */    
    int deinit();

    /*** 
    * @description : 更新图像配置
    * @author      : cyc
    * @param        {PicConfigureType_E} &enPicConfigureInfo
    * @return       {*}
    */ 
    void update(const ISP::PicConfigureType_E &enPicConfigureInfo);

    /*** 
    * @description : 更新并应用图像配置
    * @author      : cyc
    * @param        {PicConfigureType_E} &enConfigType
    * @return       {int} 0成功，非0失败
    */ 
    int update_config(const ISP::PicConfigureType_E &enConfigType);

    /**
     * @brief   : 更新日夜切换配置，并在需要时补做夜晚补光硬件同步
     * @param    {ISP::DayNightAttr_S} stOldDayNightAttr：更新前的日夜配置
     * @param    {ISP::DayNightAttr_S} stNewDayNightAttr：更新后的日夜配置
     * @return   {int} 0：成功，非0：失败
     * @note    : 当设备当前已经处于夜晚，且更新后仍保持夜晚时，会额外检查补光配置是否变化；若变化，则主动重同步IR-CUT、场景和补光灯状态
     */
    int update_daynight(const ISP::DayNightAttr_S &stOldDayNightAttr,
                        const ISP::DayNightAttr_S &stNewDayNightAttr);

    /*** 
    * @description : 加载并应用指定场景的所有参数
    * @author      : cyc
    * @param        {SceneType_E} enSceneType 目标场景类型
    * @return       {int} 0成功，非0失败
    */ 
    int apply_scene_all_params(ISP::SceneType_E enSceneType);
    int apply_awb_config();

private:

    /*** 
    * @description : 初始化日夜切换控制器
    * @author      : cyc
    * @return       {int} 0成功，非0失败
    */
    int init_daynight_controller();
    
    /*** 
    * @description : 加载并应用所有配置
    * @author      : cyc
    * @return       {int} 0成功，非0失败
    */
    int load_and_apply_all_configs();

    /* 各种配置的应用方法 */ 
    int apply_daynight_config();
    int apply_image_config();
    int apply_exposure_config();
    int apply_backlight_config();
    int apply_dnr_config();
    int apply_mirror_config();
    // note 网页无gamma设置，当前仅用于 TV-3852H* 随场景补光模式切换
    int apply_gamma_config();
    int apply_scene_config();
    int apply_schedule_config();
    /* 日夜状态变化处理 */ 
    void onDayNightStateChanged(bool isNight, ISP::DayNightMode_E mode);

    /**
     * @brief   : 根据日夜状态和补光类型执行 IR-CUT 与场景切换
     * @param    {bool} toNight：true：切到夜晚，false：切到白天
     * @param    {ISP::LightType_E} enLightType：当前补光类型
     * @return   {int} 0：成功，非0：失败
     */
    int performIrSwitch(bool toNight, ISP::LightType_E enLightType);

    /**
     * @brief   : 在当前仍为夜晚时，重新同步补光相关硬件状态
     * @param    {ISP::DayNightAttr_S} stDayNightAttr：最新的日夜配置
     * @return   {int} 0：成功，非0：失败
     * @note    : 该函数会同时同步IR-CUT、场景模式以及补光灯开关和亮度，避免仅更新配置而硬件状态未切换
     */
    int sync_night_fill_light(const ISP::DayNightAttr_S &stDayNightAttr);

};
