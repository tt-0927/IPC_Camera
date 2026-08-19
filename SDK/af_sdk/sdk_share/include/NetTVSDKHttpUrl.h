/**
 * @file NetTVSDKHttpUrl.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief NetTVSDKHttpUrl 模块接口与类型定义
 * 功能说明：
 * 1. 声明 NetTVSDKHttpUrl 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
 */
#ifndef NETSDK_HTTP_URL_H
#define NETSDK_HTTP_URL_H
#include <string>

#define NET_JSON_CONTENT_TYPE 			"application/json"		/* HTTP文本类型 JSON内容类型 */

/**
 * @author tianl (tianl@kfb.cn)
 * @brief HTTP会话管理交互使用
 */
typedef struct tagSessionMessage
{
  std::string SessionId;
}SessionMessage_S;

/************************ HTTP响应码 - 信息响应（1xx） ************************/
#define NET_HTTP_RESP_CODE_CONTINUE				100						/* HTTP响应码 继续 */
#define NET_HTTP_RESP_CODE_SWITCHING_PROTOCOLS	101						/* HTTP响应码 切换协议 */

/************************ HTTP响应码 - 成功响应（2xx） ************************/
#define NET_HTTP_RESP_CODE_SUCCESS				200						/* HTTP响应码 响应成功 */
#define NET_HTTP_RESP_CODE_CREATED				201						/* HTTP响应码 资源创建成功 */
#define NET_HTTP_RESP_CODE_NO_CONTENT			204						/* HTTP响应码 无响应内容 */
#define NET_HTTP_RESP_CODE_PARTIAL_CONTENT		206						/* HTTP响应码 部分内容（断点续传） */

/************************ HTTP响应码 - 重定向（3xx） ************************/
#define NET_HTTP_RESP_CODE_MOVED_PERMANENTLY	301						/* HTTP响应码 永久重定向 */
#define NET_HTTP_RESP_CODE_FOUND				302						/* HTTP响应码 临时重定向 */
#define NET_HTTP_RESP_CODE_NOT_MODIFIED			304						/* HTTP响应码 资源未修改（缓存命中） */
#define NET_HTTP_RESP_CODE_TEMPORARY_REDIRECT	307						/* HTTP响应码 临时重定向（保留方法） */
#define NET_HTTP_RESP_CODE_PERMANENT_REDIRECT	308						/* HTTP响应码 永久重定向（保留方法） */

/************************ HTTP响应码 - 客户端错误（4xx） ************************/
#define NET_HTTP_RESP_CODE_BAD_REQUEST			400						/* HTTP响应码 错误请求（参数非法/格式错误） */
#define NET_HTTP_RESP_CODE_UNAUTHORIZED			401						/* HTTP响应码 未授权（缺少令牌/令牌无效） */
#define NET_HTTP_RESP_CODE_FORBIDDEN			403						/* HTTP响应码 禁止访问（权限不足） */
#define NET_HTTP_RESP_CODE_NOT_FOUND			404						/* HTTP响应码 资源未找到 */
#define NET_HTTP_RESP_CODE_METHOD_NOT_ALLOWED	405						/* HTTP响应码 请求方法不允许 */
#define NET_HTTP_RESP_CODE_REQUEST_TIMEOUT		408						/* HTTP响应码 请求超时 */
#define NET_HTTP_RESP_CODE_CONFLICT				409						/* HTTP响应码 资源冲突（重复创建/修改） */
#define NET_HTTP_RESP_CODE_PAYLOAD_TOO_LARGE	413						/* HTTP响应码 请求体过大 */
#define NET_HTTP_RESP_CODE_URI_TOO_LONG			414						/* HTTP响应码 请求URI过长 */
#define NET_HTTP_RESP_CODE_UNSUPPORTED_MEDIA_TYPE 415					/* HTTP响应码 不支持的媒体类型 */
#define NET_HTTP_RESP_CODE_TOO_MANY_REQUESTS	429						/* HTTP响应码 请求过于频繁（限流触发） */

/************************ HTTP响应码 - 服务器错误（5xx） ************************/
#define NET_HTTP_RESP_CODE_INTERNAL_SERVER_ERROR 500					/* HTTP响应码 内部服务器错误 */
#define NET_HTTP_RESP_CODE_NOT_IMPLEMENTED		501						/* HTTP响应码 接口未实现 */
#define NET_HTTP_RESP_CODE_BAD_GATEWAY			502						/* HTTP响应码 网关错误（上游服务不可用） */
#define NET_HTTP_RESP_CODE_SERVICE_UNAVAILABLE	503						/* HTTP响应码 服务不可用（维护/过载） */
#define NET_HTTP_RESP_CODE_GATEWAY_TIMEOUT		504						/* HTTP响应码 网关超时（上游响应慢） */
#define NET_HTTP_RESP_CODE_HTTP_VERSION_NOT_SUPPORTED 505				/* HTTP响应码 不支持的HTTP版本 */

#define NET_API_ROOT               	"TVAPI"
#define NET_API_VERSION_V1_0       	"V1.0"

/********************************** 	基本接口URL宏定义 	***************************/
#define NET_API_MODULE_BASIC       		"Basic"
#define NET_API_INTERFACE_LOGIN    		"Login"
#define NET_API_INTERFACE_LOGOUT   		"Logout"

