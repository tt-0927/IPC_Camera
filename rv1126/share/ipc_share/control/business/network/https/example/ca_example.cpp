/*
 * @FilePath: CA_main.cpp
 * @Author: tianl
 * @Date: 2024-09-24 19:36:05
 * @LastEditors: tianl
 * @LastEditTime: 2024-09-26 11:42:49
 * @Description: CA证书测试用例
 */

#include <iostream>
#include <string>
#include "ca_manage.h"
#include "path_define.h"
#include "dlog.h"

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <option> [arguments]" << std::endl;
        std::cerr << "Options:" << std::endl;
        std::cerr << "  --generate-csr <CN> <C> [<O> <OU> <ST> <L> <Email>]                          生成证书请求" << std::endl;
        std::cerr << "  --generate-cert <certfile> <days> <CN> <C> [<O> <OU> <ST> <L> <Email>]       生成证书" << std::endl;
        std::cerr << "  --get-cert-info <certfile>                                                   获取并打印证书的所有信息" << std::endl;
        std::cerr << "  --check-ca <certfile>                                                        检查证书是否为合法的CA" << std::endl;
        return -1;
    }
    
    /* 日志初始化 */
    setLogLevel(LOG_INFO);
    initLog("ipc_logger", STREAM_LOG_PATH);

    Network::CertApplyInfo_S stApplyInfo;
    Network::CertFileInfo_S stCertInfo;

    std::string option = argv[1];

    /* 生成证书请求 */
    if (option == "--generate-csr")
    {
        if (argc < 4 || argc > 9)
        {
            std::cerr << "Usage: " << argv[0] << " --generate-csr <CN> <C> [<O> <OU> <ST> <L> <Email>] " << std::endl;
            return -1;
        }
        // 填充必选参数
        stApplyInfo.strCN = argv[2]; // CN
        stApplyInfo.strC = argv[3];  // 国家代码

        // 填充可选参数（如果提供了）
        if (argc > 4)
            stApplyInfo.strO = argv[4]; // 组织名称
        if (argc > 5)
            stApplyInfo.strOU = argv[5]; // 组织单元名称
        if (argc > 6)
            stApplyInfo.strST = argv[6]; // 省份名称
        if (argc > 7)
            stApplyInfo.strL = argv[7]; // 城市名称
        if (argc > 8)
            stApplyInfo.strEmail = argv[8]; // 电子邮件地址

        if (CCaManage::instance()->generateCsr(stApplyInfo) == 0)
        {
            std::cout << "证书请求生成成功：" << CA_REQ_CSR << std::endl;
        }
        else
        {
            std::cerr << "证书请求生成失败" << std::endl;
            return -1;
        }
    }
    /* 生成证书 */
    else if (option == "--generate-cert")
    {
        if (argc < 6 || argc > 11)
        {
            std::cerr << "Usage: " << argv[0] << " --generate-cert <certfile> <days> <CN> <C> [<O> <OU> <ST> <L> <Email>]" << std::endl;
            return -1;
        }

        std::string certFile = argv[2];

        std::cerr << "strCN:" << argv[4] << std::endl;
        // 填充必选参数
        stApplyInfo.nValday = std::stoi(argv[3]);
        stApplyInfo.strCN = argv[4]; // CN
        stApplyInfo.strC = argv[5];  // 国家代码

        // 填充可选参数（如果提供了）
        if (argc > 6)
            stApplyInfo.strO = argv[6]; // 组织名称
        if (argc > 7)
            stApplyInfo.strOU = argv[7]; // 组织单元名称
        if (argc > 8)
            stApplyInfo.strST = argv[8]; // 省份名称
        if (argc > 9)
            stApplyInfo.strL = argv[9]; // 城市名称
        if (argc > 10)
            stApplyInfo.strEmail = argv[10]; // 电子邮件地址

        if (CCaManage::instance()->generateCertificate(stApplyInfo, certFile) == 0)
        {
            std::cout << "证书生成成功: " << certFile << std::endl;
        }
        else
        {
            std::cerr << "证书生成失败" << std::endl;
            return -1;
        }
    }
    /* 获取并打印证书的所有信息 */
    else if (option == "--get-cert-info")
    {
        if (argc != 3)
        {
            std::cerr << "Usage: " << argv[0] << " --get-cert-info <certfile>" << std::endl;
            return -1;
        }
        std::string certFile = argv[2];

        // 调用获取证书信息的函数
        if (CCaManage::instance()->getCertificateInfo(certFile, stCertInfo) == 0)
        {
            std::cout << "证书到期日期: " << stCertInfo.strExpiraDate << std::endl;
            std::cout << "证书序列号: " << stCertInfo.strSerialNum << std::endl;
            std::cout << "使用者: " << stCertInfo.strUser << std::endl;
            std::cout << "颁发者: " << stCertInfo.strLicensor << std::endl;
        }
        else
        {
            std::cerr << "获取证书信息失败。" << std::endl;
            return -1;
        }
    }
    /* 检查证书是否合法 */
    else if (option == "--check-ca")
    {
        if (argc != 3)
        {
            std::cerr << "Usage: " << argv[0] << " --check-ca <certfile>" << std::endl;
            return -1;
        }
        std::string certFile = argv[2];

        if (CCaManage::instance()->isCertificateCA(certFile))
        {
            std::cout << "该文件是合法的CA证书。" << std::endl;
        }
        else
        {
            std::cout << "该文件不是CA证书。" << std::endl;
        }
    }
    else
    {
        std::cerr << "无效选项: " << option << std::endl;
        return -1;
    }

    return 0;
}