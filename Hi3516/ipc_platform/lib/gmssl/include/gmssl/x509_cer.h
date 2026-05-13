/*
*  Copyright 2014-2023 The GmSSL Project. All Rights Reserved.
*
*  Licensed under the Apache License, Version 2.0 (the License); you may
*  not use this file except in compliance with the License.
*
*  http://www.apache.org/licenses/LICENSE-2.0
*/

#ifndef GMSSL_X509_CER_H
#define GMSSL_X509_CER_H

#include <time.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <gmssl/sm2.h>
#include <gmssl/oid.h>
#include <gmssl/asn1.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief X.509证书版本枚举
 */
enum X509_Version
{
	X509_version_v1 = 0, /* X.509 v1版本 */
	X509_version_v2 = 1, /* X.509 v2版本 */
	X509_version_v3 = 2, /* X.509 v3版本 */
};

/**
 * @brief 获取X.509版本名称
 * @param version 版本号
 * @return 版本名称字符串
 */
const char *x509_version_name(int version);

/**
 * @brief 将显式版本号编码为DER格式
 * @param index 索引号
 * @param version 版本号
 * @param[out] out 输出缓冲区
 * @param[out] outlen 输出长度
 * @return 成功返回1，失败返回0
 */
int x509_explicit_version_to_der(int index, int version, uint8_t **out, size_t *outlen);

/**
 * @brief 从DER格式解码显式版本号
 * @param index 索引号
 * @param[out] version 解析出的版本号
 * @param[in,out] in 输入缓冲区指针
 * @param[in,out] inlen 输入长度指针
 * @return 成功返回1，失败返回0
 */
int x509_explicit_version_from_der(int index, int *version, const uint8_t **in, size_t *inlen);

/**
 * @brief 时间类型定义
 * Time ::= CHOICE {
 *     utcTime        UTCTime,
 *     generalTime    GeneralizedTime }
 */
#define X509_MAX_UTC_TIME 2524607999		   // "20491231235959Z"
#define X509_MAX_GENERALIZED_TIME 253402300799 // "99991231235959Z"

/**
 * @brief 将时间编码为DER格式
 * @param a 时间值
 * @param[out] out 输出缓冲区
 * @param[out] outlen 输出长度
 * @return 成功返回1，失败返回0
 */
int x509_time_to_der(time_t a, uint8_t **out, size_t *outlen);

/**
 * @brief 从DER格式解码时间
 * @param[out] a 解析出的时间值
 * @param[in,out] in 输入缓冲区指针
 * @param[in,out] inlen 输入长度指针
 * @return 成功返回1，失败返回0
 */
int x509_time_from_der(time_t *a, const uint8_t **in, size_t *inlen);

/**
 * @brief 有效期定义
 * Validity ::= SEQUENCE {
 *     notBefore    Time,
 *     notAfter     Time }
 */
#define X509_VALIDITY_MIN_DAYS 1
#define X509_VALIDITY_MAX_DAYS 3653
#define X509_VALIDITY_MAX_SECONDS (X509_VALIDITY_MAX_DAYS * 86400)

/**
 * @brief 根据起始时间和天数计算结束时间
 * @param[out] not_after 计算出的结束时间
 * @param not_before 起始时间
 * @param days 天数
 * @return 成功返回1，失败返回0
 */
int x509_validity_add_days(time_t *not_after, time_t not_before, int days);

/**
 * @brief 将有效期编码为DER格式
 * @param not_before 起始时间
 * @param not_after 结束时间
 * @param[out] out 输出缓冲区
 * @param[out] outlen 输出长度
 * @return 成功返回1，失败返回0
 */
int x509_validity_to_der(time_t not_before, time_t not_after, uint8_t **out, size_t *outlen);

/**
 * @brief 从DER格式解码有效期
 * @param[out] not_before 解析出的起始时间
 * @param[out] not_after 解析出的结束时间
 * @param[in,out] in 输入缓冲区指针
 * @param[in,out] inlen 输入长度指针
 * @return 成功返回1，失败返回0
 */
int x509_validity_from_der(time_t *not_before, time_t *not_after, const uint8_t **in, size_t *inlen);

/**
 * @brief 检查有效期是否合法
 * @param not_before 起始时间
 * @param not_after 结束时间
 * @param now 当前时间
 * @param max_secs 最大允许秒数
 * @return 合法返回1，否则返回0
 */
int x509_validity_check(time_t not_before, time_t not_after, time_t now, int max_secs);

/**
 * @brief 打印有效期信息
 * @param fp 文件指针
 * @param fmt 格式
 * @param ind 缩进
 * @param label 标签
 * @param d 数据缓冲区
 * @param dlen 数据长度
 * @return 成功返回1，失败返回0
 */
int x509_validity_print(FILE *fp, int fmt, int ind, const char *label, const uint8_t *d, size_t dlen);

/**
 * @brief 目录字符串或目录名称定义
 * DirectoryName ::= CHOICE {
 *     teletexString        TeletexString    (SIZE (1..MAX)),
 *     printableString    PrintableString    (SIZE (1..MAX)),
 *     universalString    UniversalString    (SIZE (1..MAX)),
 *     utf8String        UTF8String    (SIZE (1..MAX)),
 *     bmpString        BMPString    (SIZE (1..MAX)),
 * }
 */

/**
 * @brief 检查目录名称是否合法
 * @param tag ASN.1标签
 * @param d 数据缓冲区
 * @param dlen 数据长度
 * @return 合法返回1，否则返回0
 */
int x509_directory_name_check(int tag, const uint8_t *d, size_t dlen);

/**
 * @brief 带范围检查的目录名称验证
 * @param tag ASN.1标签
 * @param d 数据缓冲区
 * @param dlen 数据长度
 * @param minlen 最小长度
 * @param maxlen 最大长度
 * @return 合法返回1，否则返回0
 */
int x509_directory_name_check_ex(int tag, const uint8_t *d, size_t dlen, size_t minlen, size_t maxlen);

/**
 * @brief 将目录名称编码为DER格式
 * @param tag ASN.1标签
 * @param d 数据缓冲区
 * @param dlen 数据长度
 * @param[out] out 输出缓冲区
 * @param[out] outlen 输出长度
 * @return 成功返回1，失败返回0
 */
int x509_directory_name_to_der(int tag, const uint8_t *d, size_t dlen, uint8_t **out, size_t *outlen);

