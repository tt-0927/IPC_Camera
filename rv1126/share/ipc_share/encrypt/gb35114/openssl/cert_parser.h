/**
 * @FilePath     : cert_parser.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-05-26 16:48:06
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-05-27 16:55:01
 * @Description  : 证书结构化解析模块
 */

#pragma once

#include <string>
#include <vector>

/* 证书信息结构体 */
struct CertInfo_S {
    /* 证书序列号（十六进制字符串） */
    std::string strSerialNumber;
    /* Subject 通用名称（CN） */
    std::string strSubjectCN;
    /* Issuer 通用名称（CN） */
    std::string strIssuerCN;
    /* 证书生效时间 */
    std::string strNotBefore;
    /* 证书过期时间 */
    std::string strNotAfter;
    /* KeyUsage: 数字签名权限 */
    bool bKeyUsageDigitalSignature = false;
    /* KeyUsage: 证书签名权限 */
    bool bKeyUsageKeyCertSign = false;
    /* KeyUsage: CRL 签名权限 */
    bool bKeyUsageCRLSign = false;
    /* BasicConstraints: 是否为 CA 证书 */
    bool bBasicConstraintsCA = false;
    /* BasicConstraints: 路径长度约束（-1 表示无限制） */
    int iPathLenConstraint = -1;
    /* SubjectAltName 列表（DNS 或 IP 地址） */
    std::vector<std::string> vecSubjectAltName;
};

/**
 * @brief   : 解析证书文件
 * @param    {string} cert_path 证书文件路径（PEM或DER格式）
 * @param    {CertInfo_S&} info 证书信息输出结构体
 * @return   {bool} true：成功，false：失败
 */
bool parse_certificate(const std::string& cert_path, CertInfo_S& info);

/**
 * @brief   : 解析证书请求文件
 * @param    {string} csr_path CSR文件路径
 * @param    {CertInfo_S&} info 证书请求信息输出结构体
 * @return   {bool} true：成功，false：失败
 */
bool parse_csr(const std::string& csr_path, CertInfo_S& info);

/**
 * @brief   : 解析CRL文件
 * @param    {string} crl_path CRL文件路径
 * @param    {string&} info_str CRL信息输出字符串
 * @return   {bool} true：成功，false：失败
 */
bool parse_crl(const std::string& crl_path, std::string& info_str);

/**
 * @brief   : 验证CRL签名
 * @param    {string} crl_path CRL文件路径
 * @param    {string} issuer_cert_path 签发者证书路径
 * @return   {bool} true：验证成功，false：验证失败
 */
bool verify_crl(const std::string& crl_path, const std::string& issuer_cert_path);

/**
 * @brief   : 使用 CA 证书验证设备证书签名
 * @param    {string} cert_path 待验证证书路径
 * @param    {string} issuer_cert_path 签发者 CA 证书路径
 * @return   {bool} true：验证成功，false：验证失败
 * @note    : 用于上传本地设备证书时确认其由当前受信 CA 签发。
 */
bool verify_certificate_by_ca(const std::string& cert_path, const std::string& issuer_cert_path);
