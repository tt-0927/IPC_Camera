/**
 * @FilePath     : system_task.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-23 17:04:24
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-27 09:00:04
 * @Description  : 系统相关任务
 */

#include "system_task.h"

#include "dlog.h"
#include "system_convert.h"
#include "av_configure.h"
#include "log_handler.h"
#include "common_convert.h"
#include "system_manage.h"
#include "time_manage.h"
#include "osd_manage.h"
#include "ip_filter_manage.h"
#include "convert_interface.h"
#include "upgrade_client.h"
#include "operation_client.h"
#include "event_manage.h"
#include "action_code.h"
#include "web_server.h"
#include "path_define.h"
#include "base_define.h"
#include "peripheral_manage.h"
#include "time_utils.h"
#include "record_ctrl.h"

#include <cerrno>
#include <cstring>
#include <fstream>
#include <mutex>
#include <ostream>
#include <string>

namespace
{
const char *kLastUpgradeFile = "/opt/course/upload/.tvsdk_last_upgrade";
// upgrade 进程的状态持久化文件（由 upgrade 进程的 data_manage.cpp 写入）
const char *kUpgradeProcessStatusFile = "/opt/cam/.config/user_data/.tvsdk_upgrade_status";
std::mutex g_upgradeStatusMutex;
int g_lastUpgradeStatus = ::System::TI_UPGRADE_NULL;

#pragma pack(push, 1)
struct UpgradePackageHeader
{
    char id[4];
    char version[48];
    char md5[33];
    long long len;
    int reserves;
};
#pragma pack(pop)

bool is_absolute_path(const std::string &path)
{
    return !path.empty() && path[0] == '/';
}

bool has_invalid_upgrade_path_char(const std::string &path)
{
    for (char ch : path)
    {
        if (ch == '\r' || ch == '\n')
            return true;
    }
    return false;
}

bool is_valid_upgrade_filename(const std::string &name)
{
    if (name.empty())
        return false;
    if (name[0] == '.' ||
        name == "." || name == ".." ||
        name.find('/') != std::string::npos ||
        name.find('\\') != std::string::npos ||
        name.find("..") != std::string::npos)
        return false;
    return !has_invalid_upgrade_path_char(name);
}

bool is_under_upload_path(const std::string &path)
{
    const std::string uploadPath(UPLOAD_PATH);
    return path.compare(0, uploadPath.size(), uploadPath) == 0 &&
           path.size() > uploadPath.size();
}

bool normalize_upgrade_path(std::string &path)
{
    if (path.empty() || path.find("://") != std::string::npos ||
        has_invalid_upgrade_path_char(path) ||
        path.find('\\') != std::string::npos ||
        path.find("..") != std::string::npos)
        return false;

    if (is_absolute_path(path))
    {
        if (is_under_upload_path(path))
            return true;

        return false;
    }

    if (!is_valid_upgrade_filename(path))
        return false;

    path = std::string(UPLOAD_PATH) + path;
    return true;
}

std::string trim_ascii_space(const std::string &value)
{
    std::string::size_type begin = 0;
    while (begin < value.size() && static_cast<unsigned char>(value[begin]) <= ' ')
        ++begin;

    std::string::size_type end = value.size();
    while (end > begin && static_cast<unsigned char>(value[end - 1]) <= ' ')
        --end;

    return value.substr(begin, end - begin);
}

std::size_t bounded_strlen(const char *value, std::size_t maxLen)
{
    std::size_t len = 0;
    while (len < maxLen && value[len] != '\0')
        ++len;
    return len;
}

bool read_last_upgrade_path(std::string &path)
{
    std::ifstream marker(kLastUpgradeFile, std::ios::binary);
    if (!marker.is_open())
        return false;

    std::string value;
    std::getline(marker, value, '\0');
    value = trim_ascii_space(value);
    if (value.empty())
        return false;

    if (!normalize_upgrade_path(value))
        return false;

    path = value;
    return true;
}

bool read_upgrade_version_from_file(const std::string &path, std::string &version)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
        return false;

