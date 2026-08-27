/**
 * @file nvrMib.h
 * @author tianl (tianl@kfb.cn)
 * @date 2024-10-21
 * 
 * @brief MIB管理信息库
 */
#pragma once

#include <string.h>
#include <agent_pp/mib.h>
#include <agent_pp/snmp_textual_conventions.h>
#include <libagent.h>

#include "system_manage.h"
#include "snmp_manage.h"

using namespace Agentpp;

/**************MIB管理信息库OID 用于获取设备信息****************/
#define OID_ITC_VERSIONGROUP     "1.3.6.1.4.1.50001"                 /* 私有MIB库标识 **/
#define OID_NVR_GROUP            "1.3.6.1.4.1.50001.1"               /* 私有MIB组 **/
#define OID_ITC_IP               "1.3.6.1.4.1.50001.1.1.0"           /* 设备IP地址 */
#define OID_ITC_PORT             "1.3.6.1.4.1.50001.1.2.0"           /* 设备端口号 */
#define OID_ITC_ENTITYINDEX      "1.3.6.1.4.1.50001.1.3.0"           /* 设备序列号 */ 
#define OID_ITC_ENTITYTYPE       "1.3.6.1.4.1.50001.1.100.0"         /* 设备产品类型 */
#define OID_ITC_ENTITYSUBTYPE    "1.3.6.1.4.1.50001.1.101.0"         /* 设备子产品类型 */
#define OID_ITC_ONLINE           "1.3.6.1.4.1.50001.1.102.0"         /* 设备状态 */
#define OID_ITC_TRAPHOSTIP1      "1.3.6.1.4.1.50001.1.103.0"         /* Trap地址 */
#define OID_ITC_MEMORYCAPABILITY "1.3.6.1.4.1.50001.1.104.0"         /* 设备内存大小 */
#define OID_ITC_MEMORYUSAGE      "1.3.6.1.4.1.50001.1.105.0"         /* 设备内存剩余使用率 */
#define OID_ITC_DEVICELANGUAGE   "1.3.6.1.4.1.50001.1.106.0"         /* 设备语言 */
#define OID_ITC_DISKNUM          "1.3.6.1.4.1.50001.1.240.0"         /* 设备硬盘个数 */
#define OID_ITC_DISKENTRY         "1.3.6.1.4.1.50001.1.240.1"         /* 硬盘信息入口 */
#define OID_ITC_DISKINDEX         "1.3.6.1.4.1.50001.1.241.1.1"       /* 设备硬盘索引 */
#define OID_ITC_DISKVOLUM         "1.3.6.1.4.1.50001.1.241.1.2"       /* 设备硬盘卷名 */
#define OID_ITC_DISKSTATUS        "1.3.6.1.4.1.50001.1.241.1.3"       /* 设备硬盘状态 */
#define OID_ITC_DISKFREESPACE     "1.3.6.1.4.1.50001.1.241.1.4"       /* 设备硬盘剩余容量 */
#define OID_ITC_DISKCAPABILITY    "1.3.6.1.4.1.50001.1.241.1.5"       /* 设备硬盘容量 */

/**************trap消息OID 设备发送消息到网络管理设备****************/
#define OID_POWERON              "1.3.6.1.4.1.50001.0.11.3.6.1.4.1.50001.0.1"        /* Trap消息，设备开机 */
#define OID_POWEROFF             "1.3.6.1.4.1.50001.0.11.3.6.1.4.1.50001.0.2"        /* Trap消息，设备关机 */
#define OID_REBOOT               "1.3.6.1.4.1.50001.0.11.3.6.1.4.1.50001.0.3"        /* Trap消息，设备重启 */

/**************节点初始值***************/
#define NVR_NAME        "nvrGroup"      /* 定义的NVR MIB信息库名称 */
#define DEFAULT_IP      "0.0.0.0"       /* IP节点初始值 */
#define DEFAULT_PORT    0               /* 端口节点初始值 */



/**
 * @brief MIB信息库IP节点
 */
class ItcIp : public MibLeaf {

public:
	ItcIp();
    /**
     * @brief 获取IP地址
     * @return IpAddress  IP地址
     */
    IpAddress getIp();

	void get_request(Request*, int);      
};

/**
 * @brief MIB信息库端口号节点
 */
class ItcPort : public MibLeaf {

public:
	ItcPort();
    
    /**
     * @brief 获取Port端口
     * @return SnmpInt32  端口号
     */
    SnmpInt32 getPort();

	void get_request(Request*, int);      
};

/**
 * @brief MIB信息库序列号节点
 */
class ItcEntityIndex : public MibLeaf {

public:
	ItcEntityIndex();
    
    /**
     * @brief 获取设备序列号
     * @return SnmpInt32  端口号
     */
    OctetStr getEntityIndex();

