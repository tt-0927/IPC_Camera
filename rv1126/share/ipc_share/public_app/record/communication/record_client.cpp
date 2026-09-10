/**
 * @FilePath     : record_client.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-28 10:36:11
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-07-07 17:11:15
 * @Description  : 录制控制客户端
 */

#include "record_client.h"
#include "dlog.h"
#include "common_define.h"
#include "action_code.h"
#include "replay_convert.h"
#include "record_convert.h"
#include "convert_interface.h"

#include "UDSClient.h"

int CRecordClient::init()
{
    Net::Param_S stParam;
    /* 设置心跳码、状态码 */
    stParam.stInitParam.nHearbeatCode = AC_HEARTBIT;
    stParam.stInitParam.nStatusCode = AC_STATUS;
    /* 设置命令码回调函数 */
    using namespace std::placeholders;
    stParam.stInitParam.fnDefaultCallback = std::bind(&CRecordClient::deal_message, this, _1, _2);
    stParam.stInitParam.callbackMap[stParam.stInitParam.nHearbeatCode] = std::bind(&CRecordClient::deal_heartbeat, this, _1, _2);
    ;
    stParam.stInitParam.callbackMap[stParam.stInitParam.nStatusCode] = std::bind(&CRecordClient::deal_status, this, _1, _2);
    ;
    /* 设置ip、端口号 */
    stParam.stInitParam.ip = std::string("127.0.0.1");
    stParam.stInitParam.nPort = IN_CONTROL_RECORD_PROT;
    /* 创建客户端 */
    m_pHandler = std::make_shared<Net::UDSClient>(stParam);
    return 0;
}

void CRecordClient::deinit()
{
    // m_pHandler.reset();
}

void CRecordClient::set_recordManage(CRecordManage *recordManage)
{
    m_pRecordManage = recordManage;
}

int CRecordClient::send(std::string data, int nActionCode, void *pHandle)
{
    if (!m_pHandler)
    {
        return ERR;
    }
    dlog_debug("发送[%d]消息：%s", nActionCode, data.c_str());
    Net::Param_S stParam;
    Net::Message_S stMessage;
    stMessage.pHandle = pHandle;
    stMessage.nActionCode = nActionCode;
    stMessage.pData = data.c_str();
    stMessage.nDataLength = data.length() + 1;
    return m_pHandler->send(stMessage);
}

std::string CRecordClient::get_data(std::string &jsonData)
{
    Json::Object *pJsonRoot = Json::init(jsonData);
    if (!pJsonRoot)
    {
        return std::string();
    }

    Json::Object *pJsonData = Json::get(pJsonRoot, "Data");
    std::string data = Json::to_string(pJsonData);
    Json::deinit(pJsonRoot);
    return data;
}

void CRecordClient::deal_heartbeat(Net::Message_S &stMessage, Net::UserParam_S &stUserParam)
{
    // dlog_debug("接收到心跳消息：%s", stMessage.pData);
}

void CRecordClient::deal_status(Net::Message_S &stMessage, Net::UserParam_S &stUserParam)
{
    // int nStatus = *(const int *)stMessage.pData;
    // dlog_debug("接收到连接状态：%d", nStatus);
}

void CRecordClient::deal_message(Net::Message_S &stMessage, Net::UserParam_S &stUserParam)
{
    dlog_debug("接收到[%d]消息：%s", stMessage.nActionCode, (char *)stMessage.pData);
    std::string strData(static_cast<const char *>(stMessage.pData),
                        stMessage.nDataLength);
    auto data = get_data(strData);
    if (data.empty())
    {
        return;
    }
    if (m_pRecordManage == nullptr)
    {
        return;
    }
    std::string result;
    int nRet = 0;
    /* 处理消息 */
    switch (stMessage.nActionCode)
    {
    case AC_CONTROL_RECORD_INFO: // 录制设置
    {
        Record_NS::Info_S stRecordInfo;
        Convert::to_struct(data, stRecordInfo);
        int nRet = m_pRecordManage->set_record_info(stRecordInfo);
        break;
    }
    // /*rfs的回放初始化设置*/
    // case AC_TO_REPLAY_RFS_TALKBACK:
    // {
    //     int nRet;
    //     Replay::Stream::RfsRtpInfo_S stStreamInfo;
    //     Convert::to_struct(data, stStreamInfo);

    //     Replay::Stream::InitResult_S resultInfo = m_pRecordManage->set_info(stStreamInfo);
    //     result = Convert::to_string(resultInfo);
    //     dlog_info("result %s", result.c_str());
    //     break;
    // }
    // /*播放控制*/
    // case AC_TO_REPLAY_RFS_COTROL:
    // {
    //     Replay::Stream::RfsCtrl_S stStreamCtrl;
    //     Convert::to_struct(data,stStreamCtrl);
    //     m_pRecordManage->setReplayControl(stStreamCtrl);
    //     break;
    // }
    // /*播放进度倍数控制*/
    // case AC_TO_REPLAY_RFS_TIME:
    // {
    //     Replay::Stream::RfsCtrl_S stStreamCtrl;
    //     Convert::to_struct(data,stStreamCtrl);
    //     m_pRecordManage->setReplayTimeControl(stStreamCtrl);
    //     break;
    // }
    default:
        break;
    }
    int nIsResult = 0;
    /* 结果不返回 */
    if (Json::get(data.c_str(), "Return", nIsResult) == true)
    {
        return;
    }
    fill_returnHead(result, stMessage.nActionCode, nRet);
    // dlog_info("result %s", result.c_str());
    send(result, stMessage.nActionCode);
}

void CRecordClient::fill_head(std::string &data, int nActionCode)
{
    Json::Object *pJsonRoot = Json::init();
    Json::add(pJsonRoot, "ActionCode", nActionCode);
    Json::add(pJsonRoot, "DeviceName", "");
    Json::add(pJsonRoot, "UserName", "");

    Json::Object *pJsonData = Json::init(data);
    if (pJsonData)
    {
        Json::add(pJsonRoot, "Data", pJsonData);
    }

    data = Json::to_string(pJsonRoot);
    Json::deinit(pJsonRoot);
}

void CRecordClient::fill_returnHead(std::string &data, int nActionCode, int nRet)
{
    Json::Object *pJsonRoot = Json::init();
    Json::add(pJsonRoot, "ActionCode", nActionCode);
    Json::add(pJsonRoot, "DeviceName", "");
    Json::add(pJsonRoot, "UserName", "");
    Json::add(pJsonRoot, "Return", nRet);

    Json::Object *pJsonData = Json::init(data);
    if (pJsonData)
    {
        Json::add(pJsonRoot, "Data", pJsonData);
    }

    data = Json::to_string(pJsonRoot);
    Json::deinit(pJsonRoot);
}