    UpgradePackageHeader header;
    std::memset(&header, 0, sizeof(header));
    file.read(reinterpret_cast<char *>(&header), sizeof(header));
    if (file.gcount() != static_cast<std::streamsize>(sizeof(header)))
        return false;

    version.assign(header.version, bounded_strlen(header.version, sizeof(header.version)));
    version = trim_ascii_space(version);
    return !version.empty();
}

bool is_upgrade_status_value(int status)
{
    return status == ::System::TI_UPGRADE_NULL ||
           status == ::System::TI_UPGRADE_RUNING ||
           status == ::System::TI_UPGRADE_RUNFAIL ||
           status == ::System::TI_UPGRADE_RUNSUCCESS ||
           status == ::System::TI_UPGRADE_OTHERUPDATE_FAILE;
}

void set_last_upgrade_status(int status)
{
    if (!is_upgrade_status_value(status))
        return;

    std::lock_guard<std::mutex> lock(g_upgradeStatusMutex);
    g_lastUpgradeStatus = status;
}

/**
 * @brief 从 upgrade 进程的状态文件读取升级状态
 * upgrade 进程在 data_manage.cpp 中维护了持久化文件 /opt/cam/.config/user_data/.tvsdk_upgrade_status
 * 升级成功后 upgrade 进程会先写入 SUCCESS 状态，再 sleep(2) 后 reboot
 * 因此 reboot 后该文件中保存的是 SUCCESS(2)
 */
int read_upgrade_process_status()
{
    errno = 0;
    std::ifstream ifs(kUpgradeProcessStatusFile);
    const int openErrno = errno;
    if (!ifs.is_open())
    {
        dlog_warn("读取升级状态文件失败, path:%s, errno:%d", kUpgradeProcessStatusFile, openErrno);
        return ::System::TI_UPGRADE_NULL;
    }

    int status = ::System::TI_UPGRADE_NULL;
    if (!(ifs >> status))
    {
        dlog_error("升级状态文件内容无效, path:%s", kUpgradeProcessStatusFile);
        ifs.close();
        return ::System::TI_UPGRADE_NULL;
    }
    ifs.close();

    if (!is_upgrade_status_value(status))
    {
        dlog_error("升级状态文件状态非法, path:%s, status:%d", kUpgradeProcessStatusFile, status);
        return ::System::TI_UPGRADE_NULL;
    }

    // upgrade 进程启动时如果检测到上次状态是 RUNNING，会将其转为 FAIL
    // 所以这里不会出现 RUNNING 状态（除非 upgrade 进程正在升级中）
    return status;
}

int get_last_upgrade_status()
{
    std::lock_guard<std::mutex> lock(g_upgradeStatusMutex);
    return g_lastUpgradeStatus;
}

std::string make_upgrade_status_json(int status)
{
    ::System::UpgradeStatus_S stUpgradeStatus;
    stUpgradeStatus.nUpgradeStatus = status;
    return Convert::to_string(stUpgradeStatus);
}

void result_cached_upgrade_status(Task::CTask *task, const char *reason)
{
    // 优先从 upgrade 进程的持久化文件读取状态
    // upgrade 进程在升级成功后会先写入 SUCCESS 再 reboot，文件在 reboot 后仍然存在
    int status = read_upgrade_process_status();
    if (status == ::System::TI_UPGRADE_NULL)
    {
        // 文件不存在或读取失败，回退到内存缓存
        status = get_last_upgrade_status();
    }
    dlog_warn("获取升级状态未访问upgrade进程, reason:%s, status:%d", reason, status);
    set_last_upgrade_status(status);
    task->result(make_upgrade_status_json(status));
}
}

extern "C"
{
#include "openssl/aes.h"
}

/* 获取设备基本信息 */
void Task::System::GetDeviceInfo::handle()
{
    ::System::DeviceInfo_S stDeviceInfo;
    SystemManage::instance()->get_device_info(stDeviceInfo);

    result(Convert::to_string(stDeviceInfo));
}

