/*
*  Copyright 2014-2022 The GmSSL Project. All Rights Reserved.
*
*  Licensed under the Apache License, Version 2.0 (the License); you may
*  not use this file except in compliance with the License.
*
*  http://www.apache.org/licenses/LICENSE-2.0
*/

#ifndef GMSSL_X509_ALG_H
#define GMSSL_X509_ALG_H

#include <time.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <gmssl/sm2.h>
#include <gmssl/oid.h>
#include <gmssl/asn1.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file x509_alg.h
 * @brief X.509 中的算法标识符（AlgorithmIdentifier）相关接口，支持 DER 编解码与打印。
 *
 * AlgorithmIdentifier ::= SEQUENCE {
 *     algorithm      OBJECT IDENTIFIER,
 *     parameters     ANY DEFINED BY algorithm OPTIONAL
 * }
 */

/** @name 摘要算法相关接口
 *  @{
 */

/**
 * @brief 获取摘要算法名称（如 "sm3"）
 * @param oid 摘要算法 OID
 * @return 算法名称字符串
 */
const char *x509_digest_algor_name(int oid);

/**
 * @brief 根据算法名称获取摘要算法 OID
 * @param name 算法名称
 * @return 成功返回 OID，失败返回错误码
 */
int x509_digest_algor_from_name(const char *name);

/**
 * @brief 从 DER 编码中解析摘要算法标识符（OID）
 * @param[out] oid 返回解析出的 OID
 * @param[in,out] in DER 数据缓冲区指针
 * @param[in,out] inlen 缓冲区长度
 * @return 成功返回 1，失败返回错误码
 */
int x509_digest_algor_from_der(int *oid, const uint8_t **in, size_t *inlen);

/**
 * @brief 将摘要算法标识符编码为 DER 格式
 * @param oid 摘要算法 OID
 * @param[out] out 输出 DER 编码
 * @param[out] outlen 输出长度
 * @return 成功返回 1，失败返回错误码
 */
int x509_digest_algor_to_der(int oid, uint8_t **out, size_t *outlen);

/**
 * @brief 打印 DER 编码的摘要算法内容（调试用途）
 */
int x509_digest_algor_print(FILE *fp, int fmt, int ind, const char *label, const uint8_t *d, size_t dlen);
/** @} */


/** @name 对称加密算法接口（如 SM4-CBC）
 *  @{
 */

/**
 * @brief 获取加密算法名称
 */
const char *x509_encryption_algor_name(int oid);

/**
 * @brief 获取加密算法 OID
 */
int x509_encryption_algor_from_name(const char *name);

/**
 * @brief 解析 DER 编码的加密算法和参数（如 IV）
 */
int x509_encryption_algor_from_der(int *oid, const uint8_t **iv, size_t *ivlen, const uint8_t **in, size_t *inlen);

/**
 * @brief 将加密算法和参数编码为 DER
 */
int x509_encryption_algor_to_der(int oid, const uint8_t *iv, size_t ivlen, uint8_t **out, size_t *outlen);

/**
 * @brief 打印 DER 编码的加密算法标识
 */
int x509_encryption_algor_print(FILE *fp, int fmt, int ind, const char *label, const uint8_t *d, size_t dlen);
/** @} */


/** @name 签名算法接口（如 sm2sign-with-sm3）
 *  @{
 */

/**
 * @brief 获取签名算法名称
 */
const char *x509_signature_algor_name(int oid);

/**
 * @brief 获取签名算法 OID
 */
int x509_signature_algor_from_name(const char *name);

/**
 * @brief 从 DER 数据中解析签名算法标识符
 */
int x509_signature_algor_from_der(int *oid, const uint8_t **in, size_t *inlen);

/**
 * @brief 将签名算法标识符编码为 DER
 */
int x509_signature_algor_to_der(int oid, uint8_t **out, size_t *outlen);

/**
 * @brief 打印 DER 编码的签名算法内容
 */
int x509_signature_algor_print(FILE *fp, int fmt, int ind, const char *label, const uint8_t *d, size_t dlen);
/** @} */


/** @name 公钥加密算法接口（如 SM2 加密）
 *  @{
 */

/**
 * @brief 获取公钥加密算法名称
 */
const char *x509_public_key_encryption_algor_name(int oid);

/**
 * @brief 获取公钥加密算法 OID
 */
int x509_public_key_encryption_algor_from_name(const char *name);

/**
 * @brief 解析 DER 编码的公钥加密算法及其参数
 */
int x509_public_key_encryption_algor_from_der(int *oid, const uint8_t **params, size_t *params_len, const uint8_t **in, size_t *inlen);

/**
 * @brief 将公钥加密算法编码为 DER 格式
 */
int x509_public_key_encryption_algor_to_der(int oid, uint8_t **out, size_t *outlen);

/**
 * @brief 打印公钥加密算法 DER 内容
 */
int x509_public_key_encryption_algor_print(FILE *fp, int fmt, int ind, const char *label, const uint8_t *d, size_t dlen);
/** @} */


/** @name 公钥算法标识符（如 SM2 曲线）
 *  @{
 */

/**
 * @brief 获取公钥算法名称
 */
const char *x509_public_key_algor_name(int oid);

/**
 * @brief 获取公钥算法 OID
 */
int x509_public_key_algor_from_name(const char *name);

/**
 * @brief 将公钥算法及曲线参数编码为 DER 格式
 * @param oid 算法 OID
 * @param curve 曲线标识符（如 GMSSL_EC_PARAM_DEFINED）
 */
int x509_public_key_algor_to_der(int oid, int curve, uint8_t **out, size_t *outlen);

/**
 * @brief 从 DER 数据中解析公钥算法及曲线参数
 */
int x509_public_key_algor_from_der(int *oid, int *curve_or_null, const uint8_t **in, size_t *inlen);

/**
 * @brief 打印 DER 编码的公钥算法内容
 */
int x509_public_key_algor_print(FILE *fp, int fmt, int ind, const char *label, const uint8_t *d, size_t dlen);
/** @} */


#ifdef __cplusplus
}
#endif
#endif
