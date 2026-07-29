/*
 * @Author: 梁浩尧 lianghaoyao@kfb.cn
 * @Date: 2025-08-11 14:26:15
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-05-27 14:09:29
 * @FilePath: /hisi/share/ipc_share/control/business/storage/storage_manage.cpp
 * @Description: 存储管理
 */
#include <cmath>
#include <glob.h>
#include <mntent.h>
#include <filesystem>
#include "av_configure.h"
#include <sys/statvfs.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/syscall.h>
#include "storage_manage.h"
// #include "storage_format.h"
#include "record_ctrl.h"
#include "record_file_manage.h"
#include "record_file_database.h"
#include "capture_ctrl.h"
#include "capture_database.h"
#include "event_database.h"
#include "event_manage.h"
// namespace fs = std::filesystem;
static void clear_face_database_after_sd_format()
{
#if CAP_AI_FACE_COMPARE
    int nAlgoRet = -1;
    const int nSendRet = CEventManage::instance()->send_algo_controlData(
        AC_CLEAR_FACE_DATABASE, "{}", &nAlgoRet);
    if ((nSendRet != 0) || (nAlgoRet != 0))
    {
        dlog_error("clear face database after SD format failed, sendRet=%d, algoRet=%d",
                   nSendRet, nAlgoRet);
        return;
    }
    dlog_info("face persons and feature data cleared after SD format");
#endif
}

#if CAP_STORAGE_MMCBLK1  // 存储 mmcblk1 路径逻辑

/* sd卡挂载/卸载脚本 */
#define SD_CARD_MOUNT_REMOVE_SCRIPT_PATH "/etc/udev/scripts/mount_device"

// #define SD_CARD_FORMAT_COMMAND "mkfs.vfat -F 32 -n CAM_SD /dev/mmcblk0p1"
// #define SD_CARD_FORMAT_COMMAND           "mkfs.exfat -n CAM_SD /dev/mmcblk1p1"

#else

/* sd卡卸载脚本 */
#define SD_CARD_REMOVE_SCRIPT_PATH "/etc/scripts/remove_device"
/* sd卡挂载脚本 */
#define SD_CARD_MOUNT_SCRIPT_PATH  "/etc/scripts/mount_device"

// #define SD_CARD_FORMAT_COMMAND "mkfs.vfat -F 32 -n CAM_SD /dev/mmcblk0p1"
// #define SD_CARD_FORMAT_COMMAND     "mkfs.exfat -n CAM_SD /dev/mmcblk0p1"

#endif

// Linux 挂载表路径（用于获取挂载点对应设备）
constexpr const char *PROC_SELF_MOUNTS_PATH = "/proc/self/mounts";

/* Nginx master 进程 pid 文件完整路径 */
#define NGINX_PID_FILE_PATH               (THIRD_PATRY_PATH "nginx/logs/nginx.pid")

/* 进行删除录像上限阈值 */
#define DISK_DEL_RECORD_DEFAULT_VALUE_UP  0.99

/* 进行删除抓图上限阈值 */
#define DISK_DEL_CAPTURE_DEFAULT_VALUE_UP 0.98

#define MAX_DEPTH                         40

#define PROBE_FILE                        ".sd_guardian_probe"

CStorageManage::CStorageManage()
{
}

CStorageManage::~CStorageManage()
{
}

IpcRet_E CStorageManage::init()
{
    int nRet = -1;
    CStorageManageConfigure::instance()->get_configure(m_stStorageManageParam);

    // StorageFormat::instance()->init();
    nRet = init_detect();
    if (nRet < 0)
    {
        dlog_error("init_detect error");
        return ERR;
    }

    m_bRun.store(true, std::memory_order_release);
    std::thread tid;
    tid = std::thread(&CStorageManage::run, this);
    tid.detach();

    return OK;
}

IpcRet_E CStorageManage::deinit()
{
    // StorageFormat::instance()->deinit();
    close(m_nSock);
    return OK;
}

int CStorageManage::init_detect()
{
    /* 监听 multicast group 1 (kernel) */
    struct sockaddr_nl nls = {.nl_family = AF_NETLINK, .nl_groups = 1};

    m_nSock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    if (m_nSock < 0)
    {
        perror("socket");
        return -1;
    }

    // 设置 m_nSock 为非阻塞
    int flags = fcntl(m_nSock, F_GETFL, 0);
    fcntl(m_nSock, F_SETFL, flags | O_NONBLOCK);

    if (bind(m_nSock, (struct sockaddr *)&nls, sizeof(nls)) < 0)
    {
        perror("bind");
        close(m_nSock);
        return -1;
    }
    return 0;
}

/**
 * @brief 检查SD卡设备是否存在
 * @return true 存在, false 不存在
 */
bool CStorageManage::sd_card_is_exist()
{
    glob_t globbuf;
    bool   bStatus = false;

    // 使用glob函数更安全地查找设备
    if (glob("/dev/mmcblk[0-9]*", GLOB_NOSORT, NULL, &globbuf) == 0)
    {
        if (globbuf.gl_pathc > 0)
        {
            bStatus = true;  // 找到SD卡设备
        }
        globfree(&globbuf);  // 释放glob分配的内存
    }

    return bStatus;
}

