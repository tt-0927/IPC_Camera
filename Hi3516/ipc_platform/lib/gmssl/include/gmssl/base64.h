/*
 *  Copyright 2014-2023 The GmSSL Project. All Rights Reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the License); you may
 *  not use this file except in compliance with the License.
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef GMSSL_BASE64_H
#define GMSSL_BASE64_H

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*Base64编解码上下文结构体 用于保存Base64编解码过程中的中间状态*/
typedef struct
{
    int num;                    /* 当前已处理但未完成编解码的字节数 */
    int length;                 /* 编码时为输出行长度，解码时为允许的最短输入行长度 */
    unsigned char enc_data[80]; /* 存储待编码/解码的临时数据缓冲区 */
    int line_num;               /* 当前已处理的行数 */
    int expect_nl;              /* 是否期望换行符 */
} BASE64_CTX;

/* Base64编码后最大长度计算公式 */
#define BASE64_ENCODE_LENGTH(l) (((l + 2) / 3 * 4) + (l / 48 + 1) * 2 + 80)
/* Base64解码后最大长度计算公式 */
#define BASE64_DECODE_LENGTH(l) ((l + 3) / 4 * 3 + 80)

/**
 * 初始化Base64编码上下文
 * @param ctx [out] Base64编码上下文指针
 */
void base64_encode_init(BASE64_CTX *ctx);

/**
 * Base64编码更新操作
 * @param ctx [in/out] Base64编码上下文指针
 * @param in [in] 待编码的输入数据
 * @param inlen [in] 输入数据长度
 * @param out [out] 编码输出缓冲区
 * @param outlen [out] 编码输出长度
 * @return 成功返回1，失败返回0
 */
int base64_encode_update(BASE64_CTX *ctx, const uint8_t *in, int inlen, uint8_t *out, int *outlen);

/**
 * 完成Base64编码操作
 * @param ctx [in/out] Base64编码上下文指针
 * @param out [out] 编码输出缓冲区
 * @param outlen [out] 编码输出长度
 */
void base64_encode_finish(BASE64_CTX *ctx, uint8_t *out, int *outlen);

/**
 * 初始化Base64解码上下文
 * @param ctx [out] Base64解码上下文指针
 */
void base64_decode_init(BASE64_CTX *ctx);

/**
 * Base64解码更新操作
 * @param ctx [in/out] Base64解码上下文指针
 * @param in [in] 待解码的输入数据
 * @param inlen [in] 输入数据长度
 * @param out [out] 解码输出缓冲区
 * @param outlen [out] 解码输出长度
 * @return 成功返回1，失败返回0
 */
int base64_decode_update(BASE64_CTX *ctx, const uint8_t *in, int inlen, uint8_t *out, int *outlen);

/**
 * 完成Base64解码操作
 * @param ctx [in/out] Base64解码上下文指针
 * @param out [out] 解码输出缓冲区
 * @param outlen [out] 解码输出长度
 * @return 成功返回1，失败返回0
 */
int base64_decode_finish(BASE64_CTX *ctx, uint8_t *out, int *outlen);

/**
 * Base64单块编码函数
 * @param t [out] 编码输出缓冲区
 * @param f [in] 待编码的输入数据
 * @param dlen [in] 输入数据长度
 * @return 返回编码后的数据长度
 */
int base64_encode_block(unsigned char *t, const unsigned char *f, int dlen);

/**
 * Base64单块解码函数
 * @param t [out] 解码输出缓冲区
 * @param f [in] 待解码的输入数据
 * @param n [in] 输入数据长度
 * @return 返回解码后的数据长度
 */
int base64_decode_block(unsigned char *t, const unsigned char *f, int n);

#ifdef __cplusplus
}
#endif
#endif