/**
 * @brief 从DER格式解码目录名称
 * @param[out] tag 解析出的ASN.1标签
 * @param[out] d 解析出的数据缓冲区
 * @param[out] dlen 解析出的数据长度
 * @param[in,out] in 输入缓冲区指针
 * @param[in,out] inlen 输入长度指针
 * @return 成功返回1，失败返回0
 */
int x509_directory_name_from_der(int *tag, const uint8_t **d, size_t *dlen, const uint8_t **in, size_t *inlen);

/**
 * @brief 将显式目录名称编码为DER格式
 * @param index 索引号
 * @param tag ASN.1标签
 * @param d 数据缓冲区
 * @param dlen 数据长度
 * @param[out] out 输出缓冲区
 * @param[out] outlen 输出长度
 * @return 成功返回1，失败返回0
 */
int x509_explicit_directory_name_to_der(int index, int tag, const uint8_t *d, size_t dlen, uint8_t **out, size_t *outlen);

/**
 * @brief 从DER格式解码显式目录名称
 * @param index 索引号
 * @param[out] tag 解析出的ASN.1标签
 * @param[out] d 解析出的数据缓冲区
 * @param[out] dlen 解析出的数据长度
 * @param[in,out] in 输入缓冲区指针
 * @param[in,out] inlen 输入长度指针
 * @return 成功返回1，失败返回0
 */
int x509_explicit_directory_name_from_der(int index, int *tag, const uint8_t **d, size_t *dlen, const uint8_t **in, size_t *inlen);

/**
 * @brief 打印目录名称
 * @param fp 文件指针
 * @param fmt 格式
 * @param ind 缩进
 * @param label 标签
 * @param tag ASN.1标签
 * @param d 数据缓冲区
 * @param dlen 数据长度
 * @return 成功返回1，失败返回0
 */
int x509_directory_name_print(FILE *fp, int fmt, int ind, const char *label, int tag, const uint8_t *d, size_t dlen);

/**
 * @brief 属性类型和值定义
 * AttributeTypeAndValue ::= SEQUENCE {
 *     type OBJECT IDENTIFIER,
 *     value ANY -- DEFINED BY AttributeType }
 *
 * id-at
 *     OID_at_name            name            DirectoryName        1..ub-name
 *     OID_at_surname            surname            DirectoryName        1..ub-name
 *     OID_at_given_name        givenName        DirectoryName        1..ub-name
 *     OID_at_initials            initials        DirectoryName        1..ub-name
 *     OID_at_generation_qualifier    generationQualifier    DirectoryName        1..ub-name
 *     OID_at_common_name        commonName        DirectoryName        1..ub-common-name
 *     OID_at_locality_name        localityName        DirectoryName        1..ub-locality-name
 *     OID_at_state_or_province_name    stateOrProvinceName    DirectoryName        1..ub-state-name
 *     OID_at_organization_name    organizationName    DirectoryName        1..ub-organization-name
 *     OID_at_organizational_unit_name    organizationalUnitName    DirectoryName        1..ub-organizational-unit-name
 *     OID_at_title            title            DirectoryName        1..ub-title
 *     OID_at_dn_qualifier        dnQualifier        PrintableString    N/A
 *     OID_at_country_name        countryName        PrintableString        2..2
 *     OID_at_serial_number        serialNumber        PrintableString        1..ub-serial-number
 *     OID_at_pseudonym        pseudonym        DirectoryName        1..ub-pseudonym
 *     OID_domain_component        domainComponent        IA5String        N/A
 */

/**
 * @brief 获取属性类型名称
 * @param oid 属性类型OID
 * @return 属性类型名称字符串
 */
const char *x509_name_type_name(int oid);

/**
 * @brief 根据名称获取属性类型OID
 * @param name 属性类型名称
 * @return 属性类型OID
 */
int x509_name_type_from_name(const char *name);

/**
 * @brief 从DER格式解码属性类型
 * @param[out] oid 解析出的属性类型OID
 * @param[in,out] in 输入缓冲区指针
 * @param[in,out] inlen 输入长度指针
 * @return 成功返回1，失败返回0
 */
int x509_name_type_from_der(int *oid, const uint8_t **in, size_t *inlen);

/**
 * @brief 将属性类型编码为DER格式
 * @param oid 属性类型OID
 * @param[out] out 输出缓冲区
 * @param[out] outlen 输出长度
 * @return 成功返回1，失败返回0
 */
int x509_name_type_to_der(int oid, uint8_t **out, size_t *outlen);

#define X509_ub_name 32768
#define X509_ub_common_name 64
#define X509_ub_locality_name 128
#define X509_ub_state_name 128
#define X509_ub_organization_name 64
#define X509_ub_organizational_unit_name 64
#define X509_ub_title 64
#define X509_ub_serial_number 64
#define X509_ub_pseudonym 128

/**
 * @brief 检查属性类型和值是否合法
 * @param oid 属性类型OID
 * @param tag ASN.1标签
 * @param val 值数据缓冲区
 * @param vlen 值数据长度
 * @return 合法返回1，否则返回0
 */
int x509_attr_type_and_value_check(int oid, int tag, const uint8_t *val, size_t vlen);

/**
 * @brief 将属性类型和值编码为DER格式
 * @param oid 属性类型OID
 * @param tag ASN.1标签
 * @param val 值数据缓冲区
 * @param vlen 值数据长度
 * @param[out] out 输出缓冲区
 * @param[out] outlen 输出长度
 * @return 成功返回1，失败返回0
 */
int x509_attr_type_and_value_to_der(int oid, int tag, const uint8_t *val, size_t vlen, uint8_t **out, size_t *outlen);

/**
 * @brief 从DER格式解码属性类型和值
 * @param[out] oid 解析出的属性类型OID
 * @param[out] tag 解析出的ASN.1标签
 * @param[out] val 解析出的值数据缓冲区
 * @param[out] vlen 解析出的值数据长度
 * @param[in,out] in 输入缓冲区指针
 * @param[in,out] inlen 输入长度指针
 * @return 成功返回1，失败返回0
 */
int x509_attr_type_and_value_from_der(int *oid, int *tag, const uint8_t **val, size_t *vlen, const uint8_t **in, size_t *inlen);

/**
 * @brief 打印属性类型和值
 * @param fp 文件指针
 * @param fmt 格式
 * @param ind 缩进
 * @param label 标签
 * @param d 数据缓冲区
 * @param dlen 数据长度
 * @return 成功返回1，失败返回0
 */
