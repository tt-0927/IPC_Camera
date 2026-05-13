# MaintenanceManager
MaintenanceManager文件夹下是对接运维平台上传日志和配置文件的代码。

# 一、如何使用

以下就是初始化的流程：
1. 当客户端连接上运维进程的服务端后，服务端会主动推送一条命令 “BL_OPERATION_MAINTEMAMCE_GETINFO”
2. 客户端需要处理这条命令组装运维进程需要的初始化参数Json数据
3. 发送 “BL_OPERATION_MAINTEMAMCE_GETINFO” 命令给运维进程，进行初始化


完整例程：
```c

#include "bl_mqtt.h"
#include "cJSON.h"
#include "ctrl_communicate_mqtt.h"
#include "dlog.h"
#include "edukit_network.h"
#include "os.h"
#include "share_device.h"
#include "sdk_network.h"
#include "edukit_port.h"
#include "edukit_conf.h"
#include "share_define.h"
#include "xml_base.h"

/* 与本机运维程序的通讯句柄 */
Sdk_Net_Handle_t g_communicate_mqtt_handle = NULL;
/* 设备唯一码（机器码、MAC） */
char g_achMachineId[64] = { 0 };

/* 组装初始化Json数据 */
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
            int nDeviceIDLen = strlen(g_achMachineId) + 1;
            char pDeviceID[nDeviceIDLen];
            memset(pDeviceID, 0, nDeviceIDLen);
            memcpy(pDeviceID, g_achMachineId, strlen(g_achMachineId));

            cJSON_AddNumberToObject(pData, "code", BL_OPERATION_MAINTEMAMCE_GETINFO);
            cJSON_AddNumberToObject(pData, "opt", OPT_TYPE_SET);
            cJSON_AddStringToObject(pData, "project_code", "5a228c96a65ba383632c1e");
            cJSON_AddStringToObject(pData, "device_code", pDeviceID);
            cJSON_AddStringToObject(pData, "url", "https://oam.itc-pa.cn");
            cJSON_AddStringToObject(pData, "record_path", "/opt/course/log/");

            /* 添加查找路径 */
            pArrPath = cJSON_CreateArray();
            if(pArrPath != NULL)
            {
                cJSON_AddItemToObject(pData, "paths", pArrPath);

                cJSON* pTmpItem = cJSON_CreateObject();
                if(pTmpItem != NULL)
                {
                    cJSON_AddStringToObject(pTmpItem, "path", "/opt/course/log/");
                    cJSON_AddItemToArray(pArrPath, pTmpItem);
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

                cJSON* pTmpItem = cJSON_CreateObject();
                if(pTmpItem != NULL)
                {
                    cJSON_AddNumberToObject(pTmpItem, "type", 0);
                    cJSON_AddStringToObject(pTmpItem, "format", "^director[a-z|A-Z|0-9|\\-|_|.]+log$");
                    cJSON_AddItemToArray(pArrRegex, pTmpItem);
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
 * @brief  操作指令处理
 */
int communicate_DealCmd(NetCallbackMsg_t *param)
{
    if (param == NULL || param->recvvalue == NULL || param->sOperHandle == NULL)
    {
        dlog(LOG_ERROR, "communicate_mqtt_DealCmd is fail\n");
        return -1;
    }

    /* 运维进程服务器发来了获取设备信息请求 */
    if(param->Code == BL_OPERATION_MAINTEMAMCE_GETINFO)
    {
        char *pBuffer = NULL;
        int nLen = 0;
        /* 组装初始化Json数据 */
        create_maintemamce_init_json(&pBuffer, &nLen);
        if(pBuffer != NULL && nLen > 0)
        {
            dlog(LOG_INFO, "%s", pBuffer);

            /* 发出信息 */
            net_send_msgdeal(param->sOperHandle, pBuffer, nLen, BL_OPERATION_MAINTEMAMCE_GETINFO);
            
            free(pBuffer);
            pBuffer = NULL;

            return 0;
        }
        return -1;
    }

    return 0;
}

/**
 * @brief  网络状态上抛
 */
int communicate_netstatus(Net_Status_t status, Sdk_Net_Handle_t handle, void *inparam)
{
    return 0;
}

/**
 * @brief  日志上抛
 */
int communicate_logMsg(const char *format, ...)
{
    return 0;
}

/* 调用这个函数进行初始化客户端 */
void init()
{
    //创建一个客户端
    InparamClientNet_t netparm;
    memset(&netparm, 0, sizeof(InparamClientNet_t));
    netparm.cmdfun = communicate_DealCmd;      /* 操作指令处理 */     
    netparm.statusFun = communicate_netstatus; /* 网络状态上抛 */
    netparm.logFun = communicate_logMsg; /* 日志上抛回调 */
    netparm.overtime = 500;
    netparm.nReconnect = 1;
    netparm.asynchronous = 1;
    strncpy(netparm.ip, "127.0.0.1", sizeof(netparm.ip));

    netparm.nPort = OPERATION_RECORD_PORT;
	netparm.param = NULL;
	g_communicate_mqtt_handle = sdkclient_init_net(netparm);

    /* 获取设备唯一码（机器码、Mac） 以06504k为例，请根据自己项目获取自己的设备唯一码 */
    if(TS_06504K == share_get_currDeviceID() ||
       TS_0650S  == share_get_currDeviceID() 
    {
        if (FALSE == xml_get_charNode2("/root/machin_sn/", g_achMachineId, DEVUID_XML, sizeof(g_achMachineId)))
        {
            /* 读取失败则使用Mac地址 */
            if (0 != ReachMacAddrCapital(ETH0_INTERFACE, g_achMachineId))
            {
                dlog(LOG_ERROR, "获取[%s]的MAC地址失败\n", ETH0_INTERFACE);
                if (0 != ReachMacAddrCapital(ETH1_INTERFACE, g_achMachineId))
                {
                    dlog(LOG_ERROR, "获取[%s]的MAC地址失败\n", ETH1_INTERFACE);
                }
            }
        }
    }

}
```

