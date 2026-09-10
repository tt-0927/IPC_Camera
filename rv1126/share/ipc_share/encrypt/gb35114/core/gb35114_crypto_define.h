/**
 * @FilePath     : gb35114_crypto_define.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-06-13
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-13
 * @Description  : GB35114 密码协议默认参数定义
 */

#pragma once

namespace Gb35114Crypto_NS
{
/* info: 与历史 gmssl 命令行封装保持一致，注册签名、CSR 和私钥解密共用该口令 */
static constexpr const char *SM2_PRIVATE_KEY_ENCRYPT_PASSWORD = "12345678";

/* info: GmSSL 默认 SM2 ID，注册签名和平台验签必须使用同一 ID，否则平台验签失败 */
static constexpr const char *SM2_DEFAULT_ID = "1234567812345678";
}