int x509_attr_type_and_value_print(FILE *fp, int fmt, int ind, const char *label, const uint8_t *d, size_t dlen);

/**
 * @brief 将相对可区分名称(RDN)编码为DER格式
 * @param oid 属性类型OID
 * @param tag 属性值的ASN.1标签
 * @param val 属性值数据
 * @param vlen 属性值长度
 * @param more 附加的RDN数据
 * @param mlen 附加RDN数据长度
 * @param out 输出DER编码缓冲区
 * @param outlen 输出DER编码长度
 * @return 成功返回1，失败返回0
 */
int x509_rdn_to_der(int oid, int tag, const uint8_t *val, size_t vlen,
					const uint8_t *more, size_t mlen, uint8_t **out, size_t *outlen);

/**
 * @brief 从DER格式解码相对可区分名称(RDN)
 * @param oid 返回属性类型OID
 * @param tag 返回属性值的ASN.1标签
 * @param val 返回属性值数据
 * @param vlen 返回属性值长度
 * @param more 返回附加的RDN数据
 * @param mlen 返回附加RDN数据长度
 * @param in 输入DER编码数据
 * @param inlen 输入数据长度
 * @return 成功返回1，失败返回0
 */
int x509_rdn_from_der(int *oid, int *tag, const uint8_t **val, size_t *vlen,
						const uint8_t **more, size_t *mlen, const uint8_t **in, size_t *inlen);

/**
 * @brief 检查RDN数据的有效性
 * @param d RDN数据
 * @param dlen RDN数据长度
 * @return 有效返回1，无效返回0
 */
int x509_rdn_check(const uint8_t *d, size_t dlen);

/**
 * @brief 打印RDN信息
 * @param fp 输出文件指针
 * @param fmt 格式控制
 * @param ind 缩进级别
 * @param label 显示标签
 * @param d RDN数据
 * @param dlen RDN数据长度
 * @return 成功返回1，失败返回0
 */
int x509_rdn_print(FILE *fp, int fmt, int ind, const char *label,
					const uint8_t *d, size_t dlen);

/**
 * @brief 向名称中添加一个RDN条目
 * @param d 名称数据缓冲区
 * @param dlen 名称数据长度(输入输出参数)
 * @param maxlen 缓冲区最大容量
 * @param oid 属性类型OID
 * @param tag 属性值的ASN.1标签
 * @param val 属性值数据
 * @param vlen 属性值长度
 * @param more 附加RDN数据
 * @param mlen 附加RDN数据长度
 * @return 成功返回1，失败返回0
 */
int x509_name_add_rdn(uint8_t *d, size_t *dlen, size_t maxlen, int oid, int tag,
						const uint8_t *val, size_t vlen, const uint8_t *more, size_t mlen);

/**
 * @brief 向名称中添加国家名称属性
 * @param d 名称数据缓冲区
 * @param dlen 名称数据长度(输入输出参数)
 * @param maxlen 缓冲区最大容量
 * @param val 国家代码(2字节PrintableString)
 * @return 成功返回1，失败返回0
 */
int x509_name_add_country_name(uint8_t *d, size_t *dlen, size_t maxlen, const char val[2]);

/**
 * @brief 向名称中添加州/省名称属性
 * @param d 名称数据缓冲区
 * @param dlen 名称数据长度(输入输出参数)
 * @param maxlen 缓冲区最大容量
 * @param tag 名称值的ASN.1标签
 * @param val 州/省名称值
 * @param vlen 名称值长度
 * @return 成功返回1，失败返回0
 */
int x509_name_add_state_or_province_name(uint8_t *d, size_t *dlen, size_t maxlen,
											int tag, const uint8_t *val, size_t vlen);

/**
 * @brief 向名称中添加地区名称属性
 * @param d 名称数据缓冲区
 * @param dlen 名称数据长度(输入输出参数)
 * @param maxlen 缓冲区最大容量
 * @param tag 地区名称值的ASN.1标签
 * @param val 地区名称值
 * @param vlen 名称值长度
 * @return 成功返回1，失败返回0
 */
int x509_name_add_locality_name(uint8_t *d, size_t *dlen, size_t maxlen,
								int tag, const uint8_t *val, size_t vlen);

/**
 * @brief 向名称中添加组织名称属性
 * @param d 名称数据缓冲区
 * @param dlen 名称数据长度(输入输出参数)
 * @param maxlen 缓冲区最大容量
 * @param tag 组织名称值的ASN.1标签
 * @param val 组织名称值
 * @param vlen 名称值长度
 * @return 成功返回1，失败返回0
 */
int x509_name_add_organization_name(uint8_t *d, size_t *dlen, size_t maxlen,
									int tag, const uint8_t *val, size_t vlen);

/**
 * @brief 向名称中添加组织单位名称属性
 * @param d 名称数据缓冲区
 * @param dlen 名称数据长度(输入输出参数)
 * @param maxlen 缓冲区最大容量
 * @param tag 组织单位名称值的ASN.1标签
 * @param val 组织单位名称值
 * @param vlen 名称值长度
 * @return 成功返回1，失败返回0
 */
int x509_name_add_organizational_unit_name(uint8_t *d, size_t *dlen, size_t maxlen,
											int tag, const uint8_t *val, size_t vlen);

/**
 * @brief 向名称中添加通用名称属性
 * @param d 名称数据缓冲区
 * @param dlen 名称数据长度(输入输出参数)
 * @param maxlen 缓冲区最大容量
 * @param tag 通用名称值的ASN.1标签
 * @param val 通用名称值
 * @param vlen 名称值长度
 * @return 成功返回1，失败返回0
 */
int x509_name_add_common_name(uint8_t *d, size_t *dlen, size_t maxlen,
								int tag, const uint8_t *val, size_t vlen);

/**
 * @brief 向名称中添加域名组件属性
 * @param d 名称数据缓冲区
 * @param dlen 名称数据长度(输入输出参数)
 * @param maxlen 缓冲区最大容量
 * @param val 域名组件值(IA5String)
 * @param vlen 值长度
 * @return 成功返回1，失败返回0
 */
int x509_name_add_domain_component(uint8_t *d, size_t *dlen, size_t maxlen,
									const char *val, size_t vlen);

