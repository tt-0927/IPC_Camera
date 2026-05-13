/**
 * @file sm3.h
 * @brief SM3 哈希算法头文件
 * @defgroup SM3 SM3 哈希算法
 * @{
 */

/*
 *  Copyright 2014-2023 The GmSSL Project. All Rights Reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the License); you may
 *  not use this file except in compliance with the License.
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 */

 #ifndef GMSSL_SM3_H
 #define GMSSL_SM3_H
 
 #include <string.h>
 #include <stdint.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * @brief SM3 哈希值长度（字节）
  */
 #define SM3_DIGEST_SIZE		32
 
 /**
  * @brief SM3 分组长度（字节）
  */
 #define SM3_BLOCK_SIZE		64
 
 /**
  * @brief SM3 内部状态字数
  */
 #define SM3_STATE_WORDS		8
 
 /**
  * @brief SM3 上下文结构体
  */
 typedef struct {
	 uint32_t digest[SM3_STATE_WORDS]; /**< 当前哈希状态 */
	 uint64_t nblocks;                 /**< 已处理的块数 */
	 uint8_t block[SM3_BLOCK_SIZE];    /**< 当前处理的分组 */
	 size_t num;                       /**< 当前分组中已填充的字节数 */
 } SM3_CTX;
 
 /**
  * @brief SM3 压缩函数（处理多个完整分组）
  *
  * @param digest 哈希状态数组
  * @param data 输入数据指针
  * @param blocks 要处理的分组数
  */
 void sm3_compress_blocks(uint32_t digest[8], const uint8_t *data, size_t blocks);
 
 /**
  * @brief 初始化 SM3 上下文
  *
  * @param ctx SM3 上下文指针
  */
 void sm3_init(SM3_CTX *ctx);
 
 /**
  * @brief 更新 SM3 哈希计算（处理输入数据）
  *
  * @param ctx SM3 上下文指针
  * @param data 输入数据指针
  * @param datalen 输入数据长度
  */
 void sm3_update(SM3_CTX *ctx, const uint8_t *data, size_t datalen);
 
 /**
  * @brief 完成 SM3 哈希计算并输出摘要
  *
  * @param ctx SM3 上下文指针
  * @param dgst 输出的摘要缓冲区（至少 SM3_DIGEST_SIZE 字节）
  */
 void sm3_finish(SM3_CTX *ctx, uint8_t dgst[SM3_DIGEST_SIZE]);
 
 /**
  * @brief SM3-HMAC 摘要长度（字节）
  */
 #define SM3_HMAC_SIZE		(SM3_DIGEST_SIZE)
 
 /**
  * @brief SM3-HMAC 上下文结构体
  */
 typedef struct {
	 SM3_CTX sm3_ctx;                  /**< SM3 上下文 */
	 uint8_t key[SM3_BLOCK_SIZE];      /**< 经过处理的密钥 */
 } SM3_HMAC_CTX;
 
 /**
  * @brief 初始化 SM3-HMAC 上下文
  *
  * @param ctx SM3-HMAC 上下文指针
  * @param key HMAC 密钥指针
  * @param keylen HMAC 密钥长度
  */
 void sm3_hmac_init(SM3_HMAC_CTX *ctx, const uint8_t *key, size_t keylen);
 
 /**
  * @brief 更新 SM3-HMAC 计算（处理输入数据）
  *
  * @param ctx SM3-HMAC 上下文指针
  * @param data 输入数据指针
  * @param datalen 输入数据长度
  */
 void sm3_hmac_update(SM3_HMAC_CTX *ctx, const uint8_t *data, size_t datalen);
 
 /**
  * @brief 完成 SM3-HMAC 计算并输出 MAC
  *
  * @param ctx SM3-HMAC 上下文指针
  * @param mac 输出的 MAC 缓冲区（至少 SM3_HMAC_SIZE 字节）
  */
 void sm3_hmac_finish(SM3_HMAC_CTX *ctx, uint8_t mac[SM3_HMAC_SIZE]);
 
 /**
  * @brief SM3-KDF 上下文结构体
  */
 typedef struct {
	 SM3_CTX sm3_ctx;                  /**< SM3 上下文 */
	 size_t outlen;                    /**< 期望输出的密钥长度 */
 } SM3_KDF_CTX;
 
 /**
  * @brief 初始化 SM3-KDF 上下文
  *
  * @param ctx SM3-KDF 上下文指针
  * @param outlen 期望输出的密钥长度
  */
 void sm3_kdf_init(SM3_KDF_CTX *ctx, size_t outlen);
 
 /**
  * @brief 更新 SM3-KDF 计算（处理输入数据）
  *
  * @param ctx SM3-KDF 上下文指针
  * @param in 输入数据指针
  * @param inlen 输入数据长度
  */
 void sm3_kdf_update(SM3_KDF_CTX *ctx, const uint8_t *in, size_t inlen);
 
 /**
  * @brief 完成 SM3-KDF 计算并输出密钥
  *
  * @param ctx SM3-KDF 上下文指针
  * @param out 输出的密钥缓冲区
  */
 void sm3_kdf_finish(SM3_KDF_CTX *ctx, uint8_t *out);
 
 /**
  * @brief PBKDF2 最小迭代次数
  */
 #define SM3_PBKDF2_MIN_ITER		10000
 
 /**
  * @brief PBKDF2 最大迭代次数
  */
 #define SM3_PBKDF2_MAX_ITER		(16777216-1)
 
 /**
  * @brief PBKDF2 最大盐值长度
  */
 #define SM3_PBKDF2_MAX_SALT_SIZE	64
 
 /**
  * @brief PBKDF2 默认盐值长度
  */
 #define SM3_PBKDF2_DEFAULT_SALT_SIZE	8
 
 /**
  * @brief SM3-PBKDF2 密钥派生函数
  *
  * @param pass 口令字符串
  * @param passlen 口令长度
  * @param salt 盐值指针
  * @param saltlen 盐值长度
  * @param count 迭代次数
  * @param outlen 期望输出的密钥长度
  * @param out 输出的密钥缓冲区
  * @return 成功返回1，失败返回0
  */
 int sm3_pbkdf2(const char *pass, size_t passlen,
	 const uint8_t *salt, size_t saltlen, size_t count,
	 size_t outlen, uint8_t *out);
 
 /**
  * @brief SM3 摘要算法上下文结构体
  */
 typedef struct {
	 union {
		 SM3_CTX sm3_ctx;              /**< SM3 上下文 */
		 SM3_HMAC_CTX hmac_ctx;        /**< SM3-HMAC 上下文 */
	 };
	 int state;                        /**< 当前状态 */
 } SM3_DIGEST_CTX;
 
 /**
  * @brief 初始化 SM3 摘要算法上下文
  *
  * @param ctx SM3 摘要上下文指针
  * @param key HMAC 密钥指针（如为 NULL 则使用普通 SM3）
  * @param keylen HMAC 密钥长度
  * @return 成功返回1，失败返回0
  */
 int sm3_digest_init(SM3_DIGEST_CTX *ctx, const uint8_t *key, size_t keylen);
 
 /**
  * @brief 更新 SM3 摘要计算（处理输入数据）
  *
  * @param ctx SM3 摘要上下文指针
  * @param data 输入数据指针
  * @param datalen 输入数据长度
  * @return 成功返回1，失败返回0
  */
 int sm3_digest_update(SM3_DIGEST_CTX *ctx, const uint8_t *data, size_t datalen);
 
 /**
  * @brief 完成 SM3 摘要计算并输出结果
  *
  * @param ctx SM3 摘要上下文指针
  * @param dgst 输出的摘要缓冲区（至少 SM3_DIGEST_SIZE 字节）
  * @return 成功返回1，失败返回0
  */
 int sm3_digest_finish(SM3_DIGEST_CTX *ctx, uint8_t dgst[SM3_DIGEST_SIZE]);
 
 #ifdef __cplusplus
 }
 #endif
 #endif
 
 /** @} */ // end of SM3 group