/**
 * @brief 检查SD卡是否已挂载
 * @return true 已挂载, false 未挂载
 */
bool CStorageManage::sd_card_is_mounted()
{
    FILE          *fp;
    struct mntent *ent;
    bool           bStatus = false;

    // 打开挂载信息文件
    fp = setmntent("/proc/mounts", "r");
    if (fp == NULL)
    {
        perror("setmntent");
        return false;
    }

    // 遍历所有挂载点，查找SD卡设备
    while ((ent = getmntent(fp)) != NULL)
    {
        if (strstr(ent->mnt_fsname, "/dev/mmcblk") != NULL)
        {
            bStatus = true;  // 找到SD卡挂载点
            break;
        }
    }

    endmntent(fp);
    return bStatus;
}

int CStorageManage::get_fs_size(unsigned long long &llTotalSize, const std::string &strMountPoint)
{
    struct statvfs stVfs;

    if (statvfs(strMountPoint.c_str(), &stVfs) != 0)
    {
        dlog_error("statvfs failed");
        return -1;
    }

    llTotalSize = (unsigned long long)stVfs.f_blocks * stVfs.f_frsize;

    return 0;
}

int CStorageManage::get_fs_usage(unsigned long long &llUseSize, const std::string &strMountPoint)
{
    struct statvfs stVfs;

    if (statvfs(strMountPoint.c_str(), &stVfs) != 0)
    {
        dlog_error("statvfs failed");
        return -1;
    }

    llUseSize = ((unsigned long long)stVfs.f_blocks - (unsigned long long)stVfs.f_bavail) * stVfs.f_frsize;

    return 0;
}
#if 1
int CStorageManage::get_directory_size(long long &llSize, const std::string &strPath)
{
    //  -L （跟随符号链接）
    std::string strCommand = "du -sbL \"" + strPath + "\" 2>/dev/null";

    FILE *pipe = popen(strCommand.c_str(), "r");
    if (!pipe)
    {
        dlog_error("Failed to execute command");
        return -1;
    }

    if (fscanf(pipe, "%lld", &llSize) != 1)
    {
        pclose(pipe);
        return -1;
    }

    pclose(pipe);
    return 0;
}
#endif
#if 0
void CStorageManage::accumulate_dir_bytes(int dfd, const char *name, int depth, DirInfo_t *ctx)
{
    if (depth > MAX_DEPTH)
    {
        dlog_error("depth limit at %s", name);
        return;
    }

    int fd = openat(dfd, name, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (fd < 0)
    {
        dlog_error("openat %s: %s", name, strerror(errno));
        ctx->errors++;
        return;
    }

    DIR *d = fdopendir(fd);
    if (!d)
    {
        dlog_error("fdopendir %s: %s", name, strerror(errno));
        close(fd);
        ctx->errors++;
        return;
    }

    struct dirent *de;
    int            nEntries = 0;
    while ((de = readdir(d)))
    {
        if (de->d_name[0] == '.')
        {
            continue;
        }

        struct stat st;
        /* 跟随符号链接 */
        if (fstatat(fd, de->d_name, &st, 0))
        {
            dlog_error("fstatat %s/%s: %s", name, de->d_name, strerror(errno));
            ctx->errors++;
            continue;
        }

        if (S_ISREG(st.st_mode))
        {
            ctx->total_bytes += st.st_size;
        }
        else if (S_ISDIR(st.st_mode))
        {
            accumulate_dir_bytes(fd, de->d_name, depth + 1, ctx);
        }

        /* 每检测2个目录让出 CPU 10 ms */
        if (++nEntries >= 2)
        {
            usleep(10 * 1000);
            nEntries = 0;
        }
    }
    closedir(d); /* 同时 close(fd) */
    return;
}

void CStorageManage::get_directory_size(int dfd, const char *name, int depth, DirInfo_t *ctx)
{
    struct stat st;
    if (fstatat(dfd, name, &st, 0))
    {
        /* 跟随链接 */
        fprintf(stderr, "fstatat %s: %s\n", name, strerror(errno));
        ctx->errors++;
        return;
    }

    if (S_ISREG(st.st_mode))
    {
        ctx->total_bytes += st.st_size;
    }
    else if (S_ISDIR(st.st_mode))
    {
        accumulate_dir_bytes(dfd, name, 0, ctx);
    }

    return;
}
#endif
// 计算录像时长（天）
int CStorageManage::calculateRecordingTime(float fRecordSpace)
{
    std::set<Video_NS::VideoConfig_S> stVideoConfigs;
    Audio_NS::AudioConfig_S           stAudioConfig;

    CAVConfigure::instance()->get_configure(stVideoConfigs);

    CAVConfigure::instance()->get_configure(stAudioConfig);

    /* 获取录制码流视频配置 */
    auto stVideoConfig = std::next(stVideoConfigs.begin(), 1);

    // 1. 确定视频有效码率（kbps）
    int nVideoBitrate = 0;
    if (stVideoConfig->enBitrateType == Video_NS::BitrateType_E::CBR)
    {
        nVideoBitrate = stVideoConfig->nBitrateUpperLimit;
    }
    else
    {
        nVideoBitrate = stVideoConfig->nAverageBitrate;
    }

    // 2. 计算音频实际码率（kbps）
    int nAudioBitrate = 0;

    if (stAudioConfig.bAudioSwitch)
    {
        // 转换单位 bps -> kbps
        nAudioBitrate = (int)stAudioConfig.enBitRate / 1000;
    }

    // 3. 计算总码率（kbps）
    int nTotalBitrate = nVideoBitrate + nAudioBitrate;

    // 4. 计算每天数据量（GB）
    int nBytesPerDay = (nTotalBitrate / 8) * 24 * 3600 / 1024 / 1024;

    // 5. 计算存储多少天
    int nStorageDays = fRecordSpace / nBytesPerDay;

    return nStorageDays;
}

int CStorageManage::calculate_storageManage_param()
{
    int nRet = -1;

    if (!sd_card_is_exist())
    {
        m_stStorageManageParam.clear();
        dlog_error("sd card not exit");
        return -1;
    }

    if (!sd_card_is_mounted())
    {
        m_stStorageManageParam.clear();
        dlog_error("sd card not mount");
        return -1;
    }

    CStorageManageConfigure::instance()->get_configure(m_stStorageManageParam);

    unsigned long long llTotalSize;
    unsigned long long llUseSize;
    unsigned long long llOtherDirUseSize;

    std::stringstream ss;

    nRet = get_fs_size(llTotalSize, SD_CARD_MOUNT_PATH);
    if (nRet < 0)
    {
        dlog_error("获取sd卡容量失败");
        return -1;
    }
    /* 获取sd卡总空间大小 */
    float fAvailableSpace;
    fAvailableSpace = static_cast<double>(llTotalSize) / (1024.0 * 1024.0 * 1024.0);

    ss << std::fixed << std::setprecision(2) << fAvailableSpace;
    m_stStorageManageParam.strAvailableSpace = ss.str();

    /* 获取sd卡已使用空间大小 */
    get_fs_usage(llUseSize, SD_CARD_MOUNT_PATH);

    Record_NS::RecordDirInfo_S stRecordDirInfo;
    stRecordDirInfo.nChnId = 0;
    nRet                   = RecordFileDatabase::instance()->get_itemInfo(stRecordDirInfo);
    m_llRecordDirUseSize   = stRecordDirInfo.nTotalSize;

    Capture_NS::CaptureDirInfo_S stCaptureDirInfo;
    stCaptureDirInfo.nChnId = 0;
    nRet                    = CCaptureDatabase::instance()->get_itemInfo(stCaptureDirInfo);
    m_llCaptureDirUseSize   = stCaptureDirInfo.nTotalSize;

    #if CAP_AI_FACE_COMPARE
    llOtherDirUseSize = llUseSize - m_llRecordDirUseSize - m_llCaptureDirUseSize - m_llFaceDirUseSize;
    #else
    llOtherDirUseSize = llUseSize - m_llRecordDirUseSize - m_llCaptureDirUseSize;
    #endif
    /* 计算图片配置空间大小，单位GB */
    float fCaptureSpaceGb;


    m_llCaptureSpaceByte = static_cast<long long>((llTotalSize - llOtherDirUseSize) * (m_stStorageManageParam.nCaptureQuotaPercentage / 100.0));
    fCaptureSpaceGb      = static_cast<float>(m_llCaptureSpaceByte / (1024.0 * 1024 * 1024));
    ss.str("");
    ss.clear();
    ss << std::fixed << std::setprecision(2) << fCaptureSpaceGb;
    m_stStorageManageParam.strCaptureSpace = ss.str();

    /* 计算录像配置空间大小，单位GB */
    float fRecordSpaceGb;
    ss.str("");
    ss.clear();
//     #if CAP_AI_FACE_COMPARE

//     const double fFaceRatio = FACE_QUOTA_PERCENT / 100.0;

//     m_llFaceSpaceByte = static_cast<long long>((llTotalSize - llOtherDirUseSize) * fFaceRatio);

//     m_llRecordSpaceByte = static_cast<long long>((llTotalSize - llOtherDirUseSize) *
//                           ((m_stStorageManageParam.nRecordQuotaPercentage - FACE_QUOTA_PERCENT) / 100.0));


// #else
    m_llRecordSpaceByte = static_cast<long long>((llTotalSize - llOtherDirUseSize) * (m_stStorageManageParam.nRecordQuotaPercentage / 100.0));
// #endif
    fRecordSpaceGb      = static_cast<float>(m_llRecordSpaceByte / (1024.0 * 1024 * 1024));
    ss << std::fixed << std::setprecision(2) << fRecordSpaceGb;
    m_stStorageManageParam.strRecordSpace = ss.str();

    /* 计算图片剩余空间大小，单位GB */
    float fCaptureRemainingSpace;
    ss.str("");
    ss.clear();
    fCaptureRemainingSpace = fCaptureSpaceGb - static_cast<double>(m_llCaptureDirUseSize) / (1024.0 * 1024.0 * 1024.0);
    if (fCaptureRemainingSpace < 0)
    {
        fCaptureRemainingSpace = 0.0;
    }
    ss << std::fixed << std::setprecision(2) << fCaptureRemainingSpace;
    m_stStorageManageParam.strCaptureRemainingSpace = ss.str();

    /* 计算录像剩余空间大小，单位GB */
    float fRecordRemainingSpace;
    ss.str("");
    ss.clear();
    fRecordRemainingSpace = fRecordSpaceGb - static_cast<double>(m_llRecordDirUseSize) / (1024.0 * 1024.0 * 1024.0);
    if (fRecordRemainingSpace < 0)
    {
        fRecordRemainingSpace = 0.0;
    }
    ss << std::fixed << std::setprecision(2) << fRecordRemainingSpace;
    m_stStorageManageParam.strRecordRemainingSpace = ss.str();

    m_stStorageManageParam.nStorageTime = calculateRecordingTime(fRecordSpaceGb);

    CStorageManageConfigure::instance()->set_configure(m_stStorageManageParam);

    return 0;
}

int CStorageManage::get_storageManage_param(StorageManage_NS::StorageManage_S &stStorageManageParam)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    calculate_storageManage_param();
    stStorageManageParam = m_stStorageManageParam;

    return 0;
}