/**
 * @brief 设置完整的名称信息
 * @param d 名称数据缓冲区
 * @param dlen 名称数据长度(输入输出参数)
 * @param maxlen 缓冲区最大容量
 * @param country 国家代码(2字节)
 * @param state 州/省名称
 * @param locality 地区名称
 * @param org 组织名称
 * @param org_unit 组织单位名称
 * @param common_name 通用名称
 * @return 成功返回1，失败返回0
 */
int x509_name_set(uint8_t *d, size_t *dlen, size_t maxlen,
					const char country[2], const char *state, const char *locality,
					const char *org, const char *org_unit, const char *common_name);

/**
 * @brief 将名称编码为DER格式
 */
#define x509_name_to_der(d, dlen, out, outlen) asn1_sequence_to_der(d, dlen, out, outlen)

/**
 * @brief 从DER格式解码名称
 */
#define x509_name_from_der(d, dlen, in, inlen) asn1_sequence_from_der(d, dlen, in, inlen)

/**
 * @brief 检查名称数据的有效性
 * @param d 名称数据
 * @param dlen 名称数据长度
 * @return 有效返回1，无效返回0
 */
int x509_name_check(const uint8_t *d, size_t dlen);

/**
 * @brief 打印名称信息
 * @param fp 输出文件指针
 * @param fmt 格式控制
 * @param ind 缩进级别
 * @param label 显示标签
 * @param d 名称数据
 * @param dlen 名称数据长度
 * @return 成功返回1，失败返回0
 */
int x509_name_print(FILE *fp, int fmt, int ind, const char *label,
					const uint8_t *d, size_t dlen);

/**
 * @brief 根据类型获取名称中的属性值
 * @param d 名称数据
 * @param dlen 名称数据长度
 * @param oid 要查找的属性类型OID
 * @param tag 返回属性值的ASN.1标签
 * @param val 返回属性值数据
 * @param vlen 返回属性值长度
 * @return 成功返回1，失败返回0
 */
int x509_name_get_value_by_type(const uint8_t *d, size_t dlen, int oid,
								int *tag, const uint8_t **val, size_t *vlen);

/**
 * @brief 获取名称中的通用名称(CN)属性
 * @param d 名称数据
 * @param dlen 名称数据长度
 * @param tag 返回通用名称的ASN.1标签
 * @param val 返回通用名称值
 * @param vlen 返回通用名称长度
 * @return 成功返回1，失败返回0
 */
int x509_name_get_common_name(const uint8_t *d, size_t dlen,
								int *tag, const uint8_t **val, size_t *vlen);

/**
 * @brief 比较两个名称是否相等
 * @param a 第一个名称数据
 * @param alen 第一个名称长度
 * @param b 第二个名称数据
 * @param blen 第二个名称长度
 * @return 相等返回1，不等返回0
 */
int x509_name_equ(const uint8_t *a, size_t alen, const uint8_t *b, size_t blen);

/**
 * @brief 打印多个名称信息
 * @param fp 输出文件指针
 * @param fmt 格式控制
 * @param ind 缩进级别
 * @param label 显示标签
 * @param d 名称数据数组
 * @param dlen 名称数据总长度
 * @return 成功返回1，失败返回0
 */
int x509_names_print(FILE *fp, int fmt, int ind, const char *label,
						const uint8_t *d, size_t dlen);

/**
 * @brief 将公钥信息编码为DER格式
 */
#define x509_public_key_info_to_der(key, out, outlen) sm2_public_key_info_to_der(key, out, outlen)

/**
 * @brief 从DER格式解码公钥信息
 */
#define x509_public_key_info_from_der(key, in, inlen) sm2_public_key_info_from_der(key, in, inlen)

/**
 * @brief 打印公钥信息
 * @param fp 输出文件指针
 * @param fmt 格式控制
 * @param ind 缩进级别
 * @param label 显示标签
 * @param d 公钥信息数据
 * @param dlen 数据长度
 * @return 成功返回1，失败返回0
 */
int x509_public_key_info_print(FILE *fp, int fmt, int ind, const char *label,
								const uint8_t *d, size_t dlen);

/**
 * @brief 获取扩展项的名称
 * @param oid 扩展项OID
 * @return 扩展项名称字符串
 */
const char *x509_ext_id_name(int oid);

/**
 * @brief 根据名称获取扩展项OID
 * @param name 扩展项名称
 * @return 扩展项OID
 */
int x509_ext_id_from_name(const char *name);

/**
 * @brief 从DER格式解码扩展项OID
 * @param oid 返回的扩展项OID
 * @param nodes 返回的OID节点数组
 * @param nodes_count 返回的OID节点数
 * @param in 输入DER数据
 * @param inlen 输入数据长度
 * @return 成功返回1，失败返回0
 */
int x509_ext_id_from_der(int *oid, uint32_t *nodes, size_t *nodes_count,
							const uint8_t **in, size_t *inlen);

/**
 * @brief 将扩展项OID编码为DER格式
 * @param oid 扩展项OID
 * @param out 输出DER缓冲区
 * @param outlen 输出DER长度
 * @return 成功返回1，失败返回0
 */
int x509_ext_id_to_der(int oid, uint8_t **out, size_t *outlen);

/**
 * @brief 将扩展项编码为DER格式
 * @param oid 扩展项OID
 * @param critical 是否关键扩展
 * @param val 扩展值数据
 * @param vlen 扩展值长度
 * @param out 输出DER缓冲区
 * @param outlen 输出DER长度
 * @return 成功返回1，失败返回0
 */
int x509_ext_to_der(int oid, int critical, const uint8_t *val, size_t vlen,
					uint8_t **out, size_t *outlen);

/**
 * @brief 从DER格式解码扩展项
 * @param oid 返回的扩展项OID
 * @param nodes 返回的OID节点数组
 * @param nodes_cnt 返回的OID节点数
 * @param critical 返回是否关键扩展
 * @param val 返回扩展值数据
 * @param vlen 返回扩展值长度
 * @param in 输入DER数据
 * @param inlen 输入数据长度
 * @return 成功返回1，失败返回0
 */
int x509_ext_from_der(int *oid, uint32_t *nodes, size_t *nodes_cnt, int *critical,
						const uint8_t **val, size_t *vlen, const uint8_t **in, size_t *inlen);

/**
 * @brief 打印扩展项信息
 * @param fp 输出文件指针
 * @param fmt 格式控制
 * @param ind 缩进级别
 * @param label 显示标签
 * @param d 扩展项数据
 * @param dlen 数据长度
 * @return 成功返回1，失败返回0
 */
