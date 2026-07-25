/*** 
 * @FilePath     : isp_light.h
 * @Author       : cyc
 * @Date         : 2025-08-27 19:18:22
 * @LastEditors  : cyc
 * @LastEditTime : 2025-08-28 19:53:07
 * @Description  : 补光控制器
 */

#pragma once
#include "isp_define.h"
#include <cstdint>

using namespace ISP;

class CLightingController : public CSingleton<CLightingController>
{
public:
    CLightingController();
    ~CLightingController() = default;

    friend class CSingleton<CLightingController>;
    
    /**
    * @brief 智能补光控制
    * @param isNight 是否为夜晚模式
    * @param lightConfig 补光配置
    */
    void controlLighting(bool isNight, const FillLight_S& lightConfig);
    
private:
    /**
    * @brief 智能补光模式控制
    */
    void handleSmartLighting();
    
    /**
    * @brief 手动补光模式控制
    */
    void handleManualLighting(const FillLight_S& lightConfig);
    
    /**
    * @brief 根据ISO计算白光亮度
    */
    int calculateWhiteLightLevel(uint32_t isoValue) const;
    
private:
    int m_viPipe{0};
    
    /* 智能补光阈值 */ 
    static constexpr uint32_t LIGHT_OFF_THRESHOLD = 1600;
    static constexpr uint32_t IR_SWITCH_THRESHOLD = 8000;
    static constexpr int MIN_LIGHT_LEVEL = 30;
    static constexpr int MAX_LIGHT_LEVEL = 90;

};