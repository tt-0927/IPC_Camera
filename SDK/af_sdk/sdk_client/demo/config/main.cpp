/**
 * @file main.cpp
 * @brief SDK客户端 配置(Get/Set) Demo
 *
 * 演示内容：
 * 1. 使用 NET_TV_GetDevConfig 获取设备基本信息 / 网络配置
 * 2. 使用 NET_TV_SetDevConfig 修改设备基本信息 / 网络配置
 *
 * 说明：
 * - 需先启动对应的服务端 ConfigServerDemo（监听 8888 端口）
 * - 本 Demo 仅演示调用流程，配置在服务端内存中维护
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <thread>
#include <chrono>
#include "NetSdkLog.h"
#include "NetTVSDKClientInterface.h"

/* 日志配置 */
#define MAX_LOG_SIZE  (20 * 1024 * 1024)
#define MAX_LOG_FILES (10)

/* 服务端配置（与服务端 Demo 保持一致） */
#define SERVER_IP   "127.0.0.1"
#define SERVER_PORT 9888
#define USERNAME    "admin"
#define PASSWORD    "sj2@2025"
#define DEMO_OSD_CUSTOM_MAX_NUM 4
#define DEMO_OSD_TOTAL_SLOT_NUM 32

static char g_serverIp[64] = SERVER_IP;
static INT32 g_serverPort = SERVER_PORT;
static char g_username[64] = USERNAME;
static char g_password[64] = PASSWORD;



/* 全局用户句柄 */
static LPVOID g_lpUserID = NULL;
static char g_szReplaySessionId[NET_TV_REPLAY_SESSION_ID_LEN] = {0};

/* NVR control 内部平台点播命令码，客户端通过 NET_TV_ControlReplay 触发服务端转发 */
#define DEMO_AC_PLATFORM_PLAY 3213
/* demo 本地命令码：暂停也走 NET_TV_ControlReplay，不是新增 SDK 接口 */
#define DEMO_REPLAY_PAUSE_CMD 3214
/* demo 本地命令码：恢复播放 */
#define DEMO_REPLAY_RESUME_CMD 3215

static void CopyString(char* pDst, size_t dstSize, const char* pSrc)
{
    if (!pDst || dstSize == 0)
    {
        return;
    }

    pDst[0] = '\0';
    if (!pSrc)
    {
        return;
    }

    strncpy(pDst, pSrc, dstSize - 1);
    pDst[dstSize - 1] = '\0';
}

static void PrintUsage(const char* pProgram)
{
    printf("Usage: %s [server_ip] [port] [username] [password]\n",
           pProgram ? pProgram : "ConfigClientDemo");
    printf("Example: %s 172.16.25.199 9888 admin sj2@2025\n",
           pProgram ? pProgram : "ConfigClientDemo");
}

static void ConfigureByArgs(int argc, char* argv[])
{
    if (argc > 5)
    {
        PrintUsage(argv[0]);
    }

    if (argc > 1)
    {
        CopyString(g_serverIp, sizeof(g_serverIp), argv[1]);
    }

    if (argc > 2)
    {
        int port = atoi(argv[2]);
        if (port > 0)
        {
            g_serverPort = port;
        }
    }

    if (argc > 3)
    {
        CopyString(g_username, sizeof(g_username), argv[3]);
    }

    if (argc > 4)
    {
        CopyString(g_password, sizeof(g_password), argv[4]);
    }
}

static void PrintMenu()
{
    printf("\n");
    printf("========== 配置获取/设置 Demo ==========\n");
    printf(" 1 - 获取设备基本信息   (NET_TV_GET_DEVICECFG)\n");
    printf(" 2 - 设置设备基本信息   (NET_TV_SET_DEVICECFG)\n");
    printf(" 3 - 获取网络配置       (NET_TV_GET_NETWORKCFG)\n");
    printf(" 4 - 设置网络配置       (NET_TV_SET_NETWORKCFG)\n");
    printf(" 5 - 获取移动侦测配置   (NET_TV_GET_MOTIONALARM)\n");
    printf(" 6 - 设置移动侦测配置   (NET_TV_SET_MOTIONALARM)\n");
    printf(" 7 - 获取遮挡报警配置   (NET_TV_GET_TAMPERALARM)\n");
    printf(" 8 - 设置遮挡报警配置   (NET_TV_SET_TAMPERALARM)\n");
    printf(" 9 - 获取越界检测配置   (NET_TV_GET_CROSSLINEALARM)\n");
    printf("10 - 设置越界检测配置   (NET_TV_SET_CROSSLINEALARM)\n");
    printf("11 - 获取入侵检测配置   (NET_TV_GET_INTRUSIONALARM)\n");
    printf("12 - 设置入侵检测配置   (NET_TV_SET_INTRUSIONALARM)\n");
    printf("13 - 获取RTSP流地址     (NET_TV_GET_RTSPURLCFG)\n");
    printf("14 - 获取徘徊侦测配置   (NET_TV_GET_LOITERINGALARM)\n");
    printf("15 - 设置徘徊侦测配置   (NET_TV_SET_LOITERINGALARM)\n");
    printf("16 - 获取OSD能力集配置 (NET_TV_GET_OSDCAPCFG)\n");
    printf("17 - 设置OSD能力集配置 (NET_TV_SET_OSDCAPCFG)\n");
    printf("166 - OSD配置一键验证   (GET -> SET -> GET)\n");
    printf("18 - 设置升级文件路径   (NET_TV_SET_UPGRADE)\n");
    printf("19 - 获取升级文件版本   (NET_TV_GET_UPGRADEVERSION)\n");
    printf("20 - 获取升级状态       (NET_TV_UPGRADE_STATUS_S)\n");
    printf("21 - 获取抓图计划信息   (NET_TV_GET_CAPTURE_PLAN_INFO)\n");
    printf("22 - 设置抓图计划信息   (NET_TV_SET_CAPTURE_PLAN_INFO)\n");
    printf("23 - 获取抓图参数信息   (NET_TV_GET_CAPTURE_PARAM_INFO)\n");
    printf("24 - 设置抓图参数信息   (NET_TV_SET_CAPTURE_PARAM_INFO)\n");
    printf("25 - 获取曝光信息       (NET_TV_GET_EXPOSURE_INFO)\n");
    printf("26 - 设置曝光信息       (NET_TV_SET_EXPOSURE_INFO)\n");
    printf("27 - 获取日夜转换信息   (NET_TV_GET_DAYNIGHT_INFO)\n");
    printf("28 - 设置日夜转换信息   (NET_TV_SET_DAYNIGHT_INFO)\n");
    printf("29 - 获取背光信息       (NET_TV_GET_BACKLIGHT_INFO)\n");
    printf("30 - 设置背光信息       (NET_TV_SET_BACKLIGHT_INFO)\n");
    printf("31 - 获取降噪信息       (NET_TV_GET_DENOISE_INFO)\n");
    printf("32 - 设置降噪信息       (NET_TV_SET_DENOISE_INFO)\n");
    printf("33 - 获取白平衡信息     (NET_TV_GET_WHITEBALANCE_INFO)\n");
    printf("34 - 设置白平衡信息     (NET_TV_SET_WHITEBALANCE_INFO)\n");
    printf("35 - 获取音频异常侦测配置 (NET_TV_GET_AUDIOANOMALYALARM)\n");
    printf("36 - 设置音频异常侦测配置 (NET_TV_SET_AUDIOANOMALYALARM)\n");
    printf("37 - 获取预览配置       (NET_TV_GET_PREVIEW_INFO)\n");
    printf("38 - 设置预览配置       (NET_TV_SET_PREVIEW_INFO)\n");
    printf("39 - 获取场景变更侦测配置 (NET_TV_GET_SCENECHANGEALARM)\n");
    printf("40 - 设置场景变更侦测配置 (NET_TV_SET_SCENECHANGEALARM)\n");
    printf("41 - 获取人员聚集配置 (NET_TV_GET_CROWDGATHERINGALARM)\n");
    printf("42 - 设置人员聚集配置 (NET_TV_SET_CROWDGATHERINGALARM)\n");
    printf("43 - 获取垃圾暴露配置 (NET_TV_GET_GARBAGE_EXPOSURE_CFG)\n");
    printf("44 - 设置垃圾暴露配置 (NET_TV_SET_GARBAGE_EXPOSURE_CFG)\n");
    printf("45 - 获取垃圾满溢配置 (NET_TV_GET_GARBAGE_OVERFLOW_CFG)\n");
    printf("46 - 设置垃圾满溢配置 (NET_TV_SET_GARBAGE_OVERFLOW_CFG)\n");
    printf("47 - 获取人流统计配置 (NET_TV_GET_PEOPLE_FLOW_STATISTICS_CFG)\n");
    printf("48 - 设置人流统计配置 (NET_TV_SET_PEOPLE_FLOW_STATISTICS_CFG)\n");
    printf("49 - 重置人流统计 (NET_TV_RESET_PEOPLE_FLOW_STATISTICS)\n");
    printf("50 - 获取人员密度检测配置 (NET_TV_GET_PEOPLE_DENSITY_DETECTION_CFG)\n");
    printf("51 - 设置人员密度检测配置 (NET_TV_SET_PEOPLE_DENSITY_DETECTION_CFG)\n");
    printf("52 - 获取停车侦测配置信息 (NET_TV_GET_PARKINGALARM)\n");
    printf("53 - 设置对讲状态 (NET_TV_STATE_TALKBACK)\n");
    printf("54 - 发送对讲数据到指定流 (NET_TV_TO_STREAM_TALKBACK)\n");
    printf("55 - 从指定流获取对讲数据 (NET_TV_FROM_STREAM_TALKBACK)\n");
    printf("56 - 回放对讲数据 (NET_TV_REPLAY_TALKBACK)\n");
    printf("57 - 设置WIFI STA基础配置 (NET_TV_SET_CONFIG_WIFI_STA)\n");
    printf("58 - 连接WIFI STA (NET_TV_CONNECT_WIFI_STA)\n");
    printf("59 - 断开WIFI STA (NET_TV_DISCONNECT_WIFI_STA)\n");
    printf("60 - 获取4G配置 (NET_TV_GET_4G_INFO)\n");
    printf("61 - 设置4G配置 (NET_TV_SET_4G_INFO)\n");
    printf("62 - 设置热点配置 (NET_TV_SET_HOTSPOT_INFO)\n");
    printf("63 - 获取音频配置 (NET_TV_GET_AUDIOCFG)\n");
    printf("64 - 设置音频配置 (NET_TV_SET_AUDIOCFG)\n");
    printf("65 - 获取进入区域侦测配置 (NET_TV_GET_ENTERREGIONALARM)\n");
    printf("66 - 设置进入区域侦测配置 (NET_TV_SET_ENTERREGIONALARM)\n");
    printf("67 - 获取离开区域侦测配置 (NET_TV_GET_LEAVEREGIONALARM)\n");
    printf("68 - 设置离开区域侦测配置 (NET_TV_SET_LEAVEREGIONALARM)\n");
    printf("69 - 获取人脸抓拍配置 (NET_TV_GET_FACECAPTUREINFO)\n");
    printf("70 - 设置人脸抓拍配置 (NET_TV_SET_FACECAPTUREINFO)\n");
    printf("71 - 设置停车侦测配置 (NET_TV_SET_PARKINGALARM)\n");
    printf("72 - 获取物品遗留侦测配置 (NET_TV_GET_UNATTENDEDOBJECTALARM)\n");
    printf("73 - 设置物品遗留侦测配置 (NET_TV_SET_UNATTENDEDOBJECTALARM)\n");
    printf("74 - 获取物品拿取侦测配置 (NET_TV_GET_OBJECTREMOVALALARM)\n");
    printf("75 - 设置物品拿取侦测配置 (NET_TV_SET_OBJECTREMOVALALARM)\n");
    printf("76 - 获取通道信息             (NET_TV_GET_CHANNEL_INFO)\n");
    printf("77 - 获取通道列表             (NET_TV_GET_CHANNEL_LIST)\n");
    printf("78 - 获取井盖异常检测配置 (NET_TV_GET_MANHOLE_COVER_ABNORMAL_CFG)\n");
    printf("79 - 设置井盖异常检测配置 (NET_TV_SET_MANHOLE_COVER_ABNORMAL_CFG)\n");
    printf("80 - 获取睡岗识别配置 (NET_TV_GET_SLEEP_ON_DUTY_CFG)\n");
    printf("81 - 设置睡岗识别配置 (NET_TV_SET_SLEEP_ON_DUTY_CFG)\n");
    printf("82 - 获取电瓶车进电梯识别配置 (NET_TV_GET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG)\n");
    printf("83 - 设置电瓶车进电梯识别配置 (NET_TV_SET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG)\n");
    printf("84 - 获取人员倒地识别配置 (NET_TV_GET_PERSON_FALL_DOWN_CFG)\n");
    printf("85 - 设置人员倒地识别配置 (NET_TV_SET_PERSON_FALL_DOWN_CFG)\n");
    printf("86 - 获取施工占道识别配置 (NET_TV_GET_CONSTRUCTION_OCCUPY_ROAD_CFG)\n");
    printf("87 - 设置施工占道识别配置 (NET_TV_SET_CONSTRUCTION_OCCUPY_ROAD_CFG)\n");
    printf("88 - 获取拥堵识别配置 (NET_TV_GET_CONGESTION_CFG)\n");
    printf("89 - 设置拥堵识别配置 (NET_TV_SET_CONGESTION_CFG)\n");
    printf("90 - 获取车牌识别配置 (NET_TV_GET_LICENSE_PLATE_RECOGNITION_CFG)\n");
    printf("91 - 设置车牌识别配置 (NET_TV_SET_LICENSE_PLATE_RECOGNITION_CFG)\n");
    printf("92 - 获取高空安全带识别配置 (NET_TV_GET_HIGH_ALTITUDE_SEATBELT_CFG)\n");
    printf("93 - 设置高空安全带识别配置 (NET_TV_SET_HIGH_ALTITUDE_SEATBELT_CFG)\n");
    printf("94 - 获取安全帽识别配置 (NET_TV_GET_SAFETY_HELMET_CFG)\n");
    printf("95 - 设置安全帽识别配置 (NET_TV_SET_SAFETY_HELMET_CFG)\n");
    printf("96 - 获取摔倒识别配置 (NET_TV_GET_PERSON_FALL_CFG)\n");
    printf("97 - 设置摔倒识别配置 (NET_TV_SET_PERSON_FALL_CFG)\n");
    printf("98 - 获取玩手机识别配置 (NET_TV_GET_PHONE_USAGE_CFG)\n");
    printf("99 - 设置玩手机识别配置 (NET_TV_SET_PHONE_USAGE_CFG)\n");
    printf("100 - 获取抽烟识别配置 (NET_TV_GET_SMOKING_CFG)\n");
    printf("101 - 设置抽烟识别配置 (NET_TV_SET_SMOKING_CFG)\n");
    printf("102 - 获取明火识别配置 (NET_TV_GET_OPEN_FLAME_CFG)\n");
    printf("103 - 设置明火识别配置 (NET_TV_SET_OPEN_FLAME_CFG)\n");
    printf("104 - 获取黄土裸露识别配置 (NET_TV_GET_BARE_SOIL_CFG)\n");
    printf("105 - 设置黄土裸露识别配置 (NET_TV_SET_BARE_SOIL_CFG)\n");
    printf("106 - 获取洞口防护栏识别配置 (NET_TV_GET_HOLE_PROTECTION_BAR_CFG)\n");
    printf("107 - 设置洞口防护栏识别配置 (NET_TV_SET_HOLE_PROTECTION_BAR_CFG)\n");
    printf("108 - 获取反光衣识别配置 (NET_TV_GET_REFLECTIVE_CLOTHING_CFG)\n");
    printf("109 - 设置反光衣识别配置 (NET_TV_SET_REFLECTIVE_CLOTHING_CFG)\n");
    printf("110 - 获取宠物识别配置 (NET_TV_GET_PET_RECOGNITION_INFO)\n");
    printf("111 - 设置宠物识别配置 (NET_TV_SET_PET_RECOGNITION_INFO)\n");
    printf("112 - 获取翻越围栏配置 (NET_TV_GET_CLIMB_FENCE_INFO)\n");
    printf("113 - 设置翻越围栏配置 (NET_TV_SET_CLIMB_FENCE_INFO)\n");
    printf("114 - 获取离岗配置 (NET_TV_GET_DIMISSION_INFO)\n");
    printf("115 - 设置离岗配置 (NET_TV_SET_DIMISSION_INFO)\n");
    printf("116 - 获取违规变道配置 (NET_TV_GET_ILLEGAL_LANE_INFO)\n");
    printf("117 - 设置违规变道配置 (NET_TV_SET_ILLEGAL_LANE_INFO)\n");
    printf("118 - 获取逆行配置 (NET_TV_GET_RETROGRADE_INFO)\n");
    printf("119 - 设置逆行配置 (NET_TV_SET_RETROGRADE_INFO)\n");
    printf("120 - 获取非机动车闯入配置 (NET_TV_GET_NONMOTOR_VEHICLE_INTRUSION_INFO)\n");
    printf("121 - 设置非机动车闯入配置 (NET_TV_SET_NONMOTOR_VEHICLE_INTRUSION_INFO)\n");
    printf("122 - 获取应急车道占用识别配置 (NET_TV_GET_OCCUPATION_EMERGENCY_INFO)\n");
    printf("123 - 设置应急车道占用识别配置 (NET_TV_SET_OCCUPATION_EMERGENCY_INFO)\n");
    printf("124 - 获取行人闯入配置 (NET_TV_GET_PEDESTRIAN_INTRUSION_INFO)\n");
    printf("125 - 设置行人闯入配置 (NET_TV_SET_PEDESTRIAN_INTRUSION_INFO)\n");
    printf("126 - 获取烟火识别配置 (NET_TV_GET_SMOKE_FIRE_CFG)\n");
    printf("127 - 设置烟火识别配置 (NET_TV_SET_SMOKE_FIRE_CFG)\n");
    printf("128 - 获取道路积水检测配置 (NET_TV_GET_ROAD_PONDING_CFG)\n");
    printf("129 - 设置道路积水检测配置 (NET_TV_SET_ROAD_PONDING_CFG)\n");
    printf("130 - 获取视频码流配置 (NET_TV_GET_STREAMCFG)\n");
    printf("131 - 设置视频码流配置 (NET_TV_SET_STREAMCFG)\n");
    printf("132 - 获取热点连接设备 (NET_TV_GET_HOTSPOT_CONN)\n");
    printf("133 - 获取安全服务配置 (NET_TV_GET_SECURITY_SERVICES_INFO)\n");
    printf("134 - 设置安全服务配置 (NET_TV_SET_SECURITY_SERVICES_INFO)\n");
    printf("135 - 获取SSH倒计时 (NET_TV_GET_SSH_COUNTDOWN)\n");
    printf("136 - 查询日志 (NET_TV_FIND_LOG)\n");
    printf("137 - 导出日志 (NET_TV_EXPORT_LOG)\n");
    printf("138 - 获取日志服务器配置 (NET_TV_GET_LOG_SERVER)\n");
    printf("139 - 设置日志服务器配置 (NET_TV_SET_LOG_SERVER)\n");
    printf("140 - 测试日志服务器配置 (NET_TV_TEST_LOG_SERVER)\n");
    printf("141 - 获取回放播放地址 (NET_TV_GET_REPLAY_URLCFG)\n");
    printf("142 - 控制回放开始播放 (NET_TV_SET_REPLAY_CTRL/START)\n");
    printf("143 - 控制回放停止播放 (NET_TV_SET_REPLAY_CTRL/STOP)\n");
    printf("144 - 控制回放倍速播放 (NET_TV_SET_REPLAY_CTRL/SPEED)\n");
    printf("145 - 获取NVR事件录像时间段 (NET_TV_GET_REPLAY_RECORD_LIST)\n");
    printf("146 - 启动手动录像 (NET_TV_CONTROL_RECORD_INFO)\n");
    printf("147 - 停止手动录像 (NET_TV_CONTROL_RECORD_INFO)\n");
    printf("148 - 获取录像状态 (NET_TV_GET_RECORD_STATUS)\n");
    printf("149 - 获取录像计划 (NET_TV_GET_RECORD_SCHEDULE)\n");
    printf("150 - 设置录像计划 (NET_TV_SET_RECORD_SCHEDULE)\n");
    printf("151 - 获取录像高级参数 (NET_TV_GET_RECORD_ADVANCED_PARAM)\n");
    printf("152 - 设置录像高级参数 (NET_TV_SET_RECORD_ADVANCED_PARAM)\n");
    printf("153 - 查找录像文件 (NET_TV_FIND_RECORD_FILE_INFO)\n");
    printf("154 - 下载录像文件 (NET_TV_DOWNLOAD_RECORD_FILE)\n");
    printf("155 - 设置人脸比对配置 (NET_TV_SET_FACE_COMPARE_INFO)\n");
    printf("156 - 添加目标库 (NET_TV_ADD_TARGET_LIB)\n");
    printf("157 - 删除目标库 (NET_TV_DEL_TARGET_LIB)\n");
    printf("158 - 修改目标库 (NET_TV_SET_TARGET_LIB)\n");
    printf("159 - 获取目标库 (NET_TV_GET_TARGET_LIB)\n");
    printf("160 - 添加人脸 (NET_TV_ADD_FACE_INFO)\n");
    printf("161 - 删除人脸 (NET_TV_DEL_FACE_INFO)\n");
    printf("162 - 修改人脸 (NET_TV_SET_FACE_INFO)\n");
    printf("163 - 获取人脸 (NET_TV_GET_FACE_INFO)\n");
    printf("164 - 获取隐私遮盖配置 (NET_TV_GET_PRIVACYMASKCFG)\n");
    printf("165 - 设置隐私遮盖配置 (NET_TV_SET_PRIVACYMASKCFG)\n");
    printf("3213 - 平台点播回放控制类型 (自定义选择 1~8)\n");
    printf("3214 - 平台点播暂停播放 (NET_TV_SET_REPLAY_CTRL/PAUSE)\n");
    printf("3215 - 平台点播恢复播放 (NET_TV_SET_REPLAY_CTRL/RESUME)\n");
    printf(" 0 - 退出\n");
    printf("=======================================\n");
    printf("请输入命令码: ");
}

static void PrintFaceCaptureInfo(const NET_TV_FACE_CAPTURE_INFO_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 人脸抓拍配置 =====\n");
    printf("  Enable            : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  Sensitivity       : %d\n", pInfo->stRule.nSensitivity);
    printf("  RegionPointCount  : %d\n", pInfo->stRule.stRegion.dwPointCount);
    printf("  ShieldRegionCount : %d\n", pInfo->stRule.dwShieldRegionCount);
    printf("  MinIpdRect        : [%d,%d,%d,%d]\n",
           pInfo->stRule.nMinIpdRectLeft,
           pInfo->stRule.nMinIpdRectTop,
           pInfo->stRule.nMinIpdRectRight,
           pInfo->stRule.nMinIpdRectBottom);
    printf("  MinSize           : %dx%d\n", pInfo->stRule.nMinWidth, pInfo->stRule.nMinHeight);
    printf("  MaxSize           : %dx%d\n", pInfo->stRule.nMaxWidth, pInfo->stRule.nMaxHeight);
    printf("  Interval          : %d\n", pInfo->stRule.nInterval);
    printf("=================================\n");
}

static void PrintDeviceBasicInfo(const NET_TV_DEVICE_BASICINFO_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 设备基本信息 =====\n");
    printf("  DevModel      : %s\n", pInfo->szDevModel);
    printf("  SerialNum     : %s\n", pInfo->szSerialNum);
    printf("  FirmwareVer   : %s\n", pInfo->szFirmwareVersion);
    printf("  MacAddress    : %s\n", pInfo->szMacAddress);
    printf("  DeviceName    : %s\n", pInfo->szDeviceName);
    printf("  Manufacturer  : %s\n", pInfo->szManufacturer);
    printf("  DeviceTypeV2  : %s\n", pInfo->szDeviceTypeV2);
    printf("=================================\n");
}

static void PrintNetworkCfg(const NET_TV_NETWORKCFG_S* pCfg)
{
    if (!pCfg)
    {
        return;
    }

    printf("\n[Client] ===== 网络配置信息 =====\n");
    printf("  MTU          : %d\n", pCfg->dwMTU);
    printf("  IPv4DHCP     : %s\n", pCfg->bIPv4DHCP ? "ON" : "OFF");
    printf("  IPv4Address  : %s\n", pCfg->szIpv4Address);
    printf("  IPv4Gateway  : %s\n", pCfg->szIPv4GateWay);
    printf("  IPv4Subnet   : %s\n", pCfg->szIPv4SubnetMask);
    printf("=================================\n");
}

static void PrintAudioCfg(const NET_TV_AUDIO_CFG_S* pCfg)
{
    if (!pCfg)
    {
        return;
    }

    printf("\n[Client] ===== 音频配置信息 =====\n");
    printf("  AudioSwitch  : %s\n", pCfg->bAudioSwitch ? "ON" : "OFF");
    printf("  InputType    : %d\n", pCfg->enInputType);
    printf("  Format       : %d\n", pCfg->enFormat);
    printf("  SampRate     : %d\n", pCfg->enSampRate);
    printf("  BitRate      : %d\n", pCfg->enBitRate);
    printf("  InputVolume  : %u\n", pCfg->u32InputVolume);
    printf("  Denoise      : %s\n", pCfg->bDenoise ? "ON" : "OFF");
    printf("  OutputType   : %d\n", pCfg->enOutputType);
    printf("  OutputVolume : %u\n", pCfg->u32OutputVolume);
    printf("===============================\n");
}

static void PrintStreamCfg(const NET_TV_VIDEO_ENCODE_OPTION_S* pCfg)
{
    if (!pCfg)
    {
        return;
    }

    printf("\n[Client] ===== 视频码流配置信息 =====\n");
    printf("  Id               : %d\n", pCfg->nId);
    printf("  VideoType        : %d\n", pCfg->enVideoType);
    printf("  Resolution       : %dx%d\n", pCfg->stVideoResolution.dwWidth, pCfg->stVideoResolution.dwHeight);
    printf("  BitrateType      : %d\n", pCfg->enBitrateType);
    printf("  ImageQuality     : %d\n", pCfg->enImageQuality);
    printf("  FrameRate        : %d\n", pCfg->enFrameRate);
    printf("  BitrateUpperLimit: %d\n", pCfg->nBitrateUpperLimit);
    printf("  AverageBitrate   : %d\n", pCfg->nAverageBitrate);
    printf("  VideoCodec       : %d\n", pCfg->enVideoCodec);
    printf("  SmartEnable      : %s\n", pCfg->bSmartEnable ? "ON" : "OFF");
    printf("  Complexity       : %d\n", pCfg->enEncodingComplexity);
    printf("  IFrameInterval   : %d\n", pCfg->nIFrameInterval);
    printf("  SvcEnable        : %d\n", pCfg->enSvcEnable);
    printf("  BitrateSmoothing : %d\n", pCfg->nBitrateSmoothing);
    printf("=================================\n");
}

static void PrintWifiStaCfg(const NET_TV_WIFI_STA_CFG_S* pCfg)
{
    if (!pCfg)
    {
        return;
    }

    printf("\n[Client] ===== WIFI STA基础配置 =====\n");
    printf("  EnableWifi   : %s\n", pCfg->bEnableWifi ? "ON" : "OFF");
    printf("  EnableBoost  : %s\n", pCfg->bEnableBoost ? "ON" : "OFF");
    printf("===================================\n");
}

static void PrintWifiStaConnect(const NET_TV_WIFI_STA_CONNECT_S* pCfg)
{
    if (!pCfg)
    {
        return;
    }

    printf("\n[Client] ===== WIFI STA连接配置 =====\n");
    printf("  Ssid         : %s\n", pCfg->szSsid);
    printf("  SecurityMode : %d\n", pCfg->nSecurityMode);
    printf("  IpAddress    : %s\n", pCfg->szIpAddress);
    printf("  Pairwise     : %s\n", pCfg->szPairwise);
    printf("  Interface    : %s\n", pCfg->szInterfaceName);
    printf("====================================\n");
}

static void Print4GInfo(const NET_TV_4G_INFO_S* pCfg)
{
    if (!pCfg)
    {
        return;
    }

    printf("\n[Client] ===== 4G配置信息 =====\n");
    printf("  APN          : %s\n", pCfg->szApn);
    printf("  UserName     : %s\n", pCfg->szUserName);
    printf("  CallNumber   : %s\n", pCfg->szCallNumber);
    printf("  MTU          : %d\n", pCfg->nMtu);
    printf("  AuthMode     : %d\n", pCfg->nAuthMode);
    printf("  NetworkMode  : %d\n", pCfg->nNetworkMode);
    printf("  DialMode     : %d\n", pCfg->nDialMode);
    printf("===============================\n");
}

static void PrintHotspotInfo(const NET_TV_HOTSPOT_INFO_S* pCfg)
{
    if (!pCfg)
    {
        return;
    }

    printf("\n[Client] ===== 热点配置信息 =====\n");
    printf("  Enabled      : %s\n", pCfg->bEnabled ? "ON" : "OFF");
    printf("  SSID         : %s\n", pCfg->szSsid);
    printf("  SecurityMode : %s\n", pCfg->szSecurityMode);
    printf("  Encryption   : %s\n", pCfg->szEncryptionType);
    printf("================================\n");
}

static void PrintHotspotConnInfo(const NET_TV_HOTSPOT_CONN_INFO_S* pCfg)
{
    if (!pCfg)
    {
        return;
    }

    int nCount = pCfg->nDeviceCount;
    if (nCount < 0)
    {
        nCount = 0;
    }
    if (nCount > NET_TV_HOTSPOT_CONN_MAX_NUM)
    {
        nCount = NET_TV_HOTSPOT_CONN_MAX_NUM;
    }

    printf("\n[Client] ===== 热点连接设备 =====\n");
    printf("  Status      : %s\n", pCfg->szStatus);
    printf("  Total       : %d\n", pCfg->nTotal);
    printf("  DeviceCount : %d\n", pCfg->nDeviceCount);
    for (int i = 0; i < nCount; ++i)
    {
        printf("  Device[%d] Index=%d Mac=%s Ip=%s ConnTime=%s\n",
               i,
               pCfg->astDevices[i].nIndex,
               pCfg->astDevices[i].szMac,
               pCfg->astDevices[i].szIp,
               pCfg->astDevices[i].szConnTime);
    }
    printf("================================\n");
}

static void PrintSecurityServicesInfo(const NET_TV_SECURITY_SERVICES_INFO_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 安全服务配置 =====\n");
    printf("  IllegalLoginEnable   : %s\n", pInfo->stLoginLock.bIllegalLoginEnable ? "ON" : "OFF");
    printf("  CheckInterval        : %d\n", pInfo->stLoginLock.nCheckInterval);
    printf("  MaxErrorTimes        : %d\n", pInfo->stLoginLock.nMaxErrorTimes);
    printf("  LockDuration         : %d\n", pInfo->stLoginLock.nLockDuration);
    printf("  PwdSecurityLevel     : %s\n", pInfo->stPwdPolicy.bPwdSecurityLevelEnable ? "ON" : "OFF");
    printf("  AllowLowLevelPwd     : %s\n", pInfo->stPwdPolicy.bAllowLowLevelPwdLogin ? "YES" : "NO");
    printf("  SshEnable            : %s\n", pInfo->stSshAdmin.bSshEnable ? "ON" : "OFF");
    printf("  SshPort              : %d\n", pInfo->stSshAdmin.nSshPort);
    printf("  SshStartTime         : %s\n", pInfo->stSshAdmin.szSshStartTime);
    printf("  SshCountdown         : %s\n", pInfo->stSshAdmin.szSshCountdown);
    printf("================================\n");
}

static void PrintLogServerInfo(const NET_TV_LOG_SERVER_INFO_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 日志服务器配置 =====\n");
    printf("  Enable      : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  SSL         : %s\n", pInfo->bEnSsl ? "ON" : "OFF");
    printf("  ServerAddr  : %s\n", pInfo->szServerAddr);
    printf("  Port        : %d\n", pInfo->nPort);
    printf("================================\n");
}

