/*
 * @Author       : EasonLu
 * @Date         : 2024-03-18 15:00:41
 * @LastEditors: lianghaoyao 709692194@qq.com
 * @LastEditTime: 2025-02-07 10:58:50
 * @FilePath: mqtt_ctrl_communication.c
 * @Description  : 运维平台的通讯模块
 */
// #include <stdio.h>
// #include <pthread.h>
#include "bl_mqtt.h"
#include "bl_event.h"
#include "cJSON.h"
#include "mqtt_ctrl_communication.h"
#include "dlog.h"
#include "edukit_network.h"
#include "os.h"
#include "share_device.h"
#include "sdk_net_base.h"
#include "share_port.h"
#include "edukit_conf.h"
#include "share_define.h"
#include "https_download.h"
#include "edukit_value.h"
#include <sys/stat.h>

/* 与本机运维程序的通讯句柄 */
Sdk_Net_Handle_t g_communicate_mqtt_handle = NULL;

/* 初始化正在下载 */
int g_nUpgradePackDownload = XML_DOWNLOAD_STATUS;

char achFileName[128] = {0};

/**
 * @brief  读取激活/注册信息配置文件
 * @param  [char *] 文件路径
 * @return [*]
 * @author Xiezhh
 * @note 向前声明
 */
int communtication_read_registerJson(char *pRegister)
{
    if(pRegister == NULL)
	{
		dlog(LOG_ERROR, "read_registerJson is empty!\n");
		return -1;
	}
    
    struct stat stFileStat  = { 0 };
    FILE*       pFp         = NULL;
    size_t      nSize       = 0;
    char*       pchJsonData = NULL;
    size_t      nReadSize   = 0;

    /* 获取文件信息 */
    if (stat(pRegister, &stFileStat) != 0)
    {
        dlog(LOG_ERROR, "文件[%s]信息异常[%s]", pRegister, strerror(errno));
        goto EXIT;
    }

    /* 判断路径是否为文件夹 */
    if (S_ISDIR(stFileStat.st_mode))
    {
        dlog(LOG_ERROR, "传入的文件路径为文件夹[%s]", pRegister);
        goto EXIT;
    }

    /* 打开文件 */
    pFp = fopen(pRegister, "r");
    if (pFp == NULL)
    {
        dlog(LOG_ERROR, "打开文件失败[%s]", pRegister);
        goto EXIT;
    }

    /* 创建空间 */
    nSize       = stFileStat.st_size;
    pchJsonData = (char *)malloc(nSize + 1);
    if (pchJsonData == NULL)
    {
        dlog(LOG_ERROR, "创建空间失败");
        goto EXIT;
    }

    /* 读取文件 */
    nReadSize = fread(pchJsonData, sizeof(char), nSize, pFp);
    if (nReadSize != nSize)
    {
        dlog(LOG_ERROR, "读取数据长度异常");
        goto EXIT;
    }

    /* 解析文件 */
    cJSON*    pNodeData  = NULL;
    cJSON*    pChildNode = NULL;

    /*创建操作句柄*/
    pNodeData = cJSON_Parse(pchJsonData);
    if (NULL == pNodeData)
    {
        dlog(LOG_ERROR, "传入的Json字符串有问题, 无法创建句柄");
        goto EXIT;
    }

    /* 机器码 */
    pChildNode = cJSON_GetObjectItem(pNodeData, "MachinSn");
    if (NULL == pChildNode ||
        NULL == pChildNode->valuestring)
    {
        dlog(LOG_ERROR, "获取节点[MachinSn]信息失败");
        goto EXIT;
    }
    /* memcpy(g_achMachineId, pChildNode->valuestring, strlen(pChildNode->valuestring) + 1); */

EXIT:
    if (pchJsonData)
    {
        free(pchJsonData);
        pchJsonData = NULL;
    }

    if (pFp)
    {
        fclose(pFp);
        pFp = NULL;
    }

    return 1;
}