#define NET_API_PATH_BASIC_LOGIN  		"/TVAPI/V1.0/Basic/Login"           /* 登录 */
#define NET_API_PATH_BASIC_LOGOUT  		"/TVAPI/V1.0/Basic/Logout"          /* 注销 */
#define NET_API_PATH_BASIC_KEEPLIVE     "/TVAPI/V1.0/Basic/KeepLive"        /* 保活 */

/********************************** 	设备通用接口URL宏定义 	***************************/
#define NET_API_PATH_DEVICE_GETINFO  	"/TVAPI/V1.0/Device/GetInfo"		/* 获取设备信息 */
#define NET_API_PATH_DEVICE_CAPABILITY 	"/TVAPI/V1.0/Device/Capability"		/* 获取设备能力集 */
#define NET_API_PATH_DEVICE_GET_DEV_CONFIG "/TVAPI/V1.0/Device/GetDevConfig"  /* 获取设备配置 */
#define NET_API_PATH_DEVICE_SET_DEV_CONFIG "/TVAPI/V1.0/Device/SetDevConfig"  /* 设置设备配置 */
#define NET_API_PATH_DEVICE_CONTROL        "/TVAPI/V1.0/DeviceControl"        /* 设备硬件控制 */
#define NET_API_PATH_UPGRADE_UPLOAD        "/TVAPI/V1.0/Upgrade/Upload"       /* 上传升级包 */

/********************************** 	URL参数名宏定义 	***************************/
#define NET_API_PARAM_CHANNEL				"channel"		/* 通道号参数 */
#define NET_API_PARAM_COMMAND				"command"		/* 命令参数 */
#define NET_API_PARAM_FILENAME				"filename"      /* 文件名参数 */
#define NET_API_PARAM_NVRCHN				(-1)			/* 标识NVR本机通道号 */

/********************************** 	带参数的URL生成宏定义 	***************************/
/* 设备能力集URL生成宏: NET_API_URL_DEVICE_CAPABILITY(channel, command) */
#define NET_API_URL_DEVICE_CAPABILITY(ch, cmd) \
    (std::string(NET_API_PATH_DEVICE_CAPABILITY) + \
     "?" NET_API_PARAM_CHANNEL "=" + std::to_string(ch) + \
     "&" NET_API_PARAM_COMMAND "=" + std::to_string(cmd))

/* 设备配置URL生成宏 */
#define NET_API_URL_DEVICE_GET_DEV_CONFIG(ch, cmd) \
    (std::string(NET_API_PATH_DEVICE_GET_DEV_CONFIG) + \
     "?" NET_API_PARAM_CHANNEL "=" + std::to_string(ch) + \
     "&" NET_API_PARAM_COMMAND "=" + std::to_string(cmd))

#define NET_API_URL_DEVICE_SET_DEV_CONFIG(ch, cmd) \
    (std::string(NET_API_PATH_DEVICE_SET_DEV_CONFIG) + \
     "?" NET_API_PARAM_CHANNEL "=" + std::to_string(ch) + \
     "&" NET_API_PARAM_COMMAND "=" + std::to_string(cmd))

/* 升级包上传URL生成宏 */
#define NET_API_URL_UPGRADE_UPLOAD(filename) \
    (std::string(NET_API_PATH_UPGRADE_UPLOAD) + \
     "?" NET_API_PARAM_FILENAME "=" + std::string(filename))

/********************************** 	视频通用接口URL宏定义 	***************************/
#define NET_API_PATH_REPLAY_GET_URL       "/TVAPI/V1.0/Replay/GetUrl"      /* 获取回放播放地址 */
#define NET_API_PATH_REPLAY_CONTROL       "/TVAPI/V1.0/Replay/Control"     /* 控制回放开始/停止/倍速 */
#define NET_API_PATH_REPLAY_GET_RECORD_LIST "/TVAPI/V1.0/Replay/GetRecordList" /* 获取回放录像时间段 */
#define NET_API_PATH_RECORD_FRAME_STREAM_START "/TVAPI/V1.0/RecordFrameStream/Start" /* 启动录像帧TCP流 */
#define NET_API_PATH_RECORD_FRAME_STREAM_STOP  "/TVAPI/V1.0/RecordFrameStream/Stop"  /* 停止录像帧TCP流 */

#define NET_API_URL_REPLAY_GET_URL() \
    (std::string(NET_API_PATH_REPLAY_GET_URL))

#define NET_API_URL_REPLAY_CONTROL() \
    (std::string(NET_API_PATH_REPLAY_CONTROL))

#define NET_API_URL_REPLAY_GET_RECORD_LIST() \
    (std::string(NET_API_PATH_REPLAY_GET_RECORD_LIST))

#define NET_API_URL_RECORD_FRAME_STREAM_START() \
    (std::string(NET_API_PATH_RECORD_FRAME_STREAM_START))

#define NET_API_URL_RECORD_FRAME_STREAM_STOP() \
    (std::string(NET_API_PATH_RECORD_FRAME_STREAM_STOP))


/********************************** 	事件通用接口URL宏定义 	***************************/
#define NET_API_PATH_EVENT_SUBSCRIBE  		                  "/TVAPI/V1.0/Event/Subscribe"	          /* 订阅报警事件 */
#define NET_API_PATH_EVENT_UNSUBSCRIBE  		              "/TVAPI/V1.0/Event/UnSubscribe"	      /* 取消订阅报警事件 */
#define NET_API_PATH_ALARMEVENT_LISTEN  		              "/TVAPI/V1.0/Event/AlarmListen"	      /* 监听报警事件 */

#endif