int x509_ext_print(FILE *fp, int fmt, int ind, const char *label,
					const uint8_t *d, size_t dlen);

/**
 * @brief 将扩展项列表编码为DER格式(带显式标签)
 * @param index 标签索引
 * @param d 扩展项列表数据
 * @param dlen 数据长度
 * @param out 输出DER缓冲区
 * @param outlen 输出DER长度
 * @return 成功返回1，失败返回0
 */
int x509_explicit_exts_to_der(int index, const uint8_t *d, size_t dlen,
								uint8_t **out, size_t *outlen);

/**
 * @brief 从DER格式解码扩展项列表(带显式标签)
 * @param index 标签索引
 * @param d 返回的扩展项列表数据
 * @param dlen 返回的数据长度
 * @param in 输入DER数据
 * @param inlen 输入数据长度
 * @return 成功返回1，失败返回0
 */
int x509_explicit_exts_from_der(int index, const uint8_t **d, size_t *dlen,
								const uint8_t **in, size_t *inlen);

/**
 * @brief 将扩展项列表编码为DER格式(使用标准X.509标签3)
 */
#define x509_exts_to_der(d, dlen, out, outlen) x509_explicit_exts_to_der(3, d, dlen, out, outlen)

/**
 * @brief 从DER格式解码扩展项列表(使用标准X.509标签3)
 */
#define x509_exts_from_der(d, dlen, in, inlen) x509_explicit_exts_from_der(3, d, dlen, in, inlen)

/**
 * @brief 根据OID从扩展项列表中查找特定扩展项
 * @param d 扩展项列表数据
 * @param dlen 数据长度
 * @param oid 要查找的扩展项OID
 * @param critical 返回是否关键扩展
 * @param val 返回扩展值数据
 * @param vlen 返回扩展值长度
 * @return 找到返回1，未找到返回0
 */
int x509_exts_get_ext_by_oid(const uint8_t *d, size_t dlen, int oid,
								int *critical, const uint8_t **val, size_t *vlen);

/**
 * @brief 打印扩展项列表信息
 * @param fp 输出文件指针
 * @param fmt 格式控制
 * @param ind 缩进级别
 * @param label 显示标签
 * @param d 扩展项列表数据
 * @param dlen 数据长度
 * @return 成功返回1，失败返回0
 */
int x509_exts_print(FILE *fp, int fmt, int ind, const char *label,
					const uint8_t *d, size_t dlen);

/* 序列号长度限制 */
#define X509_SERIAL_NUMBER_MIN_LEN 1
#define X509_SERIAL_NUMBER_MAX_LEN 20

/* 唯一标识符长度限制 */
#define X509_UNIQUE_ID_MIN_LEN 32
#define X509_UNIQUE_ID_MAX_LEN 32

/**
 * @brief 将TBS证书结构编码为DER格式
 * @param version 证书版本
 * @param serial 序列号
 * @param serial_len 序列号长度
 * @param signature_algor 签名算法OID
 * @param issuer 颁发者名称
 * @param issuer_len 颁发者名称长度
 * @param not_before 有效期开始时间
 * @param not_after 有效期结束时间
 * @param subject 主体名称
 * @param subject_len 主体名称长度
 * @param subject_public_key 主体公钥
 * @param issuer_unique_id 颁发者唯一标识
 * @param issuer_unique_id_len 颁发者唯一标识长度
 * @param subject_unique_id 主体唯一标识
 * @param subject_unique_id_len 主体唯一标识长度
 * @param exts 扩展项列表
 * @param exts_len 扩展项列表长度
 * @param out 输出DER缓冲区
 * @param outlen 输出DER长度
 * @return 成功返回1，失败返回0
 */
int x509_tbs_cert_to_der(
	int version,
	const uint8_t *serial, size_t serial_len,
	int signature_algor,
	const uint8_t *issuer, size_t issuer_len,
	time_t not_before, time_t not_after,
	const uint8_t *subject, size_t subject_len,
	const SM2_KEY *subject_public_key,
	const uint8_t *issuer_unique_id, size_t issuer_unique_id_len,
	const uint8_t *subject_unique_id, size_t subject_unique_id_len,
	const uint8_t *exts, size_t exts_len,
	uint8_t **out, size_t *outlen);

/**
 * @brief 从DER格式解码TBS证书结构
 * @param version 返回证书版本
 * @param serial 返回序列号
 * @param serial_len 返回序列号长度
 * @param signature_algor 返回签名算法OID
 * @param issuer 返回颁发者名称
 * @param issuer_len 返回颁发者名称长度
 * @param not_before 返回有效期开始时间
 * @param not_after 返回有效期结束时间
 * @param subject 返回主体名称
 * @param subject_len 返回主体名称长度
 * @param subject_public_key 返回主体公钥
 * @param issuer_unique_id 返回颁发者唯一标识
 * @param issuer_unique_id_len 返回颁发者唯一标识长度
 * @param subject_unique_id 返回主体唯一标识
 * @param subject_unique_id_len 返回主体唯一标识长度
 * @param exts 返回扩展项列表
 * @param exts_len 返回扩展项列表长度
 * @param in 输入DER数据
 * @param inlen 输入数据长度
 * @return 成功返回1，失败返回0
 */
int x509_tbs_cert_from_der(
	int *version,
	const uint8_t **serial, size_t *serial_len,
	int *signature_algor,
	const uint8_t **issuer, size_t *issuer_len,
	time_t *not_before, time_t *not_after,
	const uint8_t **subject, size_t *subject_len,
	SM2_KEY *subject_public_key,
	const uint8_t **issuer_unique_id, size_t *issuer_unique_id_len,
	const uint8_t **subject_unique_id, size_t *subject_unique_id_len,
	const uint8_t **exts, size_t *exts_len,
	const uint8_t **in, size_t *inlen);

/**
 * @brief 打印TBS证书信息
 * @param fp 输出文件指针
 * @param fmt 格式控制
 * @param ind 缩进级别
 * @param label 显示标签
 * @param d TBS证书数据
 * @param dlen 数据长度
 * @return 成功返回1，失败返回0
 */
int x509_tbs_cert_print(FILE *fp, int fmt, int ind, const char *label,
						const uint8_t *d, size_t dlen);

