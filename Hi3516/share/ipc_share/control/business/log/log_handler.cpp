/*** 
 * @FilePath     : log_handler.cpp
 * @Author       : zhangjc (zhangjc@kfb.cn)
 * @Date         : 2025-02-11 09:59:24
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-08-08 15:46:17
 * @Description  : 日志处理类
 */

#include <iomanip>  
#include <ctime>  
#include "log_handler.h"
#include "action_code.h"
#include "operation_client.h"
#include "bl_event.h"
#include "dlog.h"

using namespace Db;

void LogHandler::write(Log::Info_S stInfo)
{
    if(stInfo.startTime.empty())
    {
        stInfo.startTime = get_dateTime();
        //log_debug("日志获取时间:%s",stInfo.startTime.c_str());
    }
    /* 去除最后的"\0" */
    size_t end = stInfo.host.find_last_not_of('\0');
    stInfo.host.resize(end != std::string::npos ? end + 1 : 0);

    LogDatabase::instance()->add(stInfo);
    MqttMsg_S stMsg;
    std::string  strMsg;
    memset(&stMsg, 0, sizeof(MqttMsg_S));
    switch (stInfo.nType)
    {

        case Log::ALARM:
        case Log::EXCEPTION:
            stMsg.enType = MQTT_TYPE_EXCEPTION;
            stMsg.enLevel = MQTT_LEVEL_COMMONLY;
            stMsg.enSource = MQTT_SOURCE_OTHER_LOG_EXCEPTION;
            break;
        case Log::OPERATION:
            stMsg.enType = MQTT_TYPE_OPERATION;
            stMsg.enLevel = MQTT_LEVEL_SLIGHT;
            stMsg.enSource = MQTT_SOURCE_OTHER_USER;
            break;
        case Log::INFOMATION:
            stMsg.enType = MQTT_TYPE_BUSSINESS;
            stMsg.enLevel = MQTT_LEVEL_COMMONLY;
            if (Log::DEVICE_RUNTIME == stInfo.nAction)
            {
                stMsg.enSource = MQTT_SOURCE_DURATION_OF_USE;
            }
            else
            {
                stMsg.enSource = MQTT_SOURCE_OTHER_USER;
            }
            break;
        default:
            stMsg.enType = MQTT_TYPE_DEFAULT;
            stMsg.enLevel = MQTT_LEVEL_DEFAULT;
            stMsg.enSource = MQTT_SOURCE_DEFAULT;
            break;
            
    }

    std::string  strAction;
    strAction = to_string((Log::Action_E)stInfo.nAction); 
    
    if(!stInfo.chnName.empty() && stInfo.user.empty())
    {
        strMsg = "通道 " + stInfo.chnName + " " + strAction;
    }
    else if(!stInfo.user.empty() && !stInfo.host.empty() && stInfo.chnName.empty())
    {
        strMsg = "远程用户：" + stInfo.user + " 主机地址：" + stInfo.host + " " + strAction;
    }
    else if(!stInfo.user.empty() && !stInfo.host.empty() && !stInfo.chnName.empty())
    {
        strMsg = "远程用户：" + stInfo.user + " 主机地址：" + stInfo.host + "操作通道" + stInfo.chnName + " " + strAction;
    }
    else if(!stInfo.user.empty() && stInfo.host.empty())
    {
        strMsg = "本地用户：" + stInfo.user  + " " + strAction;
    }
    else if(!stInfo.user.empty() && stInfo.host.empty() && !stInfo.chnName.empty())
    {
        strMsg = "本地用户：" + stInfo.user + "操作通道" + stInfo.chnName  + " " + strAction;
    }
    else
    {
    strMsg =  strAction; 
    }

    stMsg.nLen = strMsg.size();
    stMsg.pMsg = (char*)malloc(strMsg.size() + 1);
    memcpy(stMsg.pMsg, strMsg.c_str(), stMsg.nLen);
    stMsg.pMsg[stMsg.nLen] = '\0'; 

        // 发送结构体元数据 + 实际数据
    char* buffer = (char*)malloc(sizeof(MqttMsg_S) + stMsg.nLen);
    memcpy(buffer, &stMsg, sizeof(MqttMsg_S));         // 拷贝结构体
    memcpy(buffer + sizeof(MqttMsg_S), stMsg.pMsg, stMsg.nLen); // 追加数据
    //log_debug("========上传运维日志：[%s] 长度[%d]========",stMsg.pMsg,stMsg.nLen);

    /* 上报运维平台 */
    COperationClient::instance()->send(buffer, sizeof(MqttMsg_S)+ stMsg.nLen, AC_UPLOAD_OPERATION_LOG);
    free(buffer);
    free(stMsg.pMsg);
}