int CStorageManage::update_storageManage_param(StorageManage_NS::StorageManage_S &stStorageManageParam)
{
    int nRet = -1;

    std::lock_guard<std::mutex> lock(m_mutex);

    CStorageManageConfigure::instance()->set_configure(stStorageManageParam);

    m_stStorageManageParam.bEnable                 = stStorageManageParam.bEnable;
    m_stStorageManageParam.nRecordQuotaPercentage  = stStorageManageParam.nRecordQuotaPercentage;
    m_stStorageManageParam.nCaptureQuotaPercentage = stStorageManageParam.nCaptureQuotaPercentage;

    nRet = calculate_storageManage_param();

    return nRet;
}

int CStorageManage::get_captureDirUseStatus()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    Capture_NS::CaptureDirInfo_S stCaptureDirInfo;
    stCaptureDirInfo.nChnId = 0;
    int nRet                = OK;
    nRet                    = CCaptureDatabase::instance()->get_itemInfo(stCaptureDirInfo);
    if (nRet == OK)
    {
        m_llCaptureDirUseSize = stCaptureDirInfo.nTotalSize;
    }

    /* 对比抓图占用空间是否达到了设定的额定空间 */
    if (m_llCaptureSpaceByte * DISK_DEL_CAPTURE_DEFAULT_VALUE_UP <= m_llCaptureDirUseSize)
    {
        return ERR;
    }

    return OK;
}