/**
 * @brief  创建初始化运维平台的Json数据
 * @param  [char **][out] Json数据
 * @param  [int *][out] Json数据长度
 * @return [*]
 * @author Xiezhh
 * @note 向前声明
 */
void create_maintemamce_init_json(char **pBuffer, int *nLen)
{
    cJSON* pRoot = NULL;
    cJSON* pData = NULL;
    cJSON* pArrPath = NULL;
    cJSON* pArrRegex = NULL;
    *pBuffer = NULL;
    *nLen = 0;

    pRoot = cJSON_CreateObject();
    if(pRoot != NULL)
    {
        pData = cJSON_CreateObject();
        if(pData != NULL)
        {
            /*
            int nDeviceIDLen = strlen(g_achMachineId) + 1;
            char pDeviceID[nDeviceIDLen];
            memset(pDeviceID, 0, nDeviceIDLen);
            memcpy(pDeviceID, g_achMachineId, strlen(g_achMachineId));
            */

            char achMac[32] = {0};
            memset(achMac, 0, sizeof(achMac));
            if (0 != ReachMacAddrCapital(ETH0_INTERFACE, achMac))
            {
                dlog(LOG_ERROR, "获取[%s]的MAC地址失败\n", ETH0_INTERFACE);
                if (0 != ReachMacAddrCapital(ETH1_INTERFACE, achMac))
                {
                    dlog(LOG_ERROR, "获取[%s]的MAC地址失败\n", ETH1_INTERFACE);
                    dlog(LOG_ERROR, "无法获取MAC地址，无法上报消息\n");
                    return;
                }
            }

            cJSON_AddNumberToObject(pData, "code", BL_OPERATION_MAINTEMAMCE_GETINFO);
            cJSON_AddNumberToObject(pData, "opt", OPT_TYPE_SET);
            /* 项目唯一code */
            cJSON_AddStringToObject(pData, "project_code", MAINTENANCE_PROJECT_CODE);
            cJSON_AddStringToObject(pData, "device_code", achMac);
            cJSON_AddStringToObject(pData, "url", MAINTENANCE_URL);
            cJSON_AddStringToObject(pData, "record_path", MAINTENANCE_RECORD_PTAH);

            /* 添加查找路径 */
            pArrPath = cJSON_CreateArray();
            if(pArrPath != NULL)
            {
                cJSON_AddItemToObject(pData, "paths", pArrPath);

                cJSON* pTmpItem = cJSON_CreateObject();
                if(pTmpItem != NULL)
                {
                    cJSON_AddStringToObject(pTmpItem, "path", MAINTENANCE_RECORD_PTAH);
                    cJSON_AddItemToArray(pArrPath, pTmpItem);
                }

                cJSON* pTmpItem2 = cJSON_CreateObject();
                if(pTmpItem2 != NULL)
                {
                    cJSON_AddStringToObject(pTmpItem2, "path", HARDWARE_CHECK_RECORD_PTAH);
                    cJSON_AddItemToArray(pArrPath, pTmpItem2);
                }
            }
            else
            {
                dlog(LOG_ERROR, "create paths arr json fail!");
                return;
            }

            /* 添加匹配正则 */
            pArrRegex = cJSON_CreateArray();
            if(pArrRegex != NULL)
            {
                cJSON_AddItemToObject(pData, "uploadFileName", pArrRegex);

                cJSON* pStreamItem = cJSON_CreateObject();
                if(pStreamItem != NULL)
                {
                    cJSON_AddNumberToObject(pStreamItem, "type", 0);
                    cJSON_AddStringToObject(pStreamItem, "format", "^stream[a-z|A-Z|0-9|\\-|_|.]+log$");
                    cJSON_AddItemToArray(pArrRegex, pStreamItem);
                }

                cJSON* pDaemonItem = cJSON_CreateObject();
                if(pDaemonItem != NULL)
                {
                    cJSON_AddNumberToObject(pDaemonItem, "type", 0);
                    cJSON_AddStringToObject(pDaemonItem, "format", "^daemon[a-z|A-Z|0-9|\\-|_|.]+log$");
                    cJSON_AddItemToArray(pArrRegex, pDaemonItem);
                }
                
                cJSON* pRecordItem = cJSON_CreateObject();
                if(pRecordItem != NULL)
                {
                    cJSON_AddNumberToObject(pRecordItem, "type", 0);
                    cJSON_AddStringToObject(pRecordItem, "format", "^record[a-z|A-Z|0-9|\\-|_|.]+log$");
                    cJSON_AddItemToArray(pArrRegex, pRecordItem);
                }

                cJSON* pOperation_recordItem = cJSON_CreateObject();
                if(pOperation_recordItem != NULL)
                {
                    cJSON_AddNumberToObject(pOperation_recordItem, "type", 0);
                    cJSON_AddStringToObject(pOperation_recordItem, "format", "^operation_record[a-z|A-Z|0-9|\\-|_|.]+log$");
                    cJSON_AddItemToArray(pArrRegex, pOperation_recordItem);
                }

                cJSON* pOperation_record_errorItem = cJSON_CreateObject();
                if(pOperation_record_errorItem != NULL)
                {
                    cJSON_AddNumberToObject(pOperation_record_errorItem, "type", 0);
                    cJSON_AddStringToObject(pOperation_record_errorItem, "format", "^operation_record_error[a-z|A-Z|0-9|\\-|_|.]+log$");
                    cJSON_AddItemToArray(pArrRegex, pOperation_record_errorItem);
                }

                cJSON* pFilemanagementItem = cJSON_CreateObject();
                if(pFilemanagementItem != NULL)
                {
                    cJSON_AddNumberToObject(pFilemanagementItem, "type", 0);
                    cJSON_AddStringToObject(pFilemanagementItem, "format", "^filemanagement[a-z|A-Z|0-9|\\-|_|.]+log$");
                    cJSON_AddItemToArray(pArrRegex, pFilemanagementItem);
                }

                cJSON* pTestLogItem = cJSON_CreateObject();
                if(pTestLogItem != NULL)
                {
                    cJSON_AddNumberToObject(pTestLogItem, "type", 2);
                    cJSON_AddStringToObject(pTestLogItem, "format", "^hardware_check[a-z|A-Z|0-9|\\-|_|.]+xml$");
                    cJSON_AddItemToArray(pArrRegex, pTestLogItem);
                }
            }
            else
            {
                dlog(LOG_ERROR, "create Regex arr json fail!");
                return;
            }

            cJSON_AddItemToObject(pRoot, "data", pData);

            *pBuffer = cJSON_Print(pRoot);
            *nLen = strlen(*pBuffer);
        }
        else
        {
            dlog(LOG_ERROR, "create init json data object fail!");
        }

        cJSON_Delete(pRoot);
    }
    else
    {
        dlog(LOG_ERROR, "create init json root object fail!");
    }
}

