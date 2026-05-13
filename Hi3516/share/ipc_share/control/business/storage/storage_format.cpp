#include "storage_format.h"
#include "dlog.h"
#include "storage_manage.h"

#define EVENT_SIZE                     (sizeof(struct inotify_event))
#define BUF_LEN                        (1024 * (EVENT_SIZE + 16))
/* 记录检测到的事件数组 */
#define MAX_EVENTS                     5
/* 超时时长 */
#define DEVICE_READY_TIMEOUT_MS        5000
/* 等待重试时间间隔 */
#define DEVICE_READY_RETRY_INTERVAL_MS 200


StorageFormat::StorageFormat()
{

}

StorageFormat::~StorageFormat()
{

}

int StorageFormat::init()
{
    try
    {
        /* 创建配置 */
        m_config.nPollInterval     = 20;                                  // 20秒轮询一次
        m_config.nFormatDelay      = 3;                                   // 设备就绪等待3秒
        m_config.bEnableInotify    = true;                                // 启用inotify
        m_config.bEnableAutoFormat = true;                                // 启用自动格式化
        m_config.strFormatCommand    = "/sbin/mkfs.vfat -F32 %device%";   // 使用占位符

        /* 验证配置 */ 
        std::string errorMsg;
        if (!m_config.validate(errorMsg))
        {
            dlog_error("存储管理参数配置错误:%s", errorMsg.c_str());
            return -1;
        }
        
        /* 启动监控线程 */
        if (!start())
        {
            dlog_error("启动存储设备检测失败");
            return -1;
        }
        dlog_info("存储设备检测初始化成功");
        return 0;
    } 
    catch (const std::exception &e)
    {
        std::cerr << "异常: " << e.what() << std::endl;
        dlog_error("存储设备检测初始化失败：%s", e.what());
        return -1;
    }
    return 0;
}

int StorageFormat::deinit()
{
    stop();
    dlog_info("存储设备检测去初始化成功");
    return 0;
}


bool StorageFormat::start()
{
    if (m_bRunning)
    {
        return false;
    }

    m_bRunning = true;

    try
    {
        /* 初始化已知设备列表 */ 
        m_knownDevices = getCurrentDevices();

        /* 启动存储设备检测线程 */ 
        m_monitorThread = std::thread(&StorageFormat::run, this);

        return true;
    } 
    catch (const std::exception &e)
    {
        m_bRunning = false;
        return false;
    }
}

void StorageFormat::stop()
{
    if (!m_bRunning)
    {
        return;
    }

    m_bRunning = false;

    if (m_monitorThread.joinable())
    {
        m_monitorThread.join();
    }

    /* 清理inotify资源 */ 
    if (m_watchFd >= 0)
    {
        inotify_rm_watch(m_inotifyFd, m_watchFd);
        m_watchFd = -1;
    }

    m_inotifyFd.reset();
}

DeviceMap StorageFormat::getDevices()
{
    std::lock_guard<std::mutex> lock(devicesMutex);
    return m_knownDevices;
}

bool StorageFormat::initInotify()
{
    m_inotifyFd.reset(inotify_init1(IN_NONBLOCK));
    if (m_inotifyFd < 0)
    {
        dlog_error("inotify初始化失败，将使用轮询模式");
        return false;
    }

    /* 监测设备目录的设备的添加事件 */ 
    m_watchFd = inotify_add_watch(m_inotifyFd, m_config.strDeviceBasePath.c_str(), IN_CREATE | IN_DELETE);
    if (m_watchFd < 0)
    {
        dlog_error("无法监测设备目录[%s]，切换轮询模式", m_config.strDeviceBasePath.c_str());
        m_inotifyFd.reset();
        return false;
    }

    return true;
}

bool StorageFormat::isValidDeviceName(const std::string &device)
{
    /* 使用白名单验证设备名称 */
    static const std::regex device_regex("^(sd[a-z]|[a-z]+[0-9]*|mmcblk[0-9]+p?[0-9]*)$");
    if (!std::regex_match(device, device_regex))
    {
        dlog_error("设备名非法: %s\n", device.c_str());
        return false;
    }

    /* 检查设备路径是否在允许的目录中 */ 
    std::string fullPath = m_config.strDeviceBasePath + "/" + device;
    char resolvedPath[PATH_MAX] = {0};

    if (realpath(fullPath.c_str(), resolvedPath) == nullptr)
    {
        dlog_error("路径不存在或无法解析: %s\n", fullPath.c_str());
        return false;
    }

    /* 确保解析的路径在设备目录下 */ 
    std::string resolvedStr(resolvedPath);
    return resolvedStr.find(m_config.strDeviceBasePath) == 0;
}

