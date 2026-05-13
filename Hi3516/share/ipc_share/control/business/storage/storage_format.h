#ifndef STORAGE_FORMAT_H
#define STORAGE_FORMAT_H

#include <iostream>
#include <string>
#include <set>
#include <fstream>
#include <sys/inotify.h>
#include <fcntl.h>
#include <dirent.h>
#include <cstring>
#include <ctime>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>
#include <memory>
#include <regex>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <sstream>
#include <map>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <errno.h>
#include <limits.h>
#include <chrono>
#include "Singleton.h"
#include "storage_manage_define.h"

// 自定义异常类
class StorageMonitorException : public std::runtime_error 
{
public:
    StorageMonitorException(const std::string &message) : std::runtime_error(message) {}
};


using DeviceMap     = std::map<std::string, StorageManage_NS::DeviceInfo_S>;

class StorageFormat : public CSingleton<StorageFormat>
{

private:
    StorageFormat();

public:
    ~StorageFormat();
    /* 允许 Singleton 访问私有构造函数 */ 
    friend class CSingleton<StorageFormat>;

    /**
     * @brief   : 初始化存储设备检测
     * @return   {int} 成功：0  失败：小于零
     */
    int init();

    /**
     * @brief   : 去初始化存储设备检测
     * @return   {int} 成功：0  失败：小于零
     */
    int deinit();

    /**
     * @brief   : 启动监测存储设备热插拔线程
     * @return   {int} 成功：0  失败：小于零
     */
    bool start();

    /**
     * @brief   : 停止监测存储设备热插拔线程
     * @return   {int} 成功：0  失败：小于零
     */
    void stop();
    
    /**
     * @brief   : 获取当前已知设备列表
     * @return   {DeviceMap} 当前已知设备列表及信息
     */
    DeviceMap getDevices();

private:
    
    /**
     * @brief   : 初始化inotify监控
     * @return   {bool} true 成功, false 失败
     */
    bool initInotify();

    /**
     * @brief   : 验证存储设备名称是否安全
     * @param    {string} &strDevice：设备名称
     * @return   {bool} true 成功, false 失败
     */
    bool isValidDeviceName(const std::string &strDevice);

    /**
     * @brief   : 检查存储设备是否为存储卡
     * @param    {string} &strDevice：设备名称
     * @return   {bool} true 成功, false 失败
     */
    bool isStorageCard(const std::string &strDevice);

    /**
     * @brief   : 检查存储设备是否可移动
     * @param    {string} &strDevice：设备名称
     * @return   {bool} true 成功, false 失败
     */
    bool isRemovableDevice(const std::string &strDevice);
    
    /**
     * @brief   : 获取存储设备大小
     * @param    {string} &strDevice：设备名称     
     * @return   {uint64_t} 成功：返回存储设备大小, 0 失败
     */
    uint64_t getDeviceSize(const std::string &strDevice);

    /**
     * @brief   : 获取存储设备型号信息
     * @param    {string} &strDevice：设备名称     
     * @return   {string} 设备型号信息
     */
    std::string getDeviceModel(const std::string &strDevice);

    /**
     * @brief   : 获取存储设备序列号
     * @param    {string} &strDevice：设备名称     
     * @return   {string} 设备型号信息
     */
    std::string getDeviceSerial(const std::string &strDevice);

    /**
     * @brief   : 检查存储设备是否已格式化
     * @param    {string} &strDevice：设备名称     
     * @return   {string} 设备文件系统信息
     */
    std::string detectFilesystem(const std::string &strDevice);

    /**
     * @brief   : 等待存储设备就绪
     * @param    {string} &strDevice：设备名称     
     * @return   {string} 设备文件系统信息
     */
    bool waitForDeviceReady(const std::string &strDevice);

    /**
     * @brief   : 执行命令
     * @param    {vector<std::string>} &args：要执行的命令
     * @return   {int} 成功：> 0  失败：< 0
     */
    int executeCommand(const std::vector<std::string> &args);

    /**
     * @brief   : 格式化存储卡
     * @param    {string} &strDevice：设备名称     
     * @return   {bool} true 成功, false 失败
     */
    bool formatStorageCard(const std::string &strDevice);

    /**
     * @brief   : 检查分区表是否存在
     * @param    {string} &strDevice：设备名称     
     * @return   {bool} true 成功, false 失败
     */
    bool hasPartitionTable(const std::string& strDevice);

    /**
     * @brief   : 自动选择文件系统类型
     * @param    {uint64_t} size：存储设备大小    
     * @return   {string} 设备文件系统类型
     */
    std::string selectFilesystemType(uint64_t size); 

    /**
     * @brief   : 创建分区并格式化
     * @param    {string} &strDevice：设备名称     
     * @return   {bool} true 成功, false 失败
     */
    bool createPartitionAndFormat(const std::string& strDevice);

    /**
     * @brief   : 处理新设备
     * @param    {string} &strDevice：设备名称     
     * @return 
     */
    void processNewDevice(const std::string &strDevice);

    /**
     * @brief   : 获取当前设备列表
     * @return   {DeviceMap} 当前设备列表及信息
     */
    DeviceMap getCurrentDevices();

    /**
     * @brief   : 轮询检查设备变化
     * @return   
     */
    void pollDevices();

    /**
     * @brief   : 监测循环
     * @return   
     */
    void run();

    /**
     * @brief   : 使用inotify监控存储目录（如 /dev）中新设备的创建与删除事件
     * @return   
     */
    void monitorWithInotify();

    /**
     * @brief   : 使用轮询监控存储目录（如 /dev）中新设备的创建与删除事件
     * @return   
     */
    void monitorWithPolling();

private:
    /* 配置参数 */
    StorageManage_NS::StorageConfig_S m_config;
    /* inotify实例返回的文件描述符 */
    StorageManage_NS::FileDescriptor m_inotifyFd;
    /* 标识路径的描述符 */
    int m_watchFd = -1;
    /* 已知设备的的名字与设备信息映射 */
    DeviceMap m_knownDevices;
    /* 线程运行标志 */
    std::atomic<bool> m_bRunning = false;
    /* 检测插拔sd卡线程 */
    std::thread m_monitorThread;
    /* 用于保护共享资源的互斥锁 */
    std::mutex devicesMutex;

};

#endif