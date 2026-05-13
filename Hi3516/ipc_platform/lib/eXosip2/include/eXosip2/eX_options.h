/**
 * @file eX_options.h
 * @brief eXosip - 扩展的osip库
 * 
 * @copyright 版权所有 (C) 2001-2020 Aymeric MOIZARD amoizard@antisip.com
 * 
 * eXosip是自由软件；您可以根据自由软件基金会发布的GNU通用公共许可证的条款重新分发或修改它，
 * 许可证的版本为第2版，或者（根据您的选择）任何更高版本。
 * 
 * eXosip的分发是希望它有用，但没有任何保证；甚至没有适销性或特定用途适用性的暗示保证。
 * 有关更多详细信息，请参阅GNU通用公共许可证。
 * 
 * 您应该已经收到GNU通用公共许可证的副本；如果没有，请写信给自由软件基金会，
 * 地址：59 Temple Place, Suite 330, Boston, MA 02111-1307 USA。
 * 
 * 此外，作为特殊例外，版权持有人允许在符合特定条件下将此程序的部分代码与OpenSSL库链接，
 * 这些条件在各个源文件中有描述，并分发包含两者的组合。
 * 对于除OpenSSL之外的代码，您必须遵守GNU通用公共许可证的所有条款。
 */

 #ifdef ENABLE_MPATROL
 #include <mpatrol.h>  /* 内存调试工具头文件 */
 #endif
 
 #ifndef MINISIZE  /* 非最小化编译模式 */
 
 #ifndef __EX_OPTIONS_H__
 #define __EX_OPTIONS_H__
 
 #include <osipparser2/osip_parser.h>  /* osip解析器头文件 */
 #include <time.h>                    /* 时间相关函数 */
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * @file eX_options.h
  * @brief eXosip OPTIONS请求API
  *
  * 该文件提供了控制OPTIONS请求所需的API，可用于：
  *
  * <ul>
  * <li>构建OPTIONS请求</li>
  * <li>发送OPTIONS请求</li>
  * <li>构建OPTIONS响应</li>
  * <li>发送OPTIONS响应</li>
  * </ul>
  */
 
 /**
  * @defgroup eXosip2_options eXosip2 OPTIONS及UA能力管理
  * @ingroup eXosip2_msg
  * @{
  */
 
 /**
  * 构建默认OPTIONS消息。
  *
  * @param excontext eXosip实例
  * @param options   指向要构建的SIP OPTIONS请求的指针
  * @param to        被叫方的SIP URL
  * @param from      主叫方的SIP URL
  * @param route     INVITE的路由头（可选）
  * @return 成功返回0，失败返回错误码
  */
 int eXosip_options_build_request(struct eXosip_t *excontext, osip_message_t **options, const char *to, const char *from, const char *route);
 
 /**
  * 发送OPTIONS请求。
  *
  * @param excontext eXosip实例
  * @param options   要发送的SIP OPTIONS消息
  * @return 成功返回0，失败返回错误码
  */
 int eXosip_options_send_request(struct eXosip_t *excontext, osip_message_t *options);
 
 /**
  * 为OPTIONS请求构建响应。
  *
  * @param excontext eXosip实例
  * @param tid       OPTIONS事务ID
  * @param status    要构建的SIP响应状态码
  * @param answer    要构建的SIP响应
  * @return 成功返回0，失败返回错误码
  */
 int eXosip_options_build_answer(struct eXosip_t *excontext, int tid, int status, osip_message_t **answer);
 
 /**
  * 发送OPTIONS请求的响应。
  *
  * @param excontext eXosip实例
  * @param tid       OPTIONS事务ID
  * @param status    要发送的SIP响应状态码
  * @param answer    要发送的SIP响应（如果为NULL则发送默认响应）
  * @return 成功返回0，失败返回错误码
  */
 int eXosip_options_send_answer(struct eXosip_t *excontext, int tid, int status, osip_message_t *answer);
 
 /** @} */  /* 结束eXosip2_options组 */
 
 #ifdef __cplusplus
 }
 #endif
 #endif  /* __EX_OPTIONS_H__ */
 #endif  /* MINISIZE */