bool StorageFormat::isStorageCard(const std::string &device)
{
    if (!isValidDeviceName(device))
    {
        return false;
    }

    for (const auto &pattern : m_config.strDevicePatterns)
    {
        if (device.find(pattern) == 0)
        {
            // dlog_info("匹配到存储卡: device=%s, pattern=%s\n", device.c_str(), pattern.c_str());
            return true;
        }
    }
    return false;
}

bool StorageFormat::isRemovableDevice(const std::string &device)
{
    std::string sysPath = "/sys/block/";
    if (device.find("mmcblk") == 0)
    {
        sysPath += device.substr(0, device.find_first_of("p"));
    }
    else
    {
        sysPath += device.substr(0, device.find_first_of("123456789"));
    }

    sysPath += "/removable";

    std::ifstream file(sysPath);
    if (!file.is_open())
    {
        return false;
    }

    std::string value;
    file >> value;
    return value == "1";
}

// 获取设备大小
uint64_t StorageFormat::getDeviceSize(const std::string &device)
{
    std::string fullPath = m_config.strDeviceBasePath + "/" + device;
    struct stat st;
    if (stat(fullPath.c_str(), &st) != 0)
    {
        return 0;
    }

    /* 对于块设备，使用ioctl获取准确大小 */ 
    if (S_ISBLK(st.st_mode))
    {
        int fd = open(fullPath.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd >= 0)
        {
            uint64_t size = 0;
            if (ioctl(fd, BLKGETSIZE64, &size) == 0)
            {
                close(fd);
                return size;
            }
            close(fd);
        }
    }

    return 0;
}

// 获取设备型号信息
std::string StorageFormat::getDeviceModel(const std::string &strDevice)
{
    /* sysfs 下存放块设备信息的目录前缀 */ 
    std::string strModelPath = "/sys/block/";

    /* 对于 eMMC/SD 卡 (mmcblkX)，可能带有分区号 (如 mmcblk0p1)，只保留主设备名部分 (mmcblk0) */
    if (strDevice.find("mmcblk") == 0)
    {
        /* 截取到 'p' 之前，例如 "mmcblk0p1" -> "mmcblk0" */
        strModelPath += strDevice.substr(0, strDevice.find_first_of("p"));
    }
    else
    {
        /* 对于一般磁盘设备 (如 sda, sdb)，可能带有分区号 (sda1)，只保留字母部分 (sda)*/
        strModelPath += strDevice.substr(0, strDevice.find_first_of("123456789"));
    }

    /* 拼接 sysfs 下的 model 文件路径，如 /sys/block/sda/device/model */
    strModelPath += "/device/model";

    std::ifstream file(strModelPath);
    if (!file.is_open())
    {
        /* 如果文件不存在或无法访问，返回空字符串 */
        return "";
    }

    std::string strModel;
    /* 读取 model 信息 */
    std::getline(file, strModel);
    return strModel;
}

// 获取设备序列号
std::string StorageFormat::getDeviceSerial(const std::string &strDevice)
{
    std::string strSerialPath = "/sys/block/";

    if (strDevice.find("mmcblk") == 0)
    {
        strSerialPath += strDevice.substr(0, strDevice.find_first_of("p"));
    }
    else
    {
        strSerialPath += strDevice.substr(0, strDevice.find_first_of("123456789"));
    }
    strSerialPath += "/device/serial";

    std::ifstream file(strSerialPath);
    if (!file.is_open())
    {
        return "";
    }

    std::string strSerial;
    std::getline(file, strSerial);
    return strSerial;
}

