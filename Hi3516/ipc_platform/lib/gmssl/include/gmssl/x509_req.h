/**​
 * @file x509_req.h
 * @brief X.509 证书请求处理模块
 * @copyright Copyright 2014-2022 The GmSSL Project. All Rights Reserved.
 * @license Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef GMSSL_X509_REQ_H
#define GMSSL_X509_REQ_H

#include <time.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <gmssl/sm2.h>
#include <gmssl/oid.h>
#include <gmssl/asn1.h>
#include <gmssl/x509.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 将证书请求信息编码为DER格式
 *
 * 根据RFC 2986标准，将证书请求信息转换为DER编码格式
 *
 * @param[in] version 版本号
 * @param[in] subject 主题名称
 * @param[in] subject_len 主题名称长度
 * @param[in] subject_public_key 主题公钥
 * @param[in] attrs 属性数据
 * @param[in] attrs_len 属性数据长度
 * @param[out] out 输出的DER编码数据
 * @param[out] outlen 输出的DER编码数据长度
 * @return 成功返回1，失败返回0
 */
int x509_request_info_to_der(int version, const uint8_t *subject, size_t subject_len,
    const SM2_KEY *subject_public_key, const uint8_t *attrs, size_t attrs_len,
    uint8_t **out, size_t *outlen);

/**
 * @brief 从DER格式解码证书请求信息
 *
 * 从DER编码数据中解析出证书请求信息
 *
 * @param[out] version 解析出的版本号
 * @param[out] subject 解析出的主题名称
 * @param[out] subject_len 主题名称长度
 * @param[out] subject_public_key 解析出的主题公钥
 * @param[out] attrs 解析出的属性数据
 * @param[out] attrs_len 属性数据长度
 * @param[in,out] in 输入的DER编码数据
 * @param[in,out] inlen 输入的DER编码数据长度
 * @return 成功返回1，失败返回0
 */
int x509_request_info_from_der(int *version, const uint8_t **subject, size_t *subject_len,
    SM2_KEY *subject_public_key, const uint8_t **attrs, size_t *attrs_len,
    const uint8_t **in, size_t *inlen);

/**
 * @brief 打印证书请求信息
 *
 * 以可读格式打印证书请求信息
 *
 * @param[in] fp 输出文件指针
 * @param[in] fmt 格式控制
 * @param[in] ind 缩进控制
 * @param[in] label 标签文本
 * @param[in] d 证书请求信息数据
 * @param[in] dlen 数据长度
 * @return 成功返回1，失败返回0
 */
int x509_request_info_print(FILE *fp, int fmt, int ind, const char *label, const uint8_t *d, size_t dlen);

/**
 * @brief 生成并签名证书请求的DER编码
 *
 * 创建证书请求并使用私钥进行签名，输出DER编码格式
 *
 * @param[in] version 版本号
 * @param[in] subject 主题名称
 * @param[in] subject_len 主题名称长度
 * @param[in] subject_public_key 主题公钥
 * @param[in] attrs 属性数据
 * @param[in] attrs_len 属性数据长度
 * @param[in] signature_algor 签名算法标识
 * @param[in] sign_key 签名私钥
 * @param[in] signer_id 签名者标识
 * @param[in] signer_id_len 签名者标识长度
 * @param[out] out 输出的DER编码数据
 * @param[out] outlen 输出的DER编码数据长度
 * @return 成功返回1，失败返回0
 */
int x509_req_sign_to_der(
    int version,
    const uint8_t *subject, size_t subject_len,
    const SM2_KEY *subject_public_key,
    const uint8_t *attrs, size_t attrs_len,
    int signature_algor,
    const SM2_KEY *sign_key, const char *signer_id, size_t signer_id_len,
    uint8_t **out, size_t *outlen);

/**
 * @brief 验证证书请求的签名
 *
 * 验证证书请求中的签名是否有效
 *
 * @param[in] req 证书请求数据
 * @param[in] reqlen 证书请求数据长度
 * @param[in] signer_id 签名者标识
 * @param[in] signer_id_len 签名者标识长度
 * @return 签名有效返回1，无效返回0
 */
int x509_req_verify(const uint8_t *req, size_t reqlen,
    const char *signer_id, size_t signer_id_len);