int CStorageManage::get_recordDirUseStatus()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    Record_NS::RecordDirInfo_S stRecordDirInfo;
    stRecordDirInfo.nChnId = 0;
    RecordFileDatabase::instance()->get_itemInfo(stRecordDirInfo);
    m_llRecordDirUseSize = stRecordDirInfo.nTotalSize;

    /* 对比录制占用空间是否达到了设定的额定空间 */
    if (m_llRecordSpaceByte * DISK_DEL_RECORD_DEFAULT_VALUE_UP <= m_llRecordDirUseSize)
    {
        return -1;
    }

    return 0;
}

bool CStorageManage::getAutoFormatSdCardFlag()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stStorageManageParam.bEnable;
}

bool CStorageManage::is_sd_event(const char *pBuf, int *add)
{
    /* 第一行格式: action@/path/to/device */
    if (strncmp(pBuf, "add@", 4) == 0)
    {
        *add = 1;
    }
    else if (strncmp(pBuf, "remove@", 7) == 0)
    {
        *add = 0;
    }
    else
    {
        /* 不是 add/remove 事件 */
        return false;
    }

    /* 查找 /block/mmcblkXpY 路径 */
    const char *p = strstr(pBuf, "/block/mmcblk");
    if (!p)
    {
        /* 路径中不包含 /block/mmcblk */
        return false;
    }

    /* 匹配 /block/mmcblk[0-9]p[0-9] 或 /block/mmcblk[0-9]/mmcblk[0-9]p[0-9] */
    char major[2], major1[2], minor[2];

    // 情况1：/block/mmcblk0p1
    if (sscanf(p, "/block/mmcblk%1[0-9]p%1[0-9]", major, minor) == 2)
    {
        return true;
    }
    // 情况2：/block/mmcblk0/mmcblk0p1
    else if (sscanf(p, "/block/mmcblk%1[0-9]/mmcblk%1[0-9]p%1[0-9]", major, major1, minor) == 3)
    {
        return true;
    }

    return false;
}

std::string CStorageManage::get_sd_uuid(const std::string &dev)
{

#if CAP_STORAGE_MMCBLK1  // 存储 mmcblk1 路径逻辑
    const std::string cmd = "/sbin/blkid -c /dev/null /dev/mmcblk1p1 2>/dev/null | sed -n 's/.*UUID=\"\\([^\"]*\\)\".*/\\1/p'";
#else
    const std::string cmd = "/sbin/blkid -c /dev/null /dev/mmcblk0p1 2>/dev/null | sed -n 's/.*UUID=\"\\([^\"]*\\)\".*/\\1/p'";
#endif

    std::array<char, 128>                    buffer;
    std::string                              strUuid;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe)
    {
        /* 失败就返回空串 */
        dlog_error("get_sd_uuid null");
        return "";
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()))
    {
        strUuid += buffer.data();
    }

    /* 去掉结尾的 \n */
    if (!strUuid.empty() && strUuid.back() == '\n')
    {
        strUuid.pop_back();
    }
    return strUuid;
}