// 检查设备是否已格式化
std::string StorageFormat::detectFilesystem(const std::string &strDevice)
{
    std::string strFullPath = m_config.strDeviceBasePath + "/" + strDevice;

    // 生成候选设备列表
    std::vector<std::string> candidates;

    if (strDevice.find("mmcblk") == 0 && strDevice.find('p') == std::string::npos)
    {
        // mmcblk0 -> 尝试 mmcblk0p1..p4
        for (int partNum = 1; partNum <= 4; partNum++)
        {
            std::string partPath = m_config.strDeviceBasePath + "/" + strDevice + "p" + std::to_string(partNum);
            if (access(partPath.c_str(), F_OK) == 0)
                candidates.push_back(partPath);
        }
    }
    else if (strDevice.find("sd") == 0 && !isdigit(strDevice.back()))
    {
        // sda -> 尝试 sda1..sda4
        for (int partNum = 1; partNum <= 4; partNum++)
        {
            std::string partPath = m_config.strDeviceBasePath + "/" + strDevice + std::to_string(partNum);
            if (access(partPath.c_str(), F_OK) == 0)
                candidates.push_back(partPath);
        }
    }

    // 如果没找到分区，候选就是原始设备
    if (candidates.empty())
    {
        candidates.push_back(strFullPath);
    }

    // 重试机制
    int maxRetries   = 5;
    int retryDelayMs = 500;

    for (int retry = 0; retry < maxRetries; retry++)
    {
        for (const auto &devPath : candidates)
        {
            std::string strCommand = "/sbin/blkid -c /dev/null -o value -s TYPE " + devPath + " 2>/dev/null";
            FILE *pipe = popen(strCommand.c_str(), "r");
            if (!pipe)
            {
                dlog_error("执行blkid命令失败");
                continue;
            }

            char buffer[128];
            std::string result;
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
            {
                result += buffer;
            }
            pclose(pipe);

            // 去除换行符
            if (!result.empty() && result.back() == '\n')
                result.pop_back();

            if (!result.empty())
            {
                dlog_info("设备 %s 检测到文件系统: %s", devPath.c_str(), result.c_str());
                return result;
            }
        }

        // 如果没检测到，重试
        if (retry < maxRetries - 1)
        {
            usleep(retryDelayMs * 1000);
            retryDelayMs *= 2; // 指数退避
        }
    }

    dlog_warn("设备[%s]未检测到文件系统", strFullPath.c_str());
    return "";
}

// 等待设备就绪
bool StorageFormat::waitForDeviceReady(const std::string &device)
{
    std::string fullPath = m_config.strDeviceBasePath + "/" + device;

    /* 记录开始时间，用于计算超时 */ 
    auto start = std::chrono::steady_clock::now();

    /* 在超时时间内循环等待设备就绪 */
    while (std::chrono::steady_clock::now() - start < std::chrono::milliseconds(DEVICE_READY_TIMEOUT_MS))
    {
        /* 检查设备是否存在且可访问 */ 
        if (access(fullPath.c_str(), R_OK) == 0)
        {
            /* 以只读非阻塞方式打开设备，确保设备已经可以被内核完全识别 */
            int fd = open(fullPath.c_str(), O_RDONLY | O_NONBLOCK);
            if (fd >= 0)
            {
                /* 打开成功，说明设备已经就绪 */
                close(fd);
                return true;
            }
        }

        /* 等待一小段时间再重试，避免忙等 */
        std::this_thread::sleep_for(std::chrono::milliseconds(DEVICE_READY_RETRY_INTERVAL_MS));
    }

    /* 超时后仍未检测到设备就绪，返回 false */ 
    return false;
}

// 安全执行命令
int StorageFormat::executeCommand(const std::vector<std::string> &args)
{
    if (args.empty())
    {
        return -1;
    }

    // 准备参数数组
    std::vector<char *> argv;
    for (const auto &arg : args)
    {
        argv.push_back(const_cast<char *>(arg.c_str()));
    }
    argv.push_back(nullptr);

    // 创建子进程
    pid_t pid = fork();
    if (pid == -1)
    {
        return -1;
    }

    if (pid == 0)
    {
        // 在子进程中执行命令
        execvp(argv[0], argv.data());
        // 如果execvp失败，退出子进程
        _exit(127);
    }

    // 在父进程中等待子进程完成
    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status))
    {
        return WEXITSTATUS(status);
    }

    return -1;
}

// 格式化存储卡
bool StorageFormat::formatStorageCard(const std::string &strDevice)
{
    std::string strFullPath = m_config.strDeviceBasePath + "/" + strDevice;
    dlog_info("开始格式化设备 %s", strDevice.c_str());

    // 分割格式化命令
    std::istringstream       iss(m_config.strFormatCommand);
    std::vector<std::string> args;
    std::string              arg;

    while (iss >> arg)
    {
        /* 替换设备路径占位符 */ 
        if (arg == "%device%")
        {
            arg = strFullPath;
        }
        args.push_back(arg);
    }

    /* 添加设备路径作为最后一个参数（如果没有使用占位符） */ 
    if (m_config.strFormatCommand.find("%device%") == std::string::npos)
    {
        args.push_back(strFullPath);
    }

    /* 执行格式化命令 */
    int result = executeCommand(args);

    if (result == 0)
    {
        dlog_info("%s格式化成功", strDevice.c_str());
        return true;
    }
    else
    {
        dlog_error("%s格式化失败，退出码: %d", strDevice.c_str(), result);
        return false;
    }
}

