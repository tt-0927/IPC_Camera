/***
 * @FilePath     : qos_manage.cpp
 * @Author       : huangjunda
 * @Date         : 2025-04-23 11:25:46
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-04-23 11:37:44
 * @Description  :
 */

#include "qos_manage.h"
#include "rtsp_server.h"

CQosManage::CQosManage() : m_configFile(QOS_CONFIG_FILE)
{
}

IpcRet_E CQosManage::init()
{
    Network::QosConfigInfo_S stQosConfigInfo;
    Convert::read_file(QOS_CONFIG_FILE, stQosConfigInfo);
    int nRet = set_qos_config(stQosConfigInfo);
    if (nRet < OK)
    {
        return ERR;
    }

    return OK;
}

IpcRet_E CQosManage::deinit()
{
    return OK;
}

int CQosManage::set_qos_config(Network::QosConfigInfo_S stQosConfigInfo)
{
    int nRet = 0;
    /* 获取之前的 Qos配置 */
    Network::QosConfigInfo_S stOldQosConfigInfo;
    Convert::read_file(m_configFile, stOldQosConfigInfo);

    /* 设置媒体DSCP（在rtsp_server.cpp调用live555接口的socket里实现） */
    /* rtsp Qos */
    if(stOldQosConfigInfo.nMediaDscp != stQosConfigInfo.nMediaDscp)
    {
       nRet =  CRtspServer::instance()->setQosDscp(stQosConfigInfo.nMediaDscp);
       if(nRet !=  OK)
       {
          return nRet;
       }
    }

    /* 设置报警DSCP （在邮箱socket实现）*/
   nRet =  CSmtp::setQosDscp(stQosConfigInfo.nAlarmDscp);
   if(nRet !=  OK)
   {
      return nRet;
   }

    /* 设置管理DSCP （在Web socket实现）*/
   nRet =  LibWSServer::setQosDscp(stQosConfigInfo.nManageDscp);
   if(nRet !=  OK)
   {
      return nRet;
   }

   Convert::write_file(m_configFile, stQosConfigInfo);
   return nRet;
}

int CQosManage::get_qos_config(Network::QosConfigInfo_S &stQosConfigInfo)
{
    Convert::read_file(m_configFile, stQosConfigInfo);
    return 0;
}