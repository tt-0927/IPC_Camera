/*
  eXosip - 这是扩展的osip库。
  版权所有 (C) 2001-2020 Aymeric MOIZARD amoizard@antisip.com

  eXosip是自由软件；您可以自由地重新分发和/或修改
  它，依据GNU通用公共许可证的条款发布，
  可以是许可证的第2版，或者（根据您的选择）任何更新的版本。

  eXosip的分发希望它能有使用价值，
  但没有任何担保；甚至没有适销性或特定用途适用性的隐含担保。
  更多细节请参阅GNU通用公共许可证。

  您应该已经收到GNU通用公共许可证的副本
  如果没有，请写信给自由软件基金会：
  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  此外，作为一个特殊例外，版权持有人给予
  允许在特定条件下将此程序的部分代码与
  OpenSSL库链接，如每个单独的源文件所述，
  并分发包含两者的组合。
  您必须遵守GNU通用公共许可证的所有方面，
  对于除OpenSSL之外的代码。如果您修改
  带有此异常的文件，您可以将此异常扩展到您的
  文件版本，但这不是必须的。如果您不希望这样做，
  请从您的版本中删除此异常声明。如果您删除
  程序中所有源文件的此异常声明，那么也请在此删除它。
*/

#ifdef ENABLE_MPATROL
#include <mpatrol.h>
#endif

#ifndef __EX_CALL_H__
#define __EX_CALL_H__

#include <osipparser2/osip_parser.h>
#include <osipparser2/sdp_message.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file eX_call.h
 * @brief eXosip 呼叫API
 *
 * 本文件提供了控制呼叫所需的API。您可以使用它来：
 *
 * <ul>
 * <li>构建初始INVITE</li>
 * <li>发送初始INVITE</li>
 * <li>构建呼叫中的请求</li>
 * <li>发送呼叫中的请求</li>
 * </ul>
 *
 * 此API可用于构建以下消息：
 * <pre>
 *    INVITE, INFO, OPTIONS, REFER, UPDATE, NOTIFY
 * </pre>
 */

/**
 * @defgroup eXosip2_call eXosip2 INVITE和呼叫管理
 * @ingroup eXosip2_msg
 * @{
 */

struct eXosip_call_t;

/**
 * 为现有呼叫设置新的应用上下文
 *
 * @param excontext  eXosip_t实例
 * @param id         呼叫ID或对话ID
 * @param reference  新的应用上下文
 */
int eXosip_call_set_reference(struct eXosip_t *excontext, int id, void *reference);

/**
 * 获取现有呼叫的应用上下文指针
 *
 * @param excontext  eXosip_t实例
 * @param cid        呼叫ID
 * @return           应用上下文引用
 */
void *eXosip_call_get_reference(struct eXosip_t *excontext, int cid);

/**
 * 为新呼叫构建默认INVITE消息
 *
 * @param excontext eXosip_t实例
 * @param invite    用于保存SIP元素的指针
 * @param to        被叫方的SIP URL
 * @param from      主叫方的SIP URL
 * @param route     INVITE的路由头（可选）
 * @param subject   呼叫主题
 */
int eXosip_call_build_initial_invite(struct eXosip_t *excontext, osip_message_t **invite, const char *to, const char *from, const char *route, const char *subject);

/**
 * 发起呼叫
 *
 * @param excontext   eXosip_t实例
 * @param invite      要发送的SIP INVITE消息
 */
int eXosip_call_send_initial_invite(struct eXosip_t *excontext, osip_message_t *invite);

/**
 * 构建呼叫中的默认请求（INVITE, OPTIONS, INFO, REFER）
 *
 * @param excontext    eXosip_t实例
 * @param did          对话ID
 * @param method       要构建的请求类型
 * @param request      要构建的SIP请求
 */
int eXosip_call_build_request(struct eXosip_t *excontext, int did, const char *method, osip_message_t **request);

/**
 * 为收到的200ok构建默认ACK
 *
 * @param excontext    eXosip_t实例
 * @param tid          INVITE/2xx的事务ID
 * @param ack          要构建的SIP请求
 */
int eXosip_call_build_ack(struct eXosip_t *excontext, int tid, osip_message_t **ack);

/**
 * 发送收到的200ok的ACK
 *
 * @param excontext    eXosip_t实例
 * @param tid          INVITE/2xx的事务ID
 * @param ack          要发送的SIP ACK消息
 */
int eXosip_call_send_ack(struct eXosip_t *excontext, int tid, osip_message_t *ack);

/**
 * 为呼叫转移构建默认REFER
 *
 * @param excontext    eXosip_t实例
 * @param did          对话ID
 * @param refer_to     呼叫转移的URL（Refer-To头）
 * @param request      要构建的SIP请求
 */
