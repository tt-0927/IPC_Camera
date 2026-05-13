/*** 
 * @FilePath     : mqtt_ctrl_communication.h
 * @Author       : EasonLu
 * @Date         : 2024-03-18 15:00:49
 * @LastEditors  : huangjunda
 * @LastEditTime : 2024-12-07 11:44:21
 * @Description  : 运维平台的通讯模块
 */
#ifndef __MQTT_CTRL_COMMUNICATION_H__
#define __MQTT_CTRL_COMMUNICATION_H__

#ifdef __cplusplus
extern "C"{
#endif

#include "bl_event.h"

#define MAINTENANCE_URL                    "http://oam.itc-pa.cn:81"
#define MAINTENANCE_RECORD_PTAH            "/opt/course/log/"
#define HARDWARE_CHECK_RECORD_PTAH         "/opt/cam/.config/user_data/hardware_check/"

#ifdef TV_6124HU
    #define MAINTENANCE_PROJECT_CODE "ge1ejxfwnKMqHxbiPJyFkvljKwxZ44xP"
#elif defined(TV_C204U)
    #define MAINTENANCE_PROJECT_CODE "qRKuuDRiJNiXYvSo2kLoYXs2te0sgbnT"
#elif defined(TV_C304U)
    #define MAINTENANCE_PROJECT_CODE "rETMx8DjNtly6YIMeB5zaZq0e6EYTJKK"
#else
    #define MAINTENANCE_PROJECT_CODE "ge1ejxfwnKMqHxbiPJyFkvljKwxZ44xP"
#endif

/**
 * @brief  读取激活/注册信息配置文件
 * @param  [char *] 文件路径
 * @return [*]
 * @author Xiezhh
 * @note 向前声明
 */
int communtication_read_registerJson(char *pRegister);

/**
 * @brief  创建初始化运维平台的Json数据
 * @param  [char **][out] Json数据
 * @param  [int *][out] Json数据长度
 * @return [*]
 * @author Xiezhh
 * @note 向前声明
 */
void create_maintemamce_init_json(char **pBuffer, int *nLen);

/**
 * @brief  发布消息到运维平台
 * @param [MqttLogType_E] enType 消息类型
 * @param [MqttLogLevel_E] enLevel 消息级别
 * @param [MqttLogSource_E] enLevel 消息维度
 * @param [char *] pMsg 指向消息内容的指针
 * @param [int] nLen 消息内容的长度
 * @return [int] 返回值，成功返回 0，失败返回 -1
 * @author Huangjd
 * @note 发布消息到运维平台，并封装为特定格式 
 */
int publish_msg( MqttLogType_E enType, MqttLogLevel_E enLevel, MqttLogSource_E enSource, char *pMsg, int nLen);

/**
 * @brief  发布消息到运维平台
 * @param [MqttMsg_S] stMsg 待发布的消息结构体
 * @return [int] 返回值，成功返回 0，失败返回 -1
 * @author EasonLu
 * @note 将消息封装为运维平台的消息格式并发布
 */
int mqtt_publish(MqttMsg_S stMsg);

/**
 * @brief  向本机运维程序发送消息
 * @param  [char] *pMsg - 消息内容
 * @param  [int] nLen - 消息长度
 * @param  [int] nCode - 消息码
 * @return [*]
 * @author EasonLu
 * @note   
 */
int send_mqtt_msg(char *pMsg, int nLen, int nCode);

/**
 * @brief  请求升级包信息
 * @param  [char] *pMsg - 消息内容
 * @param  [int] nLen - 消息长度
 * @param  [int] nCode - 消息码
 * @return [*]
 * @author huangjd
 * @note   
 */
int req_mqtt_upgradePack(int nType);

/**
 * @brief  解析升级包信息
 * @param  [char] *pMsg - 消息内容
 * @param  [int] nLen - 消息长度
 * @param  [int] nCode - 消息码
 * @return [*]
 * @author huangjunda
 * @note   
 */
int parse_mqtt_upgradePack(const char *pMessage, char *pFileName, char *pVersion, int *nID, int nLen);

/**
 * @brief  请求下载升级包
 * @param  [char] *pMsg - 消息内容
 * @param  [int] nLen - 消息长度
 * @param  [int] nCode - 消息码
 * @return [*]
 * @author huangjunda
 * @note   
 */
int req_mqtt_downloadPack(int nID);

/**
 * @brief  解析下载升级包
 * @param  [char] *pMsg - 消息内容
 * @param  [int] nLen - 消息长度
 * @param  [int] nCode - 消息码
 * @return [*]
 * @author huangjunda
 * @note   
 */
int parse_mqtt_downloadPack(const char *pMessage, char *pUrl, int nLen);

/**
 * @brief  请求获取升级包版本号信息
 * @param  [int] nType - 升级包类型
 * @return [*]
 * @author lianghy
 * @note   
 */
int req_mqtt_upgradePack_version(int nType);

/**
 * @brief  初始化与运维平台的mqtt通讯模块
 * @return [*]
 * @author EasonLu
 * @note
 */
int communication_mqtt_init();

#ifdef __cplusplus
}
#endif

#endif // __MQTT_CTRL_COMMUNICATION_H__