/* 获取基本配置 */
void Task::System::GetDeviceConfig::handle()
{
    ::System::DeviceConfig_S stDeviceConfig;
    Convert::read_file(DEVICE_CONFIG_FILE, stDeviceConfig);

    /* 获取时区配置 */
    ::System::TimeInfo_S stTimeInfo;
    Convert::read_file(TIME_CONFIG_FILE, stTimeInfo);
    stDeviceConfig.enTimeZone = stTimeInfo.enTimeZone;

    result(Convert::to_string(stDeviceConfig));
}

/* 设置基本配置 */
void Task::System::SetDeviceConfig::handle()
{
    Log::Info_S stLogInfo;
    stLogInfo.nType = Log::OPERATION;
    stLogInfo.user = m_user;
    stLogInfo.nAction = Log::LOCAL_CONFIG;
    LogHandler::instance()->write(stLogInfo);

    ::System::DeviceConfig_S stDeviceConfig;
    Convert::to_struct(m_taskData, stDeviceConfig);

    /* 修改时间配置 */
    ::System::TimeInfo_S stTimeInfo;
    Convert::read_file(TIME_CONFIG_FILE, stTimeInfo);
    
    ::System::TimeZone_E enTimeZone = stTimeInfo.enTimeZone;
    stTimeInfo.enTimeZone = stDeviceConfig.enTimeZone;
    stTimeInfo.enDateFormat = stDeviceConfig.enDateFormat;
    if (!stDeviceConfig.strDateTime.empty())
    {
        stTimeInfo.strDateTime = stDeviceConfig.strDateTime;
    }
    dlog_info("设置基本配置同步时间配置, oldTimezone:%d, newTimezone:%d, dateTime:%s",
              static_cast<int>(enTimeZone),
              static_cast<int>(stDeviceConfig.enTimeZone),
              stDeviceConfig.strDateTime.c_str());
    CTimeManage::instance()->set_time_info(stTimeInfo);
    if (stDeviceConfig.enTimeZone != enTimeZone)
    {
        CNtpClient::instance()->bIsUpdate = true;
    }
    Convert::write_file(DEVICE_CONFIG_FILE, stDeviceConfig);

    /* 设置osd共用信息 */
    if (COsdManage::instance()->m_bInit)
    {
        COsdManage::instance()->set_osd_share_info(stDeviceConfig);
    }
    
    result(OK);
}

/* 设置设备基本信息 */
void Task::System::SetDeviceInfo::handle()
{
    ::System::DeviceInfo_S stDeviceInfo;
    Alarm::OverlayInfo_S stInfo;
    Convert::to_struct(m_taskData, stDeviceInfo);
    int nRet = Convert::write_file(DEVICE_INFO_CONFIG_FILE, stDeviceInfo);
    if (nRet != 0)
    {
        goto exit;
    }
    /* 同步修改人脸抓拍叠加信息中的设备编号 */
    nRet = CEventConfigure::instance()->get_configure(stInfo);
    if (nRet != 0)
    {
        goto exit;
    }
    stInfo.nDeviceID = stDeviceInfo.deviceID;
    nRet = CEventConfigure::instance()->set_configure(stInfo);
    if (nRet != 0)
    {
        goto exit;
    }
exit:
    result(nRet);
}

/* 获取安全服务配置 */
void Task::System::GetSecurityServicesInfo::handle()
{
    ::System::SecurityServices_S stInfo;
    SystemManage::instance()->get_security_services_info(stInfo);
    result(Convert::to_string(stInfo));   
}

/* 设置安全服务配置 */
void Task::System::SetSecurityServicesInfo::handle()
{
    ::System::SecurityServices_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    result(SystemManage::instance()->set_security_services_info(stInfo));  
}

/* 获取ssh剩余运行时长 */
void Task::System::GetSshCountdown::handle()
{
    ::System::SshCountdown_S stInfo;
    SystemManage::instance()->get_countdown(stInfo.strCountdown);
    result(Convert::to_string(stInfo));  
}


/* 获取时间配置信息 */
void Task::System::GetTimeInfo::handle()
{
    ::System::TimeInfo_S stTimeInfo;
    CTimeManage::instance()->get_time_info(stTimeInfo);
    result(Convert::to_string(stTimeInfo));
}