/* 执行指定脚本 */
static int run_script(const std::string &path, const std::vector<std::string> &args, const std::vector<std::string> &extra_env)
{
    namespace fs = std::filesystem;
    if (!fs::exists(path) || !fs::is_regular_file(path))
        throw std::runtime_error("run_script: 脚本不存在 " + path);

    /* ---------- 1. 构造新的环境表 ---------- */
    std::vector<std::string> new_env;
    /* 备份当前环境 */
    for (char **ep = environ; *ep; ++ep)
    {
        new_env.emplace_back(*ep);
    }

    /* 再把调用者给的“KEY=VALUE”进行替换 */
    for (auto &kv : extra_env)
    {
        auto pos = kv.find('=');
        if (pos == std::string::npos)
        {
            throw std::runtime_error("run_script: env 格式非法 '" + kv + "'");
        }

        std::string key = kv.substr(0, pos);

        /* 查找并覆盖 */
        auto it = std::find_if(new_env.begin(), new_env.end(), [&](const std::string &s) { return s.compare(0, key.size() + 1, key + "=") == 0; });
        if (it != new_env.end())
        {
            *it = kv;
        }
        else
        {
            new_env.push_back(kv);
        }
    }

    /* 转成 char* 数组 */
    std::vector<const char *> envp;
    for (auto &s : new_env)
    {
        envp.push_back(s.c_str());
    }
    envp.push_back(nullptr);

    /* ---------- 2. 构造 argv ---------- */
    std::vector<const char *> argv;
    argv.push_back(path.c_str());
    for (auto &a : args)
    {
        argv.push_back(a.c_str());
    }
    argv.push_back(nullptr);

    /* ---------- 3. spawn ---------- */
    pid_t pid{};
    int   rc = posix_spawn(&pid, path.c_str(), nullptr, nullptr, const_cast<char *const *>(argv.data()), const_cast<char *const *>(envp.data()));
    if (rc != 0)
    {
        throw std::runtime_error("posix_spawn: " + std::string(strerror(rc)));
    }

    /* ---------- 4. 等待完成 ---------- */
    int status{};
    if (waitpid(pid, &status, 0) == -1)
    {
        throw std::runtime_error("waitpid 失败");
    }
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

SD_CARD_STATUS_E CStorageManage::get_SdCardStatus()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_SdCardStatus;
}

int CStorageManage::update_DatabaseDirUseSize()
{
    int                          nRet = 0;
    Record_NS::RecordDirInfo_S   stRecordDirInfo;
    Capture_NS::CaptureDirInfo_S stCaptureDirInfo;

    long long llSize       = 0;
    stRecordDirInfo.nChnId = 0;
    nRet                   = RecordFileDatabase::instance()->get_itemInfo(stRecordDirInfo);
    get_directory_size(llSize, RECORD_PATH);
    m_llRecordDirUseSize       = llSize;
    stRecordDirInfo.nTotalSize = m_llRecordDirUseSize;
    if (nRet < 0)
    {
        RecordFileDatabase::instance()->add(stRecordDirInfo);
    }
    else
    {
        RecordFileDatabase::instance()->update(stRecordDirInfo);
    }
    dlog_info("录制目录大小 [%lld] byte", m_llRecordDirUseSize);

    llSize                  = 0;
    stCaptureDirInfo.nChnId = 0;
    nRet                    = CCaptureDatabase::instance()->get_itemInfo(stCaptureDirInfo);
    get_directory_size(llSize, CAPTURE_PATH);
    m_llCaptureDirUseSize       = llSize;
    stCaptureDirInfo.nTotalSize = m_llCaptureDirUseSize;
    if (nRet < 0)
    {
        CCaptureDatabase::instance()->add(stCaptureDirInfo);
    }
    else
    {
        CCaptureDatabase::instance()->update(stCaptureDirInfo);
    }
    dlog_info("抓图目录大小 [%lld] byte", m_llCaptureDirUseSize);

    #if CAP_AI_FACE_COMPARE

    llSize = 0;

    get_directory_size(llSize, FACE_CAPTURE_PATH);

    m_llFaceDirUseSize = llSize;

    dlog_info("人脸目录大小 [%lld] byte", m_llFaceDirUseSize);

    #endif
    return 0;
}

#if 0
static int formatSDCardSyncCaptureDb()
{
    CCaptureDatabase::instance()->clear_table(CAPTURE_TABLE_NAME);
    Capture_NS::CaptureDirInfo_S stDirInfo;

    stDirInfo.nChnId = 0;
    int nRet         = CCaptureDatabase::instance()->get_itemInfo(stDirInfo);

    stDirInfo.nTotalSize = 0;
    stDirInfo.nCount     = 0;

    if (nRet < 0)
    {
        CCaptureDatabase::instance()->add(stDirInfo);
    }
    else
    {
        CCaptureDatabase::instance()->update(stDirInfo);
    }
    return 0;
}
#endif

