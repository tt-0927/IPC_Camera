/**
 * @FilePath     : system_manage.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-03-02 15:32:46
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-22 11:17:41
 * @Description  : 系统管理
 */

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <filesystem>
#include <sys/sysinfo.h>
#include <dirent.h>

#include "system_manage.h"

#include "system_convert.h"
#include "event_configure.h"
#include "action_code.h"
#include "network_define.h"
#include "convert_interface.h"
#include "user_manage.h"
#include "time_manage.h"
#include "xml_base.h"
#include "email_manage.h"
#include "ca_manage.h"
#include "network_manage.h"
#include "plugin_version_utils.h"
#include "share_define.h"
#include "base_define.h"
#include "log_handler.h"
#include "task_publish.h"
#include "event_linkage.h"
#include "event_alarm.h"
#include "gpio_ctrl.h"
#include "record_ctrl.h"
#include "capture_ctrl.h"
#if CAP_SYSTEM_REBOOT_MUTE // 系统重启静音处理
#include "stream_ao.h"
#endif

SystemManage::SystemManage()
    : m_deviceInfoFile(DEVICE_INFO_CONFIG_FILE),
      m_deviceConfigFile(DEVICE_CONFIG_FILE),
      m_securityServicesFile(SECURITY_SERVICES_FILE),
      m_upgradeMaintainFile(UPGRADE_MAINTAIN_FILE)
{
}

IpcRet_E SystemManage::init()
{
    System::DeviceInfo_S stDeviceInfo;
    get_device_info(stDeviceInfo);

    System::DeviceConfig_S stDeviceConfig;
    get_device_config(stDeviceConfig);

    std::filesystem::create_directories(CRON_DIR);

    /*初始化默认ssh账号，即网页账号*/
    init_ssh_admin_group();
    /* 从用户管理中获取admin的实际密码，避免重启后密码不一致 */
    std::string strAdminPassword = CUserManage::instance()->get_passwd(USER_DEFAULT_NAME);
    if (strAdminPassword.empty())
    {
        /* 如果获取失败（首次启动），使用默认密码 */
        dlog_debug("未找到admin用户密码，使用默认密码初始化");
        strAdminPassword = USER_DEFAULT_PASSWD;
    }
    else
    {
        dlog_debug("使用用户管理中的admin密码初始化SSH账号");
    }
    add_ssh_admin(USER_DEFAULT_NAME, strAdminPassword.c_str());

    /* 安全服务初始化 */
    System::SecurityServices_S stInfo;
    get_security_services_info(stInfo);
    /* 开机初始化，默认关闭ssh */
    stInfo.stSshAdmin.bSshEnable = false;
    set_security_services_info(stInfo);

    /* 自动维护初始化 */
    System::UpgradeMaintain_S stUpgradeMaintain;
    get_automatic_maintain_info(stUpgradeMaintain);
    stUpgradeMaintain.strCurrentVersion = SYSTEM_VERSION;
    set_automatic_maintain_info(stUpgradeMaintain);

    /* io输入初始化 */
    m_ioInput.start_detectiond();
    IoInput::Observer observer = std::bind(&SystemManage::callback_ioInputEvent, this, std::placeholders::_1);
    m_ioInput.set_observer(observer);
    std::map<int, bool> stListenMap;

    std::set<Alarm::IoInputInfo_S> stIoInputInfos;
    /* 获取所有IO输入信息 */ 
    CEventConfigure::instance()->get_configure(stIoInputInfos);
    // 收集需要启用的IO输入编号
    for (auto &stIOInputInfo : stIoInputInfos)
    {
        
        if (stIOInputInfo.nDealType != 0)
        {
            stListenMap[stIOInputInfo.nIoNumer] = stIOInputInfo.bNormallyOpen;
        }
    }
    /* 启用收集到的IO输入编号 */ 
    enable_ioInputNumber(stListenMap);

    /* 操作日志-开机 */
    Log::Info_S stLogInfo;
    stLogInfo.nType = Log::OPERATION;
    stLogInfo.nAction = Log::LOCAL_STARTUP;
    LogHandler::instance()->write(stLogInfo);

    /* 开启设备时长日志信息记录 */
    SystemManage::instance()->start_logUptime();

    return OK;
}

IpcRet_E SystemManage::deinit()
{
    m_ioInput.stop_detectiond();
    stop_logUptime();

    return OK;
}

int SystemManage::set_device_config(System::DeviceConfig_S stDeviceConfig)
{
    Convert::write_file(m_deviceConfigFile, stDeviceConfig);

    return 0;
}

int SystemManage::get_device_config(System::DeviceConfig_S &stDeviceConfig)
{
    if (0 == Convert::read_file(m_deviceConfigFile, stDeviceConfig))
    {
        return 0;
    }
    stDeviceConfig.strDateTime = CTimeManage::instance()->get_current_time(System::Language_E::ENGLISH, stDeviceConfig.enDateFormat);

    Convert::write_file(m_deviceConfigFile, stDeviceConfig);

    return 0;
}

int SystemManage::set_device_info(System::DeviceInfo_S stDeviceInfo)
{
    Convert::write_file(m_deviceInfoFile, stDeviceInfo);

    return 0;
}

