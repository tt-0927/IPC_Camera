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

        /**
         * @brief   : 国际证书管理（国密）
         */
        TaskSubClass(GmCreateCertRequestFile)
        TaskSubClass(GmUploadCaCert)
        TaskSubClass(GmUploadDeviceCert)
        TaskSubClass(GmUploadCrlFile)
        TaskSubClass(GmGetCertInfo)
        TaskSubClass(GmDeleteCertFile)
        /** 
        *  @brief ：WiFi功能
        */
        TaskSubClass(SetWifiStaInfo)
        TaskSubClass(ConnectWifiSta)
        TaskSubClass(DisconnectWifiSta)

         /** 
        *  @brief ：4G功能
        */
        TaskSubClass(Get4GInfo)
        TaskSubClass(Set4GInfo)
        /** 
        *  @brief ：热点功能
        */
        TaskSubClass(SetHotspot)
        TaskSubClass(GetHotspotConn)
#ifdef ENABLE_GAT1400_SRC       
        /**
         * @brief   : GAT1400相关
         */
        TaskSubClass(GetGat1400Info)
        TaskSubClass(SetGat1400Info)
#endif

    } // namespace Network
} // namespace Task