static bool nginxReload()
{
    /* 打开 nginx pid 文件 */
    std::ifstream ifs(NGINX_PID_FILE_PATH);
    if (!ifs)
    {
        dlog_error("open %s failed", NGINX_PID_FILE_PATH);
        return false;
    }

    /* 从 pid 文件中读取 nginx master 进程 pid */
    pid_t pid = 0;
    ifs >> pid;

    if (pid <= 0)
    {
        dlog_error("invalid pid in %s", NGINX_PID_FILE_PATH);
        return false;
    }

    /* 发送 SIGHUP 触发平滑重载 */
    if (kill(pid, SIGHUP) != 0)
    {
        dlog_error("kill(%d, SIGHUP) failed: %m", pid);
        return false;
    }

    dlog_info("nginx reload sent (pid=%d)", pid);
    return true;
}

static int mkdirIfNotExist(const std::string &path)
{
    try
    {
        fs::create_directories(path); /* 已存在不会报错 */
        dlog_info("creat %s success", path.c_str());
        return 0;
    } catch (const fs::filesystem_error &e)
    {
        dlog_error("mkdir failed: %s, path=%s, ec=%s", e.what(), e.path1().c_str(), e.code().message().c_str());
        return -1;
    } catch (const std::exception &e)
    {
        dlog_error("mkdir failed: %s, path=%s", e.what(), path.c_str());
        return -1;
    } catch (...)
    {
        dlog_error("mkdir failed: unknown exception, path=%s", path.c_str());
        return -1;
    }
}

static std::string getDeviceByMountPoint(const std::string &strMountPoint)
{
    std::ifstream file(PROC_SELF_MOUNTS_PATH);
    if (!file.is_open())
    {
        return "";
    }

    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        std::string        strDevice, strMount;

        if (iss >> strDevice >> strMount)
        {
            if (strMount == strMountPoint)
            {
                return strDevice;
            }
        }
    }

    return "";
}

