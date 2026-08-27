/**
 * @file nvrMib.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2024-10-21
 * 
 * @brief MIB管理信息库接口实现
 */


#include "ipc_mib.h"
// #include "DiskManage.h"
#include "dlog.h"
extern "C" {
   #include "edukit_network.h"
   #include "get_sys_info_interface.h"
}




/**********************************************************************
 *
 *  class ItcIp IP 只读
 *
 **********************************************************************/
ItcIp::ItcIp(): MibLeaf(OID_ITC_IP, READONLY, new IpAddress(DEFAULT_IP))
{
      
}

IpAddress ItcIp::getIp()
{
    char *achIp = get_primary_ip();
    std::string strDynamicIp; 
    if(achIp == NULL)
    {
        dlog_error("没有网络");
        strDynamicIp = "";
    }
    else
    {
        strDynamicIp = achIp;
    }
    
    return IpAddress(strDynamicIp.c_str());
}

void ItcIp::get_request(Request* req, int ind)
{
     /* 动态获取 IP 地址并设置到 MibLeaf */ 
     *((IpAddress*)value) = getIp(); 
    MibLeaf::get_request(req, ind);
}


/**********************************************************************
 *
 *  class ItcPort 端口号 只读
 *
 **********************************************************************/
ItcPort::ItcPort(): MibLeaf(OID_ITC_PORT, READONLY,  new SnmpInt32(DEFAULT_PORT))
{
      
}

SnmpInt32 ItcPort::getPort()
{
    SnmpInt32 nPort = 9000; 
    return nPort;
}

void ItcPort::get_request(Request* req, int ind)
{
     /* 动态获取端口号并设置到 MibLeaf */ 
    *((SnmpInt32*)value) = getPort();;
    MibLeaf::get_request(req, ind);
}

/**********************************************************************
 * 
 *  class ItcEntityIndex 序列号 只读
 *
 **********************************************************************/
ItcEntityIndex::ItcEntityIndex(): MibLeaf(OID_ITC_ENTITYINDEX, READONLY,  new OctetStr(""))
{
}

OctetStr ItcEntityIndex::getEntityIndex()
{
    std::string strEntityIndex = CSnmpManage::instance()->getDeviceInfo().serialNumber;
    return OctetStr(strEntityIndex.c_str());
}

void ItcEntityIndex::get_request(Request* req, int ind)
{
     /* 获取序列号并设置到 MibLeaf */ 
    *((OctetStr*)value) = getEntityIndex();;
    MibLeaf::get_request(req, ind);
}


/**********************************************************************
 *
 *  class ItcEntityType 产品类型 只读
 *
 **********************************************************************/
ItcEntityType::ItcEntityType(): MibLeaf(OID_ITC_ENTITYTYPE, READONLY,  new OctetStr(""))
{
}

OctetStr ItcEntityType::getEntityType()
{
    std::string strEntityType = "ITC NVR";
    return OctetStr(strEntityType.c_str());
}

void ItcEntityType::get_request(Request* req, int ind)
{
     /* 获取产品类型并设置到 MibLeaf */ 
    *((OctetStr*)value) = getEntityType();;
    MibLeaf::get_request(req, ind);
}

/**********************************************************************
 *
 *  class ItcEntityType 产品子类型 只读
 *
 **********************************************************************/
ItcEntitySubType::ItcEntitySubType(): MibLeaf(OID_ITC_ENTITYSUBTYPE, READONLY,  new OctetStr(""))
{
}

OctetStr ItcEntitySubType::getEntitySubType()
{
    std::string strEntitySubType = CSnmpManage::instance()->getDeviceInfo().strUnitTpye;
    return OctetStr(strEntitySubType.c_str());
}

void ItcEntitySubType::get_request(Request* req, int ind)
{
     /* 获取产品子类型并设置到 MibLeaf */ 
    *((OctetStr*)value) = getEntitySubType();;
    MibLeaf::get_request(req, ind);
}

/**********************************************************************
 *
 *  class ItcOnline 设备状态 只读
 *
 **********************************************************************/
ItcOnline::ItcOnline(): MibLeaf(OID_ITC_ONLINE, READONLY,  new OctetStr(""))
{
}

OctetStr ItcOnline::getOnline()
{
    std::string strOnline = "Online";
    return OctetStr(strOnline.c_str());
}

void ItcOnline::get_request(Request* req, int ind)
{
     /* 获取设备状态并设置到 MibLeaf */ 
    *((OctetStr*)value) = getOnline();
    MibLeaf::get_request(req, ind);
}


/**********************************************************************
 *
 *  class ItcTrapHostIp1 trap地址 只读
 *
 **********************************************************************/
ItcTrapHostIp1::ItcTrapHostIp1(): MibLeaf(OID_ITC_TRAPHOSTIP1, READONLY,  new OctetStr(""))
{
}

OctetStr ItcTrapHostIp1::getTrapHostIp1()
{
    std::string strTrapHostIp1 = CSnmpManage::instance()->getTrapAdress();
    return OctetStr(strTrapHostIp1.c_str());
}