/* 设置时间配置信息 */
void Task::System::SetTimeInfo::handle()
{
    time_t nOldTime = time(NULL);
    ::System::TimeInfo_S stTimeInfo;
    Convert::to_struct(m_taskData, stTimeInfo);
    dlog_info("设置时间，未修改前时间戳:%lld", nOldTime);
    int nRet = CTimeManage::instance()->set_time_info(stTimeInfo);
    if(stTimeInfo.bManualSync)
    {
        /* 操作日志-手动校时 */
        Log::Info_S stLogInfo;
        stLogInfo.nType = Log::OPERATION;
        stLogInfo.user = m_user;
        stLogInfo.nAction = Log::LOCAL_MANUAL_TIME_SYNC;
        LogHandler::instance()->write(stLogInfo);
    }
    result(nRet);
}

/* 获取当前时间 */
void Task::System::GetNowTime::handle()
{
    ::System::RealTime_S stNowTime;
    ::System::TimeInfo_S stTimeInfo;
    Convert::to_struct(m_taskData, stTimeInfo);
    stNowTime.strNowTime = CTimeManage::instance()->get_current_time(::System::Language_E::ENGLISH, stTimeInfo.enDateFormat);
    result(Convert::to_string(stNowTime));

}

/* 测试时间 */
void Task::System::TestNtp::handle()
{
    ::System::TestNtp_S stTestNtp;
    Convert::to_struct(m_taskData, stTestNtp);
    std::function<void(int)> func = std::bind(static_cast<void (CTask::*)(int)>(&CTask::result), this, std::placeholders::_1);
    CTimeManage::instance()->ntp_test(stTestNtp, func);
}

/* 设备重启 */
void Task::System::Reboot::handle()
{
    Log::Info_S stLogInfo;
    stLogInfo.nType = Log::OPERATION;
    stLogInfo.user = m_user;
    stLogInfo.nAction = Log::LOCAL_REBOOT;
    LogHandler::instance()->write(stLogInfo);

    std::function<void(int)> func = 
    std::bind(static_cast<void(CTask::*)(int)>(&CTask::result), this, std::placeholders::_1);
    SystemManage::instance()->system_reboot(func);

}
/* 简单恢复 */
void Task::System::ResetSimple::handle()
{
    Log::Info_S stLogInfo;
    stLogInfo.nType = Log::OPERATION;
    stLogInfo.user = m_user;
    stLogInfo.nAction = Log::LOCAL_SIMPLE_RESET;
    LogHandler::instance()->write(stLogInfo);

    std::function<void(int)> func = 
    std::bind(static_cast<void(CTask::*)(int)>(&CTask::result), this, std::placeholders::_1);
    SystemManage::instance()->system_reset_simple(func);
}
/* 完全恢复 */
void Task::System::ResetCompletely::handle()
{
    Log::Info_S stLogInfo;
    stLogInfo.nType = Log::OPERATION;
    stLogInfo.user = m_user;
    stLogInfo.nAction = Log::LOCAL_FACTORY_RESET;
    LogHandler::instance()->write(stLogInfo);

    std::function<void(int)> func = 
    std::bind(static_cast<void(CTask::*)(int)>(&CTask::result), this, std::placeholders::_1);
    SystemManage::instance()->system_reset_complete(func);
}
/* 导出设备参数 */
void Task::System::ExportDeviceParam::handle()
{
    Log::Info_S stLogInfo;
    stLogInfo.nType = Log::OPERATION;
    stLogInfo.user = m_user;
    stLogInfo.nAction = Log::LOCAL_EXPORT_CONFIG;
    LogHandler::instance()->write(stLogInfo);

    Password password;
    Path path;
    Convert::to_struct(m_taskData, password, path);
    /* 确保密码长度正确 */
    if (password.size() > 16)
    {
        result(ERR);
        return;
    }
    password.value.append(16 - password.size(), '\0');

    ::System::AllDeviceParam_S stAllDeviceParam;
    stAllDeviceParam.strPassword = password.value;
    SystemManage::instance()->export_device_param(stAllDeviceParam);
    /* 填写默认校验码 */
    stAllDeviceParam.strCheckCode = "itcIPC";
    std::string data = Convert::to_string(stAllDeviceParam);

    /* PKCS#7 填充 */
    size_t padding = AES_BLOCK_SIZE - (data.size() % AES_BLOCK_SIZE);
    data.append(padding, static_cast<char>(padding));

    /* 确保 encData 适配填充后的数据 */
    std::vector<unsigned char> encData(data.size());

    /* AES 加密 */
    AES_KEY aesKey;
    AES_set_encrypt_key((const unsigned char *) password.value.data(), 128, &aesKey);
    for (size_t i = 0; i < data.size(); i += AES_BLOCK_SIZE)
    {
        AES_encrypt((const unsigned char *) data.data() + i, encData.data() + i, &aesKey);
    }

    /* 写入文件 */
    if (path.value.empty())
    {
        path.value = DEVICE_PARAM_FILE;
    }
    else if (path.value.back() == '/')
    {
        path.value += "IpcDeviceParams.txt";
    }
    std::ofstream file(path.value, std::ios::out | std::ios::binary);
    if (!file.is_open())
    {
        result(ERR);
        return;
    }
    file.write((char *) encData.data(), encData.size());
    file.flush();
    file.close();
    result(Convert::to_string(path));
}