int CStorageManage::format_sd_card(bool bIsInitSdCard)
{
    int nRet        = 0;
    int nRetryCount = 0;
    if (!m_stStorageManageParam.bEnable)
    {
        dlog_info("未启用轻存储功能");
        return -1;
    }

    if (!bIsInitSdCard)
    {
        dlog_info("不使用格式化功能");
        return -1;
    }

    /* sd卡存在 */
    if ( (m_SdCardStatus == SD_CARD_STATUS_E::NORMAL) || (sd_card_is_exist() && sd_card_is_mounted()) )
    {
        CRecordCtrl::instance()->stop_record();
        sleep(2); /* 等待停止录制 */
        
        std::lock_guard<std::mutex> lock(m_mutex);
        
        m_SdCardStatus = SD_CARD_STATUS_E::FORMATING;
        m_stStorageManageParam.clear();

        /* 去初始化已经打开的数据库 */
        CCaptureDatabase::instance()->deinit();
        RecordFileDatabase::instance()->deinit();
        EventDatabase::instance()->deinit();

        std::string strDevice = getDeviceByMountPoint(SD_CARD_MOUNT_PATH);

#if CAP_STORAGE_MMCBLK1  // 存储 mmcblk1 路径逻辑
        nRet = run_script(SD_CARD_MOUNT_REMOVE_SCRIPT_PATH, {strDevice, "remove"}, {});  // 关键环境变量
#else
        std::string strTmpDevice = strDevice;

        size_t pos = strTmpDevice.find_last_of('/');

        if (pos != std::string::npos)
        {
            strTmpDevice = strTmpDevice.substr(pos + 1);  // 截取 '/' 之后的部分
        }

        std::string strMdev = "MDEV=" + strTmpDevice;
        /* 执行卸载sd卡脚本 */
        nRet = run_script(SD_CARD_REMOVE_SCRIPT_PATH, {}, {strMdev});  // 关键环境变量
#endif
        if (nRet != 0)
        {
            dlog_error("sd卡卸载失败");
            // return -1;
        }
        /* ============================================== 进行格式化 ============================================== */
        while (1)
        {
            usleep(300 * 1000);

            std::string strFormatCommand = "mkfs.exfat -n CAM_SD " + strDevice;
            nRet                         = system(strFormatCommand.c_str());
            if (nRet == 0)
            {
                dlog_info("sd卡格式化成功");
                break;
            }
            else
            {
                if (nginxReload())
                {
                    /* 等待 worker 退出 */
                    usleep(500 * 1000);
                }
            }

            nRetryCount++;
            if (nRetryCount >= 5)
            {
                break;
            }
        }
        if (nRet != 0)
        {
            dlog_error("sd卡格式化失败，进行重新挂载");
            sleep(1);

#if CAP_STORAGE_MMCBLK1                                                                   // 存储 mmcblk1 路径逻辑
            nRet = run_script(SD_CARD_MOUNT_REMOVE_SCRIPT_PATH, {strDevice, "add"}, {});  // 关键环境变量
#else
            // nRet = run_script(SD_CARD_MOUNT_SCRIPT_PATH, {}, {strMdev});
            std::string strFaceStorage = "ENABLE_FACE_STORAGE=1"; 
            nRet = run_script(SD_CARD_MOUNT_SCRIPT_PATH, {}, {strMdev, strFaceStorage}); 
#endif
            if (nRet == 0)
            {
                time_t nCurTimeStamp = time(NULL);
                while (1)
                {
                    usleep(100 * 1000);
                    if (sd_card_is_exist() && sd_card_is_mounted())
                    {
                        std::string strDelCommnad = std::string("rm -rf ") + SD_CARD_MOUNT_PATH + std::string("/*");

                        nRet = system(strDelCommnad.c_str());

                        if (nRet == 0)
                        {
                            #if CAP_AI_FACE_COMPARE
                            clear_face_database_after_sd_format();
                            #endif
                            std::string strMkstrRecordPath = SD_CARD_MOUNT_PATH + std::string("/record");
                            std::string strMkCapturePath   = SD_CARD_MOUNT_PATH + std::string("/capture");
                            mkdirIfNotExist(strMkstrRecordPath);
                            mkdirIfNotExist(strMkCapturePath);
                            #if CAP_AI_FACE_COMPARE
                            std::string strMkFacePath = SD_CARD_MOUNT_PATH + std::string("/face");

                            mkdirIfNotExist(strMkFacePath);
                            #endif

                            CStorageManageConfigure::instance()->set_configure(m_stStorageManageParam);

                            /* 插入sd卡初始化存放到sd卡空间的数据库 */
                            CCaptureDatabase::instance()->init();
                            RecordFileDatabase::instance()->init();
                            EventDatabase::instance()->init();

                            /* 同步数据库记录大小和SD卡实际使用空间大小 */
                            update_DatabaseDirUseSize();
                            calculate_storageManage_param();
                            // RecordFileManage::instance()->formatSDCardSyncRecordDb();
                            // formatSDCardSyncCaptureDb();
                            m_SdCardStatus = SD_CARD_STATUS_E::NORMAL;

                            /* 恢复录制 */
                            CRecordCtrl::instance()->start_record();

                            dlog_info("执行%s成功：%d", strDelCommnad.c_str(), nRet);
                            // break;
                            return 0;
                        }
                        else
                        {
                            dlog_error("执行%s失败：%d", strDelCommnad.c_str(), nRet);
                            nRet = -1;
                        }
                    }
                    else
                    {
                        nRet = -1;
                        dlog_info("检测sd卡状态未挂载/不存在，重新检测");
                    }

                    if (time(NULL) - nCurTimeStamp > 5)
                    {
                        dlog_info("检测sd卡状态超时");
                        break;
                    }
                }
            }
            else
            {
                dlog_error("重新挂载失败");
            }

            return nRet;
        }
        /* ============================================== 进行格式化 ============================================== */

        /* ============================================== 格式化成功，重新挂载 ============================================== */
        nRetryCount = 0;
        while (1)
        {
            usleep(300 * 1000);

#if CAP_STORAGE_MMCBLK1                                                                   // 存储 mmcblk1 路径逻辑
            nRet = run_script(SD_CARD_MOUNT_REMOVE_SCRIPT_PATH, {strDevice, "add"}, {});  // 关键环境变量
#else
            // nRet = run_script(SD_CARD_MOUNT_SCRIPT_PATH, {}, {strMdev});
            std::string strFaceStorage = "ENABLE_FACE_STORAGE=1"; 
            nRet = run_script(SD_CARD_MOUNT_SCRIPT_PATH, {}, {strMdev, strFaceStorage}); 
#endif
            if (nRet == 0)
            {
                dlog_info("sd卡格式化成功，重新挂载成功");
                break;
            }
            nRetryCount++;
            if (nRetryCount >= 5)
            {
                break;
            }
        }
        if (nRet != 0)
        {
            dlog_error("sd卡挂载失败");
            return -1;
        }
        /* ============================================== 格式化成功，重新挂载 ============================================== */

        while (1)
        {
            if (sd_card_is_exist() && sd_card_is_mounted())
            {
                #if CAP_AI_FACE_COMPARE
                clear_face_database_after_sd_format();
                #endif
                CStorageManageConfigure::instance()->set_configure(m_stStorageManageParam);

                /* 插入sd卡初始化存放到sd卡空间的数据库 */
                CCaptureDatabase::instance()->init();
                RecordFileDatabase::instance()->init();
                EventDatabase::instance()->init();

                /* 同步数据库记录大小和SD卡实际使用空间大小 */
                update_DatabaseDirUseSize();
                calculate_storageManage_param();
                // RecordFileManage::instance()->formatSDCardSyncRecordDb();
                // formatSDCardSyncCaptureDb();
                m_SdCardStatus = SD_CARD_STATUS_E::NORMAL;

                /* 恢复录制 */
                CRecordCtrl::instance()->start_record();
                // break;
                return 0;
            }
            usleep(100 * 1000);
        }
    }
    else
    {
        dlog_error("sd卡不存在或没挂载");
        return -1;
    }

    return 0;
}

