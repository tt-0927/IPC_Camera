/*
 * @FilePath: ca_manage.h
 * @Author: tianl
 * @Date: 2024-09-24 19:36:05
 * @LastEditors: 梁浩尧 lianghaoyao@kfb.cn
 * @LastEditTime: 2025-08-07 11:07:16
 * @Description: CA证书管理类
 */

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cstdio>
#include <ctime>
#include <time.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <openssl/bio.h>
#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/rand.h>
#include <openssl/asn1.h>
#include <openssl/x509.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509v3.h>
#include <openssl/conf.h>
#include <time.h>
#include "Singleton.h"
#include "network_define.h"

#include "path_define.h"

#define DATA_LEN            128 /* 数组大小 */
#define CERTIFICATE_VERSION 2   /* 证书版本号 */
#define SERIAL_NUMBER_BITS  64  /* 序列号随机数位数 */

#define KEY_LENGTH_2048       2048             /* 2048位的密钥长度 */
#define KEY_LENGTH_1024       1024             /* 1024位的密钥长度 */
#define DAYS_TO_SECONDS(days) ((days) * 86400) /* 天数转换为秒数 */
#define OFFSET_EIGHT_HOUR     (8 * 3600)       /* 偏移八个小时 */

#define MONTHS_ARRAY {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"} /* 月份 */

class CCaManage : public CSingleton<CCaManage>
{
    CCaManage();

public:
    ~CCaManage();
    friend class CSingleton<CCaManage>;

    /**
     * @brief 获取信任证书信息
     * @param stCertInfo
     * @return int
     */
    int getTrustInfo(std::vector<Network::CertFileInfo_S> &stCertInfo);

    /**
     * @brief 安装受信任证书
     * @param stCertFileInfo
     * @return int
     */
    int installTrustCert(Network::CertFileInfo_S &stCertFileInfo);

    /**
     * @brief 删除受信任证书
     * @param stCertFileInfo
     * @return int
     */
    int deleteTrustceCert(Network::CertFileInfo_S &stCertFileInfo);

    /**
     * @brief 获取设备证书信息
     * @param stCertInfo
     * @return int
     */
    int getDeviceInfo(std::vector<Network::CertFileInfo_S> &stCertInfo);

    /**
     * @brief 安装设备证书
     * @param stCertFileInfo
     * @return int
     */
    int installDeviceCert(Network::CertFileInfo_S &stCertFileInfo);

    /**
     * @brief 创建安装设备证书
     * @param stApplyInfo
     * @return int
     */
    int creatInstallDeviceCert(const Network::CertApplyInfo_S &stApplyInfo);

    /**
     * @brief 删除设备证书
     * @param strSerialNum
     * @return int
     */
    int deleteDeviceCert(const Network::CertFileInfo_S &stCertFileInfo);

    /**
     * @brief 删除请求证书
     * @param
     * @return int
     */
    int deleteRequestCsr();

    /**
     * @brief 生成证书签名请求文件
     * @param {CertFileInfo_S&} 证书请求信息
     * @return {int} 返回 0 表示成功，返回 -1 表示失败
     */
    int generateCsr(const Network::CertApplyInfo_S &stApplyInfo);

    /**
     * @brief: 获取证书的信息，包括到期日期、序列号、使用者和颁发者。
     * @param {string} strCertFilePath: 证书文件的路径。
     * @param {CertFileInfo_S&} stCertInfo: 用于存储获取到的证书信息的结构体引用。
     * @return {int}: 返回0表示成功，返回-1表示失败。
     */
    int getCertificateInfo(const std::string &strCertFilePath, Network::CertFileInfo_S &stCertInfo);

    /**
     * @brief: 检查给定的证书是否为合法的 CA 证书
     * @param {string&*} strCertPath: 要检查的证书
     * @return {bool}: 如果是 CA 证书返回 true，否则返回 false
     */
    bool isCertificateCA(const std::string &strCertPath);

    /**
     * @brief: 比对给定证书的公钥和私钥是否匹配
     * @param {std::string} strCertPath: 证书文件的路径
     * @param {std::string} strKeyPath: 私钥文件的路径
     * @return {int }: 返回 0 表示匹配成功，返回 -1 表示匹配失败
     */
    int compareCertKey(const std::string &strCertPath, const std::string &strKeyPath);

    /**
     * @brief: 将 time_t 格式化为 YYYY-MM-DD HH:MM:SS
     * @param {time_t} t: time_t 格式的时间
     * @return {string}: 格式化后的时间字符串，如果失败则返回 NULL
     */
    std::string convertASN1Time(const ASN1_TIME *pTime);

