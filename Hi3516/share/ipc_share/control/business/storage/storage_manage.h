/*
 * @Author: 梁浩尧 lianghaoyao@kfb.cn
 * @Date: 2025-08-11 14:26:04
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-05-27 14:09:19
 * @FilePath: /hisi/share/ipc_share/control/business/storage/storage_manage.h
 * @Description: 存储管理
 */
#pragma once

#include <atomic>
#include <string>
#include <spawn.h>
#include <sys/wait.h>
#include <stdexcept>
#include <filesystem>
#include <cstring>
#include <iostream>
#include <vector>
#include <algorithm>
#include "storage_manage_configure.h"
#include "IpcRet.h"

typedef enum _SD_CARD_STATUS_E_
{
    /* 异常 */
    WRITE_ERROR = -1,
    /* 插入 */
    INSERT = 0,
    /* 拔出 */
    UNPLUG = 1,
    /* 格式化中 */
    FORMATING = 2,
    /* 正常工作 */
    NORMAL = 3
} SD_CARD_STATUS_E;

class CStorageManage : public CSingleton<CStorageManage>
{
    CStorageManage();
public:
    virtual ~CStorageManage();
   /* 允许 Singleton 访问私有构造函数 */ 
    friend class CSingleton<CStorageManage>;

public: 
    struct linux_dirent64 {
        uint64_t       d_ino;
        int64_t        d_off;
        unsigned short d_reclen;
        unsigned char  d_type;
        char           d_name[];
    };

    typedef struct 
    {
        uint64_t total_bytes;
        int      errors;
    } DirInfo_t;

public: 
    /**
     * @brief 初始化抓图模块
     * @return IpcRet_E <0:失败, >=0:成功
     */
    IpcRet_E init();
    
    /**
     * @brief 线程函数：检测sd卡插入拔出
     */
    void run();
    
    /**
     * @brief 获取sd卡工作状态
     * @return  sd卡状态
     */
    SD_CARD_STATUS_E get_SdCardStatus();

    /**
     * @brief 初始化 netlink socket，用来监听内核发出的热插拔事件
     * @return  <0:失败, =0:成功
     */
    int init_detect();

    /**
     * @brief 去初始化抓图模块
     * @return IpcRet_E <0:失败, >=0:成功
     */
    IpcRet_E deinit();

    /**
     * @brief   : 检查SD卡设备是否存在
     * @return   {bool} true 存在, false 不存在
     */
    bool sd_card_is_exist();

    /**
     * @brief   : 检查SD卡是否已挂载
     * @return   {bool} true 存在, false 不存在
     */
    bool sd_card_is_mounted();

    /**
     * @brief   : 获取挂载点空间大小
     * @param    {unsigned long long} &llTotalSize：挂载点空间大小
     * @param    {string} &strMountPoint：挂载点路径
     * @return   {int} 成功：0  失败：小于零
     */
    int get_fs_size(unsigned long long &llTotalSize, const std::string &strMountPoint);

    /**
     * @brief   : 获取挂载点已使用空间大小
     * @param    {unsigned long long} &llUseSize：挂载点已使用空间大小
     * @param    {string} &strMountPoint：挂载点路径
     * @return   {int} 成功：0  失败：小于零
     */
    int get_fs_usage(unsigned long long &llUseSize, const std::string &strMountPoint);

    /**
     * @brief   : 获取指定目录大小
     * @param    {unsigned long long} &llUseSize：指定目录大小
     * @param    {string} &strMountPoint：目录路径
     * @return   {int} 成功：0  失败：小于零
     */
    int get_directory_size(long long &llSize, const std::string &strPath);
    // void get_directory_size(int dfd, const char *name, int depth, DirInfo_t *ctx);
    // void accumulate_dir_bytes (int dfd, const char *name, int depth, DirInfo_t *ctx);

    /**
     * @brief   : 计算录制天数
     * @param    {float} fRecordSpace：录制分配空间大小
     * @return   {int} 成功：大于0  失败：小于0
     */
    int calculateRecordingTime(float fRecordSpace);

    /**
     * @brief   : 计算存储管理参数信息
     * @return   {int} 成功：大于0  失败：小于0
     */
    int calculate_storageManage_param();

    /**
     * @brief   : 获取存储管理信息
     * @param    {StorageManage_S} &stStorageManageParam：存储管理信息
     * @return   {int} 成功：0  失败：小于零
     */
    int get_storageManage_param(StorageManage_NS::StorageManage_S &stStorageManageParam);

    /**
     * @brief   : 设置存储管理信息
     * @param    {StorageManage_S} &stStorageManageParam：存储管理信息
     * @return   {int} 成功：0  失败：小于零
     */
    int update_storageManage_param(StorageManage_NS::StorageManage_S &stStorageManageParam);
    
    /**
     * @brief   : 获取抓图目录使用状态
     * @return   {int} 空间未达到设定大小：0 空间达到设定大小：-1
     */
    int get_captureDirUseStatus();

    /**
     * @brief   : 获取录制目录使用状态
     * @return   {int} 空间未达到设定大小：0 空间达到设定大小：-1
     */
    int get_recordDirUseStatus();

    bool getAutoFormatSdCardFlag();

    /**
     * @brief   : 更新实际的录制目录和抓图目录大小到数据库
     * @return   {int} 成功：返回0，失败：其他
     */
    int update_DatabaseDirUseSize();
    
        /**
     * @brief   : 格式化sd卡
     * @return   {int} 成功：返回0，失败：其他
     */
    int format_sd_card(bool bIsInitSdCard);

private:

    /**
     * @brief   : 判断是否是sd卡事件
     * @param    {const char *buf} buf：检测到事件消息
     * @param    {int} add：检测是sd卡插入还是拔出
     * @return   {bool} 成功：true，失败：false
     */
    bool is_sd_event(const char *buf, int *add);

    /**
     * @brief   : 获取 SD 卡 UUID，失败返回空串
     * @param    {const std::string&} dev：sd卡设备节点路径
     * @return   {int} 成功：返回sd卡uuid  失败：返回空
     */
    std::string get_sd_uuid(const std::string& dev = "/dev/mmcblk0p1");

    /**
     * @brief   : 测试sd卡写数据是否正常
     * @return   {bool} 成功：返回true  失败：返回false
     */
    bool test_write_operation();

private:
    /*是否停止检测sd线程函数*/
    std::atomic_bool m_bRun = false;
    /* 当前sd卡状态 */
    SD_CARD_STATUS_E m_SdCardStatus = UNPLUG;
    // /* sd卡uuid */
    // std::string m_strSdCardUuid; 
    /* netlink 套接字的文件描述符 */
    int m_nSock = -1;
    /* 储存管理参数 */
    StorageManage_NS::StorageManage_S m_stStorageManageParam;
    /*用于保护共享资源的互斥锁*/
    std::mutex m_mutex;
    long long m_llCaptureSpaceByte = 0;
    /* 分配给录像的可用空间 */
    long long m_llRecordSpaceByte = 0;
    /* 抓图已使用空间 */
    long long m_llCaptureDirUseSize = 0;
    /* 录像已使用空间 */
    long long m_llRecordDirUseSize = 0;
};