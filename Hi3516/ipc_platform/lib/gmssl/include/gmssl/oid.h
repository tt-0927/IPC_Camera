/*
 *  Copyright 2014-2023 The GmSSL Project. All Rights Reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the License); you may
 *  not use this file except in compliance with the License.
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 */

// 防止头文件被重复包含
#ifndef GMSSL_OID_H
#define GMSSL_OID_H

#include <stdint.h>

// 如果是 C++ 环境，使用 extern "C" 确保 C 函数调用约定
#ifdef __cplusplus
extern "C" {
#endif

// 定义对象标识符（OID）的枚举
enum {
    // 未定义的 OID
    OID_undef = 0,

    // GM/T 0006 - 2012 中的商密算法方案
    // SM1 对称加密算法
    OID_sm1,
    // SSF33 对称加密算法
    OID_ssf33,
    // SM4 对称加密算法
    OID_sm4,
    // ZUC 流密码算法
    OID_zuc,
    // SM2 椭圆曲线公钥密码算法
    OID_sm2,
    // SM2 签名算法
    OID_sm2sign,
    // SM2 密钥交换算法
    OID_sm2keyagreement,
    // SM2 加密算法
    OID_sm2encrypt,
    // SM9 标识密码算法
    OID_sm9,
    // SM9 签名算法
    OID_sm9sign,
    // SM9 密钥交换算法
    OID_sm9keyagreement,
    // SM9 加密算法
    OID_sm9encrypt,
    // SM3 密码杂凑算法
    OID_sm3,
    // 无密钥的 SM3 算法
    OID_sm3_keyless,
    // 使用 SM3 的 HMAC 算法
    OID_hmac_sm3,
    // 使用 SM3 进行 SM2 签名
    OID_sm2sign_with_sm3,
    // 使用 SM3 进行 RSA 签名
    OID_rsasign_with_sm3,
    // X9.62 标准中的椭圆曲线公钥
    OID_ec_public_key,
    // NIST P-192 椭圆曲线
    OID_prime192v1,
    // NIST P-256 椭圆曲线
    OID_prime256v1,
    // SECG secp256k1 椭圆曲线
    OID_secp256k1,
    // SECG secp192k1 椭圆曲线
    OID_secp192k1,
    // SECG secp224k1 椭圆曲线
    OID_secp224k1,
    // SECG secp224r1 椭圆曲线
    OID_secp224r1,
    // SECG secp384r1 椭圆曲线
    OID_secp384r1,
    // SECG secp521r1 椭圆曲线
    OID_secp521r1,

    // X.500 目录属性相关 OID
    // 名称
    OID_at_name,
    // 姓氏
    OID_at_surname,
    // 名字
    OID_at_given_name,
    // 名字首字母
    OID_at_initials,
    // 世代限定词
    OID_at_generation_qualifier,
    // 通用名
    OID_at_common_name,
    // 所在地名称
    OID_at_locality_name,
    // 州或省名称
    OID_at_state_or_province_name,
    // 组织名称
    OID_at_organization_name,
    // 组织单位名称
    OID_at_organizational_unit_name,
    // 头衔
    OID_at_title,
    // 区别名限定词
    OID_at_dn_qualifier,
    // 国家名称
    OID_at_country_name,
    // 序列号
    OID_at_serial_number,
    // 笔名
    OID_at_pseudonym,
    // 域名组件
    OID_domain_component,
    // 电子邮件地址
    OID_email_address,

    // 证书扩展相关 OID
    // 颁发者密钥标识符扩展
    OID_ce_authority_key_identifier,
    // 主题密钥标识符扩展
    OID_ce_subject_key_identifier,
    // 密钥使用扩展
    OID_ce_key_usage,
    // 证书策略扩展
    OID_ce_certificate_policies,
    // 策略映射扩展
    OID_ce_policy_mappings,
    // 主题备用名称扩展
    OID_ce_subject_alt_name,
    // 颁发者备用名称扩展
    OID_ce_issuer_alt_name,
    // 主题目录属性扩展
    OID_ce_subject_directory_attributes,
    // 基本约束扩展
    OID_ce_basic_constraints,
    // 名称约束扩展
    OID_ce_name_constraints,
    // 策略约束扩展
    OID_ce_policy_constraints,
    // 扩展密钥使用扩展
    OID_ce_ext_key_usage,
    // CRL 分发点扩展
    OID_ce_crl_distribution_points,
    // 抑制 AnyPolicy 扩展
    OID_ce_inhibit_any_policy,
    // 最新 CRL 扩展
    OID_ce_freshest_crl,
    // Netscape 证书类型扩展
    OID_netscape_cert_type,
    // Netscape 证书注释扩展
    OID_netscape_cert_comment,
    // CT 预证书 SCTs 扩展
    OID_ct_precertificate_scts,

    // 权威信息访问描述符 OID
    // CA 颁发者访问描述符
    OID_ad_ca_issuers,
    // OCSP 访问描述符
    OID_ad_ocsp,

    // CRL 扩展相关 OID
    // CRL 编号扩展
    OID_ce_crl_number,
    // 增量 CRL 指示符扩展
    OID_ce_delta_crl_indicator,
    // 颁发分发点扩展
    OID_ce_issuing_distribution_point,
    // 权威信息访问扩展
    OID_pe_authority_info_access,

    // CRL 条目扩展相关 OID
    // CRL 撤销原因扩展
    OID_ce_crl_reasons,
    // 无效日期扩展
    OID_ce_invalidity_date,
    // 证书颁发者扩展
    OID_ce_certificate_issuer,

    // X.509 密钥用途标识符 OID
    // 任何扩展密钥使用
    OID_any_extended_key_usage,
    // 服务器身份验证
    OID_kp_server_auth,
    // 客户端身份验证
    OID_kp_client_auth,
    // 代码签名
    OID_kp_code_signing,
    // 电子邮件保护
    OID_kp_email_protection,
    // 时间戳签名
    OID_kp_time_stamping,
    // OCSP 签名
    OID_kp_ocsp_signing,

    // 合格声明相关 OID
    // CPS 声明
    OID_qt_cps,
    // 免责声明
    OID_qt_unotice,

    // 哈希算法 OID
    // MD5 哈希算法
    OID_md5,
    // SHA-1 哈希算法
    OID_sha1,
    // SHA-224 哈希算法
    OID_sha224,
    // SHA-256 哈希算法
    OID_sha256,
    // SHA-384 哈希算法
    OID_sha384,
    // SHA-512 哈希算法
    OID_sha512,
    // SHA-512/224 哈希算法
    OID_sha512_224,
    // SHA-512/256 哈希算法
    OID_sha512_256,

    // 使用哈希算法的 HMAC 算法 OID
    // 使用 SHA-1 的 HMAC 算法
    OID_hmac_sha1,
    // 使用 SHA-224 的 HMAC 算法
    OID_hmac_sha224,
    // 使用 SHA-256 的 HMAC 算法
    OID_hmac_sha256,
    // 使用 SHA-384 的 HMAC 算法
    OID_hmac_sha384,
    // 使用 SHA-512 的 HMAC 算法
    OID_hmac_sha512,
    // 使用 SHA-512/224 的 HMAC 算法
    OID_hmac_sha512_224,
    // 使用 SHA-512/256 的 HMAC 算法
    OID_hmac_sha512_256,

    // PKCS#5 PBKDF2 密钥派生函数
    OID_pbkdf2,
    // PKCS#5 PBES2 密码基加密方案
    OID_pbes2,

    // SM4 对称加密模式 OID
    // SM4 ECB 模式
    OID_sm4_ecb,
    // SM4 CBC 模式
    OID_sm4_cbc,

    // AES 对称加密算法 OID
    // AES 算法
    OID_aes,
    // AES-128 CBC 模式
    OID_aes128_cbc,
    // AES-192 CBC 模式
    OID_aes192_cbc,
    // AES-256 CBC 模式
    OID_aes256_cbc,

    // AES-128 算法
    OID_aes128,

    // ECDSA 签名算法与哈希算法组合 OID
    // 使用 SHA-1 的 ECDSA 签名算法
    OID_ecdsa_with_sha1,
    // 使用 SHA-224 的 ECDSA 签名算法
    OID_ecdsa_with_sha224,
    // 使用 SHA-256 的 ECDSA 签名算法
    OID_ecdsa_with_sha256,
    // 使用 SHA-384 的 ECDSA 签名算法
    OID_ecdsa_with_sha384,
    // 使用 SHA-512 的 ECDSA 签名算法
    OID_ecdsa_with_sha512,

    // RSA 签名算法与哈希算法组合 OID
    // 使用 MD5 的 RSA 签名算法
    OID_rsasign_with_md5,
    // 使用 SHA-1 的 RSA 签名算法
    OID_rsasign_with_sha1,
    // 使用 SHA-224 的 RSA 签名算法
    OID_rsasign_with_sha224,
    // 使用 SHA-256 的 RSA 签名算法
    OID_rsasign_with_sha256,
    // 使用 SHA-384 的 RSA 签名算法
    OID_rsasign_with_sha384,
    // 使用 SHA-512 的 RSA 签名算法
    OID_rsasign_with_sha512,

    // RSA 加密算法 OID
    // RSA 加密算法
    OID_rsa_encryption,
    // RSAES-OAEP 加密方案
    OID_rsaes_oaep,

    // 任何策略
    OID_any_policy,

    // CMS 数据类型 OID
    // CMS 数据类型
    OID_cms_data,
    // CMS 签名数据类型
    OID_cms_signed_data,
    // CMS 封装数据类型
    OID_cms_enveloped_data,
    // CMS 签名并封装数据类型
    OID_cms_signed_and_enveloped_data,
    // CMS 加密数据类型
    OID_cms_encrypted_data,
    // CMS 密钥协商信息类型
    OID_cms_key_agreement_info,
};

// {iso(1) org(3) dod(6) internet(1) security(5) mechanisms(5) pkix(7)}
// PKIX 相关 OID 前缀
#define oid_pkix    1,3,6,1,5,5,7

// 策略信息扩展 OID 前缀
#define oid_pe      oid_pkix,1
// 合格声明 OID 前缀
#define oid_qt      oid_pkix,2
// 密钥用途 OID 前缀
#define oid_kp      oid_pkix,3
// 权威信息访问 OID 前缀
#define oid_ad      oid_pkix,48

// {iso(1) member-body(2) us(840) rsadsi(113549)}
// RSA 数据安全公司相关 OID 前缀
#define oid_rsadsi  1,2,840,113549
// PKCS 相关 OID 前缀
#define oid_pkcs    oid_rsadsi,1
// PKCS#5 相关 OID 前缀
#define oid_pkcs5   oid_pkcs,5

// {iso(1) member-body(2) us(840) ansi-x962(10045)}
// X9.62 标准相关 OID 前缀
#define oid_x9_62   1,2,840,10045

// 目录属性相关 OID 前缀
#define oid_at 2,5,4
// 证书扩展相关 OID 前缀
#define oid_ce 2,5,29

// 商密算法相关 OID 前缀
#define oid_sm      1,2,156,10197
// 商密算法列表相关 OID 前缀
#define oid_sm_algors oid_sm,1
// SM2 CMS 相关 OID 前缀
#define oid_sm2_cms oid_sm,6,1,4,2

// 计算 OID 节点数组的元素个数
#define oid_cnt(nodes) (sizeof(nodes)/sizeof((nodes)[0]))

// 如果是 C++ 环境，结束 extern "C" 块
#ifdef __cplusplus
}
#endif

// 结束头文件保护
#endif
    