/**
 * @brief  发布消息到运维平台
 * @param [MqttLogType_E] enType 消息类型
 * @param [MqttLogLevel_E] enLevel 消息级别
 * @param [char *] pMsg 指向消息内容的指针
 * @param [int] nLen 消息内容的长度
 * @return [int] 返回值，成功返回 0，失败返回 -1
 * @author Huangjd
 * @note 发布消息到运维平台，并封装为特定格式 
 */
int publish_msg( MqttLogType_E enType, MqttLogLevel_E enLevel, MqttLogSource_E enSource, char *pMsg, int nLen)
{
    if (pMsg == NULL || nLen <= 0)
    {
        return -1;
    }

    MqttMsg_S stMsg;
    memset(&stMsg, 0, sizeof(MqttMsg_S));
    stMsg.enType  = enType;
    stMsg.enLevel = enLevel;
    stMsg.enSource = enSource;
    stMsg.pMsg    = pMsg;
    stMsg.nLen = nLen;
    return mqtt_publish(stMsg);
}

/**
 * @brief  发布消息到运维平台
 * @param [MqttMsg_S] stMsg 待发布的消息结构体
 * @return [int] 返回值，成功返回 0，失败返回 -1
 * @author EasonLu
 * @note 将消息封装为运维平台的消息格式并发布
 */
