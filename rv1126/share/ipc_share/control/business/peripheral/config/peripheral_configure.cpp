/**
 * @FilePath     : peripheral_configure.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-20 10:28:16
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-20 10:39:59
 * @Description  : 外设补光配置存储实现
 */

#include "peripheral_configure.h"

#include "path_define.h"

CPeripheralConfigure::CPeripheralConfigure() : m_stPeripheral(PERIPHERAL_CONFIG_FILE)
{
}

CPeripheralConfigure::~CPeripheralConfigure()
{
}

int CPeripheralConfigure::set_configure(const System::Peripheral_S &data)
{
    return m_stPeripheral.set(data);
}

int CPeripheralConfigure::get_configure(System::Peripheral_S &data) const
{
    return m_stPeripheral.get(data);
}