/**
 * @brief 将完整证书编码为DER格式
 * @param tbs TBS证书数据
 * @param tbslen TBS证书长度
 * @param signature_algor 签名算法OID
 * @param sig 签名值
 * @param siglen 签名长度
 * @param out 输出DER缓冲区
 * @param outlen 输出DER长度
 * @return 成功返回1，失败返回0
 */
int x509_certificate_to_der(
	const uint8_t *tbs, size_t tbslen,
	int signature_algor,
	const uint8_t *sig, size_t siglen,
	uint8_t **out, size_t *outlen);

/**
 * @brief 从DER格式解码完整证书
 * @param tbs 返回TBS证书数据
 * @param tbslen 返回TBS证书长度
 * @param signature_algor 返回签名算法OID
 * @param sig 返回签名值
 * @param siglen 返回签名长度
 * @param in 输入DER数据
 * @param inlen 输入数据长度
 * @return 成功返回1，失败返回0
 */
int x509_certificate_from_der(
	const uint8_t **tbs, size_t *tbslen,
	int *signature_algor,
	const uint8_t **sig, size_t *siglen,
	const uint8_t **in, size_t *inlen);

/**
 * @brief 从DER格式解码签名结构
 * @param tbs 返回TBS数据
 * @param tbslen 返回TBS长度
 * @param signature_algor 返回签名算法OID
 * @param sig 返回签名值
 * @param siglen 返回签名长度
 * @param in 输入DER数据
 * @param inlen 输入数据长度
 * @return 成功返回1，失败返回0
 */
int x509_signed_from_der(
	const uint8_t **tbs, size_t *tbslen,
	int *signature_algor,
	const uint8_t **sig, size_t *siglen,
	const uint8_t **in, size_t *inlen);

/**
 * @brief 验证签名结构的签名
 * @param a 签名结构数据
 * @param alen 数据长度
 * @param pub_key 验证公钥
 * @param signer_id 签名者ID
 * @param signer_id_len 签名者ID长度
 * @return 验证成功返回1，失败返回0
 */
int x509_signed_verify(const uint8_t *a, size_t alen, const SM2_KEY *pub_key,
						const char *signer_id, size_t signer_id_len);

/**
 * @brief 使用CA证书验证签名结构的签名
 * @param a 签名结构数据
 * @param alen 数据长度
 * @param cacert CA证书
 * @param cacertlen CA证书长度
 * @param signer_id 签名者ID
 * @param signer_id_len 签名者ID长度
 * @return 验证成功返回1，失败返回0
 */
int x509_signed_verify_by_ca_cert(const uint8_t *a, size_t alen, const uint8_t *cacert, size_t cacertlen,
									const char *signer_id, size_t signer_id_len);
/**
 * @file x509_cert.h
 * @brief X.509 证书处理模块
 * @defgroup x509 X.509 证书
 * @{
 */

/**
 * @brief 将证书信息编码为 DER 格式并签名
 *
 * @param version 证书版本号
 * @param serial 序列号指针
 * @param serial_len 序列号长度
 * @param signature_algor 签名算法标识
 * @param issuer 颁发者名称指针
 * @param issuer_len 颁发者名称长度
 * @param not_before 证书生效时间
 * @param not_after 证书过期时间
 * @param subject 主体名称指针
 * @param subject_len 主体名称长度
 * @param subject_public_key 主体公钥指针
 * @param issuer_unique_id 颁发者唯一标识指针
 * @param issuer_unique_id_len 颁发者唯一标识长度
 * @param subject_unique_id 主体唯一标识指针
 * @param subject_unique_id_len 主体唯一标识长度
 * @param exts 扩展字段指针
 * @param exts_len 扩展字段长度
 * @param sign_key 签名私钥指针
 * @param signer_id 签名者标识
 * @param signer_id_len 签名者标识长度
 * @param out 输出缓冲区指针
 * @param outlen 输出长度指针
 * @return 成功返回1，失败返回0
 */
int x509_cert_sign_to_der(
	int version,
	const uint8_t *serial, size_t serial_len,
	int signature_algor,
	const uint8_t *issuer, size_t issuer_len,
	time_t not_before, time_t not_after,
	const uint8_t *subject, size_t subject_len,
	const SM2_KEY *subject_public_key,
	const uint8_t *issuer_unique_id, size_t issuer_unique_id_len,
	const uint8_t *subject_unique_id, size_t subject_unique_id_len,
	const uint8_t *exts, size_t exts_len,
	const SM2_KEY *sign_key, const char *signer_id, size_t signer_id_len,
	uint8_t **out, size_t *outlen);

/**
 * @brief 将证书转换为 DER 格式
 *
 * @param a 证书数据指针
 * @param alen 证书数据长度
 * @param out 输出缓冲区指针
 * @param outlen 输出长度指针
 * @return 成功返回1，失败返回0
 */
int x509_cert_to_der(const uint8_t *a, size_t alen, uint8_t **out, size_t *outlen);

/**
 * @brief 从 DER 格式解析证书
 *
 * @param a 证书数据指针
 * @param alen 证书数据长度指针
 * @param in 输入数据指针
 * @param inlen 输入数据长度指针
 * @return 成功返回1，失败返回0
 */
int x509_cert_from_der(const uint8_t **a, size_t *alen, const uint8_t **in, size_t *inlen);

/**
 * @brief 将证书转换为 PEM 格式并写入文件
 *
 * @param a 证书数据指针
 * @param alen 证书数据长度
 * @param fp 文件指针
 * @return 成功返回1，失败返回0
 */
int x509_cert_to_pem(const uint8_t *a, size_t alen, FILE *fp);

/**
 * @brief 从 PEM 文件读取证书
 *
 * @param a 证书数据缓冲区指针
 * @param alen 证书数据长度指针
 * @param maxlen 最大可读取长度
 * @param fp 文件指针
 * @return 成功返回1，失败返回0
 */
int x509_cert_from_pem(uint8_t *a, size_t *alen, size_t maxlen, FILE *fp);

/**
 * @brief 根据主题名称从 PEM 文件读取证书
 *
 * @param a 证书数据缓冲区指针
 * @param alen 证书数据长度指针
 * @param maxlen 最大可读取长度
 * @param name 主题名称指针
 * @param namelen 主题名称长度
 * @param fp 文件指针
 * @return 成功返回1，失败返回0
 */
int x509_cert_from_pem_by_subject(uint8_t *a, size_t *alen, size_t maxlen, const uint8_t *name, size_t namelen, FILE *fp);