static void PrintLogListInfo(const NET_TV_LOG_LIST_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    int nCount = pInfo->nLogCount;
    if (nCount < 0)
    {
        nCount = 0;
    }
    if (nCount > NET_TV_LOG_QUERY_COND_NUM)
    {
        nCount = NET_TV_LOG_QUERY_COND_NUM;
    }

    printf("\n[Client] ===== 日志列表 =====\n");
    printf("  Type=%d Action=%d Start=%s End=%s\n",
           pInfo->stCond.nType,
           pInfo->stCond.nAction,
           pInfo->stCond.szStartTime,
           pInfo->stCond.szEndTime);
    printf("  Page Cur=%d Size=%d Total=%d PageTotal=%d Count=%d\n",
           pInfo->stPage.nCurPage,
           pInfo->stPage.nPageSize,
           pInfo->stPage.nDataTotal,
           pInfo->stPage.nPageTotal,
           pInfo->nLogCount);
    for (int i = 0; i < nCount; ++i)
    {
        printf("  Log[%d] Time=%s Type=%d Action=%d Chn=%s User=%s Host=%s Context=%s\n",
               i,
               pInfo->astLogs[i].szStartTime,
               pInfo->astLogs[i].nType,
               pInfo->astLogs[i].nAction,
               pInfo->astLogs[i].szChnName,
               pInfo->astLogs[i].szUser,
               pInfo->astLogs[i].szHost,
               pInfo->astLogs[i].szContext);
    }
    printf("============================\n");
}

static void PrintRecordInfo(const NET_TV_RECORD_INFO_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 手动录像控制 =====\n");
    printf("  ChnId        : %d\n", pInfo->nChnId);
    printf("  VideoStatus  : %d\n", pInfo->nVideoStatus);
    printf("  AudioStatus  : %d\n", pInfo->nAudioStatus);
    printf("  RecordStatus : %d\n", pInfo->nRecordStatus);
    printf("  RecordFormat : %d\n", pInfo->nRecordFormat);
    printf("  EventType    : %d\n", pInfo->nEventType);
    printf("  Path         : %s\n", pInfo->szPath);
    printf("  RedunPath    : %s\n", pInfo->szRedunPath);
    printf("  RecordName   : %s\n", pInfo->szRecordName);
    printf("  RecordTime   : %s\n", pInfo->szRecordTime);
    printf("  StreamType   : %d\n", pInfo->nStreamType);
    printf("============================\n");
}

static void PrintRecordStatusInfo(const NET_TV_RECORD_STATUS_INFO_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 录像状态 =====\n");
    printf("  Status : %d\n", pInfo->nStatus);
    printf("=========================\n");
}

static void PrintRecordSchedule(const NET_TV_RECORD_SCHEDULE_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    int nDayCount = pInfo->nDayScheduleCount;
    if (nDayCount < 0)
    {
        nDayCount = 0;
    }
    if (nDayCount > NET_TV_PLAN_DAY_NUM_AWEEK)
    {
        nDayCount = NET_TV_PLAN_DAY_NUM_AWEEK;
    }

    printf("\n[Client] ===== 录像计划 =====\n");
    printf("  Enable   : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  DayCount : %d\n", pInfo->nDayScheduleCount);
    for (int i = 0; i < nDayCount; ++i)
    {
        int nTimeCount = pInfo->astDaySchedules[i].nRecordTimeCount;
        if (nTimeCount < 0)
        {
            nTimeCount = 0;
        }
        if (nTimeCount > NET_TV_TIME_DURATION_NUM)
        {
            nTimeCount = NET_TV_TIME_DURATION_NUM;
        }

        printf("  Day[%d] DayOfWeek=%d TimeCount=%d\n",
               i,
               pInfo->astDaySchedules[i].nDayOfWeek,
               pInfo->astDaySchedules[i].nRecordTimeCount);
        for (int j = 0; j < nTimeCount; ++j)
        {
            printf("    Time[%d] Type=%d Start=%d End=%d\n",
                   j,
                   pInfo->astDaySchedules[i].astRecordTimes[j].nType,
                   pInfo->astDaySchedules[i].astRecordTimes[j].nStartTime,
                   pInfo->astDaySchedules[i].astRecordTimes[j].nEndTime);
        }
    }
    printf("=========================\n");
}

static void PrintRecordAdvancedParam(const NET_TV_RECORD_ADVANCED_PARAM_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 录像高级参数 =====\n");
    printf("  LoopWrite  : %s\n", pInfo->bLoopWrite ? "ON" : "OFF");
    printf("  PreTime    : %d\n", pInfo->nPreTime);
    printf("  DelayTime  : %d\n", pInfo->nDelayTime);
    printf("  StreamType : %d\n", pInfo->nStreamType);
    printf("==============================\n");
}

static void PrintRecordFileList(const NET_TV_RECORD_FILE_LIST_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    int nResultCount = pInfo->nResultCount;
    if (nResultCount < 0)
    {
        nResultCount = 0;
    }
    if (nResultCount > NET_TV_RECORD_FILE_MAX_NUM)
    {
        nResultCount = NET_TV_RECORD_FILE_MAX_NUM;
    }

    printf("\n[Client] ===== 录像文件列表 =====\n");
    printf("  Find ChnId=%d Type=%d Date=%s Start=%s End=%s Filename=%s\n",
           pInfo->stFind.nChnId,
           pInfo->stFind.nType,
           pInfo->stFind.szDate,
           pInfo->stFind.szStartTime,
           pInfo->stFind.szEndTime,
           pInfo->stFind.szFilename);
    printf("  ResultCount : %d\n", pInfo->nResultCount);
    for (int i = 0; i < nResultCount; ++i)
    {
        int nDateCount = pInfo->astResults[i].nDateCount;
        int nVideoTimeCount = pInfo->astResults[i].nVideoTimeCount;
        if (nDateCount < 0)
        {
            nDateCount = 0;
        }
        if (nDateCount > NET_TV_RECORD_DATE_MAX_NUM)
        {
            nDateCount = NET_TV_RECORD_DATE_MAX_NUM;
        }
        if (nVideoTimeCount < 0)
        {
            nVideoTimeCount = 0;
        }
        if (nVideoTimeCount > NET_TV_TIME_DURATION_NUM)
        {
            nVideoTimeCount = NET_TV_TIME_DURATION_NUM;
        }

        printf("  Result[%d] ChnId=%d Filename=%s DateCount=%d VideoTimeCount=%d\n",
               i,
               pInfo->astResults[i].nChnId,
               pInfo->astResults[i].szFilename,
               pInfo->astResults[i].nDateCount,
               pInfo->astResults[i].nVideoTimeCount);
        for (int j = 0; j < nDateCount; ++j)
        {
            printf("    Date[%d]=%s\n", j, pInfo->astResults[i].aszDates[j]);
        }
        for (int j = 0; j < nVideoTimeCount; ++j)
        {
            printf("    VideoTime[%d] Start=%d End=%d\n",
                   j,
                   pInfo->astResults[i].astVideoTimes[j].nStartTime,
                   pInfo->astResults[i].astVideoTimes[j].nEndTime);
        }
    }
    printf("==============================\n");
}

static void PrintRecordDownloadList(const NET_TV_RECORD_DOWNLOAD_LIST_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    int nDownloadCount = pInfo->nDownloadCount;
    int nProgressCount = pInfo->nProgressCount;
    if (nDownloadCount < 0)
    {
        nDownloadCount = 0;
    }
    if (nDownloadCount > NET_TV_RECORD_DOWNLOAD_MAX_NUM)
    {
        nDownloadCount = NET_TV_RECORD_DOWNLOAD_MAX_NUM;
    }
    if (nProgressCount < 0)
    {
        nProgressCount = 0;
    }
    if (nProgressCount > NET_TV_RECORD_DOWNLOAD_MAX_NUM)
    {
        nProgressCount = NET_TV_RECORD_DOWNLOAD_MAX_NUM;
    }

    printf("\n[Client] ===== 录像下载任务 =====\n");
    printf("  DownloadCount : %d\n", pInfo->nDownloadCount);
    for (int i = 0; i < nDownloadCount; ++i)
    {
        printf("  Download[%d] ChnId=%d Path=%s Start=%s End=%s\n",
               i,
               pInfo->astDownloads[i].nChnId,
               pInfo->astDownloads[i].szPath,
               pInfo->astDownloads[i].szStartTime,
               pInfo->astDownloads[i].szEndTime);
    }
    printf("  ProgressCount : %d\n", pInfo->nProgressCount);
    for (int i = 0; i < nProgressCount; ++i)
    {
        printf("  Progress[%d] Filename=%s Progress=%d\n",
               i,
               pInfo->astProgress[i].szFilename,
               pInfo->astProgress[i].nProgress);
    }
    printf("==============================\n");
}

static void PrintRecordDownloadProgress(const NET_TV_RECORD_DOWNLOAD_PROGRESS_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 录像下载进度通知 =====\n");
    printf("  Filename : %s\n", pInfo->szFilename);
    printf("  Progress : %d\n", pInfo->nProgress);
    printf("==================================\n");
}

static void STDCALL ConfigAlarmCallback(INT64 lCommand,
                                        NET_TV_ALARMER_S* pAlarmer,
                                        CHAR* pAlarmInfo,
                                        INT32* dwBufLen,
                                        LPVOID lpUserData)
{
    (void)pAlarmer;
    (void)lpUserData;

    if (lCommand != NET_TV_NOTICE_DOWNLOAD_RECORD_PROGRESS)
    {
        printf("[Client] 收到报警通知 Command=%lld\n", (long long)lCommand);
        return;
    }

    if (!pAlarmInfo || !dwBufLen || *dwBufLen < (INT32)sizeof(NET_TV_RECORD_DOWNLOAD_PROGRESS_S))
    {
        printf("[Client] 收到录像下载进度通知，但数据长度无效\n");
        return;
    }

    PrintRecordDownloadProgress((LPNET_TV_RECORD_DOWNLOAD_PROGRESS_S)pAlarmInfo);
}

static void FillDemoPolygon4(FLOAT afPointX[32], FLOAT afPointY[32])
{
    afPointX[0] = 0.2f; afPointY[0] = 0.2f;
    afPointX[1] = 0.8f; afPointY[1] = 0.2f;
    afPointX[2] = 0.8f; afPointY[2] = 0.8f;
    afPointX[3] = 0.2f; afPointY[3] = 0.8f;
}

static const char* OsdAlignToString(OSD_ALIGN_E value)
{
    switch (value)
    {
    case OSD_ALIFN_CUSTOMIZE:       return "CUSTOMIZE";
    case OSD_ALIFN_CHARACTER_LEFT:  return "CHARACTER_LEFT";
    case OSD_ALIFN_CHARACTER_RIGHT: return "CHARACTER_RIGHT";
    case OSD_ALIFN_ALL_LEFT:        return "ALL_LEFT";
    case OSD_ALIFN_ALL_RIGHT:       return "ALL_RIGHT";
    case OSD_ALIFN_GB_MODE:         return "GB_MODE";
    default:                        return "UNKNOWN";
    }
}

static const char* OsdTimeFormatToString(OSD_TIME_FORMAT_E value)
{
    switch (value)
    {
    case OSD_TIME_FORMAT_24: return "24H";
    case OSD_TIME_FORMAT_12: return "12H";
    default:                 return "UNKNOWN";
    }
}

static const char* OsdDateFormatToString(OSD_DATE_FORMAT_E value)
{
    switch (value)
    {
    case ENGLISH_YYYY_MM_DD: return "YYYY-MM-DD";
    case ENGLISH_MM_DD_YYYY: return "MM-DD-YYYY";
    case ENGLISH_DD_MM_YYYY: return "DD-MM-YYYY";
    case CHINESE_YYYYMMDD:   return "YYYY年MM月DD日";
    case CHINESE_MMDDYYYY:   return "MM月DD日YYYY年";
    case CHINESE_DDMMYYYY:   return "DD日MM月YYYY年";
    case ENGLISH_YYYYMMDD:   return "YYYY/MM/DD";
    case ENGLISH_MMDDYYYY:   return "MM/DD/YYYY";
    case ENGLISH_DDMMYYYY:   return "DD/MM/YYYY";
    default:                 return "UNKNOWN";
    }
}

static const char* OsdFontSizeToString(OSD_FONT_SIZE_E value)
{
    switch (value)
    {
    case OSD_FONT_SIZE_ADAPTIVE: return "ADAPTIVE";
    case OSD_FONT_SIZE_16:       return "16x16";
    case OSD_FONT_SIZE_32:       return "32x32";
    case OSD_FONT_SIZE_48:       return "48x48";
    case OSD_FONT_SIZE_64:       return "64x64";
    default:                     return "UNKNOWN";
    }
}

static const char* OsdColorToString(OSD_COLOR_E value)
{
    switch (value)
    {
    case OSD_COLOR_BLACK:     return "BLACK";
    case OSD_COLOR_WHITE:     return "WHITE";
    case OSD_COLOR_CUSTOMIZE: return "CUSTOMIZE";
    default:                  return "UNKNOWN";
    }
}

static const char* OsdAttributeToString(OSD_ATTRIBUTE_E value)
{
    switch (value)
    {
    case OSD_ATTR_ALPHA_N_FLASH_N: return "ALPHA_N_FLASH_N";
    case OSD_ATTR_ALPHA_N_FLASH_Y: return "ALPHA_N_FLASH_Y";
    case OSD_ATTR_ALPHA_Y_FLASH_N: return "ALPHA_Y_FLASH_N";
    case OSD_ATTR_ALPHA_Y_FLASH_Y: return "ALPHA_Y_FLASH_Y";
    default:                       return "UNKNOWN";
    }
}

static void FillOsdAttr(OsdAttribute_S* pAttr,
                        INT32 x,
                        INT32 y,
                        INT32 width,
                        INT32 height,
                        OSD_FONT_SIZE_E fontSize,
                        OSD_COLOR_E fontColor,
                        const char* pColor,
                        const char* pToken)
{
    if (!pAttr)
    {
        return;
    }

    memset(pAttr, 0, sizeof(*pAttr));
    pAttr->nX = x;
    pAttr->nY = y;
    pAttr->nW = width;
    pAttr->nH = height;
    pAttr->enAttribute = OSD_ATTR_ALPHA_N_FLASH_N;
    pAttr->enFontSize = fontSize;
    pAttr->enFontColor = fontColor;
    if (pColor)
    {
        strncpy(pAttr->strFontColor, pColor, sizeof(pAttr->strFontColor) - 1);
    }
    if (pToken)
    {
        strncpy(pAttr->strToken, pToken, sizeof(pAttr->strToken) - 1);
    }
}

static void PrintOsdAttr(const char* pPrefix, const OsdAttribute_S* pAttr)
{
    if (!pAttr)
    {
        return;
    }

    printf("%sPos=(%d,%d,%d,%d), Attr=%s(%d), FontSize=%s(%d), FontColor=%s(%d), CustomColor=%s, Token=%s\n",
           pPrefix ? pPrefix : "",
           pAttr->nX,
           pAttr->nY,
           pAttr->nW,
           pAttr->nH,
           OsdAttributeToString(pAttr->enAttribute),
           pAttr->enAttribute,
           OsdFontSizeToString(pAttr->enFontSize),
           pAttr->enFontSize,
           OsdColorToString(pAttr->enFontColor),
           pAttr->enFontColor,
           pAttr->strFontColor,
           pAttr->strToken);
}

static void BuildDemoOSDCfg(NET_TV_VIDEO_OSD_CFG_S* pCfg)
{
    int i = 0;

    if (!pCfg)
    {
        return;
    }

    memset(pCfg, 0, sizeof(*pCfg));
    pCfg->enAlign = OSD_ALIFN_CUSTOMIZE;

    pCfg->stOsdNameInfo.bEnable = TRUE;
    strncpy(pCfg->stOsdNameInfo.strName,
            "SDK-Config-Demo",
            sizeof(pCfg->stOsdNameInfo.strName) - 1);
    FillOsdAttr(&pCfg->stOsdNameInfo.stOsdAttr,
                32,
                32,
                -1,
                32,
                OSD_FONT_SIZE_32,
                OSD_COLOR_WHITE,
                "#FFFFFF",
                "client_name_token");

    pCfg->stOsdTimeInfo.bEnable = TRUE;
    pCfg->stOsdTimeInfo.bEnableWeek = TRUE;
    pCfg->stOsdTimeInfo.enTimeFormat = OSD_TIME_FORMAT_24;
    pCfg->stOsdTimeInfo.enDateFormat = ENGLISH_YYYY_MM_DD;
    FillOsdAttr(&pCfg->stOsdTimeInfo.stOsdAttr,
                32,
                80,
                -1,
                32,
                OSD_FONT_SIZE_32,
                OSD_COLOR_WHITE,
                "#FFFFFF",
                "client_time_token");

    for (i = 0; i < DEMO_OSD_CUSTOM_MAX_NUM; ++i)
    {
        char name[NET_TV_LEN_128] = {0};
        char color[NET_TV_LEN_16] = {0};
        char token[NET_TV_LEN_512] = {0};

        snprintf(name, sizeof(name), "Client Set OSD %d", i + 1);
        snprintf(color, sizeof(color), "#%02X%02X%02X", 32 + i * 32, 200 - i * 24, 64 + i * 32);
        snprintf(token, sizeof(token), "client_custom_token_%d", i);

        pCfg->OsdInfo[i].nId = i + 1;
        pCfg->OsdInfo[i].bEnable = TRUE;
        strncpy(pCfg->OsdInfo[i].strName, name, sizeof(pCfg->OsdInfo[i].strName) - 1);
        FillOsdAttr(&pCfg->OsdInfo[i].stOsdAttr,
                    32,
                    128 + i * 40,
                    -1,
                    32,
                    (i % 2 == 0) ? OSD_FONT_SIZE_32 : OSD_FONT_SIZE_16,
                    OSD_COLOR_CUSTOMIZE,
                    color,
                    token);
    }

    for (i = DEMO_OSD_CUSTOM_MAX_NUM; i < DEMO_OSD_TOTAL_SLOT_NUM; ++i)
    {
        memset(&pCfg->OsdInfo[i], 0, sizeof(pCfg->OsdInfo[i]));
    }
}

static void PrintOSDCfg(const NET_TV_VIDEO_OSD_CFG_S* pCfg)
{
    int i = 0;
    int enabledCustomCount = 0;

    if (!pCfg)
    {
        return;
    }

    printf("\n[Client] ===== OSD 配置信息 =====\n");

    printf("  Align            : %s(%d)\n", OsdAlignToString(pCfg->enAlign), pCfg->enAlign);

    printf("\n  [Name OSD]\n");
    printf("    Enable         : %s\n", pCfg->stOsdNameInfo.bEnable ? "ON" : "OFF");
    printf("    Name           : %s\n", pCfg->stOsdNameInfo.strName);
    PrintOsdAttr("    ", &pCfg->stOsdNameInfo.stOsdAttr);

    printf("\n  [Time OSD]\n");
    printf("    Enable         : %s\n", pCfg->stOsdTimeInfo.bEnable ? "ON" : "OFF");
    printf("    EnableWeek     : %s\n", pCfg->stOsdTimeInfo.bEnableWeek ? "ON" : "OFF");
    printf("    TimeFormat     : %s(%d)\n",
           OsdTimeFormatToString(pCfg->stOsdTimeInfo.enTimeFormat),
           pCfg->stOsdTimeInfo.enTimeFormat);
    printf("    DateFormat     : %s(%d)\n",
           OsdDateFormatToString(pCfg->stOsdTimeInfo.enDateFormat),
           pCfg->stOsdTimeInfo.enDateFormat);
    PrintOsdAttr("    ", &pCfg->stOsdTimeInfo.stOsdAttr);

    printf("\n  [Custom OSD] first %d slots are used by current IPC capability\n", DEMO_OSD_CUSTOM_MAX_NUM);
    for (i = 0; i < DEMO_OSD_CUSTOM_MAX_NUM; ++i)
    {
        printf("    ---- OSD[%d] ----\n", i);
        printf("    Id             : %d\n", pCfg->OsdInfo[i].nId);
        printf("    Enable         : %s\n", pCfg->OsdInfo[i].bEnable ? "ON" : "OFF");
        printf("    Name           : %s\n", pCfg->OsdInfo[i].strName);
        PrintOsdAttr("    ", &pCfg->OsdInfo[i].stOsdAttr);
        if (pCfg->OsdInfo[i].bEnable)
        {
            ++enabledCustomCount;
        }
    }

    printf("  Enabled custom OSD count in first %d slots: %d\n",
           DEMO_OSD_CUSTOM_MAX_NUM,
           enabledCustomCount);

    printf("================================\n");
}

/* 获取设备基本信息 */
static void DoGetDeviceCfg()
{
    NET_TV_DEVICE_BASICINFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;

    printf("[Client] 调用 NET_TV_GetDevConfig 获取设备基本信息...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID,
        1, /* ChannelID */
        NET_TV_GET_DEVICECFG,
        &stInfo,
        (INT32)sizeof(stInfo),
        &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 获取设备基本信息成功! BytesReturned=%d\n", dwBytesReturned);
        PrintDeviceBasicInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取设备基本信息失败! Error=%d\n", NET_TV_GetLastError());
    }
}

/* 设置设备基本信息（使用示例数据） */
static void DoSetDeviceCfg()
{
    NET_TV_DEVICE_BASICINFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    /* 先获取一次当前配置，便于在其基础上修改 */
    INT32 dwBytesReturned = 0;
    BOOL bRetGet = NET_TV_GetDevConfig(
        g_lpUserID,
        1,
        NET_TV_GET_DEVICECFG,
        &stInfo,
        (INT32)sizeof(stInfo),
        &dwBytesReturned
    );

    if (!bRetGet)
    {
        printf("[Client] 预获取设备基本信息失败，直接使用默认示例数据进行设置. Error=%d\n", NET_TV_GetLastError());
        memset(&stInfo, 0, sizeof(stInfo));
    }

    /* 修改为 Demo 值（可以根据需要调整） */
    strncpy(stInfo.szDeviceName,   "ConfigDemoDevice", sizeof(stInfo.szDeviceName) - 1);
    strncpy(stInfo.szDevModel,     "Demo-Model-Config", sizeof(stInfo.szDevModel) - 1);
    strncpy(stInfo.szManufacturer, "DemoManufacturer-Config", sizeof(stInfo.szManufacturer) - 1);

    printf("[Client] 调用 NET_TV_SetDevConfig 设置设备基本信息(示例值)...\n");
    INT32 dwBytesReturnedSet = 0;
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID,
        1,
        NET_TV_SET_DEVICECFG,
        &stInfo,
        (INT32)sizeof(stInfo),
        &dwBytesReturnedSet
    );

    if (bRet)
    {
        printf("[Client] 设置设备基本信息成功! BytesReturned=%d\n", dwBytesReturnedSet);
        /* 再获取一次，验证修改结果 */
        DoGetDeviceCfg();
    }
    else
    {
        printf("[Client] 设置设备基本信息失败! Error=%d\n", NET_TV_GetLastError());
    }
}

