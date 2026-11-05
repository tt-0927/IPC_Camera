/*** 
 * @FilePath     : network_task.h
 * @Author       : zhangjc (zhangjc@kfb.cn)
 * @Date         : 2024-10-14 08:38:41
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-07-03 20:08:55
 * @Description  : 
 */

#pragma once
#include "task_sub_class.h"

namespace Task
{
    namespace Network
    {
        TaskSubClass(GetCheckMacValid)
        TaskSubClass(GetNetworkInfo)
        TaskSubClass(SetNetworkInfo)
        TaskSubClass(GetDdnsInfo)
        TaskSubClass(SetDdnsInfo)
        TaskSubClass(GetPppoeInfo)
        TaskSubClass(SetPppoeInfo)
        TaskSubClass(GetPortInfo)
        TaskSubClass(SetPortInfo)
        TaskSubClass(GetPortMapInfo)
        TaskSubClass(SetPortMapInfo)
        TaskSubClass(GetLogServerInfo)
        TaskSubClass(SetLogServerInfo)
        TaskSubClass(GetOhterBaseInfo)
        TaskSubClass(SetOhterBaseInfo)
        TaskSubClass(GetSnmpInfo)
        TaskSubClass(SetSnmpInfo)
        TaskSubClass(GetEamilInfo)
        TaskSubClass(SetEamilInfo)
        TaskSubClass(GetGb28181Info)
        TaskSubClass(SetGb28181Info)
        // TaskSubClass(GetOhterSeniorInfo)
        TaskSubClass(SetOhterSeniorInfo)
        TaskSubClass(GetIntegrationProtoInfo)
        TaskSubClass(SetIntegrationProtoInfo)
        TaskSubClass(TestEamil)
        TaskSubClass(EventEamil)
        TaskSubClass(GetTrustCertInfo)
        TaskSubClass(InstallTrustCert)
        TaskSubClass(DeleteTrustCert)
        TaskSubClass(DownloadTrustCert)
        TaskSubClass(GetDeviceCertInfo)
        TaskSubClass(InstallDeviceCert)
        TaskSubClass(DeleteDeviceCert)
        TaskSubClass(DownloadDeviceCert)
        TaskSubClass(CreateRequestCert)
        TaskSubClass(CreateAndInstallDeviceCert)
        TaskSubClass(DeleteRuquestCsr)
        TaskSubClass(GetHttpsInfo)
        TaskSubClass(ConfigHttpsInfo)
        TaskSubClass(GetAuthMethod)
        TaskSubClass(SetAuthMethod)

        TaskSubClass(GetOnvifConfigInfo)
        TaskSubClass(SetOnvifConfigInfo)
        TaskSubClass(GetQosInfo)
        TaskSubClass(SetQosInfo)
        TaskSubClass(GetBonjourInfo)
        TaskSubClass(SetBonjourInfo)

#if CAP_GB35114
        /**
         * @brief   : 国际证书管理（国密）
         */
        TaskSubClass(GmCreateCertRequestFile)
        TaskSubClass(GmUploadCaCert)
        TaskSubClass(GmUploadPlatformCert)
        TaskSubClass(GmUploadDeviceCert)
        TaskSubClass(GmUploadCrlFile)
        TaskSubClass(GmGetCertInfo)
        TaskSubClass(GmDeleteCertFile)
#endif

        TaskSubClass(SetNetworkService)
        TaskSubClass(GetNetworkService)

#if CAP_NETWORK_WIFI
        /** 
        *  @brief ：WiFi功能
        */
        TaskSubClass(SetWifiStaInfo)
        TaskSubClass(GetWifiStaInfo)
        TaskSubClass(ConnectWifiSta)
        TaskSubClass(DisconnectWifiSta)
        TaskSubClass(SetConfigWifiIpv4)
        TaskSubClass(GetConfigWifiIpv4)
        TaskSubClass(ScanWifiSta)
#endif

        /*
         * 进程间BLE事务任务类型无条件声明，避免不同目标解析能力宏的时机
         * 不一致导致 bind<T> 看不到类型；实际绑定和业务逻辑仍受能力宏控制。
         */
        TaskSubClass(ReleaseBleProvisioning)
        TaskSubClass(CancelBleProvisioning)
        TaskSubClass(CommitBleProvisioning)
        /** 获取二维码/APP连接BLE所需的完整连接字符串。 */
        TaskSubClass(GetBleConnectionInfo)


#if CAP_NETWORK_4G
        /** 
        *  @brief ：4G功能
        */
        TaskSubClass(Get4GInfo)
        TaskSubClass(Set4GInfo)
#endif

#if CAP_NETWORK_WIFI
        /** 
        *  @brief ：热点功能
        */
        TaskSubClass(SetHotspot)
        TaskSubClass(GetHotspot)
        TaskSubClass(GetHotspotConn)
#endif

#if CAP_GARBAGE_STATION_PLATFORM
        /** 
        *  @brief ：平台功能
        */
        TaskSubClass(ConnPlatform)
        TaskSubClass(storePlatformDevices)
        TaskSubClass(GetConnPlatformInfo)
#endif

#ifdef ENABLE_GAT1400_SRC       
        /**
         * @brief   : GAT1400相关
         */
        TaskSubClass(GetGat1400Info)
        TaskSubClass(SetGat1400Info)
#endif

    } // namespace Network
} // namespace Task