int LogHandler::find(Log::RetrievalCond_S &stRetrievalCond, Common::PageInfo_S &stPageInfo, std::vector<Log::Info_S> &logInfos)
{
    if (stRetrievalCond.startTime.empty() || stRetrievalCond.endTime.empty() || stPageInfo.nCurPage == -1)
    {
        dlog_error("startTime or endTime or nCurPage is empty");
        return -1;
    }
    if (stPageInfo.nCurPage == -1)
    {
        /* 默认第一页 */
        stPageInfo.nCurPage = 1;
        /* 默认每页100条 */
        stPageInfo.nPageSize = 100;
    }
    MatchMethods methods;
    if (stRetrievalCond.enType != Log::Type::ALL)
    {
        methods.push_back(MatchMethod(Element(LOG_FIELD_TYPE, stRetrievalCond.enType), FIND_CRITERION_EQ, FIND_CRITERION_AND));
    }
    if (stRetrievalCond.enAction != Log::Action::UNDEFINED)
    {
        methods.push_back(MatchMethod(Element(LOG_FIELD_ACTION, stRetrievalCond.enAction), FIND_CRITERION_EQ, FIND_CRITERION_AND));
    }
    
    methods.push_back(MatchMethod(Element(LOG_FIELD_START_TIME, stRetrievalCond.startTime), FIND_CRITERION_GE, FIND_CRITERION_AND));
    
    methods.push_back(MatchMethod(Element(LOG_FIELD_START_TIME, stRetrievalCond.endTime), FIND_CRITERION_IE, FIND_CRITERION_AND));

    MatchMethod &lastMethod = methods.back();
    lastMethod.enAndOr = FIND_CRITERION_NONE;

    /* 总个数, 要放在前面 */
    int nCount = -1;
    LogDatabase::instance()->get_count(methods, nCount, DB_COMMON_FIELD_ID);
    stPageInfo.nDataTotal = nCount;
    /* 每页数据个数,默认20 */
    stPageInfo.nPageSize = stPageInfo.nPageSize == 0 ? 20 : stPageInfo.nPageSize;
    stPageInfo.nPageTotal = (stPageInfo.nDataTotal + stPageInfo.nPageSize - 1) / stPageInfo.nPageSize;
    /* 根据id升序 */
    std::string key = "order by " + std::string(LOG_FIELD_START_TIME);
    std::string order = "desc";
    methods.push_back(MatchMethod(Element(key, order), FIND_CRITERION_NONE, FIND_CRITERION_NONE));
    /* 一页数据个数 */
    key = "limit";
    methods.push_back(MatchMethod(Element(key, stPageInfo.nPageSize), FIND_CRITERION_NONE, FIND_CRITERION_NONE));
    /* 第几页 */
    key = "OFFSET" ;
    methods.push_back(MatchMethod(Element(key, std::to_string(stPageInfo.nPageSize * (stPageInfo.nCurPage - 1))), FIND_CRITERION_NONE, FIND_CRITERION_NONE));
    
    return LogDatabase::instance()->find(methods, logInfos);
}


std::string LogHandler::get_dateTime()
{
    /* 获取当前时间点 */
    auto now = std::chrono::system_clock::now();
    /* 转换为 time_t 格式，以便格式化输出 */
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
    std::tm *localTime = std::localtime(&currentTime);
    /* 转为日期 */
    std::stringstream dateStream;
    dateStream << std::put_time(localTime, "%Y-%m-%d");
    std::string date = dateStream.str();
    /* 转为时间 */
    std::stringstream timeStream;
    timeStream << std::put_time(localTime, "%H:%M:%S");
    std::string time = timeStream.str();

    return  date + " " + time;
}