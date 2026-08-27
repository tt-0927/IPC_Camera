/**
 * @file mqtt_communicate.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-05-15
 *
 * @brief 与运维平台mqtt通讯
 */
#include "dlog.h"
#include "mqtt_communicate.h"
#include "edukit_network.h"
#include "ini_disposed.h"
#include "register_convert.h"
#include "convert_interface.h"
#include <cstring>
#include <pthread.h>
#include <unistd.h>
#include <atomic>
#include <mutex>
#include <condition_variable>

/* Mqtt句柄 */
static BlMqtt_S *g_pstMqtt = NULL;
/* 配置信息锁 */
static pthread_mutex_t g_mutexMqttConf;
/* 上报Topic名称 */
static char g_achTopicName[128] = {0};
/* 当前设备的配置信息数据结构 */
static MqttConfig_S g_stMqttConf = {0};
static System::LogServerInfo_S g_logServerInfo;

// 连接状态同步控制
std::mutex g_conn_mutex;                 // 互斥锁
std::condition_variable g_conn_cv;       // 条件变量
std::atomic<int> g_conn_status(-1);      // -1=等待 0=失败 1=成功
std::atomic<bool> g_conn_timeout(false); // 超时标记
bool g_ignore_first = true;

int mqtt_init(System::LogServerInfo_S stLogServerInfo)
{

    g_logServerInfo = stLogServerInfo;
    /* 先断开连接 */
    if (g_pstMqtt != NULL)
    {
        if (g_pstMqtt->bConnected == 1)
        {
            g_pstMqtt->uninit(g_pstMqtt);
        }

        bl_mqtt_release(g_pstMqtt);
        g_pstMqtt = NULL;
    }

    if (!stLogServerInfo.bEnable)
    {

        return 0;
    }

    /* 初始化Mqtt连接 */
    BlMqttNeedParam_S stNeedParam;
    memset(&stNeedParam, 0, sizeof(BlMqttNeedParam_S));
    BlMqttExParam_S stMqttinfo = {
        "",
        "",
        0,
        "ychjasp",
        "821006",
        20,
        30,
        0};
    /* 服务器地址和端口 */
    snprintf(stMqttinfo.achURL, sizeof(stMqttinfo.achURL), "%s", stLogServerInfo.strServerAddr.c_str());
    stMqttinfo.nPort = stLogServerInfo.nPort;

    stNeedParam.pfnCallback = mqtt_callback;

    g_pstMqtt = bl_mqtt_alloc(&stNeedParam, &stMqttinfo);
    if (g_pstMqtt == NULL)
    {
        return -1;
    }

    g_pstMqtt->init(g_pstMqtt);

    /* 拼接上报消息的Topic名称 */
    char achMac[32] = {0};
    if (0 != ReachMacAddrCapital(ETH0_INTERFACE, achMac))
    {
        dlog_error("获取[%s]的MAC地址失败\n", ETH0_INTERFACE);
        if (0 != ReachMacAddrCapital(ETH1_INTERFACE, achMac))
        {
            dlog_error("获取[%s]的MAC地址失败\n", ETH1_INTERFACE);
            dlog_error("无法获取MAC地址，无法上报消息\n");
            return -2;
        }
    }
    snprintf(g_achTopicName, sizeof(g_achTopicName), "device/%s/report/log", achMac);

    return 0;
}

static int mqtt_testcallback(BlMqttMsg_S stMsg)
{
    if (stMsg.enMsgType == BL_MQTT_MSG_CONNECT_SUCCESS)
    {
        {
            std::lock_guard<std::mutex> lock(g_conn_mutex);
            g_conn_status = 1; // 标记连接成功
            dlog_debug("mqtt回调函数 连接成功");
        }
        g_conn_cv.notify_all();
    }
    else if (stMsg.enMsgType == BL_MQTT_MSG_CONNECT_FAILURE)
    {
        {
            std::lock_guard<std::mutex> lock(g_conn_mutex);
            g_conn_status = 0; // 标记连接失败
            dlog_debug("mqtt回调函数 连接失败");
        }
        g_conn_cv.notify_all();
    }

    return 0;
}

int mqtt_test(System::LogServerInfo_S stLogServerInfo)
{
    /* 先断开连接 */
    if (g_pstMqtt != NULL)
    {
        if (g_pstMqtt->bConnected == 1)
        {
            dlog_debug("断开mqtt连接");
            g_pstMqtt->uninit(g_pstMqtt);
        }

        bl_mqtt_release(g_pstMqtt);
        g_pstMqtt = NULL;
    }
    // sleep(3);
    /* 初始化Mqtt连接 */
    int nRet = -1;
    BlMqttNeedParam_S stNeedParam;
    memset(&stNeedParam, 0, sizeof(BlMqttNeedParam_S));
    BlMqttExParam_S stMqttinfo = {
        "",
        "",
        0,
        "ychjasp",
        "821006",
        20,
        30,
        0};
    /* 服务器地址和端口 */
    snprintf(stMqttinfo.achURL, sizeof(stMqttinfo.achURL), "%s", stLogServerInfo.strServerAddr.c_str());
    stMqttinfo.nPort = stLogServerInfo.nPort;
    // dlog_debug("服务器地址：%s->%s",stLogServerInfo.strServerAddr.c_str(),stMqttinfo.achURL);
    stNeedParam.pfnCallback = mqtt_testcallback;

    g_pstMqtt = bl_mqtt_alloc(&stNeedParam, &stMqttinfo);
    if (g_pstMqtt == NULL)
    {
        return -1;
    }
    g_pstMqtt->init(g_pstMqtt);
    // 初始化状态
    g_conn_status = -1;
    g_conn_timeout = false;
    // 同步等待结果
    std::unique_lock<std::mutex> lock(g_conn_mutex);
    if (g_conn_cv.wait_for(lock,
                           std::chrono::seconds(30),
                           []
                           { return g_conn_status != -1; }))
    {
        nRet = -1;
        if (g_conn_status)
        {
            nRet = 0;
        }
    }
    else
    {
        // 超时处理
        g_conn_timeout = true;
        dlog_error("MQTT连接超时");
        nRet = -1;
    }

    /* 恢复mqtt  */
    mqtt_init(g_logServerInfo);
    return nRet;
}

