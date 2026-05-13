/**
 * @file eXosip.h
 * @brief eXosip - 扩展的osip库
 *
 * @copyright 版权所有 (C) 2001-2020 Aymeric MOIZARD amoizard@antisip.com
 *
 * eXosip是自由软件；您可以根据GNU通用公共许可证自由分发或修改，
 * 许可证版本为第2版或（按您的选择）任何更高版本。
 *
 * eXosip的分发希望它有用，但不提供任何担保；包括适销性或特定用途适用性的暗示担保。
 * 详情请参阅GNU通用公共许可证。
 *
 * 您应已收到GNU通用公共许可证副本；如果没有，请致信自由软件基金会，
 * 地址：59 Temple Place, Suite 330, Boston, MA 02111-1307 USA。
 *
 * 此外，作为特殊例外，版权持有人允许在特定条件下将此程序的部分代码与OpenSSL库链接，
 * 条件在各源文件中描述，并分发包含两者的组合。
 * 对于除OpenSSL外的代码，您必须完全遵守GNU通用公共许可证。
 */

 #ifdef ENABLE_MPATROL
 #include <mpatrol.h>  /* 内存调试工具头文件 */
 #endif
 
 #ifndef __EXOSIP_H__
 #define __EXOSIP_H__
 
 /* 包含eXosip各模块头文件 */
 #include <eXosip2/eX_setup.h>    /* 基础设置API */
 #include <eXosip2/eX_register.h> /* 注册相关API */
 #include <eXosip2/eX_call.h>     /* 呼叫相关API */
 #include <eXosip2/eX_options.h>  /* OPTIONS相关API */
 #include <eXosip2/eX_subscribe.h> /* 订阅相关API */
 #include <eXosip2/eX_message.h>  /* 即时消息相关API */
 #include <eXosip2/eX_publish.h>  /* 发布相关API */
 
 /* 包含oSIP解析器头文件 */
 #include <osipparser2/osip_parser.h>  /* SIP消息解析 */
 #include <osipparser2/sdp_message.h>  /* SDP消息解析 */
 #include <time.h>                    /* 时间相关函数 */
 
 /**
  * @file eXosip.h
  * @brief eXosip主API头文件
  *
  * eXosip是针对RFC3261(SIP协议)的高层库，提供简单易用的API。
  * eXosip2为实现SIP终端提供了极大的灵活性，可用于：
  * <ul>
  * <li>SIP用户代理</li>
  * <li>SIP语音邮件或IVR系统</li>
  * <li>任何作为终端的SIP服务器（如音乐服务器）</li>
  * </ul>
  *
  * 如需实现代理或复杂SIP应用，应考虑直接使用osip库。
  *
  * eXosip支持的功能：
  * <pre>
  *    REGISTER                 处理注册
  *    INVITE/BYE               启动/停止VoIP会话
  *    INFO                     在VoIP会话中发送DTMF
  *    OPTIONS                  模拟VoIP会话
  *    re-INVITE                修改VoIP会话
  *    REFER/NOTIFY             呼叫转移
  *    MESSAGE                  发送即时消息
  *    SUBSCRIBE/REFER/NOTIFY   处理状态信息
  *    其他任何请求             处理自定义需求（对话框外）!
  * </pre>
  */
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * 事件描述结构体
  * @var eXosip_event_t
  */
 typedef struct eXosip_event eXosip_event_t;
 
 /**
  * @defgroup eXosip2_authentication eXosip2认证API
  * @ingroup eXosip2_msg
  * @{
  */
 
 /**
  * 添加认证凭据。当出站请求收到需要认证的响应时使用这些凭据。
  *
  * @param excontext eXosip实例
  * @param username 用户名
  * @param userid 登录ID（通常与用户名相同）
  * @param passwd 密码
  * @param ha1 MD5 ha1哈希值
  * @param realm 认证域（NULL表示应用于未识别域）
  * @return 成功返回0，失败返回错误码
  */
 int eXosip_add_authentication_info(struct eXosip_t *excontext, 
                                  const char *username, const char *userid,
                                  const char *passwd, const char *ha1, 
                                  const char *realm);
 
 /**
  * 移除认证凭据
  *
  * @param excontext eXosip实例
  * @param username 用户名
  * @param realm 认证域（必须与添加时完全一致）
  * @return 成功返回0，失败返回错误码
  */
 int eXosip_remove_authentication_info(struct eXosip_t *excontext, 
                                     const char *username, const char *realm);
 
 /**
  * 清除eXosip中存储的所有认证凭据
  *
  * @param excontext eXosip实例
  * @return 成功返回0，失败返回错误码
  */
 int eXosip_clear_authentication_info(struct eXosip_t *excontext);
 
 /**
  * 执行默认操作：
  *
  *  收到401/407时使用凭据重试
  *  收到3xx响应时使用Contact头重试
  *
  *  当eXosip_automatic_action()无法自动处理时特别有用：
  *  1/ 收到BYE的401或407（事件EXOSIP_CALL_MESSAGE_REQUESTFAILURE）
  *  2/ 收到对话框外任何SIP请求的401或407（EXOSIP_MESSAGE_REQUESTFAILURE）
  *
  * @param excontext eXosip实例
  * @param je 要处理的事件
  * @return 成功返回0，失败返回错误码
  */
 int eXosip_default_action(struct eXosip_t *excontext, eXosip_event_t *je);
 
 /**
  * 执行自动操作：
  *
  *  收到401/407时使用凭据重试
  *  收到422时使用更高Session-Expires重试
  *  在过期前刷新REGISTER和SUBSCRIBE/REFER
  *  收到3xx响应时使用Contact头重试
  *  为会话定时器功能发送自动UPDATE
  *
  * @param excontext eXosip实例
  */
 void eXosip_automatic_action(struct eXosip_t *excontext);
 
 #ifndef MINISIZE
 /**
  * 对话框包的自动内部处理
  *
  * @param excontext eXosip实例
  * @param evt 对话框包的入站SUBSCRIBE
  * @return 成功返回0，失败返回错误码
  */
 int eXosip_insubscription_automatic(struct eXosip_t *excontext, eXosip_event_t *evt);
 #endif
 
 /**
  * 生成随机字符串（仅数字，最大无符号整数）
  *
  * @param buf 目标缓冲区
  * @param buf_size 缓冲区大小
  * @return 成功返回0，失败返回错误码
  */
 int eXosip_generate_random(char *buf, int buf_size);
 
 /**
  * 生成随机字符串（低熵，仅十六进制）
  *
  * @param buf 目标缓冲区
  * @param buf_size 缓冲区大小
  * @param str1 随机输入字符串1
  * @param str2 随机输入字符串2
  * @param str3 随机输入字符串3
  * @return 成功返回0，失败返回错误码
  */
 int eXosip_hexa_generate_random(char *buf, int buf_size, 
                                char *str1, char *str2, char *str3);
 
 /**
  * 生成随机字符串（编译时使用openssl则高熵）
  *
  * @param buf 目标缓冲区
  * @param buf_size 缓冲区大小
  * @return 成功返回0，失败返回错误码
  */
 int eXosip_byte_generate_random(char *buf, int buf_size);
 
 /** @} */  /* 结束eXosip2_authentication组 */
 
 /**
  * @defgroup eXosip2_sdp eXosip2 SDP辅助API
  * @ingroup eXosip2_msg
  * @{
  */
 
 /**
  * 获取呼叫最新INVITE的远程SDP体
  *
  * @param excontext eXosip实例
  * @param did 对话框ID
  * @return SDP消息指针
  */
 sdp_message_t *eXosip_get_remote_sdp(struct eXosip_t *excontext, int did);
 
 /**
  * 获取呼叫最新INVITE的本地SDP体
  *
  * @param excontext eXosip实例
  * @param did 对话框ID
  * @return SDP消息指针
  */
 sdp_message_t *eXosip_get_local_sdp(struct eXosip_t *excontext, int did);
 
 /**
  * 获取呼叫前一次INVITE的本地SDP体
  *
  * @param excontext eXosip实例
  * @param did 对话框ID
  * @return SDP消息指针
  */
 sdp_message_t *eXosip_get_previous_local_sdp(struct eXosip_t *excontext, int did);
 
 /**
  * 从事务ID获取远程SDP体
  *
  * @param excontext eXosip实例
  * @param tid 事务ID
  * @return SDP消息指针
  */
 sdp_message_t *eXosip_get_remote_sdp_from_tid(struct eXosip_t *excontext, int tid);
 
 /**
  * 从事务ID获取本地SDP体
  *
  * @param excontext eXosip实例
  * @param tid 事务ID
  * @return SDP消息指针
  */
 sdp_message_t *eXosip_get_local_sdp_from_tid(struct eXosip_t *excontext, int tid);
 
 /**
  * 从消息获取SDP信息
  *
  * @param message 包含SDP的消息
  * @return SDP消息指针
  */
 sdp_message_t *eXosip_get_sdp_info(osip_message_t *message);
 
 /**
  * 获取呼叫的音频连接信息
  *
  * @param sdp SDP信息
  * @return SDP连接指针
  */
 sdp_connection_t *eXosip_get_audio_connection(sdp_message_t *sdp);
 
 /**
  * 获取呼叫的音频媒体信息
  *
  * @param sdp SDP信息
  * @return SDP媒体指针
  */
 sdp_media_t *eXosip_get_audio_media(sdp_message_t *sdp);
 
 /**
  * 获取呼叫的视频连接信息
  *
  * @param sdp SDP信息
  * @return SDP连接指针
  */
 sdp_connection_t *eXosip_get_video_connection(sdp_message_t *sdp);
 
 /**
  * 获取呼叫的视频媒体信息
  *
  * @param sdp SDP信息
  * @return SDP媒体指针
  */
 sdp_media_t *eXosip_get_video_media(sdp_message_t *sdp);
 
 /**
  * 获取指定媒体的连接信息
  *
  * @param sdp SDP信息
  * @param media 媒体类型
  * @return SDP连接指针
  */
 sdp_connection_t *eXosip_get_connection(sdp_message_t *sdp, const char *media);
 
 /**
  * 获取指定媒体的信息
  *
  * @param sdp SDP信息
  * @param media 媒体类型
  * @return SDP媒体指针
  */
 sdp_media_t *eXosip_get_media(sdp_message_t *sdp, const char *media);
 
 /** @} */  /* 结束eXosip2_sdp组 */
 
 /**
  * @defgroup eXosip2_event eXosip2事件API
  * @ingroup eXosip2_setup
  * @{
  */
 
 /**
  * 事件类型枚举
  * @enum eXosip_event_type
  */
 typedef enum eXosip_event_type {
   /* 注册相关事件 */
   EXOSIP_REGISTRATION_SUCCESS, /**< 用户注册成功 */
   EXOSIP_REGISTRATION_FAILURE, /**< 用户注册失败 */
 
   /* 呼叫中的INVITE相关事件 */
   EXOSIP_CALL_INVITE,   /**< 新呼叫通知 */
   EXOSIP_CALL_REINVITE, /**< 呼叫中的新INVITE通知 */
 
   EXOSIP_CALL_NOANSWER,       /**< 超时内无应答通知 */
   EXOSIP_CALL_PROCEEDING,     /**< 远程应用处理中通知 */
   EXOSIP_CALL_RINGING,        /**< 回铃音通知 */
   EXOSIP_CALL_ANSWERED,       /**< 呼叫开始通知 */
   EXOSIP_CALL_REDIRECTED,     /**< 呼叫重定向通知 */
   EXOSIP_CALL_REQUESTFAILURE, /**< 请求失败通知 */
   EXOSIP_CALL_SERVERFAILURE,  /**< 服务器失败通知 */
   EXOSIP_CALL_GLOBALFAILURE,  /**< 全局失败通知 */
   EXOSIP_CALL_ACK,            /**< 收到对INVITE的200ok的ACK */
 
   EXOSIP_CALL_CANCELLED, /**< 呼叫已取消通知 */
 
   /* 呼叫中除INVITE外的请求相关事件 */
   EXOSIP_CALL_MESSAGE_NEW,            /**< 新入站请求通知 */
   EXOSIP_CALL_MESSAGE_PROCEEDING,     /**< 请求的1xx响应通知 */
   EXOSIP_CALL_MESSAGE_ANSWERED,       /**< 200ok响应通知 */
   EXOSIP_CALL_MESSAGE_REDIRECTED,     /**< 失败通知 */
   EXOSIP_CALL_MESSAGE_REQUESTFAILURE, /**< 请求失败通知 */
   EXOSIP_CALL_MESSAGE_SERVERFAILURE,  /**< 服务器失败通知 */
   EXOSIP_CALL_MESSAGE_GLOBALFAILURE,  /**< 全局失败通知 */
 
   EXOSIP_CALL_CLOSED, /**< 收到该呼叫的BYE通知 */
 
   /* UAS和UAC通用事件 */
   EXOSIP_CALL_RELEASED, /**< 呼叫上下文已清除通知 */
 
   /* 呼叫外请求事件 */
   EXOSIP_MESSAGE_NEW,            /**< 新入站请求通知 */
   EXOSIP_MESSAGE_PROCEEDING,     /**< 请求的1xx响应通知 */
   EXOSIP_MESSAGE_ANSWERED,       /**< 200ok响应通知 */
   EXOSIP_MESSAGE_REDIRECTED,     /**< 失败通知 */
   EXOSIP_MESSAGE_REQUESTFAILURE, /**< 请求失败通知 */
   EXOSIP_MESSAGE_SERVERFAILURE,  /**< 服务器失败通知 */
   EXOSIP_MESSAGE_GLOBALFAILURE,  /**< 全局失败通知 */
 
   /* 状态和即时消息 */
   EXOSIP_SUBSCRIPTION_NOANSWER,       /**< 无应答通知 */
   EXOSIP_SUBSCRIPTION_PROCEEDING,     /**< 1xx响应通知 */
   EXOSIP_SUBSCRIPTION_ANSWERED,       /**< 200ok响应通知 */
   EXOSIP_SUBSCRIPTION_REDIRECTED,     /**< 重定向通知 */
   EXOSIP_SUBSCRIPTION_REQUESTFAILURE, /**< 请求失败通知 */
   EXOSIP_SUBSCRIPTION_SERVERFAILURE,  /**< 服务器失败通知 */
   EXOSIP_SUBSCRIPTION_GLOBALFAILURE,  /**< 全局失败通知 */
   EXOSIP_SUBSCRIPTION_NOTIFY,         /**< 新NOTIFY请求通知 */
 
   EXOSIP_IN_SUBSCRIPTION_NEW, /**< 新入站SUBSCRIBE/REFER通知 */
 
   EXOSIP_NOTIFICATION_NOANSWER,       /**< 无应答通知 */
   EXOSIP_NOTIFICATION_PROCEEDING,     /**< 1xx响应通知 */
   EXOSIP_NOTIFICATION_ANSWERED,       /**< 200ok响应通知 */
   EXOSIP_NOTIFICATION_REDIRECTED,     /**< 重定向通知 */
   EXOSIP_NOTIFICATION_REQUESTFAILURE, /**< 请求失败通知 */
   EXOSIP_NOTIFICATION_SERVERFAILURE,  /**< 服务器失败通知 */
   EXOSIP_NOTIFICATION_GLOBALFAILURE,  /**< 全局失败通知 */
 
   EXOSIP_EVENT_COUNT /**< 事件类型总数 */
 } eXosip_event_type_t;
 
 /**
  * 事件描述结构体
  * @struct eXosip_event
  */
 struct eXosip_event {
   eXosip_event_type_t type; /**< 事件类型 */
   char textinfo[256];       /**< 事件文本描述 */
   void *external_reference; /**< 外部引用（用于呼叫） */
 
   osip_message_t *request;  /**< 当前事务中的请求 */
   osip_message_t *response; /**< 当前事务中的最新响应 */
   osip_message_t *ack;      /**< 当前事务中的ACK */
 
   int tid; /**< 事务唯一ID（用于响应） */
   int did; /**< SIP对话框唯一ID */
 
   int rid; /**< 注册唯一ID */
   int cid; /**< SIP呼叫唯一ID（可能有多个对话框） */
   int sid; /**< 出站订阅唯一ID */
   int nid; /**< 入站订阅唯一ID */
 
   int ss_status; /**< 订阅的当前Subscription-State */
   int ss_reason; /**< 订阅的当前Reason状态 */
 };
 
 /**
  * 释放eXosip事件资源
  *
  * @param je 要处理的事件
  */
 void eXosip_event_free(eXosip_event_t *je);
 
 /**
  * 等待eXosip事件
  *
  * @param excontext eXosip实例
  * @param tv_s 超时值（秒）
  * @param tv_ms 超时值（毫秒）
  * @return 事件指针，超时返回NULL
  */
 eXosip_event_t *eXosip_event_wait(struct eXosip_t *excontext, int tv_s, int tv_ms);
 
 /**
  * 获取下一个eXosip事件（已弃用API）
  * 此API会阻塞 - 应使用更便捷的eXosip_event_wait
  * 限制：此方法不会处理INVITE事务的200ok自动重传
  *
  * @param excontext eXosip实例
  * @return 事件指针
  */
 eXosip_event_t *eXosip_event_get(struct eXosip_t *excontext);
 
 /**
  * 获取内部事件发生的套接字
  * 注意：必须调用eXosip_event_wait直到队列中没有更多事件
  *
  * @param excontext eXosip实例
  * @return 套接字描述符
  */
 int eXosip_event_geteventsocket(struct eXosip_t *excontext);
 
 /** @} */  /* 结束eXosip2_event组 */
 
 #ifdef __cplusplus
 }
 #endif
 #endif  /* __EXOSIP_H__ */