	void get_request(Request*, int);      
};


/**
 * @brief MIB信息库产品类型节点
 */
class ItcEntityType : public MibLeaf {

public:
	ItcEntityType();
    
    /**
     * @brief 获取产品类型
     * @return OctetStr  产品类型
     */
    OctetStr getEntityType();

	void get_request(Request*, int);      
};


/**
 * @brief MIB信息库产品子类型节点
 */
class ItcEntitySubType : public MibLeaf {

public:
	ItcEntitySubType();
    
    /**
     * @brief 获取产品子类型节点
     * @return OctetStr  产品子类型
     */
    OctetStr getEntitySubType();

	void get_request(Request*, int);      
};


/**
 * @brief MIB信息库设备状态节点
 */
class ItcOnline : public MibLeaf {

public:
	ItcOnline();
    
    /**
     * @brief 获取设备状态
     * @return OctetStr  设备状态
     */
    OctetStr getOnline();

	void get_request(Request*, int);      
};


/**
 * @brief MIB信息库trap地址节点
 */
class ItcTrapHostIp1 : public MibLeaf {

public:
	ItcTrapHostIp1();
    
    /**
     * @brief 获取trap地址
     * @return OctetStr trap地址
     */
    OctetStr getTrapHostIp1();

	void get_request(Request*, int);      
};


/**
 * @brief MIB信息库内存大小节点
 */
class ItcMemoryCapability : public MibLeaf {

public:
	ItcMemoryCapability();
    
    /**
     * @brief 获取内存大小
     * @return OctetStr 内存大小
     */
    OctetStr getMemoryCapability();

	void get_request(Request*, int);      
};

/**
 * @brief 内存剩余使用率
 */
class ItcMemoryUsage: public MibLeaf {

public:
	ItcMemoryUsage();
    
    /**
     * @brief 获取内存剩余使用率
     * @return OctetStr 内存剩余使用率
     */
    OctetStr getMemoryUsage();

	void get_request(Request*, int);      
};


/**
 * @brief MIB信息库设备语言
 */
class ItcDeviceLanguage: public MibLeaf {

public:
	ItcDeviceLanguage();
    
    /**
     * @brief 获取设备语言
     * @return OctetStr 设备语言
     */
    OctetStr getDeviceLanguage();

	void get_request(Request*, int);      
};

/**
 * @brief MIB信息库硬盘个数
 */
class ItcDiskNum: public MibLeaf {

public:
	ItcDiskNum();
    
    /**
     * @brief 硬盘个数
     * @return SnmpInt32 硬盘个数
     */
    SnmpInt32 getDiskNum();

	void get_request(Request*, int);      
};

/**
 * @brief MIB信息库硬盘信息入口
 */
class ItcDiskEntry: public MibLeaf {

public:
	ItcDiskEntry();
    
    /**
     * @brief 获取硬盘信息入口
     * @return OctetStr 硬盘信息入口
     */
    OctetStr getDiskEntry();

	void get_request(Request*, int);      
};

/**
 * @brief MIB信息库硬盘索引
 */
class ItcDiskIndex: public MibLeaf {

public:
	ItcDiskIndex();
    
    /**
     * @brief 获取硬盘索引
     * @return OctetStr 硬盘索引
     */
    OctetStr getDiskIndex();

	void get_request(Request*, int);      
};

/**
 * @brief MIB信息库硬盘卷名
 */
class ItcDiskVolum: public MibLeaf {

public:
	ItcDiskVolum();
    
    /**
     * @brief 获取硬盘卷名
     * @return OctetStr 硬盘卷名
     */
    OctetStr getDiskVolum();

	void get_request(Request*, int);      
};

/**
 * @brief MIB信息库硬盘状态
 */
class ItcDiskStatus: public MibLeaf {

public:
	ItcDiskStatus();
    
    /**
     * @brief 获取硬盘卷名
     * @return OctetStr 硬盘卷名
     */
    OctetStr getDiskStatus();

	void get_request(Request*, int);      
};

/**
 * @brief MIB信息库硬盘剩余容量
 */
class ItcDiskFreeSpace: public MibLeaf {

public:
	ItcDiskFreeSpace();
    
    /**
     * @brief 获取硬盘剩余容量
     * @return OctetStr 硬盘剩余容量
     */
    OctetStr getDiskFreeSpace();

	void get_request(Request*, int);      
};

/**
 * @brief MIB信息库硬盘容量
 */
class ItcDiskCapability: public MibLeaf {

public:
	ItcDiskCapability();
    
    /**
     * @brief 获取硬盘容量
     * @return OctetStr 硬盘容量
     */
    OctetStr getDiskCapability();

	void get_request(Request*, int);      
};


/**
 * @brief MIB信息库根节点
 */
class  nvrGroup: public MibGroup {
public:
    nvrGroup();
};