int mqtt_publish(MqttMsg_S stMsg)
{
    if (stMsg.pMsg == NULL || stMsg.nLen <= 0)
    {
        return -1;
    }
    /* 发送到运维程序 */
    int nCode = 0;
    bl_event_encode_msgCode(stMsg.enType, stMsg.enLevel, stMsg.enSource, &nCode);
    return send_mqtt_msg(stMsg.pMsg, stMsg.nLen, nCode);
}

/**
 * @brief  向本机运维程序发送消息
 * @param  [char] *pMsg - 消息内容
 * @param  [int] nLen - 消息长度
 * @param  [int] nCode - 消息码
 * @return [*]
 * @author EasonLu
 * @note   
 */
int send_mqtt_msg(char *pMsg, int nLen, int nCode)
{
    if (g_communicate_mqtt_handle)
    {
        return net_send_msg(g_communicate_mqtt_handle, pMsg, nLen, nCode);
    }
    return -1;
}

/**
 * @brief  请求升级包信息
 * @param  [char] *pMsg - 消息内容
 * @param  [int] nLen - 消息长度
 * @param  [int] nCode - 消息码
 * @return [*]
 * @author huangjd
 * @note   
 */
int req_mqtt_upgradePack(int nType)
{
    cJSON *pRootJson = NULL;
    cJSON *pDataJson = NULL;
    char *pMsg = NULL;
    int nLen = 0;
    int nRet = 0;
    
    g_nUpgradePackDownload = XML_DOWNLOAD_STATUS;

    /* 请求运维平台获取最新升级包 */
    pRootJson = cJSON_CreateObject();
    pDataJson = cJSON_CreateObject();
    cJSON_AddNumberToObject(pRootJson, "ActionCode", BL_OPERATION_UPGRADEPACK_GETINFO);
    cJSON_AddNumberToObject(pDataJson, "type", nType);
    cJSON_AddItemToObject(pRootJson, "Data", pDataJson);

    pMsg = cJSON_Print(pRootJson);
    nLen = strlen(pMsg);

    nRet = send_mqtt_msg(pMsg, nLen, BL_OPERATION_UPGRADEPACK_GETINFO);
    if (0 != nRet)
    {
        dlog(LOG_ERROR, "send_mqtt_msg err");
        free(pMsg);
        cJSON_Delete(pRootJson);
        return nRet;
    }

    free(pMsg);
    cJSON_Delete(pRootJson);
    return nRet;
}

/**
 * @brief  解析升级包信息
 * @param  [char] *pMsg - 消息内容
 * @param  [int] nLen - 消息长度
 * @param  [int] nCode - 消息码
 * @return [*]
 * @author huangjunda
 * @note   
 */