// 检查分区表是否存在
bool StorageFormat::hasPartitionTable(const std::string& strDevice) 
{
    std::string strFullPath = m_config.strDeviceBasePath + "/" + strDevice;
    
    /* 读取MBR（主引导记录） */ 
    char mbr[512];
    int fd = open(strFullPath.c_str(), O_RDONLY);
    if (fd < 0) 
    {
        return false;
    }
    
    bool bHasTable = false;
    if (read(fd, mbr, sizeof(mbr)) == sizeof(mbr)) 
    {
        /* 检查MBR签名（0x55AA） */
        if (mbr[510] == 0x55 && mbr[511] == 0xAA) {
            bHasTable = true;
        }
    }
    
    close(fd);
    return bHasTable;
}

// 自动选择文件系统类型
std::string StorageFormat::selectFilesystemType(uint64_t size) 
{
    /* 根据SD卡大小选择文件系统 */
    /* 32GB以上 */ 
    if (size > 32 * 1024 * 1024 * 1024ULL) 
    { 
        return "exfat";
    } 
    else 
    {
        return "vfat";
    }
}

// 创建分区并格式化（使用fdisk）
bool StorageFormat::createPartitionAndFormat(const std::string& strDevice) 
{
    std::string strFullPath = m_config.strDeviceBasePath + "/" + strDevice;
    uint64_t size = getDeviceSize(strDevice);
        
    /* 使用fdisk创建分区 */ 
    /* 命令说明：创建新的DOS分区表，创建一个主分区，使用整个磁盘 */
    std::string fdiskCmd = "echo -e 'o\nn\np\n1\n\n\nw' | fdisk " + strFullPath + " > /dev/null 2>&1";
    int result = system(fdiskCmd.c_str());
    
    if (result != 0) 
    {
        dlog_error("创建分区失败 %s", strDevice.c_str());
        return false;
    }
    
    /* 等待系统识别新分区 */ 
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    /* 通知内核重新读取分区表 */ 
    system(("partprobe " + strFullPath).c_str());
    
    /* 确定分区设备名 */ 
    std::string strPartitionDevice;
    if (strDevice.find("mmcblk") == 0 || strDevice.find("nvme") == 0)
    {
        strPartitionDevice = strDevice + "p1"; // 如mmcblk0p1
    } 
    else 
    {
        strPartitionDevice = strDevice + "1";  // 如sda1
    }
    
    /* 等待分区设备就绪 */
    if (!waitForDeviceReady(strPartitionDevice)) 
    {
        // dlog_error("分区设备未就绪:[%s] [%s]", strPartitionDevice.c_str(), strDevice.c_str());
        return false;
    }
    
    // 选择文件系统类型
    std::string fsType = selectFilesystemType(size);
    std::string formatCmd;
    
    if (fsType == "exfat") 
    {
        formatCmd = "mkfs.exfat /dev/" + strPartitionDevice;
    } 
    else 
    {
        formatCmd = "mkfs.vfat -F32 /dev/" + strPartitionDevice;
    }
    
    // 执行格式化
    result = system(formatCmd.c_str());
    
    if (result == 0)
    {
        dlog_info("分区创建并格式化成功 %s", strDevice.c_str());
        return true;
    } 
    else 
    {
        dlog_error("格式化分区失败:%s", strDevice.c_str());
        return false;
    }
}

