/**
 * @file eX_register.h
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
 
 #ifndef __EX_REGISTER_H__
 #define __EX_REGISTER_H__
 
 #include <osipparser2/osip_parser.h>  /* osip解析器头文件 */
 #include <time.h>                    /* 时间相关函数 */
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * @file eX_register.h
  * @brief eXosip注册API
  *
  * 该文件提供控制注册所需的API，可用于：
  *
  * <ul>
  * <li>构建初始REGISTER请求</li>
  * <li>构建REGISTER请求</li>
  * <li>发送REGISTER请求</li>
  * </ul>
  */
 
 /**
  * @defgroup eXosip2_registration eXosip2 REGISTER及注册管理
  * @ingroup eXosip2_msg
  * @{
  */
 
 struct eXosip_reg_t;  /* 注册上下文结构体前向声明 */
 
 /**
  * 构建初始REGISTER请求
  *
  * @param excontext eXosip实例
  * @param from      主叫方SIP地址
  * @param proxy     注册使用的代理服务器
  * @param contact   联系地址（可选）
  * @param expires   注册有效期（秒）
  * @param reg       待构建的SIP请求指针
  * @return 成功返回0，失败返回错误码
  */
 int eXosip_register_build_initial_register(struct eXosip_t *excontext, 
                                          const char *from, const char *proxy,
                                          const char *contact, int expires,
                                          osip_message_t **reg);
 
 /**
  * 构建带qvalue参数的初始REGISTER请求
  *
  * @param excontext eXosip实例
  * @param from      主叫方SIP地址
  * @param proxy     注册使用的代理服务器
  * @param contact   联系地址（可选）
  * @param expires   注册有效期（秒）
  * @param qvalue    联系头的q值参数
  * @param reg       待构建的SIP请求指针
  * @return 成功返回0，失败返回错误码
  */
 int eXosip_register_build_initial_register_withqvalue(struct eXosip_t *excontext,
                                                      const char *from, const char *proxy,
                                                      const char *contact, int expires,
                                                      const char *qvalue, osip_message_t **reg);
 
 /**
  * 为现有注册构建新的REGISTER请求
  *
  * @param excontext eXosip实例
  * @param rid       注册上下文唯一标识符
  * @param expires   注册有效期（秒）
  * @param reg       待构建的SIP请求指针
  * @return 成功返回0，失败返回错误码
  */
 int eXosip_register_build_register(struct eXosip_t *excontext, 
                                   int rid, int expires,
                                   osip_message_t **reg);
 
 /**
  * 发送现有注册的REGISTER请求
  *
  * @param excontext eXosip实例
  * @param rid       注册上下文唯一标识符
  * @param reg       待发送的SIP请求（NULL表示使用默认REGISTER）
  * @return 成功返回0，失败返回错误码
  */
 int eXosip_register_send_register(struct eXosip_t *excontext,
                                  int rid, osip_message_t *reg);
 
 /**
  * 移除现有注册（不发送REGISTER请求）
  *
  * @param excontext eXosip实例
  * @param rid       注册上下文唯一标识符
  * @return 成功返回0，失败返回错误码
  */
 int eXosip_register_remove(struct eXosip_t *excontext, int rid);
 
 /** @} */  /* 结束eXosip2_registration组 */
 
 #ifdef __cplusplus
 }
 #endif
 #endif  /* __EX_REGISTER_H__ */