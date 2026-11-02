/**
 * @FilePath     : sha256.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-10-19 15:00:00
 * @Description  : SHA-256 哈希接口（流式计算与文件哈希）
 */

#ifndef _IPC_SHARE_SHA256_H_
#define _IPC_SHARE_SHA256_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* SHA-256 上下文：分段喂数据，流式计算，不落中间文件 */
typedef struct Sha256Context_S
{
    unsigned int state[8];     /* 中间哈希状态（8 个 32 位字） */
    unsigned long long bitlen; /* 已处理的总位数 */
    unsigned char buffer[64];  /* 不足一个分组（64 字节）的暂存数据 */
    size_t buflen;             /* 暂存区有效字节数 */
} Sha256Context_T;

/* 初始化上下文，开始一次新的计算 */
void sha256_init(Sha256Context_T *ctx);

/* 追加一段数据；可多次调用，顺序即数据顺序 */
void sha256_update(Sha256Context_T *ctx, const unsigned char *data, size_t len);

/* 结束计算，输出 32 字节摘要；调用后上下文作废，需重新 init */
void sha256_final(Sha256Context_T *ctx, unsigned char digest[32]);

/**
 * @brief   : 计算整个文件的 SHA-256，输出小写十六进制字符串
 * @param    {const char*} pchPath：文件路径
 * @param    {char*} pchHexOut：输出缓冲区，至少 65 字节（64 个字符 + 结尾 \0）
 * @return   {int} 0 成功，-1 打开/读取失败
 * @note    : 内部按 16KB 分块流式读取，内存占用与文件大小无关
 */
int sha256_file_hex(const char *pchPath, char *pchHexOut);

#ifdef __cplusplus
}
#endif

#endif /* _IPC_SHARE_SHA256_H_ */