void ItcTrapHostIp1::get_request(Request* req, int ind)
{
     /* 获取trap地址并设置到 MibLeaf */ 
    *((OctetStr*)value) = getTrapHostIp1();
    MibLeaf::get_request(req, ind);
}

/**********************************************************************
 *
 *  class ItcMemoryCapability 内存大小 只读
 *
 **********************************************************************/
ItcMemoryCapability::ItcMemoryCapability(): MibLeaf(OID_ITC_MEMORYCAPABILITY, READONLY,  new OctetStr(""))
{
}

OctetStr ItcMemoryCapability::getMemoryCapability()
{
    MemInfo_S stMemInfo;
    get_mem_info(&stMemInfo);
    double dMem = stMemInfo.ulTotal / 1024.0 / 1024.0;
    std::string strMemoryCapability =  std::to_string(dMem) + "GB";
    dlog_info("内存大小：%s",strMemoryCapability.c_str());
    return OctetStr(strMemoryCapability.c_str());
}

void ItcMemoryCapability::get_request(Request* req, int ind)
{
     /* 获取内存大小并设置到 MibLeaf */ 
    *((OctetStr*)value) = getMemoryCapability();
    MibLeaf::get_request(req, ind);
}

/**********************************************************************
 *
 *  class ItcMemoryUsage 内存剩余使用率 只读
 *
 **********************************************************************/
ItcMemoryUsage::ItcMemoryUsage(): MibLeaf(OID_ITC_MEMORYUSAGE, READONLY,  new OctetStr(""))
{
}

OctetStr ItcMemoryUsage::getMemoryUsage()
{
    MemInfo_S stMemInfo;
    get_mem_info(&stMemInfo);
    double dUsed = calculate_usage(&stMemInfo);
    std::string strMemoryUsage =  std::to_string(dUsed) + "%";;
    dlog_info("内存使用率：%s",strMemoryUsage.c_str());
    return OctetStr(strMemoryUsage.c_str());
}

void ItcMemoryUsage::get_request(Request* req, int ind)
{
     /* 获取内存大小并设置到 MibLeaf */ 
    *((OctetStr*)value) = getMemoryUsage();
    MibLeaf::get_request(req, ind);
}

/**********************************************************************
 *
 *  class ItcDeviceLanguage 设备语言 只读
 *
 **********************************************************************/
ItcDeviceLanguage::ItcDeviceLanguage(): MibLeaf(OID_ITC_DEVICELANGUAGE, READONLY,  new OctetStr(""))
{
}

OctetStr ItcDeviceLanguage::getDeviceLanguage()
{
    std::string strDeviceLanguage = "Chinese";
    return OctetStr(strDeviceLanguage.c_str());
}

void ItcDeviceLanguage::get_request(Request* req, int ind)
{
     /* 获取设备语言并设置到 MibLeaf */ 
    *((OctetStr*)value) = getDeviceLanguage();
    MibLeaf::get_request(req, ind);
}

/**********************************************************************
 *
 *  class ItcDiskNum 硬盘个数 只读
 *
 **********************************************************************/
ItcDiskNum::ItcDiskNum(): MibLeaf(OID_ITC_DISKNUM, READONLY,  new SnmpInt32(0))
{
}

SnmpInt32 ItcDiskNum::getDiskNum()
{
    // SnmpInt32 nDiskNum =  CSnmpManage::instance()->getDeviceInfo().nHardDriveCount;
    // return nDiskNum;
    return 1;
}

void ItcDiskNum::get_request(Request* req, int ind)
{
     /* 获取设备硬盘个数并设置到 MibLeaf */ 
    *((SnmpInt32*)value) = getDiskNum();
    MibLeaf::get_request(req, ind);
}


/**********************************************************************
 *
 *  class ItcDiskEntry 硬盘信息入口 只读
 *
 **********************************************************************/
ItcDiskEntry::ItcDiskEntry(): MibLeaf(OID_ITC_DISKENTRY, READONLY,  new OctetStr(""))
{
}

OctetStr ItcDiskEntry::getDiskEntry()
{
    std::string strDiskEntry = "ITC NVR DiskEntry";
    return OctetStr(strDiskEntry.c_str());
}

void ItcDiskEntry::get_request(Request* req, int ind)
{
     /* 获取设备硬盘信息入口并设置到 MibLeaf */ 
    *((OctetStr*)value) = getDiskEntry();
    MibLeaf::get_request(req, ind);
}

/**********************************************************************
 *
 *  class ItcDiskIndex 硬盘索引 只读
 *
 **********************************************************************/
ItcDiskIndex::ItcDiskIndex(): MibLeaf(OID_ITC_DISKINDEX, READONLY,  new OctetStr(""))
{
}

OctetStr ItcDiskIndex::getDiskIndex()
{
    std::string strDiskIndex = "ITC DiskIndex";
    return OctetStr(strDiskIndex.c_str());
}