# 二、接口说明

当前运维进程指令码定义在：bl_event.h 头文件 BlEvent_E 枚举中，其中运用到的就只有一个：

> BL_OPERATION_MAINTEMAMCE_GETINFO = 10200, /* 运维平台主动发送至control，获取设备信息命令 */

***
该指令在客户端连接上之后，运维进程服务端主动推送当前命令一次，客户端需要响应该命令，发送初始化数据给运维进程进行初始化。当然客户端也可以主动推送该命令进行初始化。

运维进程向客户端发送的数据内容格式：
```json
{
	"data":	{
        //命令码
		"code":	10200,
        //操作状态码：0（获取）；1（设置）
		"opt":	0
	}
}
```

客户端向服务端发送的数据内容格式：
```json
{
	"data":	{
        //命令码
		"code":	10200,
        //操作状态码：0（获取）；1（设置）
		"opt":	1,
        //项目唯一代码，可以问运维人员
		"project_code":	"5a228c96a65ba383632c1e",
        //设备唯一代码/ID，可以是机器码也可以是Mac地址
		"device_code":	"EG-b15c5-f3dad-9c1b1-91333",
        //运维平台Url，一般不用动
		"url":	"https://oam.itc-pa.cn",
        //运维平台记录文件存储位置
		"record_path":	"/opt/course/log/",
        //上传文件搜索路径
		"paths":	
        [
            //一条路径
            {
				"path":	"/opt/course/log/"
			}
            //可以往下继续追加
        ],
        //用于匹配上传的文件
		"uploadFileName":	
        [
            {
                //文件类型，0（日志）；1（配置）
				"type":	0,
                //文件名称的匹配正则表达式
				"format":	"^director[a-z|A-Z|0-9|\\-|_|.]+log$"
			}
            //可以往下继续追加
        ]
	}
}

```

# 注意

Log文件名称必须带日期(yyyy_MM_dd 或 yyyy-MM-dd)

推荐使用log的名称：项目名_时间.log

# 正则表达式

推荐编辑正则表达式的网站：[https://regex101.com/](https://regex101.com/)

推荐使用的正则表达式：

```c
//匹配director开头，
//中间允许出现[a-z|A-Z|0-9|\\-|_|.]，可以匹配0到多次
//以log为结尾

"^director[a-z|A-Z|0-9|\\-|_|.]+log$"

//能匹配以下格式
director.log
director_name.log
director_naME-.log
director_2024_06_-24.log
director_2024-06-24-_1.log
director_a2024-06M-24-_1.log
```

注意：需要注意一下正则语法。放到https://regex101.com/网页上时，需要将反斜杠减少一个。即
```c
"^director[a-z|A-Z|0-9|\-|_|.]+log$"
```


# 记录文件（内部）
记录文件是用于记录那些文件是已经上传过了的，程序创建。目前记录文件有两种，这两种文件都会存储在配置文件指定log目录中的mainteanance_record目录下，
如果关闭了程序修改了配置文件中的log目录下次启动读取记录文件的路径也会随之改变。
1. 过滤日期文件：记录已经全部上传文件的日期，比如26号上传了所有25号的日志和配置文件后，
时间到了27号，那么会将25号上传的log文件中所有日期，写出到这个文件，下次检查文件名称时将跳过包含该日期的文件。
2. 上传记录文件：记录已经上传的文件记录，每日创建一个新的记录文件，只将当日上传好的记录写入该文件，一般用于程序运行过程中崩溃或者关闭后重启防止重复上传的作用。

## 过滤日期文件
```json
{
  "data":
      [
         {"date": "2024-06-20"},
         {"date": "2024-06-21"}
      ]
}
```
## 上传记录文件
```json
{
	"data":	
    [
        {
            //文件类型，0:日志，1：配置
            "type":	0,
            //文件唯一码，格式：机器码_文件名称
            "identifier": "机器码_stream_2024_06_14.log",
            //文件路径
            "path":	"./",
            //文件名称
            "name":	"stream_2024_06_14.log"
        }, 
        {
            "type":	1,
            "identifier": "机器码_stream_2024_06_14.json",
            "path":	"./",
            "name":	"stream_2024_06_14.json"
        }
    ]
}
```