/* 导入设备参数 */
void Task::System::ImportDeviceParam::handle()
{
    Password password;
    Path path;
    Convert::to_struct(m_taskData, password, path);

    /* 查看是否是txt文件 */
    if (path.value.rfind(".txt") == std::string::npos)
    {
        /* 删除导入的设备参数 */
        path.value = std::string(UPLOAD_PATH) + path.value;
        remove(path.value.c_str());
        result(ERR);
        return;
    }

    /* 确保密码长度正确 */
    if (password.size() > 16)
    {
        result(ERR);
        return;
    }
    password.value.append(16 - password.size(), '\0');

    /* 读取文件 */
    path.value = std::string(UPLOAD_PATH) + path.value;
    std::ifstream file(path.value, std::ios::in | std::ios::binary);
    if (!file.is_open())
    {
        dlog_error("读取文件失败");
        /* 删除导入的设备参数 */
        remove(path.value.c_str());
        result(ERR);
        return;
    }

    std::vector<unsigned char> encData(std::istreambuf_iterator<char>(file), {});
    file.close();
    /* 读取并解密 */
    std::string decData(encData.size(), '\0');

    AES_KEY aesKey;
    AES_set_decrypt_key((const unsigned char *) password.value.data(), 128, &aesKey);
    for (size_t i = 0; i < encData.size(); i += AES_BLOCK_SIZE)
    {
        AES_decrypt(encData.data() + i, (unsigned char *) decData.data() + i, &aesKey);
    }

    /* 去除 PKCS#7 填充 */
    size_t padVal = static_cast<size_t>(decData.back());
    if (padVal > 0 && padVal <= AES_BLOCK_SIZE && decData.size() >= padVal)
    {
        decData.resize(decData.size() - padVal);
    }
    else
    {
        dlog_error("解密失败: 填充数据错误");
        /* 删除导入的设备参数 */
        remove(path.value.c_str());
        result(ERR);
        return;
    }

    ::System::AllDeviceParam_S stAllDeviceParam;
    Convert::to_struct(decData, stAllDeviceParam);

    /* 确保密码匹配 */
    password.value.resize(stAllDeviceParam.strPassword.size());
    if (password.value != stAllDeviceParam.strPassword)
    {
        dlog_error("密码不匹配 %s != %s", password.value.c_str(), stAllDeviceParam.strPassword.c_str());
        /* 删除导入的设备参数 */
        remove(path.value.c_str());
        result(ERR);
        return;
    }

    /* 校验校验码 */
    if (stAllDeviceParam.strCheckCode != "itcIPC")
    {
        dlog_error("校验码错误");
        /* 删除导入的设备参数 */
        remove(path.value.c_str());
        result(ERR);
        return;
    }

    int nRet = SystemManage::instance()->import_device_param(stAllDeviceParam);
    sync();
    if (nRet == 0)
    {
        /* 操作日志-导入设备配置文件 */
        Log::Info_S stLogInfo;
        stLogInfo.nType = Log::OPERATION;
        stLogInfo.user = m_user;
        stLogInfo.nAction = Log::LOCAL_IMPORT_CONFIG;
        stLogInfo.host = CWebServer::instance()->get_userclient_ip();
        LogHandler::instance()->write(stLogInfo);
    }

    /* 删除上传目录的参数txt文件 */
    if (remove(path.value.c_str()) == 0)
    {
        dlog_info("删除导入的参数txt文件[%s]成功: ", path.value.c_str());
    }
    else
    {
        dlog_error("删除导入的参数txt文件[%s]失败:[%s]", path.value.c_str(), strerror(errno));
    }

    result(nRet);
}

