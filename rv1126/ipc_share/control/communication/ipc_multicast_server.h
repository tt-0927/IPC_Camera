/***
 * @FilePath     : ipc_multicast_server.h
 * @Author       : huangjunda
 * @Date         : 2025-06-23 16:54:39
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-09-06 10:56:54
 * @Description  : 多播服务
 */

#ifndef __IPC_MULTICAST_SERVER_H__
#define __IPC_MULTICAST_SERVER_H__

#pragma once

#include "os_network_multicast.h"
#include "data_length.h"
#include "Singleton.h"
#include "IpcRet.h"

#define MULTICAST_ADDR   "239.225.225.105"
#define MULTICAST_PORT   39580

#define HH_MAGIC_FLAG    0x4567
#define COMM_BUFFER_SIZE (12 * 1024 - 18)

#define USER_NAME_LENGTH 16
#define USER_PSW_LENGTH  16

#define XOR_ENCRYPT		(const char*)"*(!*^$%^$%@!@#^*^)*"
#define	PLACE_ENCRYPT	(const char*)"123456"

typedef enum _ERR_CODE
{
    ERR_SUCCESS,
    ERR_FAILURE,
} ERR_CODE;

typedef enum _MULTICAST_SERVICE_TYPE
{
    MULTICAST_CLIENT,
    MULTICAST_SERVER,
} MULTICAST_SERVICE_TYPE;

typedef enum _NET_CMD_TYPE
{
    NET_LOGON_SERVER,
    NET_KEEP_ALIVE,
    NET_UPDATE_FILE,
    NET_TALKBACK,
    NET_TALKBACK_STOP,
    NET_SEARCH_SERVER,
    NET_CONFIG_SERVER,
    NET_MODIFY_MAC,
} NET_CMD_TYPE;

typedef enum MULTI_COMM_E
{
	MULTI_COMM_NETIP,
	MULTI_COMM_MAC,
	MULTI_COMM_DEFAULT,
	MULTI_COMM_ID,
	MULTI_COMM_RESTORE_FACTORY,
	MULTI_COMM_REBOOT,
	MULTI_COMM_PTZ,
} MULTI_COMM;

typedef struct _COMM_HEAD
{
    unsigned long nFlag;
    unsigned short nCommand;
    int nLogonID;
    int nPriority;
    int nMisc;
    unsigned long nErrorCode;
    unsigned long nBufSize;
} COMM_HEAD, *PCOMM_HEAD;

typedef struct _COMM_BUFFER
{
    COMM_HEAD commHead;
    char commBuf[COMM_BUFFER_SIZE];
} COMM_BUFFER;

typedef struct _SEARCH_SER_INFO
{
    /* 设备信息 */
    int            nDeviceType;
    unsigned int   nDeviceId;
    char           szDevName[LENGTH64];
    int            nChannelNum;
    char           szSoftVer[LENGTH32];
    char           szWebPageVer[LENGTH32];
    unsigned int   nRunTime;
    char           szSerial[LENGTH64];
    char           szModel[LENGTH32];
    char           bEncrypt;
    char           szRes3[3];
    char           szRes1[140];
    /* 网络 */
    unsigned int   ipLocal;
    unsigned char  macAddr[LENGTH6];
    unsigned short wPortWeb;
    unsigned short wPortListen;
    unsigned int   ipSubMask;
    unsigned int   ipGateway;
    unsigned int   ipMultiAddr;
    unsigned int   ipDnsAddr;
    unsigned short wMultiPort;
    int	           s4GStrength;
    char           szRes2[604];
} __attribute__((packed)) SEARCH_SER_INFO;

typedef struct _ACCEPT_CONFIG
{
	char          szUserName[USER_NAME_LENGTH + 1];
	char          szUserPassword[USER_PSW_LENGTH + 1];
	int           nCommand;
	unsigned char byMac[LENGTH6];
	char          res[20];
} __attribute__((packed)) ACCEPT_CONFIG;

typedef struct NETIP_CFG_S
{
	unsigned char dhcp;
	unsigned int  nIpAddr;
	unsigned int  nGateway;
	unsigned int  nSubmask;
	unsigned char res[52];
} __attribute__((packed)) NETIP_CFG;

class CIpcMulticastServer : public CSingleton<CIpcMulticastServer>
{
    CIpcMulticastServer() = default;

public:
    ~CIpcMulticastServer() = default;

    friend class CSingleton<CIpcMulticastServer>;

    /***
     * @description : 初始化
     * @author      : huangjunda
     * @return       {IpcRet_E}
     */
    IpcRet_E init();

    /***
     * @description : 去初始化
     * @author      : huangjunda
     * @return       {IpcRet_E}
     */
    IpcRet_E deinit();

    /***
     * @description : 启动服务
     * @author      : huangjunda
     * @param        {char *} pAddress
     * @return       {*}
     */
    void start(const char * pAddress);

    /***
     * @description : 停止服务
     * @author      : huangjunda
     * @return       {*}
     */
    void stop();

private:
    /***
     * @description : 多播处理
     * @author      : huangjunda
     * @param        {void} *pParam
     * @return       {*}
     */
    static void *multicast_handle(void *pParam);

    /***
     * @description : 消息回复
     * @author      : huangjunda
     * @param        {void} *pParam
     * @return       {*}
     */
    void message_response(void *pParam);

    /***
     * @description : 搜索处理
     * @author      : huangjunda
     * @param        {void} *pParam
     * @return       {*}
     */
    void search_handle(void *pParam);

    /***
     * @description : 配置处理
     * @author      : huangjunda
     * @param        {void} *pParam
     * @return       {*}
     */
    void config_handle(void *pParam);

    /***
     * @description : 配置网络处理
     * @author      : huangjunda
     * @param        {void} *pParam
     * @return       {*}
     */
    void config_network_handle(void *pParam);

    /***
    * @description : 配置恢复出厂设置处理
    * @author      : huangjunda
    * @param        {void} *pParam
    * @return       {*}
    */
    void config_reset_handle(void *pParam);

    /***
     * @description : 配置重启处理
     * @author      : huangjunda
     * @param        {void} *pParam
     * @return       {*}
     */
    void config_reboot_handle(void *pParam);

    /***
     * @description : 多播记录日志
     * @author      : huangjunda
     * @param        {void} *pParam
     * @return       {*}
     */
    void multicast_write_log(char *pIp, int nType, int nAction);

    /***
     * @description : 打印搜索参数
     * @author      : huangjunda
     * @param        {void*} pParam
     * @return       {*}
     */
    void print_search_params(void* pParam);

    /***
     * @description : 打印配置参数
     * @author      : huangjunda
     * @param        {void*} pParam
     * @return       {*}
     */
    void print_config_params(void* pParam);

    /***
     * @description : 加密字符串
     * @author      : huangjunda
     * @param        {char} *achText
     * @return       {int}
     */
    int encrypt_string(char *achText);

    NetworkMulticast_S *m_pHandle;    /* 服务句柄 */
    time_t              m_nStartTime; /* 服务启动时间 */
};

#endif /* __IPC_MULTICAST_SERVER_H__ */