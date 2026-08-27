/***
 * @FilePath     : snmp_example.cpp
 * @Author       : tianl (tianl@kfb.cn)
 * @Date         : 2024-10-18 09:18:54
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-04-15 09:53:34
 * @Description  : snmp测试程序
 */

#include <csignal>
#include <unistd.h>
#include <iostream>
#include <string>
#include "snmp_manage.h"
#include "network_define.h"

static bool signal_received = false;

void signal_handler(int sig)
{
    std::cout << "Signal " << sig << " received, exiting..." << std::endl;
    // 设置一个全局变量或者静态局部变量来表示接收到了信号
    signal_received = true;
}

int main()
{
    // signal(SIGINT, signal_handler);

    CSnmpManage::instance()->init();

    Network::SnmpConfig_S stSnmpConf;

    stSnmpConf.bEnableSnmp = true;
    // 是否开启V3版本认证
    stSnmpConf.bEnableSmnpV3 = false;
    stSnmpConf.strReadCommunityName = "public";
    stSnmpConf.strWriteCommunityName = "private";
    stSnmpConf.strTrapAddress = "127.0.0.1";
    stSnmpConf.nTrapPort = 162;
    stSnmpConf.nSnmpPort = 161;

    // 初始化SNMPv3安全设置
    stSnmpConf.strReadSecurityName = "read1";
    stSnmpConf.stReadSecuritySettings.enAuth = Network::AUTH_MD5;
    stSnmpConf.stReadSecuritySettings.strAuthPassword = "readAuthPassword";
    stSnmpConf.stReadSecuritySettings.enPriv = Network::PRIV_DES;
    stSnmpConf.stReadSecuritySettings.strPrivPassword = "readPrivPassword";

    stSnmpConf.strWriteSecurityName = "write1";
    stSnmpConf.stWriteSecuritySettings.enAuth = Network::AUTH_SHA;
    stSnmpConf.stWriteSecuritySettings.strAuthPassword = "writeAuthPassword";
    stSnmpConf.stWriteSecuritySettings.enPriv = Network::PRIV_AES;
    stSnmpConf.stWriteSecuritySettings.strPrivPassword = "writePrivPassword";

    CSnmpManage::instance()->initializeSnmp(stSnmpConf);

    // 死循环，直到接收到 SIGINT 信号
    bool running = true;
    while (running)
    {
        // 检查是否接收到了 SIGINT 信号
        if (signal_received)
        {
            std::cout << "SNMP service is exit..." << std::endl;
            CSnmpManage::instance()->stopSnmpAgent();

            running = false;
        }
        else
        {
            sleep(1); // 每秒检查一次
        }
    }

    return 0;
}