/* 获取网络配置 */
static void DoGetNetworkCfg()
{
    NET_TV_NETWORKCFG_S stCfg;
    memset(&stCfg, 0, sizeof(stCfg));

    INT32 dwBytesReturned = 0;

    printf("[Client] 调用 NET_TV_GetDevConfig 获取网络配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID,
        1,
        NET_TV_GET_NETWORKCFG,
        &stCfg,
        (INT32)sizeof(stCfg),
        &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 获取网络配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintNetworkCfg(&stCfg);
    }
    else
    {
        printf("[Client] 获取网络配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

/* 设置网络配置（使用示例数据） */
static void DoSetNetworkCfg()
{
    NET_TV_NETWORKCFG_S stCfg;
    memset(&stCfg, 0, sizeof(stCfg));

    /* 同样先获取一次当前配置 */
    INT32 dwBytesReturned = 0;
    BOOL bRetGet = NET_TV_GetDevConfig(
        g_lpUserID,
        1,
        NET_TV_GET_NETWORKCFG,
        &stCfg,
        (INT32)sizeof(stCfg),
        &dwBytesReturned
    );

    if (!bRetGet)
    {
        printf("[Client] 预获取网络配置失败，直接使用默认示例数据进行设置. Error=%d\n", NET_TV_GetLastError());
        memset(&stCfg, 0, sizeof(stCfg));
    }

    /* 修改为 Demo 值 */
    stCfg.dwMTU     = 1400;
    stCfg.bIPv4DHCP = 0;
    strncpy(stCfg.szIpv4Address,   "192.168.1.150", sizeof(stCfg.szIpv4Address) - 1);
    strncpy(stCfg.szIPv4GateWay,   "192.168.1.1",   sizeof(stCfg.szIPv4GateWay) - 1);
    strncpy(stCfg.szIPv4SubnetMask,"255.255.255.0", sizeof(stCfg.szIPv4SubnetMask) - 1);

    printf("[Client] 调用 NET_TV_SetDevConfig 设置网络配置(示例值)...\n");
    INT32 dwBytesReturnedSet = 0;
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID,
        1,
        NET_TV_SET_NETWORKCFG,
        &stCfg,
        (INT32)sizeof(stCfg),
        &dwBytesReturnedSet
    );

    if (bRet)
    {
        printf("[Client] 设置网络配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        /* 再获取一次，验证修改结果 */
        DoGetNetworkCfg();
    }
    else
    {
        printf("[Client] 设置网络配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

/* 获取音频配置 */
static void DoGetAudioCfg()
{
    NET_TV_AUDIO_CFG_S stCfg;
    memset(&stCfg, 0, sizeof(stCfg));

    INT32 dwBytesReturned = 0;

    printf("[Client] 调用 NET_TV_GetDevConfig 获取音频配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID,
        1,
        NET_TV_GET_AUDIOCFG,
        &stCfg,
        (INT32)sizeof(stCfg),
        &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 获取音频配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintAudioCfg(&stCfg);
    }
    else
    {
        printf("[Client] 获取音频配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

/* 设置音频配置 */
static void DoSetAudioCfg()
{
    NET_TV_AUDIO_CFG_S stCfg;
    memset(&stCfg, 0, sizeof(stCfg));

    INT32 dwBytesReturned = 0;
    BOOL bRetGet = NET_TV_GetDevConfig(
        g_lpUserID,
        1,
        NET_TV_GET_AUDIOCFG,
        &stCfg,
        (INT32)sizeof(stCfg),
        &dwBytesReturned
    );

    if (!bRetGet)
    {
        printf("[Client] 预获取音频配置失败，使用默认值. Error=%d\n", NET_TV_GetLastError());
        memset(&stCfg, 0, sizeof(stCfg));
    }

    stCfg.bAudioSwitch = TRUE;
    stCfg.enInputType = NET_TV_AUDIO_INPUT_MICIN;
    stCfg.enFormat = NET_TV_AUDIO_FORMAT_AAC;
    stCfg.enSampRate = NET_TV_AUDIO_SAMPRATE_16000;
    stCfg.enBitRate = NET_TV_AUDIO_BITRATE_64K;
    stCfg.u32InputVolume = 65;
    stCfg.bDenoise = TRUE;
    stCfg.enOutputType = NET_TV_AUDIO_OUTPUT_SPEAKER;
    stCfg.u32OutputVolume = 55;

    printf("[Client] 调用 NET_TV_SetDevConfig 设置音频配置...\n");
    INT32 dwBytesReturnedSet = 0;
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID,
        1,
        NET_TV_SET_AUDIOCFG,
        &stCfg,
        (INT32)sizeof(stCfg),
        &dwBytesReturnedSet
    );

    if (bRet)
    {
        printf("[Client] 设置音频配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetAudioCfg();
    }
    else
    {
        printf("[Client] 设置音频配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

/* 获取视频码流配置 */
static void DoGetStreamCfg()
{
    NET_TV_VIDEO_ENCODE_OPTION_S stCfg;
    memset(&stCfg, 0, sizeof(stCfg));

    INT32 dwBytesReturned = 0;

    printf("[Client] 调用 NET_TV_GetDevConfig 获取视频码流配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID,
        1,
        NET_TV_GET_STREAMCFG,
        &stCfg,
        (INT32)sizeof(stCfg),
        &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 获取视频码流配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintStreamCfg(&stCfg);
    }
    else
    {
        printf("[Client] 获取视频码流配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

/* 设置视频码流配置 */
static void DoSetStreamCfg()
{
    NET_TV_VIDEO_ENCODE_OPTION_S stCfg;
    memset(&stCfg, 0, sizeof(stCfg));

    INT32 dwBytesReturned = 0;
    BOOL bRetGet = NET_TV_GetDevConfig(
        g_lpUserID,
        1,
        NET_TV_GET_STREAMCFG,
        &stCfg,
        (INT32)sizeof(stCfg),
        &dwBytesReturned
    );

    if (!bRetGet)
    {
        printf("[Client] 预获取视频码流配置失败，使用默认值. Error=%d\n", NET_TV_GetLastError());
        memset(&stCfg, 0, sizeof(stCfg));
    }

    stCfg.nId = NET_TV_LIVE_STREAM_INDEX_MAIN;
    stCfg.enVideoType = 0;
    stCfg.stVideoResolution.dwWidth = 1920;
    stCfg.stVideoResolution.dwHeight = 1080;
    stCfg.enBitrateType = 0;
    stCfg.enImageQuality = 60;
    stCfg.enFrameRate = 25;
    stCfg.nBitrateUpperLimit = 4096;
    stCfg.nAverageBitrate = 2048;
    stCfg.enVideoCodec = NET_TV_VIDEO_CODE_H264;
    stCfg.bSmartEnable = FALSE;
    stCfg.enEncodingComplexity = 1;
    stCfg.nIFrameInterval = 50;
    stCfg.enSvcEnable = 0;
    stCfg.nBitrateSmoothing = 50;

    printf("[Client] 调用 NET_TV_SetDevConfig 设置视频码流配置...\n");
    INT32 dwBytesReturnedSet = 0;
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID,
        1,
        NET_TV_SET_STREAMCFG,
        &stCfg,
        (INT32)sizeof(stCfg),
        &dwBytesReturnedSet
    );

    if (bRet)
    {
        printf("[Client] 设置视频码流配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetStreamCfg();
    }
    else
    {
        printf("[Client] 设置视频码流配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

/* 设置WIFI STA基础配置 */
static void DoSetConfigWifiSta()
{
    NET_TV_WIFI_STA_CFG_S stCfg;
    memset(&stCfg, 0, sizeof(stCfg));

    stCfg.bEnableWifi = TRUE;
    stCfg.bEnableBoost = FALSE;

    printf("[Client] 调用 NET_TV_SetDevConfig 设置WIFI STA基础配置...\n");
    INT32 dwBytesReturnedSet = 0;
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID,
        1,
        NET_TV_SET_CONFIG_WIFI_STA,
        &stCfg,
        (INT32)sizeof(stCfg),
        &dwBytesReturnedSet
    );

    if (bRet)
    {
        printf("[Client] 设置WIFI STA基础配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        PrintWifiStaCfg(&stCfg);
    }
    else
    {
        printf("[Client] 设置WIFI STA基础配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

/* 连接WIFI STA */
static void DoConnectWifiSta()
{
    NET_TV_WIFI_STA_CONNECT_S stCfg;
    memset(&stCfg, 0, sizeof(stCfg));

    strncpy(stCfg.szSsid, "DemoWifi", sizeof(stCfg.szSsid) - 1);
    stCfg.nSecurityMode = NET_TV_WIFI_SECURITY_WPA_PERSONAL;
    strncpy(stCfg.szIpAddress, "192.168.1.120", sizeof(stCfg.szIpAddress) - 1);
    strncpy(stCfg.szPassword, "12345678", sizeof(stCfg.szPassword) - 1);
    strncpy(stCfg.szPairwise, "CCMP", sizeof(stCfg.szPairwise) - 1);
    stCfg.nWepKeyLen = 128;
    stCfg.bWepIsHex = FALSE;
    strncpy(stCfg.szAuthAlg, "OPEN", sizeof(stCfg.szAuthAlg) - 1);
    stCfg.nWepKeyCount = 1;
    stCfg.astWepKeys[0].nIndex = 1;
    strncpy(stCfg.astWepKeys[0].szValue, "1234567890", sizeof(stCfg.astWepKeys[0].szValue) - 1);
    strncpy(stCfg.szCtrlInterface, "/var/run/wpa_supplicant", sizeof(stCfg.szCtrlInterface) - 1);
    strncpy(stCfg.szInterfaceName, "wlan0", sizeof(stCfg.szInterfaceName) - 1);

    printf("[Client] 调用 NET_TV_SetDevConfig 连接WIFI STA...\n");
    INT32 dwBytesReturnedSet = 0;
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID,
        1,
        NET_TV_CONNECT_WIFI_STA,
        &stCfg,
        (INT32)sizeof(stCfg),
        &dwBytesReturnedSet
    );

    if (bRet)
    {
        printf("[Client] 连接WIFI STA成功! BytesReturned=%d\n", dwBytesReturnedSet);
        PrintWifiStaConnect(&stCfg);
    }
    else
    {
        printf("[Client] 连接WIFI STA失败! Error=%d\n", NET_TV_GetLastError());
    }
}

/* 断开WIFI STA */
static void DoDisconnectWifiSta()
{
    NET_TV_WIFI_STA_CONNECT_S stCfg;
    memset(&stCfg, 0, sizeof(stCfg));

    strncpy(stCfg.szSsid, "DemoWifi", sizeof(stCfg.szSsid) - 1);
    strncpy(stCfg.szInterfaceName, "wlan0", sizeof(stCfg.szInterfaceName) - 1);

    printf("[Client] 调用 NET_TV_SetDevConfig 断开WIFI STA...\n");
    INT32 dwBytesReturnedSet = 0;
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID,
        1,
        NET_TV_DISCONNECT_WIFI_STA,
        &stCfg,
        (INT32)sizeof(stCfg),
        &dwBytesReturnedSet
    );

    if (bRet)
    {
        printf("[Client] 断开WIFI STA成功! BytesReturned=%d\n", dwBytesReturnedSet);
    }
    else
    {
        printf("[Client] 断开WIFI STA失败! Error=%d\n", NET_TV_GetLastError());
    }
}

/* 获取4G配置 */
static void DoGet4GInfo()
{
    NET_TV_4G_INFO_S stCfg;
    memset(&stCfg, 0, sizeof(stCfg));

    INT32 dwBytesReturned = 0;

    printf("[Client] 调用 NET_TV_GetDevConfig 获取4G配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID,
        1,
        NET_TV_GET_4G_INFO,
        &stCfg,
        (INT32)sizeof(stCfg),
        &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 获取4G配置成功! BytesReturned=%d\n", dwBytesReturned);
        Print4GInfo(&stCfg);
    }
    else
    {
        printf("[Client] 获取4G配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

/* 设置4G配置 */
static void DoSet4GInfo()
{
    NET_TV_4G_INFO_S stCfg;
    memset(&stCfg, 0, sizeof(stCfg));

    strncpy(stCfg.szApn, "ctnet", sizeof(stCfg.szApn) - 1);
    strncpy(stCfg.szUserName, "user", sizeof(stCfg.szUserName) - 1);
    strncpy(stCfg.szPassword, "pwd", sizeof(stCfg.szPassword) - 1);
    strncpy(stCfg.szCallNumber, "*99#", sizeof(stCfg.szCallNumber) - 1);
    stCfg.nMtu = 1500;
    stCfg.nAuthMode = 0;
    stCfg.nNetworkMode = 0;
    stCfg.nDialMode = 0;

    printf("[Client] 调用 NET_TV_SetDevConfig 设置4G配置...\n");
    INT32 dwBytesReturnedSet = 0;
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID,
        1,
        NET_TV_SET_4G_INFO,
        &stCfg,
        (INT32)sizeof(stCfg),
        &dwBytesReturnedSet
    );

    if (bRet)
    {
        printf("[Client] 设置4G配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGet4GInfo();
    }
    else
    {
        printf("[Client] 设置4G配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

/* 设置热点配置 */
static void DoSetHotspotInfo()
{
    NET_TV_HOTSPOT_INFO_S stCfg;
    memset(&stCfg, 0, sizeof(stCfg));

    stCfg.bEnabled = TRUE;
    strncpy(stCfg.szSsid, "DemoHotspot", sizeof(stCfg.szSsid) - 1);
    strncpy(stCfg.szSecurityMode, "WPA2-personal", sizeof(stCfg.szSecurityMode) - 1);
    strncpy(stCfg.szEncryptionType, "TKIP", sizeof(stCfg.szEncryptionType) - 1);
    strncpy(stCfg.szPassword, "12345678", sizeof(stCfg.szPassword) - 1);
    strncpy(stCfg.szConfirmPassword, "12345678", sizeof(stCfg.szConfirmPassword) - 1);

    printf("[Client] 调用 NET_TV_SetDevConfig 设置热点配置...\n");
    INT32 dwBytesReturnedSet = 0;
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID,
        1,
        NET_TV_SET_HOTSPOT_INFO,
        &stCfg,
        (INT32)sizeof(stCfg),
        &dwBytesReturnedSet
    );

    if (bRet)
    {
        printf("[Client] 设置热点配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        PrintHotspotInfo(&stCfg);
    }
    else
    {
        printf("[Client] 设置热点配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

/* 获取热点连接设备 */
static void DoGetHotspotConn()
{
    NET_TV_HOTSPOT_CONN_INFO_S stCfg;
    memset(&stCfg, 0, sizeof(stCfg));

    INT32 dwBytesReturned = 0;

    printf("[Client] 调用 NET_TV_GetDevConfig 获取热点连接设备...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID,
        1,
        NET_TV_GET_HOTSPOT_CONN,
        &stCfg,
        (INT32)sizeof(stCfg),
        &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 获取热点连接设备成功! BytesReturned=%d\n", dwBytesReturned);
        PrintHotspotConnInfo(&stCfg);
    }
    else
    {
        printf("[Client] 获取热点连接设备失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoGetSecurityServicesInfo()
{
    NET_TV_SECURITY_SERVICES_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_GetDevConfig 获取安全服务配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID,
        1,
        NET_TV_GET_SECURITY_SERVICES_INFO,
        &stInfo,
        (INT32)sizeof(stInfo),
        &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 获取安全服务配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintSecurityServicesInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取安全服务配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoSetSecurityServicesInfo()
{
    NET_TV_SECURITY_SERVICES_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    stInfo.stLoginLock.bIllegalLoginEnable = TRUE;
    stInfo.stLoginLock.nCheckInterval = 30;
    stInfo.stLoginLock.nMaxErrorTimes = 5;
    stInfo.stLoginLock.nLockDuration = 0;
    stInfo.stPwdPolicy.bPwdSecurityLevelEnable = FALSE;
    stInfo.stPwdPolicy.bAllowLowLevelPwdLogin = FALSE;
    stInfo.stSshAdmin.bSshEnable = TRUE;
    stInfo.stSshAdmin.nSshPort = 22;
    strncpy(stInfo.stSshAdmin.szSshStartTime, "2026-05-06 10:00:00", sizeof(stInfo.stSshAdmin.szSshStartTime) - 1);
    strncpy(stInfo.stSshAdmin.szSshCountdown, "08:00:00", sizeof(stInfo.stSshAdmin.szSshCountdown) - 1);

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_SetDevConfig 设置安全服务配置...\n");
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID,
        1,
        NET_TV_SET_SECURITY_SERVICES_INFO,
        &stInfo,
        (INT32)sizeof(stInfo),
        &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 设置安全服务配置成功! BytesReturned=%d\n", dwBytesReturned);
        DoGetSecurityServicesInfo();
    }
    else
    {
        printf("[Client] 设置安全服务配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoGetSshCountdown()
{
    NET_TV_SSH_COUNTDOWN_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_GetDevConfig 获取SSH倒计时...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID,
        1,
        NET_TV_GET_SSH_COUNTDOWN,
        &stInfo,
        (INT32)sizeof(stInfo),
        &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 获取SSH倒计时成功! BytesReturned=%d Countdown=%s\n", dwBytesReturned, stInfo.szCountdown);
    }
    else
    {
        printf("[Client] 获取SSH倒计时失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void FillDemoLogQuery(NET_TV_LOG_LIST_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    memset(pInfo, 0, sizeof(*pInfo));
    pInfo->stCond.nType = 0;
    pInfo->stCond.nAction = 0;
    strncpy(pInfo->stCond.szStartTime, "2026-05-06 00:00:00", sizeof(pInfo->stCond.szStartTime) - 1);
    strncpy(pInfo->stCond.szEndTime, "2026-05-06 23:59:59", sizeof(pInfo->stCond.szEndTime) - 1);
    pInfo->stPage.nCurPage = 1;
    pInfo->stPage.nPageSize = 20;
}

static void DoQueryLog(INT32 dwCommand, const char* pszName)
{
    NET_TV_LOG_LIST_S stInfo;
    FillDemoLogQuery(&stInfo);

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_GetDevConfig %s...\n", pszName);
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID,
        1,
        dwCommand,
        &stInfo,
        (INT32)sizeof(stInfo),
        &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] %s成功! BytesReturned=%d\n", pszName, dwBytesReturned);
        PrintLogListInfo(&stInfo);
    }
    else
    {
        printf("[Client] %s失败! Error=%d\n", pszName, NET_TV_GetLastError());
    }
}

static void DoFindLog()
{
    DoQueryLog(NET_TV_FIND_LOG, "查询日志");
}

static void DoExportLog()
{
    DoQueryLog(NET_TV_EXPORT_LOG, "导出日志");
}

static void DoGetLogServer()
{
    NET_TV_LOG_SERVER_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_GetDevConfig 获取日志服务器配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID,
        1,
        NET_TV_GET_LOG_SERVER,
        &stInfo,
        (INT32)sizeof(stInfo),
        &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 获取日志服务器配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintLogServerInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取日志服务器配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void FillDemoLogServer(NET_TV_LOG_SERVER_INFO_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    memset(pInfo, 0, sizeof(*pInfo));
    pInfo->bEnable = TRUE;
    pInfo->bEnSsl = FALSE;
    strncpy(pInfo->szServerAddr, "oam.itc-pa.cn", sizeof(pInfo->szServerAddr) - 1);
    pInfo->nPort = 1883;
}

static void DoSetLogServer()
{
    NET_TV_LOG_SERVER_INFO_S stInfo;
    FillDemoLogServer(&stInfo);

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_SetDevConfig 设置日志服务器配置...\n");
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID,
        1,
        NET_TV_SET_LOG_SERVER,
        &stInfo,
        (INT32)sizeof(stInfo),
        &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 设置日志服务器配置成功! BytesReturned=%d\n", dwBytesReturned);
        DoGetLogServer();
    }
    else
    {
        printf("[Client] 设置日志服务器配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void FillDemoRecordInfo(NET_TV_RECORD_INFO_S* pInfo, INT32 nRecordStatus)
{
    if (!pInfo)
    {
        return;
    }

    memset(pInfo, 0, sizeof(*pInfo));
    pInfo->nChnId = 0;
    pInfo->nVideoStatus = 1;
    pInfo->nAudioStatus = 1;
    pInfo->nRecordStatus = nRecordStatus;
    pInfo->nRecordFormat = 0;
    pInfo->nEventType = 0;
    strncpy(pInfo->szPath, "/tmp/", sizeof(pInfo->szPath) - 1);
    strncpy(pInfo->szRedunPath, "/tmp/", sizeof(pInfo->szRedunPath) - 1);
    strncpy(pInfo->szRecordName, "manual_record_demo.ts", sizeof(pInfo->szRecordName) - 1);
    strncpy(pInfo->szRecordTime, "2026-05-06 10:00:00", sizeof(pInfo->szRecordTime) - 1);
    pInfo->nStreamType = 0;
}

static void DoControlRecordInfo(INT32 nRecordStatus)
{
    NET_TV_RECORD_INFO_S stInfo;
    FillDemoRecordInfo(&stInfo, nRecordStatus);

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_SetDevConfig 控制手动录像...\n");
    PrintRecordInfo(&stInfo);
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID,
        1,
        NET_TV_CONTROL_RECORD_INFO,
        &stInfo,
        (INT32)sizeof(stInfo),
        &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 控制手动录像成功! BytesReturned=%d\n", dwBytesReturned);
    }
    else
    {
        printf("[Client] 控制手动录像失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoGetRecordStatus()
{
    NET_TV_RECORD_STATUS_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_GetDevConfig 获取录像状态...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID,
        1,
        NET_TV_GET_RECORD_STATUS,
        &stInfo,
        (INT32)sizeof(stInfo),
        &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 获取录像状态成功! BytesReturned=%d\n", dwBytesReturned);
        PrintRecordStatusInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取录像状态失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void FillDemoRecordSchedule(NET_TV_RECORD_SCHEDULE_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    memset(pInfo, 0, sizeof(*pInfo));
    pInfo->bEnable = TRUE;
    pInfo->nDayScheduleCount = NET_TV_PLAN_DAY_NUM_AWEEK;
    for (int i = 0; i < NET_TV_PLAN_DAY_NUM_AWEEK; ++i)
    {
        pInfo->astDaySchedules[i].nDayOfWeek = i + 1;
        pInfo->astDaySchedules[i].nRecordTimeCount = 1;
        pInfo->astDaySchedules[i].astRecordTimes[0].nType = 1;
        pInfo->astDaySchedules[i].astRecordTimes[0].nStartTime = 0;
        pInfo->astDaySchedules[i].astRecordTimes[0].nEndTime = 86400;
    }
}

static void DoGetRecordSchedule()
{
    NET_TV_RECORD_SCHEDULE_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_GetDevConfig 获取录像计划...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID,
        1,
        NET_TV_GET_RECORD_SCHEDULE,
        &stInfo,
        (INT32)sizeof(stInfo),
        &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 获取录像计划成功! BytesReturned=%d\n", dwBytesReturned);
        PrintRecordSchedule(&stInfo);
    }
    else
    {
        printf("[Client] 获取录像计划失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoSetRecordSchedule()
{
    NET_TV_RECORD_SCHEDULE_S stInfo;
    FillDemoRecordSchedule(&stInfo);

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_SetDevConfig 设置录像计划...\n");
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID,
        1,
        NET_TV_SET_RECORD_SCHEDULE,
        &stInfo,
        (INT32)sizeof(stInfo),
        &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 设置录像计划成功! BytesReturned=%d\n", dwBytesReturned);
        DoGetRecordSchedule();
    }
    else
    {
        printf("[Client] 设置录像计划失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void FillDemoRecordAdvancedParam(NET_TV_RECORD_ADVANCED_PARAM_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    memset(pInfo, 0, sizeof(*pInfo));
    pInfo->bLoopWrite = TRUE;
    pInfo->nPreTime = 0;
    pInfo->nDelayTime = 5;
    pInfo->nStreamType = 0;
}

static void DoGetRecordAdvancedParam()
{
    NET_TV_RECORD_ADVANCED_PARAM_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_GetDevConfig 获取录像高级参数...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID,
        1,
        NET_TV_GET_RECORD_ADVANCED_PARAM,
        &stInfo,
        (INT32)sizeof(stInfo),
        &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 获取录像高级参数成功! BytesReturned=%d\n", dwBytesReturned);
        PrintRecordAdvancedParam(&stInfo);
    }
    else
    {
        printf("[Client] 获取录像高级参数失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoSetRecordAdvancedParam()
{
    NET_TV_RECORD_ADVANCED_PARAM_S stInfo;
    FillDemoRecordAdvancedParam(&stInfo);

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_SetDevConfig 设置录像高级参数...\n");
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID,
        1,
        NET_TV_SET_RECORD_ADVANCED_PARAM,
        &stInfo,
        (INT32)sizeof(stInfo),
        &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 设置录像高级参数成功! BytesReturned=%d\n", dwBytesReturned);
        DoGetRecordAdvancedParam();
    }
    else
    {
        printf("[Client] 设置录像高级参数失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void FillDemoRecordFindCond(NET_TV_RECORD_FILE_LIST_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    memset(pInfo, 0, sizeof(*pInfo));
    pInfo->stFind.nChnId = 0;
    pInfo->stFind.nType = 0;
    strncpy(pInfo->stFind.szYear, "2026", sizeof(pInfo->stFind.szYear) - 1);
    strncpy(pInfo->stFind.szMonth, "05", sizeof(pInfo->stFind.szMonth) - 1);
    strncpy(pInfo->stFind.szDate, "2026-05-06", sizeof(pInfo->stFind.szDate) - 1);
    strncpy(pInfo->stFind.szStartTime, "2026-05-06 00:00:00", sizeof(pInfo->stFind.szStartTime) - 1);
    strncpy(pInfo->stFind.szEndTime, "2026-05-06 23:59:59", sizeof(pInfo->stFind.szEndTime) - 1);
}

static void DoFindRecordFileInfo()
{
    NET_TV_RECORD_FILE_LIST_S stInfo;
    FillDemoRecordFindCond(&stInfo);

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_GetDevConfig 查找录像文件...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID,
        1,
        NET_TV_FIND_RECORD_FILE_INFO,
        &stInfo,
        (INT32)sizeof(stInfo),
        &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 查找录像文件成功! BytesReturned=%d\n", dwBytesReturned);
        PrintRecordFileList(&stInfo);
    }
    else
    {
        printf("[Client] 查找录像文件失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void FillDemoRecordDownloadList(NET_TV_RECORD_DOWNLOAD_LIST_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    memset(pInfo, 0, sizeof(*pInfo));
    pInfo->nDownloadCount = 1;
    pInfo->astDownloads[0].nChnId = 0;
    strncpy(pInfo->astDownloads[0].szPath, "/tmp/manual_record_demo.ts", sizeof(pInfo->astDownloads[0].szPath) - 1);
    strncpy(pInfo->astDownloads[0].szStartTime, "2026-05-06 10:00:00", sizeof(pInfo->astDownloads[0].szStartTime) - 1);
    strncpy(pInfo->astDownloads[0].szEndTime, "2026-05-06 10:30:00", sizeof(pInfo->astDownloads[0].szEndTime) - 1);
}

static void DoDownloadRecordFile()
{
    NET_TV_RECORD_DOWNLOAD_LIST_S stInfo;
    FillDemoRecordDownloadList(&stInfo);

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_SetDevConfig 下载录像文件...\n");
    PrintRecordDownloadList(&stInfo);
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID,
        1,
        NET_TV_DOWNLOAD_RECORD_FILE,
        &stInfo,
        (INT32)sizeof(stInfo),
        &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 下载录像文件请求成功! BytesReturned=%d\n", dwBytesReturned);
    }
    else
    {
        printf("[Client] 下载录像文件请求失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoTestLogServer()
{
    NET_TV_LOG_SERVER_INFO_S stInfo;
    FillDemoLogServer(&stInfo);

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_SetDevConfig 测试日志服务器配置...\n");
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID,
        1,
        NET_TV_TEST_LOG_SERVER,
        &stInfo,
        (INT32)sizeof(stInfo),
        &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 测试日志服务器配置成功! BytesReturned=%d\n", dwBytesReturned);
    }
    else
    {
        printf("[Client] 测试日志服务器配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

/* 获取OSD能力集配置 */
static void DoGetOSDCapCfg()
{
    NET_TV_VIDEO_OSD_CFG_S stCfg;
    memset(&stCfg, 0, sizeof(stCfg));

    INT32 dwBytesReturned = 0;

    printf("[Client] 调用 NET_TV_GetDevConfig 获取OSD能力集配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID,
        1,
        NET_TV_GET_OSDCAPCFG,
        &stCfg,
        (INT32)sizeof(stCfg),
        &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 获取OSD能力集配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintOSDCfg(&stCfg);
    }
    else
    {
        printf("[Client] 获取OSD能力集配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}
/* 设置OSD能力集配置（使用示例数据） */
static void DoSetOSDCapCfg()
{
    NET_TV_VIDEO_OSD_CFG_S stCfg;
    memset(&stCfg, 0, sizeof(stCfg));

    /* 同样先获取一次当前配置 */
    INT32 dwBytesReturned = 0;
    BOOL bRetGet = NET_TV_GetDevConfig(
        g_lpUserID,
        1,
        NET_TV_GET_OSDCAPCFG,
        &stCfg,
        (INT32)sizeof(stCfg),
        &dwBytesReturned
    );

    if (!bRetGet)
    {
        printf("[Client] 预获取OSD能力集配置失败，直接使用默认示例数据进行设置。Error=%d\n", NET_TV_GetLastError());
        memset(&stCfg, 0, sizeof(stCfg));
    }

    /* 修改为 Demo 值，固定只使用当前能力声明的前4个自定义字符叠加槽位 */
    BuildDemoOSDCfg(&stCfg);

    printf("[Client] 调用 NET_TV_SetDevConfig 设置OSD能力集配置(示例值)...\n");
    PrintOSDCfg(&stCfg);

    INT32 dwBytesReturnedSet = 0;
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID,
        1,
        NET_TV_SET_OSDCAPCFG,
        &stCfg,
        (INT32)sizeof(stCfg),
        &dwBytesReturnedSet
    );

    if (bRet)
    {
        printf("[Client] 设置OSD能力集配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        /* 再获取一次，验证设置结果 */
        DoGetOSDCapCfg();
    }
    else
    {
        printf("[Client] 设置OSD能力集配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

/* 一键验证OSD配置 Get -> Set -> Get */
static void DoTestOSDCapCfg()
{
    printf("\n[Client] ===== OSD配置一键验证开始 =====\n");
    printf("[Client] Step 1: 获取当前OSD配置\n");
    DoGetOSDCapCfg();
    printf("[Client] Step 2: 设置Demo OSD配置\n");
    DoSetOSDCapCfg();
    printf("[Client] ===== OSD配置一键验证结束 =====\n");
}

/* 获取RTSP流地址 */
static void DoGetRtspUrl()
{
    NET_TV_RTSP_URL_INFO_S stRtsp;
    memset(&stRtsp, 0, sizeof(stRtsp));

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_GetDevConfig 获取RTSP流地址...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID,
        1,
        NET_TV_GET_RTSPURLCFG,
        &stRtsp,
        (INT32)sizeof(stRtsp),
        &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 获取RTSP流地址成功! BytesReturned=%d\n", dwBytesReturned);
        printf("  Channel     : %d\n", stRtsp.dwChannel);
        printf("  StreamIndex : %d\n", stRtsp.dwStreamIndex);
        printf("  RtspUrl     : %s\n", stRtsp.szRtspUrl);
    }
    else
    {
        printf("[Client] 获取RTSP流地址失败! Error=%d\n", NET_TV_GetLastError());
    }
}
/* 打印移动侦测配置 */
static void PrintMotionAlarmInfo(const NET_TV_MOTION_ALARM_INFO_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 移动侦测配置 =====\n");
    printf("  Enable              : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  DynamicAnalysis     : %s\n", pInfo->bDynamicAnalysisEnable ? "ON" : "OFF");
    printf("  Mode                : %s\n", pInfo->dwMode == NET_TV_MOTION_MODE_NORMAL ? "普通模式" : "专家模式");
    if (pInfo->dwMode == NET_TV_MOTION_MODE_NORMAL)
    {
        printf("  Sensitivity         : %d\n", pInfo->stNormalMode.nSensitivity);
        printf("  RegionType          : %s\n", pInfo->stNormalMode.nRegionType == 0 ? "筒型" : "网格");
        if (pInfo->stNormalMode.nRegionType == 0)
        {
            printf("  Rect               : [%d,%d,%d,%d]\n",
                   pInfo->stNormalMode.nRectLeft, pInfo->stNormalMode.nRectTop,
                   pInfo->stNormalMode.nRectRight, pInfo->stNormalMode.nRectBottom);
        }
    }
    printf("=================================\n");
}

/* 打印遮挡报警配置 */
static void PrintTamperAlarmInfo(const NET_TV_TAMPER_ALARM_INFO_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 遮挡报警配置 =====\n");
    printf("  Enable      : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  Sensitivity : %d\n", pInfo->dwSensitivity);
    printf("  Rect        : [%d,%d,%d,%d]\n",
           pInfo->nRectLeft, pInfo->nRectTop,
           pInfo->nRectRight, pInfo->nRectBottom);
    printf("=================================\n");
}

/* 打印隐私遮盖配置 */
static void PrintPrivacyMaskCfg(const NET_TV_PRIVACY_MASK_CFG_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    INT32 nCount = pInfo->dwAreaCount;
    if (nCount < 0)
    {
        nCount = 0;
    }
    if (nCount > NET_TV_MAX_PRIVACY_MASK_AREA_NUM)
    {
        nCount = NET_TV_MAX_PRIVACY_MASK_AREA_NUM;
    }

    printf("\n[Client] ===== 隐私遮盖配置 =====\n");
    printf("  Enable    : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  AreaCount : %d\n", pInfo->dwAreaCount);
    for (int i = 0; i < nCount; ++i)
    {
        const NET_TV_PRIVACY_MASK_AREA_S* pArea = &pInfo->astArea[i];
        printf("  Area[%d]   : ID=%d, Enable=%s, Rect=[%d,%d,%d,%d]\n",
               i,
               pArea->nAreaID,
               pArea->bEnable ? "ON" : "OFF",
               pArea->nRectLeft,
               pArea->nRectTop,
               pArea->nRectRight,
               pArea->nRectBottom);
    }
    printf("=================================\n");
}

/* 打印越界检测配置 */
static void PrintCrossLineAlarmInfo(const NET_TV_CROSS_LINE_ALARM_INFO_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 越界检测配置 =====\n");
    printf("  Enable    : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  RuleCount : %d\n", pInfo->dwRuleCount);
    for (int i = 0; i < pInfo->dwRuleCount && i < 4; i++)
    {
        printf("  Rule[%d]   : Enable=%s, StartPos=(%.2f,%.2f), EndPos=(%.2f,%.2f), Sensitivity=%d\n",
               i, pInfo->astRule[i].bEnable ? "ON" : "OFF",
               pInfo->astRule[i].fStartPosX, pInfo->astRule[i].fStartPosY,
               pInfo->astRule[i].fEndPosX, pInfo->astRule[i].fEndPosY,
               pInfo->astRule[i].nSensitivity);
    }
    printf("=================================\n");
}

/* 打印入侵检测配置 */
static void PrintIntrusionAlarmInfo(const NET_TV_INTRUSION_ALARM_INFO_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 入侵检测配置 =====\n");
    printf("  Enable    : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  RuleCount : %d\n", pInfo->dwRuleCount);
    for (int i = 0; i < pInfo->dwRuleCount && i < 4; i++)
    {
        printf("  Rule[%d]   : Enable=%s, PointCount=%d, Sensitivity=%d, TimeThreshold=%d\n",
               i, pInfo->astRule[i].bEnable ? "ON" : "OFF",
               pInfo->astRule[i].dwPointCount,
               pInfo->astRule[i].nSensitivity,
               pInfo->astRule[i].nTimeThreshold);
    }
    printf("=================================\n");
}

static void PrintEnterRegionAlarmInfo(const NET_TV_ENTER_REGION_ALARM_INFO_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 进入区域侦测配置 =====\n");
    printf("  Enable    : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  RuleCount : %d\n", pInfo->dwRuleCount);
    for (int i = 0; i < pInfo->dwRuleCount && i < 4; i++)
    {
        printf("  Rule[%d]   : Enable=%s, PointCount=%d, Sensitivity=%d, TimeThreshold=%d\n",
               i,
               pInfo->astRule[i].bEnable ? "ON" : "OFF",
               pInfo->astRule[i].dwPointCount,
               pInfo->astRule[i].nSensitivity,
               pInfo->astRule[i].nTimeThreshold);
    }
    printf("=====================================\n");
}

static void PrintLeaveRegionAlarmInfo(const NET_TV_LEAVE_REGION_ALARM_INFO_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 离开区域侦测配置 =====\n");
    printf("  Enable    : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  RuleCount : %d\n", pInfo->dwRuleCount);
    for (int i = 0; i < pInfo->dwRuleCount && i < 4; i++)
    {
        printf("  Rule[%d]   : Enable=%s, PointCount=%d, Sensitivity=%d, TimeThreshold=%d\n",
               i,
               pInfo->astRule[i].bEnable ? "ON" : "OFF",
               pInfo->astRule[i].dwPointCount,
               pInfo->astRule[i].nSensitivity,
               pInfo->astRule[i].nTimeThreshold);
    }
    printf("=====================================\n");
}

/* 打印徘徊侦测配置 */
static void PrintLoiteringAlarmInfo(const NET_TV_LOITERING_ALARM_INFO_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 徘徊侦测配置 =====\n");
    printf("  Enable    : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  RuleCount : %d\n", pInfo->dwRuleCount);
    for (int i = 0; i < pInfo->dwRuleCount && i < 4; i++)
    {
        printf("  Rule[%d]   : Enable=%s, PointCount=%d, Sensitivity=%d, TimeThreshold=%d\n",
               i, pInfo->astRule[i].bEnable ? "ON" : "OFF",
               pInfo->astRule[i].dwPointCount,
               pInfo->astRule[i].nSensitivity,
               pInfo->astRule[i].nTimeThreshold);
    }
    printf("=================================\n");
}

/* 获取移动侦测配置 */
static void DoGetMotionAlarm()
{
    NET_TV_MOTION_ALARM_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;

    printf("[Client] 调用 NET_TV_GetDevConfig 获取移动侦测配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID,
        1,
        NET_TV_GET_MOTIONALARM,
        &stInfo,
        (INT32)sizeof(stInfo),
        &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 获取移动侦测配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintMotionAlarmInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取移动侦测配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

/* 设置移动侦测配置 */
static void DoSetMotionAlarm()
{
    NET_TV_MOTION_ALARM_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    BOOL bRetGet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_MOTIONALARM,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (!bRetGet)
    {
        printf("[Client] 预获取移动侦测配置失败，使用默认值. Error=%d\n", NET_TV_GetLastError());
        memset(&stInfo, 0, sizeof(stInfo));
    }

    /* 修改为示例值 */
    stInfo.bEnable = TRUE;
    stInfo.bDynamicAnalysisEnable = TRUE;
    stInfo.dwMode = NET_TV_MOTION_MODE_NORMAL;
    stInfo.stNormalMode.nSensitivity = 60;
    stInfo.stNormalMode.nRegionType = 0;
    stInfo.stNormalMode.nRectLeft = 150;
    stInfo.stNormalMode.nRectTop = 150;
    stInfo.stNormalMode.nRectRight = 750;
    stInfo.stNormalMode.nRectBottom = 550;

    printf("[Client] 调用 NET_TV_SetDevConfig 设置移动侦测配置...\n");
    INT32 dwBytesReturnedSet = 0;
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_MOTIONALARM,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturnedSet
    );

    if (bRet)
    {
        printf("[Client] 设置移动侦测配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetMotionAlarm();
    }
    else
    {
        printf("[Client] 设置移动侦测配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

/* 获取遮挡报警配置 */
static void DoGetTamperAlarm()
{
    NET_TV_TAMPER_ALARM_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;

    printf("[Client] 调用 NET_TV_GetDevConfig 获取遮挡报警配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_TAMPERALARM,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 获取遮挡报警配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintTamperAlarmInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取遮挡报警配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

/* 设置遮挡报警配置 */
static void DoSetTamperAlarm()
{
    NET_TV_TAMPER_ALARM_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    BOOL bRetGet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_TAMPERALARM,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (!bRetGet)
    {
        printf("[Client] 预获取遮挡报警配置失败，使用默认值. Error=%d\n", NET_TV_GetLastError());
        memset(&stInfo, 0, sizeof(stInfo));
    }

    /* 修改为示例值 */
    stInfo.bEnable = TRUE;
    stInfo.dwSensitivity = 3;
    stInfo.nRectLeft = 300;
    stInfo.nRectTop = 300;
    stInfo.nRectRight = 600;
    stInfo.nRectBottom = 500;

    printf("[Client] 调用 NET_TV_SetDevConfig 设置遮挡报警配置...\n");
    INT32 dwBytesReturnedSet = 0;
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_TAMPERALARM,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturnedSet
    );

    if (bRet)
    {
        printf("[Client] 设置遮挡报警配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetTamperAlarm();
    }
    else
    {
        printf("[Client] 设置遮挡报警配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

/* 获取隐私遮盖配置 */
static void DoGetPrivacyMaskCfg()
{
    NET_TV_PRIVACY_MASK_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;

    printf("[Client] 调用 NET_TV_GetDevConfig 获取隐私遮盖配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_PRIVACYMASKCFG,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 获取隐私遮盖配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintPrivacyMaskCfg(&stInfo);
    }
    else
    {
        printf("[Client] 获取隐私遮盖配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

/* 设置隐私遮盖配置 */
static void DoSetPrivacyMaskCfg()
{
    NET_TV_PRIVACY_MASK_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    BOOL bRetGet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_PRIVACYMASKCFG,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (!bRetGet)
    {
        printf("[Client] 预获取隐私遮盖配置失败，使用默认值. Error=%d\n", NET_TV_GetLastError());
        memset(&stInfo, 0, sizeof(stInfo));
    }

    stInfo.bEnable = TRUE;
    stInfo.dwAreaCount = 1;
    stInfo.astArea[0].nAreaID = 0;
    stInfo.astArea[0].bEnable = TRUE;
    stInfo.astArea[0].nRectLeft = 120;
    stInfo.astArea[0].nRectTop = 120;
    stInfo.astArea[0].nRectRight = 420;
    stInfo.astArea[0].nRectBottom = 320;

    printf("[Client] 调用 NET_TV_SetDevConfig 设置隐私遮盖配置...\n");
    INT32 dwBytesReturnedSet = 0;
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_PRIVACYMASKCFG,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturnedSet
    );

    if (bRet)
    {
        printf("[Client] 设置隐私遮盖配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetPrivacyMaskCfg();
    }
    else
    {
        printf("[Client] 设置隐私遮盖配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

/* 获取越界检测配置 */
static void DoGetCrossLineAlarm()
{
    NET_TV_CROSS_LINE_ALARM_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;

    printf("[Client] 调用 NET_TV_GetDevConfig 获取越界检测配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 
        1, 
        NET_TV_GET_CROSSLINEALARM,
        &stInfo, 
        (INT32)sizeof(stInfo),
        &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 获取越界检测配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintCrossLineAlarmInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取越界检测配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

/* 设置越界检测配置 */
static void DoSetCrossLineAlarm()
{
    NET_TV_CROSS_LINE_ALARM_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    BOOL bRetGet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_CROSSLINEALARM,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (!bRetGet)
    {
        printf("[Client] 预获取越界检测配置失败，使用默认值. Error=%d\n", NET_TV_GetLastError());
        memset(&stInfo, 0, sizeof(stInfo));
    }

    /* 修改为示例值 */
    stInfo.bEnable = TRUE;
    stInfo.dwRuleCount = 1;
    stInfo.astRule[0].bEnable = TRUE;
    stInfo.astRule[0].fStartPosX = 0.2f;
    stInfo.astRule[0].fStartPosY = 0.3f;
    stInfo.astRule[0].fEndPosX = 0.8f;
    stInfo.astRule[0].fEndPosY = 0.7f;
    stInfo.astRule[0].enCrossDirection = NET_TV_CROSS_BOTH_WAYS;
    stInfo.astRule[0].nSensitivity = 60;
    stInfo.astRule[0].dwDetectionTargetCount = 1;
    stInfo.astRule[0].adwDetectionTarget[0] = NET_TV_TARGET_HUMAN;

    printf("[Client] 调用 NET_TV_SetDevConfig 设置越界检测配置...\n");
    INT32 dwBytesReturnedSet = 0;
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_CROSSLINEALARM,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturnedSet
    );

    if (bRet)
    {
        printf("[Client] 设置越界检测配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetCrossLineAlarm();
    }
    else
    {
        printf("[Client] 设置越界检测配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

/* 获取入侵检测配置 */
static void DoGetIntrusionAlarm()
{
    NET_TV_INTRUSION_ALARM_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;

    printf("[Client] 调用 NET_TV_GetDevConfig 获取入侵检测配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_INTRUSIONALARM,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 获取入侵检测配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintIntrusionAlarmInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取入侵检测配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

/* 设置入侵检测配置 */
static void DoSetIntrusionAlarm()
{
    NET_TV_INTRUSION_ALARM_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    BOOL bRetGet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_INTRUSIONALARM,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (!bRetGet)
    {
        printf("[Client] 预获取入侵检测配置失败，使用默认值. Error=%d\n", NET_TV_GetLastError());
        memset(&stInfo, 0, sizeof(stInfo));
    }

    /* 修改为示例值 */
    stInfo.bEnable = TRUE;
    stInfo.dwRuleCount = 1;
    stInfo.astRule[0].bEnable = TRUE;
    stInfo.astRule[0].dwPointCount = 4;
    stInfo.astRule[0].afPointX[0] = 0.3f; stInfo.astRule[0].afPointY[0] = 0.3f;
    stInfo.astRule[0].afPointX[1] = 0.7f; stInfo.astRule[0].afPointY[1] = 0.3f;
    stInfo.astRule[0].afPointX[2] = 0.7f; stInfo.astRule[0].afPointY[2] = 0.7f;
    stInfo.astRule[0].afPointX[3] = 0.3f; stInfo.astRule[0].afPointY[3] = 0.7f;
    stInfo.astRule[0].nTimeThreshold = 15;
    stInfo.astRule[0].nSensitivity = 60;
    stInfo.astRule[0].dwDetectionTargetCount = 1;
    stInfo.astRule[0].adwDetectionTarget[0] = NET_TV_TARGET_HUMAN;

    printf("[Client] 调用 NET_TV_SetDevConfig 设置入侵检测配置...\n");
    INT32 dwBytesReturnedSet = 0;
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_INTRUSIONALARM,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturnedSet
    );

    if (bRet)
    {
        printf("[Client] 设置入侵检测配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetIntrusionAlarm();
    }
    else
    {
        printf("[Client] 设置入侵检测配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

/* 获取进入区域侦测配置 */
static void DoGetEnterRegionAlarm()
{
    NET_TV_ENTER_REGION_ALARM_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_ENTERREGIONALARM,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 获取进入区域侦测配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintEnterRegionAlarmInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取进入区域侦测配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

/* 设置进入区域侦测配置 */
static void DoSetEnterRegionAlarm()
{
    NET_TV_ENTER_REGION_ALARM_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    BOOL bRetGet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_ENTERREGIONALARM,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (!bRetGet)
    {
        memset(&stInfo, 0, sizeof(stInfo));
    }

    stInfo.bEnable = TRUE;
    stInfo.dwRuleCount = 1;
    stInfo.astRule[0].bEnable = TRUE;
    stInfo.astRule[0].dwPointCount = 4;
    FillDemoPolygon4(stInfo.astRule[0].afPointX, stInfo.astRule[0].afPointY);
    stInfo.astRule[0].nTimeThreshold = 12;
    stInfo.astRule[0].nSensitivity = 58;
    stInfo.astRule[0].dwDetectionTargetCount = 1;
    stInfo.astRule[0].adwDetectionTarget[0] = NET_TV_TARGET_HUMAN;

    INT32 dwBytesReturnedSet = 0;
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_ENTERREGIONALARM,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturnedSet
    );

    if (bRet)
    {
        printf("[Client] 设置进入区域侦测配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetEnterRegionAlarm();
    }
    else
    {
        printf("[Client] 设置进入区域侦测配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

/* 获取离开区域侦测配置 */
static void DoGetLeaveRegionAlarm()
{
    NET_TV_LEAVE_REGION_ALARM_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_LEAVEREGIONALARM,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 获取离开区域侦测配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintLeaveRegionAlarmInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取离开区域侦测配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

/* 设置离开区域侦测配置 */
static void DoSetLeaveRegionAlarm()
{
    NET_TV_LEAVE_REGION_ALARM_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    BOOL bRetGet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_LEAVEREGIONALARM,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (!bRetGet)
    {
        memset(&stInfo, 0, sizeof(stInfo));
    }

    stInfo.bEnable = TRUE;
    stInfo.dwRuleCount = 1;
    stInfo.astRule[0].bEnable = TRUE;
    stInfo.astRule[0].dwPointCount = 4;
    FillDemoPolygon4(stInfo.astRule[0].afPointX, stInfo.astRule[0].afPointY);
    stInfo.astRule[0].nTimeThreshold = 12;
    stInfo.astRule[0].nSensitivity = 58;
    stInfo.astRule[0].dwDetectionTargetCount = 1;
    stInfo.astRule[0].adwDetectionTarget[0] = NET_TV_TARGET_HUMAN;

    INT32 dwBytesReturnedSet = 0;
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_LEAVEREGIONALARM,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturnedSet
    );

    if (bRet)
    {
        printf("[Client] 设置离开区域侦测配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetLeaveRegionAlarm();
    }
    else
    {
        printf("[Client] 设置离开区域侦测配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

/* 获取徘徊侦测配置 */
static void DoGetLoiteringAlarm()
{
    NET_TV_LOITERING_ALARM_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;

    printf("[Client] 调用 NET_TV_GetDevConfig 获取徘徊侦测配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_LOITERINGALARM,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 获取徘徊侦测配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintLoiteringAlarmInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取徘徊侦测配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

/* 设置徘徊侦测配置 */
static void DoSetLoiteringAlarm()
{
    NET_TV_LOITERING_ALARM_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    BOOL bRetGet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_SET_LOITERINGALARM,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (!bRetGet)
    {
        printf("[Client] 预获取徘徊侦测配置失败，使用默认值. Error=%d\n", NET_TV_GetLastError());
        memset(&stInfo, 0, sizeof(stInfo));
    }

    /* 修改为示例值 */
    stInfo.bEnable = TRUE;
    stInfo.dwRuleCount = 1;
    stInfo.astRule[0].bEnable = TRUE;
    stInfo.astRule[0].dwPointCount = 4;
    stInfo.astRule[0].afPointX[0] = 0.3f; stInfo.astRule[0].afPointY[0] = 0.3f;
    stInfo.astRule[0].afPointX[1] = 0.7f; stInfo.astRule[0].afPointY[1] = 0.3f;
    stInfo.astRule[0].afPointX[2] = 0.7f; stInfo.astRule[0].afPointY[2] = 0.7f;
    stInfo.astRule[0].afPointX[3] = 0.3f; stInfo.astRule[0].afPointY[3] = 0.7f;
    stInfo.astRule[0].nTimeThreshold = 15;
    stInfo.astRule[0].nSensitivity = 60;
    stInfo.astRule[0].dwDetectionTargetCount = 1;
    stInfo.astRule[0].adwDetectionTarget[0] = NET_TV_TARGET_HUMAN;

    printf("[Client] 调用 NET_TV_SetDevConfig 设置徘徊侦测配置...\n");
    INT32 dwBytesReturnedSet = 0;
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_LOITERINGALARM,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturnedSet
    );

    if (bRet)
    {
        printf("[Client] 设置徘徊侦测配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetLoiteringAlarm();
    }
    else
    {
        printf("[Client] 设置徘徊侦测配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}


static void PrintSceneChangeAlarmInfo(const NET_TV_SCENE_CHANGE_ALARM_INFO_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 打印场景变更侦测配置 =====\n");
    printf("  Enable      : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  Sensitivity : %d\n", pInfo->nSensitivity);
    printf("=================================\n");
}

static void DoGetSceneChangeAlarm()
{
    NET_TV_SCENE_CHANGE_ALARM_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;

    printf("[Client] 调用 NET_TV_GetDevConfig 获取场景变更侦测配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_SCENECHANGEALARM,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 获取场景变更侦测配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintSceneChangeAlarmInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取场景变更侦测配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoSetSceneChangeAlarm()
{
    NET_TV_SCENE_CHANGE_ALARM_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    BOOL bRetGet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_SCENECHANGEALARM,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (!bRetGet)
    {
        printf("[Client] 预获取场景变更侦测配置失败，使用默认值. Error=%d\n", NET_TV_GetLastError());
        memset(&stInfo, 0, sizeof(stInfo));
    }

    stInfo.bEnable = TRUE;
    stInfo.nSensitivity = 60;

    printf("[Client] 调用 NET_TV_SetDevConfig 设置场景变更侦测配置...\n");
    INT32 dwBytesReturnedSet = 0;
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_SCENECHANGEALARM,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturnedSet
    );

    if (bRet)
    {
        printf("[Client] 设置场景变更侦测配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetSceneChangeAlarm();
    }
    else
    {
        printf("[Client] 设置场景变更侦测配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void PrintCrowdGatheringAlarmInfo(const NET_TV_CROWD_GATHERING_ALARM_INFO_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 打印人员聚集配置 =====\n");
    printf("  Enable    : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  RuleCount : %d\n", pInfo->dwRuleCount);
    for (int i = 0; i < pInfo->dwRuleCount && i < 4; i++)
    {
        printf("  Rule[%d]   : PointCount=%d, ObjectOccup=%d\n",
               i,
               pInfo->astRule[i].dwPointCount,
               pInfo->astRule[i].nObjectOccup);
    }
    printf("=================================\n");
}

static void DoGetCrowdGatheringAlarm()
{
    NET_TV_CROWD_GATHERING_ALARM_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;

    printf("[Client] 调用 NET_TV_GetDevConfig 获取人员聚集配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_CROWDGATHERINGALARM,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 获取人员聚集配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintCrowdGatheringAlarmInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取人员配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoSetCrowdGatheringAlarm()
{
    NET_TV_CROWD_GATHERING_ALARM_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    BOOL bRetGet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_CROWDGATHERINGALARM,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (!bRetGet)
    {
        printf("[Client] 预获取人员聚集配置失败，使用默认值. Error=%d\n", NET_TV_GetLastError());
        memset(&stInfo, 0, sizeof(stInfo));
    }

    stInfo.bEnable = TRUE;
    stInfo.dwRuleCount = 1;
    stInfo.astRule[0].dwPointCount = 4;
    FillDemoPolygon4(stInfo.astRule[0].afPointX, stInfo.astRule[0].afPointY);
    stInfo.astRule[0].nObjectOccup = 55;

    printf("[Client] 调用 NET_TV_SetDevConfig 设置人员聚集配置...\n");
    INT32 dwBytesReturnedSet = 0;
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_CROWDGATHERINGALARM,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturnedSet
    );

    if (bRet)
    {
        printf("[Client] 设置人员聚集配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetCrowdGatheringAlarm();
    }
    else
    {
        printf("[Client] 设置人员聚集配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void PrintGarbageExposureCfgInfo(const NET_TV_GARBAGE_EXPOSURE_CFG_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 打印垃圾暴露配置 =====\n");
    printf("  Enable    : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  Sensitivity: %d\n", pInfo->stRule.nSensitivity);
    printf("  PointCount: %d\n", pInfo->stRule.dwPointCount);
    printf("=================================\n");
}

static void DoGetGarbageExposureCfg()
{
    NET_TV_GARBAGE_EXPOSURE_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;

    printf("[Client] 调用 NET_TV_GetDevConfig 获取垃圾暴露配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_GARBAGE_EXPOSURE_CFG,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 获取垃圾暴露配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintGarbageExposureCfgInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取垃圾暴露配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoSetGarbageExposureCfg()
{
    NET_TV_GARBAGE_EXPOSURE_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    BOOL bRetGet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_GARBAGE_EXPOSURE_CFG,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (!bRetGet)
    {
        printf("[Client] 预获取垃圾暴露配置失败，使用默认值. Error=%d\n", NET_TV_GetLastError());
        memset(&stInfo, 0, sizeof(stInfo));
    }

    stInfo.bEnable = TRUE;
    stInfo.stRule.nSensitivity = 50;
    stInfo.stRule.dwPointCount = 4;
    FillDemoPolygon4(stInfo.stRule.afPointX, stInfo.stRule.afPointY);

    printf("[Client] 调用 NET_TV_SetDevConfig 设置垃圾暴露配置...\n");
    INT32 dwBytesReturnedSet = 0;
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_GARBAGE_EXPOSURE_CFG,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturnedSet
    );

    if (bRet)
    {
        printf("[Client] 设置垃圾暴露配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetGarbageExposureCfg();
    }
    else
    {
        printf("[Client] 设置垃圾暴露配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void PrintGarbageOverflowCfgInfo(const NET_TV_GARBAGE_OVERFLOW_CFG_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 打印垃圾满溢配置 =====\n");
    printf("  Enable    : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  Sensitivity: %d\n", pInfo->stRule.nSensitivity);
    printf("  PointCount: %d\n", pInfo->stRule.dwPointCount);
    printf("  TimeThreshold: %d\n", pInfo->nTimeThreshold);
    printf("=================================\n");
}

static void DoGetGarbageOverflowCfg()
{
    NET_TV_GARBAGE_OVERFLOW_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;

    printf("[Client] 调用 NET_TV_GetDevConfig 获取垃圾满溢配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_GARBAGE_OVERFLOW_CFG,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 获取垃圾满溢配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintGarbageOverflowCfgInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取垃圾满溢配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoSetGarbageOverflowCfg()
{
    NET_TV_GARBAGE_OVERFLOW_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    BOOL bRetGet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_GARBAGE_OVERFLOW_CFG,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (!bRetGet)
    {
        printf("[Client] 预获取垃圾满溢配置失败，使用默认值. Error=%d\n", NET_TV_GetLastError());
        memset(&stInfo, 0, sizeof(stInfo));
    }

    stInfo.bEnable = TRUE;
    stInfo.stRule.nSensitivity = 50;
    stInfo.stRule.dwPointCount = 4;
    FillDemoPolygon4(stInfo.stRule.afPointX, stInfo.stRule.afPointY);
    stInfo.nTimeThreshold = 60;

    printf("[Client] 调用 NET_TV_SetDevConfig 设置垃圾满溢配置...\n");
    INT32 dwBytesReturnedSet = 0;
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_GARBAGE_OVERFLOW_CFG,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturnedSet
    );

    if (bRet)
    {
        printf("[Client] 设置垃圾满溢配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetGarbageOverflowCfg();
    }
    else
    {
        printf("[Client] 设置垃圾满溢配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void PrintPeopleFlowStatisticsCfgInfo(const NET_TV_PEOPLE_FLOW_STATISTICS_CFG_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 打印人流统计配置 =====\n");
    printf("  Enable         : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  Sensitivity    : %d\n", pInfo->nSensitivity);
    printf("  StatisticsType : %d\n", pInfo->enStatisticsType);
    printf("  ReportInterval: %d\n", pInfo->nReportInterval);
    printf("  TimedReset    : %s, Hour=%d, Minute=%d\n",
           pInfo->stTimedReset.bEnable ? "ON" : "OFF",
           pInfo->stTimedReset.nHour,
           pInfo->stTimedReset.nMinute);
    printf("=================================\n");
}

static void DoGetPeopleFlowStatisticsCfg()
{
    NET_TV_PEOPLE_FLOW_STATISTICS_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;

    printf("[Client] 调用 NET_TV_GetDevConfig 获取人流统计配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_PEOPLE_FLOW_STATISTICS_CFG,
        &stInfo, sizeof(stInfo), &dwBytesReturned);

    if (bRet)
    {
        printf("[Client] 获取人流统计配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintPeopleFlowStatisticsCfgInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取人流统计配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoSetPeopleFlowStatisticsCfg()
{
    NET_TV_PEOPLE_FLOW_STATISTICS_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    stInfo.bEnable = TRUE;
    stInfo.nSensitivity = 60;
    stInfo.nReportInterval = 120;
    stInfo.enStatisticsType = NET_TV_PEOPLE_FLOW_STAT_TOTAL;

    INT32 dwBytesReturnedSet = 0;

    printf("[Client] 调用 NET_TV_SetDevConfig 设置人流统计配置...\n");
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_PEOPLE_FLOW_STATISTICS_CFG,
        &stInfo, sizeof(stInfo), &dwBytesReturnedSet);

    if (bRet)
    {
        printf("[Client] 设置人流统计配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetPeopleFlowStatisticsCfg();
    }
    else
    {
        printf("[Client] 设置人流统计配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoResetPeopleFlowStatistics()
{
    NET_TV_PEOPLE_FLOW_STATISTICS_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturnedSet = 0;

    printf("[Client] 调用 NET_TV_SetDevConfig 清零人流统计...\n");
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_RESET_PEOPLE_FLOW_STATISTICS,
        &stInfo, sizeof(stInfo), &dwBytesReturnedSet);

    if (bRet)
    {
        printf("[Client] 清零人流统计成功! BytesReturned=%d\n", dwBytesReturnedSet);
    }
    else
    {
        printf("[Client] 清零人流统计失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void PrintPeopleDensityDetectionCfgInfo(const NET_TV_PEOPLE_DENSITY_DETECTION_CFG_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 打印人员密度检测配置 =====\n");
    printf("  Enable         : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  Sensitivity    : %d\n", pInfo->nSensitivity);
    printf("  ReportInterval : %d\n", pInfo->nReportInterval);
    printf("=================================\n");
}

static void DoGetPeopleDensityDetectionCfg()
{
    NET_TV_PEOPLE_DENSITY_DETECTION_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;

    printf("[Client] 调用 NET_TV_GetDevConfig 获取人员密度检测配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_PEOPLE_DENSITY_DETECTION_CFG,
        &stInfo, sizeof(stInfo), &dwBytesReturned);

    if (bRet)
    {
        printf("[Client] 获取人员密度检测配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintPeopleDensityDetectionCfgInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取人员密度检测配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoSetPeopleDensityDetectionCfg()
{
    NET_TV_PEOPLE_DENSITY_DETECTION_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    stInfo.bEnable = TRUE;
    stInfo.nSensitivity = 50;
    stInfo.nReportInterval = 60;

    INT32 dwBytesReturnedSet = 0;

    printf("[Client] 调用 NET_TV_SetDevConfig 设置人员密度检测配置...\n");
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_PEOPLE_DENSITY_DETECTION_CFG,
        &stInfo, sizeof(stInfo), &dwBytesReturnedSet);

    if (bRet)
    {
        printf("[Client] 设置人员密度检测配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetPeopleDensityDetectionCfg();
    }
    else
    {
        printf("[Client] 设置人员密度检测配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void FillSimpleAiDemoSchedule(NET_TV_ALARM_SCHEDULE_S* pSchedule)
{
    if (!pSchedule)
    {
        return;
    }

    for (int day = 0; day < 7; day++)
    {
        pSchedule->dwTimeSectionCount[day] = 1;
        pSchedule->astTimeSection[day][0].nStartHour = 0;
        pSchedule->astTimeSection[day][0].nStartMinute = 0;
        pSchedule->astTimeSection[day][0].nEndHour = 23;
        pSchedule->astTimeSection[day][0].nEndMinute = 59;
    }
}

static void FillSmartRegionDemo(NET_TV_SMART_REGION_S* pRegion)
{
    if (!pRegion)
    {
        return;
    }

    pRegion->dwPointCount = 4;
    pRegion->afPointX[0] = 0.20f; pRegion->afPointY[0] = 0.20f;
    pRegion->afPointX[1] = 0.80f; pRegion->afPointY[1] = 0.20f;
    pRegion->afPointX[2] = 0.80f; pRegion->afPointY[2] = 0.80f;
    pRegion->afPointX[3] = 0.20f; pRegion->afPointY[3] = 0.80f;
}

static void FillSmartRegionRuleDemo(NET_TV_SMART_REGION_RULE_S* pRule)
{
    if (!pRule)
    {
        return;
    }

    pRule->bEnable = TRUE;
    pRule->dwPointCount = 4;
    pRule->afPointX[0] = 0.20f; pRule->afPointY[0] = 0.20f;
    pRule->afPointX[1] = 0.80f; pRule->afPointY[1] = 0.20f;
    pRule->afPointX[2] = 0.80f; pRule->afPointY[2] = 0.80f;
    pRule->afPointX[3] = 0.20f; pRule->afPointY[3] = 0.80f;
    pRule->nTimeThreshold = 10;
    pRule->nSensitivity = 55;
    pRule->dwDetectionTargetCount = 1;
    pRule->adwDetectionTarget[0] = NET_TV_TARGET_HUMAN;
}

static void FillSmartLineRuleDemo(NET_TV_SMART_LINE_RULE_S* pRule)
{
    if (!pRule)
    {
        return;
    }

    pRule->bEnable = TRUE;
    pRule->fStartPosX = 0.20f;
    pRule->fStartPosY = 0.50f;
    pRule->fEndPosX = 0.80f;
    pRule->fEndPosY = 0.50f;
    pRule->enCrossDirection = NET_TV_CROSS_A_TO_B;
    pRule->nSensitivity = 55;
}

static void PrintManholeCoverAbnormalCfgInfo(const NET_TV_MANHOLE_COVER_ABNORMAL_CFG_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 打印井盖异常检测配置 =====\n");
    printf("  Enable      : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  Sensitivity : %d\n", pInfo->stRule.nSensitivity);
    printf("=================================\n");
}

static void DoGetManholeCoverAbnormalCfg()
{
    NET_TV_MANHOLE_COVER_ABNORMAL_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_GetDevConfig 获取井盖异常检测配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_MANHOLE_COVER_ABNORMAL_CFG,
        &stInfo, sizeof(stInfo), &dwBytesReturned);

    if (bRet)
    {
        printf("[Client] 获取井盖异常检测配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintManholeCoverAbnormalCfgInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取井盖异常检测配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoSetManholeCoverAbnormalCfg()
{
    NET_TV_MANHOLE_COVER_ABNORMAL_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    stInfo.bEnable = TRUE;
    stInfo.stRule.nSensitivity = 50;
    FillSimpleAiDemoSchedule(&stInfo.stAlarmSchedule);

    INT32 dwBytesReturnedSet = 0;
    printf("[Client] 调用 NET_TV_SetDevConfig 设置井盖异常检测配置...\n");
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_MANHOLE_COVER_ABNORMAL_CFG,
        &stInfo, sizeof(stInfo), &dwBytesReturnedSet);

    if (bRet)
    {
        printf("[Client] 设置井盖异常检测配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetManholeCoverAbnormalCfg();
    }
    else
    {
        printf("[Client] 设置井盖异常检测配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void PrintSleepOnDutyCfgInfo(const NET_TV_SLEEP_ON_DUTY_CFG_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 打印睡岗识别配置 =====\n");
    printf("  Enable      : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  Sensitivity : %d\n", pInfo->stRule.nSensitivity);
    printf("=================================\n");
}

static void DoGetSleepOnDutyCfg()
{
    NET_TV_SLEEP_ON_DUTY_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_GetDevConfig 获取睡岗识别配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_SLEEP_ON_DUTY_CFG,
        &stInfo, sizeof(stInfo), &dwBytesReturned);

    if (bRet)
    {
        printf("[Client] 获取睡岗识别配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintSleepOnDutyCfgInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取睡岗识别配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoSetSleepOnDutyCfg()
{
    NET_TV_SLEEP_ON_DUTY_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    stInfo.bEnable = TRUE;
    stInfo.stRule.nSensitivity = 50;
    FillSimpleAiDemoSchedule(&stInfo.stAlarmSchedule);

    INT32 dwBytesReturnedSet = 0;
    printf("[Client] 调用 NET_TV_SetDevConfig 设置睡岗识别配置...\n");
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_SLEEP_ON_DUTY_CFG,
        &stInfo, sizeof(stInfo), &dwBytesReturnedSet);

    if (bRet)
    {
        printf("[Client] 设置睡岗识别配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetSleepOnDutyCfg();
    }
    else
    {
        printf("[Client] 设置睡岗识别配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void PrintElectricVehicleInElevatorCfgInfo(const NET_TV_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 打印电瓶车进电梯识别配置 =====\n");
    printf("  Enable      : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  Sensitivity : %d\n", pInfo->stRule.nSensitivity);
    printf("=================================\n");
}

static void DoGetElectricVehicleInElevatorCfg()
{
    NET_TV_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_GetDevConfig 获取电瓶车进电梯识别配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG,
        &stInfo, sizeof(stInfo), &dwBytesReturned);

    if (bRet)
    {
        printf("[Client] 获取电瓶车进电梯识别配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintElectricVehicleInElevatorCfgInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取电瓶车进电梯识别配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoSetElectricVehicleInElevatorCfg()
{
    NET_TV_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    stInfo.bEnable = TRUE;
    stInfo.stRule.nSensitivity = 50;
    FillSimpleAiDemoSchedule(&stInfo.stAlarmSchedule);

    INT32 dwBytesReturnedSet = 0;
    printf("[Client] 调用 NET_TV_SetDevConfig 设置电瓶车进电梯识别配置...\n");
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG,
        &stInfo, sizeof(stInfo), &dwBytesReturnedSet);

    if (bRet)
    {
        printf("[Client] 设置电瓶车进电梯识别配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetElectricVehicleInElevatorCfg();
    }
    else
    {
        printf("[Client] 设置电瓶车进电梯识别配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void PrintPersonFallDownCfgInfo(const NET_TV_PERSON_FALL_DOWN_CFG_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 打印人员倒地识别配置 =====\n");
    printf("  Enable      : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  Sensitivity : %d\n", pInfo->stRule.nSensitivity);
    printf("=================================\n");
}

static void DoGetPersonFallDownCfg()
{
    NET_TV_PERSON_FALL_DOWN_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_GetDevConfig 获取人员倒地识别配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_PERSON_FALL_DOWN_CFG,
        &stInfo, sizeof(stInfo), &dwBytesReturned);

    if (bRet)
    {
        printf("[Client] 获取人员倒地识别配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintPersonFallDownCfgInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取人员倒地识别配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoSetPersonFallDownCfg()
{
    NET_TV_PERSON_FALL_DOWN_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    stInfo.bEnable = TRUE;
    stInfo.stRule.nSensitivity = 50;
    FillSimpleAiDemoSchedule(&stInfo.stAlarmSchedule);

    INT32 dwBytesReturnedSet = 0;
    printf("[Client] 调用 NET_TV_SetDevConfig 设置人员倒地识别配置...\n");
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_PERSON_FALL_DOWN_CFG,
        &stInfo, sizeof(stInfo), &dwBytesReturnedSet);

    if (bRet)
    {
        printf("[Client] 设置人员倒地识别配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetPersonFallDownCfg();
    }
    else
    {
        printf("[Client] 设置人员倒地识别配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void PrintConstructionOccupyRoadCfgInfo(const NET_TV_CONSTRUCTION_OCCUPY_ROAD_CFG_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 打印施工占道识别配置 =====\n");
    printf("  Enable      : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  Sensitivity : %d\n", pInfo->stRule.nSensitivity);
    printf("=================================\n");
}

static void DoGetConstructionOccupyRoadCfg()
{
    NET_TV_CONSTRUCTION_OCCUPY_ROAD_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_GetDevConfig 获取施工占道识别配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_CONSTRUCTION_OCCUPY_ROAD_CFG,
        &stInfo, sizeof(stInfo), &dwBytesReturned);

    if (bRet)
    {
        printf("[Client] 获取施工占道识别配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintConstructionOccupyRoadCfgInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取施工占道识别配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoSetConstructionOccupyRoadCfg()
{
    NET_TV_CONSTRUCTION_OCCUPY_ROAD_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    stInfo.bEnable = TRUE;
    stInfo.stRule.nSensitivity = 50;
    FillSimpleAiDemoSchedule(&stInfo.stAlarmSchedule);

    INT32 dwBytesReturnedSet = 0;
    printf("[Client] 调用 NET_TV_SetDevConfig 设置施工占道识别配置...\n");
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_CONSTRUCTION_OCCUPY_ROAD_CFG,
        &stInfo, sizeof(stInfo), &dwBytesReturnedSet);

    if (bRet)
    {
        printf("[Client] 设置施工占道识别配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetConstructionOccupyRoadCfg();
    }
    else
    {
        printf("[Client] 设置施工占道识别配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void PrintCongestionCfgInfo(const NET_TV_CONGESTION_CFG_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 打印拥堵识别配置 =====\n");
    printf("  Enable      : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  Sensitivity : %d\n", pInfo->stRule.nSensitivity);
    printf("=================================\n");
}

static void DoGetCongestionCfg()
{
    NET_TV_CONGESTION_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_GetDevConfig 获取拥堵识别配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_CONGESTION_CFG,
        &stInfo, sizeof(stInfo), &dwBytesReturned);

    if (bRet)
    {
        printf("[Client] 获取拥堵识别配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintCongestionCfgInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取拥堵识别配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoSetCongestionCfg()
{
    NET_TV_CONGESTION_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    stInfo.bEnable = TRUE;
    stInfo.stRule.nSensitivity = 50;
    FillSimpleAiDemoSchedule(&stInfo.stAlarmSchedule);

    INT32 dwBytesReturnedSet = 0;
    printf("[Client] 调用 NET_TV_SetDevConfig 设置拥堵识别配置...\n");
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_CONGESTION_CFG,
        &stInfo, sizeof(stInfo), &dwBytesReturnedSet);

    if (bRet)
    {
        printf("[Client] 设置拥堵识别配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetCongestionCfg();
    }
    else
    {
        printf("[Client] 设置拥堵识别配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void PrintLicensePlateRecognitionCfgInfo(const NET_TV_LICENSE_PLATE_RECOGNITION_CFG_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 打印车牌识别配置 =====\n");
    printf("  Enable      : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  Sensitivity : %d\n", pInfo->stRule.nSensitivity);
    printf("=================================\n");
}

static void DoGetLicensePlateRecognitionCfg()
{
    NET_TV_LICENSE_PLATE_RECOGNITION_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_GetDevConfig 获取车牌识别配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_LICENSE_PLATE_RECOGNITION_CFG,
        &stInfo, sizeof(stInfo), &dwBytesReturned);

    if (bRet)
    {
        printf("[Client] 获取车牌识别配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintLicensePlateRecognitionCfgInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取车牌识别配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoSetLicensePlateRecognitionCfg()
{
    NET_TV_LICENSE_PLATE_RECOGNITION_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    stInfo.bEnable = TRUE;
    stInfo.stRule.nSensitivity = 50;
    FillSimpleAiDemoSchedule(&stInfo.stAlarmSchedule);

    INT32 dwBytesReturnedSet = 0;
    printf("[Client] 调用 NET_TV_SetDevConfig 设置车牌识别配置...\n");
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_LICENSE_PLATE_RECOGNITION_CFG,
        &stInfo, sizeof(stInfo), &dwBytesReturnedSet);

    if (bRet)
    {
        printf("[Client] 设置车牌识别配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetLicensePlateRecognitionCfg();
    }
    else
    {
        printf("[Client] 设置车牌识别配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void PrintHighAltitudeSeatbeltCfgInfo(const NET_TV_HIGH_ALTITUDE_SEATBELT_CFG_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 打印高空安全带识别配置 =====\n");
    printf("  Enable      : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  Sensitivity : %d\n", pInfo->stRule.nSensitivity);
    printf("=================================\n");
}

static void DoGetHighAltitudeSeatbeltCfg()
{
    NET_TV_HIGH_ALTITUDE_SEATBELT_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_GetDevConfig 获取高空安全带识别配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_HIGH_ALTITUDE_SEATBELT_CFG,
        &stInfo, sizeof(stInfo), &dwBytesReturned);

    if (bRet)
    {
        printf("[Client] 获取高空安全带识别配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintHighAltitudeSeatbeltCfgInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取高空安全带识别配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoSetHighAltitudeSeatbeltCfg()
{
    NET_TV_HIGH_ALTITUDE_SEATBELT_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    stInfo.bEnable = TRUE;
    stInfo.stRule.nSensitivity = 50;
    FillSimpleAiDemoSchedule(&stInfo.stAlarmSchedule);

    INT32 dwBytesReturnedSet = 0;
    printf("[Client] 调用 NET_TV_SetDevConfig 设置高空安全带识别配置...\n");
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_HIGH_ALTITUDE_SEATBELT_CFG,
        &stInfo, sizeof(stInfo), &dwBytesReturnedSet);

    if (bRet)
    {
        printf("[Client] 设置高空安全带识别配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetHighAltitudeSeatbeltCfg();
    }
    else
    {
        printf("[Client] 设置高空安全带识别配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void PrintSafetyHelmetCfgInfo(const NET_TV_SAFETY_HELMET_CFG_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 打印安全帽识别配置 =====\n");
    printf("  Enable      : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  Sensitivity : %d\n", pInfo->stRule.nSensitivity);
    printf("=================================\n");
}

static void DoGetSafetyHelmetCfg()
{
    NET_TV_SAFETY_HELMET_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_GetDevConfig 获取安全帽识别配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_SAFETY_HELMET_CFG,
        &stInfo, sizeof(stInfo), &dwBytesReturned);

    if (bRet)
    {
        printf("[Client] 获取安全帽识别配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintSafetyHelmetCfgInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取安全帽识别配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoSetSafetyHelmetCfg()
{
    NET_TV_SAFETY_HELMET_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    stInfo.bEnable = TRUE;
    stInfo.stRule.nSensitivity = 50;
    FillSimpleAiDemoSchedule(&stInfo.stAlarmSchedule);

    INT32 dwBytesReturnedSet = 0;
    printf("[Client] 调用 NET_TV_SetDevConfig 设置安全帽识别配置...\n");
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_SAFETY_HELMET_CFG,
        &stInfo, sizeof(stInfo), &dwBytesReturnedSet);

    if (bRet)
    {
        printf("[Client] 设置安全帽识别配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetSafetyHelmetCfg();
    }
    else
    {
        printf("[Client] 设置安全帽识别配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void PrintPersonFallCfgInfo(const NET_TV_PERSON_FALL_CFG_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 打印摔倒识别配置 =====\n");
    printf("  Enable      : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  Sensitivity : %d\n", pInfo->stRule.nSensitivity);
    printf("=================================\n");
}

static void DoGetPersonFallCfg()
{
    NET_TV_PERSON_FALL_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_GetDevConfig 获取摔倒识别配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_PERSON_FALL_CFG,
        &stInfo, sizeof(stInfo), &dwBytesReturned);

    if (bRet)
    {
        printf("[Client] 获取摔倒识别配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintPersonFallCfgInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取摔倒识别配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoSetPersonFallCfg()
{
    NET_TV_PERSON_FALL_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    stInfo.bEnable = TRUE;
    stInfo.stRule.nSensitivity = 50;
    FillSimpleAiDemoSchedule(&stInfo.stAlarmSchedule);

    INT32 dwBytesReturnedSet = 0;
    printf("[Client] 调用 NET_TV_SetDevConfig 设置摔倒识别配置...\n");
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_PERSON_FALL_CFG,
        &stInfo, sizeof(stInfo), &dwBytesReturnedSet);

    if (bRet)
    {
        printf("[Client] 设置摔倒识别配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetPersonFallCfg();
    }
    else
    {
        printf("[Client] 设置摔倒识别配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void PrintPhoneUsageCfgInfo(const NET_TV_PHONE_USAGE_CFG_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 打印玩手机识别配置 =====\n");
    printf("  Enable      : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  Sensitivity : %d\n", pInfo->stRule.nSensitivity);
    printf("=================================\n");
}

static void DoGetPhoneUsageCfg()
{
    NET_TV_PHONE_USAGE_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_GetDevConfig 获取玩手机识别配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_PHONE_USAGE_CFG,
        &stInfo, sizeof(stInfo), &dwBytesReturned);

    if (bRet)
    {
        printf("[Client] 获取玩手机识别配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintPhoneUsageCfgInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取玩手机识别配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoSetPhoneUsageCfg()
{
    NET_TV_PHONE_USAGE_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    stInfo.bEnable = TRUE;
    stInfo.stRule.nSensitivity = 50;
    FillSimpleAiDemoSchedule(&stInfo.stAlarmSchedule);

    INT32 dwBytesReturnedSet = 0;
    printf("[Client] 调用 NET_TV_SetDevConfig 设置玩手机识别配置...\n");
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_PHONE_USAGE_CFG,
        &stInfo, sizeof(stInfo), &dwBytesReturnedSet);

    if (bRet)
    {
        printf("[Client] 设置玩手机识别配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetPhoneUsageCfg();
    }
    else
    {
        printf("[Client] 设置玩手机识别配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void PrintSmokingCfgInfo(const NET_TV_SMOKING_CFG_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 打印抽烟识别配置 =====\n");
    printf("  Enable      : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  Sensitivity : %d\n", pInfo->stRule.nSensitivity);
    printf("=================================\n");
}

static void DoGetSmokingCfg()
{
    NET_TV_SMOKING_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_GetDevConfig 获取抽烟识别配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_SMOKING_CFG,
        &stInfo, sizeof(stInfo), &dwBytesReturned);

    if (bRet)
    {
        printf("[Client] 获取抽烟识别配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintSmokingCfgInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取抽烟识别配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoSetSmokingCfg()
{
    NET_TV_SMOKING_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    stInfo.bEnable = TRUE;
    stInfo.stRule.nSensitivity = 50;
    FillSimpleAiDemoSchedule(&stInfo.stAlarmSchedule);

    INT32 dwBytesReturnedSet = 0;
    printf("[Client] 调用 NET_TV_SetDevConfig 设置抽烟识别配置...\n");
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_SMOKING_CFG,
        &stInfo, sizeof(stInfo), &dwBytesReturnedSet);

    if (bRet)
    {
        printf("[Client] 设置抽烟识别配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetSmokingCfg();
    }
    else
    {
        printf("[Client] 设置抽烟识别配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void PrintOpenFlameCfgInfo(const NET_TV_OPEN_FLAME_CFG_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 打印明火识别配置 =====\n");
    printf("  Enable      : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  Sensitivity : %d\n", pInfo->stRule.nSensitivity);
    printf("=================================\n");
}

static void DoGetOpenFlameCfg()
{
    NET_TV_OPEN_FLAME_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_GetDevConfig 获取明火识别配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_OPEN_FLAME_CFG,
        &stInfo, sizeof(stInfo), &dwBytesReturned);

    if (bRet)
    {
        printf("[Client] 获取明火识别配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintOpenFlameCfgInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取明火识别配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoSetOpenFlameCfg()
{
    NET_TV_OPEN_FLAME_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    stInfo.bEnable = TRUE;
    stInfo.stRule.nSensitivity = 50;
    FillSimpleAiDemoSchedule(&stInfo.stAlarmSchedule);

    INT32 dwBytesReturnedSet = 0;
    printf("[Client] 调用 NET_TV_SetDevConfig 设置明火识别配置...\n");
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_OPEN_FLAME_CFG,
        &stInfo, sizeof(stInfo), &dwBytesReturnedSet);

    if (bRet)
    {
        printf("[Client] 设置明火识别配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetOpenFlameCfg();
    }
    else
    {
        printf("[Client] 设置明火识别配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void PrintBareSoilCfgInfo(const NET_TV_BARE_SOIL_CFG_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 打印黄土裸露识别配置 =====\n");
    printf("  Enable      : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  Sensitivity : %d\n", pInfo->stRule.nSensitivity);
    printf("=================================\n");
}

static void DoGetBareSoilCfg()
{
    NET_TV_BARE_SOIL_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_GetDevConfig 获取黄土裸露识别配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_BARE_SOIL_CFG,
        &stInfo, sizeof(stInfo), &dwBytesReturned);

    if (bRet)
    {
        printf("[Client] 获取黄土裸露识别配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintBareSoilCfgInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取黄土裸露识别配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoSetBareSoilCfg()
{
    NET_TV_BARE_SOIL_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    stInfo.bEnable = TRUE;
    stInfo.stRule.nSensitivity = 50;
    FillSimpleAiDemoSchedule(&stInfo.stAlarmSchedule);

    INT32 dwBytesReturnedSet = 0;
    printf("[Client] 调用 NET_TV_SetDevConfig 设置黄土裸露识别配置...\n");
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_BARE_SOIL_CFG,
        &stInfo, sizeof(stInfo), &dwBytesReturnedSet);

    if (bRet)
    {
        printf("[Client] 设置黄土裸露识别配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetBareSoilCfg();
    }
    else
    {
        printf("[Client] 设置黄土裸露识别配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void PrintHoleProtectionBarCfgInfo(const NET_TV_HOLE_PROTECTION_BAR_CFG_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 打印洞口防护栏识别配置 =====\n");
    printf("  Enable      : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  Sensitivity : %d\n", pInfo->stRule.nSensitivity);
    printf("=================================\n");
}

static void DoGetHoleProtectionBarCfg()
{
    NET_TV_HOLE_PROTECTION_BAR_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_GetDevConfig 获取洞口防护栏识别配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_HOLE_PROTECTION_BAR_CFG,
        &stInfo, sizeof(stInfo), &dwBytesReturned);

    if (bRet)
    {
        printf("[Client] 获取洞口防护栏识别配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintHoleProtectionBarCfgInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取洞口防护栏识别配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoSetHoleProtectionBarCfg()
{
    NET_TV_HOLE_PROTECTION_BAR_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    stInfo.bEnable = TRUE;
    stInfo.stRule.nSensitivity = 50;
    FillSimpleAiDemoSchedule(&stInfo.stAlarmSchedule);

    INT32 dwBytesReturnedSet = 0;
    printf("[Client] 调用 NET_TV_SetDevConfig 设置洞口防护栏识别配置...\n");
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_HOLE_PROTECTION_BAR_CFG,
        &stInfo, sizeof(stInfo), &dwBytesReturnedSet);

    if (bRet)
    {
        printf("[Client] 设置洞口防护栏识别配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetHoleProtectionBarCfg();
    }
    else
    {
        printf("[Client] 设置洞口防护栏识别配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void PrintReflectiveClothingCfgInfo(const NET_TV_REFLECTIVE_CLOTHING_CFG_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 打印反光衣识别配置 =====\n");
    printf("  Enable      : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  Sensitivity : %d\n", pInfo->stRule.nSensitivity);
    printf("=================================\n");
}

static void DoGetReflectiveClothingCfg()
{
    NET_TV_REFLECTIVE_CLOTHING_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_GetDevConfig 获取反光衣识别配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_REFLECTIVE_CLOTHING_CFG,
        &stInfo, sizeof(stInfo), &dwBytesReturned);

    if (bRet)
    {
        printf("[Client] 获取反光衣识别配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintReflectiveClothingCfgInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取反光衣识别配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoSetReflectiveClothingCfg()
{
    NET_TV_REFLECTIVE_CLOTHING_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    stInfo.bEnable = TRUE;
    stInfo.stRule.nSensitivity = 50;
    FillSimpleAiDemoSchedule(&stInfo.stAlarmSchedule);

    INT32 dwBytesReturnedSet = 0;
    printf("[Client] 调用 NET_TV_SetDevConfig 设置反光衣识别配置...\n");
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_REFLECTIVE_CLOTHING_CFG,
        &stInfo, sizeof(stInfo), &dwBytesReturnedSet);

    if (bRet)
    {
        printf("[Client] 设置反光衣识别配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetReflectiveClothingCfg();
    }
    else
    {
        printf("[Client] 设置反光衣识别配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void PrintPetRecognitionInfo(const NET_TV_PET_RECOGNITION_INFO_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 打印宠物识别配置 =====\n");
    printf("  Enable                : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  DynamicAnalysisEnable : %s\n", pInfo->bDynamicAnalysisEnable ? "ON" : "OFF");
    printf("  Sensitivity           : %d\n", pInfo->nSensitivity);
    printf("  RegionPointCount      : %d\n", pInfo->stRegion.dwPointCount);
    for (int i = 0; i < pInfo->stRegion.dwPointCount && i < 32; i++)
    {
        printf("  RegionPoint[%d]        : X=%.2f, Y=%.2f\n",
               i,
               pInfo->stRegion.afPointX[i],
               pInfo->stRegion.afPointY[i]);
    }
    printf("=================================\n");
}

static void DoGetPetRecognitionInfo()
{
    NET_TV_PET_RECOGNITION_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_GetDevConfig 获取宠物识别配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_PET_RECOGNITION_INFO,
        &stInfo, sizeof(stInfo), &dwBytesReturned);

    if (bRet)
    {
        printf("[Client] 获取宠物识别配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintPetRecognitionInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取宠物识别配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoSetPetRecognitionInfo()
{
    NET_TV_PET_RECOGNITION_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    stInfo.bEnable = TRUE;
    stInfo.bDynamicAnalysisEnable = TRUE;
    stInfo.nSensitivity = 55;
    FillSmartRegionDemo(&stInfo.stRegion);
    FillSimpleAiDemoSchedule(&stInfo.stAlarmSchedule);

    INT32 dwBytesReturnedSet = 0;
    printf("[Client] 调用 NET_TV_SetDevConfig 设置宠物识别配置...\n");
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_PET_RECOGNITION_INFO,
        &stInfo, sizeof(stInfo), &dwBytesReturnedSet);

    if (bRet)
    {
        printf("[Client] 设置宠物识别配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetPetRecognitionInfo();
    }
    else
    {
        printf("[Client] 设置宠物识别配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void PrintClimbFenceInfo(const NET_TV_CLIMB_FENCE_INFO_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 打印翻越围栏配置 =====\n");
    printf("  Enable    : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  RuleCount : %d\n", pInfo->dwRuleCount);
    for (int i = 0; i < pInfo->dwRuleCount && i < 4; i++)
    {
        printf("  Rule[%d]   : Enable=%s, PointCount=%d, Sensitivity=%d, TimeThreshold=%d\n",
               i,
               pInfo->astRule[i].bEnable ? "ON" : "OFF",
               pInfo->astRule[i].dwPointCount,
               pInfo->astRule[i].nSensitivity,
               pInfo->astRule[i].nTimeThreshold);
    }
    printf("=================================\n");
}

static void DoGetClimbFenceInfo()
{
    NET_TV_CLIMB_FENCE_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_GetDevConfig 获取翻越围栏配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_CLIMB_FENCE_INFO,
        &stInfo, sizeof(stInfo), &dwBytesReturned);

    if (bRet)
    {
        printf("[Client] 获取翻越围栏配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintClimbFenceInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取翻越围栏配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoSetClimbFenceInfo()
{
    NET_TV_CLIMB_FENCE_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    stInfo.bEnable = TRUE;
    stInfo.dwRuleCount = 1;
    FillSmartRegionRuleDemo(&stInfo.astRule[0]);
    FillSimpleAiDemoSchedule(&stInfo.stAlarmSchedule);

    INT32 dwBytesReturnedSet = 0;
    printf("[Client] 调用 NET_TV_SetDevConfig 设置翻越围栏配置...\n");
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_CLIMB_FENCE_INFO,
        &stInfo, sizeof(stInfo), &dwBytesReturnedSet);

    if (bRet)
    {
        printf("[Client] 设置翻越围栏配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetClimbFenceInfo();
    }
    else
    {
        printf("[Client] 设置翻越围栏配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void PrintDimissionInfo(const NET_TV_DIMISSION_INFO_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 打印离岗配置 =====\n");
    printf("  Enable    : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  RuleCount : %d\n", pInfo->dwRuleCount);
    for (int i = 0; i < pInfo->dwRuleCount && i < 4; i++)
    {
        printf("  Rule[%d]   : Enable=%s, PointCount=%d, Sensitivity=%d, TimeThreshold=%d\n",
               i,
               pInfo->astRule[i].bEnable ? "ON" : "OFF",
               pInfo->astRule[i].dwPointCount,
               pInfo->astRule[i].nSensitivity,
               pInfo->astRule[i].nTimeThreshold);
    }
    printf("=================================\n");
}

static void DoGetDimissionInfo()
{
    NET_TV_DIMISSION_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_GetDevConfig 获取离岗配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_DIMISSION_INFO,
        &stInfo, sizeof(stInfo), &dwBytesReturned);

    if (bRet)
    {
        printf("[Client] 获取离岗配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintDimissionInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取离岗配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoSetDimissionInfo()
{
    NET_TV_DIMISSION_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    stInfo.bEnable = TRUE;
    stInfo.dwRuleCount = 1;
    FillSmartRegionRuleDemo(&stInfo.astRule[0]);
    FillSimpleAiDemoSchedule(&stInfo.stAlarmSchedule);

    INT32 dwBytesReturnedSet = 0;
    printf("[Client] 调用 NET_TV_SetDevConfig 设置离岗配置...\n");
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_DIMISSION_INFO,
        &stInfo, sizeof(stInfo), &dwBytesReturnedSet);

    if (bRet)
    {
        printf("[Client] 设置离岗配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetDimissionInfo();
    }
    else
    {
        printf("[Client] 设置离岗配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void PrintIllegalLaneInfo(const NET_TV_ILLEGAL_LANE_INFO_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 打印违规变道配置 =====\n");
    printf("  Enable    : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  RuleCount : %d\n", pInfo->dwRuleCount);
    for (int i = 0; i < pInfo->dwRuleCount && i < 4; i++)
    {
        printf("  Rule[%d]   : Enable=%s, Start=(%.2f, %.2f), End=(%.2f, %.2f), Direction=%d, Sensitivity=%d\n",
               i,
               pInfo->astRule[i].bEnable ? "ON" : "OFF",
               pInfo->astRule[i].fStartPosX,
               pInfo->astRule[i].fStartPosY,
               pInfo->astRule[i].fEndPosX,
               pInfo->astRule[i].fEndPosY,
               pInfo->astRule[i].enCrossDirection,
               pInfo->astRule[i].nSensitivity);
    }
    printf("=================================\n");
}

static void DoGetIllegalLaneInfo()
{
    NET_TV_ILLEGAL_LANE_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_GetDevConfig 获取违规变道配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_ILLEGAL_LANE_INFO,
        &stInfo, sizeof(stInfo), &dwBytesReturned);

    if (bRet)
    {
        printf("[Client] 获取违规变道配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintIllegalLaneInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取违规变道配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoSetIllegalLaneInfo()
{
    NET_TV_ILLEGAL_LANE_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    stInfo.bEnable = TRUE;
    stInfo.dwRuleCount = 1;
    FillSmartLineRuleDemo(&stInfo.astRule[0]);
    FillSimpleAiDemoSchedule(&stInfo.stAlarmSchedule);

    INT32 dwBytesReturnedSet = 0;
    printf("[Client] 调用 NET_TV_SetDevConfig 设置违规变道配置...\n");
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_ILLEGAL_LANE_INFO,
        &stInfo, sizeof(stInfo), &dwBytesReturnedSet);

    if (bRet)
    {
        printf("[Client] 设置违规变道配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetIllegalLaneInfo();
    }
    else
    {
        printf("[Client] 设置违规变道配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void PrintRetrogradeInfo(const NET_TV_RETROGRADE_INFO_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 打印逆行配置 =====\n");
    printf("  Enable    : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  RuleCount : %d\n", pInfo->dwRuleCount);
    for (int i = 0; i < pInfo->dwRuleCount && i < 4; i++)
    {
        printf("  Rule[%d]   : Enable=%s, Start=(%.2f, %.2f), End=(%.2f, %.2f), Direction=%d, Sensitivity=%d\n",
               i,
               pInfo->astRule[i].bEnable ? "ON" : "OFF",
               pInfo->astRule[i].fStartPosX,
               pInfo->astRule[i].fStartPosY,
               pInfo->astRule[i].fEndPosX,
               pInfo->astRule[i].fEndPosY,
               pInfo->astRule[i].enCrossDirection,
               pInfo->astRule[i].nSensitivity);
    }
    printf("=================================\n");
}

static void DoGetRetrogradeInfo()
{
    NET_TV_RETROGRADE_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_GetDevConfig 获取逆行配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_RETROGRADE_INFO,
        &stInfo, sizeof(stInfo), &dwBytesReturned);

    if (bRet)
    {
        printf("[Client] 获取逆行配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintRetrogradeInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取逆行配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoSetRetrogradeInfo()
{
    NET_TV_RETROGRADE_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    stInfo.bEnable = TRUE;
    stInfo.dwRuleCount = 1;
    FillSmartLineRuleDemo(&stInfo.astRule[0]);
    FillSimpleAiDemoSchedule(&stInfo.stAlarmSchedule);

    INT32 dwBytesReturnedSet = 0;
    printf("[Client] 调用 NET_TV_SetDevConfig 设置逆行配置...\n");
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_RETROGRADE_INFO,
        &stInfo, sizeof(stInfo), &dwBytesReturnedSet);

    if (bRet)
    {
        printf("[Client] 设置逆行配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetRetrogradeInfo();
    }
    else
    {
        printf("[Client] 设置逆行配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void PrintNonmotorVehicleIntrusionInfo(const NET_TV_NONMOTOR_VEHICLE_INTRUSION_INFO_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 打印非机动车闯入配置 =====\n");
    printf("  Enable    : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  RuleCount : %d\n", pInfo->dwRuleCount);
    for (int i = 0; i < pInfo->dwRuleCount && i < 4; i++)
    {
        printf("  Rule[%d]   : Enable=%s, PointCount=%d, Sensitivity=%d, TimeThreshold=%d\n",
               i,
               pInfo->astRule[i].bEnable ? "ON" : "OFF",
               pInfo->astRule[i].dwPointCount,
               pInfo->astRule[i].nSensitivity,
               pInfo->astRule[i].nTimeThreshold);
    }
    printf("=================================\n");
}

static void DoGetNonmotorVehicleIntrusionInfo()
{
    NET_TV_NONMOTOR_VEHICLE_INTRUSION_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_GetDevConfig 获取非机动车闯入配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_NONMOTOR_VEHICLE_INTRUSION_INFO,
        &stInfo, sizeof(stInfo), &dwBytesReturned);

    if (bRet)
    {
        printf("[Client] 获取非机动车闯入配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintNonmotorVehicleIntrusionInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取非机动车闯入配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoSetNonmotorVehicleIntrusionInfo()
{
    NET_TV_NONMOTOR_VEHICLE_INTRUSION_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    stInfo.bEnable = TRUE;
    stInfo.dwRuleCount = 1;
    FillSmartRegionRuleDemo(&stInfo.astRule[0]);
    stInfo.astRule[0].adwDetectionTarget[0] = NET_TV_TARGET_VEHICLE;
    FillSimpleAiDemoSchedule(&stInfo.stAlarmSchedule);

    INT32 dwBytesReturnedSet = 0;
    printf("[Client] 调用 NET_TV_SetDevConfig 设置非机动车闯入配置...\n");
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_NONMOTOR_VEHICLE_INTRUSION_INFO,
        &stInfo, sizeof(stInfo), &dwBytesReturnedSet);

    if (bRet)
    {
        printf("[Client] 设置非机动车闯入配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetNonmotorVehicleIntrusionInfo();
    }
    else
    {
        printf("[Client] 设置非机动车闯入配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void PrintOccupationEmergencyInfo(const NET_TV_OCCUPATION_EMERGENCY_INFO_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 打印应急车道占用识别配置 =====\n");
    printf("  Enable    : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  RuleCount : %d\n", pInfo->dwRuleCount);
    for (int i = 0; i < pInfo->dwRuleCount && i < 4; i++)
    {
        printf("  Rule[%d]   : Enable=%s, PointCount=%d, Sensitivity=%d, TimeThreshold=%d\n",
               i,
               pInfo->astRule[i].bEnable ? "ON" : "OFF",
               pInfo->astRule[i].dwPointCount,
               pInfo->astRule[i].nSensitivity,
               pInfo->astRule[i].nTimeThreshold);
    }
    printf("=================================\n");
}

static void DoGetOccupationEmergencyInfo()
{
    NET_TV_OCCUPATION_EMERGENCY_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_GetDevConfig 获取应急车道占用识别配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_OCCUPATION_EMERGENCY_INFO,
        &stInfo, sizeof(stInfo), &dwBytesReturned);

    if (bRet)
    {
        printf("[Client] 获取应急车道占用识别配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintOccupationEmergencyInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取应急车道占用识别配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoSetOccupationEmergencyInfo()
{
    NET_TV_OCCUPATION_EMERGENCY_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    stInfo.bEnable = TRUE;
    stInfo.dwRuleCount = 1;
    FillSmartRegionRuleDemo(&stInfo.astRule[0]);
    FillSimpleAiDemoSchedule(&stInfo.stAlarmSchedule);

    INT32 dwBytesReturnedSet = 0;
    printf("[Client] 调用 NET_TV_SetDevConfig 设置应急车道占用识别配置...\n");
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_OCCUPATION_EMERGENCY_INFO,
        &stInfo, sizeof(stInfo), &dwBytesReturnedSet);

    if (bRet)
    {
        printf("[Client] 设置应急车道占用识别配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetOccupationEmergencyInfo();
    }
    else
    {
        printf("[Client] 设置应急车道占用识别配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void PrintPedestrianIntrusionInfo(const NET_TV_PEDESTRIAN_INTRUSION_INFO_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 打印行人闯入配置 =====\n");
    printf("  Enable    : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  RuleCount : %d\n", pInfo->dwRuleCount);
    for (int i = 0; i < pInfo->dwRuleCount && i < 4; i++)
    {
        printf("  Rule[%d]   : Enable=%s, PointCount=%d, Sensitivity=%d, TimeThreshold=%d\n",
               i,
               pInfo->astRule[i].bEnable ? "ON" : "OFF",
               pInfo->astRule[i].dwPointCount,
               pInfo->astRule[i].nSensitivity,
               pInfo->astRule[i].nTimeThreshold);
    }
    printf("=================================\n");
}

static void DoGetPedestrianIntrusionInfo()
{
    NET_TV_PEDESTRIAN_INTRUSION_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_GetDevConfig 获取行人闯入配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_PEDESTRIAN_INTRUSION_INFO,
        &stInfo, sizeof(stInfo), &dwBytesReturned);

    if (bRet)
    {
        printf("[Client] 获取行人闯入配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintPedestrianIntrusionInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取行人闯入配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoSetPedestrianIntrusionInfo()
{
    NET_TV_PEDESTRIAN_INTRUSION_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    stInfo.bEnable = TRUE;
    stInfo.dwRuleCount = 1;
    FillSmartRegionRuleDemo(&stInfo.astRule[0]);
    FillSimpleAiDemoSchedule(&stInfo.stAlarmSchedule);

    INT32 dwBytesReturnedSet = 0;
    printf("[Client] 调用 NET_TV_SetDevConfig 设置行人闯入配置...\n");
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_PEDESTRIAN_INTRUSION_INFO,
        &stInfo, sizeof(stInfo), &dwBytesReturnedSet);

    if (bRet)
    {
        printf("[Client] 设置行人闯入配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetPedestrianIntrusionInfo();
    }
    else
    {
        printf("[Client] 设置行人闯入配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void PrintSmokeFireCfgInfo(const NET_TV_SMOKE_FIRE_CFG_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 打印烟火识别配置 =====\n");
    printf("  Enable      : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  Sensitivity : %d\n", pInfo->stRule.nSensitivity);
    printf("=================================\n");
}

static void DoGetSmokeFireCfg()
{
    NET_TV_SMOKE_FIRE_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_GetDevConfig 获取烟火识别配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_SMOKE_FIRE_CFG,
        &stInfo, sizeof(stInfo), &dwBytesReturned);

    if (bRet)
    {
        printf("[Client] 获取烟火识别配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintSmokeFireCfgInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取烟火识别配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoSetSmokeFireCfg()
{
    NET_TV_SMOKE_FIRE_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    stInfo.bEnable = TRUE;
    stInfo.stRule.nSensitivity = 55;
    FillSimpleAiDemoSchedule(&stInfo.stAlarmSchedule);

    INT32 dwBytesReturnedSet = 0;
    printf("[Client] 调用 NET_TV_SetDevConfig 设置烟火识别配置...\n");
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_SMOKE_FIRE_CFG,
        &stInfo, sizeof(stInfo), &dwBytesReturnedSet);

    if (bRet)
    {
        printf("[Client] 设置烟火识别配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetSmokeFireCfg();
    }
    else
    {
        printf("[Client] 设置烟火识别配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void PrintRoadPondingCfgInfo(const NET_TV_ROAD_PONDING_CFG_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 打印道路积水检测配置 =====\n");
    printf("  Enable      : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  Sensitivity : %d\n", pInfo->stRule.nSensitivity);
    printf("=================================\n");
}

static void DoGetRoadPondingCfg()
{
    NET_TV_ROAD_PONDING_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_GetDevConfig 获取道路积水检测配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_ROAD_PONDING_CFG,
        &stInfo, sizeof(stInfo), &dwBytesReturned);

    if (bRet)
    {
        printf("[Client] 获取道路积水检测配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintRoadPondingCfgInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取道路积水检测配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoSetRoadPondingCfg()
{
    NET_TV_ROAD_PONDING_CFG_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    stInfo.bEnable = TRUE;
    stInfo.stRule.nSensitivity = 55;
    FillSimpleAiDemoSchedule(&stInfo.stAlarmSchedule);

    INT32 dwBytesReturnedSet = 0;
    printf("[Client] 调用 NET_TV_SetDevConfig 设置道路积水检测配置...\n");
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_ROAD_PONDING_CFG,
        &stInfo, sizeof(stInfo), &dwBytesReturnedSet);

    if (bRet)
    {
        printf("[Client] 设置道路积水检测配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetRoadPondingCfg();
    }
    else
    {
        printf("[Client] 设置道路积水检测配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void PrintParkingAlarmInfo(const NET_TV_PARKING_ALARM_INFO_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 打印停车侦测配置 =====\n");
    printf("  Enable    : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  RuleCount : %d\n", pInfo->dwRuleCount);
    for (int i = 0; i < pInfo->dwRuleCount && i < 8; i++)
    {
        printf("  Rule[%d]   : PointCount=%d, Sensitivity=%d, TimeThreshold=%d\n",
               i,
               pInfo->astRule[i].dwPointCount,
               pInfo->astRule[i].nSensitivity,
               pInfo->astRule[i].nTimeThreshold);
    }
    printf("=================================\n");
}

static void DoGetParkingAlarm()
{
    NET_TV_PARKING_ALARM_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;

    printf("[Client] 调用 NET_TV_GetDevConfig 获取停车侦测配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_PARKINGALARM,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 获取停车侦测配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintParkingAlarmInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取停车侦测配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoSetParkingAlarm()
{
    NET_TV_PARKING_ALARM_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    BOOL bRetGet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_PARKINGALARM,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (!bRetGet)
    {
        printf("[Client] 预获取停车配置失败，使用默认值. Error=%d\n", NET_TV_GetLastError());
        memset(&stInfo, 0, sizeof(stInfo));
    }

    stInfo.bEnable = TRUE;
    stInfo.dwRuleCount = 1;
    stInfo.astRule[0].dwPointCount = 4;
    FillDemoPolygon4(stInfo.astRule[0].afPointX, stInfo.astRule[0].afPointY);
    stInfo.astRule[0].nSensitivity = 60;
    stInfo.astRule[0].nTimeThreshold = 15;

    printf("[Client] 调用 NET_TV_SetDevConfig 设置停车侦测配置...\n");
    INT32 dwBytesReturnedSet = 0;
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_PARKINGALARM,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturnedSet
    );

    if (bRet)
    {
        printf("[Client] 设置停车侦测配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetParkingAlarm();
    }
    else
    {
        printf("[Client] 设置停车侦测配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void PrintUnattendedObjectAlarmInfo(const NET_TV_UNATTENDED_OBJECT_ALARM_INFO_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 打印物品遗留侦测配置 =====\n");
    printf("  Enable    : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  RuleCount : %d\n", pInfo->dwRuleCount);
    for (int i = 0; i < pInfo->dwRuleCount && i < 4; i++)
    {
        printf("  Rule[%d]   : PointCount=%d, Sensitivity=%d, TimeThreshold=%d\n",
               i,
               pInfo->astRule[i].dwPointCount,
               pInfo->astRule[i].nSensitivity,
               pInfo->astRule[i].nTimeThreshold);
    }
    printf("=================================\n");
}

static void DoGetUnattendedObjectAlarm()
{
    NET_TV_UNATTENDED_OBJECT_ALARM_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;

    printf("[Client] 调用 NET_TV_GetDevConfig 获取物品遗留侦测配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_UNATTENDEDOBJECTALARM,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 获取物品遗留侦测配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintUnattendedObjectAlarmInfo(&stInfo);
    }
    else
    {
        printf("[Client] 设置物品遗留侦测配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoSetUnattendedObjectAlarm()
{
    NET_TV_UNATTENDED_OBJECT_ALARM_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    BOOL bRetGet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_UNATTENDEDOBJECTALARM,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (!bRetGet)
    {
        printf("[Client] 预获取物品遗留侦测配置失败，使用默认值. Error=%d\n", NET_TV_GetLastError());
        memset(&stInfo, 0, sizeof(stInfo));
    }

    stInfo.bEnable = TRUE;
    stInfo.dwRuleCount = 1;
    stInfo.astRule[0].dwPointCount = 4;
    FillDemoPolygon4(stInfo.astRule[0].afPointX, stInfo.astRule[0].afPointY);
    stInfo.astRule[0].nSensitivity = 60;
    stInfo.astRule[0].nTimeThreshold = 30;

    printf("[Client] 调用 NET_TV_SetDevConfig 设置物品遗留侦测配置...\n");
    INT32 dwBytesReturnedSet = 0;
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_UNATTENDEDOBJECTALARM,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturnedSet
    );

    if (bRet)
    {
        printf("[Client] 设置物品遗留侦测配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetUnattendedObjectAlarm();
    }
    else
    {
        printf("[Client] 设置物品遗留侦测配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void PrintObjectRemovalAlarmInfo(const NET_TV_OBJECT_REMOVAL_ALARM_INFO_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 打印物品拿取侦测配置 =====\n");
    printf("  Enable    : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  RuleCount : %d\n", pInfo->dwRuleCount);
    for (int i = 0; i < pInfo->dwRuleCount && i < 4; i++)
    {
        printf("  Rule[%d]   : PointCount=%d, Sensitivity=%d, TimeThreshold=%d\n",
               i,
               pInfo->astRule[i].dwPointCount,
               pInfo->astRule[i].nSensitivity,
               pInfo->astRule[i].nTimeThreshold);
    }
    printf("=================================\n");
}

static void DoGetObjectRemovalAlarm()
{
    NET_TV_OBJECT_REMOVAL_ALARM_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;

    
    printf("[Client] 调用 NET_TV_GetDevConfig 获取物品拿去侦测配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_OBJECTREMOVALALARM,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 获取物品拿取侦测配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintObjectRemovalAlarmInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取物品拿取侦测配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoSetObjectRemovalAlarm()
{
    NET_TV_OBJECT_REMOVAL_ALARM_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    BOOL bRetGet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_OBJECTREMOVALALARM,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (!bRetGet)
    {
        printf("[Client] 预获取物品拿取侦测配置失败，使用默认值. Error=%d\n", NET_TV_GetLastError());
        memset(&stInfo, 0, sizeof(stInfo));
    }

    stInfo.bEnable = TRUE;
    stInfo.dwRuleCount = 1;
    stInfo.astRule[0].dwPointCount = 4;
    FillDemoPolygon4(stInfo.astRule[0].afPointX, stInfo.astRule[0].afPointY);
    stInfo.astRule[0].nSensitivity = 60;
    stInfo.astRule[0].nTimeThreshold = 30;

    INT32 dwBytesReturnedSet = 0;
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_OBJECTREMOVALALARM,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturnedSet
    );

    if (bRet)
    {
        printf("[Client] 设置物品拿取侦测配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetObjectRemovalAlarm();
    }
    else
    {
        printf("[Client] 设置物品拿取侦测配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

/* 打印音频异常侦测配置 */
static void PrintAudioAnomalyAlarmInfo(const NET_TV_AUDIO_ANOMALY_ALARM_INFO_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 音频异常侦测配置 =====\n");
    printf("  Enable            : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  AudioInputAnomaly : %s\n", pInfo->bAudioInputAnomaly ? "ON" : "OFF");
    printf("  UpEnable          : %s\n", pInfo->bUpEnable ? "ON" : "OFF");
    printf("  UpSensitivity     : %d\n", pInfo->nUpSensitivity);
    printf("  UpThreshold       : %d\n", pInfo->nUpThreshold);
    printf("  DownEnable        : %s\n", pInfo->bDownEnable ? "ON" : "OFF");
    printf("  DownSensitivity   : %d\n", pInfo->nDownSensitivity);
    printf("=================================\n");
}

/* 获取音频异常侦测配置 */
static void DoGetAudioAnomalyAlarm()
{
    NET_TV_AUDIO_ANOMALY_ALARM_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;

    printf("[Client] 调用 NET_TV_GetDevConfig 获取音频异常侦测配置...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_AUDIOANOMALYALARM,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 获取音频异常侦测配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintAudioAnomalyAlarmInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取音频异常侦测配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

/* 设置音频异常侦测配置 */
static void DoSetAudioAnomalyAlarm()
{
    NET_TV_AUDIO_ANOMALY_ALARM_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    BOOL bRetGet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_AUDIOANOMALYALARM,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (!bRetGet)
    {
        printf("[Client] 预获取音频异常侦测配置失败，使用默认值 Error=%d\n", NET_TV_GetLastError());
        memset(&stInfo, 0, sizeof(stInfo));
    }

    /* 修改为示例值*/
    stInfo.bEnable = TRUE;
    stInfo.bAudioInputAnomaly = TRUE;
    stInfo.bUpEnable = TRUE;
    stInfo.nUpSensitivity = 65;
    stInfo.nUpThreshold = 55;
    stInfo.bDownEnable = TRUE;
    stInfo.nDownSensitivity = 60;

    printf("[Client] 调用 NET_TV_SetDevConfig 设置音频异常侦测配置...\n");
    INT32 dwBytesReturnedSet = 0;
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_AUDIOANOMALYALARM,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturnedSet
    );

    if (bRet)
    {
        printf("[Client] 设置音频异常侦测配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetAudioAnomalyAlarm();
    }
    else
    {
        printf("[Client] 设置音频异常侦测配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

/* Print preview info */
static void PrintPreviewInfo(const NET_TV_PREVIEW_INFO_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== Preview Info =====\n");
    printf("  MainUrl     : %s\n", pInfo->stRtspUrl.szRtspMainUrl);
    printf("  SubUrl      : %s\n", pInfo->stRtspUrl.szRtspSubUrl);
    printf("  Brightness  : %d\n", pInfo->stImageParam.nBrightness);
    printf("  Contrast    : %d\n", pInfo->stImageParam.nContrast);
    printf("  Saturation  : %d\n", pInfo->stImageParam.nSaturation);
    printf("  Sharpness   : %d\n", pInfo->stImageParam.nSharpness);
    printf("=================================\n");
}

static void PrintChannelInfo(const NET_TV_CHANNEL_INFO_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== Channel Info =====\n");
    printf("  Channel       : %u\n", pInfo->dwChannel);
    printf("  Enable        : %u\n", pInfo->byEnable);
    printf("  Online        : %u\n", pInfo->byOnline);
    printf("  StreamType    : %u\n", pInfo->byStreamType);
    printf("  HasRecord     : %u\n", pInfo->byHasRecord);
    printf("  RecordStatus  : %d\n", pInfo->nRecordStatus);
    printf("  ChannelName   : %s\n", pInfo->szChannelName);
    printf("  RtspMainUrl   : %s\n", pInfo->szRtspMainUrl);
    printf("  RtspSubUrl    : %s\n", pInfo->szRtspSubUrl);
    printf("  PreviewMainUrl: %s\n", pInfo->szPreviewMainUrl);
    printf("  PreviewSubUrl : %s\n", pInfo->szPreviewSubUrl);
    printf("  DeviceIP      : %s\n", pInfo->szDeviceIP);
    printf("=================================\n");
}

static void PrintChannelList(const NET_TV_CHANNEL_LIST_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== Channel List =====\n");
    printf("  Size         : %u\n", pInfo->dwSize);
    printf("  ChannelCount : %u\n", pInfo->dwChannelCount);
    for (UINT32 i = 0; i < pInfo->dwChannelCount && i < NET_TV_MAX_CHANNEL_NUM; ++i)
    {
        const NET_TV_CHANNEL_INFO_S* pChannel = &pInfo->stChannels[i];
        printf("  [%u] Channel=%u, Enable=%u, Online=%u, Name=%s\n",
               i, pChannel->dwChannel, pChannel->byEnable, pChannel->byOnline, pChannel->szChannelName);
        printf("      HasRecord=%u, RecordStatus=%d\n", pChannel->byHasRecord, pChannel->nRecordStatus);
        printf("      DevType=%s, SerialNum=%s\n", pChannel->szDevType, pChannel->szSerialNum);
        printf("      DeviceIP=%s, CtrlPort=%d\n", pChannel->szDeviceIP, pChannel->nCtrlPort);
        printf("      PreviewMainUrl=%s\n", pChannel->szPreviewMainUrl);
        printf("      PreviewSubUrl =%s\n", pChannel->szPreviewSubUrl);
        printf("      RtspMainUrl=%s\n", pChannel->szRtspMainUrl);
        printf("      RtspSubUrl =%s\n", pChannel->szRtspSubUrl);
    }
    printf("=================================\n");
}

static void DoGetChannelInfo()
{
    NET_TV_CHANNEL_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    printf("[Client] Calling NET_TV_GetDevConfig to get channel info...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_CHANNEL_INFO,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] Get channel info success! BytesReturned=%d\n", dwBytesReturned);
        PrintChannelInfo(&stInfo);
    }
    else
    {
        printf("[Client] Get channel info failed! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoGetChannelList()
{
    NET_TV_CHANNEL_LIST_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    printf("[Client] Calling NET_TV_GetDevConfig to get channel list...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_CHANNEL_LIST,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] Get channel list success! BytesReturned=%d\n", dwBytesReturned);
        PrintChannelList(&stInfo);
    }
    else
    {
        printf("[Client] Get channel list failed! Error=%d\n", NET_TV_GetLastError());
    }
}

/* Get preview info */
static void DoGetPreviewInfo()
{
    NET_TV_PREVIEW_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    printf("[Client] Calling NET_TV_GetDevConfig to get preview info...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_PREVIEW_INFO,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] Get preview info success! BytesReturned=%d\n", dwBytesReturned);
        PrintPreviewInfo(&stInfo);
    }
    else
    {
        printf("[Client] Get preview info failed! Error=%d\n", NET_TV_GetLastError());
    }
}

/* Set preview info */
static void DoSetPreviewInfo()
{
    NET_TV_PREVIEW_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    BOOL bRetGet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_PREVIEW_INFO,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (!bRetGet)
    {
        printf("[Client] Pre-get preview info failed, use default values! Error=%d\n", NET_TV_GetLastError());
        memset(&stInfo, 0, sizeof(stInfo));
    }

    strncpy(stInfo.stRtspUrl.szRtspMainUrl, "rtsp://127.0.0.1:554/live/main", sizeof(stInfo.stRtspUrl.szRtspMainUrl) - 1);
    stInfo.stRtspUrl.szRtspMainUrl[sizeof(stInfo.stRtspUrl.szRtspMainUrl) - 1] = '\0';
    strncpy(stInfo.stRtspUrl.szRtspSubUrl, "rtsp://127.0.0.1:554/live/sub", sizeof(stInfo.stRtspUrl.szRtspSubUrl) - 1);
    stInfo.stRtspUrl.szRtspSubUrl[sizeof(stInfo.stRtspUrl.szRtspSubUrl) - 1] = '\0';
    stInfo.stImageParam.nBrightness = 55;
    stInfo.stImageParam.nContrast = 60;
    stInfo.stImageParam.nSaturation = 58;
    stInfo.stImageParam.nSharpness = 62;

    printf("[Client] Calling NET_TV_SetDevConfig to set preview info...\n");
    INT32 dwBytesReturnedSet = 0;
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_PREVIEW_INFO,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturnedSet
    );

    if (bRet)
    {
        printf("[Client] Set preview info success! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetPreviewInfo();
    }
    else
    {
        printf("[Client] Set preview info failed! Error=%d\n", NET_TV_GetLastError());
    }
}

/* 打印升级状态 */
static void PrintUpgradeStatusCfg(const NET_TV_UPGRADE_STATUS_S *pstCfg)
{
    if (!pstCfg)
        return;
    printf("    nUpgradeStatus=%d\n", pstCfg->nUpgradeStatus);
}

/* 打印升级文件版本信息 */
static void PrintUpgradeVersionCfg(const NET_TV_UPGRADE_VERSION_S *pstCfg)
{
    if (!pstCfg)
        return;
    printf("    szVersion=%s\n", pstCfg->szVersion);
}

/* 设置升级文件路径 */
static void DoSetUpgradeCfg()
{
    NET_TV_UPGRADE_INFO_S stCfg;
    BOOL bRet = FALSE;

    INT32 dwBytesReturned = 0;
    memset(&stCfg, 0, sizeof(stCfg));    

    bRet = NET_TV_GetDevConfig(
        g_lpUserID,
        1,
        NET_TV_SET_UPGRADE,
        &stCfg,
        (INT32)sizeof(stCfg),
        &dwBytesReturned
    );
    if (!bRet)
    {
        printf("[Client] 预获取升级文件配置失败，使用默认值. Error=%d\n", NET_TV_GetLastError());
        memset(&stCfg, 0, sizeof(stCfg));
    }

    strncpy(stCfg.szUpgradePath, "/opt/course/UpgradePacket.bin", sizeof(stCfg.szUpgradePath) - 1);
    stCfg.szUpgradePath[sizeof(stCfg.szUpgradePath) - 1] = '\0';
    INT32 dwBytesReturnedSet = 0;
    bRet = NET_TV_SetDevConfig(
        g_lpUserID,
        1,
        NET_TV_SET_UPGRADE,
        &stCfg,
        (INT32)sizeof(stCfg),
        &dwBytesReturnedSet
    );

    if (bRet)
    {
        printf("[Client] 设置升级参数成功，升级文件路径为 %s!\n", stCfg.szUpgradePath);
    }
    else
    {
        printf("[Client] 设置升级参数失败! Error=%d\n", NET_TV_GetLastError());
    }
}

/* 获取升级状态信息 */
static void DoGetUpgradeStatusCfg()
{
    NET_TV_UPGRADE_STATUS_S stCfg;
    INT32 dwBytesReturned = 0;
    BOOL bRet = FALSE;

    memset(&stCfg, 0, sizeof(stCfg));

    printf("[Client] 调用 NET_TV_GetDevConfig 获取升级状态...\n");
    bRet = NET_TV_GetDevConfig(
        g_lpUserID,
        1,
        NET_TV_GET_UPGRADESTATUS,
        &stCfg,
        (INT32)sizeof(stCfg),
        &dwBytesReturned);
    
    if (bRet)
    {
        printf("[Client] 获取升级状态成功! BytesReturned=%d\n", dwBytesReturned);
        PrintUpgradeStatusCfg(&stCfg);
    }
    else
    {
        printf("[Client] 获取升级状态失败! Error=%d\n", NET_TV_GetLastError());
    }
}

/* 获取升级版本信息 */
static void DoGetUpgradeVersionCfg(void)
{
    NET_TV_UPGRADE_VERSION_S stCfg;
    INT32 dwBytesReturned = 0;
    BOOL bRet = FALSE;

    memset(&stCfg, 0, sizeof(stCfg));

    printf("[Client] 调用 NET_TV_GetDevConfig 获取可升级版本...\n");
    bRet = NET_TV_GetDevConfig(
        g_lpUserID,
        1,
        NET_TV_GET_UPGRADEVERSION,
        &stCfg,
        (INT32)sizeof(stCfg),
        &dwBytesReturned);

    if (bRet)
    {
        printf("[Client] 获取可升级版本成功! BytesReturned=%d\n", dwBytesReturned);
        PrintUpgradeVersionCfg(&stCfg);
    }
    else
    {
        printf("[Client] 获取可升级版本失败! Error=%d\n", NET_TV_GetLastError());
    }
}

/* 打印抓图计划配置 */
static void PrintCapturePlanCfg(const NET_TV_CAPTURE_PLAN_INFO_S *pstCfg)
{
    UINT32 i = 0;
    if (!pstCfg)
        return;

    for (i = 0; i < NET_TV_PLAN_DAY_NUM_AWEEK; ++i)
    {
        UINT32 j = 0;
        UINT32 nTimeCount = pstCfg->astDaySchedules[i].udwTimeCount;
        if (nTimeCount > NET_TV_PLAN_TIME_SECTION_NUM_ADAY)
            nTimeCount = NET_TV_PLAN_TIME_SECTION_NUM_ADAY;

        printf("    Day[%u] week=%d timeCount=%u\n",
               i,
               pstCfg->astDaySchedules[i].nDayOfWeek,
               nTimeCount);

        for (j = 0; j < nTimeCount; ++j)
        {
            printf("      Time[%u] start=%d end=%d\n",
                   j,
                   pstCfg->astDaySchedules[i].astTimes[j].nStartTime,
                   pstCfg->astDaySchedules[i].astTimes[j].nEndTime);
        }
    }
}

/* 打印单词抓图配置 */
static void PrintOneCaptureConfig(const char *prefix, const NET_TV_CAPTURE_CONFIG_S *pstCfg)
{
    if (!pstCfg)
        return;

    printf("    %s enable=%d format=%d resolution=%dx%d quality=%d interval=%u unit=%d number=%u\n",
           prefix ? prefix : "CaptureCfg",
           pstCfg->bEnable,
           pstCfg->enPictureFormat,
           pstCfg->nWidth,
           pstCfg->nHeight,
           pstCfg->enImageQuality,
           pstCfg->unInterval,
           pstCfg->enTimeUnit,
           pstCfg->unNumber);
}

/* 打印抓图参数配置 */
static void PrintCaptureParamCfg(const NET_TV_CAPTURE_PARAM_INFO_S *pstCfg)
{
    if (!pstCfg)
        return;

    PrintOneCaptureConfig("TimingCfg", &pstCfg->stCaptureTimingConfig);
    PrintOneCaptureConfig("EventCfg", &pstCfg->stCaptureEventConfig);
}

/* 获取抓图计划配置 */
static void DoGetCapturePlanCfg(void)
{
    NET_TV_CAPTURE_PLAN_INFO_S stCfg;
    INT32 dwBytesReturned = 0;
    BOOL bRet = FALSE;

    memset(&stCfg, 0, sizeof(stCfg));
    printf("[Client] 调用 NET_TV_GetDevConfig 获取告警抓图计划...\n");

    bRet = NET_TV_GetDevConfig(
        g_lpUserID,
        1,
        NET_TV_GET_CAPTURE_PLAN_INFO,
        &stCfg,
        (INT32)sizeof(stCfg),
        &dwBytesReturned);

    if (bRet)
    {
        printf("[Client] 获取告警抓图计划成功! BytesReturned=%d\n", dwBytesReturned);
        PrintCapturePlanCfg(&stCfg);
    }
    else
    {
        printf("[Client] 获取告警抓图计划失败! Error=%d\n", NET_TV_GetLastError());
    }
}

/* 设置抓图计划配置 */
static void DoSetCapturePlanCfg(void)
{
    NET_TV_CAPTURE_PLAN_INFO_S stCfg;
    INT32 dwBytesReturned = 0;
    BOOL bRetGet = FALSE;
    BOOL bRet = FALSE;
    UINT32 i = 0;

    memset(&stCfg, 0, sizeof(stCfg));
    bRetGet = NET_TV_GetDevConfig(
        g_lpUserID,
        1,
        NET_TV_GET_CAPTURE_PLAN_INFO,
        &stCfg,
        (INT32)sizeof(stCfg),
        &dwBytesReturned);

    if (!bRetGet)
    {
        printf("[Client] 预获取告警抓图计划失败，使用默认值. Error=%d\n", NET_TV_GetLastError());
        memset(&stCfg, 0, sizeof(stCfg));
    }
    else
    {
        printf("[Client] 预获取告警抓图计划成功. BytesReturned=%d\n", dwBytesReturned);
    }

    /* 设置默认值并给出示例修改 */
    for (i = 0; i < NET_TV_PLAN_DAY_NUM_AWEEK; ++i)
    {
        stCfg.astDaySchedules[i].nDayOfWeek = (INT32)(i + 1);
        stCfg.astDaySchedules[i].udwTimeCount = 1;
        stCfg.astDaySchedules[i].astTimes[0].nStartTime = 0;
        stCfg.astDaySchedules[i].astTimes[0].nEndTime = 24 * 60 * 60;
    }
    stCfg.astDaySchedules[0].udwTimeCount = 2;
    stCfg.astDaySchedules[0].astTimes[0].nStartTime = 8 * 60 * 60;
    stCfg.astDaySchedules[0].astTimes[0].nEndTime = 12 * 60 * 60;
    stCfg.astDaySchedules[0].astTimes[1].nStartTime = 14 * 60 * 60;
    stCfg.astDaySchedules[0].astTimes[1].nEndTime = 18 * 60 * 60;

    printf("[Client] 调用 NET_TV_SetDevConfig 设置告警抓图计划...\n");
    PrintCapturePlanCfg(&stCfg);

    INT32 dwBytesReturnedSet = 0;
    bRet = NET_TV_SetDevConfig(
        g_lpUserID,
        1,
        NET_TV_SET_CAPTURE_PLAN_INFO,
        &stCfg,
        (INT32)sizeof(stCfg),
        &dwBytesReturnedSet
    );

    if (bRet)
    {
        printf("[Client] 设置告警抓图计划成功!\n");
        DoGetCapturePlanCfg();
    }
    else
    {
        printf("[Client] 设置告警抓图计划失败! Error=%d\n", NET_TV_GetLastError());
    }
}

/* 获取抓图参数配置 */
static void DoGetCaptureParamCfg(void)
{
    NET_TV_CAPTURE_PARAM_INFO_S stCfg;
    INT32 dwBytesReturned = 0;
    BOOL bRet = FALSE;

    memset(&stCfg, 0, sizeof(stCfg));
    printf("[Client] 调用 NET_TV_GetDevConfig 获取告警抓图参数...\n");

    bRet = NET_TV_GetDevConfig(
        g_lpUserID,
        1,
        NET_TV_GET_CAPTURE_PARAM_INFO,
        &stCfg,
        (INT32)sizeof(stCfg),
        &dwBytesReturned);

    if (bRet)
    {
        printf("[Client] 获取告警抓图参数成功! BytesReturned=%d\n", dwBytesReturned);
        PrintCaptureParamCfg(&stCfg);
    }
    else
    {
        printf("[Client] 获取告警抓图参数失败! Error=%d\n", NET_TV_GetLastError());
    }
}

/* 设置抓图参数配置 */
static void DoSetCaptureParamCfg(void)
{
    NET_TV_CAPTURE_PARAM_INFO_S stCfg;
    INT32 dwBytesReturned = 0;
    BOOL bRetGet = FALSE;
    BOOL bRet = FALSE;

    memset(&stCfg, 0, sizeof(stCfg));
    bRetGet = NET_TV_GetDevConfig(
        g_lpUserID,
        1,
        NET_TV_GET_CAPTURE_PARAM_INFO,
        &stCfg,
        (INT32)sizeof(stCfg),
        &dwBytesReturned);

    if (!bRetGet)
    {
        printf("[Client] 预获取告警抓图参数失败，使用默认值. Error=%d\n", NET_TV_GetLastError());
        memset(&stCfg, 0, sizeof(stCfg));
    }
    else
    {
        printf("[Client] 预获取告警抓图参数成功. BytesReturned=%d\n", dwBytesReturned);
    }

    /* 设置默认值并给出示例修改 */
    stCfg.stCaptureTimingConfig.bEnable = TRUE;
    stCfg.stCaptureTimingConfig.enPictureFormat = NET_TV_CAPTURE_PICTURE_FORMAT_JPEG;
    stCfg.stCaptureTimingConfig.nWidth = 1920;
    stCfg.stCaptureTimingConfig.nHeight = 1080;
    stCfg.stCaptureTimingConfig.enImageQuality = NET_TV_CAPTURE_IMAGE_QUALITY_MEDIUM;
    stCfg.stCaptureTimingConfig.unInterval = 2000;
    stCfg.stCaptureTimingConfig.enTimeUnit = NET_TV_CAPTURE_TIME_UNIT_MILLISECONDS;
    stCfg.stCaptureTimingConfig.unNumber = 20;

    stCfg.stCaptureEventConfig.bEnable = TRUE;
    stCfg.stCaptureEventConfig.enPictureFormat = NET_TV_CAPTURE_PICTURE_FORMAT_JPEG;
    stCfg.stCaptureEventConfig.nWidth = 1920;
    stCfg.stCaptureEventConfig.nHeight = 1080;
    stCfg.stCaptureEventConfig.enImageQuality = NET_TV_CAPTURE_IMAGE_QUALITY_HIGH;
    stCfg.stCaptureEventConfig.unInterval = 1;
    stCfg.stCaptureEventConfig.enTimeUnit = NET_TV_CAPTURE_TIME_UNIT_SECONDS;
    stCfg.stCaptureEventConfig.unNumber = 1;

    printf("[Client] 调用 NET_TV_SetDevConfig 设置告警抓图参数...\n");
    PrintCaptureParamCfg(&stCfg);

    INT32 dwBytesReturnedSet = 0;
    bRet = NET_TV_SetDevConfig(
        g_lpUserID,
        1,
        NET_TV_SET_CAPTURE_PARAM_INFO,
        &stCfg,
        (INT32)sizeof(stCfg),
        &dwBytesReturnedSet
    );

    if (bRet)
    {
        printf("[Client] 设置告警抓图参数成功!\n");
        DoGetCaptureParamCfg();
    }
    else
    {
        printf("[Client] 设置告警抓图参数失败! Error=%d\n", NET_TV_GetLastError());
    }
}

/**
 * @brief 打印曝光信息
 * @param pstCfg 曝光信息结构体指针
 */
static void PrintExposureCfg(const NET_TV_EXPOSURE_INFO_S *pstCfg)
{
    if (!pstCfg)
        return;
    printf("    enExpTime=%d antiBanding=%d\n", pstCfg->enExpTime, pstCfg->bAntiBanding);
}

/**
 * @brief 打印日夜信息
 * @param pstCfg 日夜信息结构体指针
 */
static void PrintDayNightCfg(const NET_TV_DAYNIGHT_INFO_S *pstCfg)
{
    if (!pstCfg)
        return;
    printf("    mode=%d begin=%02d:%02d:%02d.%03d end=%02d:%02d:%02d.%03d\n",
           pstCfg->enDayNightMode,
           pstCfg->nBeginHour,
           pstCfg->nBeginMinute,
           pstCfg->nBeginSecond,
           pstCfg->nBeginMilliSec,
           pstCfg->nEndHour,
           pstCfg->nEndMinute,
           pstCfg->nEndSecond,
           pstCfg->nEndMilliSec);
    printf("    sensitivity=%u filterTime=%u fillLightExp=%d lightMode=%d lightType=%d\n",
           pstCfg->nSensitivityLevel,
           pstCfg->nFilterTime,
           pstCfg->bFillLightExp,
           pstCfg->enLightMode,
           pstCfg->enLightType);
    printf("    whiteEnable=%d whiteLevel=%d redEnable=%d redLevel=%d\n",
           pstCfg->bWhiteLightEnable,
           pstCfg->nWhiteLightLevel,
           pstCfg->bRedLightEnable,
           pstCfg->nRedLightLevel);
}

/**
 * @brief 打印背光信息
 * @param pstCfg 背光信息结构体指针
 */
static void PrintBackLightCfg(const NET_TV_BACKLIGHT_INFO_S *pstCfg)
{
    if (!pstCfg)
        return;
    printf("    area=%d wdrEnable=%d wdrLevel=%d hlsEnable=%d hlsLevel=%d\n",
           pstCfg->enBackLightArea,
           pstCfg->bWdrEnable,
           pstCfg->nWdrLevel,
           pstCfg->bHlsEnable,
           pstCfg->nHlsLevel);
}

/**
 * @brief 打印降噪信息
 * @param pstCfg 降噪信息结构体指针
 */
static void PrintDenoiseCfg(const NET_TV_DENOISE_INFO_S *pstCfg)
{
    if (!pstCfg)
        return;
    printf("    mode=%d dnrLevel=%u snrLevel=%u tnrLevel=%u\n",
           pstCfg->enDnrMode,
           pstCfg->nDnrLevel,
           pstCfg->nSnrLevel,
           pstCfg->nTnrLevel);
}

/**
 * @brief 打印白平衡信息
 * @param pstCfg 白平衡信息结构体指针
 */
static void PrintWhiteBalanceCfg(const NET_TV_WHITEBALANCE_INFO_S *pstCfg)
{
    if (!pstCfg)
        return;
    printf("    mode=%d rGain=%u bGain=%u\n",
           pstCfg->enAwbMode,
           pstCfg->nRGain,
           pstCfg->nBGain);
}

/**
 * @brief 获取曝光信息
 */
static void DoGetExposureCfg(void)
{
    NET_TV_EXPOSURE_INFO_S stCfg;
    INT32 dwBytesReturned = 0;
    BOOL bRet = FALSE;

    memset(&stCfg, 0, sizeof(stCfg));
    printf("[Client] 调用 NET_TV_GetDevConfig 获取曝光信息\n");

    bRet = NET_TV_GetDevConfig(
        g_lpUserID,
        1,
        NET_TV_GET_EXPOSURE_INFO,
        &stCfg,
        (INT32)sizeof(stCfg),
        &dwBytesReturned);

    if (bRet)
    {
        printf("[Client] 获取曝光信息成功, bytes=%d\n", dwBytesReturned);
        PrintExposureCfg(&stCfg);
    }
    else
    {
        printf("[Client] 获取曝光信息失败, error=%d\n", NET_TV_GetLastError());
    }
}

/**
 * @brief 设置曝光信息
 */
static void DoSetExposureCfg(void)
{
    NET_TV_EXPOSURE_INFO_S stCfg;
    INT32 dwBytesReturned = 0;
    BOOL bRetGet = FALSE;
    BOOL bRet = FALSE;

    memset(&stCfg, 0, sizeof(stCfg));
    bRetGet = NET_TV_GetDevConfig(
        g_lpUserID,
        1,
        NET_TV_GET_EXPOSURE_INFO,
        &stCfg,
        (INT32)sizeof(stCfg),
        &dwBytesReturned);

    if (!bRetGet)
    {
        printf("[Client] 预获取曝光信息失败, 使用默认值, error=%d\n", NET_TV_GetLastError());
        memset(&stCfg, 0, sizeof(stCfg));
    }
    else
    {
        printf("[Client] 预获取曝光信息成功, bytes=%d\n", dwBytesReturned);
    }

    stCfg.enExpTime = 0;
    stCfg.bAntiBanding = TRUE;

    printf("[Client] 调用 NET_TV_SetDevConfig 设置曝光信息\n");
    PrintExposureCfg(&stCfg);

    INT32 dwBytesReturnedSet = 0;
    bRet = NET_TV_SetDevConfig(
        g_lpUserID,
        1,
        NET_TV_SET_EXPOSURE_INFO,
        &stCfg,
        (INT32)sizeof(stCfg),
        &dwBytesReturnedSet
    );

    if (bRet)
    {
        printf("[Client] 设置曝光信息成功\n");
        DoGetExposureCfg();
    }
    else
    {
        printf("[Client] 设置曝光信息失败, error=%d\n", NET_TV_GetLastError());
    }
}

/**
 * @brief 获取日夜信息
 */
static void DoGetDayNightCfg(void)
{
    NET_TV_DAYNIGHT_INFO_S stCfg;
    INT32 dwBytesReturned = 0;
    BOOL bRet = FALSE;

    memset(&stCfg, 0, sizeof(stCfg));
    printf("[Client] 调用 NET_TV_GetDevConfig 获取日夜转换信息\n");

    bRet = NET_TV_GetDevConfig(
        g_lpUserID,
        1,
        NET_TV_GET_DAYNIGHT_INFO,
        &stCfg,
        (INT32)sizeof(stCfg),
        &dwBytesReturned);

    if (bRet)
    {
        printf("[Client] 获取日夜转换信息成功, bytes=%d\n", dwBytesReturned);
        PrintDayNightCfg(&stCfg);
    }
    else
    {
        printf("[Client] 获取日夜转换信息失败, error=%d\n", NET_TV_GetLastError());
    }
}

/**
 * @brief 设置日夜信息
 */
static void DoSetDayNightCfg(void)
{
    NET_TV_DAYNIGHT_INFO_S stCfg;
    INT32 dwBytesReturned = 0;
    BOOL bRetGet = FALSE;
    BOOL bRet = FALSE;

    memset(&stCfg, 0, sizeof(stCfg));
    bRetGet = NET_TV_GetDevConfig(
        g_lpUserID,
        1,
        NET_TV_GET_DAYNIGHT_INFO,
        &stCfg,
        (INT32)sizeof(stCfg),
        &dwBytesReturned);

    if (!bRetGet)
    {
        printf("[Client] 预获取获取日夜转换信息失败, 使用默认值, error=%d\n", NET_TV_GetLastError());
        memset(&stCfg, 0, sizeof(stCfg));
    }
    else
    {
        printf("[Client] 预获取日夜转换信息成功, bytes=%d\n", dwBytesReturned);
    }

    stCfg.enDayNightMode = 0;
    stCfg.nBeginHour = 18;
    stCfg.nBeginMinute = 0;
    stCfg.nBeginSecond = 0;
    stCfg.nBeginMilliSec = 0;
    stCfg.nEndHour = 6;
    stCfg.nEndMinute = 0;
    stCfg.nEndSecond = 0;
    stCfg.nEndMilliSec = 0;
    stCfg.nSensitivityLevel = 50;
    stCfg.nFilterTime = 5;
    stCfg.bFillLightExp = TRUE;
    stCfg.enLightMode = 0;
    stCfg.enLightType = 0;
    stCfg.bWhiteLightEnable = TRUE;
    stCfg.nWhiteLightLevel = 50;
    stCfg.bRedLightEnable = FALSE;
    stCfg.nRedLightLevel = 0;

    printf("[Client] 调用 NET_TV_SetDevConfig 设置日夜转换信息\n");
    PrintDayNightCfg(&stCfg);

    INT32 dwBytesReturnedSet = 0;
    bRet = NET_TV_SetDevConfig(
        g_lpUserID,
        1,
        NET_TV_SET_DAYNIGHT_INFO,
        &stCfg,
        (INT32)sizeof(stCfg),
        &dwBytesReturnedSet
    );

    if (bRet)
    {
        printf("[Client] 设置日夜转换信息成功\n");
        DoGetDayNightCfg();
    }
    else
    {
        printf("[Client] 设置日夜转换信息失败, error=%d\n", NET_TV_GetLastError());
    }
}

/**
 * @brief 获取背光信息
 */
static void DoGetBackLightCfg(void)
{
    NET_TV_BACKLIGHT_INFO_S stCfg;
    INT32 dwBytesReturned = 0;
    BOOL bRet = FALSE;

    memset(&stCfg, 0, sizeof(stCfg));
    printf("[Client] 调用 NET_TV_GetDevConfig 获取背光信息\n");

    bRet = NET_TV_GetDevConfig(
        g_lpUserID,
        1,
        NET_TV_GET_BACKLIGHT_INFO,
        &stCfg,
        (INT32)sizeof(stCfg),
        &dwBytesReturned);

    if (bRet)
    {
        printf("[Client] 获取背光信息成功, bytes=%d\n", dwBytesReturned);
        PrintBackLightCfg(&stCfg);
    }
    else
    {
        printf("[Client] 获取背光信息失败, error=%d\n", NET_TV_GetLastError());
    }
}

/**
 * @brief 设置背光信息
 */
static void DoSetBackLightCfg(void)
{
    NET_TV_BACKLIGHT_INFO_S stCfg;
    INT32 dwBytesReturned = 0;
    BOOL bRetGet = FALSE;
    BOOL bRet = FALSE;

    memset(&stCfg, 0, sizeof(stCfg));
    bRetGet = NET_TV_GetDevConfig(
        g_lpUserID,
        1,
        NET_TV_GET_BACKLIGHT_INFO,
        &stCfg,
        (INT32)sizeof(stCfg),
        &dwBytesReturned);

    if (!bRetGet)
    {
        printf("[Client] 预获取背光信息失败, 使用默认值, error=%d\n", NET_TV_GetLastError());
        memset(&stCfg, 0, sizeof(stCfg));
    }
    else
    {
        printf("[Client] 预获取背光信息成功, bytes=%d\n", dwBytesReturned);
    }

    stCfg.enBackLightArea = 0;
    stCfg.bWdrEnable = TRUE;
    stCfg.nWdrLevel = 50;
    stCfg.bHlsEnable = FALSE;
    stCfg.nHlsLevel = 0;

    printf("[Client] 调用 NET_TV_SetDevConfig 设置背光信息\n");
    PrintBackLightCfg(&stCfg);

    INT32 dwBytesReturnedSet = 0;
    bRet = NET_TV_SetDevConfig(
        g_lpUserID,
        1,
        NET_TV_SET_BACKLIGHT_INFO,
        &stCfg,
        (INT32)sizeof(stCfg),
        &dwBytesReturnedSet
    );

    if (bRet)
    {
        printf("[Client] 设置背光信息成功\n");
        DoGetBackLightCfg();
    }
    else
    {
        printf("[Client] 设置背光信息失败, error=%d\n", NET_TV_GetLastError());
    }
}

/**
 * @brief 获取降噪信息
 */
static void DoGetDenoiseCfg(void)
{
    NET_TV_DENOISE_INFO_S stCfg;
    INT32 dwBytesReturned = 0;
    BOOL bRet = FALSE;

    memset(&stCfg, 0, sizeof(stCfg));
    printf("[Client] 调用 NET_TV_GetDevConfig 获取降噪信息\n");

    bRet = NET_TV_GetDevConfig(
        g_lpUserID,
        1,
        NET_TV_GET_DENOISE_INFO,
        &stCfg,
        (INT32)sizeof(stCfg),
        &dwBytesReturned);

    if (bRet)
    {
        printf("[Client] 获取降噪信息成功, bytes=%d\n", dwBytesReturned);
        PrintDenoiseCfg(&stCfg);
    }
    else
    {
        printf("[Client] 获取降噪信息失败, error=%d\n", NET_TV_GetLastError());
    }
}

/**
 * @brief 设置降噪信息
 */
static void DoSetDenoiseCfg(void)
{
    NET_TV_DENOISE_INFO_S stCfg;
    INT32 dwBytesReturned = 0;
    BOOL bRetGet = FALSE;
    BOOL bRet = FALSE;

    memset(&stCfg, 0, sizeof(stCfg));
    bRetGet = NET_TV_GetDevConfig(
        g_lpUserID,
        1,
        NET_TV_GET_DENOISE_INFO,
        &stCfg,
        (INT32)sizeof(stCfg),
        &dwBytesReturned);

    if (!bRetGet)
    {
        printf("[Client] 预获取降噪信息失败, 使用默认值, error=%d\n", NET_TV_GetLastError());
        memset(&stCfg, 0, sizeof(stCfg));
    }
    else
    {
        printf("[Client] 预获取降噪信息成功, bytes=%d\n", dwBytesReturned);
    }

    stCfg.enDnrMode = 0;
    stCfg.nDnrLevel = 50;
    stCfg.nSnrLevel = 50;
    stCfg.nTnrLevel = 50;

    printf("[Client] 调用 NET_TV_SetDevConfig 设置降噪信息\n");
    PrintDenoiseCfg(&stCfg);

    INT32 dwBytesReturnedSet = 0;
    bRet = NET_TV_SetDevConfig(
        g_lpUserID,
        1,
        NET_TV_SET_DENOISE_INFO,
        &stCfg,
        (INT32)sizeof(stCfg),
        &dwBytesReturnedSet
    );

    if (bRet)
    {
        printf("[Client] 设置降噪信息成功\n");
        DoGetDenoiseCfg();
    }
    else
    {
        printf("[Client] 设置降噪信息失败, error=%d\n", NET_TV_GetLastError());
    }
}

/**
 * @brief 获取白平衡信息
 */
static void DoGetWhiteBalanceCfg(void)
{
    NET_TV_WHITEBALANCE_INFO_S stCfg;
    INT32 dwBytesReturned = 0;
    BOOL bRet = FALSE;

    memset(&stCfg, 0, sizeof(stCfg));
    printf("[Client] 调用 NET_TV_GetDevConfig 获取白平衡信息\n");

    bRet = NET_TV_GetDevConfig(
        g_lpUserID,
        1,
        NET_TV_GET_WHITEBALANCE_INFO,
        &stCfg,
        (INT32)sizeof(stCfg),
        &dwBytesReturned);

    if (bRet)
    {
        printf("[Client] 获取白平衡信息成功, bytes=%d\n", dwBytesReturned);
        PrintWhiteBalanceCfg(&stCfg);
    }
    else
    {
        printf("[Client] 获取白平衡信息失败, error=%d\n", NET_TV_GetLastError());
    }
}

/**
 * @brief 设置白平衡信息
 */
static void DoSetWhiteBalanceCfg(void)
{
    NET_TV_WHITEBALANCE_INFO_S stCfg;
    INT32 dwBytesReturned = 0;
    BOOL bRetGet = FALSE;
    BOOL bRet = FALSE;

    memset(&stCfg, 0, sizeof(stCfg));
    bRetGet = NET_TV_GetDevConfig(
        g_lpUserID,
        1,
        NET_TV_GET_WHITEBALANCE_INFO,
        &stCfg,
        (INT32)sizeof(stCfg),
        &dwBytesReturned);

    if (!bRetGet)
    {
        printf("[Client] 预获取白平衡信息失败, 使用默认值, error=%d\n", NET_TV_GetLastError());
        memset(&stCfg, 0, sizeof(stCfg));
    }
    else
    {
        printf("[Client] 预获取白平衡信息成功, bytes=%d\n", dwBytesReturned);
    }

    stCfg.enAwbMode = 0;
    stCfg.nRGain = 100;
    stCfg.nBGain = 100;

    printf("[Client] 调用 NET_TV_SetDevConfig 设置白平衡信息\n");
    PrintWhiteBalanceCfg(&stCfg);

    INT32 dwBytesReturnedSet = 0;
    bRet = NET_TV_SetDevConfig(
        g_lpUserID,
        1,
        NET_TV_SET_WHITEBALANCE_INFO,
        &stCfg,
        (INT32)sizeof(stCfg),
        &dwBytesReturnedSet
    );

    if (bRet)
    {
        printf("[Client] 设置白平衡信息成功\n");
        DoGetWhiteBalanceCfg();
    }
    else
    {
        printf("[Client] 设置白平衡信息失败, error=%d\n", NET_TV_GetLastError());
    }
}

static void PrintTalkbackStreamInfo(const NET_TV_TALKBACK_STREAM_INFO_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== Talkback Stream Info =====\n");
    printf("  Host         : %s\n", pInfo->szHost);
    printf("  Port         : %d\n", pInfo->nPort);
    printf("  ChnId        : %d\n", pInfo->nChnId);
    printf("  UserId       : %d\n", pInfo->nUserID);
    printf("  MainStream   : %s\n", pInfo->bMainStream ? "ON" : "OFF");
    printf("  Protocol     : %s\n", pInfo->szProtocol);
    printf("  StartTime    : %s\n", pInfo->szStartTime);
    printf("  EndTime      : %s\n", pInfo->szEndTime);
    printf("  Filename     : %s\n", pInfo->szFileName);
    printf("========================================\n");
}

static void DoGetTalkbackFromStream()
{
    NET_TV_TALKBACK_STREAM_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_FROM_STREAM_TALKBACK,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] Get talkback from stream success! BytesReturned=%d\n", dwBytesReturned);
        PrintTalkbackStreamInfo(&stInfo);
    }
    else
    {
        printf("[Client] Get talkback from stream failed! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoSetTalkbackState()
{
    NET_TV_TALKBACK_STATE_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    stInfo.bEnable = TRUE;
    strncpy(stInfo.szSdp, "v=0\\r\\no=- 0 0 IN IP4 127.0.0.1\\r\\ns=Talkback\\r\\n", sizeof(stInfo.szSdp) - 1);
    strncpy(stInfo.szUrl, "rtsp://127.0.0.1:554/talkback", sizeof(stInfo.szUrl) - 1);
    strncpy(stInfo.szLocalIP, "127.0.0.1", sizeof(stInfo.szLocalIP) - 1);

    INT32 dwBytesReturned = 0;
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_STATE_TALKBACK,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] Set talkback state success! BytesReturned=%d\n", dwBytesReturned);
    }
    else
    {
        printf("[Client] Set talkback state failed! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoSetTalkbackToStream()
{
    NET_TV_TALKBACK_STREAM_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    strncpy(stInfo.szHost, "239.0.0.1", sizeof(stInfo.szHost) - 1);
    stInfo.nPort = 5004;
    stInfo.nChnId = 1;
    stInfo.nUserID = 1001;
    stInfo.bMainStream = TRUE;
    strncpy(stInfo.szProtocol, "rtp", sizeof(stInfo.szProtocol) - 1);
    strncpy(stInfo.szStartTime, "2026-01-01 08:00:00", sizeof(stInfo.szStartTime) - 1);
    strncpy(stInfo.szEndTime, "2026-01-01 08:30:00", sizeof(stInfo.szEndTime) - 1);
    strncpy(stInfo.szFileName, "talkback_live.aac", sizeof(stInfo.szFileName) - 1);

    INT32 dwBytesReturned = 0;
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_TO_STREAM_TALKBACK,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] Set talkback to stream success! BytesReturned=%d\n", dwBytesReturned);
        DoGetTalkbackFromStream();
    }
    else
    {
        printf("[Client] Set talkback to stream failed! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoSetReplayTalkback()
{
    NET_TV_REPLAY_TALKBACK_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    strncpy(stInfo.szNvrIp, "192.168.1.200", sizeof(stInfo.szNvrIp) - 1);
    strncpy(stInfo.szRemoteIp, "192.168.1.10", sizeof(stInfo.szRemoteIp) - 1);
    strncpy(stInfo.stIPCInfo.szHost, "239.0.0.1", sizeof(stInfo.stIPCInfo.szHost) - 1);
    stInfo.stIPCInfo.nPort = 5004;
    stInfo.stIPCInfo.nChnId = 1;
    stInfo.stIPCInfo.nUserID = 1001;
    stInfo.stIPCInfo.bMainStream = TRUE;
    strncpy(stInfo.stIPCInfo.szProtocol, "rtp", sizeof(stInfo.stIPCInfo.szProtocol) - 1);
    strncpy(stInfo.stIPCInfo.szStartTime, "2026-01-01 08:00:00", sizeof(stInfo.stIPCInfo.szStartTime) - 1);
    strncpy(stInfo.stIPCInfo.szEndTime, "2026-01-01 08:30:00", sizeof(stInfo.stIPCInfo.szEndTime) - 1);
    strncpy(stInfo.stIPCInfo.szFileName, "talkback_replay.aac", sizeof(stInfo.stIPCInfo.szFileName) - 1);

    INT32 dwBytesReturned = 0;
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_REPLAY_TALKBACK,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] Set replay talkback success! BytesReturned=%d\n", dwBytesReturned);
    }
    else
    {
        printf("[Client] Set replay talkback failed! Error=%d\n", NET_TV_GetLastError());
    }
}

static void PrintReplayUrlInfo(const NET_TV_REPLAY_URL_INFO_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 回放播放地址 =====\n");
    printf("  Channel    : %d\n", pInfo->dwChannel);
    printf("  StartTime  : %s\n", pInfo->szStartTime);
    printf("  EndTime    : %s\n", pInfo->szEndTime);
    printf("  Url        : %s\n", pInfo->szUrl);
    printf("================================\n");
}

static void DoGetReplayUrl()
{
    NET_TV_REPLAY_URL_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    stInfo.dwChannel = 1;
    strncpy(stInfo.szStartTime, "2026-01-01 08:00:00", sizeof(stInfo.szStartTime) - 1);
    strncpy(stInfo.szEndTime, "2026-01-01 08:30:00", sizeof(stInfo.szEndTime) - 1);

    INT32 dwBytesReturned = 0;
    BOOL bRet = false;
    NET_TV_GetReplayUrl(g_lpUserID, &stInfo, &dwBytesReturned);

    if (bRet)
    {
        printf("[Client] Get replay url success! BytesReturned=%d\n", dwBytesReturned);
        PrintReplayUrlInfo(&stInfo);
    }
    else
    {
        printf("[Client] Get replay url failed! Error=%d\n", NET_TV_GetLastError());
    }
}

static void PrintReplayCtrlInfo(const NET_TV_REPLAY_CTRL_INFO_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 回放控制结果 =====\n");
    printf("  Channel    : %d\n", pInfo->dwChannel);
    printf("  CtrlType   : %d\n", pInfo->dwCtrlType);
    printf("  SessionId  : %s\n", pInfo->szSessionId);
    printf("  Speed      : %.2f\n", pInfo->fSpeed);
    printf("  SeekTime   : %d\n", pInfo->nSeekTime);
    printf("  ReplayType : %d\n", pInfo->nReplayType);
    printf("  StartTime  : %s\n", pInfo->szStartTime);
    printf("  EndTime    : %s\n", pInfo->szEndTime);
    printf("  Url        : %s\n", pInfo->szUrl);
    printf("================================\n");
}

static void FlushInputLine()
{
    int ch = 0;
    while ((ch = getchar()) != '\n' && ch != EOF)
    {
    }
}

static void TrimLineEnd(char* text)
{
    if (!text)
    {
        return;
    }

    size_t len = strlen(text);
    while (len > 0 && (text[len - 1] == '\n' || text[len - 1] == '\r'))
    {
        text[--len] = '\0';
    }
}

static void ReadTextWithDefault(const char* prompt, char* buffer, size_t bufferSize, const char* defaultValue)
{
    if (!buffer || bufferSize == 0)
    {
        return;
    }

    char line[128] = {0};
    if (defaultValue && defaultValue != buffer)
    {
        strncpy(buffer, defaultValue, bufferSize - 1);
        buffer[bufferSize - 1] = '\0';
    }
    else if (!defaultValue)
    {
        buffer[0] = '\0';
    }

    printf("%s[%s]: ", prompt, buffer);
    if (!fgets(line, sizeof(line), stdin))
    {
        return;
    }

    TrimLineEnd(line);
    if (line[0] != '\0')
    {
        strncpy(buffer, line, bufferSize - 1);
        buffer[bufferSize - 1] = '\0';
    }
}

static int ReadIntWithDefault(const char* prompt, int defaultValue)
{
    char defaultText[32] = {0};
    char inputText[32] = {0};
    snprintf(defaultText, sizeof(defaultText), "%d", defaultValue);
    ReadTextWithDefault(prompt, inputText, sizeof(inputText), defaultText);
    return atoi(inputText);
}

static float ReadFloatWithDefault(const char* prompt, float defaultValue)
{
    char defaultText[32] = {0};
    char inputText[32] = {0};
    snprintf(defaultText, sizeof(defaultText), "%.1f", defaultValue);
    ReadTextWithDefault(prompt, inputText, sizeof(inputText), defaultText);
    return (float)atof(inputText);
}


static void DoControlReplayStart()
{
    NET_TV_REPLAY_CTRL_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    int nChannel = 1;
    char szDate[32] = "2026-05-11";
    char szStartClock[32] = "00:00:00";
    char szEndClock[32] = "23:59:59";
    float fSpeed = 1.0f;

    FlushInputLine();
    printf("[Client] 平台点播默认走普通录像全天，直接回车使用默认值。\n");
    nChannel = ReadIntWithDefault("[Client] 请输入点播通道号", nChannel);
    ReadTextWithDefault("[Client] 请输入点播日期(YYYY-MM-DD)", szDate, sizeof(szDate), szDate);
    ReadTextWithDefault("[Client] 请输入开始时间(HH:MM:SS)", szStartClock, sizeof(szStartClock), szStartClock);
    ReadTextWithDefault("[Client] 请输入结束时间(HH:MM:SS)", szEndClock, sizeof(szEndClock), szEndClock);
    fSpeed = ReadFloatWithDefault("[Client] 请输入播放倍速", fSpeed);

    stInfo.dwChannel = nChannel;
    stInfo.dwCtrlType = NET_TV_REPLAY_CTRL_START;
    stInfo.fSpeed = fSpeed > 0.0f ? fSpeed : 1.0f;
    snprintf(stInfo.szStartTime, sizeof(stInfo.szStartTime), "%s %s", szDate, szStartClock);
    snprintf(stInfo.szEndTime, sizeof(stInfo.szEndTime), "%s %s", szDate, szEndClock);

    INT32 dwBytesReturned = 0;
    printf("[Client] Calling NET_TV_ControlReplay START, server will forward AC_PLATFORM_PLAY(%d)...\n",
           DEMO_AC_PLATFORM_PLAY);
    BOOL bRet = NET_TV_ControlReplay(g_lpUserID, &stInfo, &dwBytesReturned);

    if (bRet)
    {
        strncpy(g_szReplaySessionId, stInfo.szSessionId, sizeof(g_szReplaySessionId) - 1);
        printf("[Client] Control replay start success! BytesReturned=%d\n", dwBytesReturned);
        PrintReplayCtrlInfo(&stInfo);
    }
    else
    {
        printf("[Client] Control replay start failed! Error=%d\n", NET_TV_GetLastError());
    }

}

static void DoControlReplayStop()
{
    NET_TV_REPLAY_CTRL_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    int nChannel = 1;
    char szSessionId[NET_TV_REPLAY_SESSION_ID_LEN] = {0};
    strncpy(szSessionId,
            g_szReplaySessionId[0] ? g_szReplaySessionId : "demo_replay_1",
            sizeof(szSessionId) - 1);

    FlushInputLine();
    nChannel = ReadIntWithDefault("[Client] 请输入停止播放通道号", nChannel);
    ReadTextWithDefault("[Client] 请输入回放会话ID", szSessionId, sizeof(szSessionId), szSessionId);

    stInfo.dwChannel = nChannel;
    stInfo.dwCtrlType = NET_TV_REPLAY_CTRL_STOP;
    strncpy(stInfo.szSessionId, szSessionId, sizeof(stInfo.szSessionId) - 1);

    INT32 dwBytesReturned = 0;
    BOOL bRet = NET_TV_ControlReplay(g_lpUserID, &stInfo, &dwBytesReturned);

    if (bRet)
    {
        printf("[Client] Control replay stop success! BytesReturned=%d\n", dwBytesReturned);
        PrintReplayCtrlInfo(&stInfo);
    }
    else
    {
        printf("[Client] Control replay stop failed! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoControlReplayPause()
{
    NET_TV_REPLAY_CTRL_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    int nChannel = 1;
    char szSessionId[NET_TV_REPLAY_SESSION_ID_LEN] = {0};
    strncpy(szSessionId,
            g_szReplaySessionId[0] ? g_szReplaySessionId : "demo_replay_1",
            sizeof(szSessionId) - 1);

    FlushInputLine();
    nChannel = ReadIntWithDefault("[Client] 请输入暂停播放通道号", nChannel);
    ReadTextWithDefault("[Client] 请输入回放会话ID", szSessionId, sizeof(szSessionId), szSessionId);

    stInfo.dwChannel = nChannel;
    stInfo.dwCtrlType = NET_TV_REPLAY_CTRL_PAUSE;
    strncpy(stInfo.szSessionId, szSessionId, sizeof(stInfo.szSessionId) - 1);

    INT32 dwBytesReturned = 0;
    BOOL bRet = NET_TV_ControlReplay(g_lpUserID, &stInfo, &dwBytesReturned);

    if (bRet)
    {
        printf("[Client] Control replay pause success! BytesReturned=%d\n", dwBytesReturned);
        PrintReplayCtrlInfo(&stInfo);
    }
    else
    {
        printf("[Client] Control replay pause failed! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoControlReplayResume()
{
    NET_TV_REPLAY_CTRL_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    int nChannel = 1;
    char szSessionId[NET_TV_REPLAY_SESSION_ID_LEN] = {0};
    strncpy(szSessionId,
            g_szReplaySessionId[0] ? g_szReplaySessionId : "demo_replay_1",
            sizeof(szSessionId) - 1);

    FlushInputLine();
    nChannel = ReadIntWithDefault("[Client] 请输入恢复播放通道号", nChannel);
    ReadTextWithDefault("[Client] 请输入回放会话ID", szSessionId, sizeof(szSessionId), szSessionId);

    stInfo.dwChannel = nChannel;
    stInfo.dwCtrlType = NET_TV_REPLAY_CTRL_RESUME;
    strncpy(stInfo.szSessionId, szSessionId, sizeof(stInfo.szSessionId) - 1);

    INT32 dwBytesReturned = 0;
    BOOL bRet = NET_TV_ControlReplay(g_lpUserID, &stInfo, &dwBytesReturned);

    if (bRet)
    {
        printf("[Client] Control replay resume success! BytesReturned=%d\n", dwBytesReturned);
        PrintReplayCtrlInfo(&stInfo);
    }
    else
    {
        printf("[Client] Control replay resume failed! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoControlReplaySpeed()
{
    NET_TV_REPLAY_CTRL_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    int nChannel = 1;
    float fSpeed = 2.0f;
    char szSessionId[NET_TV_REPLAY_SESSION_ID_LEN] = {0};
    strncpy(szSessionId,
            g_szReplaySessionId[0] ? g_szReplaySessionId : "demo_replay_1",
            sizeof(szSessionId) - 1);

    FlushInputLine();
    nChannel = ReadIntWithDefault("[Client] 请输入倍速播放通道号", nChannel);
    fSpeed = ReadFloatWithDefault("[Client] 请输入播放倍速", fSpeed);
    ReadTextWithDefault("[Client] 请输入回放会话ID", szSessionId, sizeof(szSessionId), szSessionId);

    stInfo.dwChannel = nChannel;
    stInfo.dwCtrlType = NET_TV_REPLAY_CTRL_SET_SPEED;
    stInfo.fSpeed = fSpeed > 0.0f ? fSpeed : 1.0f;
    strncpy(stInfo.szSessionId, szSessionId, sizeof(stInfo.szSessionId) - 1);

    INT32 dwBytesReturned = 0;
    BOOL bRet = NET_TV_ControlReplay(g_lpUserID, &stInfo, &dwBytesReturned);

    if (bRet)
    {
        printf("[Client] Control replay speed success! BytesReturned=%d\n", dwBytesReturned);
        PrintReplayCtrlInfo(&stInfo);
    }
    else
    {
        printf("[Client] Control replay speed failed! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoControlReplaySeek()
{
    NET_TV_REPLAY_CTRL_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    int nChannel = 1;
    int nSeekTime = 60;
    char szSessionId[NET_TV_REPLAY_SESSION_ID_LEN] = {0};
    strncpy(szSessionId,
            g_szReplaySessionId[0] ? g_szReplaySessionId : "demo_replay_1",
            sizeof(szSessionId) - 1);

    FlushInputLine();
    nChannel = ReadIntWithDefault("[Client] 请输入跳转播放通道号", nChannel);
    nSeekTime = ReadIntWithDefault("[Client] 请输入跳转秒数(相对回放时间轴)", nSeekTime);
    ReadTextWithDefault("[Client] 请输入回放会话ID", szSessionId, sizeof(szSessionId), szSessionId);

    stInfo.dwChannel = nChannel;
    stInfo.dwCtrlType = NET_TV_REPLAY_CTRL_SET_SEEK;
    stInfo.nSeekTime = nSeekTime;
    strncpy(stInfo.szSessionId, szSessionId, sizeof(stInfo.szSessionId) - 1);

    INT32 dwBytesReturned = 0;
    BOOL bRet = NET_TV_ControlReplay(g_lpUserID, &stInfo, &dwBytesReturned);

    if (bRet)
    {
        printf("[Client] Control replay seek success! BytesReturned=%d\n", dwBytesReturned);
        PrintReplayCtrlInfo(&stInfo);
    }
    else
    {
        printf("[Client] Control replay seek failed! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoPlatformReplayControlByType(int nReplayType)
{
    NET_TV_REPLAY_CTRL_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    int nChannel = 1;
    int nSeekTime = 60;
    float fSpeed = 2.0f;
    char szDate[32] = "2026-05-07";
    char szStartClock[32] = "00:00:00";
    char szEndClock[32] = "23:59:59";
    char szSessionId[NET_TV_REPLAY_SESSION_ID_LEN] = {0};
    strncpy(szSessionId,
            g_szReplaySessionId[0] ? g_szReplaySessionId : "demo_replay_1",
            sizeof(szSessionId) - 1);

    nChannel = ReadIntWithDefault("[Client] 请输入平台点播通道号", nChannel);

    stInfo.dwChannel = nChannel;
    stInfo.nReplayType = nReplayType;

    switch (nReplayType)
    {
        case NET_TV_REPLAY_PLATFORM_CTRL_JUMP_TIME:
            ReadTextWithDefault("[Client] 请输入回放会话ID", szSessionId, sizeof(szSessionId), szSessionId);
            ReadTextWithDefault("[Client] 请输入跳转日期(YYYY-MM-DD)", szDate, sizeof(szDate), szDate);
            ReadTextWithDefault("[Client] 请输入跳转开始时间(HH:MM:SS)", szStartClock, sizeof(szStartClock), szStartClock);
            ReadTextWithDefault("[Client] 请输入跳转结束时间(HH:MM:SS)", szEndClock, sizeof(szEndClock), szEndClock);
            stInfo.dwCtrlType = NET_TV_REPLAY_CTRL_SET_SEEK;
            strncpy(stInfo.szSessionId, szSessionId, sizeof(stInfo.szSessionId) - 1);
            snprintf(stInfo.szStartTime, sizeof(stInfo.szStartTime), "%s %s", szDate, szStartClock);
            snprintf(stInfo.szEndTime, sizeof(stInfo.szEndTime), "%s %s", szDate, szEndClock);
            break;
        case NET_TV_REPLAY_PLATFORM_CTRL_BACKWARD_30S:
        case NET_TV_REPLAY_PLATFORM_CTRL_FORWARD_30S:
        case NET_TV_REPLAY_PLATFORM_CTRL_PERSON_EVENT:
        case NET_TV_REPLAY_PLATFORM_CTRL_VEHICLE_EVENT:
        case NET_TV_REPLAY_PLATFORM_CTRL_PERSON_VEHICLE_EVENT:
        case NET_TV_REPLAY_PLATFORM_CTRL_CANCEL_EVENT:
            ReadTextWithDefault("[Client] 请输入回放会话ID", szSessionId, sizeof(szSessionId), szSessionId);
            stInfo.dwCtrlType = NET_TV_REPLAY_CTRL_SET_SEEK;
            strncpy(stInfo.szSessionId, szSessionId, sizeof(stInfo.szSessionId) - 1);
            break;
        case NET_TV_REPLAY_PLATFORM_CTRL_SPEED:
            ReadTextWithDefault("[Client] 请输入回放会话ID", szSessionId, sizeof(szSessionId), szSessionId);
            fSpeed = ReadFloatWithDefault("[Client] 请输入播放倍速", fSpeed);
            stInfo.dwCtrlType = NET_TV_REPLAY_CTRL_SET_SPEED;
            stInfo.fSpeed = fSpeed > 0.0f ? fSpeed : 1.0f;
            strncpy(stInfo.szSessionId, szSessionId, sizeof(stInfo.szSessionId) - 1);
            break;
        case NET_TV_REPLAY_PLATFORM_CTRL_NONE:
        default:
            printf("[Client] 无效的平台点播回放控制类型: %d\n", nReplayType);
            return;
    }

    if (nReplayType == NET_TV_REPLAY_PLATFORM_CTRL_BACKWARD_30S ||
        nReplayType == NET_TV_REPLAY_PLATFORM_CTRL_FORWARD_30S)
    {
        nSeekTime = ReadIntWithDefault("[Client] 请输入跳转秒数(默认30秒，可自定义)", 30);
        // stInfo.nSeekTime = nSeekTime;
        stInfo.nSeekTime = 1777341741;
    }

    INT32 dwBytesReturned = 0;
    BOOL bRet = NET_TV_ControlReplay(g_lpUserID, &stInfo, &dwBytesReturned);

    if (bRet)
    {
        printf("[Client] Platform replay control success! BytesReturned=%d\n", dwBytesReturned);
        PrintReplayCtrlInfo(&stInfo);
    }
    else
    {
        printf("[Client] Platform replay control failed! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoReplayControlCustomMenu()
{
    int nType = 1;

    FlushInputLine();
    printf("\n[Client] ===== 平台点播回放控制类型 =====\n");
    printf("  1 - 跳进度条\n");
    printf("  2 - 后退30秒\n");
    printf("  3 - 前进30秒\n");
    printf("  4 - 倍速\n");
    printf("  5 - 人员事件\n");
    printf("  6 - 车辆事件\n");
    printf("  7 - 人车事件\n");
    printf("  8 - 取消事件\n");
    printf("=====================================\n");
    nType = ReadIntWithDefault("[Client] 请选择控制类型", nType);

    DoPlatformReplayControlByType(nType);
}

static void PrintReplayRecordSegments(const char* title, const NET_TV_REPLAY_RECORD_TIME_S* pSegments, int nCount)
{
    printf("  %s (%d):\n", title, nCount);
    for (int i = 0; i < nCount; ++i)
    {
        printf("    [%d] %d -> %d\n", i, pSegments[i].nStartTime, pSegments[i].nEndTime);
    }
}

static void PrintReplayRecordList(const NET_TV_REPLAY_RECORD_LIST_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 回放录像时间段 =====\n");
    printf("  Channel    : %d\n", pInfo->dwChannel);
    printf("  FilterType : %d\n", pInfo->bFilterByEventType);
    printf("  EventType  : %d\n", pInfo->dwEventType);
    printf("  Date       : %s\n", pInfo->szDate);
    printf("  StartTime  : %s\n", pInfo->szStartTime);
    printf("  EndTime    : %s\n", pInfo->szEndTime);
    PrintReplayRecordSegments("Video", pInfo->astVideoTimes, pInfo->nVideoCount);
    PrintReplayRecordSegments("PersonEvent", pInfo->astPersonEventTimes, pInfo->nPersonEventCount);
    PrintReplayRecordSegments("VehicleEvent", pInfo->astVehicleEventTimes, pInfo->nVehicleEventCount);
    PrintReplayRecordSegments("OtherEvent", pInfo->astOtherEventTimes, pInfo->nOtherEventCount);
    printf("==================================\n");
}

static void DoGetReplayRecordList()
{
    NET_TV_REPLAY_RECORD_LIST_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    int nChannel = 1;
    int nFilterByEventType = 0;
    int nEventType = 100;
    char szDate[32] = "2026-05-06";
    char szStartClock[32] = "00:00:00";
    char szEndClock[32] = "23:59:59";

    printf("[Client] 请输入通道号: ");
    if (scanf("%d", &nChannel) != 1)
    {
        printf("[Client] 通道号输入无效!\n");
        return;
    }

    printf("[Client] 是否按事件类型过滤(0:否 1:是): ");
    if (scanf("%d", &nFilterByEventType) != 1)
    {
        printf("[Client] 事件过滤标志输入无效!\n");
        return;
    }

    if (nFilterByEventType != 0)
    {
        printf("[Client] 请输入事件类型(例如 100 表示全部事件): ");
        if (scanf("%d", &nEventType) != 1)
        {
            printf("[Client] 事件类型输入无效!\n");
            return;
        }
    }

    printf("[Client] 请输入日期(YYYY-MM-DD): ");
    if (scanf("%31s", szDate) != 1)
    {
        printf("[Client] 日期输入无效!\n");
        return;
    }

    printf("[Client] 请输入开始时间(HH:MM:SS): ");
    if (scanf("%31s", szStartClock) != 1)
    {
        printf("[Client] 开始时间输入无效!\n");
        return;
    }

    printf("[Client] 请输入结束时间(HH:MM:SS): ");
    if (scanf("%31s", szEndClock) != 1)
    {
        printf("[Client] 结束时间输入无效!\n");
        return;
    }

    stInfo.dwChannel = nChannel;
    stInfo.bFilterByEventType = (nFilterByEventType != 0) ? TRUE : FALSE;
    stInfo.dwEventType = stInfo.bFilterByEventType ? nEventType : 100;
    strncpy(stInfo.szDate, szDate, sizeof(stInfo.szDate) - 1);
    snprintf(stInfo.szStartTime, sizeof(stInfo.szStartTime), "%s %s", szDate, szStartClock);
    snprintf(stInfo.szEndTime, sizeof(stInfo.szEndTime), "%s %s", szDate, szEndClock);

    INT32 dwBytesReturned = 0;
    BOOL bRet = NET_TV_GetReplayRecordList(g_lpUserID, &stInfo, &dwBytesReturned);

    if (bRet)
    {
        printf("[Client] Get replay record list success! BytesReturned=%d\n", dwBytesReturned);
        PrintReplayRecordList(&stInfo);
    }
    else
    {
        printf("[Client] Get replay record list failed! Error=%d\n", NET_TV_GetLastError());
    }
}

/* 获取人脸抓拍配置 */
static void DoGetFaceCaptureInfo()
{
    NET_TV_FACE_CAPTURE_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_FACECAPTUREINFO,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 获取人脸抓拍配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintFaceCaptureInfo(&stInfo);
    }
    else
    {
        printf("[Client] 获取人脸抓拍配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

/* 设置人脸抓拍配置 */
static void DoSetFaceCaptureInfo()
{
    NET_TV_FACE_CAPTURE_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    BOOL bRetGet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_FACECAPTUREINFO,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (!bRetGet)
    {
        memset(&stInfo, 0, sizeof(stInfo));
    }

    stInfo.bEnable = TRUE;
    stInfo.stRule.nSensitivity = 65;
    stInfo.stRule.stRegion.dwPointCount = 4;
    FillDemoPolygon4(stInfo.stRule.stRegion.afPointX, stInfo.stRule.stRegion.afPointY);

    stInfo.stRule.dwShieldRegionCount = 1;
    stInfo.stRule.astShieldRegion[0].dwPointCount = 4;
    stInfo.stRule.astShieldRegion[0].afPointX[0] = 0.45f; stInfo.stRule.astShieldRegion[0].afPointY[0] = 0.45f;
    stInfo.stRule.astShieldRegion[0].afPointX[1] = 0.55f; stInfo.stRule.astShieldRegion[0].afPointY[1] = 0.45f;
    stInfo.stRule.astShieldRegion[0].afPointX[2] = 0.55f; stInfo.stRule.astShieldRegion[0].afPointY[2] = 0.55f;
    stInfo.stRule.astShieldRegion[0].afPointX[3] = 0.45f; stInfo.stRule.astShieldRegion[0].afPointY[3] = 0.55f;

    stInfo.stRule.nMinIpdRectLeft = 10;
    stInfo.stRule.nMinIpdRectTop = 10;
    stInfo.stRule.nMinIpdRectRight = 90;
    stInfo.stRule.nMinIpdRectBottom = 90;
    stInfo.stRule.nMinWidth = 20;
    stInfo.stRule.nMinHeight = 20;
    stInfo.stRule.nMaxWidth = 300;
    stInfo.stRule.nMaxHeight = 300;
    stInfo.stRule.nInterval = 2;

    INT32 dwBytesReturnedSet = 0;
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_FACECAPTUREINFO,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturnedSet
    );

    if (bRet)
    {
        printf("[Client] 设置人脸抓拍配置成功! BytesReturned=%d\n", dwBytesReturnedSet);
        DoGetFaceCaptureInfo();
    }
    else
    {
        printf("[Client] 设置人脸抓拍配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void PrintFaceCompareInfo(const NET_TV_FACE_COMPARE_INFO_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 人脸比对配置 =====\n");
    printf("  Enable              : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  SuccessSnapshotCount: %d\n", pInfo->stLinkageListSuccess.dwSnapshotChannelCount);
    printf("  FailSnapshotCount   : %d\n", pInfo->stLinkageListFail.dwSnapshotChannelCount);
    printf("=================================\n");
}

static void DoSetFaceCompareInfo()
{
    NET_TV_FACE_COMPARE_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    stInfo.bEnable = TRUE;
    FillSimpleAiDemoSchedule(&stInfo.stAlarmSchedule);
    stInfo.stLinkageListSuccess.dwSnapshotChannelCount = 1;
    stInfo.stLinkageListSuccess.adwSnapshotChannel[0] = 1;
    stInfo.stLinkageListFail.dwSnapshotChannelCount = 1;
    stInfo.stLinkageListFail.adwSnapshotChannel[0] = 1;

    printf("[Client] 调用 NET_TV_SetDevConfig 设置人脸比对配置...\n");
    INT32 dwBytesReturned = 0;
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_FACE_COMPARE_INFO,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 设置人脸比对配置成功! BytesReturned=%d\n", dwBytesReturned);
        PrintFaceCompareInfo(&stInfo);
    }
    else
    {
        printf("[Client] 设置人脸比对配置失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void FillDemoFaceLib(NET_TV_FACE_LIB_INFO_S* pInfo, const char* szName)
{
    if (!pInfo)
    {
        return;
    }

    memset(pInfo, 0, sizeof(*pInfo));
    strncpy(pInfo->szFaceLibName, szName, sizeof(pInfo->szFaceLibName) - 1);
    pInfo->nTotalFace = 1;
    pInfo->nNormalNum = 1;
    pInfo->nAbnormalNum = 0;
}

static void PrintFaceLibInfo(const NET_TV_FACE_LIB_INFO_S* pInfo, INT32 nIndex)
{
    if (!pInfo)
    {
        return;
    }

    printf("  Lib[%d] LibId=%s, TotalFace=%d, NormalNum=%d, AbnormalNum=%d\n",
           nIndex,
           pInfo->szFaceLibName,
           pInfo->nTotalFace,
           pInfo->nNormalNum,
           pInfo->nAbnormalNum);
}

static void DoGetTargetLib()
{
    NET_TV_FACE_LIB_LIST_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_GetDevConfig 获取目标库...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_TARGET_LIB,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 获取目标库成功! BytesReturned=%d, Count=%d\n", dwBytesReturned, stInfo.nTargetLibCount);
        for (INT32 i = 0; i < stInfo.nTargetLibCount && i < NET_TV_FACE_LIB_MAX_NUM; ++i)
        {
            PrintFaceLibInfo(&stInfo.astTargetLibInfos[i], i);
        }
    }
    else
    {
        printf("[Client] 获取目标库失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoAddTargetLib()
{
    NET_TV_FACE_LIB_INFO_S stInfo;
    FillDemoFaceLib(&stInfo, "demo_library");

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_SetDevConfig 添加目标库...\n");
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_ADD_TARGET_LIB,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 添加目标库成功! BytesReturned=%d\n", dwBytesReturned);
        DoGetTargetLib();
    }
    else
    {
        printf("[Client] 添加目标库失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoDelTargetLib()
{
    NET_TV_FACE_LIB_INFO_S stInfo;
    FillDemoFaceLib(&stInfo, "demo_library");

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_SetDevConfig 删除目标库...\n");
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_DEL_TARGET_LIB,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 删除目标库成功! BytesReturned=%d\n", dwBytesReturned);
        DoGetTargetLib();
    }
    else
    {
        printf("[Client] 删除目标库失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoSetTargetLib()
{
    NET_TV_FACE_LIB_INFO_S stInfo;
    FillDemoFaceLib(&stInfo, "demo_library");
    stInfo.nTotalFace = 2;
    stInfo.nNormalNum = 2;

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_SetDevConfig 修改目标库...\n");
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_TARGET_LIB,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 修改目标库成功! BytesReturned=%d\n", dwBytesReturned);
        DoGetTargetLib();
    }
    else
    {
        printf("[Client] 修改目标库失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void FillDemoFaceInfo(NET_TV_FACE_INFO_S* pInfo, INT32 nId, const char* szName)
{
    if (!pInfo)
    {
        return;
    }

    memset(pInfo, 0, sizeof(*pInfo));
    pInfo->nId = nId;
    strncpy(pInfo->szFaceLibName, "demo_library", sizeof(pInfo->szFaceLibName) - 1);
    strncpy(pInfo->szName, szName, sizeof(pInfo->szName) - 1);
    strncpy(pInfo->szPhoneNum, "13800000001", sizeof(pInfo->szPhoneNum) - 1);
    strncpy(pInfo->szPicPath, "/tmp/demo_face.jpg", sizeof(pInfo->szPicPath) - 1);
    strncpy(pInfo->szBinPath, "/tmp/demo_face.bin", sizeof(pInfo->szBinPath) - 1);
    strncpy(pInfo->szPicType, "jpg", sizeof(pInfo->szPicType) - 1);
    pInfo->nPicSize = 2048;
    strncpy(pInfo->szPicDate, "2026-05-07 11:00:00", sizeof(pInfo->szPicDate) - 1);
    pInfo->nModelState = 1;
    pInfo->nRatingLevel = 3;
}

static void PrintFaceInfo(const NET_TV_FACE_INFO_S* pInfo, INT32 nIndex)
{
    if (!pInfo)
    {
        return;
    }

    printf("  Face[%d] Id=%d, LibId=%s, Name=%s, Phone=%s, PicPath=%s, BinPath=%s, ModelState=%d, RatingLevel=%d\n",
           nIndex,
           pInfo->nId,
           pInfo->szFaceLibName,
           pInfo->szName,
           pInfo->szPhoneNum,
           pInfo->szPicPath,
           pInfo->szBinPath,
           pInfo->nModelState,
           pInfo->nRatingLevel);
}

static void DoGetFaceInfo()
{
    NET_TV_FACE_INFO_LIST_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_GetDevConfig 获取人脸...\n");
    BOOL bRet = NET_TV_GetDevConfig(
        g_lpUserID, 1, NET_TV_GET_FACE_INFO,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 获取人脸成功! BytesReturned=%d, Count=%d\n", dwBytesReturned, stInfo.nFaceInfoCount);
        for (INT32 i = 0; i < stInfo.nFaceInfoCount && i < NET_TV_FACE_INFO_MAX_NUM; ++i)
        {
            PrintFaceInfo(&stInfo.astFaceInfos[i], i);
        }
    }
    else
    {
        printf("[Client] 获取人脸失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoAddFaceInfo()
{
    NET_TV_FACE_INFO_S stInfo;
    FillDemoFaceInfo(&stInfo, 2, "demo_face");

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_SetDevConfig 添加人脸...\n");
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_ADD_FACE_INFO,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 添加人脸成功! BytesReturned=%d\n", dwBytesReturned);
        DoGetFaceInfo();
    }
    else
    {
        printf("[Client] 添加人脸失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoDelFaceInfo()
{
    NET_TV_FACE_ID_INFO_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));
    stInfo.nIdCount = 1;
    stInfo.anIds[0] = 2;

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_SetDevConfig 删除人脸...\n");
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_DEL_FACE_INFO,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 删除人脸成功! BytesReturned=%d\n", dwBytesReturned);
        DoGetFaceInfo();
    }
    else
    {
        printf("[Client] 删除人脸失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void DoSetFaceInfo()
{
    NET_TV_FACE_INFO_S stInfo;
    FillDemoFaceInfo(&stInfo, 2, "demo_face_updated");
    stInfo.nPicSize = 4096;

    INT32 dwBytesReturned = 0;
    printf("[Client] 调用 NET_TV_SetDevConfig 修改人脸...\n");
    BOOL bRet = NET_TV_SetDevConfig(
        g_lpUserID, 1, NET_TV_SET_FACE_INFO,
        &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned
    );

    if (bRet)
    {
        printf("[Client] 修改人脸成功! BytesReturned=%d\n", dwBytesReturned);
        DoGetFaceInfo();
    }
    else
    {
        printf("[Client] 修改人脸失败! Error=%d\n", NET_TV_GetLastError());
    }
}

static void ProcessCommand(int cmd)
{
    switch (cmd)
    {
        case 1:
            DoGetDeviceCfg();
            break;
        case 2:
            DoSetDeviceCfg();
            break;
        case 3:
            DoGetNetworkCfg();
            break;
        case 4:
            DoSetNetworkCfg();
            break;
        case 5:
            DoGetMotionAlarm();
            break;
        case 6:
            DoSetMotionAlarm();
            break;
        case 7:
            DoGetTamperAlarm();
            break;
        case 8:
            DoSetTamperAlarm();
            break;
        case 9:
            DoGetCrossLineAlarm();
            break;
        case 10:
            DoSetCrossLineAlarm();
            break;
        case 11:
            DoGetIntrusionAlarm();
            break;
        case 12:
            DoSetIntrusionAlarm();
            break;
        case 13:
            DoGetRtspUrl();
            break;
        case 14:
            DoGetLoiteringAlarm();
            break;
        case 15:
            DoSetLoiteringAlarm();
            break;
        case 16:
            DoGetOSDCapCfg();
            break;
        case 17:
            DoSetOSDCapCfg();
            break;
        case 18:
            DoSetUpgradeCfg();
            break;
        case 19:
            DoGetUpgradeVersionCfg();
            break;
        case 20:
            DoGetUpgradeStatusCfg();
            break;
        case 21:
            DoGetCapturePlanCfg();
            break;
        case 22:
            DoSetCapturePlanCfg();
            break;
        case 23:
            DoGetCaptureParamCfg();
            break;
        case 24:
            DoSetCaptureParamCfg();
            break;
        case 25:
            DoGetExposureCfg();
            break;
        case 26:
            DoSetExposureCfg();
            break;
        case 27:
            DoGetDayNightCfg();
            break;
        case 28:
            DoSetDayNightCfg();
            break;
        case 29:
            DoGetBackLightCfg();
            break;
        case 30:
            DoSetBackLightCfg();
            break;
        case 31:
            DoGetDenoiseCfg();
            break;
        case 32:
            DoSetDenoiseCfg();
            break;
        case 33:
            DoGetWhiteBalanceCfg();
            break;
        case 34:
            DoSetWhiteBalanceCfg();
            break;    
        case 35:
            DoGetAudioAnomalyAlarm();
            break;
        case 36:
            DoSetAudioAnomalyAlarm();
            break;
        case 37:
            DoGetPreviewInfo();
            break;
        case 38:
            DoSetPreviewInfo();
            break;
        case 39:
            DoGetSceneChangeAlarm();
            break;
        case 40:
            DoSetSceneChangeAlarm();
            break;
        case 41:
            DoGetCrowdGatheringAlarm();
            break;
        case 42:
            DoSetCrowdGatheringAlarm();
            break;
        case 43:
            DoGetGarbageExposureCfg();
            break;
        case 44:
            DoSetGarbageExposureCfg();
            break;
        case 45:
            DoGetGarbageOverflowCfg();
            break;
        case 46:
            DoSetGarbageOverflowCfg();
            break;
        case 47:
            DoGetPeopleFlowStatisticsCfg();
            break;
        case 48:
            DoSetPeopleFlowStatisticsCfg();
            break;
        case 49:
            DoResetPeopleFlowStatistics();
            break;
        case 50:
            DoGetPeopleDensityDetectionCfg();
            break;
        case 51:
            DoSetPeopleDensityDetectionCfg();
            break;
        case 52:
            DoGetParkingAlarm();
            break;            
        case 53:
            DoSetTalkbackState();
            break;
        case 54:
            DoSetTalkbackToStream();
            break;
        case 55:
            DoGetTalkbackFromStream();
            break;
        case 56:
            DoSetReplayTalkback();
            break;
        case 57:
            DoSetConfigWifiSta();
            break;
        case 58:
            DoConnectWifiSta();
            break;
        case 59:
            DoDisconnectWifiSta();
            break;
        case 60:
            DoGet4GInfo();
            break;
        case 61:
            DoSet4GInfo();
            break;
        case 62:
            DoSetHotspotInfo();
            break;
        case 63:
            DoGetAudioCfg();
            break;
        case 64:
            DoSetAudioCfg();
            break;
        case 65:
            DoGetEnterRegionAlarm();
            break;
        case 66:
            DoSetEnterRegionAlarm();
            break;
        case 67:
            DoGetLeaveRegionAlarm();
            break;
        case 68:
            DoSetLeaveRegionAlarm();
            break;
        case 69:
            DoGetFaceCaptureInfo();
            break;
        case 70:
            DoSetFaceCaptureInfo();
            break;
        case 71:
            DoSetParkingAlarm();
            break;
        case 72:
            DoGetUnattendedObjectAlarm();
            break;
        case 73:
            DoSetUnattendedObjectAlarm();
            break;
        case 74:
            DoGetObjectRemovalAlarm();
            break;
        case 75:
            DoSetObjectRemovalAlarm();
            break;
        case 76:
            DoGetChannelInfo();
            break;
        case 77:
            DoGetChannelList();
            break;
        case 78:
            DoGetManholeCoverAbnormalCfg();
            break;
        case 79:
            DoSetManholeCoverAbnormalCfg();
            break;
        case 80:
            DoGetSleepOnDutyCfg();
            break;
        case 81:
            DoSetSleepOnDutyCfg();
            break;
        case 82:
            DoGetElectricVehicleInElevatorCfg();
            break;
        case 83:
            DoSetElectricVehicleInElevatorCfg();
            break;
        case 84:
            DoGetPersonFallDownCfg();
            break;
        case 85:
            DoSetPersonFallDownCfg();
            break;
        case 86:
            DoGetConstructionOccupyRoadCfg();
            break;
        case 87:
            DoSetConstructionOccupyRoadCfg();
            break;
        case 88:
            DoGetCongestionCfg();
            break;
        case 89:
            DoSetCongestionCfg();
            break;
        case 90:
            DoGetLicensePlateRecognitionCfg();
            break;
        case 91:
            DoSetLicensePlateRecognitionCfg();
            break;
        case 92:
            DoGetHighAltitudeSeatbeltCfg();
            break;
        case 93:
            DoSetHighAltitudeSeatbeltCfg();
            break;
        case 94:
            DoGetSafetyHelmetCfg();
            break;
        case 95:
            DoSetSafetyHelmetCfg();
            break;
        case 96:
            DoGetPersonFallCfg();
            break;
        case 97:
            DoSetPersonFallCfg();
            break;
        case 98:
            DoGetPhoneUsageCfg();
            break;
        case 99:
            DoSetPhoneUsageCfg();
            break;
        case 100:
            DoGetSmokingCfg();
            break;
        case 101:
            DoSetSmokingCfg();
            break;
        case 102:
            DoGetOpenFlameCfg();
            break;
        case 103:
            DoSetOpenFlameCfg();
            break;
        case 104:
            DoGetBareSoilCfg();
            break;
        case 105:
            DoSetBareSoilCfg();
            break;
        case 106:
            DoGetHoleProtectionBarCfg();
            break;
        case 107:
            DoSetHoleProtectionBarCfg();
            break;
        case 108:
            DoGetReflectiveClothingCfg();
            break;
        case 109:
            DoSetReflectiveClothingCfg();
            break;
        case 110:
            DoGetPetRecognitionInfo();
            break;
        case 111:
            DoSetPetRecognitionInfo();
            break;
        case 112:
            DoGetClimbFenceInfo();
            break;
        case 113:
            DoSetClimbFenceInfo();
            break;
        case 114:
            DoGetDimissionInfo();
            break;
        case 115:
            DoSetDimissionInfo();
            break;
        case 116:
            DoGetIllegalLaneInfo();
            break;
        case 117:
            DoSetIllegalLaneInfo();
            break;
        case 118:
            DoGetRetrogradeInfo();
            break;
        case 119:
            DoSetRetrogradeInfo();
            break;
        case 120:
            DoGetNonmotorVehicleIntrusionInfo();
            break;
        case 121:
            DoSetNonmotorVehicleIntrusionInfo();
            break;
        case 122:
            DoGetOccupationEmergencyInfo();
            break;
        case 123:
            DoSetOccupationEmergencyInfo();
            break;
        case 124:
            DoGetPedestrianIntrusionInfo();
            break;
        case 125:
            DoSetPedestrianIntrusionInfo();
            break;
        case 126:
            DoGetSmokeFireCfg();
            break;
        case 127:
            DoSetSmokeFireCfg();
            break;
        case 128:
            DoGetRoadPondingCfg();
            break;
        case 129:
            DoSetRoadPondingCfg();
            break;
        case 130:
            DoGetStreamCfg();
            break;
        case 131:
            DoSetStreamCfg();
            break;
        case 132:
            DoGetHotspotConn();
            break;
        case 133:
            DoGetSecurityServicesInfo();
            break;
        case 134:
            DoSetSecurityServicesInfo();
            break;
        case 135:
            DoGetSshCountdown();
            break;
        case 136:
            DoFindLog();
            break;
        case 137:
            DoExportLog();
            break;
        case 138:
            DoGetLogServer();
            break;
        case 139:
            DoSetLogServer();
            break;
        case 140:
            DoTestLogServer();
            break;
        case 141:
            DoGetReplayUrl();
            break;
        case 142:
            DoControlReplayStart();
            break;
        case DEMO_AC_PLATFORM_PLAY:
            DoReplayControlCustomMenu();
            break;
        case 143:
            DoControlReplayStop();
            break;
        case 144:
            DoControlReplaySpeed();
            break;
        case 145:
            DoGetReplayRecordList();
            break;
        case 146:
            DoControlRecordInfo(NET_TV_RECORD_STATUS_RECORDING);
            break;
        case 147:
            DoControlRecordInfo(NET_TV_RECORD_STATUS_STOP);
            break;
        case 148:
            DoGetRecordStatus();
            break;
        case 149:
            DoGetRecordSchedule();
            break;
        case 150:
            DoSetRecordSchedule();
            break;
        case 151:
            DoGetRecordAdvancedParam();
            break;
        case 152:
            DoSetRecordAdvancedParam();
            break;
        case 153:
            DoFindRecordFileInfo();
            break;
        case 154:
            DoDownloadRecordFile();
            break;
        case 155:
            DoSetFaceCompareInfo();
            break;
        case 156:
            DoAddTargetLib();
            break;
        case 157:
            DoDelTargetLib();
            break;
        case 158:
            DoSetTargetLib();
            break;
        case 159:
            DoGetTargetLib();
            break;
        case 160:
            DoAddFaceInfo();
            break;
        case 161:
            DoDelFaceInfo();
            break;
        case 162:
            DoSetFaceInfo();
            break;
        case 163:
            DoGetFaceInfo();
            break;
        case 164:
            DoGetPrivacyMaskCfg();
            break;
        case 165:
            DoSetPrivacyMaskCfg();
            break;
        case 166:
            DoTestOSDCapCfg();
            break;
        case DEMO_REPLAY_PAUSE_CMD:
            DoControlReplayPause();
            break;
        case DEMO_REPLAY_RESUME_CMD:
            DoControlReplayResume();
            break;
        default:
            printf("[Client] 无效的命令码: %d\n", cmd);
            break;
    }
}

int main(int argc, char* argv[])
{
    printf("=============== SDK Client Config Demo ================\n");
    ConfigureByArgs(argc, argv);

    /* 初始化日志 */
    initSdkLogBySize("ConfigClientDemo", "/tmp/ConfigClientDemo.log", MAX_LOG_SIZE, MAX_LOG_FILES);
    syncPrintf(1);
    setLogLevel(NETSDK_LOG_TRACE);

    /* 初始化 SDK */
    printf("[Client] Initializing SDK...\n");
    if (!NET_TV_Init())
    {
        printf("[Client] NET_TV_Init FAILED!\n");
        return -1;
    }
    printf("[Client] SDK initialized.\n");

    /* 登录设备 */
    NET_TV_DEVICE_LOGIN_INFO_S struLoginInfo;
    NET_TV_DEVICE_INFO_S       struDeviceInfo;
    memset(&struLoginInfo, 0, sizeof(struLoginInfo));
    memset(&struDeviceInfo, 0, sizeof(struDeviceInfo));

    struLoginInfo.dwPort = g_serverPort;
    strncpy(struLoginInfo.szIPAddr,  g_serverIp, sizeof(struLoginInfo.szIPAddr) - 1);
    strncpy(struLoginInfo.szUserName,g_username, sizeof(struLoginInfo.szUserName) - 1);
    strncpy(struLoginInfo.szPassword,g_password, sizeof(struLoginInfo.szPassword) - 1);

    printf("[Client] Logging in to %s:%d, username=%s...\n",
           g_serverIp,
           g_serverPort,
           g_username);
    g_lpUserID = NET_TV_Login(&struLoginInfo, &struDeviceInfo);
    if (!g_lpUserID)
    {
        printf("[Client] Login FAILED! Error=%d\n", NET_TV_GetLastError());
        NET_TV_Cleanup();
        return -1;
    }
    printf("[Client] Login SUCCESS! UserID=%p\n", g_lpUserID);

    /* 主循环 - 处理用户输入 */
    int cmd = -1;
    while (1)
    {
        PrintMenu();

        if (scanf("%d", &cmd) != 1)
        {
            int c;
            while ((c = getchar()) != '\n' && c != EOF)
                ;
            printf("[Client] 输入无效，请输入数字!\n");
            continue;
        }

        if (cmd == 0)
        {
            printf("[Client] 退出程序...\n");
            break;
        }

        ProcessCommand(cmd);
    }

    /* 登出与清理 */
    if (g_lpUserID)
    {
        NET_TV_Logout(g_lpUserID);
        printf("[Client] Logged out.\n");
    }

    NET_TV_Cleanup();
    printf("[Client] SDK cleaned up. Bye!\n");

    return 0;
}