int parse_mqtt_upgradePack(const char *pMessage, char *pFileName, char *pVersion, int *nID, int nLen)
{
    int nRet = -1;
    cJSON* pNodeData  = NULL;
    cJSON* pChildNode = NULL;
    cJSON* pItem = NULL;
    if (NULL == pMessage)
    {
        dlog(LOG_ERROR, "传入参数异常");
        return nRet;
    }
    /*创建操作句柄*/
    pNodeData = cJSON_Parse(pMessage);
    if (NULL == pNodeData)
    {
        dlog(LOG_ERROR, "传入的Json字符串有问题, 无法创建句柄");
        goto EXIT;
    }
    /* Data */
    pChildNode = cJSON_GetObjectItem(pNodeData, "Data");
    if (NULL == pChildNode)
    {
        dlog(LOG_ERROR, "获取节点[Data]信息失败");
        goto EXIT;
    }
    pItem = cJSON_GetObjectItem(pChildNode, "file_name");
    if (NULL == pChildNode)
    {
        dlog(LOG_ERROR, "获取节点[type]信息失败");
        goto EXIT;
    }
    strncpy(pFileName, pItem->valuestring, nLen);
    pItem = cJSON_GetObjectItem(pChildNode, "version");
    if (NULL == pChildNode)
    {
        dlog(LOG_ERROR, "获取节点[type]信息失败");
        goto EXIT;
    }
    strncpy(pVersion, pItem->valuestring, nLen);
    pItem = cJSON_GetObjectItem(pChildNode, "id");
    if (NULL == pChildNode)
    {
        dlog(LOG_ERROR, "获取节点[type]信息失败");
        goto EXIT;
    }
    *nID = pItem->valueint;
    nRet = 0;
EXIT:
    if (pNodeData)
    {
        cJSON_Delete(pNodeData);
        pNodeData = NULL;
    }
    return nRet;
}

/**
 * @brief  请求下载升级包
 * @param  [char] *pMsg - 消息内容
 * @param  [int] nLen - 消息长度
 * @param  [int] nCode - 消息码
 * @return [*]
 * @author huangjunda
 * @note   
 */
int req_mqtt_downloadPack(int nID)
{
    cJSON *pRootJson = NULL;
    cJSON *pDataJson = NULL;
    char *pMsg = NULL;
    int nLen = 0;
    int nRet = 0;
    /* 请求运维平台获取最新升级包 */
    pRootJson = cJSON_CreateObject();
    pDataJson = cJSON_CreateObject();
    cJSON_AddNumberToObject(pRootJson, "ActionCode", BL_OPERATION_UPGRADEPACK_DOWNLOAD);
    cJSON_AddNumberToObject(pDataJson, "id", nID);
    cJSON_AddItemToObject(pRootJson, "Data", pDataJson);

    pMsg = cJSON_Print(pRootJson);
    nLen = strlen(pMsg);

    nRet = send_mqtt_msg(pMsg, nLen, BL_OPERATION_UPGRADEPACK_DOWNLOAD);
    if (0 != nRet)
    {
        dlog(LOG_ERROR, "send_mqtt_msg err");
        free(pMsg);
        cJSON_Delete(pRootJson);
        return nRet;
    }

    free(pMsg);
    cJSON_Delete(pRootJson);
    return nRet;
}

/**
 * @brief  请求获取升级包版本号信息
 * @param  [int] nType - 升级包类型
 * @return [*]
 * @author lianghy
 * @note   
 */
int req_mqtt_upgradePack_version(int nType)
{
    cJSON *pRootJson = NULL;
    cJSON *pDataJson = NULL;
    char *pMsg = NULL;
    int nLen = 0;
    int nRet = 0;
    
    /* 请求运维平台获取最新升级包 */
    pRootJson = cJSON_CreateObject();
    pDataJson = cJSON_CreateObject();
    cJSON_AddNumberToObject(pRootJson, "ActionCode", BL_OPERATION_LATESTUPGRADEPACK_GETINFO);
    cJSON_AddNumberToObject(pDataJson, "type", nType);
    cJSON_AddItemToObject(pRootJson, "Data", pDataJson);

    pMsg = cJSON_Print(pRootJson);
    nLen = strlen(pMsg);
 
    nRet = send_mqtt_msg(pMsg, nLen, BL_OPERATION_LATESTUPGRADEPACK_GETINFO);
    if (0 != nRet)
    {
        dlog(LOG_ERROR, "send_mqtt_msg err");
        free(pMsg);
        cJSON_Delete(pRootJson);
        return nRet;
    }

    free(pMsg);
    cJSON_Delete(pRootJson);
    return nRet;
}

