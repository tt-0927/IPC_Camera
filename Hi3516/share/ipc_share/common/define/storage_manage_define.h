/*
 * @Author: 梁浩尧 lianghaoyao@kfb.cn
 * @Date: 2025-08-11 10:25:37
 * @LastEditors: 梁浩尧 lianghaoyao@kfb.cn
 * @LastEditTime: 2025-12-25 09:57:36
 * @FilePath: /hisi/share/ipc_share/common/define/storage_define.h
 * @Description: 存储管理定义
 */
#pragma once

#include <string>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sstream>
#include <iomanip>

namespace StorageManage_NS 
{
    /* SD 卡未插入状态值，与 SD_CARD_STATUS_E::UNPLUG 保持一致。 */
    static constexpr int STORAGE_MANAGE_SD_CARD_STATUS_UNPLUGGED = 1;

        /* 存储管理结构体 */
    typedef struct StorageManage
	{
        /* 是否启用 */
        bool bEnable = false;
        /* 总可用空间 */
        std::string strAvailableSpace = std::string();
        /* 图片容量 */
        std::string strCaptureSpace = std::string();
        /* 图片剩余空间 */
        std::string strCaptureRemainingSpace = std::string();
        /* 录像容量 */
        std::string strRecordSpace = std::string();
        /* 录像剩余空间 */
        std::string strRecordRemainingSpace = std::string();
        /* 抓图配额百分比 */
        int nCaptureQuotaPercentage = 10;
        /* 录像配额百分比 */
        #if CAP_AI_FACE_COMPARE
        int nRecordQuotaPercentage = 80;
        #else
        int nRecordQuotaPercentage = 90;
        #endif
        /* 存储时间 */
        int nStorageTime = 0;

        // std::string strSdCardUuid = std::string();

        void clear()
        {
            strAvailableSpace = std::string();
            strCaptureSpace = std::string();
            strCaptureRemainingSpace = std::string();
            strRecordSpace = std::string();
            strRecordRemainingSpace = std::string();
            nStorageTime = 0;
            return  ;;
        }

	} StorageManage_S;

    /**
     * @brief SD 卡当前状态信息
     * @note  nStatus 对应 CStorageManage 的 SD_CARD_STATUS_E 枚举值
     */
    typedef struct SdCardStatus
    {
        int nStatus = STORAGE_MANAGE_SD_CARD_STATUS_UNPLUGGED;
        std::string strStatusText = "unplugged";
        bool bReady = false;
    } SdCardStatus_S;

    typedef struct DirInfo
    {
        int nFileCount = 0;
        long long llDirTotalSize = 0;
    }DirInfo_S;

    /* RAII包装器用于文件描述符 */
    class FileDescriptor 
    {
    public:
        explicit FileDescriptor(int fd = -1) : fd_(fd) {}

        ~FileDescriptor()
        {
            if (fd_ >= 0)
            {
                close(fd_);
            }
        }

        /* 禁用拷贝 */
        FileDescriptor(const FileDescriptor &)            = delete;
        FileDescriptor &operator=(const FileDescriptor &) = delete;

        /* 允许移动 */
        FileDescriptor(FileDescriptor &&other) noexcept : fd_(other.fd_)
        {
            other.fd_ = -1;
        }

        FileDescriptor &operator=(FileDescriptor &&other) noexcept
        {
            if (this != &other)
            {
                if (fd_ >= 0)
                {
                    close(fd_);
                }
                fd_       = other.fd_;
                other.fd_ = -1;
            }
            return *this;
        }

        int get() const { return fd_; }
        operator int() const { return fd_; }

        void reset(int fd = -1)
        {
            if (fd_ >= 0)
            {
                close(fd_);
            }
            fd_ = fd;
        }

    private:
        int fd_;
    };

    /* 配置结构 */
    typedef struct StorageConfig
    {
        int nPollInterval = 30;                                  // 轮询间隔(秒)
        int nFormatDelay = 2;                                    // 设备就绪等待时间(秒)
        bool bEnableInotify = true;                              // 是否启用inotify监控
        bool bEnableAutoFormat = true;                           // 是否启用自动格式化
        std::string strFormatCommand = "/sbin/mkfs.vfat -F32";   // 格式化命令
        std::string strDeviceBasePath = "/dev";                  // 设备基础路径
        std::vector<std::string> strDevicePatterns = {"mmcblk", "sd"};   // 设备匹配模式

        // 验证配置有效性
        bool validate(std::string &errorMessage) const
        {
            if (nPollInterval <= 0)
            {
                errorMessage = "PollInterval must be positive";
                return false;
            }
            if (nFormatDelay <= 0)
            {
                errorMessage = "FormatDelay must be positive";
                return false;
            }
            if (strDeviceBasePath.empty())
            {
                errorMessage = "DeviceBasePath cannot be empty";
                return false;
            }
            if (strDevicePatterns.empty())
            {
                errorMessage = "devicePatterns cannot be empty";
                return false;
            }

            // 检查设备基础路径是否存在
            struct stat st;
            if (stat(strDeviceBasePath.c_str(), &st) != 0 || !S_ISDIR(st.st_mode))
            {
                errorMessage = "deviceBasePath does not exist or is not a directory";
                return false;
            }

            return true;
        }
    }StorageConfig_S;

    /* 设备信息结构 */
    typedef struct DeviceInfo
    {
        std::string name;
        std::string path;
        uint64_t    size;
        std::string filesystem;
        bool        isRemovable;
        bool        isFormatted;
        time_t      detectedTime;
        std::string model;   // 设备型号
        std::string serial;  // 序列号

        DeviceInfo() : size(0), isRemovable(false), isFormatted(false), detectedTime(0) {}

        std::string toString() const
        {
            std::ostringstream oss;
            oss << "Device: " << name
                << ", Path: " << path
                << ", Size: " << std::fixed << std::setprecision(2)
                << (size / (1024.0 * 1024.0 * 1024.0)) << "GB"
                << ", Filesystem: " << (filesystem.empty() ? "None" : filesystem)
                << ", Removable: " << (isRemovable ? "Yes" : "No")
                << ", Formatted: " << (isFormatted ? "Yes" : "No");
            if (!model.empty())
            {
                oss << ", Model: " << model;
            }
            return oss.str();
        }

        // 添加比较运算符，便于在容器中使用
        bool operator<(const DeviceInfo &other) const
        {
            return path < other.path;
        }
    }DeviceInfo_S;
}