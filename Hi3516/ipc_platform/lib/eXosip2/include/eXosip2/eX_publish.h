/**
 * @file eX_publish.h  
 * @brief eXosip - 扩展的osip库
 *
 * @copyright 版权所有 (C) 2001-2020 Aymeric MOIZARD amoizard@antisip.com
 *
 * eXosip是自由软件；您可以根据自由软件基金会发布的GNU通用公共许可证重新分发或修改，
 * 许可证版本为第2版或（按您的选择）任何更高版本。
 *
 * eXosip的分发希望它有用，但没有任何担保；包括适销性或特定用途适用性的暗示担保。
 * 详情请参阅GNU通用公共许可证。
 *
 * 您应已收到GNU通用公共许可证的副本；如果没有，请致信自由软件基金会，
 * 地址：59 Temple Place, Suite 330, Boston, MA 02111-1307 USA。
 *
 * 此外，作为特殊例外，版权持有人允许在符合特定条件下将此程序的部分代码与OpenSSL库链接，
 * 条件在各源文件中描述，并分发包含两者的组合。
 * 对于除OpenSSL外的代码，您必须完全遵守GNU通用公共许可证。
 */

 #ifdef ENABLE_MPATROL
 #include <mpatrol.h>  /* 内存调试工具头文件 */
 #endif
 
 #ifndef MINISIZE  /* 非最小化编译模式 */
 
 #ifndef __EX_PUBLISH_H__
 #define __EX_PUBLISH_H__
 
 #include <osipparser2/osip_parser.h>  /* osip解析器头文件 */
 #include <time.h>                    /* 时间相关函数 */
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * @file eX_publish.h
  * @brief eXosip发布请求API
  *
  * 该文件提供控制PUBLISH请求所需的API，可用于：
  *
  * <ul>
  * <li>构建PUBLISH请求</li>
  * <li>发送PUBLISH请求</li>
  * </ul>
  */
 
 /**
  * @defgroup eXosip2_publish eXosip2发布管理
  * @ingroup eXosip2_msg
  * @{
  */
 
 /**
  * 为用户构建发布请求(PUBLISH请求)
  *
  * @param excontext eXosip实例
  * @param message   返回的发布请求指针
  * @param to        被叫方SIP地址
  * @param from      主叫方SIP地址  
  * @param route     发布使用的路由
  * @param event     SIP Event头字段
  * @param expires   SIP Expires头字段
  * @param ctype     消息体的Content-Type
  * @param body      发布的消息体
  * @return 成功返回0，失败返回错误码
  */
 int eXosip_build_publish(struct eXosip_t *excontext, osip_message_t **message, 
                         const char *to, const char *from, const char *route,
                         const char *event, const char *expires, 
                         const char *ctype, const char *body);
 
 /**
  * 发送发布消息(PUBLISH请求)
  *
  * @param excontext eXosip实例
  * @param message   待发送的发布消息
  * @param to        发布请求的目标地址(AOR)
  * @return 成功返回0，失败返回错误码 
  */
 int eXosip_publish(struct eXosip_t *excontext, osip_message_t *message, const char *to);
 
 /** @} */  /* 结束eXosip2_publish组 */
 
 #ifdef __cplusplus
 }
 #endif
 #endif  /* __EX_PUBLISH_H__ */
 #endif  /* MINISIZE */