int SystemManage::get_device_info(System::DeviceInfo_S &stDeviceInfo)
{
    /* 尝试从文件读取设备信息 */
    System::DeviceInfo_S stSavedInfo;
    /* 文件是否存在 */
    bool bFileExists = (OK == Convert::read_file(m_deviceInfoFile, stSavedInfo));
    /* 是否需要更新配置文件 */
    bool bNeedUpdate = false;

    /* 设备型号 */
    stDeviceInfo.strUnitTpye = DEVICE_CODE;
    /* 设备序列号 */
    stDeviceInfo.serialNumber = get_cpu_serialNumber();
    /* 硬件版本 */
    stDeviceInfo.hardwareVersion = HARDWARE_VERSION;
    /* 系统版本 */
    stDeviceInfo.systemVersion = SYSTEM_VERSION;
    /* 插件版本必须与 S81appinit 当前指向的实际插件文件保持一致。 */
    stDeviceInfo.pluginVersion = PluginVersionUtils_NS::get_active_version();
    /* web版本 */
    stDeviceInfo.webVersion = WEB_VERSION;
    /* 报警输入个数 */
    stDeviceInfo.nAlarmInputCount = GPIO_INPUT_COUNT;
    /* 报警输出个数 */
    stDeviceInfo.nAlarmOutputCount = GPIO_OUTPUT_COUNT;

    /* 检查是否需要更新配置文件 */
    if (bFileExists)
    {
        /* 文件已存在，同步用户可能修改过的字段 */
        stDeviceInfo.deviceName = stSavedInfo.deviceName;
        stDeviceInfo.deviceID = stSavedInfo.deviceID;
        /* 比较信息，任一信息不一致则需要更新配置文件 */
        if (stDeviceInfo != stSavedInfo)
        {
            bNeedUpdate = true;
            dlog_info("检测到版本或其他信息更新，需要刷新配置文件");
        }
        else
        {
            /* 版本一致，直接返回 */
            return OK;
        }
    }
    else
    {
        /* 配置文件不存在，需要创建 */
        bNeedUpdate = true;
        dlog_info("设备信息配置文件不存在，创建新配置");
    }

    /* 更新或创建设备信息 */
    if (bNeedUpdate)
    {
        /* 写入配置文件 */
        if (OK != Convert::write_file(m_deviceInfoFile, stDeviceInfo))
        {
            dlog_error("写入设备信息配置文件失败");
            return ERR;
        }
        dlog_info("设备信息配置文件已更新");
    }

    return OK;
}

/* 设备重启 */
int SystemManage::system_reboot(std::function<void(int)> result)
{
    auto thrRun = [](std::function<void(int)> result)
    {
        CCaptureCtrl::instance()->stop_capture();
        CRecordCtrl::instance()->stop_record();
        result(IpcRet_E::OK_SYSTEM_REBOOT);
        /* 延时2秒重启 */
        std::this_thread::sleep_for(std::chrono::seconds(2));  
#if CAP_SYSTEM_REBOOT_MUTE // 系统重启静音处理
        /* 静音，防止重启喇叭异响 */
        CStreamAo::instance()->update_audioOutputType(Audio_NS::AudioOutputType_E::MUTE);
#endif
        system("sync;reboot");
    };

    std::thread thr(thrRun, result);
    thr.detach();

    return 0;
}

/* 简单恢复 */
int SystemManage::system_reset_simple(std::function<void(int)> result)
{
    auto thrRun = [](std::function<void(int)> result)
    {
        /* 删除配置文件 */
        std::string strDeleteCommand = "rm -f " USER_DATA_PATH "*";
        int nRet = std::system(strDeleteCommand.c_str());

        if (nRet < 0)
        {
            result(IpcRet_E::ERR_RESET_SIMPLE);
        }
        else
        {
            result(IpcRet_E::OK_RESET_SIMPLE);
        }
    };

    std::thread thr(thrRun, result);
    thr.detach();
    return 0;
}

/* 完全恢复 */
int SystemManage::system_reset_complete(std::function<void(int)> result)
{
    auto thrRun = [](std::function<void(int)> result)
    {
        /* 网络参数重置 */
        Network::Info_S stNetInfo;
        CNetworkManage::instance()->set_system_networkInfo(stNetInfo,false);

        /* 删除配置文件 */
        std::string strDeleteAllJson = "rm -f " USER_DATA_PATH "*";
        std::system(strDeleteAllJson.c_str());

        /* 删除数据库 */
        std::string strDeleteDb = "rm -f " DATABASE_PATH "*";
        std::system(strDeleteDb.c_str());

        /* 日志删除 */
        std::string strDeleteLog = "rm -f " LOG_PATH "*";
        std::system(strDeleteLog.c_str());

        /* 延时2秒 */
        std::this_thread::sleep_for(std::chrono::seconds(2));

        result(IpcRet_E::OK_RESET_COMPLETE);
    };

    std::thread thr(thrRun, result);
    thr.detach();
    return 0;
}

std::string SystemManage::get_cpu_serialNumber()
{
    std::ifstream cpuinfoFile("/proc/cpuinfo");
    if (!cpuinfoFile.is_open())
    {
        return "Error: Unable to open /proc/cpuinfo";
    }

    std::string line;
    const std::string strKey = "Serial";
    while (std::getline(cpuinfoFile, line))
    {
        /* 查找包含 "Serial" 的行 */
        if (line.find(strKey) != std::string::npos)
        {
            /* 提取序列号部分 */
            size_t pos = line.find(':');
            if (pos != std::string::npos)
            {
                std::string serial = line.substr(pos + 1);
                size_t start = serial.find_first_not_of(" \t");
                serial = (start == std::string::npos) ? "" : serial.substr(start);
                return serial;
            }
        }
    }

    return "Error: Serial number not found";
}