/* 设置升级维护 */
void Task::System::SetUpgradeMaintain::handle()
{
    ::System::UpgradeMaintain_S stUpgradeMaintain;
    Convert::to_struct(m_taskData, stUpgradeMaintain);
    result(SystemManage::instance()->set_automatic_maintain_info(stUpgradeMaintain));
}
/* 获取升级维护 */
void Task::System::GetUpgradeMaintain::handle()
{
    ::System::UpgradeMaintain_S stUpgradeMaintain;
    SystemManage::instance()->get_automatic_maintain_info(stUpgradeMaintain);
    result(Convert::to_string(stUpgradeMaintain));
}
/* 系统升级 */
void Task::System::DoUpgrade::handle()
{
    ::System::UpgradeInfo_S stUpgradeInfo;
    Convert::to_struct(m_taskData, stUpgradeInfo);
    // if(verify_requester(Common::REQUESTER_WEB))
    // {
    //     stUpgradeInfo.strUpgradePath = "/root/test/" + stUpgradeInfo.strUpgradePath;
    // }
    /* 操作日志：记录用户已发起本地升级，不代表升级包已校验或安装成功。 */
    Log::Info_S stLogInfo;
    stLogInfo.nType = Log::OPERATION;
    stLogInfo.nAction = Log::LOCAL_UPGRADE;
    Json::get(m_data.c_str(), "UserName", stLogInfo.user);
    stLogInfo.host = m_strUserIp;
    LogHandler::instance()->write(stLogInfo);

    /* 通知upgrade */
    stUpgradeInfo.strUpgradePath = UPLOAD_PATH + stUpgradeInfo.strUpgradePath;
    dlog_trace("升级路径[%s]", stUpgradeInfo.strUpgradePath.c_str());
    std::string data = Convert::to_string(stUpgradeInfo);
    fill_head(data, AC_DO_UPGRADE);
    CUpgradeClient::instance()->send(data, AC_DO_UPGRADE);

    result(IpcRet_E::OK);
}
/* 获取系统升级状态信息 */
void Task::System::GetUpgradeStatus::handle()
{
    /* 通知upgrade */
    CUpgradeClient::instance()->send(m_data, AC_GET_UPGRADE_STATUS);
    deal_result([this](std::string resultData){
        std::string data = get_data(resultData);
        ::System::UpgradeStatus_S stUpgradeStatus;
        Convert::to_struct(data, stUpgradeStatus);
        std::string strData = Convert::to_string(stUpgradeStatus);
        result(strData);
    });
}
/* 导入/设置升级包 */
void Task::System::SetUpgrade::handle()
{
    result(IpcRet_E::ERR_IMPORT_UPGRADE);
}
/* 检测是否有升级包 */
void Task::System::CheckUpgrade::handle()
{
    result(IpcRet_E::ERR_CHECK_UPGRADE);
}

void Task::System::FindLog::handle()
{
    Log::RetrievalCond_S stRetrievalCond;
    Common::PageInfo_S stPageInfo;
    Convert::to_struct(m_taskData, stRetrievalCond, stPageInfo);
    std::vector<Log::Info_S> logInfos;
    int nRet = LogHandler::instance()->find(stRetrievalCond, stPageInfo, logInfos);
    if (nRet < 0)
    { 
        dlog_error("获取日志失败:%d", nRet);
        result(std::string(), nRet);
        return;
    }
    std::string strSend = Convert::to_string(logInfos, stPageInfo);
    result(strSend);
}