/**
 * @brief 打印证书信息
 *
 * @param fp 文件指针
 * @param fmt 格式标识
 * @param ind 缩进量
 * @param label 标签字符串
 * @param a 证书数据指针
 * @param alen 证书数据长度
 * @return 成功返回1，失败返回0
 */
int x509_cert_print(FILE *fp, int fmt, int ind, const char *label, const uint8_t *a, size_t alen);

/**
 * @brief 使用 CA 证书验证证书签名
 *
 * @param a 待验证证书数据指针
 * @param alen 待验证证书数据长度
 * @param cacert CA 证书数据指针
 * @param cacertlen CA 证书数据长度
 * @param signer_id 签名者标识
 * @param signer_id_len 签名者标识长度
 * @return 验证成功返回1，失败返回0
 */
int x509_cert_verify_by_ca_cert(const uint8_t *a, size_t alen, const uint8_t *cacert, size_t cacertlen,
								const char *signer_id, size_t signer_id_len);

/**
 * @brief 获取证书详细信息
 *
 * @param a 证书数据指针
 * @param alen 证书数据长度
 * @param version 版本号指针
 * @param serial_number 序列号指针
 * @param serial_number_len 序列号长度指针
 * @param inner_signature_algor 内部签名算法指针
 * @param issuer 颁发者名称指针
 * @param issuer_len 颁发者名称长度指针
 * @param not_before 生效时间指针
 * @param not_after 过期时间指针
 * @param subject 主体名称指针
 * @param subject_len 主体名称长度指针
 * @param subject_public_key 主体公钥指针
 * @param issuer_unique_id 颁发者唯一标识指针
 * @param issuer_unique_id_len 颁发者唯一标识长度指针
 * @param subject_unique_id 主体唯一标识指针
 * @param subject_unique_id_len 主体唯一标识长度指针
 * @param extensions 扩展字段指针
 * @param extensions_len 扩展字段长度指针
 * @param signature_algor 签名算法指针
 * @param signature 签名值指针
 * @param signature_len 签名值长度指针
 * @return 成功返回1，失败返回0
 */
int x509_cert_get_details(const uint8_t *a, size_t alen,
							int *version,
							const uint8_t **serial_number, size_t *serial_number_len,
							int *inner_signature_algor,
							const uint8_t **issuer, size_t *issuer_len,
							time_t *not_before, time_t *not_after,
							const uint8_t **subject, size_t *subject_len,
							SM2_KEY *subject_public_key,
							const uint8_t **issuer_unique_id, size_t *issuer_unique_id_len,
							const uint8_t **subject_unique_id, size_t *subject_unique_id_len,
							const uint8_t **extensions, size_t *extensions_len,
							int *signature_algor,
							const uint8_t **signature, size_t *signature_len);
/**
 * @brief X.509 证书类型枚举
 */
typedef enum
{
	X509_cert_server_auth,		   /**< 服务器认证 */
	X509_cert_client_auth,		   /**< 客户端认证 */
	X509_cert_server_key_encipher, /**< 服务器密钥加密 */
	X509_cert_client_key_encipher, /**< 客户端密钥加密 */
	X509_cert_ca,				   /**< CA 证书 */
	X509_cert_root_ca,			   /**< 根 CA 证书 */
	X509_cert_crl_sign,			   /**< CRL 签名证书 */
} X509_CERT_TYPE;

/**
 * @brief 检查证书是否符合指定类型要求
 *
 * @param cert 证书数据指针
 * @param certlen 证书数据长度
 * @param cert_type 证书类型，参见 X509_CERT_TYPE 枚举
 * @param path_len_constraint 路径长度约束指针
 * @return 符合返回1，不符合返回0
 */
int x509_cert_check(const uint8_t *cert, size_t certlen, int cert_type, int *path_len_constraint);

/**
 * @brief 获取证书的颁发者和序列号
 *
 * @param a 证书数据指针
 * @param alen 证书数据长度
 * @param issuer 颁发者名称指针
 * @param issuer_len 颁发者名称长度指针
 * @param serial_number 序列号指针
 * @param serial_number_len 序列号长度指针
 * @return 成功返回1，失败返回0
 */
int x509_cert_get_issuer_and_serial_number(const uint8_t *a, size_t alen,
											const uint8_t **issuer, size_t *issuer_len,
											const uint8_t **serial_number, size_t *serial_number_len);

/**
 * @brief 获取证书的颁发者名称
 *
 * @param a 证书数据指针
 * @param alen 证书数据长度
 * @param name 颁发者名称指针
 * @param namelen 颁发者名称长度指针
 * @return 成功返回1，失败返回0
 */
int x509_cert_get_issuer(const uint8_t *a, size_t alen, const uint8_t **name, size_t *namelen);

/**
 * @brief 获取证书的主体名称
 *
 * @param a 证书数据指针
 * @param alen 证书数据长度
 * @param subj 主体名称指针
 * @param subj_len 主体名称长度指针
 * @return 成功返回1，失败返回0
 */
int x509_cert_get_subject(const uint8_t *a, size_t alen, const uint8_t **subj, size_t *subj_len);

/**
 * @brief 获取证书的主体公钥
 *
 * @param a 证书数据指针
 * @param alen 证书数据长度
 * @param public_key SM2公钥结构指针
 * @return 成功返回1，失败返回0
 */
int x509_cert_get_subject_public_key(const uint8_t *a, size_t alen, SM2_KEY *public_key);

/**
 * @brief 获取证书的扩展字段
 *
 * @param a 证书数据指针
 * @param alen 证书数据长度
 * @param d 扩展字段数据指针
 * @param dlen 扩展字段数据长度指针
 * @return 成功返回1，失败返回0
 */
int x509_cert_get_exts(const uint8_t *a, size_t alen, const uint8_t **d, size_t *dlen);

/**
 * @brief 将多个证书转换为 PEM 格式并写入文件
 *
 * @param d 证书数据指针
 * @param dlen 证书数据长度
 * @param fp 文件指针
 * @return 成功返回1，失败返回0
 */
int x509_certs_to_pem(const uint8_t *d, size_t dlen, FILE *fp);

/**
 * @brief 从 PEM 文件读取多个证书
 *
 * @param d 证书数据缓冲区指针
 * @param dlen 证书数据长度指针
 * @param maxlen 最大可读取长度
 * @param fp 文件指针
 * @return 成功返回1，失败返回0
 */