bool SystemManage::cron_check(const char *pTask)
{
    char achCmd[LENGTH256] = {0};
    snprintf(achCmd, sizeof(achCmd), CRON_CHECK, pTask);
    if (0 != std::system(achCmd))
    {
        dlog_debug("检查自动维护任务不存在");
        return false;
    }
    dlog_debug("检查自动维护任务已存在");
    return true;
}
int SystemManage::cron_add(const char *pTask)
{
    char achCmd[LENGTH256] = {0};
    snprintf(achCmd, sizeof(achCmd), CRON_ADD, pTask);
    if (0 != std::system(achCmd))
    {
        dlog_error("添加自动维护任务失败");
        return ERR;
    }
    dlog_debug("添加自动维护任务成功");
    return OK;
}
int SystemManage::cron_delete(const char *pTask)
{
    char achCmd[LENGTH256] = {0};
    snprintf(achCmd, sizeof(achCmd), CRON_DELETE, pTask);
    if (0 != std::system(achCmd))
    {
        dlog_error("删除自动维护任务失败");
        return ERR;
    }
    dlog_debug("删除自动维护任务成功");
    return OK;
}

/* 设置自动维护信息 */
void SystemManage::get_automatic_maintain_info(System::UpgradeMaintain_S& stInfo)
{
    if (Convert::read_file(m_upgradeMaintainFile, stInfo))
    {
        dlog_info("未找到升级维护配置信息文件，生成默认配置");
        Convert::write_file(m_upgradeMaintainFile, stInfo);
    }
}

/* 获取自动维护信息 */
int SystemManage::set_automatic_maintain_info(System::UpgradeMaintain_S stInfo)
{
    /* 检查任务 */
    if (cron_check(REBOOT_CMD))
    {
        /* 删除任务 */
        if (cron_delete(REBOOT_CMD))
        {
            return ERR;
        }
    }

    FILE *pFp = popen(CROND_FIND_PID, "r");
    if (nullptr != pFp)
    {
        char achPid[LENGTH256] = {0};
        if (nullptr != fgets(achPid, sizeof(achPid), pFp))
        {
            std::string strCmd = "kill " + std::string(achPid);
            std::system(strCmd.c_str());
            dlog_debug("杀死crond服务");
        }
        pclose(pFp);
    }

    if (!stInfo.bAutoMaintain)
    {
        System::SecurityServices_S stSeSecurityServicesInfo;
        Convert::read_file(m_securityServicesFile, stSeSecurityServicesInfo);
        if (stSeSecurityServicesInfo.stSshAdmin.bSshEnable)
        {
            if (0 != system("crond"))
            {
                dlog_error("执行crond失败");
                return ERR;
            }
            dlog_debug("启动crond服务");
        }
        Convert::write_file(m_upgradeMaintainFile, stInfo);

        return OK;
    }

    if (0 != system("crond"))
    {
        dlog_error("执行crond失败");
        return ERR;
    }

    int hour, minute;
    char chDummy;
    std::istringstream timeStream(stInfo.strMaintainTime);
    if (!(timeStream >> hour >> chDummy >> minute))
    {
        dlog_error("无效的时间格式");
        return ERR;
    }

    /* 定时任务字符串 */
    std::string strCronJob = std::to_string(minute) + " " +
                             std::to_string(hour) + " " +
                             "* * " +
                             std::to_string(static_cast<int>(stInfo.enWeek)) +
                             " " +
                             REBOOT_CMD;

    /* 添加任务 */
    if (cron_add(strCronJob.c_str()))
    {
        return ERR;
    }

    Convert::write_file(m_upgradeMaintainFile, stInfo);

    return 0;
}

void SystemManage::get_security_services_info(System::SecurityServices_S &stInfo)
{
    if (0 != Convert::read_file(m_securityServicesFile, stInfo))
    {
        Convert::write_file(m_securityServicesFile, stInfo);
        return;
    }
}

/* 初始化 SSH 用户组 */
void SystemManage::init_ssh_admin_group()
{
    m_sshAccountManager.init_admin_group();
}

/* 设置 SSH 密码 */
int SystemManage::set_ssh_password(const char *username, const char *password)
{
    return m_sshAccountManager.set_password(username, password);
}

/* 添加或更新 SSH 账号 */
int SystemManage::add_ssh_admin(const char *username, const char *password)
{
    return m_sshAccountManager.add_admin(username, password);
}

/* 删除 SSH 账号 */
int SystemManage::del_ssh_admin(const char *username)
{
    return m_sshAccountManager.del_admin(username);
}

int SystemManage::set_security_services_info(System::SecurityServices_S stInfo)
{
    System::UpgradeMaintain_S stUpgradeMaintainInfo;
    get_automatic_maintain_info(stUpgradeMaintainInfo);

    const int nRet = m_sshServiceManager.apply(stInfo, stUpgradeMaintainInfo.bAutoMaintain);
    if (nRet != OK)
    {
        dlog_error("设置安全服务信息失败，SSH 开关[%d]，端口[%d]",
                   stInfo.stSshAdmin.bSshEnable,
                   stInfo.stSshAdmin.nSshPort);
        return nRet;
    }

    if (OK != Convert::write_file(m_securityServicesFile, stInfo))
    {
        dlog_error("写入安全服务配置文件失败");
        return ERR;
    }

    dlog_info("设置安全服务信息成功，SSH 开关[%d]，端口[%d]",
              stInfo.stSshAdmin.bSshEnable,
              stInfo.stSshAdmin.nSshPort);
    return OK;
}

int SystemManage::get_countdown(std::string &strCountdown)
{
    System::SecurityServices_S stInfo;
    get_security_services_info(stInfo);
    return m_sshServiceManager.get_countdown(stInfo.stSshAdmin.nSshPort, strCountdown);
}