void Task::System::ExportLog::handle()
{
    static constexpr int EXPORT_DEBOUNCE_SEC = 2;
    static std::atomic<time_t> s_lastExportTime{0};
    time_t now = time(NULL);
    time_t last = s_lastExportTime.load();
    if (now - last < EXPORT_DEBOUNCE_SEC)
    {
        dlog_warn("导出日志请求过于频繁，已忽略 (距上次导出%lds)", (long)(now - last));
        return;
    }
    s_lastExportTime.store(now);

    Log::RetrievalCond_S stRetrievalCond;
    Common::PageInfo_S stPageInfo;
    Convert::to_struct(m_taskData, stRetrievalCond, stPageInfo);
    std::vector<Log::Info_S> logInfos;
    int nRet = LogHandler::instance()->find(stRetrievalCond, stPageInfo, logInfos);
    if (nRet < 0)
    { 
        dlog_error("获取日志失败:%d", nRet);
        result(std::string(), nRet);
        return;
    }
    std::string strSend = Convert::to_string(logInfos, stPageInfo);
    result(strSend);
}

/* 测试日志服务器 */
void Task::System::TestLogServer::handle()
{
    int nRet = -1;
    ::System::LogServerInfo_S stLogServerInfo;
    Convert::to_struct(m_taskData, stLogServerInfo);
    nRet = COperationClient::instance()->send(static_cast<const void*>(m_data.c_str()), m_data.size(), m_nActionCode);
    if ( nRet  < 0)
    {
        result(ERR);
    }
    else
    {
        deal_result([this](std::string data)
        {
            int nRet;
            size_t pos = data.find("\"Return\""); 
            pos = data.find(":", pos);           
            std::istringstream(data.substr(pos + 1)) >> nRet; 
            result(nRet);
        });
        
    }
}
/* 设置日志服务器 */
void Task::System::SetLogServer::handle()
{
    ::System::LogServerInfo_S stLogServerInfo;
    Convert::to_struct(m_taskData, stLogServerInfo);
    /* 用户关闭日志上传时，同时关闭MQTT连接 */
    if (!stLogServerInfo.bLogUpload)
    {
        stLogServerInfo.bEnable = false;
    }
    Convert::write_file(LOG_SERVER_INFO_FILE, stLogServerInfo);
    /* 同步日志上传开关到LogHandler，实时生效 */
    LogHandler::instance()->setLogUpload(stLogServerInfo.bLogUpload);
    /* 用更新后的数据通知运维进程重新连接MQTT */
    std::string strUpdated = Convert::to_string(stLogServerInfo);
    COperationClient::instance()->send_withHead(strUpdated, m_nActionCode);
    result(OK);
}
/* 获取日志服务器 */
void Task::System::GetLogServer::handle()
{
    ::System::LogServerInfo_S stLogServerInfo;
    Convert::read_file(LOG_SERVER_INFO_FILE, stLogServerInfo);
    result(Convert::to_string(stLogServerInfo));
}
/* 获取IP地址过滤 */
void Task::System::GetIpFilterInfo::handle()
{
    ::System::IpFilterConfigInfo_S stInfo;
    CIpFilterManage::instance()->get_ip_filter_info(stInfo);
    result(Convert::to_string(stInfo));   
}
/* 设置IP地址过滤 */
void Task::System::SetIpFilterInfo::handle()
{
    ::System::IpFilterConfigInfo_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    result(CIpFilterManage::instance()->set_ip_filter_info(stInfo));  
}
/* 添加IP过滤地址 */
void Task::System::AddIpFilterAddress::handle()
{
    std::string strIp;
    Convert::to_struct(m_taskData, strIp);
    result(CIpFilterManage::instance()->add_ip(strIp));  
}
/* 删除IP过滤地址 */
void Task::System::RemoveIpFilterAddress::handle()
{
    std::vector<std::string> IpList;
    Convert::to_struct(m_taskData, IpList);

    int failCount = 0;
    for (const auto& ip : IpList)
    {
        int ret = CIpFilterManage::instance()->remove_ip(ip);
        if (ret != OK)
        {
            ++failCount;
            //dlog_error("删除IP过滤地址失败: %s", ip.c_str());
        }
    }

    result(failCount == 0 ? OK : ERR);
}