int eXosip_call_build_refer(struct eXosip_t *excontext, int did, const char *refer_to, osip_message_t **request);

/**
 * 构建呼叫中的默认INFO
 *
 * @param excontext    eXosip_t实例
 * @param did          对话ID
 * @param request      要构建的SIP请求
 */
int eXosip_call_build_info(struct eXosip_t *excontext, int did, osip_message_t **request);

/**
 * 构建呼叫中的默认OPTIONS
 *
 * @param excontext    eXosip_t实例
 * @param did          对话ID
 * @param request      要构建的SIP请求
 */
int eXosip_call_build_options(struct eXosip_t *excontext, int did, osip_message_t **request);

/**
 * 构建呼叫中的默认UPDATE
 *
 * @param excontext    eXosip_t实例
 * @param did          对话ID
 * @param request      要构建的SIP请求
 */
int eXosip_call_build_update(struct eXosip_t *excontext, int did, osip_message_t **request);

/**
 * 构建呼叫中的默认NOTIFY
 *
 * @param excontext             eXosip_t实例
 * @param did                   对话ID
 * @param subscription_status   请求的订阅状态
 * @param request               要构建的SIP请求
 */
int eXosip_call_build_notify(struct eXosip_t *excontext, int did, int subscription_status, osip_message_t **request);

/**
 * 发送呼叫中的请求（INVITE, OPTIONS, INFO, REFER, UPDATE）
 *
 * @param excontext    eXosip_t实例
 * @param did          对话ID
 * @param request      要发送的SIP请求
 */
int eXosip_call_send_request(struct eXosip_t *excontext, int did, osip_message_t *request);

/**
 * 构建请求的默认应答
 *
 * @param excontext    eXosip_t实例
 * @param tid          要应答的事务ID
 * @param status       使用的状态码
 * @param answer       要构建的SIP应答
 */
int eXosip_call_build_answer(struct eXosip_t *excontext, int tid, int status, osip_message_t **answer);

/**
 * 发送INVITE的应答
 *
 * @param excontext    eXosip_t实例
 * @param tid          要应答的事务ID
 * @param status       如果answer为NULL时的响应状态（不允许用于2XX）
 * @param answer       要发送的SIP应答
 */
int eXosip_call_send_answer(struct eXosip_t *excontext, int tid, int status, osip_message_t *answer);

/**
 * 终止呼叫
 * 发送CANCEL、BYE或603 Decline
 *
 * @param excontext    eXosip_t实例
 * @param cid          呼叫ID
 * @param did          对话ID
 */
int eXosip_call_terminate(struct eXosip_t *excontext, int cid, int did);

/**
 * 终止呼叫并添加Reason头
 * 发送CANCEL、BYE或603 Decline
 *
 * @param excontext    eXosip_t实例
 * @param cid          呼叫ID
 * @param did          对话ID
 * @param reason       Reason头
 */
int eXosip_call_terminate_with_reason(struct eXosip_t *excontext, int cid, int did, const char *reason);

/**
 * 终止呼叫并添加自定义头
 * 发送CANCEL、BYE或603 Decline
 *
 * @param excontext    eXosip_t实例
 * @param cid          呼叫ID
 * @param did          对话ID
 * @param header_name  头名称
 * @param header_value 头值
 */
int eXosip_call_terminate_with_header(struct eXosip_t *excontext, int cid, int did, const char *header_name, const char *header_value);

/**
 * 为INVITE构建PRACK
 *
 * @param excontext    eXosip_t实例
 * @param tid          INVITE事务ID
 * @param response1xx  要构建PRACK的SIP响应
 * @param prack        要构建的SIP PRACK
 */
int eXosip_call_build_prack(struct eXosip_t *excontext, int tid, osip_message_t *response1xx, osip_message_t **prack);

/**
 * 发送INVITE的PRACK
 *
 * @param excontext    eXosip_t实例
 * @param tid          INVITE事务ID
 * @param prack        要发送的SIP PRACK
 */
int eXosip_call_send_prack(struct eXosip_t *excontext, int tid, osip_message_t *prack);

/**
 * 从对话中获取带有Replace参数的Refer-To头
 *
 * @param excontext    eXosip_t实例
 * @param did          对话ID
 * @param refer_to     用于填充refer-to信息的缓冲区
 * @param refer_to_len refer_to缓冲区大小
 */
int eXosip_call_get_referto(struct eXosip_t *excontext, int did, char *refer_to, size_t refer_to_len);

/**
 * 通过replace头返回did（或cid）
 *
 * @param excontext    eXosip_t实例
 * @param replaces     包含refer-to信息的缓冲区
 */
int eXosip_call_find_by_replaces(struct eXosip_t *excontext, char *replaces);

/**​ @} */

#ifdef __cplusplus
}
#endif
#endif