/* 导出设备参数 */
int SystemManage::export_device_param(System::AllDeviceParam_S &stAllDeviceParam)
{
    /* 预览 */
    Convert::read_file(PREVIEW_CONFIG_FILE, stAllDeviceParam.stPreviewInfo);                                /* 预览 */
    /* 回放 */
    /* 图片 */
    /* 配置 */
    /* 本地设置 */
    Convert::read_file(WEB_PLUGIN_CONFIG_FILE, stAllDeviceParam.stParam);                                   /* 本地设置 */
    /* 系统配置 */
    Convert::read_file(DEVICE_CONFIG_FILE, stAllDeviceParam.stDeviceConfig);                                /* 系统设置-基本配置 */
    Convert::read_file(DEVICE_INFO_CONFIG_FILE, stAllDeviceParam.stDeviceInfo);                             /* 系统设置-基本信息 */
    Convert::read_file(TIME_CONFIG_FILE, stAllDeviceParam.stTimeInfo);                                      /* 系统设置-时间配置 */
                                                                                                            /* 系统设置-智能资源分配 */
    Convert::read_file(UPGRADE_MAINTAIN_FILE, stAllDeviceParam.stUpgradeMaintain);                          /* 系统维护-升级维护 */
    Convert::read_file(LOG_SERVER_INFO_FILE, stAllDeviceParam.stLogServerInfo);                             /* 系统维护-安全审计日志 */
    Convert::read_file(SECURITY_CERT_CONFIG_FILE, stAllDeviceParam.stSecurityCert);                         /* 安全管理-认证方式 */
    Convert::read_file(IP_FILTER_CONFIG_FILE, stAllDeviceParam.stIpFilterConfigInfo);                       /* 安全管理-IP地址过滤 */
    Convert::read_file(SECURITY_SERVICES_FILE, stAllDeviceParam.stSecurityServices);                        /* 安全管理-安全服务 */
                                                                                                            /* 安全管理-客户端证书 */
                                                                                                            /* 安全管理-CA证书 */
    CUserManage::instance()->find(User::Find_S(), stAllDeviceParam.vecUserInfo);                            /* 用户管理-用户管理 */
                                                                                                            /* 用户管理-在线用户 */
    /* 网络配置 */
    Convert::read_file(NETWORK_CONFIG_FILE, stAllDeviceParam.stNetInfo);                                    /* 基本配置-TCP/IP */
    Convert::read_file(PORT_CONFIG_FILE, stAllDeviceParam.stPortConfig);                                    /* 基本配置-端口 */
    Convert::read_file(UPNP_CONFIG_FILE, stAllDeviceParam.stPortMapConfig);                                 /* 基本配置-端口映射 */
    Convert::read_file(SNMP_CONFIG_FILE, stAllDeviceParam.stSnmpConfig);                                    /* 高级配置-SNMP */
    Convert::read_file(EMAIL_CONFIG_FILE, stAllDeviceParam.stEmailInfo);                                    /* 高级配置-Email */
    Convert::read_file(GB28181_CLIENT_CONFIG_FILE, stAllDeviceParam.stGB28181Client);                       /* 高级配置-平台接入 */
    Convert::read_file(GM_CERT_INFO_FILE, stAllDeviceParam.stGmCertFileInfo);                               /* 高级配置-国标证书管理 */
    Convert::read_file(HTTPS_CONFIG_FILE, stAllDeviceParam.stHttpsConfigInfo);                              /* 高级配置-HTTPS */
    Convert::read_file(QOS_CONFIG_FILE, stAllDeviceParam.stQosConfigInfo);                                  /* 高级配置-Qos */
    Convert::read_file(ONVIF_CONFIG_FILE, stAllDeviceParam.stOnvifConfigInfo);                              /* 高级配置-集成协议 */
                                                                                                            /* 高级配置-网络服务 */
    /* 事件配置 */
    Convert::read_file(EVENT_MOTION_DETECTION_CONFIG_FILE, stAllDeviceParam.stMotionDetection);             /* 普通事件-移动侦测 */
    Convert::read_file(EVENT_HIDE_ALARM_CONFIG_FILE, stAllDeviceParam.stHideAlarm);                         /* 普通事件-遮挡报警 */
    Convert::read_file(EVENT_ABNORMAL_ALARM_CONFIG_FILE, stAllDeviceParam.stAbnormalDetection);             /* 普通事件-异常 */
    Convert::read_file(EVENT_SOUND_ALARM_OUTPUT_CONFIG_FILE, stAllDeviceParam.stSoundOutputAlarm);          /* 普通事件-声音报警输出 */
    Convert::read_file(EVENT_ALARM_INPUT_CONFIG_FILE, stAllDeviceParam.stIoInputInfo);                      /* 普通事件-报警输入 */
    Convert::read_file(EVENT_ALARM_OUTPUT_CONFIG_FILE, stAllDeviceParam.stIoOutputInfo);                    /* 普通事件-报警输出 */
    Convert::read_file(EVENT_FLASHING_LIGHT_ALARM_OUTPUT_CONFIG_FILE, stAllDeviceParam.stFlashInfo);        /* 普通事件-闪光灯报警输出 */
    Convert::read_file(EVENT_PIR_ALARM_CONFIG_FILE, stAllDeviceParam.stPirAlarmInfo);                       /* 普通事件-PIR报警 */
    Convert::read_file(EVENT_LINE_CROSSING_DETECTION_CONFIG_FILE, stAllDeviceParam.stBoundaryDetection);    /* 周界事件-越界侦测 */
    Convert::read_file(EVENT_REGIONAL_INTRUSION_DETECTION_CONFIG_FILE, stAllDeviceParam.stFieldDetection);  /* 周界事件-区域入侵侦测 */
    Convert::read_file(EVENT_ENTER_REGION_DETECTION_CONFIG_FILE, stAllDeviceParam.stEntranceDetection);     /* 周界事件-进入区域侦测 */
    Convert::read_file(EVENT_LEAVE_REGION_DETECTION_CONFIG_FILE, stAllDeviceParam.stExitingDetection);      /* 周界事件-离开区域侦测 */
                                                                                                            /* 场景智能-翻阅围栏识别 */
                                                                                                            /* 场景智能-离岗识别 */
                                                                                                            /* 场景智能-违规变道识别 */
                                                                                                            /* 场景智能-逆行识别 */
                                                                                                            /* 场景智能-非机动车闯入识别 */
                                                                                                            /* 场景智能-应急车道占用识别 */
                                                                                                            /* 场景智能-行人闯入识别 */
                                                                                                            /* 场景智能-其他智能事件 */
    Convert::read_file(EVENT_AUDIO_ANOMALY_DETECTION_CONFIG_FILE, stAllDeviceParam.stAudioAnomaly);         /* Smart事件-音频异常侦测 */
    Convert::read_file(EVENT_SCENE_CHANGE_DETECTION_CONFIG_FILE, stAllDeviceParam.stSceneChange);           /* Smart事件-场景变更侦测 */
    Convert::read_file(EVENT_FACE_DETECTION_CONFIG_FILE, stAllDeviceParam.stFaceDetection);                 /* Smart事件-人脸侦测 */
    Convert::read_file(EVENT_LOITERING_DETECTION_CONFIG_FILE, stAllDeviceParam.stLoiteringDetection);       /* Smart事件-徘徊侦测 */
    Convert::read_file(EVENT_CROWD_GATHERING_DETECTION_CONFIG_FILE, stAllDeviceParam.stCrowdGathering);     /* Smart事件-人员聚集侦测 */
    Convert::read_file(EVENT_PARKING_DETECTION_CONFIG_FILE, stAllDeviceParam.stParkingDetection);           /* Smart事件-停车侦测 */
    Convert::read_file(EVENT_UNATTENDED_OBJECT_DETECTION_CONFIG_FILE, stAllDeviceParam.stUnattendedObject); /* Smart事件-物品遗留侦测 */
    Convert::read_file(EVENT_OBJECT_REMOVAL_DETECTION_CONFIG_FILE, stAllDeviceParam.stObjectRemoval);       /* Smart事件-物品拿取侦测 */
    Convert::read_file(EVENT_PET_RECOGNITION_CONFIG_FILE, stAllDeviceParam.stPetRecognition);               /* Smart事件-宠物侦测 */
#if defined(SCENE_INTELLIGENCE) || CAP_AI_GARBAGE_DETECT
    Convert::read_file(EVENT_GARBAGE_EXPOSURE_INFO_FILE, stAllDeviceParam.stGarbageExposure);   /* Smart事件-人流统计 */
    Convert::read_file(EVENT_GARBAGE_OVERFLOW_INFO_FILE, stAllDeviceParam.stGarbageOverflow); /* Smart事件-人员密度检测 */
#endif
#if CAP_AI_PEOPLE_STATISTICS
    Convert::read_file(EVENT_PEOPLE_FLOW_STATISTICS_CONFIG_FILE, stAllDeviceParam.stPeopleFlowStatistics);   /* Smart事件-人流统计 */
    Convert::read_file(EVENT_PEOPLE_DENSITY_DETECTION_CONFIG_FILE, stAllDeviceParam.stPeopleDensityDetection); /* Smart事件-人员密度检测 */
#endif

    /* 存储管理 */
    Convert::read_file(RECORD_SCHEDULE_CONFIG_FILE, stAllDeviceParam.stSchedule);                           /* 计划配置-录像计划 */
    Convert::read_file(RECORD_ADVANCED_PARAM_CONFIG_FILE, stAllDeviceParam.stAdvancedParamParam);           /* 计划配置-录像计划-录像参数 */
    Convert::read_file(CAPTURE_PLAN_CONFIG_FILE, stAllDeviceParam.stCapturePlan);                           /* 计划配置-抓图计划 */
    Convert::read_file(CAPTURE_PARAM_CONFIG_FILE, stAllDeviceParam.stCaptureParam);                         /* 计划配置-抓图计划-抓图参数 */
    Convert::read_file(STORAGE_MANAGE_CONFIG_FILE, stAllDeviceParam.stStorageManage);                       /* 存储管理-轻存储 */
    /* 视音频 */
    Convert::read_file(VIDEO_CONFIG_FILE, stAllDeviceParam.vecVideoConfig);                                 /* 视频 */
    Convert::read_file(AUDIO_CONFIG_FILE, stAllDeviceParam.stAudioConfig);                                  /* 音频 */
    Convert::read_file(ROI_CONFIG_FILE, stAllDeviceParam.vecVideoRoi);                                      /* ROI */
    Convert::read_file(AREA_CROP_CONFIG_FILE, stAllDeviceParam.vecAreaCrop);                                /* 区域裁剪 */
    /* 人脸抓拍 */
    Convert::read_file(EVENT_FACE_CAPTURE_CONFIG_FILE, stAllDeviceParam.stFaceCapture);                     /* 人脸抓拍 */
    /* 图像管理 */
    Convert::read_file(PIC_SCENEPARAM_CONFIG_FILE, stAllDeviceParam.stAllSceneParams);                      /* 显示设置 */
    Convert::read_file(OSD_CONFIG_FILE, stAllDeviceParam.stOsdConfig);                                      /* OSD设置 */
    Convert::read_file(COVER_CONFIG_FILE, stAllDeviceParam.stCoverConfig);                                  /* 视频遮盖 */
    Convert::read_file(IMAGE_SCHEDULE_CONFIG_FILE, stAllDeviceParam.stSceneSchedule);                       /* 图像参数切换 */

    return 0;
}