/**
 * @brief 获取证书请求的详细信息
 *
 * 从证书请求中提取详细信息
 *
 * @param[in] req 证书请求数据
 * @param[in] reqlen 证书请求数据长度
 * @param[out] version 版本号
 * @param[out] subject 主题名称
 * @param[out] subject_len 主题名称长度
 * @param[out] subject_public_key 主题公钥
 * @param[out] attributes 属性数据
 * @param[out] attributes_len 属性数据长度
 * @param[out] signature_algor 签名算法标识
 * @param[out] signature 签名数据
 * @param[out] signature_len 签名数据长度
 * @return 成功返回1，失败返回0
 */
int x509_req_get_details(const uint8_t *req, size_t reqlen,
    int *version,
    const uint8_t **subject, size_t *subject_len,
    SM2_KEY *subject_public_key,
    const uint8_t **attributes, size_t *attributes_len,
    int *signature_algor,
    const uint8_t **signature, size_t *signature_len);

/**
 * @brief 将证书请求转换为DER格式
 *
 * 将内存中的证书请求转换为DER编码格式
 *
 * @param[in] a 证书请求数据
 * @param[in] alen 证书请求数据长度
 * @param[out] out 输出的DER编码数据
 * @param[out] outlen 输出的DER编码数据长度
 * @return 成功返回1，失败返回0
 */
int x509_req_to_der(const uint8_t *a, size_t *alen, uint8_t **out, size_t *outlen);

/**
 * @brief 从DER格式解析证书请求
 *
 * 从DER编码数据中解析证书请求
 *
 * @param[out] a 解析出的证书请求数据
 * @param[out] alen 证书请求数据长度
 * @param[in,out] in 输入的DER编码数据
 * @param[in,out] inlen 输入的DER编码数据长度
 * @return 成功返回1，失败返回0
 */
int x509_req_from_der(const uint8_t **a, size_t *alen, const uint8_t **in, size_t *inlen);

/**
 * @brief 将证书请求输出为PEM格式
 *
 * 将证书请求以PEM格式写入文件
 *
 * @param[in] req 证书请求数据
 * @param[in] reqlen 证书请求数据长度
 * @param[in] fp 输出文件指针
 * @return 成功返回1，失败返回0
 */
int x509_req_to_pem(const uint8_t *req, size_t reqlen, FILE *fp);

/**
 * @brief 从PEM文件读取证书请求
 *
 * 从PEM格式文件中读取证书请求
 *
 * @param[out] req 输出的证书请求数据
 * @param[out] reqlen 证书请求数据长度
 * @param[in] maxlen 最大可接受长度
 * @param[in] fp 输入文件指针
 * @return 成功返回1，失败返回0
 */
int x509_req_from_pem(uint8_t *req, size_t *reqlen, size_t maxlen, FILE *fp);

/**
 * @brief 打印证书请求信息
 *
 * 以可读格式打印证书请求的全部信息
 *
 * @param[in] fp 输出文件指针
 * @param[in] fmt 格式控制
 * @param[in] ind 缩进控制
 * @param[in] label 标签文本
 * @param[in] req 证书请求数据
 * @param[in] reqlen 证书请求数据长度
 * @return 成功返回1，失败返回0
 */
int x509_req_print(FILE *fp, int fmt, int ind, const char *label, const uint8_t *req, size_t reqlen);

/**
 * @brief 从PEM文件创建新的证书请求
 *
 * 从PEM格式文件中读取并创建新的证书请求
 *
 * @param[out] req 输出的证书请求数据指针
 * @param[out] reqlen 证书请求数据长度
 * @param[in] fp 输入文件指针
 * @return 成功返回1，失败返回0
 */
int x509_req_new_from_pem(uint8_t **req, size_t *reqlen, FILE *fp);

/**
 * @brief 从文件创建新的证书请求
 *
 * 从指定路径的文件中读取并创建新的证书请求
 *
 * @param[out] req 输出的证书请求数据指针
 * @param[out] reqlen 证书请求数据长度
 * @param[in] file 文件路径
 * @return 成功返回1，失败返回0
 */
int x509_req_new_from_file(uint8_t **req, size_t *reqlen, const char *file);

#ifdef __cplusplus
}
#endif
#endif