void ItcDiskIndex::get_request(Request* req, int ind)
{
     /* 获取设备硬盘索引并设置到 MibLeaf */ 
    *((OctetStr*)value) = getDiskIndex();
    MibLeaf::get_request(req, ind);
}


/**********************************************************************
 *
 *  class ItcDiskVolum 硬盘卷名 只读
 *
 **********************************************************************/
ItcDiskVolum::ItcDiskVolum(): MibLeaf(OID_ITC_DISKVOLUM, READONLY,  new OctetStr(""))
{
}

OctetStr ItcDiskVolum::getDiskVolum()
{
    std::string strDiskVolum = "ITC NVR DiskVolum";
    return OctetStr(strDiskVolum.c_str());
}

void ItcDiskVolum::get_request(Request* req, int ind)
{
     /* 获取设备硬盘索引并设置到 MibLeaf */ 
    *((OctetStr*)value) = getDiskVolum();
    MibLeaf::get_request(req, ind);
}

/**********************************************************************
 *
 *  class ItcDiskStatus 硬盘状态 只读
 *
 **********************************************************************/
ItcDiskStatus::ItcDiskStatus(): MibLeaf(OID_ITC_DISKSTATUS, READONLY,  new OctetStr(""))
{
}

OctetStr ItcDiskStatus::getDiskStatus()
{
    std::string strDiskStatus = "ITC NVR DiskStatus";
    return OctetStr(strDiskStatus.c_str());
}

void ItcDiskStatus::get_request(Request* req, int ind)
{
     /* 获取设备硬盘状态并设置到 MibLeaf */ 
    *((OctetStr*)value) = getDiskStatus();
    MibLeaf::get_request(req, ind);
}


/**********************************************************************
 *
 *  class ItcDiskFreeSpac 硬盘剩余容量 只读
 *
 **********************************************************************/
ItcDiskFreeSpace::ItcDiskFreeSpace(): MibLeaf(OID_ITC_DISKFREESPACE, READONLY,  new OctetStr(""))
{
}

OctetStr ItcDiskFreeSpace::getDiskFreeSpace()
{
    double dFreeSize = 1; // CDiskManage::instance()->get_diskFreeCapacity();
    std::string strDiskFreeSpace = std::to_string(dFreeSize) + "GB";
    return OctetStr(strDiskFreeSpace.c_str());
}

void ItcDiskFreeSpace::get_request(Request* req, int ind)
{
     /* 获取设备硬盘剩余容量并设置到 MibLeaf */ 
    *((OctetStr*)value) = getDiskFreeSpace();
    MibLeaf::get_request(req, ind);
}


/**********************************************************************
 *
 *  class ItcDiskCapability 硬盘容量 只读
 *
 **********************************************************************/
ItcDiskCapability::ItcDiskCapability(): MibLeaf(OID_ITC_DISKCAPABILITY, READONLY,  new OctetStr(""))
{
}

OctetStr ItcDiskCapability::getDiskCapability()
{
    double dTotalSize = 1; //CDiskManage::instance()->get_diskTotalCapacity();
    std::string strDiskCapability = std::to_string(dTotalSize) + "GB";
    return OctetStr(strDiskCapability.c_str());
}

void ItcDiskCapability::get_request(Request* req, int ind)
{
     /* 获取设备硬盘容量并设置到 MibLeaf */ 
    *((OctetStr*)value) = getDiskCapability();
    MibLeaf::get_request(req, ind);
}

/**********************************************************************
 *
 *  class nvrGroup NVR管理信息库 
 *
 **********************************************************************/
nvrGroup::nvrGroup():MibGroup(OID_NVR_GROUP, NVR_NAME)
{
    /* 添加获取ip节点 */
    add(new ItcIp());

    /* 添加获取端口节点 */
	add(new ItcPort());

    /* 添加获取序列号节点 */
    add(new ItcEntityIndex());

     /* 添加获取产品类型节点 */
    add(new ItcEntityIndex());

    /* 添加获取产品子类型节点 */
    add(new ItcEntitySubType());

     /* 添加获取设备状态节点 */
    add(new ItcEntitySubType());

    /* 添加获取trap地址节点 */
    add(new ItcTrapHostIp1());

    /* 添加获取内存大小节点 */
    add(new ItcMemoryCapability());

    /* 添加获取设备内存剩余使用率节点 */
    add(new ItcMemoryUsage());

    /* 添加获取设备语言节点 */
    add(new ItcDeviceLanguage());

    /* 添加获取硬盘个数节点 */
    add(new ItcDiskNum());

    /* 添加获取硬盘信息入口节点 */
    add(new ItcDiskEntry());

    /* 添加获取硬盘索引节点 */
    add(new ItcDiskIndex());

    /* 添加获取硬盘卷名节点 */
    add(new ItcDiskVolum());

    /* 添加获取硬盘状态节点 */
    add(new ItcDiskStatus());

    /* 添加获取硬盘剩余容量节点 */
    add(new ItcDiskFreeSpace());

    /* 添加获取硬盘容量节点 */
    add(new ItcDiskCapability());
}