// 处理新设备
void StorageFormat::processNewDevice(const std::string &strDevice)
{
    {
        std::lock_guard<std::mutex> lock(devicesMutex);
        if (m_knownDevices.find(strDevice) != m_knownDevices.end())
        {
            return;  // 已经处理过的设备
        }
    }

    dlog_info("检测到新存储设备%s", strDevice.c_str());

    /* 等待设备就绪 */ 
    if (!waitForDeviceReady(strDevice))
    {
        return;
    }

    /* 检查是否有分区表 */ 
    if (!hasPartitionTable(strDevice)) 
    {
        dlog_info("设备[%s]没有分区表，需要创建分区", strDevice.c_str());
        if (createPartitionAndFormat(strDevice)) 
        {
            // 更新设备信息
            std::lock_guard<std::mutex> lock(devicesMutex);
            StorageManage_NS::DeviceInfo_S stDeviceInfo;
            stDeviceInfo.name = strDevice;
            stDeviceInfo.path = m_config.strDeviceBasePath + "/" + strDevice;
            stDeviceInfo.size = getDeviceSize(strDevice);
            stDeviceInfo.isFormatted = true;
            stDeviceInfo.filesystem = selectFilesystemType(stDeviceInfo.size);
            stDeviceInfo.detectedTime = time(nullptr);
            m_knownDevices[strDevice] = stDeviceInfo;
        }
        /* 分区创建完成后直接返回，不再执行后续的文件系统检查 */ 
        return; 
    }

    /* 收集设备信息 */ 
    StorageManage_NS::DeviceInfo_S stDeviceInfo;
    stDeviceInfo.name         = strDevice;
    stDeviceInfo.path         = m_config.strDeviceBasePath + "/" + strDevice;
    stDeviceInfo.size         = getDeviceSize(strDevice);
    stDeviceInfo.isRemovable  = isRemovableDevice(strDevice);
    stDeviceInfo.filesystem   = detectFilesystem(strDevice);
    stDeviceInfo.isFormatted  = !stDeviceInfo.filesystem.empty();
    stDeviceInfo.detectedTime = time(nullptr);
    stDeviceInfo.model        = getDeviceModel(strDevice);
    stDeviceInfo.serial       = getDeviceSerial(strDevice);

    if (m_config.bEnableAutoFormat && !stDeviceInfo.isFormatted)
    {
        dlog_info("设备[%s]未格式化，开始格式化...\n", strDevice.c_str());
        if (formatStorageCard(strDevice))
        {
            // 更新文件系统信息
            stDeviceInfo.filesystem  = detectFilesystem(strDevice);
            stDeviceInfo.isFormatted = !stDeviceInfo.filesystem.empty();

            std::lock_guard<std::mutex> lock(devicesMutex);
            m_knownDevices[strDevice] = stDeviceInfo;
        }
    }
    else
    {
        if(stDeviceInfo.isFormatted)
        {
            dlog_info("设备已格式化");
        }
        else 
        {
            dlog_info("设备未格式化但自动格式化已禁用");
        }

        std::lock_guard<std::mutex> lock(devicesMutex);
        m_knownDevices[strDevice] = stDeviceInfo;
    }
}

// 获取当前设备列表
DeviceMap StorageFormat::getCurrentDevices()
{
    DeviceMap     devices;
    std::ifstream partitions("/proc/partitions");
    std::string   line;

    /* 跳过前两行标题 */ 
    std::getline(partitions, line);
    std::getline(partitions, line);

    while (std::getline(partitions, line))
    {
        if (line.empty())
        {
            continue;
        }
            
        /* 解析设备名称 */
        size_t lastSpace = line.find_last_of(' ');
        if (lastSpace != std::string::npos)
        {
            std::string strDevice = line.substr(lastSpace + 1);

            if (isStorageCard(strDevice))
            {
                StorageManage_NS::DeviceInfo_S strDeviceInfo;

                strDeviceInfo.name         = strDevice;
                strDeviceInfo.path         = m_config.strDeviceBasePath + "/" + strDevice;
                strDeviceInfo.size         = getDeviceSize(strDevice);
                strDeviceInfo.isRemovable  = isRemovableDevice(strDevice);
                strDeviceInfo.filesystem   = detectFilesystem(strDevice);
                strDeviceInfo.isFormatted  = !strDeviceInfo.filesystem.empty();
                strDeviceInfo.detectedTime = time(nullptr);
                strDeviceInfo.model        = getDeviceModel(strDevice);
                strDeviceInfo.serial       = getDeviceSerial(strDevice);
                devices[strDevice] = strDeviceInfo;
            }
        }
    }

    return devices;
}

