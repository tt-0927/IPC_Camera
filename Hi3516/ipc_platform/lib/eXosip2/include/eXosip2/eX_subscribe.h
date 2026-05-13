/**
 * @file eX_subscribe.h
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
 
 #ifndef __EX_SUBSCRIBE_H__
 #define __EX_SUBSCRIBE_H__
 
 #include <osipparser2/osip_parser.h>  /* osip解析器头文件 */
 #include <time.h>                    /* 时间相关函数 */
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * @file eX_subscribe.h
  * @brief eXosip订阅请求API
  *
  * 该文件提供控制SUBSCRIBE请求所需的API，可用于：
  *
  * <ul>
  * <li>构建SUBSCRIBE请求</li>
  * <li>发送SUBSCRIBE请求</li>
  * <li>构建SUBSCRIBE响应</li>
  * <li>发送SUBSCRIBE响应</li>
  * </ul>
  */
 
 /**
  * @defgroup eXosip2_subscribe eXosip2 SUBSCRIBE及出站订阅
  * @ingroup eXosip2_msg
  * @{
  */
 
 /**
  * 订阅状态枚举
  * @enum eXosip_ss
  */
 enum eXosip_ss {
   EXOSIP_SUBCRSTATE_UNKNOWN,   /**< 未知订阅状态 */
   EXOSIP_SUBCRSTATE_PENDING,   /**< 待定订阅状态 */
   EXOSIP_SUBCRSTATE_ACTIVE,    /**< 活跃订阅状态 */
   EXOSIP_SUBCRSTATE_TERMINATED /**< 终止订阅状态 */
 };
 
 /**
  * 订阅原因枚举
  * @enum eXosip_ss_reason
  */
 enum eXosip_ss_reason {
   DEACTIVATED, /**< 订阅状态：已停用 */
   PROBATION,   /**< 订阅状态：试用期 */
   REJECTED,    /**< 订阅状态：已拒绝 */
   TIMEOUT,     /**< 订阅状态：已超时 */
   GIVEUP,      /**< 订阅状态：已放弃 */
   NORESOURCE   /**< 订阅状态：无资源 */
 };
 
 /**
  * 通知状态枚举
  * @enum eXosip_ss_status
  */
 enum eXosip_ss_status {
   EXOSIP_NOTIFY_UNKNOWN, /**< 未知订阅状态 */
   EXOSIP_NOTIFY_PENDING, /**< 订阅尚未接受 */
   EXOSIP_NOTIFY_ONLINE,  /**< 在线状态 */
   EXOSIP_NOTIFY_BUSY,    /**< 忙碌状态 */
   EXOSIP_NOTIFY_BERIGHTBACK, /**< 稍后回来状态 */
   EXOSIP_NOTIFY_AWAY,       /**< 离开状态 */
   EXOSIP_NOTIFY_ONTHEPHONE, /**< 通话中状态 */
   EXOSIP_NOTIFY_OUTTOLUNCH, /**< 外出午餐状态 */
   EXOSIP_NOTIFY_CLOSED      /**< 关闭状态 */
 };
 
 #ifndef MINISIZE  /* 非最小化编译模式 */
 
 /**
  * 构建默认初始SUBSCRIBE请求
  *
  * @param excontext eXosip实例
  * @param subscribe 指向要构建的SIP请求的指针
  * @param to 被叫方SIP地址
  * @param from 主叫方SIP地址
  * @param route SUBSCRIBE的路由头（可选）
  * @param event SUBSCRIBE的Event头
  * @param expires SUBSCRIBE的Expires头
  * @return 成功返回0，失败返回错误码
  */
 int eXosip_subscription_build_initial_subscribe(struct eXosip_t *excontext, 
                                                osip_message_t **subscribe, 
                                                const char *to, const char *from, 
                                                const char *route, 
                                                const char *event, int expires);
 
 /**
  * 构建默认初始REFER请求
  *
  * @param excontext eXosip实例
  * @param refer 指向要构建的SIP请求的指针
  * @param to 被叫方SIP地址
  * @param from 主叫方SIP地址
  * @param route REFER的路由头（可选）
  * @param refer_to 转接的SIP地址
  * @return 成功返回0，失败返回错误码
  */
 int eXosip_subscription_build_initial_refer(struct eXosip_t *excontext, 
                                            osip_message_t **refer, 
                                            const char *to, const char *from, 
                                            const char *route, 
                                            const char *refer_to);
 
 /**
  * 发送初始SUBSCRIBE/REFER请求
  *
  * @param excontext eXosip实例
  * @param subscribe 要发送的SIP SUBSCRIBE消息
  * @return 成功返回0，失败返回错误码
  */
 int eXosip_subscription_send_initial_request(struct eXosip_t *excontext, 
                                            osip_message_t *subscribe);
 
 /**
  * 构建默认的SUBSCRIBE/REFER刷新消息
  *
  * @param excontext eXosip实例
  * @param did 订阅标识符
  * @param sub 指向要构建的SIP请求的指针
  * @return 成功返回0，失败返回错误码
  */
 int eXosip_subscription_build_refresh_request(struct eXosip_t *excontext, 
                                             int did, 
                                             osip_message_t **sub);
 
 /**
  * 发送新的SUBSCRIBE/REFER刷新请求
  *
  * @param excontext eXosip实例
  * @param did 订阅标识符
  * @param sub 要发送的SIP SUBSCRIBE消息
  * @return 成功返回0，失败返回错误码
  */
 int eXosip_subscription_send_refresh_request(struct eXosip_t *excontext, 
                                            int did, 
                                            osip_message_t *sub);
 
 /**
  * 移除出站订阅上下文
  *
  * @param excontext eXosip实例
  * @param did 订阅标识符
  * @return 成功返回0，失败返回错误码
  */
 int eXosip_subscription_remove(struct eXosip_t *excontext, int did);
 
 /** @} */  /* 结束eXosip2_subscribe组 */
 
 /**
  * @defgroup eXosip2_notify eXosip2 SUBSCRIBE及入站订阅
  * @ingroup eXosip2_msg
  * @{
  */
 
 /**
  * 为SUBSCRIBE请求构建响应
  *
  * @param excontext eXosip实例
  * @param tid SUBSCRIBE事务ID
  * @param status 要构建的SIP响应状态码
  * @param answer 要构建的SIP响应
  * @return 成功返回0，失败返回错误码
  */
 int eXosip_insubscription_build_answer(struct eXosip_t *excontext, 
                                       int tid, int status, 
                                       osip_message_t **answer);
 
 /**
  * 发送SUBSCRIBE请求的响应
  *
  * @param excontext eXosip实例
  * @param tid SUBSCRIBE事务ID
  * @param status 要发送的SIP响应状态码
  * @param answer 要发送的SIP响应（NULL则发送默认响应）
  * @return 成功返回0，失败返回错误码
  */
 int eXosip_insubscription_send_answer(struct eXosip_t *excontext, 
                                     int tid, int status, 
                                     osip_message_t *answer);
 
 /**
  * 在订阅内构建请求
  *
  * @param excontext eXosip实例
  * @param did 入站订阅ID
  * @param method 要构建的请求方法
  * @param request 要构建的SIP请求
  * @return 成功返回0，失败返回错误码
  */
 int eXosip_insubscription_build_request(struct eXosip_t *excontext, 
                                        int did, const char *method, 
                                        osip_message_t **request);
 
 /**
  * 在订阅内构建NOTIFY请求
  *
  * @param excontext eXosip实例
  * @param did 入站订阅ID
  * @param subscription_status 订阅状态（待定、活跃、终止）
  * @param subscription_reason 订阅原因
  * @param request 要构建的SIP请求
  * @return 成功返回0，失败返回错误码
  */
 int eXosip_insubscription_build_notify(struct eXosip_t *excontext, 
                                      int did, 
                                      int subscription_status, 
                                      int subscription_reason, 
                                      osip_message_t **request);
 
 /**
  * 在订阅内发送请求
  *
  * @param excontext eXosip实例
  * @param did 入站订阅ID
  * @param request 要发送的SIP请求
  * @return 成功返回0，失败返回错误码
  */
 int eXosip_insubscription_send_request(struct eXosip_t *excontext, 
                                       int did, 
                                       osip_message_t *request);
 
 /**
  * 移除入站订阅上下文
  *
  * @param excontext eXosip实例
  * @param did 订阅标识符
  * @return 成功返回0，失败返回错误码
  */
 int eXosip_insubscription_remove(struct eXosip_t *excontext, int did);
 
 #endif  /* MINISIZE */
 
 /** @} */  /* 结束eXosip2_notify组 */
 
 #ifdef __cplusplus
 }
 #endif
 #endif  /* __EX_SUBSCRIBE_H__ */