/* 导入设备参数 */
int SystemManage::import_device_param(System::AllDeviceParam_S &stAllDeviceParam)
{
    /* 预览 */
    Convert::write_file(PREVIEW_CONFIG_FILE, stAllDeviceParam.stPreviewInfo);                                /* 预览 */
    /* 回放 */
    /* 图片 */
    /* 配置 */
    /* 本地设置 */
    Convert::write_file(WEB_PLUGIN_CONFIG_FILE, stAllDeviceParam.stParam);                                   /* 本地设置 */
    /* 系统配置 */
    Convert::write_file(DEVICE_CONFIG_FILE, stAllDeviceParam.stDeviceConfig);                                /* 系统设置-基本配置 */
    Convert::write_file(DEVICE_INFO_CONFIG_FILE, stAllDeviceParam.stDeviceInfo);                             /* 系统设置-基本信息 */
    Convert::write_file(TIME_CONFIG_FILE, stAllDeviceParam.stTimeInfo);                                      /* 系统设置-时间配置 */
                                                                                                             /* 系统设置-智能资源分配 */
    Convert::write_file(UPGRADE_MAINTAIN_FILE, stAllDeviceParam.stUpgradeMaintain);                          /* 系统维护-升级维护 */
    Convert::write_file(LOG_SERVER_INFO_FILE, stAllDeviceParam.stLogServerInfo);                             /* 系统维护-安全审计日志 */
    Convert::write_file(SECURITY_CERT_CONFIG_FILE, stAllDeviceParam.stSecurityCert);                         /* 安全管理-认证方式 */
    Convert::write_file(IP_FILTER_CONFIG_FILE, stAllDeviceParam.stIpFilterConfigInfo);                       /* 安全管理-IP地址过滤 */
    Convert::write_file(SECURITY_SERVICES_FILE, stAllDeviceParam.stSecurityServices);                        /* 安全管理-安全服务 */
                                                                                                             /* 安全管理-客户端证书 */
                                                                                                             /* 安全管理-CA证书 */
    for (auto &userInfo : stAllDeviceParam.vecUserInfo)
    {
        CUserManage::instance()->add(userInfo,stAllDeviceParam.stSecurityServices);                                                             /* 用户管理-用户管理 */
    }
                                                                                                            /* 用户管理-在线用户 */
    /* 网络配置 */
    Convert::write_file(NETWORK_CONFIG_FILE, stAllDeviceParam.stNetInfo);                                    /* 基本配置-TCP/IP */
    Convert::write_file(PORT_CONFIG_FILE, stAllDeviceParam.stPortConfig);                                    /* 基本配置-端口 */
    Convert::write_file(UPNP_CONFIG_FILE, stAllDeviceParam.stPortMapConfig);                                 /* 基本配置-端口映射 */
    Convert::write_file(SNMP_CONFIG_FILE, stAllDeviceParam.stSnmpConfig);                                    /* 高级配置-SNMP */
    Convert::write_file(EMAIL_CONFIG_FILE, stAllDeviceParam.stEmailInfo);                                    /* 高级配置-Email */
    Convert::write_file(GB28181_CLIENT_CONFIG_FILE, stAllDeviceParam.stGB28181Client);                       /* 高级配置-平台接入 */
    Convert::write_file(GM_CERT_INFO_FILE, stAllDeviceParam.stGmCertFileInfo);                               /* 高级配置-国标证书管理 */
    Convert::write_file(HTTPS_CONFIG_FILE, stAllDeviceParam.stHttpsConfigInfo);                              /* 高级配置-HTTPS */
    Convert::write_file(QOS_CONFIG_FILE, stAllDeviceParam.stQosConfigInfo);                                  /* 高级配置-Qos */
    Convert::write_file(ONVIF_CONFIG_FILE, stAllDeviceParam.stOnvifConfigInfo);                              /* 高级配置-集成协议 */
                                                                                                             /* 高级配置-网络服务 */
    /* 事件配置 */
    Convert::write_file(EVENT_MOTION_DETECTION_CONFIG_FILE, stAllDeviceParam.stMotionDetection);             /* 普通事件-移动侦测 */
    Convert::write_file(EVENT_HIDE_ALARM_CONFIG_FILE, stAllDeviceParam.stHideAlarm);                         /* 普通事件-遮挡报警 */
    Convert::write_file(EVENT_ABNORMAL_ALARM_CONFIG_FILE, stAllDeviceParam.stAbnormalDetection);             /* 普通事件-异常 */
    Convert::write_file(EVENT_SOUND_ALARM_OUTPUT_CONFIG_FILE, stAllDeviceParam.stSoundOutputAlarm);          /* 普通事件-声音报警输出 */
    Convert::write_file(EVENT_ALARM_INPUT_CONFIG_FILE, stAllDeviceParam.stIoInputInfo);                      /* 普通事件-报警输入 */
    Convert::write_file(EVENT_ALARM_OUTPUT_CONFIG_FILE, stAllDeviceParam.stIoOutputInfo);                    /* 普通事件-报警输出 */
    Convert::write_file(EVENT_FLASHING_LIGHT_ALARM_OUTPUT_CONFIG_FILE, stAllDeviceParam.stFlashInfo);        /* 普通事件-闪光灯报警输出 */
    Convert::write_file(EVENT_PIR_ALARM_CONFIG_FILE, stAllDeviceParam.stPirAlarmInfo);                       /* 普通事件-PIR报警 */
    Convert::write_file(EVENT_LINE_CROSSING_DETECTION_CONFIG_FILE, stAllDeviceParam.stBoundaryDetection);    /* 周界事件-越界侦测 */
    Convert::write_file(EVENT_REGIONAL_INTRUSION_DETECTION_CONFIG_FILE, stAllDeviceParam.stFieldDetection);  /* 周界事件-区域入侵侦测 */
    Convert::write_file(EVENT_ENTER_REGION_DETECTION_CONFIG_FILE, stAllDeviceParam.stEntranceDetection);     /* 周界事件-进入区域侦测 */
    Convert::write_file(EVENT_LEAVE_REGION_DETECTION_CONFIG_FILE, stAllDeviceParam.stExitingDetection);      /* 周界事件-离开区域侦测 */
                                                                                                             /* 场景智能-翻阅围栏识别 */
                                                                                                             /* 场景智能-离岗识别 */
                                                                                                             /* 场景智能-违规变道识别 */
                                                                                                             /* 场景智能-逆行识别 */
                                                                                                             /* 场景智能-非机动车闯入识别 */
                                                                                                             /* 场景智能-应急车道占用识别 */
                                                                                                             /* 场景智能-行人闯入识别 */
                                                                                                             /* 场景智能-其他智能事件 */
    Convert::write_file(EVENT_AUDIO_ANOMALY_DETECTION_CONFIG_FILE, stAllDeviceParam.stAudioAnomaly);         /* Smart事件-音频异常侦测 */
    Convert::write_file(EVENT_SCENE_CHANGE_DETECTION_CONFIG_FILE, stAllDeviceParam.stSceneChange);           /* Smart事件-场景变更侦测 */
    Convert::write_file(EVENT_FACE_DETECTION_CONFIG_FILE, stAllDeviceParam.stFaceDetection);                 /* Smart事件-人脸侦测 */
    Convert::write_file(EVENT_LOITERING_DETECTION_CONFIG_FILE, stAllDeviceParam.stLoiteringDetection);       /* Smart事件-徘徊侦测 */
    Convert::write_file(EVENT_CROWD_GATHERING_DETECTION_CONFIG_FILE, stAllDeviceParam.stCrowdGathering);     /* Smart事件-人员聚集侦测 */
    Convert::write_file(EVENT_PARKING_DETECTION_CONFIG_FILE, stAllDeviceParam.stParkingDetection);           /* Smart事件-停车侦测 */
    Convert::write_file(EVENT_UNATTENDED_OBJECT_DETECTION_CONFIG_FILE, stAllDeviceParam.stUnattendedObject); /* Smart事件-物品遗留侦测 */
    Convert::write_file(EVENT_OBJECT_REMOVAL_DETECTION_CONFIG_FILE, stAllDeviceParam.stObjectRemoval);       /* Smart事件-物品拿取侦测 */
    Convert::write_file(EVENT_PET_RECOGNITION_CONFIG_FILE, stAllDeviceParam.stPetRecognition);               /* Smart事件-宠物侦测 */
#if defined(SCENE_INTELLIGENCE) || CAP_AI_GARBAGE_DETECT
    Convert::write_file(EVENT_GARBAGE_EXPOSURE_INFO_FILE, stAllDeviceParam.stGarbageExposure);               /* Smart事件-垃圾暴露识别 */
    Convert::write_file(EVENT_GARBAGE_OVERFLOW_INFO_FILE, stAllDeviceParam.stGarbageOverflow);               /* Smart事件-垃圾满溢识别 */
#endif
#if CAP_AI_PEOPLE_STATISTICS
    Convert::write_file(EVENT_PEOPLE_FLOW_STATISTICS_CONFIG_FILE, stAllDeviceParam.stPeopleFlowStatistics);   /* Smart事件-人流统计 */
    Convert::write_file(EVENT_PEOPLE_DENSITY_DETECTION_CONFIG_FILE, stAllDeviceParam.stPeopleDensityDetection); /* Smart事件-人员密度检测 */
#endif

    /* 存储管理 */
    Convert::write_file(RECORD_SCHEDULE_CONFIG_FILE, stAllDeviceParam.stSchedule);                           /* 计划配置-录像计划 */
    Convert::write_file(RECORD_ADVANCED_PARAM_CONFIG_FILE, stAllDeviceParam.stAdvancedParamParam);           /* 计划配置-录像计划-录像参数 */
    Convert::write_file(CAPTURE_PLAN_CONFIG_FILE, stAllDeviceParam.stCapturePlan);                           /* 计划配置-抓图计划 */
    Convert::write_file(CAPTURE_PARAM_CONFIG_FILE, stAllDeviceParam.stCaptureParam);                         /* 计划配置-抓图计划-抓图参数 */
    Convert::write_file(STORAGE_MANAGE_CONFIG_FILE, stAllDeviceParam.stStorageManage);                       /* 存储管理-轻存储 */
    /* 视音频 */
    Convert::write_file(VIDEO_CONFIG_FILE, stAllDeviceParam.vecVideoConfig);                                 /* 视频 */
    Convert::write_file(AUDIO_CONFIG_FILE, stAllDeviceParam.stAudioConfig);                                  /* 音频 */
    Convert::write_file(ROI_CONFIG_FILE, stAllDeviceParam.vecVideoRoi);                                      /* ROI */
    Convert::write_file(AREA_CROP_CONFIG_FILE, stAllDeviceParam.vecAreaCrop);                                /* 区域裁剪 */
    /* 人脸抓拍 */
    Convert::write_file(EVENT_FACE_CAPTURE_CONFIG_FILE, stAllDeviceParam.stFaceCapture);                     /* 人脸抓拍 */
    /* 图像管理 */
    Convert::write_file(PIC_SCENEPARAM_CONFIG_FILE, stAllDeviceParam.stAllSceneParams);                      /* 显示设置 */
    Convert::write_file(OSD_CONFIG_FILE, stAllDeviceParam.stOsdConfig);                                      /* OSD设置 */
    Convert::write_file(COVER_CONFIG_FILE, stAllDeviceParam.stCoverConfig);                                  /* 视频遮盖 */
    Convert::write_file(IMAGE_SCHEDULE_CONFIG_FILE, stAllDeviceParam.stSceneSchedule);                       /* 图像参数切换 */

    return 0;
}