    /**
     * @brief: 将 GMT 时间字符串转换为北京时间，并格式化为 "YYYY-MM-DD HH:MM:SS"
     * @param {const std::string&} strTimeStr: GMT 格式的时间字符串，例如 "Oct 9 08:10:23 2024"
     * @return {std::string}: 格式化后的北京时间字符串，如果转换失败则返回空字符串
     */
    std::string convertGMTToBeijingTime(const std::string &strTimeStr);

    /**
     * @brief 获取文件的扩展名
     * @param {std::string} strFilename 文件名字符串
     * @return {std::string} 返回文件扩展名。如果没有扩展名，则返回空字符串s
     */
    std::string getFileExtension(const std::string &strFilename);

    /**
     * @brief 检查证书是否为 CA 证书
     * @param { X509*} pCert 指向要检查的证书对象的指针
     * @return {bool} 如果证书是 CA 证书，则返回 true；否则返回 false
     */
    bool isCA(X509 *pCert);

    /**
     * @brief 生成证书
     * @param {CertFileInfo_S&} stApplyInfo 证书请求的详细信息
     * @param {std::string&} strCertFile  生成的证书
     * @param {int} nValday  证书的有效期
     * @return {int} 返回 0 表示成功，返回 -1 表示失败
     */
    int generateCertificate(const Network::CertApplyInfo_S &stApplyInfo, std::string &strCertFile);

    /**
     * @brief 检查证书是否是合法证书
     * @param strCertPath 证书路径
     * @return true 合法
     * @return false 不合法
     */
    bool isCertificateDevice(const std::string& strCertPath);

    /**
     * @brief 检查证书是否已经到期
     * @param strCertPath 证书路径
     * @return int 
     */
    int CheckCertExpired(const std::string& strCertPath);
    
    /**
     * @brief 处理导入的设备证书
     * @return int 
     */
    int dealUploadDeviceCert();

    /**
     * @brief 处理导入的CA证书
     * @return int 
     */
    int dealUploadCaCert();

private:
    /**
     * @brief: 生成一个密钥对
     * @param {int} bits 指定的密钥长度
     * @return {EVP_PKEY*} 返回一个指向新生成的密钥对的指针，NULL失败
     */
    EVP_PKEY *generateKey(int nBits);

    /**
     * @brief: 获取证书的有效期，并将其格式化为 YYYY-MM-DD HH:MM:SS。
     * @param {string&} strCertPath: 证书文件的路径。
     * @return {string}: 格式化后的有效期字符串，如果失败则返回 NULL。
     */
    std::string getCertificateExpirationDate(const std::string &strCertPath);

    /**
     * @brief: 获取指定路径的证书序列号并转换为十六进制字符
     * @param {std::string&} certPath: 证书文件路径
     * @return {std::string}: 证书序列号的十六进制字符串，如果失败返回空字符串
     */
    std::string getCertificateSerialNumber(const std::string &strCertPath);

    /**
     * @brief: 获取证书颁发者中的 CN 字段
     * @param {std::string} strCertFilePath: 证书文件的路径
     * @return {std::string}: 返回颁发者的 CN 字段，如果获取失败，则返回空字符串
     */
    std::string getIssuerCN(const std::string &strCertFilePath);

    /**
     * @brief: 获取证书使用者中的 CN 字段
     * @param {std::string} strCertFilePath: 证书文件的路径
     * @return {std::string}: 返回使用者的 CN 字段，如果获取失败，则返回空字符串
     */
    std::string getSubjectCN(const std::string &strCertFilePath);

    /**
     * @description: 将 CRT 格式的证书转换为 CER 格式。
     * @param {const std::string&} strCrtFile: 输入的 CRT 文件路径。
     * @param {const std::string&} strCerFile: 输出的 CER 文件路径。
     * @return {int}: 0 表示成功，-1 表示失败。
     */
    int convertCrtToCer(const std::string &strCrtFile, const std::string &strCerFile);

    /**
     * @description: 将 CER 格式的证书转换为 CRT 格式。
     * @param {const std::string&} strCerFile: 输入的 CER 文件路径。
     * @param {const std::string&} strCrtFile: 输出的 CRT 文件路径。
     * @return {int}: 0 表示成功，-1 表示失败。
     */
    int convertCerToCrt(const std::string &strCerFile, const std::string &strCrtFile);
};
