/*
 * @FilePath     : DevManageExtern.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-03-22 10:11:02
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-04-12 14:11:10
 * @Description  :
 */
#pragma once

#include <iostream>
#include <vector>

namespace Ai0630_NS
{
    struct GetReqDevInfo_S
    {
        int         nCurPageNum;   /* 搜索的页数 */
        int         nPageSize;     /* 这一页的数据数量 */
        std::string strMacKey;     /* 搜索MAC关键字 */
        std::string strDevNameKey; /* 搜索设备名称关键字 */
        std::string strModelKey;   /* 搜索设备型号关键字 */

        void clear()
        {
            nCurPageNum = 0;
            nPageSize   = 0;
            strMacKey.clear();
            strDevNameKey.clear();
            strModelKey.clear();
        }

        void print() const
        {
            std::cout << "\n获取设备信息请求:=============" << std::endl;
            std::cout << "搜索的页数:" << nCurPageNum << std::endl;
            std::cout << "这一页的数据数量:" << nPageSize << std::endl;
            std::cout << "搜索MAC关键字:" << strMacKey << std::endl;
            std::cout << "搜索设备名称关键字:" << strDevNameKey << std::endl;
            std::cout << "搜索设备型号关键字:" << strModelKey << std::endl;
            std::cout << "end:=============" << std::endl;
        }
    };

    /* 新增设备 */
    struct AddDevInfo_S
    {
        std::string strDevName; /* 设备名称 */
        std::string strIp;      /* IP地址 */

        void clear()
        {
            strDevName.clear();
            strIp.clear();
        }
    };

    /* 编辑设备 */
    struct EditDevInfo_S
    {
        int         nId;        /* 唯一ID */
        std::string strDevName; /* 设备名称 */

        void clear()
        {
            nId = 0;
            strDevName.clear();
        }
    };

    /* 删除设备 */
    struct DelDevInfo_S
    {
        std::vector<int> vDelId; /* 需要删除的ID */

        void clear()
        {
            vDelId.clear();
        }
    };

    struct DevDataInfo_S
    {
        int         nId;
        std::string strDevName;  /* 设备名称 */
        std::string strDevModel; /* 设备型号 */
        std::string strMac;      /* MAC地址 */
        std::string strIP;       /* IP地址 */
        int         nState;      /* 连接状态 0-断开 1-连接 */

        void clear()
        {
            nId = 0;
            strDevName.clear();
            strDevModel.clear();
            strMac.clear();
            strIP.clear();
            nState = 0;
        }

        void print() const
        {
            std::cout << "\n设备信息:=============" << std::endl;
            std::cout << "ID:" << nId << std::endl;
            std::cout << "设备名称:" << strDevName << std::endl;
            std::cout << "设备型号:" << strDevModel << std::endl;
            std::cout << "MAC地址:" << strMac << std::endl;
            std::cout << "IP地址:" << strIP << std::endl;
            std::cout << "连接状态:" << nState << std::endl;
            std::cout << "end:=============" << std::endl;
        }
    };

    typedef struct _DevCommInfo_
    {
        void*         pHandle;
        DevDataInfo_S stDevInfo;

        void clear()
        {
            pHandle = nullptr;
            stDevInfo.clear();
        }

    } DevCommInfo_S;

}    // namespace Ai0630_NS