int x509_certs_from_pem(uint8_t *d, size_t *dlen, size_t maxlen, FILE *fp);

/**
 * @brief 获取证书链中的证书数量
 *
 * @param d 证书数据指针
 * @param dlen 证书数据长度
 * @param cnt 证书数量指针
 * @return 成功返回1，失败返回0
 */
int x509_certs_get_count(const uint8_t *d, size_t dlen, size_t *cnt);

/**
 * @brief 根据索引获取证书链中的证书
 *
 * @param d 证书数据指针
 * @param dlen 证书数据长度
 * @param index 证书索引
 * @param cert 证书数据指针
 * @param certlen 证书数据长度指针
 * @return 成功返回1，失败返回0
 */
int x509_certs_get_cert_by_index(const uint8_t *d, size_t dlen, int index, const uint8_t **cert, size_t *certlen);

/**
 * @brief 根据主体名称获取证书链中的证书
 *
 * @param d 证书数据指针
 * @param dlen 证书数据长度
 * @param subject 主体名称指针
 * @param subject_len 主体名称长度
 * @param cert 证书数据指针
 * @param certlen 证书数据长度指针
 * @return 成功返回1，失败返回0
 */
int x509_certs_get_cert_by_subject(const uint8_t *d, size_t dlen, const uint8_t *subject, size_t subject_len, const uint8_t **cert, size_t *certlen);

/**
 * @brief 获取证书链中的最后一个证书
 *
 * @param d 证书数据指针
 * @param dlen 证书数据长度
 * @param cert 证书数据指针
 * @param certlen 证书数据长度指针
 * @return 成功返回1，失败返回0
 */
int x509_certs_get_last(const uint8_t *d, size_t dlen, const uint8_t **cert, size_t *certlen);

/**
 * @brief 根据主体名称和密钥标识符获取证书链中的证书
 *
 * @param d 证书数据指针
 * @param dlen 证书数据长度
 * @param subject 主体名称指针
 * @param subject_len 主体名称长度
 * @param key_id 密钥标识符指针
 * @param key_id_len 密钥标识符长度
 * @param cert 返回的证书数据指针
 * @param certlen 返回的证书数据长度指针
 * @return 成功返回1，失败返回0
 */
int x509_certs_get_cert_by_subject_and_key_identifier(const uint8_t *d, size_t dlen,
														const uint8_t *subject, size_t subject_len,
														const uint8_t *key_id, size_t key_id_len,
														const uint8_t **cert, size_t *certlen);

/**
 * @brief 根据颁发者名称和序列号获取证书链中的证书
 *
 * @param certs 证书链数据指针
 * @param certs_len 证书链数据长度
 * @param issuer 颁发者名称指针
 * @param issuer_len 颁发者名称长度
 * @param serial 序列号指针
 * @param serial_len 序列号长度
 * @param cert 返回的证书数据指针
 * @param cert_len 返回的证书数据长度指针
 * @return 成功返回1，失败返回0
 */
int x509_certs_get_cert_by_issuer_and_serial_number(
	const uint8_t *certs, size_t certs_len,
	const uint8_t *issuer, size_t issuer_len,
	const uint8_t *serial, size_t serial_len,
	const uint8_t **cert, size_t *cert_len);

/**
 * @brief 证书链类型枚举
 */
typedef enum
{
	X509_cert_chain_server, /**< 服务器证书链 */
	X509_cert_chain_client, /**< 客户端证书链 */
} X509_CERT_CHAIN_TYPE;

/**
 * @brief 最大验证深度
 */
#define X509_MAX_VERIFY_DEPTH 6

/**
 * @brief 验证证书链的有效性
 *
 * @param certs 证书链数据指针
 * @param certslen 证书链数据长度
 * @param certs_type 证书链类型，参见 X509_CERT_CHAIN_TYPE 枚举
 * @param rootcerts 根证书数据指针
 * @param rootcertslen 根证书数据长度
 * @param depth 验证深度
 * @param verify_result 验证结果指针
 * @return 成功返回1，失败返回0
 */
int x509_certs_verify(const uint8_t *certs, size_t certslen, int certs_type,
						const uint8_t *rootcerts, size_t rootcertslen, int depth, int *verify_result);

/**
 * @brief 验证 TLCP 证书链的有效性
 *
 * @param certs 证书链数据指针
 * @param certslen 证书链数据长度
 * @param certs_type 证书链类型，参见 X509_CERT_CHAIN_TYPE 枚举
 * @param rootcerts 根证书数据指针
 * @param rootcertslen 根证书数据长度
 * @param depth 验证深度
 * @param verify_result 验证结果指针
 * @return 成功返回1，失败返回0
 */
int x509_certs_verify_tlcp(const uint8_t *certs, size_t certslen, int certs_type,
							const uint8_t *rootcerts, size_t rootcertslen, int depth, int *verify_result);

/**
 * @brief 获取证书链中所有证书的主体名称
 *
 * @param certs 证书链数据指针
 * @param certslen 证书链数据长度
 * @param names 主体名称缓冲区指针
 * @param nameslen 主体名称长度指针(输入时为缓冲区大小，输出时为实际长度)
 * @return 成功返回1，失败返回0
 */
int x509_certs_get_subjects(const uint8_t *certs, size_t certslen, uint8_t *names, size_t *nameslen);

/**
 * @brief 打印证书链信息
 *
 * @param fp 文件指针
 * @param fmt 格式标识
 * @param ind 缩进量
 * @param label 标签字符串
 * @param d 证书链数据指针
 * @param dlen 证书链数据长度
 * @return 成功返回1，失败返回0
 */
int x509_certs_print(FILE *fp, int fmt, int ind, const char *label, const uint8_t *d, size_t dlen);

/**
 * @brief 从文件创建新证书
 *
 * @param out 输出的证书数据指针
 * @param outlen 输出的证书数据长度指针
 * @param file 证书文件路径
 * @return 成功返回1，失败返回0
 */
int x509_cert_new_from_file(uint8_t **out, size_t *outlen, const char *file);

/**
 * @brief 从文件创建新证书链
 *
 * @param out 输出的证书链数据指针
 * @param outlen 输出的证书链数据长度指针
 * @param file 证书链文件路径
 * @return 成功返回1，失败返回0
 */
int x509_certs_new_from_file(uint8_t **out, size_t *outlen, const char *file);

#ifdef __cplusplus
}
#endif
#endif