bool CStorageManage::test_write_operation()
{
    std::string probe_path = std::string(SD_CARD_MOUNT_PATH) + "/" + PROBE_FILE;

    // 测试1：尝试打开文件写入（ O_DIRECT 绕过缓存）
    int fd = open(probe_path.c_str(), O_WRONLY | O_CREAT | O_SYNC | O_TRUNC, 0644);
    if (fd < 0)
    {
        if (errno == EROFS || errno == EPERM || errno == EACCES)
        {
            // 明确是只读
            return false;
        }
        return false;  // 其他错误
    }

    // 测试2：写入实际数据
    const char test_pattern[] = "SD_GUARDIAN_PROBE_TEST_12345";
    ssize_t    written        = write(fd, test_pattern, sizeof(test_pattern));
    if (written != sizeof(test_pattern))
    {
        close(fd);
        unlink(probe_path.c_str());
        return false;
    }

    // 测试3：强制同步到硬件（避免出现假写入的问题）
    if (fsync(fd) != 0)
    {
        close(fd);
        unlink(probe_path.c_str());
        return false;
    }
    close(fd);

    // 测试4：读取验证
    fd = open(probe_path.c_str(), O_RDONLY);
    if (fd < 0)
    {
        unlink(probe_path.c_str());
        return false;
    }

    char    read_buf[64] = {0};
    ssize_t n            = read(fd, read_buf, sizeof(read_buf));
    close(fd);
    unlink(probe_path.c_str());

    if (n != sizeof(test_pattern) || strcmp(read_buf, test_pattern) != 0)
    {
        return false;  // 数据损坏
    }

    return true;
}

void CStorageManage::run()
{
    char buf[4096];
    pthread_setname_np(pthread_self(), "SDFormatRun");
    struct pollfd pfd = {.fd = m_nSock, .events = POLLIN};

    int nCount = 0;
    /* 检测sd卡连续异常次数 */
    unsigned int unAbnormalCount = 0;

    // struct pollfd pfd[2];
    // pfd[0].fd     = m_nSock;
    // pfd[0].events = POLLIN;

    // int nRet = 0;
    // DirInfo_t ctx = { .total_bytes = 0, .errors = 0 };

    if (sd_card_is_exist() && sd_card_is_mounted())
    {
        update_DatabaseDirUseSize();
        m_SdCardStatus = SD_CARD_STATUS_E::NORMAL;
    }

    calculate_storageManage_param();

    while (m_bRun.load(std::memory_order_acquire))
    {
        /* 每隔15s检测一次是否写入正常 */
        if (nCount >= 15)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_SdCardStatus == SD_CARD_STATUS_E::NORMAL || m_SdCardStatus == SD_CARD_STATUS_E::WRITE_ERROR)
            {
                if (!test_write_operation())
                {
                    unAbnormalCount++;
                }
                else
                {
                    unAbnormalCount = 0;
                    if (m_SdCardStatus == SD_CARD_STATUS_E::WRITE_ERROR)
                    {
                        dlog_info("SD卡异常状态恢复正常\n");
                        m_SdCardStatus = SD_CARD_STATUS_E::NORMAL;
                    }
                }
                /* 连续检测到sd卡三次写入失败 */
                if (unAbnormalCount >= 3)
                {
                    m_SdCardStatus = SD_CARD_STATUS_E::WRITE_ERROR;
                    dlog_error("SD卡写入异常\n");
                }
            }
            nCount = 0;
        }
        nCount++;

        int ret = poll(&pfd, 1, 1000); /* 1000ms 超时 */
        if (ret < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            perror("poll");
            break;
        }
        if (ret == 0)
        {
            /* 超时 */
            continue;
        }

        ssize_t len = recv(m_nSock, buf, sizeof(buf) - 1, 0);
        if (len < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                /* 无数据，继续轮询 */
                continue;
            }
            perror("recv");
            break;
        }
        buf[len] = '\0';

        int add;
        if (is_sd_event(buf, &add))
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            const char                 *dev = strstr(buf, "/block/");
            if (dev)
            {
                /* 跳过/block/ */
                dev += 7;
            }
            else
            {
                dev = "mmcblk0";
            }
            if (add)
            {
                /* sd卡插入 */
                m_SdCardStatus = SD_CARD_STATUS_E::INSERT;

                time_t nStartTime = time(NULL);
                while (1)
                {
                    if (sd_card_is_exist() && sd_card_is_mounted())
                    {
                        usleep(100 * 1000);
                        /* 插入sd卡初始化存放到sd卡空间的数据库 */
                        CCaptureDatabase::instance()->init();
                        RecordFileDatabase::instance()->init();
                        EventDatabase::instance()->init();

                        /* 同步数据库记录大小和SD卡实际使用空间大小 */
                        update_DatabaseDirUseSize();
                        m_SdCardStatus = SD_CARD_STATUS_E::NORMAL;
                        calculate_storageManage_param();

                        /* 恢复录制 */
                        CRecordCtrl::instance()->start_record();

                        break;
                    }

                    if (time(NULL) - nStartTime > 5)
                    {
                        dlog_error("检测sd卡存在-挂载超时");
                        break;
                    }
                }
            }
            else
            {
                /* 检测到拔出sd卡 */
                m_stStorageManageParam.clear();
                m_SdCardStatus = SD_CARD_STATUS_E::UNPLUG;

                /* 停止录制 */
                CRecordCtrl::instance()->stop_record();

                /* 去初始化存放到sd卡空间的数据库 */
                CCaptureDatabase::instance()->deinit();
                RecordFileDatabase::instance()->deinit();
                EventDatabase::instance()->deinit();
            }
            dlog_debug("%s: SD card %s\n", dev, add ? "插入" : "拔出");
        }
    }
}
