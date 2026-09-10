/*** 
 * @FilePath     : system_task.h
 * @Author       : zhangjc (zhangjc@kfb.cn)
 * @Date         : 2024-10-09 15:19:03
 * @LastEditors  : cyc
 * @LastEditTime : 2025-08-22 17:38:08
 * @Description  : 
 */

#pragma once
#include "task_sub_class.h"

namespace Task
{
    namespace System
    {
        TaskSubClass(GetDeviceInfo)
        TaskSubClass(SetDeviceInfo)

        TaskSubClass(GetDeviceConfig)
        TaskSubClass(SetDeviceConfig)

        TaskSubClass(GetTimeInfo)
        TaskSubClass(SetTimeInfo)
        TaskSubClass(GetNowTime)
        TaskSubClass(TestNtp)
        
        TaskSubClass(GetSecurityServicesInfo)
        TaskSubClass(SetSecurityServicesInfo)
        TaskSubClass(GetSshCountdown);

        TaskSubClass(Reboot)
        TaskSubClass(ResetSimple)
        TaskSubClass(ResetCompletely)

        TaskSubClass(ExportDeviceParam)
        TaskSubClass(ImportDeviceParam)

        TaskSubClass(SetUpgradeMaintain)
        TaskSubClass(GetUpgradeMaintain)
        TaskSubClass(DoUpgrade)
        TaskSubClass(GetUpgradeStatus)
        TaskSubClass(SetUpgrade)
        TaskSubClass(CheckUpgrade)

        TaskSubClass(FindLog)
        TaskSubClass(ExportLog)
        TaskSubClass(TestLogServer);
        TaskSubClass(SetLogServer);
        TaskSubClass(GetLogServer);

        TaskSubClass(GetIpFilterInfo)
        TaskSubClass(SetIpFilterInfo)
        TaskSubClass(AddIpFilterAddress)
        TaskSubClass(RemoveIpFilterAddress)
        TaskSubClass(ModifyIpFilterAddress)

        /* 外设配置 */
        TaskSubClass(GetPeripheralConfig)
        TaskSubClass(SetPeripheralConfig)

        /* 智能资源分配 */
        TaskSubClass(GetSmartEventEnableStatus)
        TaskSubClass(SetSmartEventEnableStatus)

        /* Metadata配置 */
        TaskSubClass(GetMetadataConfig)
        TaskSubClass(SetMetadataConfig)

    } /* namespace system end */
} /* namespace Task end */
 