int mqtt_deinit()
{
    return 0;
}

int mqtt_publish(MqttMsg_S stMsg)
{
    if (g_pstMqtt == NULL)
    {
        dlog_error("g_pstMqtt is empty!");
        return -1;
    }

    if (g_pstMqtt->bConnected == 0)
    {
        dlog_error("mqtt未连接");
        return -1;
    }

    char *pJsonBuf = NULL;
    cJSON *pRoot = cJSON_CreateObject();
    cJSON *pData = cJSON_CreateObject();

    cJSON_AddStringToObject(pRoot, "company", MQTT_COMPANY);
    cJSON_AddStringToObject(pRoot, "device_name", MQTT_DEVICE_NAME);

    /* Data字段 */
    pthread_mutex_lock(&g_mutexMqttConf);
    cJSON_AddNumberToObject(pData, "project_id", g_stMqttConf.nPorjectID);
    cJSON_AddStringToObject(pData, "product_name", g_stMqttConf.achProductName);
    cJSON_AddStringToObject(pData, "model", g_stMqttConf.achDeviceName);
    cJSON_AddStringToObject(pData, "serial_number", g_stMqttConf.achSN);
    pthread_mutex_unlock(&g_mutexMqttConf);

    cJSON_AddNumberToObject(pData, "timestamp", time(NULL));
    cJSON_AddNumberToObject(pData, "log_type", stMsg.enType);
    cJSON_AddNumberToObject(pData, "log_level", stMsg.enLevel);
    cJSON_AddNumberToObject(pData, "log_source", stMsg.enSource);

    char achDesc[stMsg.nLen + 1];
    bzero(achDesc, sizeof(achDesc));
    memcpy(achDesc, stMsg.pMsg, stMsg.nLen);
    cJSON_AddStringToObject(pData, "log_desc", achDesc);

    cJSON_AddItemToObject(pRoot, "data", pData);

    pJsonBuf = cJSON_Print(pRoot);
    if (pJsonBuf)
    {
        dlog_info("发布Topic【%s】消息:\n%s\n", g_achTopicName, pJsonBuf);
        g_pstMqtt->publish(g_pstMqtt, g_achTopicName, pJsonBuf, strlen(pJsonBuf), 1);
        free(pJsonBuf);
        pJsonBuf = NULL;
    }
    if (pRoot)
    {
        cJSON_Delete(pRoot);
    }
    return 0;
}

int mqtt_callback(BlMqttMsg_S stMsg)
{
    dlog_debug("mqtt连接回调");
    return 0;
}

int mqtt_load_config()
{
    /* 加锁加载配置文件信息 */
    pthread_mutex_lock(&g_mutexMqttConf);

    g_stMqttConf.nPorjectID = MQTT_PROJECT_ID;
    strncpy(g_stMqttConf.achDeviceName, MQTT_DEVICE_NAME, sizeof(g_stMqttConf.achDeviceName) - 1);
    g_stMqttConf.achDeviceName[sizeof(g_stMqttConf.achDeviceName) - 1] = '\0';

    strncpy(g_stMqttConf.achProductName, MQTT_PRODUCT_NAME, sizeof(g_stMqttConf.achProductName) - 1);
    g_stMqttConf.achProductName[sizeof(g_stMqttConf.achProductName) - 1] = '\0';

    strncpy(g_stMqttConf.achDeviceCode, MQTT_CODE, sizeof(g_stMqttConf.achDeviceCode) - 1);
    g_stMqttConf.achDeviceCode[sizeof(g_stMqttConf.achDeviceCode) - 1] = '\0';

    strncpy(g_stMqttConf.achLogin, MQTT_LOGIN, sizeof(g_stMqttConf.achLogin) - 1);
    g_stMqttConf.achLogin[sizeof(g_stMqttConf.achLogin) - 1] = '\0';

    strncpy(g_stMqttConf.achPasswd, MQTT_PASSWORD, sizeof(g_stMqttConf.achPasswd) - 1);
    g_stMqttConf.achPasswd[sizeof(g_stMqttConf.achPasswd) - 1] = '\0';

    dlog_debug("加载到的ID:[%d]", g_stMqttConf.nPorjectID);
    dlog_debug("加载到的设备名称:[%s]", g_stMqttConf.achDeviceName);
    dlog_debug("加载到的product:[%s]", g_stMqttConf.achProductName);
    dlog_debug("加载到的code:[%s]", g_stMqttConf.achDeviceCode);
    dlog_debug("加载到的账号:[%s]", g_stMqttConf.achLogin);
    dlog_debug("加载到的密码:[%s]", g_stMqttConf.achPasswd);

    /* 机器码获取 */
    Register::RegisterInfo_S stRegInfo;
    Convert::read_file(REGISTER_INFO_FILE, stRegInfo);
    strcpy(g_stMqttConf.achSN, stRegInfo.strMachinSn.c_str());
    pthread_mutex_unlock(&g_mutexMqttConf);
    return 0;
}