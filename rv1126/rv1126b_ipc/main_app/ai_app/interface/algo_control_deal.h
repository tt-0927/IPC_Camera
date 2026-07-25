/**
 * @FilePath     : algo_control_deal.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-06 16:02:10
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-07-09 15:55:11
 * @Description  : aiapp->control通讯客户端
 */

#pragma once

#include "IpcRet.h"
#include "IOBase.h"
#include "Singleton.h"
#include "TCPClient.h"
// #include "netlink_message.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <iostream>
#include <condition_variable>
#include "common_process.h"

#include "event_convert.h"
#include "convert_interface.h"


class AlgoControlDeal : public CSingleton<AlgoControlDeal>
{
    AlgoControlDeal() = default;

public:
    ~AlgoControlDeal() = default;
    friend class CSingleton<AlgoControlDeal>;
    
    /**
     * @brief   : 消息处理
     * @param    {int} nCode
     * @param    {string} strData
     */
    void deal_message(int nCode, std::string strData, void *pData);
};
