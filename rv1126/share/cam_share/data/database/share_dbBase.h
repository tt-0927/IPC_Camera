/*
 * @FilePath: share_dbBase.h
 * @Author: yangwenyao
 * @Date: 2022-12-09 16:44:33
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2022-12-09 16:54:42
 * @Descripttion: 数据库服务
 */
#ifndef __SHARE_DBBASE_H__
#define __SHARE_DBBASE_H__

#include <unistd.h>
#include <sys/types.h>
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <time.h>
#include "dlog.h"
#include "share_os.h"
#include "share_socket.h"
#include "share_port.h"
#include "db_middle.h"


/*==============================================================================
    函数: <DB_connect_server>
    功能: 创建连接DB server
    参数:
    返回值:成功返回连接的socket ,   否则失败
==============================================================================*/
int DB_connect_server(int port);



#endif