/**
 * @brief  解析下载升级包
 * @param  [char] *pMsg - 消息内容
 * @param  [int] nLen - 消息长度
 * @param  [int] nCode - 消息码
 * @return [*]
 * @author huangjunda
 * @note   
 */
int parse_mqtt_downloadPack(const char *pMessage, char *pUrl, int nLen)
{
    int nRet = -1;
    cJSON* pNodeData  = NULL;
    cJSON* pChildNode = NULL;
    cJSON* pItem = NULL;
    if (NULL == pMessage)
    {
        dlog(LOG_ERROR, "传入参数异常");
        return nRet;
    }
    /*创建操作句柄*/
    pNodeData = cJSON_Parse(pMessage);
    if (NULL == pNodeData)
    {
        dlog(LOG_ERROR, "传入的Json字符串有问题, 无法创建句柄");
        goto EXIT;
    }
    /* Data */
    pChildNode = cJSON_GetObjectItem(pNodeData, "Data");
    if (NULL == pChildNode)
    {
        dlog(LOG_ERROR, "获取节点[Data]信息失败");
        goto EXIT;
    }
    pItem = cJSON_GetObjectItem(pChildNode, "download_url");
    if (NULL == pChildNode)
    {
        dlog(LOG_ERROR, "获取节点[type]信息失败");
        goto EXIT;
    }
    strncpy(pUrl, pItem->valuestring, nLen);
    nRet = 0;
EXIT:
    if (pNodeData)
    {
        cJSON_Delete(pNodeData);
        pNodeData = NULL;
    }
    return nRet;
}

/**
 * @brief  操作指令处理
 * @param  [NetCallbackMsg_t] *param - 回调数据
 * @return [*]
 * @author EasonLu
 * @note
 */