void SystemManage::start_logUptime()
{
    dlog_debug("开始记录设备使用时长");
    m_UptimeFile = UPTIME_FILE;
    std::ifstream in_file(m_UptimeFile);
    long tempTotal = 0;
    long tempUptime = 0;

    /* 读取设备运行时长 */
    if (in_file.good()) 
    {
        dlog_debug("读取到运行时长文件");
        in_file >> tempTotal >> tempUptime;
    }
    in_file.close();
    m_lgPersistedTotal.store(tempTotal);
    m_lgLastsaveduptime.store(tempUptime);

    /* 设置线程退出标志为false */
    m_bLogThreadExitFlag.store(false);

    m_logThread = std::thread(&SystemManage::update_upTime, this);
    m_logThread.detach();
}

void SystemManage::stop_logUptime()
{
    /* 设置线程退出标志为true */
    m_bLogThreadExitFlag.store(true);
    
    /* 等待线程结束 */
    if (m_logThread.joinable())
    {
        m_logThread.join();
    }
    
    dlog_debug("已停止记录设备使用时长");
}

void SystemManage::update_upTime()
{
    auto format = [](long seconds) 
    {
        if (seconds < 0) return std::string("0小时0分");
        int hours = seconds / 3600;
        int minutes = (seconds % 3600) / 60;
        return std::to_string(hours) + "小时" + std::to_string(minutes) + "分";
    };

    while(!m_bLogThreadExitFlag.load())
    {
        struct sysinfo info;
        sysinfo(&info);
        long lgCurrentuUptime = info.uptime;
        //log_debug("获取当前设备时长:%s",std::to_string(lgCurrentuUptime).c_str());

        long lgDelta = (lgCurrentuUptime >= m_lgLastsaveduptime.load()) 
                   ? (lgCurrentuUptime - m_lgLastsaveduptime.load())
                   : lgCurrentuUptime;

        m_lgPersistedTotal += lgDelta;
        m_lgLastsaveduptime.store(lgCurrentuUptime);

        std::ofstream out_file(m_UptimeFile);
        out_file << m_lgPersistedTotal.load() << "\n"
                    << m_lgLastsaveduptime.load();
        out_file.close();
        
        long lgControlSeconds = get_process_startTime("stream");
        //log_debug("设备运行时长 第一次开机累计总运行时长:[%s] 上一次保存时的系统运行时间:[%s]"
        //            ,std::to_string(m_lgPersistedTotal.load()).c_str(),std::to_string(m_lgLastsaveduptime.load()).c_str());
        std::string strTimeLog = "\n设备运行时长：" + format(m_lgPersistedTotal.load()) +"\n"
                                + "程序运行时长：" + format(lgControlSeconds)  +"\n";
        dlog_debug("设备时长日志:%s",strTimeLog.c_str());

        /* 信息日志-设备运行时长 */
        Log::Info_S stLogInfo;
        stLogInfo.nType = Log::Type::INFOMATION;
        stLogInfo.nAction = Log::DEVICE_RUNTIME;
        stLogInfo.context = strTimeLog;
        LogHandler::instance()->write(stLogInfo);

        std::this_thread::sleep_for(std::chrono::hours(1));
    }
     
}

