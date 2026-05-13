/*
 *  Copyright 2014-2023 The GmSSL Project. All Rights Reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the License); you may
 *  not use this file except in compliance with the License.
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 */

 #ifndef GMSSL_X509_CRL_H
 #define GMSSL_X509_CRL_H
 
 #include <time.h>
 #include <stdint.h>
 #include <gmssl/sm2.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
/**
  * @brief CRL原因码枚举
  * 
  * 定义证书撤销列表(CRL)中使用的撤销原因代码
  */
 typedef enum {
	 X509_cr_unspecified = 0,           /* 未指定原因 */
	 X509_cr_key_compromise = 1,        /* 密钥泄露 */
	 X509_cr_ca_compromise = 2,         /* CA泄露 */
	 X509_cr_affiliation_changed = 3,   /* 隶属关系变更 */
	 X509_cr_superseded = 4,            /* 证书被取代 */
	 X509_cr_cessation_of_operation = 5,/* 停止操作 */
	 X509_cr_certificate_hold = 6,      /* 证书暂挂 */
	 X509_cr_not_assigned = 7,          /* 未分配 */
	 X509_cr_remove_from_crl = 8,       /* 从CRL中移除 */
	 X509_cr_privilege_withdrawn = 9,   /* 特权撤销 */
	 X509_cr_aa_compromise = 10,        /* AA泄露 */
 } X509_CRL_REASON;
 
/**
  * @brief 获取CRL原因码的名称
  * 
  * @param reason 原因码值
  * @return 返回原因码对应的名称字符串
  */
 const char *x509_crl_reason_name(int reason);
 
/**
  * @brief 从名称获取CRL原因码
  * 
  * @param reason 输出参数，存储获取到的原因码
  * @param name 原因码名称字符串
  * @return 成功返回1，失败返回0
  */
 int x509_crl_reason_from_name(int *reason, const char *name);
 
/**
  * @brief 将CRL原因码编码为DER格式
  * 
  * @param reason 原因码值
  * @param out 输出DER编码的指针
  * @param outlen 输出DER编码的长度
  * @return 成功返回1，失败返回0
  */
 int x509_crl_reason_to_der(int reason, uint8_t **out, size_t *outlen);
 
/**
  * @brief 从DER格式解码CRL原因码
  * 
  * @param reason 输出参数，存储解码得到的原因码
  * @param in 输入DER编码的指针
  * @param inlen 输入DER编码的长度
  * @return 成功返回1，失败返回0
  */
 int x509_crl_reason_from_der(int *reason, const uint8_t **in, size_t *inlen);
 
/**
  * @brief 从DER格式隐式解码CRL原因码
  * 
  * @param index 索引值
  * @param reason 输出参数，存储解码得到的原因码
  * @param in 输入DER编码的指针
  * @param inlen 输入DER编码的长度
  * @return 成功返回1，失败返回0
  */
 int x509_implicit_crl_reason_from_der(int index, int *reason, const uint8_t **in, size_t *inlen);
 
/**
  * @brief 获取CRL条目扩展的OID名称
  * 
  * @param oid 扩展OID值
  * @return 返回扩展OID对应的名称字符串
  */
 const char *x509_crl_entry_ext_id_name(int oid);
 
/**
  * @brief 从名称获取CRL条目扩展的OID
  * 
  * @param name 扩展名称字符串
  * @return 返回扩展OID值
  */
 int x509_crl_entry_ext_id_from_name(const char *name);
 
/**
  * @brief 将CRL条目扩展OID编码为DER格式
  * 
  * @param oid 扩展OID值
  * @param out 输出DER编码的指针
  * @param outlen 输出DER编码的长度
  * @return 成功返回1，失败返回0
  */
 int x509_crl_entry_ext_id_to_der(int oid, uint8_t **out, size_t *outlen);
 
/**
  * @brief 从DER格式解码CRL条目扩展OID
  * 
  * @param oid 输出参数，存储解码得到的扩展OID
  * @param in 输入DER编码的指针
  * @param inlen 输入DER编码的长度
  * @return 成功返回1，失败返回0
  */
 int x509_crl_entry_ext_id_from_der(int *oid, const uint8_t **in, size_t *inlen);
 
/**
  * @brief 将CRL条目扩展编码为DER格式
  * 
  * @param oid 扩展OID值
  * @param critical 是否关键扩展
  * @param val 扩展值
  * @param vlen 扩展值长度
  * @param out 输出DER编码的指针
  * @param outlen 输出DER编码的长度
  * @return 成功返回1，失败返回0
  */
 int x509_crl_entry_ext_to_der(int oid, int critical, const uint8_t *val, size_t vlen, uint8_t **out, size_t *outlen);
 
/**
  * @brief 从DER格式解码CRL条目扩展
  * 
  * @param oid 输出参数，存储解码得到的扩展OID
  * @param critical 输出参数，存储是否关键扩展
  * @param val 输出参数，存储扩展值
  * @param vlen 输出参数，存储扩展值长度
  * @param in 输入DER编码的指针
  * @param inlen 输入DER编码的长度
  * @return 成功返回1，失败返回0
  */
 int x509_crl_entry_ext_from_der(int *oid, int *critical, const uint8_t **val, size_t *vlen, const uint8_t **in, size_t *inlen);
 
/**
  * @brief 检查CRL条目扩展的关键性是否符合规范
  * 
  * @param oid 扩展OID值
  * @param critical 是否关键扩展
  * @return 符合规范返回1，否则返回0
  */
 int x509_crl_entry_ext_critical_check(int oid, int critical);
 
/**
  * @brief 打印CRL条目扩展信息
  * 
  * @param fp 输出文件指针
  * @param fmt 格式控制
  * @param ind 缩进控制
  * @param label 标签字符串
  * @param d 扩展数据
  * @param dlen 扩展数据长度
  * @return 成功返回1，失败返回0
  */
 int x509_crl_entry_ext_print(FILE *fp, int fmt, int ind, const char *label, const uint8_t *d, size_t dlen);
 
 /* 其他函数的Doxygen注释按照相同模式添加... */
 
 #ifdef __cplusplus
 }
 #endif
 #endif