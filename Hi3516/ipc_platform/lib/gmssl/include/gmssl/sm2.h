/*
 *  Copyright 2014-2024 The GmSSL Project. All Rights Reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the License); you may
 *  not use this file except in compliance with the License.
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 */

 #ifndef GMSSL_SM2_H
 #define GMSSL_SM2_H
 
 #include <stdio.h>
 #include <stdint.h>
 #include <stdlib.h>
 #include <gmssl/sm3.h>
 #include <gmssl/sm2_z256.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * SM2密钥对结构体
  * 包含SM2的公钥和私钥
  */
 typedef struct {
	 SM2_Z256_POINT public_key;  /* SM2公钥，椭圆曲线上的点 */
	 sm2_z256_t private_key;     /* SM2私钥，大整数 */
 } SM2_KEY;
 
 /**
  * 生成SM2密钥对
  * @param key [out] 生成的SM2密钥对
  * @return 成功返回1，失败返回0
  */
 int sm2_key_generate(SM2_KEY *key);
 
 /**
  * 打印SM2密钥信息
  * @param fp [in] 输出文件指针
  * @param fmt [in] 格式控制
  * @param ind [in] 缩进量
  * @param label [in] 标签文本
  * @param key [in] SM2密钥
  * @return 成功返回1，失败返回0
  */
 int sm2_key_print(FILE *fp, int fmt, int ind, const char *label, const SM2_KEY *key);
 
 /**
  * 设置SM2私钥
  * @param key [out] SM2密钥结构
  * @param private_key [in] 要设置的私钥
  * @return 成功返回1，失败返回0
  */
 int sm2_key_set_private_key(SM2_KEY *key, const sm2_z256_t private_key);
 
 /**
  * 设置SM2公钥
  * @param key [out] SM2密钥结构
  * @param public_key [in] 要设置的公钥
  * @return 成功返回1，失败返回0
  */
 int sm2_key_set_public_key(SM2_KEY *key, const SM2_Z256_POINT *public_key);
 
 /**
  * 比较两个SM2公钥是否相等
  * @param sm2_key [in] 第一个SM2密钥
  * @param pub_key [in] 第二个SM2密钥
  * @return 相等返回1，否则返回0
  */
 int sm2_public_key_equ(const SM2_KEY *sm2_key, const SM2_KEY *pub_key);
 
 /**
  * 计算SM2公钥的摘要
  * @param key [in] SM2密钥
  * @param dgst [out] 输出的摘要值(32字节)
  * @return 成功返回1，失败返回0
  */
 int sm2_public_key_digest(const SM2_KEY *key, uint8_t dgst[32]);
 
 /**
  * 打印SM2公钥信息
  * @param fp [in] 输出文件指针
  * @param fmt [in] 格式控制
  * @param ind [in] 缩进量
  * @param label [in] 标签文本
  * @param pub_key [in] SM2公钥
  * @return 成功返回1，失败返回0
  */
 int sm2_public_key_print(FILE *fp, int fmt, int ind, const char *label, const SM2_KEY *pub_key);
 
 /* RFC 5915定义的EC私钥结构 */
 #define SM2_PRIVATE_KEY_DEFAULT_SIZE 120 /* 默认生成的私钥大小 */
 #define SM2_PRIVATE_KEY_BUF_SIZE 512     /* 缓冲区大小，必须>=SM2_PRIVATE_KEY_DEFAULT_SIZE */
 
 /**
  * 将SM2私钥转换为DER格式
  * @param key [in] SM2密钥
  * @param out [out] 输出的DER数据
  * @param outlen [out] 输出的DER数据长度
  * @return 成功返回1，失败返回0
  */
 int sm2_private_key_to_der(const SM2_KEY *key, uint8_t **out, size_t *outlen);
 
 /**
  * 从DER格式解析SM2私钥
  * @param key [out] 解析得到的SM2密钥
  * @param in [in] 输入的DER数据
  * @param inlen [in] 输入的DER数据长度
  * @return 成功返回1，失败返回0
  */
 int sm2_private_key_from_der(SM2_KEY *key, const uint8_t **in, size_t *inlen);
 
 /**
  * 打印SM2私钥信息
  * @param fp [in] 输出文件指针
  * @param fmt [in] 格式控制
  * @param ind [in] 缩进量
  * @param label [in] 标签文本
  * @param d [in] 私钥数据
  * @param dlen [in] 私钥数据长度
  * @return 成功返回1，失败返回0
  */
 int sm2_private_key_print(FILE *fp, int fmt, int ind, const char *label, const uint8_t *d, size_t dlen);
 
 /**
  * 将SM2私钥写入PEM文件
  * @param key [in] SM2密钥
  * @param fp [in] 文件指针
  * @return 成功返回1，失败返回0
  */
 int sm2_private_key_to_pem(const SM2_KEY *key, FILE *fp);
 
 /**
  * 从PEM文件读取SM2私钥
  * @param key [out] 读取的SM2密钥
  * @param fp [in] 文件指针
  * @return 成功返回1，失败返回0
  */
 int sm2_private_key_from_pem(SM2_KEY *key, FILE *fp);
 
 /* 算法标识结构 */
 int sm2_public_key_algor_to_der(uint8_t **out, size_t *outlen);
 int sm2_public_key_algor_from_der(const uint8_t **in, size_t *inlen);
 
 /* RFC 5280定义的主题公钥信息结构 */
 int sm2_public_key_info_to_der(const SM2_KEY *a, uint8_t **out, size_t *outlen);
 int sm2_public_key_info_from_der(SM2_KEY *a, const uint8_t **in, size_t *inlen);
 int sm2_public_key_info_to_pem(const SM2_KEY *a, FILE *fp);
 int sm2_public_key_info_from_pem(SM2_KEY *a, FILE *fp);
 
 /* RFC 5208定义的PKCS #8私钥信息结构 */
 enum {
	 PKCS8_private_key_info_version = 0, /* PKCS8版本号 */
 };
 
 int sm2_private_key_info_to_der(const SM2_KEY *key, uint8_t **out, size_t *outlen);
 int sm2_private_key_info_from_der(SM2_KEY *key, const uint8_t **attrs, size_t *attrslen, const uint8_t **in, size_t *inlen);
 int sm2_private_key_info_print(FILE *fp, int fmt, int ind, const char *label, const uint8_t *d, size_t dlen);
 int sm2_private_key_info_to_pem(const SM2_KEY *key, FILE *fp);
 int sm2_private_key_info_from_pem(SM2_KEY *key, FILE *fp);
 
 /** 加密的私钥信息结构 */
 int sm2_private_key_info_encrypt_to_der(const SM2_KEY *key,
	 const char *pass, uint8_t **out, size_t *outlen);
 int sm2_private_key_info_decrypt_from_der(SM2_KEY *key, const uint8_t **attrs, size_t *attrs_len,
	 const char *pass, const uint8_t **in, size_t *inlen);
 int sm2_private_key_info_encrypt_to_pem(const SM2_KEY *key, const char *pass, FILE *fp);
 int sm2_private_key_info_decrypt_from_pem(SM2_KEY *key, const char *pass, FILE *fp);
 
 /**
  * SM2签名结构体
  * 包含签名值r和s
  */
 typedef struct {
	 uint8_t r[32];  /* 签名r值 */
	 uint8_t s[32];  /* 签名s值 */
 } SM2_SIGNATURE;
 
 /**
  * SM2签名操作
  * @param key [in] SM2密钥
  * @param dgst [in] 待签名的摘要(32字节)
  * @param sig [out] 生成的签名
  * @return 成功返回1，失败返回0
  */
 int sm2_do_sign(const SM2_KEY *key, const uint8_t dgst[32], SM2_SIGNATURE *sig);
 
 /**
  * SM2验证签名
  * @param key [in] SM2密钥
  * @param dgst [in] 待验证的摘要(32字节)
  * @param sig [in] 待验证的签名
  * @return 验证成功返回1，失败返回0
  */
 int sm2_do_verify(const SM2_KEY *key, const uint8_t dgst[32], const SM2_SIGNATURE *sig);
 
 /**
  * 计算快速签名所需的密钥
  * @param key [in] SM2密钥
  * @param fast_private [out] 计算得到的快速签名私钥
  * @return 成功返回1，失败返回0
  */
 int sm2_fast_sign_compute_key(const SM2_KEY *key, sm2_z256_t fast_private);
 
 /**
  * SM2签名预计算结构体
  * 用于加速签名过程
  */
 typedef struct {
	 sm2_z256_t k;           /* 随机数k */
	 sm2_z256_t x1_modn;     /* x1 mod n */
 } SM2_SIGN_PRE_COMP;
 
 #define SM2_SIGN_PRE_COMP_COUNT 32  /** 预计算项数量 */
 
 /**
  * SM2签名预计算
  * @param pre_comp [out] 预计算结果数组
  * @return 成功返回1，失败返回0
  */
 int sm2_fast_sign_pre_compute(SM2_SIGN_PRE_COMP pre_comp[32]);
 
 /**
  * SM2快速签名
  * @param fast_private [in] 快速签名私钥
  * @param pre_comp [in] 预计算结果
  * @param dgst [in] 待签名的摘要(32字节)
  * @param sig [out] 生成的签名
  * @return 成功返回1，失败返回0
  */
 int sm2_fast_sign(const sm2_z256_t fast_private, SM2_SIGN_PRE_COMP *pre_comp,
	 const uint8_t dgst[32], SM2_SIGNATURE *sig);
 
 /**
  * SM2快速验证签名
  * @param point_table [in] 预计算的公钥点表
  * @param dgst [in] 待验证的摘要(32字节)
  * @param sig [in] 待验证的签名
  * @return 验证成功返回1，失败返回0
  */
 int sm2_fast_verify(const SM2_Z256_POINT point_table[16],
	 const uint8_t dgst[32], const SM2_SIGNATURE *sig);
 
 #define SM2_MIN_SIGNATURE_SIZE 8    /* 最小签名长度 */
 #define SM2_MAX_SIGNATURE_SIZE 72   /* 最大签名长度 */
 
 int sm2_signature_to_der(const SM2_SIGNATURE *sig, uint8_t **out, size_t *outlen);
 int sm2_signature_from_der(SM2_SIGNATURE *sig, const uint8_t **in, size_t *inlen);
 int sm2_signature_print(FILE *fp, int fmt, int ind, const char *label, const uint8_t *sig, size_t siglen);
 int sm2_sign(const SM2_KEY *key, const uint8_t dgst[32], uint8_t *sig, size_t *siglen);
 int sm2_verify(const SM2_KEY *key, const uint8_t dgst[32], const uint8_t *sig, size_t siglen);
 
 enum {
	 SM2_signature_compact_size = 70,    /* 紧凑格式签名大小 */
	 SM2_signature_typical_size = 71,    /* 典型签名大小 */
	 SM2_signature_max_size = 72,        /* 最大签名大小 */
 };
 
 /**
  * 生成固定长度的SM2签名
  * @param key [in] SM2密钥
  * @param dgst [in] 待签名的摘要(32字节)
  * @param siglen [in] 期望的签名长度
  * @param sig [out] 生成的签名
  * @return 成功返回1，失败返回0
  */
 int sm2_sign_fixlen(const SM2_KEY *key, const uint8_t dgst[32], size_t siglen, uint8_t *sig);
 
 /** 默认SM2 ID定义 */
 #define SM2_DEFAULT_ID       "1234567812345678"
 #define SM2_DEFAULT_ID_LENGTH    (sizeof(SM2_DEFAULT_ID) - 1)  /* 默认ID长度(字节) */
 #define SM2_DEFAULT_ID_BITS  (SM2_DEFAULT_ID_LENGTH * 8)       /* 默认ID长度(比特) */
 #define SM2_MAX_ID_BITS      65535                              /* 最大ID比特数 */
 #define SM2_MAX_ID_LENGTH    (SM2_MAX_ID_BITS/8)               /* 最大ID字节数 */
 
 /**
  * 计算SM2签名所需的Z值
  * @param z [out] 计算的Z值(32字节)
  * @param pub [in] 公钥点
  * @param id [in] 用户ID
  * @param idlen [in] 用户ID长度
  * @return 成功返回1，失败返回0
  */
 int sm2_compute_z(uint8_t z[32], const SM2_Z256_POINT *pub, const char *id, size_t idlen);
 
 /**
  * SM2签名上下文结构体
  * 用于增量式签名操作
  */
 typedef struct {
	 SM3_CTX sm3_ctx;                    /* SM3哈希上下文 */
	 SM3_CTX saved_sm3_ctx;              /* 保存的SM3哈希上下文 */
	 SM2_KEY key;                        /* SM2密钥 */
	 sm2_z256_t fast_sign_private;      /* 快速签名私钥 */
	 SM2_SIGN_PRE_COMP pre_comp[SM2_SIGN_PRE_COMP_COUNT]; /* 预计算结果 */
	 unsigned int num_pre_comp;          /* 预计算结果数量 */
 
	 /** 验证公钥点表，包含P, 2P, ..., 16P */
	 SM2_Z256_POINT public_point_table[16];
 } SM2_SIGN_CTX;
 
 /**
  * 初始化SM2签名上下文
  * @param ctx [out] 签名上下文
  * @param key [in] SM2密钥
  * @param id [in] 用户ID
  * @param idlen [in] 用户ID长度
  * @return 成功返回1，失败返回0
  */
 int sm2_sign_init(SM2_SIGN_CTX *ctx, const SM2_KEY *key, const char *id, size_t idlen);
 
 /**
  * 更新SM2签名数据
  * @param ctx [in/out] 签名上下文
  * @param data [in] 待签名数据
  * @param datalen [in] 待签名数据长度
  * @return 成功返回1，失败返回0
  */
 int sm2_sign_update(SM2_SIGN_CTX *ctx, const uint8_t *data, size_t datalen);
 
 /**
  * 完成SM2签名操作
  * @param ctx [in/out] 签名上下文
  * @param sig [out] 生成的签名
  * @param siglen [out] 生成的签名长度
  * @return 成功返回1，失败返回0
  */
 int sm2_sign_finish(SM2_SIGN_CTX *ctx, uint8_t *sig, size_t *siglen);
 
 /**
  * 重置SM2签名上下文
  * @param ctx [in/out] 签名上下文
  * @return 成功返回1，失败返回0
  */
 int sm2_sign_reset(SM2_SIGN_CTX *ctx);
 
 /**
  * 完成固定长度的SM2签名操作
  * @param ctx [in/out] 签名上下文
  * @param siglen [in] 期望的签名长度
  * @param sig [out] 生成的签名
  * @return 成功返回1，失败返回0
  */
 int sm2_sign_finish_fixlen(SM2_SIGN_CTX *ctx, size_t siglen, uint8_t *sig);
 
 /**
  * SM2验证签名上下文结构体
  * 用于增量式验证签名操作
  */
 typedef struct {
	 SM3_CTX sm3_ctx;                    /* SM3哈希上下文 */
	 SM3_CTX saved_sm3_ctx;              /* 保存的SM3哈希上下文 */
	 SM2_KEY key;                        /* SM2密钥 */
	 SM2_Z256_POINT public_point_table[16]; /* 预计算的公钥点表 */
 } SM2_VERIFY_CTX;
 
 /**
  * 初始化SM2验证签名上下文
  * @param ctx [out] 验证上下文
  * @param key [in] SM2密钥
  * @param id [in] 用户ID
  * @param idlen [in] 用户ID长度
  * @return 成功返回1，失败返回0
  */
 int sm2_verify_init(SM2_VERIFY_CTX *ctx, const SM2_KEY *key, const char *id, size_t idlen);
 
 /**
  * 更新SM2验证签名数据
  * @param ctx [in/out] 验证上下文
  * @param data [in] 待验证数据
  * @param datalen [in] 待验证数据长度
  * @return 成功返回1，失败返回0
  */
 int sm2_verify_update(SM2_VERIFY_CTX *ctx, const uint8_t *data, size_t datalen);
 
 /**
  * 完成SM2验证签名操作
  * @param ctx [in/out] 验证上下文
  * @param sig [in] 待验证的签名
  * @param siglen [in] 签名的长度
  * @return 验证成功返回1，失败返回0
  */
 int sm2_verify_finish(SM2_VERIFY_CTX *ctx, const uint8_t *sig, size_t siglen);
 
 /**
  * 重置SM2验证签名上下文
  * @param ctx [in/out] 验证上下文
  * @return 成功返回1，失败返回0
  */
 int sm2_verify_reset(SM2_VERIFY_CTX *ctx);
 
 /** SM2密文结构定义 */
 #define SM2_MIN_PLAINTEXT_SIZE  1   /* 最小明文长度 */
 #define SM2_MAX_PLAINTEXT_SIZE  255 /* 最大明文长度 */
 
 /**
  * SM2点结构体
  * 包含x和y坐标
  */
 typedef struct {
	 uint8_t x[32];  /* x坐标 */
	 uint8_t y[32];  /* y坐标 */
 } SM2_POINT;
 
 /**
  * SM2密文结构体
  * 包含加密后的数据
  */
 typedef struct {
	 SM2_POINT point;                /* 加密使用的临时公钥点 */
	 uint8_t hash[32];               /* 哈希值 */
	 uint8_t ciphertext_size;        /* 密文长度 */
	 uint8_t ciphertext[SM2_MAX_PLAINTEXT_SIZE]; /* 密文数据 */
 } SM2_CIPHERTEXT;
 
 /**
  * SM2密钥派生函数(KDF)
  * @param in [in] 输入数据
  * @param inlen [in] 输入数据长度
  * @param outlen [in] 期望的输出长度
  * @param out [out] 输出的密钥数据
  * @return 成功返回1，失败返回0
  */
 int sm2_kdf(const uint8_t *in, size_t inlen, size_t outlen, uint8_t *out);
 
 /**
  * SM2加密操作
  * @param key [in] SM2公钥
  * @param in [in] 待加密的明文
  * @param inlen [in] 明文长度
  * @param out [out] 输出的密文
  * @return 成功返回1，失败返回0
  */
 int sm2_do_encrypt(const SM2_KEY *key, const uint8_t *in, size_t inlen, SM2_CIPHERTEXT *out);
 
 /**
  * SM2解密操作
  * @param key [in] SM2私钥
  * @param in [in] 待解密的密文
  * @param out [out] 解密后的明文
  * @param outlen [out] 明文的长度
  * @return 成功返回1，失败返回0
  */
 int sm2_do_decrypt(const SM2_KEY *key, const SM2_CIPHERTEXT *in, uint8_t *out, size_t *outlen);
 
 #define SM2_MIN_CIPHERTEXT_SIZE   45  /* 最小密文长度 */
 #define SM2_MAX_CIPHERTEXT_SIZE   366 /* 最大密文长度 */
 
 int sm2_ciphertext_to_der(const SM2_CIPHERTEXT *c, uint8_t **out, size_t *outlen);
 int sm2_ciphertext_from_der(SM2_CIPHERTEXT *c, const uint8_t **in, size_t *inlen);
 int sm2_ciphertext_print(FILE *fp, int fmt, int ind, const char *label, const uint8_t *a, size_t alen);
 int sm2_encrypt(const SM2_KEY *key, const uint8_t *in, size_t inlen, uint8_t *out, size_t *outlen);
 int sm2_decrypt(const SM2_KEY *key, const uint8_t *in, size_t inlen, uint8_t *out, size_t *outlen);
 
 enum {
	 SM2_ciphertext_compact_point_size = 68, /* 紧凑格式点大小 */
	 SM2_ciphertext_typical_point_size = 69, /* 典型点大小 */
	 SM2_ciphertext_max_point_size = 70,     /* 最大点大小 */
 };
 
 /**
  * 生成固定点长度的SM2加密结果
  * @param key [in] SM2公钥
  * @param in [in] 待加密的明文
  * @param inlen [in] 明文长度
  * @param point_size [in] 期望的点长度
  * @param out [out] 输出的密文
  * @return 成功返回1，失败返回0
  */
 int sm2_do_encrypt_fixlen(const SM2_KEY *key, const uint8_t *in, size_t inlen, int point_size, SM2_CIPHERTEXT *out);
 
 /**
  * SM2加密并生成固定长度的结果
  * @param key [in] SM2公钥
  * @param in [in] 待加密的明文
  * @param inlen [in] 明文长度
  * @param point_size [in] 期望的点长度
  * @param out [out] 输出的密文
  * @param outlen [out] 密文长度
  * @return 成功返回1，失败返回0
  */
 int sm2_encrypt_fixlen(const SM2_KEY *key, const uint8_t *in, size_t inlen, int point_size, uint8_t *out, size_t *outlen);
 
 /**
  * SM2密钥交换操作
  * @param key [in] 本地SM2密钥
  * @param peer_public [in] 对端公钥
  * @param out [out] 生成的共享密钥点
  * @return 成功返回1，失败返回0
  */
 int sm2_do_ecdh(const SM2_KEY *key, const SM2_Z256_POINT *peer_public, SM2_Z256_POINT *out);
 
 /**
  * SM2密钥交换
  * @param key [in] 本地SM2密钥
  * @param peer_public [in] 对端公钥数据
  * @param peer_public_len [in] 对端公钥长度
  * @param out [out] 输出的共享密钥(64字节)
  * @return 成功返回1，失败返回0
  */
 int sm2_ecdh(const SM2_KEY *key, const uint8_t *peer_public, size_t peer_public_len, uint8_t out[64]);
 
 /**
  * SM2加密预计算结构体
  * 用于加速加密过程
  */
 typedef struct {
	 sm2_z256_t k;       /* 随机数k */
	 SM2_POINT C1;       /* 临时公钥点 */
 } SM2_ENC_PRE_COMP;
 
 #define SM2_ENC_PRE_COMP_NUM 8  /* 预计算项数量 */
 
 /**
  * SM2加密预计算
  * @param pre_comp [out] 预计算结果数组
  * @return 成功返回1，失败返回0
  */
 int sm2_encrypt_pre_compute(SM2_ENC_PRE_COMP pre_comp[SM2_ENC_PRE_COMP_NUM]);
 
 /**
  * 使用预计算结果的SM2加密
  * @param key [in] SM2公钥
  * @param pre_comp [in] 预计算结果
  * @param in [in] 待加密的明文
  * @param inlen [in] 明文长度
  * @param out [out] 输出的密文
  * @return 成功返回1，失败返回0
  */
 int sm2_do_encrypt_ex(const SM2_KEY *key, const SM2_ENC_PRE_COMP *pre_comp,
	 const uint8_t *in, size_t inlen, SM2_CIPHERTEXT *out);
 
 /**
  * SM2加密上下文结构体
  * 用于增量式加密操作
  */
 typedef struct {
	 SM2_ENC_PRE_COMP pre_comp[SM2_ENC_PRE_COMP_NUM]; /* 预计算结果 */
	 size_t pre_comp_num;                             /* 预计算结果数量 */
	 uint8_t buf[SM2_MAX_PLAINTEXT_SIZE];             /* 缓冲区 */
	 size_t buf_size;                                 /* 缓冲区数据大小 */
 } SM2_ENC_CTX;
 
 /**
  * 初始化SM2加密上下文
  * @param ctx [out] 加密上下文
  * @return 成功返回1，失败返回0
  */
 int sm2_encrypt_init(SM2_ENC_CTX *ctx);
 
 /**
  * 更新SM2加密数据
  * @param ctx [in/out] 加密上下文
  * @param in [in] 待加密数据
  * @param inlen [in] 待加密数据长度
  * @return 成功返回1，失败返回0
  */
 int sm2_encrypt_update(SM2_ENC_CTX *ctx, const uint8_t *in, size_t inlen);
 
 /**
  * 完成SM2加密操作
  * @param ctx [in/out] 加密上下文
  * @param public_key [in] SM2公钥
  * @param out [out] 输出的密文
  * @param outlen [out] 密文长度
  * @return 成功返回1，失败返回0
  */
 int sm2_encrypt_finish(SM2_ENC_CTX *ctx, const SM2_KEY *public_key, uint8_t *out, size_t *outlen);
 
 /**
  * 重置SM2加密上下文
  * @param ctx [in/out] 加密上下文
  * @return 成功返回1，失败返回0
  */
 int sm2_encrypt_reset(SM2_ENC_CTX *ctx);
 
 /**
  * SM2解密上下文结构体
  * 用于增量式解密操作
  */
 typedef struct {
	 uint8_t buf[SM2_MAX_CIPHERTEXT_SIZE]; /* 缓冲区 */
	 size_t buf_size;                      /* 缓冲区数据大小 */
 } SM2_DEC_CTX;
 
 /**
  * 初始化SM2解密上下文
  * @param ctx [out] 解密上下文
  * @return 成功返回1，失败返回0
  */
 int sm2_decrypt_init(SM2_DEC_CTX *ctx);
 
 /**
  * 更新SM2解密数据
  * @param ctx [in/out] 解密上下文
  * @param in [in] 待解密数据
  * @param inlen [in] 待解密数据长度
  * @return 成功返回1，失败返回0
  */
 int sm2_decrypt_update(SM2_DEC_CTX *ctx, const uint8_t *in, size_t inlen);
 
 /**
  * 完成SM2解密操作
  * @param ctx [in/out] 解密上下文
  * @param key [in] SM2私钥
  * @param out [out] 解密后的明文
  * @param outlen [out] 明文长度
  * @return 成功返回1，失败返回0
  */
 int sm2_decrypt_finish(SM2_DEC_CTX *ctx, const SM2_KEY *key, uint8_t *out, size_t *outlen);
 
 /**
  * 重置SM2解密上下文
  * @param ctx [in/out] 解密上下文
  * @return 成功返回1，失败返回0
  */
 int sm2_decrypt_reset(SM2_DEC_CTX *ctx);
 
 #ifdef __cplusplus
 }
 #endif
 #endif