/* 修改IP过滤地址 */
void Task::System::ModifyIpFilterAddress::handle()
{
    ::System::IpFilterModify_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    result(CIpFilterManage::instance()->modify_ip(stInfo));  
}

/* 获取外设配置 */
void Task::System::GetPeripheralConfig::handle()
{
    ::System::Peripheral_S stInfo;
    const int nRet = CPeripheralManage::instance()->get_fill_light_config(stInfo);
    if (nRet != OK)
    {
        result(nRet);
        return;
    }
    result(Convert::to_string(stInfo));
}
/* 设置外设配置 */
void Task::System::SetPeripheralConfig::handle()
{
    ::System::Peripheral_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    result(CPeripheralManage::instance()->set_fill_light_config(stInfo));
}

/* 获取智能资源分配-智能事件启用情况 */
void Task::System::GetSmartEventEnableStatus::handle()
{
    ::Event::SmartResourceAlloc_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo.stSmartEventEnableStatus);
    CEventResource::instance()->get_canEventResource_rules(stInfo.stSmartEventEnableStatus, stInfo.aCanEventTypeArray);
    CEventResource::instance()->enableStatus_convertArray(stInfo.stSmartEventEnableStatus, stInfo.aSmartEventEnableStatusArray);
    result(Convert::to_string(stInfo));
}

/* 设置智能资源分配-智能事件启用情况 */
void Task::System::SetSmartEventEnableStatus::handle()
{
    /* 从任务数据中解析出新的启用状态 */
    ::Event::SmartEventEnableStatus_S stNewInfo;
    Convert::to_struct(m_taskData, stNewInfo);

#if CAP_SMART_EVENT_PERF_LIMIT // 智能事件性能限制
    // ! /* 性能限制 */
    /* 对于部分智能事件，需判断是否开启了smart编码，如果开启，不允许进行设置 */
    if (stNewInfo.bUnattendedObject || stNewInfo.bObjectRemoval || stNewInfo.bCrowdGathering)
    {
        Video_NS::VideoConfig_S stVideoConfig;
        /* 仅判断第一码流 */
        stVideoConfig.nId = 0;
        CAVConfigure::instance()->get_configure(stVideoConfig);
        if (stVideoConfig.bSmartEnable)
        {
            result(ERR_WEB_SMART_SET_EVENT);
            return;
        }
    }
#endif

    /* 获取设置前的旧状态，用于后续比较 */
    ::Event::SmartEventEnableStatus_S stOldInfo;
    CEventConfigure::instance()->get_configure(stOldInfo);
    
    /* 保存新的智能事件启用状态配置 */
    CEventConfigure::instance()->set_configure(stNewInfo);

    /* 检查是否有事件被禁用，并更新其具体配置 */
    CEventResource::instance()->update_event_configurations_on_disable(stOldInfo, stNewInfo);
#ifdef SCENE_INTELLIGENT_ANALYSIS
    CEventResource::instance()->update_event_configurations_on_enable(stOldInfo, stNewInfo);
#endif

    /* 根据当前启用的智能事件，获取还可以启用的事件列表 */
    std::vector<::Event::Type_E> vCanEnableEvent;
    CEventResource::instance()->get_canEventResource_rules(stNewInfo, vCanEnableEvent);
    result(Convert::to_string(vCanEnableEvent)); 
}

/* 获取Metadata配置 */
void Task::System::GetMetadataConfig::handle()
{
    ::Event::MetadataConfig_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);
    result(Convert::to_string(stInfo));
}

/* 设置Metadata配置 */
void Task::System::SetMetadataConfig::handle()
{
    ::Event::MetadataConfig_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    int nRet = CEventConfigure::instance()->set_configure(stInfo);
    result(nRet);
}