// 轮询检查设备变化
void StorageFormat::pollDevices()
{
    DeviceMap currentDevices = getCurrentDevices();

    /* 检查新设备 */ 
    for (const auto &pair : currentDevices)
    {
        bool bIsNew = false;
        {
            std::lock_guard<std::mutex> lock(devicesMutex);
            bIsNew = m_knownDevices.find(pair.first) == m_knownDevices.end();
        }

        if (bIsNew)
        {
            processNewDevice(pair.first);
        }
    }

    /* 检查移除的设备 */ 
    std::vector<std::string> removedDevices;
    {
        std::lock_guard<std::mutex> lock(devicesMutex);
        for (const auto &pair : m_knownDevices)
        {
            if (currentDevices.find(pair.first) == currentDevices.end())
            {
                removedDevices.push_back(pair.first);
                dlog_info("设备[%s]已移除", pair.first.c_str());
            }
        }

        /* 更新已知设备列表 */
        for (const auto &device : removedDevices)
        {
            m_knownDevices.erase(device);
        }
    }
}

void StorageFormat::run()
{
    pthread_setname_np(pthread_self(), "SDFormatRun");
    bool useInotify = m_config.bEnableInotify && initInotify();

    try
    {
        if (useInotify)
        {
            dlog_info("使用inotify监测存储设备"); 
            monitorWithInotify();
        }
        else
        {
            dlog_info("使用轮询方式监测存储设备");
            monitorWithPolling();
        }
    } 
    catch (const std::exception &e)
    {
        dlog_error("监控循环异常:%s", e.what());
    }
}

// 监控设备事件(inotify模式)
void StorageFormat::monitorWithInotify()
{
    char   buffer[BUF_LEN]; /* 用于存放 inotify 事件数据 */
    time_t lastPollTime = 0;

    /* 创建epoll实例 */
    StorageManage_NS::FileDescriptor epollFd(epoll_create1(0));
    if (epollFd < 0)
    {
        dlog_error("无法创建epoll实例，切换到轮询模式");
        monitorWithPolling();
        return;
    }

    /* 将 inotify 文件描述符加入 epoll 监听 */ 
    struct epoll_event event;
    event.events  = EPOLLIN;
    event.data.fd = m_inotifyFd;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, m_inotifyFd, &event) < 0)
    {
        dlog_error("无法添加inotify到epoll，切换到轮询模式"); 
        monitorWithPolling();
        return;
    }

    struct epoll_event events[MAX_EVENTS];

    while (m_bRunning)
    {
        if(!CStorageManage::instance()->getAutoFormatSdCardFlag())
        {
            sleep(1);
            continue;
        }

        time_t currentTime = time(nullptr);

        /* 定期使用轮询作为备份 */
        if (currentTime - lastPollTime >= m_config.nPollInterval)
        {
            pollDevices();
            lastPollTime = currentTime;
        }

        /* 使用epoll等待事件，超时时间为1秒 */
        int nfds = epoll_wait(epollFd, events, MAX_EVENTS, 1000);
        if (nfds < 0)
        {
            if (errno == EINTR)
            {
                /* 被信号中断，继续等待 */
                continue;  
            }
            dlog_info("epoll等待错误，切换到轮询模式");

            monitorWithPolling();
            return;
        }

        if (nfds == 0)
        {
            /* 超时，无事件，继续循环 */
            continue;  
        }

        /* 处理inotify事件，从 inotifyFd 读取事件 */
        int length = read(m_inotifyFd, buffer, BUF_LEN);
        if (length < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                /* 没有数据，继续等待 */
                continue;
            }
            dlog_error("inotify读取错误，切换到轮询模式");
            monitorWithPolling();
            return;
        }

        int i = 0;
        while (i < length)
        {
            struct inotify_event *evt = (struct inotify_event *)&buffer[i];
            if (evt->len)
            {
                std::string deviceName = evt->name;

                if (evt->mask & IN_CREATE)
                {
                    if (isStorageCard(deviceName))
                    {
                        processNewDevice(deviceName);
                    }
                }
                else if (evt->mask & IN_DELETE)
                {
                    std::lock_guard<std::mutex> lock(devicesMutex);
                    if (m_knownDevices.erase(deviceName) > 0)
                    {
                        dlog_info("设备[%s]已移除", deviceName.c_str());
                    }
                }
            }
            i += EVENT_SIZE + evt->len;
        }
    }
}

// 监控设备事件(轮询模式)
void StorageFormat::monitorWithPolling()
{
    while (m_bRunning)
    {
        if(!CStorageManage::instance()->getAutoFormatSdCardFlag())
        {
            sleep(1);
            continue;
        }

        pollDevices();

        /* 等待下一次轮询 */
        for (int i = 0; i < m_config.nPollInterval && m_bRunning; i++)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}