long SystemManage::getSystemUptime() 
{
    struct sysinfo info;
    sysinfo(&info);
    return info.uptime;
}

long SystemManage::get_process_startTime(const std::string& strProcessName)
{
    DIR* dir = opendir("/proc");
    if (!dir) return -1;

    struct dirent* entry;
    long clk_tck = sysconf(_SC_CLK_TCK);
    long uptime_seconds = getSystemUptime();

    while ((entry = readdir(dir)) != nullptr) 
    {
        if (entry->d_type != DT_DIR) continue;

        std::string pid_dir = entry->d_name;
        if (!isdigit(pid_dir[0])) continue;

        std::string comm_path = "/proc/" + pid_dir + "/comm";
        std::ifstream comm_file(comm_path);
        if (!comm_file) continue;

        std::string name;
        getline(comm_file, name);
        comm_file.close();

        if (name == strProcessName) 
        {
            std::string stat_path = "/proc/" + pid_dir + "/stat";
            std::ifstream stat_file(stat_path);
            if (!stat_file) continue;

            std::string line;
            getline(stat_file, line);
            stat_file.close();

            std::istringstream iss(line);
            std::vector<std::string> fields;
            std::string field;
            while (iss >> field) 
            {
                fields.push_back(field);
            }

            if (fields.size() < 22) continue;

            long start_time_ticks = stol(fields[21]);
            long start_time_seconds = start_time_ticks / clk_tck;
            long process_uptime = uptime_seconds - start_time_seconds;

            closedir(dir);
            return process_uptime;
        }
    }

    closedir(dir);
    return -1;
}

int SystemManage::enable_ioInputNumber(std::map<int, bool> numberSet)
{
    m_ioInput.enable_number(numberSet);
    return 0;
}

void SystemManage::callback_ioInputEvent(int nNumber)
{ 
    Event::AlgorithmConfig_S stAlarmInfo;
    Alarm::IoInputInfo_S stIoInputInfo;
    CEventConfigure::instance()->get_configure(stAlarmInfo);

    stIoInputInfo.nIoNumer = nNumber;
    CEventConfigure::instance()->get_configure(stIoInputInfo);

    if(stAlarmInfo.nEnAlarmInput && stIoInputInfo.nDealType)
    {
        /* 触发对应的报警事件 */
        CEventLinkage::instance()->handleEvent(Event::Type::ALARM_INPUT);
    }

}
