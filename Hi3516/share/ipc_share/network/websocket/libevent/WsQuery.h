/**
 * @file WsQuery.h
 * @brief WebSocket 握手 URI 的 query 解析工具（evws 后端专用）
 *
 * evws 会话建立（evws_new_session）后 request 即被释放，query 参数必须在
 * HTTP 回调内提前解析并随连接上下文保存；本工具供该场景使用。
 * lws 后端的等价能力是 lws_get_urlarg_by_name（返回值已 urldecode）。
 */

#pragma once

#include <map>
#include <string>

namespace WsQuery
{

/**
 * @brief 从完整 URI 中提取 query 部分
 * @param strUri 形如 "/file-upload?filename=a.zip&chunk=0" 的 URI
 * @return query 子串（不含 '?'），无 query 时返回空串
 */
std::string extract_query(const std::string &strUri);

/**
 * @brief 解析 "k1=v1&k2=v2" 形式的 query，值做 %XX 与 '+' 的 urldecode
 * @param strQuery query 子串
 * @return key -> value 映射；无值参数存空串
 */
std::map<std::string, std::string> parse(const std::string &strQuery);

/**
 * @brief 取参数（等价于 lws_get_urlarg_by_name 的取值语义）
 * @param mapQuery parse() 的结果
 * @param strKey 参数名
 * @param strValue 输出参数值
 * @return true 存在该参数
 */
bool get(const std::map<std::string, std::string> &mapQuery, const std::string &strKey, std::string &strValue);

} /* namespace WsQuery */
