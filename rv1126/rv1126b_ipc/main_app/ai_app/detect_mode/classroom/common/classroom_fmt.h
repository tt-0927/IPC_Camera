/**
 * @FilePath     : classroom_fmt.h
 * @Author       : leiyy
 * @Date         : 2026-10-23 10:56:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-23 10:56:00
 * @Description  : 报文/心跳行的拼装原语
 */

#pragma once

#include <string>

namespace Classroom
{

/**
 * @brief   : 往 JSON 报文追加一段格式化文本。定长栈缓冲放得下就一趟，
 *            放不下按实长走第二趟——**任何一段都完整落地**，不产出非法 JSON
 */
void appendFmt(std::string &rstrJson, const char *pFormat, ...);

/**
 * @brief   : 拼一条完整的心跳行并返回。与 appendFmt 同一套两趟口径
 */
std::string fmtMetricLine(const char *pFormat, ...);

/**
 * @brief   : 按字节上限截断，且**不切断一个 UTF-8 序列**
 */
std::string truncateUtf8(const std::string &strIn, int32_t nMaxBytes, bool &rbTruncated);

} /* namespace Classroom */
