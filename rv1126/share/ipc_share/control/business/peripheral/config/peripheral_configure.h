/**
 * @FilePath     : peripheral_configure.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-20 10:28:16
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-20 10:39:59
 * @Description  : 外设补光配置存储
 */

#pragma once

#include "Singleton.h"
#include "config_storage.h"
#include "system_define.h"

class CPeripheralConfigure : public CSingleton<CPeripheralConfigure>
{
    CPeripheralConfigure();

public:
    ~CPeripheralConfigure();
    friend class CSingleton<CPeripheralConfigure>;

    /**
     * @brief   : 设置外设补光配置
     * @param    {const System::Peripheral_S&} data：外设补光配置
     * @return   {int} OK：成功，非OK：失败
     */
    int set_configure(const System::Peripheral_S &data);

    /**
     * @brief   : 获取外设补光配置
     * @param    {System::Peripheral_S&} data：外设补光配置输出
     * @return   {int} OK：成功，非OK：失败
     */
    int get_configure(System::Peripheral_S &data) const;

private:
    /* 外设补光配置。 */
    ConfigStorage<System::Peripheral_S, StorageType_E::Single> m_stPeripheral;
};