int communicate_mqtt_DealCmd(NetCallbackMsg_t *param)
{
    if (param == NULL || param->recvvalue == NULL || param->sOperHandle == NULL)
    {
        dlog(LOG_ERROR, "communicate_mqtt_DealCmd is fail\n");
        return -1;
    }

    switch (param->Code)
    {
        dlog(LOG_DEBUG, "收到本机运维通信程序发来的消息码：%d", param->Code);
        case BL_OPERATION_MAINTEMAMCE_GETINFO:
        {
            char *pMsg = NULL;
            int nLen = 0;
            create_maintemamce_init_json(&pMsg, &nLen);
            if(pMsg != NULL && nLen > 0)
            {
                // dlog(LOG_DEBUG, "%s", pMsg);
                send_mqtt_msg(pMsg, nLen, BL_OPERATION_MAINTEMAMCE_GETINFO);
                free(pMsg);
                pMsg = NULL;
            }
            break;
        }
        case BL_OPERATION_LOAD_INI:
        {
            send_mqtt_msg(MQTT_CONF_INI, strlen(MQTT_CONF_INI), BL_OPERATION_LOAD_INI);
            break;
        }
        case BL_OPERATION_UPGRADEPACK_GETINFO:
        {
            int nRet = 0;
            int nID = 0;
            char achVersion[128] = {0};
            nRet = parse_mqtt_upgradePack(param->recvvalue, achFileName, achVersion, &nID, sizeof(achFileName));
            if (0 != nRet)
            {
                dlog(LOG_ERROR, "解析升级包失败");
                g_nUpgradePackDownload = XML_DOWNLOAD_FAILED;
                break;
            }
            req_mqtt_downloadPack(nID);
            break;
        }
        case BL_OPERATION_UPGRADEPACK_DOWNLOAD:
        {
            int nRet = 0;
            char achUrl[1024] = {0};
            nRet = parse_mqtt_downloadPack(param->recvvalue, achUrl, sizeof(achUrl));
            if (0 != nRet)
            {
                dlog(LOG_ERROR, "解析下载包失败");
                g_nUpgradePackDownload = XML_DOWNLOAD_FAILED;
                break;
            }

            if ('\0' == achFileName[0])
            {
                dlog(LOG_DEBUG, "升级包文件名为空，没有获取到名字");
                g_nUpgradePackDownload = XML_DOWNLOAD_FAILED;
                break;
            }

            nRet = download_https_upgrade_package(achUrl, achFileName);
            if (0 != nRet)
            {
                dlog(LOG_ERROR, "平台升级包下载失败");
                g_nUpgradePackDownload = XML_DOWNLOAD_FAILED;
                break;
            }
            dlog(LOG_DEBUG, "平台升级包下载成功");

            // nRet = check_package();
            if (0 != nRet)
            {
                dlog(LOG_ERROR, "平台升级包检查失败");
                g_nUpgradePackDownload = XML_DOWNLOAD_FAILED;
                break;
            }
            dlog(LOG_DEBUG, "平台升级包检查成功");
            g_nUpgradePackDownload = XML_DOWNLOAD_SUCCESS;
            break;
        }
        case BL_OPERATION_LATESTUPGRADEPACK_GETINFO:
        {
            int nRet = 0;
            int nID = 0;
            float fNewVersion;
            float fOldVersion;
            char newVersion[128] = {0};
            char oldVersion[128] = {0};

            nRet = parse_mqtt_upgradePack(param->recvvalue, achFileName, newVersion, &nID, sizeof(achFileName));
            if (0 != nRet)
            {
                dlog(LOG_ERROR, "解析升级包失败");
                g_nUpgradePackDownload = XML_DOWNLOAD_FAILED;
                break;
            }

            if(FALSE == xml_get_charNode2("/root/Device_info/Software_version/", oldVersion, DEVICE_CONFIG, sizeof(oldVersion)))
            {
                xml_set_charNode2("/root/Device_info/Software_version/", PREFIX_VERSION, DEVICE_CONFIG);
                memset(oldVersion, 0, sizeof(oldVersion));
                memcpy(oldVersion, PREFIX_VERSION, sizeof(PREFIX_VERSION));
            }

            fOldVersion = (float)atof(oldVersion+1);
            fNewVersion = (float)atof(newVersion+1);
            if(fNewVersion > fOldVersion)
            {
                xml_set_charNode2("/root/Device_info/Software_version/", newVersion, DEVICE_CONFIG);
            } 

            break;
        }
        
        default:
            break;
    }

    return 0;
}

/**
 * @brief  网络状态上抛
 * @param  [Net_Status_t] status - 网络状态
 * @param  [Sdk_Net_Handle_t] handle - 网络连接句柄
 * @param  [void] *inparam - 自定义参数
 * @return [*]
 * @author EasonLu
 * @note
 */
int communicate_mqtt_netstatus(
    Net_Status_t status,
    Sdk_Net_Handle_t handle,
    void *inparam)
{
    return 0;
}

/**
 * @brief  日志上抛
 * @param  [char] *format - 格式
 * @return [*]
 * @author EasonLu
 * @note
 */
int communicate_mqtt_logMsg(const char *format, ...)
{
    return 0;
}

/**
 * @brief  初始化与运维平台的mqtt通讯模块
 * @return [*]
 * @author EasonLu
 * @note
 */
int communication_mqtt_init()
{
	InparamClientNet_t netparm;
	memset(&netparm, 0, sizeof(InparamClientNet_t));
#ifdef MQTT_STREAM
	netparm.cmdfun = communicate_mqtt_DealCmd;
#else
    netparm.cmdfun = NULL;
#endif
	netparm.statusFun = communicate_mqtt_netstatus;
	netparm.logFun = communicate_mqtt_logMsg;
	netparm.overtime = 500;
	netparm.nReconnect = 1;
	netparm.asynchronous = 1;
	strncpy(netparm.ip, "127.0.0.1", sizeof(netparm.ip));

	netparm.nPort = OPERATION_RECORD_PORT;
	netparm.param = NULL;
	g_communicate_mqtt_handle = sdkclient_init_net(netparm);
   
    return 0;
}