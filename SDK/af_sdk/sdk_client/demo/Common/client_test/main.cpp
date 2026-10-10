/**
 * @file main.cpp
 * @brief SDK 客户端接口综合测试 Demo
 *        支持多客户端并发接入，逐项测试 sdk_client 对外暴露的全部接口
 *        覆盖: 基础/会话/报警监听/设备控制/能力集/配置查询/回放/录像帧流/语音对讲
 *
 * 编译命令（在 af_sdk/build 目录下执行）：
 *   ./build.sh demo_client client_test --autofile
 *
 * 用法：
 *   ./ClientTestDemo [server_ip] [port] [username] [password]
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <unistd.h>
#include <csetjmp>

#include "NetTVSDKClientInterface.h"
#include "web_server.h"

/* Convert 头文件必须在 SDKConvert.h 之前包含 */
#include "AlarmInfoConvert.h"
#include "CapabilityInfoConvert.h"
#include "DeviceInfoConvert.h"
#include "IpcInfoConvert.h"
#include "NvrInfoConvert.h"
#include "BG6_ZHSJ/BU_SJCL/RecordInfoConvert.h"
#include "BG6_ZHSJ/BU_SJLB/RecordInfoConvert.h"
#include "SDKConvert.h"

/* ===================== 常量定义 ===================== */
#define MAX_CLIENTS     4
#define MAX_DEVICES     64

static WebServer g_webServer;  /* 嵌入式 Web 服务器实例 */

#define SERVER_IP       "172.16.25.213"
#define SERVER_PORT     9019
#define USERNAME        "admin"
#define PASSWORD        "itc20232024"

/* ===================== 全局状态 ===================== */
static char  g_serverIp[64]   = SERVER_IP;
static INT32 g_serverPort     = SERVER_PORT;
static char  g_username[64]   = USERNAME;
static char  g_password[64]   = PASSWORD;

static LPVOID g_lpUserID[MAX_CLIENTS] = {NULL};
static NET_DeviceInfo_S g_devInfo[MAX_CLIENTS];
static bool  g_bSdkInit   = false;
static bool  g_bLogInit   = false;  /* 日志文件已设置标记, 重复设置会崩溃 */
static bool  g_bWebMode    = false;   /* Web 模式: 命令由网页按钮驱动, 参数由网页输入经 stdin 管道到达 */

/* 流媒体/回放会话状态 */
static char  g_replaySessionId[NET_REPLAY_SESSION_ID_LEN] = ""; /* 最近一次回放会话ID */
static INT32 g_replayChannel = 0;                               /* 最近一次回放所在通道, 2~6 控制命令默认沿用 */
static char  g_recordStreamId[NET_STREAM_ID_LEN] = "";          /* 录像帧流ID */
static int   g_recordStreamSlot   = -1;                          /* 录像帧流所在客户端槽位 */
static int   g_voiceComSlot       = -1;                          /* 语音对讲所在客户端槽位 */
static UINT32 g_voiceComFrameBytes = 0;                          /* 对讲单帧字节数 */
static volatile unsigned g_recordFrameCount = 0;                 /* 录像帧累计数量 */
static volatile unsigned g_voiceRecvBytes   = 0;                 /* 对讲接收累计字节 */
static int   g_lastLoginSlot   = -1;                              /* Web模式: 最近一次成功登录的槽位 */

static jmp_buf g_quitJmp;                                         /* quit 回跳点 */

/* ===================== 辅助函数 ===================== */
static void SafeCopy(char* pDst, size_t dstSize, const char* pSrc)
{
    if (!pDst || dstSize == 0) return;
    pDst[0] = '\0';
    if (!pSrc) return;
    strncpy(pDst, pSrc, dstSize - 1);
    pDst[dstSize - 1] = '\0';
}

static void ClearInputBuf()
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

static int ReadInt(const char* prompt)
{
    char lineBuf[256];
    printf("%s", prompt);
    fflush(stdout);
    if (!fgets(lineBuf, sizeof(lineBuf), stdin)) return -1;
    size_t len = strlen(lineBuf);
    if (len > 0 && lineBuf[len - 1] == '\n') lineBuf[--len] = '\0';
    if (strcmp(lineBuf, "quit") == 0) {
        printf("\n[中断] 用户输入 quit, 中止当前测试\n");
        fflush(stdout);
        longjmp(g_quitJmp, 1);
    }
    if (strcmp(lineBuf, "clear") == 0) {
        printf("\n[CLEAR]\n");
        fflush(stdout);
        longjmp(g_quitJmp, 1);
    }
    int val = 0;
    if (sscanf(lineBuf, "%d", &val) != 1) return -1;
    return val;
}

static void ReadString(const char* prompt, char* buf, size_t size)
{
    printf("%s", prompt);
    fflush(stdout);
    if (fgets(buf, (int)size, stdin)) {
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') buf[--len] = '\0';
        if (strcmp(buf, "quit") == 0) {
            printf("\n[中断] 用户输入 quit, 中止当前测试\n");
            fflush(stdout);
            longjmp(g_quitJmp, 1);
        }
        if (strcmp(buf, "clear") == 0) {
            printf("\n[CLEAR]\n");
            fflush(stdout);
            longjmp(g_quitJmp, 1);
        }
    }
}

/* 带默认值的字符串输入: 直接回车采用默认值 */
static void ReadStringDefault(const char* prompt, char* buf, size_t size, const char* def)
{
    printf("%s [%s]: ", prompt, def ? def : "");
    fflush(stdout);
    char tmp[256];
    if (fgets(tmp, sizeof(tmp), stdin) && tmp[0] != '\n') {
        size_t len = strlen(tmp);
        if (len > 0 && tmp[len - 1] == '\n') tmp[len - 1] = '\0';
        if (strcmp(tmp, "quit") == 0) {
            printf("\n[中断] 用户输入 quit, 中止当前测试\n");
            fflush(stdout);
            longjmp(g_quitJmp, 1);
        }
        if (strcmp(tmp, "clear") == 0) {
            printf("\n[CLEAR]\n");
            fflush(stdout);
            longjmp(g_quitJmp, 1);
        }
        SafeCopy(buf, size, tmp);
    } else {
        SafeCopy(buf, size, def);
    }
}

/* 以今天日期生成默认起止时间 "YYYY-MM-DD HH:MM:SS" */
static void GetDefaultTimeRange(char* pStart, size_t startSize, char* pEnd, size_t endSize)
{
    time_t now = time(NULL);
    struct tm tmNow = *localtime(&now);
    if (pStart) strftime(pStart, startSize, "%Y-%m-%d 00:00:00", &tmNow);
    if (pEnd)   strftime(pEnd, endSize, "%Y-%m-%d 23:59:59", &tmNow);
}

/* 当天秒数打印为 HH:MM:SS 时间段 */
static void PrintSecRange(INT32 nStart, INT32 nEnd)
{
    printf("%02d:%02d:%02d ~ %02d:%02d:%02d",
           nStart / 3600, (nStart % 3600) / 60, nStart % 60,
           nEnd / 3600, (nEnd % 3600) / 60, nEnd % 60);
}

/* 统一的失败打印 */
static void PrintFailure(const char* pTag)
{
    printf("%s 失败! Error=%d, Msg=%s\n", pTag,
           NET_clientGetLastError(), NET_clientGetErrorMsg());
}

/* 统一结构体打印: 测试接口/实际调用/输出结构体/JSON 四段输出 */
template <typename T>
static void PrintStructJson(const char* pTestFunc, const char* pApiCall,
                            const char* pStructName, T& stStruct)
{
    printf("测试接口：%s\n", pTestFunc);
    printf("实际调用：%s\n", pApiCall);
    printf("输出结构体：%s\n", pStructName);
    printf("%s\n", SDKConvert::to_string(stStruct).c_str());
}

/* Web 模式: 打印提示并从 stdin 读取一行 JSON 解析到结构体
 * 空行或解析失败时保持结构体当前值(即默认参数),
 * from_string 仅覆盖 JSON 中出现的字段 */
template <typename T>
static void ReadStructJson(T& stStruct)
{
    printf("等待输入Json数据：\n");
    fflush(stdout);
    char line[2048];
    if (!fgets(line, sizeof(line), stdin)) return;
    size_t len = strlen(line);
    if (len > 0 && line[len - 1] == '\n') line[--len] = '\0';
    if (strcmp(line, "quit") == 0) {
        printf("\n[中断] 用户输入 quit, 中止当前测试\n");
        fflush(stdout);
        longjmp(g_quitJmp, 1);
    }
    if (strcmp(line, "clear") == 0) {
        printf("\n[CLEAR]\n");
        fflush(stdout);
        longjmp(g_quitJmp, 1);
    }
    if (line[0] == '\0') return;  /* 空行: 使用默认参数 */
    if (!SDKConvert::from_string(std::string(line), stStruct)) {
        printf("[输入] JSON解析失败, 使用默认参数\n");
    }
}

/* 读取通道号参数: 直接输入数字, 空行/非法输入用 nDefault
 * 通道号不在结构体内时使用(由 NET_clientGetDevConfig/GetDeviceCapability 的 dwChannelID 参数携带):
 *   -1 (NET_API_PARAM_NVRCHN): 设备级列表查询, 如命令300全通道列表
 *   >=0: 通道级命令(能力集/音频配置/SD卡/录像状态等), 服务端对 channel<0 返回参数错误
 * Web/CLI 同一交互: 打印提示并等待一行输入(数字/空行=默认/quit=中止/clear=清屏) */
static INT32 ReadChannelParam(INT32 nDefault)
{
    printf("等待输入channel值(通道从0开始, 空行=%d)：\n", nDefault);
    fflush(stdout);
    char line[128];
    if (!fgets(line, sizeof(line), stdin)) return nDefault;
    size_t len = strlen(line);
    if (len > 0 && line[len - 1] == '\n') line[--len] = '\0';
    if (strcmp(line, "quit") == 0) {
        printf("\n[中断] 用户输入 quit, 中止当前测试\n");
        fflush(stdout);
        longjmp(g_quitJmp, 1);
    }
    if (strcmp(line, "clear") == 0) {
        printf("\n[CLEAR]\n");
        fflush(stdout);
        longjmp(g_quitJmp, 1);
    }
    if (line[0] == '\0') return nDefault;  /* 空行: 默认值 */
    int nVal = 0;
    if (sscanf(line, "%d", &nVal) == 1) {
        return (INT32)nVal;
    }
    printf("[输入] 非数字, 使用默认(%d)\n", nDefault);
    return nDefault;
}

/**
 * @brief 选择操作的客户端槽位
 * @return 槽位索引 [0, MAX_CLIENTS)，-1 表示取消
 */
static int SelectClient(const char* action)
{
    int active = 0;
    /* Web 模式: 优先使用最近登录的槽位, 避免选中已失效的旧 session */
    if (g_bWebMode) {
        if (g_lastLoginSlot >= 0 && g_lastLoginSlot < MAX_CLIENTS
            && g_lpUserID[g_lastLoginSlot])
            return g_lastLoginSlot;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (g_lpUserID[i]) return i;
        }
        printf("[跳过] 无已登录客户端, 请先登录\n");
        return -1;
    }
    printf("\n--- 选择客户端槽位 (%s) ---\n", action);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        const char* status = g_lpUserID[i] ? "已登录" : "空闲";
        printf("  [%d] 客户端%d (%s)\n", i, i + 1, status);
        if (g_lpUserID[i]) active++;
    }
    int idx = ReadInt("请输入槽位号 (0-3, -1取消): ");
    if (idx < 0 || idx >= MAX_CLIENTS) {
        printf("[取消]\n");
        return -1;
    }
    return idx;
}

/* ===================== 回调函数 ===================== */
static void NET_STDCALL ExceptionCallBack(LPVOID lpUserID, INT32 dwType,
                                           LPVOID lpExpHandle, LPVOID lpUserData)
{
    printf("\n[异常回调] UserID=%p, Type=0x%X, Handle=%p\n",
           lpUserID, dwType, lpExpHandle);
}

static void NET_STDCALL AlarmCallBack(INT64 lCommand, NET_Alarmer_S *pAlarmer,
                                       CHAR *pAlarmInfo, INT32 *dwBufLen, LPVOID lpUserData)
{
    printf("\n[报警回调] Cmd=%lld", (long long)lCommand);
    if (pAlarmer) {
        printf(", 设备=%s, 序列号=%s, IP=%s",
               pAlarmer->strDeviceName,
               (const char*)pAlarmer->strSerialNumber,
               pAlarmer->strDeviceIP);
    }
    if (pAlarmInfo && dwBufLen && *dwBufLen > 0) {
        INT32 len = (*dwBufLen > 256) ? 256 : *dwBufLen;
        printf("\n  报警数据(%d字节): %.*s", *dwBufLen, len, pAlarmInfo);
    }
    printf("\n");
}

static void NET_STDCALL ChannelStatusCallBack(NET_ChannelInfo_S *pChannelInfo, LPVOID lpUserData)
{
    if (!pChannelInfo) return;
    printf("\n[通道状态回调] 通道=%u, 启用=%s, 在线=%s, 名称=%s, 设备IP=%s\n",
           pChannelInfo->uChannel,
           pChannelInfo->byEnable ? "是" : "否",
           pChannelInfo->byOnline ? "是" : "否",
           pChannelInfo->szChannelName,
           pChannelInfo->szDeviceIP);
}

static void NET_STDCALL RecordFrameCallBack(const NET_RecordFrameInfo_S *pstFrameInfo,
                                            const CHAR *pData, UINT32 dwSize, LPVOID lpUserData)
{
    (void)pData; (void)lpUserData;
    if (!pstFrameInfo) return;
    g_recordFrameCount++;
    if (pstFrameInfo->uFlags & NET_RECORD_FRAME_FLAG_STREAM_END) {
        printf("[录像帧回调] 流结束包 (共收到 %u 帧)\n", g_recordFrameCount);
        return;
    }
    /* 每30帧打印一次，避免刷屏 */
    if ((g_recordFrameCount % 30) == 1) {
        printf("[录像帧回调] #%u 类型=%s, 编码=%s, 大小=%u, PTS=%llums\n",
               g_recordFrameCount,
               pstFrameInfo->uMediaType == NET_RECORD_FRAME_MEDIA_VIDEO ? "视频" : "音频",
               pstFrameInfo->uCodecType == NET_RECORD_FRAME_CODEC_H264 ? "H264" :
               pstFrameInfo->uCodecType == NET_RECORD_FRAME_CODEC_H265 ? "H265" : "其他",
               dwSize, (unsigned long long)pstFrameInfo->ullPtsMs);
    }
}

static void NET_STDCALL VoiceComCallBack(const char *data, unsigned int size, LPVOID lpUserData)
{
    (void)data; (void)lpUserData;
    g_voiceRecvBytes += size;
}

/* ===================== 测试接口实现 ===================== */

/** 1. 初始化SDK */
static void Test_Init()
{
    if (g_bSdkInit) {
        printf("[Init] SDK已经初始化过了\n");
        return;
    }
    printf("[Init] 正在初始化SDK...\n");
    BOOL ret = NET_clientInit();
    if (ret) {
        g_bSdkInit = true;
        printf("[Init] SDK初始化成功!\n");
    } else {
        printf("[Init] SDK初始化失败! Error=%d, Msg=%s\n",
               NET_clientGetLastError(), NET_clientGetErrorMsg());
    }
}

/** 2. 清理SDK (终态: 单例销毁后无法重新初始化, 清理后直接退出程序) */
static void Test_Cleanup()
{
    if (!g_bSdkInit) {
        printf("[Cleanup] SDK未初始化\n");
        return;
    }
    /* 先登出所有已登录客户端 */
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (g_lpUserID[i]) {
            if (g_voiceComSlot == i) {
                NET_clientStopVoiceCom(g_lpUserID[i]);
                g_voiceComSlot = -1;
            }
            if (g_recordStreamSlot == i && g_recordStreamId[0]) {
                NET_clientStopRecordFrameStream(g_lpUserID[i], g_recordStreamId);
                g_recordStreamId[0] = '\0';
                g_recordStreamSlot = -1;
            }
            NET_clientLogout(g_lpUserID[i]);
            printf("[Cleanup] 客户端%d 已登出\n", i + 1);
            g_lpUserID[i] = NULL;
        }
    }
    g_replaySessionId[0] = '\0';
    BOOL ret = NET_clientCleanup();
    g_bSdkInit = false;
    g_bLogInit = false;
    printf("[Cleanup] SDK资源清理 %s\n", ret ? "成功" : "失败");
    /* SDK单例销毁后无法重新初始化, 清理即终态: 提示后直接退出程序 */
    printf("请重新执行Demo测试脚本\n");
    fflush(stdout);
    if (g_bWebMode) sleep(2);  /* Web模式: 留时间让提示送达网页 */
    exit(0);
}

/** 3. 获取SDK版本 */
static void Test_GetVersion()
{
    INT32 ver = NET_clientGetSdkVersion();
    int major = (ver >> 24) & 0xFF;
    int minor = (ver >> 16) & 0xFF;
    int sub   = ver & 0xFFFF;
    printf("[版本] SDK版本: %d.%d.%d (0x%08X)\n", major, minor, sub, ver);
}

/** 4. 获取最后错误码 */
static void Test_GetLastError()
{
    INT32 err = NET_clientGetLastError();
    const char* msg = NET_clientGetErrorMsg();
    printf("[错误码] Code=%d, Msg=%s\n", err, msg ? msg : "(null)");
}

/** 5. 设置连接超时 */
static void Test_SetConnectTime()
{
    int waitTime  = ReadInt("等待时间(秒, 建议5): ");
    int tryTimes  = ReadInt("重试次数(建议3): ");
    BOOL ret = NET_clientSetConnectTime(waitTime, tryTimes);
    printf("[连接超时] 设置(%ds, %d次) %s\n", waitTime, tryTimes, ret ? "成功" : "失败");
}

/** 6. 设置接收超时 */
static void Test_SetRevTimeOut()
{
    int recvTimeout = ReadInt("接收超时(毫秒, 建议5000): ");
    NET_RevTimeout_S stTimeout;
    memset(&stTimeout, 0, sizeof(stTimeout));
    stTimeout.uRevTimeOut = recvTimeout;
    stTimeout.uFileReportTimeOut = 30000;
    BOOL ret = NET_clientSetRevTimeOut(&stTimeout);
    printf("[接收超时] 设置(%dms) %s\n", recvTimeout, ret ? "成功" : "失败");
}

/** 7. 设置日志 */
static void Test_SetLogToFile()
{
    int level = ReadInt("日志等级 (0-关闭, 1-ERROR, 2-DEBUG, 3-ALL): ");
    char dir[256] = "/opt/course/log";
    printf("日志目录 [%s]: ", dir);
    char input[256];
    if (fgets(input, sizeof(input), stdin) && input[0] != '\n') {
        size_t len = strlen(input);
        if (len > 0 && input[len-1] == '\n') input[len-1] = '\0';
        SafeCopy(dir, sizeof(dir), input);
    }
    if (g_bLogInit) {
        printf("[日志] 已经设置过, 重复设置会崩溃, 已跳过 (level=%d, dir=%s)\n", level, dir);
    } else {
        BOOL ret = NET_clientSetLogToFile(level, dir, 10 * 1024 * 1024, 5);
        if (ret) g_bLogInit = true;
        printf("[日志] 设置(level=%d, dir=%s) %s\n", level, dir, ret ? "成功" : "失败");
    }
}

/** 8. 设置异常回调 */
static void Test_SetExceptionCb()
{
    BOOL ret = NET_clientSetExceptionCallBack(ExceptionCallBack, NULL);
    printf("[异常回调] 注册 %s\n", ret ? "成功" : "失败");
}

/** 9. 用户登录 */
static void Test_Login()
{
    if (!g_bSdkInit) { printf("[Login] 请先初始化SDK (命令1)\n"); return; }

    int idx = -1;
    /* 找一个空闲槽位 */
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!g_lpUserID[i]) { idx = i; break; }
    }
    if (idx < 0) {
        printf("[Login] 所有%d个客户端槽位已满\n", MAX_CLIENTS);
        return;
    }

    char ip[64], user[64], pass[64];
    int port;
    SafeCopy(ip, sizeof(ip), g_serverIp);
    SafeCopy(user, sizeof(user), g_username);
    SafeCopy(pass, sizeof(pass), g_password);
    port = g_serverPort;

    printf("服务器IP [%s]: ", ip);
    char tmp[256];
    bool bCustomServer = false;
    if (fgets(tmp, sizeof(tmp), stdin) && tmp[0] != '\n') {
        size_t len = strlen(tmp); if (len > 0 && tmp[len-1] == '\n') tmp[len-1] = '\0';
        SafeCopy(ip, sizeof(ip), tmp);
        bCustomServer = true;
    }
    if (!bCustomServer) {
        /* IP 用默认值: 端口/用户名/密码全部用默认, 不再逐项询问 */
        printf("[Login] IP使用默认值, 端口/用户名/密码均使用默认值\n");
    } else {
        printf("端口号 [%d]: ", port);
        if (fgets(tmp, sizeof(tmp), stdin) && tmp[0] != '\n') {
            int p = atoi(tmp);
            if (p > 0) port = p;
        }
        printf("用户名 [%s]: ", user);
        if (fgets(tmp, sizeof(tmp), stdin) && tmp[0] != '\n') {
            size_t len = strlen(tmp); if (len > 0 && tmp[len-1] == '\n') tmp[len-1] = '\0';
            SafeCopy(user, sizeof(user), tmp);
        }
        printf("密码 [****]: ");
        if (fgets(tmp, sizeof(tmp), stdin) && tmp[0] != '\n') {
            size_t len = strlen(tmp); if (len > 0 && tmp[len-1] == '\n') tmp[len-1] = '\0';
            SafeCopy(pass, sizeof(pass), tmp);
        }
    }

    NET_DeviceLoginInfo_S loginInfo;
    memset(&loginInfo, 0, sizeof(loginInfo));
    SafeCopy(loginInfo.szIPAddr, sizeof(loginInfo.szIPAddr), ip);
    loginInfo.uPort = port;
    SafeCopy(loginInfo.szUserName, sizeof(loginInfo.szUserName), user);
    SafeCopy(loginInfo.szPassword, sizeof(loginInfo.szPassword), pass);

    memset(&g_devInfo[idx], 0, sizeof(NET_DeviceInfo_S));

    printf("[Login] 正在登录 %s:%d (用户=%s) -> 槽位%d ...\n", ip, port, user, idx + 1);
    printf("测试接口：Test_Login\n");
    LPVOID lpUser = NET_clientLogin(&loginInfo, &g_devInfo[idx]);
    if (lpUser) {
        g_lpUserID[idx] = lpUser;
        g_lastLoginSlot = idx;
        printf("[Login] 登录成功! UserID=%p, 槽位=%d\n", lpUser, idx + 1);
        PrintStructJson("Test_Login", "NET_clientLogin", "NET_DeviceInfo_S", g_devInfo[idx]);
    } else {
        INT32 err = NET_clientGetLastError();
        const char* msg = NET_clientGetErrorMsg();
        printf("实际调用：NET_clientLogin -> 失败\n");
        printf("返回NULL! Error=%d, Msg=%s\n", err, msg ? msg : "");
        if (err == NET_E_NO_USER) {
            printf("原因: 用户名不存在或密码错误\n");
        } else if (err == NET_E_CONNECT_ERROR || err == NET_E_SEND_MSG_ERROR) {
            printf("原因: 无法连接服务端, 请检查地址/端口及服务是否启动\n");
        } else {
            printf("[Login] 注意: NET_clientLogin 内部会先登录再获取设备信息,\n");
            printf("        如果登录成功但 GetDeviceInfo 失败, SDK 会自动登出并返回NULL.\n");
            printf("        请检查服务端是否注册并实现了 GetDeviceInfo 回调.\n");
            printf("        服务端路径: GET /TVAPI/V1.0/Device/GetInfo\n");
        }
    }
}

/** 10. 用户登出 */
static void Test_Logout()
{
    int idx = SelectClient("登出");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) {
        printf("[Logout] 客户端%d未登录\n", idx + 1);
        return;
    }
    /* 停止该客户端上的流媒体业务 */
    if (g_voiceComSlot == idx) {
        NET_clientStopVoiceCom(g_lpUserID[idx]);
        g_voiceComSlot = -1;
        printf("[Logout] 已停止语音对讲\n");
    }
    if (g_recordStreamSlot == idx && g_recordStreamId[0]) {
        NET_clientStopRecordFrameStream(g_lpUserID[idx], g_recordStreamId);
        g_recordStreamId[0] = '\0';
        g_recordStreamSlot = -1;
        printf("[Logout] 已停止录像帧流\n");
    }
    printf("测试接口：Test_Logout\n");
    BOOL ret = NET_clientLogout(g_lpUserID[idx]);
    printf("实际调用：NET_clientLogout -> %s\n", ret ? "成功" : "失败");
    if (ret) {
        printf("客户端%d 登出成功\n", idx + 1);
        g_lpUserID[idx] = NULL;
        if (g_lastLoginSlot == idx) g_lastLoginSlot = -1;
    } else {
        printf("[Logout] 客户端%d 登出失败! Error=%d\n", idx + 1, NET_clientGetLastError());
    }
}

/** 11. 设置重连功能 */
static void Test_SetAutoReconnect()
{
    int idx = SelectClient("设置重连");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) {
        printf("[Reconnect] 客户端%d未登录\n", idx + 1);
        return;
    }
    int enable = ReadInt("启用自动重连? (1=启用, 0=禁用): ");
    printf("测试接口：Test_SetAutoReconnect\n");
    BOOL ret = NET_clientSetAutoReconnect(g_lpUserID[idx], enable ? TRUE : FALSE);
    printf("实际调用：NET_clientSetAutoReconnect -> %s\n", ret ? "成功" : "失败");
    printf("客户端%d 自动重连=%s\n",
           idx + 1, enable ? "开启" : "关闭");
}

/** 12. 搜索局域网设备 */
static void Test_SearchDiscovery()
{
    printf("[Discovery] 搜索局域网设备(默认网卡, 超时3000ms)...\n");
    NET_DiscoveryDeviceInfo_S devices[MAX_DEVICES];
    int count = 0;

    printf("测试接口：Test_SearchDiscovery\n");
    BOOL ret = NET_clientSearchDiscovery(
        NULL,
        3000,
        devices, MAX_DEVICES, &count);
    printf("实际调用：NET_clientSearchDiscovery -> %s\n", ret ? "成功" : "失败");

    if (!ret) {
        printf("搜索失败! Error=%d, Msg=%s\n",
               NET_clientGetLastError(), NET_clientGetErrorMsg());
        return;
    }

    printf("[Discovery] 发现 %d 个设备:\n", count);
    for (int i = 0; i < count; i++) {
        const NET_DiscoveryDeviceInfo_S& d = devices[i];
        printf("\n  ┌─ 设备 #%d ──────────────────────────\n", i + 1);
        printf("  │ 名称:     %s\n", d.strDeviceName);
        printf("  │ ID:       %s\n", d.strDeviceID);
        printf("  │ 类型:     %s\n", d.strDeviceType);
        printf("  │ IP:       %s\n", d.strIPv4Address);
        printf("  │ 子网掩码: %s\n", d.strIPv4SubnetMask);
        printf("  │ 网关:     %s\n", d.strIPv4Gateway);
        printf("  │ MAC:      %s\n", d.strMACAddress);
        printf("  │ 固件:     %s\n", d.strFirmwareVersion);
        printf("  │ HTTP端口: %u\n", d.uHttpPort);
        printf("  │ 厂商:     %s\n", d.strManufacturer);
        printf("  └────────────────────────────────────\n");
    }
}

/** 13. 上传文件 (NET_clientUploadFile) */
static void Test_UploadFile()
{
    int idx = SelectClient("上传文件");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) {
        printf("[Upload] 客户端%d未登录\n", idx + 1);
        return;
    }

    char path[256] = "";
    ReadString("本地文件路径: ", path, sizeof(path));
    if (path[0] == '\0') {
        printf("[Upload] 未输入路径, 取消\n");
        return;
    }

    /* 从本地路径中提取文件名作为默认远程名 (支持 Windows '\' 和 Linux '/') */
    const char* pBase = strrchr(path, '/');
    const char* pBase2 = strrchr(path, '\\');
    if (pBase2 && (!pBase || pBase2 > pBase)) pBase = pBase2;
    char defaultRemote[128] = "";
    SafeCopy(defaultRemote, sizeof(defaultRemote), pBase ? pBase + 1 : path);

    char remote[128] = "";
    ReadStringDefault("上传后文件名", remote, sizeof(remote), defaultRemote);

    printf("[Upload] 正在上传 %s -> /opt/course/upload/%s ...\n", path, remote);
    printf("测试接口：Test_UploadFile\n");
    printf("实际调用：NET_clientUploadFile\n");
    BOOL ret = NET_clientUploadFile(g_lpUserID[idx], path, remote);
    printf("  -> %s\n", ret ? "成功" : "失败");
    if (ret) {
        printf("上传成功! 设备端路径: /opt/course/upload/%s\n", remote);
    } else {
        PrintFailure("[Upload] 上传");
    }
}

/** 14. 报警回调+开始监听 (SetAlarmCallBack + StartListen) */
static void Test_StartListen()
{
    int idx = SelectClient("报警监听");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) {
        printf("[Alarm] 客户端%d未登录\n", idx + 1);
        return;
    }

    printf("测试接口：Test_StartListen\n");
    printf("实际调用：NET_clientSetAlarmCallBack\n");
    BOOL ret = NET_clientSetAlarmCallBack(g_lpUserID[idx], AlarmCallBack, NULL);
    if (!ret) {
        printf("设置回调失败! Error=%d\n", NET_clientGetLastError());
        return;
    }
    printf("[Alarm] 报警回调注册成功\n");

    printf("实际调用：NET_clientStartListen\n");
    ret = NET_clientStartListen(g_lpUserID[idx]);
    printf("  -> %s\n", ret ? "成功" : "失败");
    if (ret) {
        printf("开始监听成功! 等待报警推送...\n");
    } else {
        PrintFailure("[Alarm] 开始监听");
    }
}

/** 15. 停止报警监听 (NET_clientStopListen) */
static void Test_StopListen()
{
    int idx = SelectClient("停止报警监听");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) {
        printf("[Alarm] 客户端%d未登录\n", idx + 1);
        return;
    }
    printf("测试接口：Test_StopListen\n");
    BOOL ret = NET_clientStopListen(g_lpUserID[idx]);
    printf("实际调用：NET_clientStopListen -> %s\n", ret ? "成功" : "失败");
}

/** 16. 通道状态回调 (NET_clientSetChannelStatusCallBack) */
static void Test_SetChannelStatusCb()
{
    int idx = SelectClient("通道状态回调");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) {
        printf("[Channel] 客户端%d未登录\n", idx + 1);
        return;
    }
    printf("测试接口：Test_SetChannelStatusCb\n");
    BOOL ret = NET_clientSetChannelStatusCallBack(g_lpUserID[idx], ChannelStatusCallBack, NULL);
    printf("实际调用：NET_clientSetChannelStatusCallBack -> %s\n", ret ? "成功" : "失败");
    if (ret) {
        printf("通道状态回调注册成功, 等待事件推送...\n");
    } else {
        PrintFailure("[Channel] 注册");
    }
}

/** 17. 设备控制 (NET_clientDeviceControl) */
static void Test_DeviceControl()
{
    int idx = SelectClient("设备控制");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) { printf("[DevCtrl] 客户端%d未登录\n", idx+1); return; }

    NET_DeviceControlInfo_S ctrl;
    memset(&ctrl, 0, sizeof(ctrl));
    ctrl.uSize = sizeof(ctrl);

    if (g_bWebMode) {
        /* Web 模式: 必要参数以 JSON 一次性输入 (字段为小写), 如 {"channel":0,"controlType":1,"command":1,"speed":3} */
        ReadStructJson(ctrl);
        if (ctrl.uControlType < 1 || ctrl.uControlType > 7) { printf("[取消] controlType无效\n"); return; }
    } else {
        printf("  控制类型:\n");
        printf("  [1] 云台控制  [2] 声光控制  [3] 雨刷控制\n");
        printf("  [4] 补光灯    [5] 继电器    [6] 重启设备  [7] 恢复出厂\n");
        int type = ReadInt("选择控制类型: ");
        if (type < 1 || type > 7) { printf("[取消]\n"); return; }
        ctrl.uControlType = type;
        ctrl.uChannelID = ReadInt("通道号(0=第一通道, 回车=-1设备级): ");

        switch (type) {
        case NET_DEVICE_CTRL_TYPE_PTZ:
            printf("  云台命令: 1上 2下 3左 4右 5左上 6左下 7右上 8右下\n");
            printf("           9变倍+ 10变倍- 11聚焦近 12聚焦远 15停止\n");
            ctrl.uCommand = ReadInt("选择命令: ");
            ctrl.uSpeed = ReadInt("速度(1-7, 默认3): ");
            if (ctrl.uSpeed <= 0) ctrl.uSpeed = 3;
            ctrl.uDurationMs = ReadInt("持续时间ms(0=默认): ");
            break;
        case NET_DEVICE_CTRL_TYPE_ALARM_LIGHT:
            printf("  1=开启声光 2=停止声光 3=设置模式\n");
            ctrl.uCommand = ReadInt("选择命令: ");
            break;
        case NET_DEVICE_CTRL_TYPE_REBOOT:
            ctrl.uCommand = 1;
            printf("[DevCtrl] 即将重启设备...\n");
            break;
        case NET_DEVICE_CTRL_TYPE_RESET:
            printf("  1=完全恢复(含IP) 2=简单恢复\n");
            ctrl.uCommand = ReadInt("选择重置类型: ");
            break;
        default:
            ctrl.uCommand = 1;
            ctrl.uParam1 = ReadInt("uParam1(默认0): ");
            break;
        }
    }

    printf("测试接口：Test_DeviceControl\n");
    BOOL ret = NET_clientDeviceControl(g_lpUserID[idx], &ctrl);
    printf("实际调用：NET_clientDeviceControl -> %s\n", ret ? "成功" : "失败");
    printf("类型=%d, 命令=%d\n", ctrl.uControlType, ctrl.uCommand);
    if (!ret) PrintFailure("[DevCtrl]");
}

/** 18. 视频编码能力集 (NET_CAP_VIDEO_ENCODE) */
static void Test_GetVideoCap()
{
    int idx = SelectClient("视频编码能力集");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) { printf("[VideoCap] 客户端%d未登录\n", idx+1); return; }

    /* 通道级命令(能力集), channel<0 服务端返回参数错误, 默认0 */
    INT32 nChannel = ReadChannelParam(0);
    NET_VideoEncodeCap_S cap;
    memset(&cap, 0, sizeof(cap));
    INT32 nRetBytes = 0;
    BOOL ret = NET_clientGetDeviceCapability(g_lpUserID[idx], nChannel, NET_CAP_VIDEO_ENCODE, &cap, sizeof(cap), &nRetBytes);
    if (!ret) { PrintFailure("[VideoCap] 查询"); return; }
    PrintStructJson("Test_GetVideoCap", "NET_clientGetDeviceCapability【NET_CAP_VIDEO_ENCODE=1】", "NET_VideoEncodeCap_S", cap);
}

/** 19. 音频能力集 (NET_CAP_AUDIO) */
static void Test_GetAudioCap()
{
    int idx = SelectClient("音频能力集");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) { printf("[AudioCap] 客户端%d未登录\n", idx+1); return; }

    /* 通道级命令(能力集), channel<0 服务端返回参数错误, 默认0 */
    INT32 nChannel = ReadChannelParam(0);
    NET_AudioCap_S cap;
    memset(&cap, 0, sizeof(cap));
    INT32 nRetBytes = 0;
    BOOL ret = NET_clientGetDeviceCapability(g_lpUserID[idx], nChannel, NET_CAP_AUDIO, &cap, sizeof(cap), &nRetBytes);
    if (!ret) { PrintFailure("[AudioCap] 查询"); return; }
    PrintStructJson("Test_GetAudioCap", "NET_clientGetDeviceCapability【NET_CAP_AUDIO=6】", "NET_AudioCap_S", cap);
}

/** 20. OSD能力集 (NET_CAP_OSD) */
static void Test_GetOsdCap()
{
    int idx = SelectClient("OSD能力集");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) { printf("[OsdCap] 客户端%d未登录\n", idx+1); return; }

    /* 通道级命令(能力集), channel<0 服务端返回参数错误, 默认0 */
    INT32 nChannel = ReadChannelParam(0);
    NET_OsdCap_S cap;
    memset(&cap, 0, sizeof(cap));
    INT32 nRetBytes = 0;
    BOOL ret = NET_clientGetDeviceCapability(g_lpUserID[idx], nChannel, NET_CAP_OSD, &cap, sizeof(cap), &nRetBytes);
    if (!ret) { PrintFailure("[OsdCap] 查询"); return; }
    PrintStructJson("Test_GetOsdCap", "NET_clientGetDeviceCapability【NET_CAP_OSD=2】", "NET_OsdCap_S", cap);
}

/** 21. 获取设备信息 (NET_GET_DEVICECFG=100) */
static void Test_GetDeviceInfo()
{
    int idx = SelectClient("获取设备信息");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) { printf("[DevInfo] 客户端%d未登录\n", idx+1); return; }

    NET_DeviceBasicInfo_S info;
    memset(&info, 0, sizeof(info));
    INT32 nRetBytes = 0;
    BOOL ret = NET_clientGetDevConfig(g_lpUserID[idx], -1, NET_GET_DEVICECFG, &info, sizeof(info), &nRetBytes);
    if (!ret) { PrintFailure("[DevInfo] 查询"); return; }
    PrintStructJson("Test_GetDeviceInfo", "NET_clientGetDevConfig【100】", "NET_DeviceBasicInfo_S", info);
}

/** 22. 获取通道列表 (NET_GET_CHANNEL_INFO=300) */
static void Test_GetChannelList()
{
    int idx = SelectClient("获取通道列表");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) { printf("[ChList] 客户端%d未登录\n", idx+1); return; }

    /* 通道号由网页/终端参数输入决定: >=0 单通道, 空行(默认-1)全通道列表 */
    INT32 nChannel = ReadChannelParam(-1);

    /* NET_ChannelList_S 较大(~400KB+)，必须堆分配 */
    NET_ChannelList_S* pList = (NET_ChannelList_S*)calloc(1, sizeof(NET_ChannelList_S));
    if (!pList) { printf("[ChList] 内存分配失败\n"); return; }

    INT32 nRetBytes = 0;
    BOOL ret = NET_clientGetDevConfig(g_lpUserID[idx], nChannel, NET_GET_CHANNEL_INFO, pList, sizeof(*pList), &nRetBytes);
    if (!ret) { PrintFailure("[ChList] 查询"); free(pList); return; }
    PrintStructJson("Test_GetChannelList", "NET_clientGetDevConfig【300】", "NET_ChannelList_S", *pList);
    free(pList);
}

/** 23. 获取存储信息 (NET_GET_STORAGE_INFO=504) */
static void Test_GetStorageInfo()
{
    int idx = SelectClient("存储信息");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) { printf("[Storage] 客户端%d未登录\n", idx+1); return; }

    NET_DeviceStorageInfo_S info;
    memset(&info, 0, sizeof(info));
    INT32 nRetBytes = 0;
    BOOL ret = NET_clientGetDevConfig(g_lpUserID[idx], -1, NET_GET_STORAGE_INFO, &info, sizeof(info), &nRetBytes);
    if (!ret) { PrintFailure("[Storage] 查询"); return; }
    PrintStructJson("Test_GetStorageInfo", "NET_clientGetDevConfig【504】", "NET_DeviceStorageInfo_S", info);
}

/** 25. 录像状态 (NET_GET_RECORD_STATUS=474) */
static void Test_GetRecordStatus()
{
    int idx = SelectClient("录像状态");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) { printf("[RecSt] 客户端%d未登录\n", idx+1); return; }

    /* 通道级命令, 空行默认0 (原 ReadInt 的 scanf 遇空行会永久阻塞, 换 fgets 实现) */
    INT32 ch = ReadChannelParam(0);
    NET_RecordStatusInfo_S st;
    memset(&st, 0, sizeof(st));
    INT32 nRetBytes = 0;
    BOOL ret = NET_clientGetDevConfig(g_lpUserID[idx], ch, NET_GET_RECORD_STATUS, &st, sizeof(st), &nRetBytes);
    if (!ret) { PrintFailure("[RecSt] 查询"); return; }
    PrintStructJson("Test_GetRecordStatus", "NET_clientGetDevConfig【474】", "NET_RecordStatusInfo_S", st);
}

/** 26. 音频配置查询 (NET_GET_AUDIOCFG=130) */
static void Test_GetAudioCfg()
{
    int idx = SelectClient("音频配置查询");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) { printf("[AudioCfg] 客户端%d未登录\n", idx+1); return; }

    /* 通道级命令, channel<0 被一级路由拒绝, 默认0 */
    INT32 nChannel = ReadChannelParam(0);
    NET_AudioCfg_S cfg;
    memset(&cfg, 0, sizeof(cfg));
    INT32 nRetBytes = 0;
    BOOL ret = NET_clientGetDevConfig(g_lpUserID[idx], nChannel, NET_GET_AUDIOCFG, &cfg, sizeof(cfg), &nRetBytes);
    if (!ret) { PrintFailure("[AudioCfg] 查询"); return; }
    PrintStructJson("Test_GetAudioCfg", "NET_clientGetDevConfig【130】", "NET_AudioCfg_S", cfg);
}

/** 27. 获取RTSP地址 (NET_GET_RTSPURLCFG=122) */
static void Test_GetRtspUrl()
{
    int idx = SelectClient("RTSP地址");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) { printf("[RTSP] 客户端%d未登录\n", idx+1); return; }

    NET_RtspUrlInfo_S rtsp;
    memset(&rtsp, 0, sizeof(rtsp));
    if (g_bWebMode) {
        /* Web 模式: 必要参数以 JSON 一次性输入, 如 {"Channel":0,"StreamIndex":1} */
        ReadStructJson(rtsp);
    } else {
        rtsp.uChannel = ReadInt("通道号(0=第一通道, 回车=-1设备级): ");
        rtsp.uStreamIndex = ReadInt("码流索引(0主/1子): ");
    }
    INT32 nRetBytes = 0;
    BOOL ret = NET_clientGetDevConfig(g_lpUserID[idx], rtsp.uChannel, NET_GET_RTSPURLCFG, &rtsp, sizeof(rtsp), &nRetBytes);
    if (ret) PrintStructJson("Test_GetRtspUrl", "NET_clientGetDevConfig【122】", "NET_RtspUrlInfo_S", rtsp);
    else PrintFailure("[RTSP] 查询");
}

/** 28. 获取NTP配置 (NET_GET_NTPCFG=110) */
static void Test_GetNtpConfig()
{
    int idx = SelectClient("NTP配置");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) { printf("[NTP] 客户端%d未登录\n", idx+1); return; }

    NET_SystemNtpInfo_S ntp;
    memset(&ntp, 0, sizeof(ntp));
    INT32 nRetBytes = 0;
    BOOL ret = NET_clientGetDevConfig(g_lpUserID[idx], -1, NET_GET_NTPCFG, &ntp, sizeof(ntp), &nRetBytes);
    if (!ret) { PrintFailure("[NTP] 查询"); return; }
    PrintStructJson("Test_GetNtpConfig", "NET_clientGetDevConfig【110】", "NET_SystemNtpInfo_S", ntp);
}

/** 29. 设置NTP配置 (NET_SET_NTPCFG=111) */
static void Test_SetNtpConfig()
{
    int idx = SelectClient("设置NTP");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) { printf("[NTP] 客户端%d未登录\n", idx+1); return; }

    /* 先获取当前配置 */
    NET_SystemNtpInfo_S ntp;
    memset(&ntp, 0, sizeof(ntp));
    INT32 nRetBytes = 0;
    BOOL ret = NET_clientGetDevConfig(g_lpUserID[idx], -1, NET_GET_NTPCFG, &ntp, sizeof(ntp), &nRetBytes);
    if (!ret) { PrintFailure("[NTP] 查询当前配置"); return; }
    PrintStructJson("Test_SetNtpConfig", "NET_clientGetDevConfig【110】", "NET_SystemNtpInfo_S", ntp);
    if (g_bWebMode) {
        /* Web 模式: 以 JSON 输入要设置的字段, 未给出的字段保持当前配置 */
        ReadStructJson(ntp);
    } else {
        char addr[128];
        ReadStringDefault("NTP服务器地址", addr, sizeof(addr), ntp.strAddress);
        SafeCopy(ntp.strAddress, sizeof(ntp.strAddress), addr);
        ntp.nPort = ReadInt("NTP端口(默认123): ");
        if (ntp.nPort <= 0) ntp.nPort = 123;
        ntp.nSyncInterval = ReadInt("同步间隔(分钟, 默认60): ");
        if (ntp.nSyncInterval <= 0) ntp.nSyncInterval = 60;
        ntp.bEnableNTPSync = TRUE;
    }

    ret = NET_clientSetDevConfig(g_lpUserID[idx], -1, NET_SET_NTPCFG, &ntp, sizeof(ntp), &nRetBytes);
    printf("实际调用：NET_clientSetDevConfig【111】 -> %s\n", ret ? "成功" : "失败");
    if (!ret) PrintFailure("SetNtpConfig");
}

/** 30. 设置系统时间 (NET_SET_SYSTEM_TIME=112) */
static void Test_SetSystemTime()
{
    int idx = SelectClient("设置系统时间");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) { printf("[SysTime] 客户端%d未登录\n", idx+1); return; }

    NET_SystemTime_S stTime;
    memset(&stTime, 0, sizeof(stTime));

    /* 默认填充当前时间 */
    {
        time_t now = time(NULL);
        struct tm tmNow = *localtime(&now);
        strftime(stTime.strDateTime, sizeof(stTime.strDateTime), "%Y-%m-%d %H:%M:%S", &tmNow);
    }

    if (g_bWebMode) {
        ReadStructJson(stTime);
    } else {
        char dt[64];
        ReadStringDefault("日期时间(YYYY-MM-DD HH:MM:SS)", dt, sizeof(dt), stTime.strDateTime);
        SafeCopy(stTime.strDateTime, sizeof(stTime.strDateTime), dt);
    }

    INT32 nRetBytes = 0;
    printf("测试接口：Test_SetSystemTime\n");
    printf("实际调用：NET_clientSetDevConfig【112】 dateTime=%s\n", stTime.strDateTime);
    BOOL ret = NET_clientSetDevConfig(g_lpUserID[idx], -1, NET_SET_SYSTEM_TIME, &stTime, sizeof(stTime), &nRetBytes);
    printf("实际调用：NET_clientSetDevConfig【112】 -> %s\n", ret ? "成功" : "失败");
    if (!ret) PrintFailure("SetSystemTime");
}

/** 31. 查询录像时间段 (NET_GET_REPLAY_RECORD_LIST=124) */
static void Test_GetReplayRecordList()
{
    int idx = SelectClient("录像时间段");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) { printf("[RecList] 客户端%d未登录\n", idx+1); return; }

    NET_ReplayRecordList_S list;
    memset(&list, 0, sizeof(list));
    if (g_bWebMode) {
        /* Web 模式: 必要参数以 JSON 一次性输入, 如 {"Channel":0,"Date":"2026-09-01"} */
        time_t now = time(NULL);
        struct tm* tmNow = localtime(&now);
        strftime(list.szDate, sizeof(list.szDate), "%Y-%m-%d", tmNow);  /* 默认今天 */
        ReadStructJson(list);
    } else {
        list.uChannel = ReadInt("通道号(0=第一通道, 回车=-1设备级): ");
        time_t now = time(NULL);
        struct tm* tmNow = localtime(&now);
        strftime(list.szDate, sizeof(list.szDate), "%Y-%m-%d", tmNow);
        char dateBuf[32];
        ReadStringDefault("日期(YYYY-MM-DD)", dateBuf, sizeof(dateBuf), list.szDate);
        SafeCopy(list.szDate, sizeof(list.szDate), dateBuf);
    }

    INT32 nRetBytes = 0;
    BOOL ret = NET_clientGetDevConfig(g_lpUserID[idx], list.uChannel, NET_GET_REPLAY_RECORD_LIST, &list, sizeof(list), &nRetBytes);
    if (!ret) { PrintFailure("[RecList] 查询"); return; }
    PrintStructJson("Test_GetReplayRecordList", "NET_clientGetDevConfig【124】", "NET_ReplayRecordList_S", list);
}

/** 31. 获取回放地址 (NET_clientGetReplayUrl) */
static void Test_GetReplayUrl()
{
    int idx = SelectClient("回放地址");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) { printf("[ReplayUrl] 客户端%d未登录\n", idx+1); return; }

    NET_ReplayUrlInfo_S url;
    memset(&url, 0, sizeof(url));
    if (g_bWebMode) {
        /* Web 模式: 必要参数以 JSON 一次性输入, 如 {"Channel":0,"StartTime":"2026-09-01 00:00:00","EndTime":"2026-09-01 23:59:59"} */
        GetDefaultTimeRange(url.szStartTime, sizeof(url.szStartTime), url.szEndTime, sizeof(url.szEndTime));
        ReadStructJson(url);
    } else {
        url.uChannel = ReadInt("通道号(0=第一通道, 回车=-1设备级): ");
        char start[64], end[64];
        GetDefaultTimeRange(start, sizeof(start), end, sizeof(end));
        ReadStringDefault("开始时间", url.szStartTime, sizeof(url.szStartTime), start);
        ReadStringDefault("结束时间", url.szEndTime, sizeof(url.szEndTime), end);
    }

    INT32 nRetBytes = 0;
    BOOL ret = NET_clientGetReplayUrl(g_lpUserID[idx], &url, &nRetBytes);
    if (ret) PrintStructJson("Test_GetReplayUrl", "NET_clientGetReplayUrl", "NET_ReplayUrlInfo_S", url);
    else PrintFailure("[ReplayUrl]");
}

/** 32. 回放控制 (NET_clientControlReplay) */
static void Test_ControlReplay()
{
    int idx = SelectClient("回放控制");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) { printf("[Replay] 客户端%d未登录\n", idx+1); return; }

    NET_ReplayCtrlInfo_S ctrl;
    memset(&ctrl, 0, sizeof(ctrl));
    if (g_bWebMode) {
        /* Web 模式: 必要参数以 JSON 一次性输入, 输入前打印 CtrlType/字段说明
         * 注意 Channel 是必填项: 设备端 ControlReplay 直接用它做 relay 组定位,
         * 缺失(默认0)时若会话不在 0 通道上, 控制命令会落到空组并回 102/失败。 */
        printf("[Replay] CtrlType: 1=开始回放 | 2=停止 | 3=倍速 | 4=暂停 | 5=跳转 | 6=恢复\n");
        printf("[Replay] Channel: 通道号(必填, 与第1条开始回放保持一致)\n");
        printf("[Replay] 格式示例:\n");
        printf("[Replay]   开始={\"CtrlType\":1,\"Channel\":0,\"StartTime\":\"2026-09-18 00:00:00\",\"EndTime\":\"2026-09-18 23:59:59\"}\n");
        printf("[Replay]   暂停={\"CtrlType\":4,\"Channel\":0}\n");
        printf("[Replay]   恢复={\"CtrlType\":6,\"Channel\":0}\n");
        printf("[Replay]   停止={\"CtrlType\":2,\"Channel\":0}\n");
        printf("[Replay]   倍速={\"CtrlType\":3,\"Channel\":0,\"Speed\":4}\n");
        printf("[Replay]   跳转={\"CtrlType\":5,\"Channel\":0,\"SeekTime\":60}\n");
        printf("[Replay] 2~6 无需填 SessionId/时间, 自动沿用上次会话; 但 Channel 每条都要填\n");
        ReadStructJson(ctrl);
        if (ctrl.uCtrlType < 1 || ctrl.uCtrlType > 6) { printf("[取消] CtrlType无效(1~6)\n"); return; }
        /* from_string 对 char 数组字段是覆写语义(JSON缺key即清空), 解析前的预填会被抹掉,
         * 所以默认参数统一在解析后兜底: 2~6补上次会话ID(服务端校验 stop/pause/resume 缺失即拒 102),
         * 1补默认时间(仅起止都未填时) */
        if (ctrl.uCtrlType != NET_REPLAY_CTRL_START && ctrl.szSessionId[0] == '\0' && g_replaySessionId[0]) {
            SafeCopy(ctrl.szSessionId, sizeof(ctrl.szSessionId), g_replaySessionId);
            printf("[Replay] 自动填上次会话: %s\n", g_replaySessionId);
        }
        if (ctrl.uCtrlType == NET_REPLAY_CTRL_START && ctrl.szStartTime[0] == '\0' && ctrl.szEndTime[0] == '\0') {
            GetDefaultTimeRange(ctrl.szStartTime, sizeof(ctrl.szStartTime), ctrl.szEndTime, sizeof(ctrl.szEndTime));
            printf("[Replay] 自动填默认时间: %s ~ %s\n", ctrl.szStartTime, ctrl.szEndTime);
        }
        /* 2~6 若未填 Channel, 沿用上次会话所在的通道, 避免全部落到 0 通道 */
        if (ctrl.uCtrlType != NET_REPLAY_CTRL_START && ctrl.uChannel == 0) {
            ctrl.uChannel = g_replayChannel;
            printf("[Replay] 自动填上次通道: %d\n", ctrl.uChannel);
        }
        /* 2~5(停止/倍速/暂停/跳转)与6(恢复)都不需要用户填时间:
         * 服务端按会话续位, 传时间也不会报错但会被忽略 —— 这里直接清空,
         * 避免用户误以为要自己算暂停时刻, 也避免把无关时间下发到服务端 */
        if (ctrl.uCtrlType == NET_REPLAY_CTRL_RESUME || ctrl.uCtrlType == NET_REPLAY_CTRL_PAUSE) {
            ctrl.szStartTime[0] = '\0';
            ctrl.szEndTime[0] = '\0';
        }
    } else {
        printf("  [1] 开始回放  [2] 停止  [3] 倍速  [4] 暂停  [5] 跳转  [6] 恢复\n");
        printf("  说明: 2~6 自动沿用上次会话, 无需填时间; 第1条才需填通道与起止时间\n");
        int cmd = ReadInt("选择命令: ");
        if (cmd < 1 || cmd > 6) { printf("[取消]\n"); return; }
        ctrl.uCtrlType = cmd;

        if (cmd == NET_REPLAY_CTRL_START) {
            ctrl.uChannel = ReadInt("通道号(0=第一通道, 回车=-1设备级): ");
            g_replayChannel = ctrl.uChannel;
            char start[64], end[64];
            GetDefaultTimeRange(start, sizeof(start), end, sizeof(end));
            ReadStringDefault("开始时间", ctrl.szStartTime, sizeof(ctrl.szStartTime), start);
            ReadStringDefault("结束时间", ctrl.szEndTime, sizeof(ctrl.szEndTime), end);
        } else {
            if (g_replaySessionId[0]) {
                SafeCopy(ctrl.szSessionId, sizeof(ctrl.szSessionId), g_replaySessionId);
                printf("使用会话: %s\n", g_replaySessionId);
            } else {
                ReadString("会话ID(上次为空): ", ctrl.szSessionId, sizeof(ctrl.szSessionId));
            }
            /* 通道号必须带: 设备端 ControlReplay 用它定位 relay 组, 缺省 0 会与
             * 实际会话通道不一致 —— 默认直接沿用开始回放时的通道 */
            {
                char szPrompt[64] = {0};
                snprintf(szPrompt, sizeof(szPrompt), "通道号(默认%d): ", g_replayChannel);
                ctrl.uChannel = ReadInt(szPrompt);
            }
            if (cmd == NET_REPLAY_CTRL_SET_SPEED) {
                printf("倍速值(仅支持1/2/4/8): ");
                float spd = 1.0f;
                if (scanf("%f", &spd) != 1) spd = 1.0f;
                ClearInputBuf();
                ctrl.fSpeed = spd;
            }
            if (cmd == NET_REPLAY_CTRL_SET_SEEK) {
                ctrl.nSeekTime = ReadInt("跳转秒数: ");
            }
        }
    }

    INT32 nRetBytes = 0;
    BOOL ret = NET_clientControlReplay(g_lpUserID[idx], &ctrl, &nRetBytes);
    if (ret) {
        PrintStructJson("Test_ControlReplay", "NET_clientControlReplay", "NET_ReplayCtrlInfo_S", ctrl);
        if (ctrl.uCtrlType == NET_REPLAY_CTRL_START && ctrl.szSessionId[0])
            SafeCopy(g_replaySessionId, sizeof(g_replaySessionId), ctrl.szSessionId);
        if (ctrl.uCtrlType == NET_REPLAY_CTRL_STOP) g_replaySessionId[0] = '\0';
        /* 2~6 的回包可能带真实流地址(尤其6=恢复播放重建后), 单独打印便于直接拿去拉流 */
        if (ctrl.uCtrlType != NET_REPLAY_CTRL_STOP && ctrl.szUrl[0])
            printf("[Replay] 回包Url: %s\n", ctrl.szUrl);
    } else {
        PrintFailure("[Replay]");
    }
}

/** 33. 录像帧流 (StartRecordFrameStream / StopRecordFrameStream) */
static void Test_RecordFrameStream()
{
    int idx = SelectClient("录像帧流");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) { printf("[RecFrame] 客户端%d未登录\n", idx+1); return; }

    if (g_recordStreamId[0]) {
        printf("当前有活动录像帧流: %s\n", g_recordStreamId);
        int stop = ReadInt("停止? (1=停止, 0=取消): ");
        if (stop == 1) {
            BOOL ret = NET_clientStopRecordFrameStream(g_lpUserID[idx], g_recordStreamId);
            printf("[RecFrame] 停止 %s (共收到 %u 帧)\n", ret ? "成功" : "失败", g_recordFrameCount);
            g_recordStreamId[0] = '\0';
            g_recordStreamSlot = -1;
            g_recordFrameCount = 0;
        }
        return;
    }

    NET_RecordFrameStreamCond_S cond;
    memset(&cond, 0, sizeof(cond));
    cond.uSize = sizeof(cond);
    if (g_bWebMode) {
        /* Web 模式: 必要参数以 JSON 一次性输入 (字段为小写), 如 {"channel":0,"startTime":"...","endTime":"..."} */
        GetDefaultTimeRange(cond.szStartTime, sizeof(cond.szStartTime), cond.szEndTime, sizeof(cond.szEndTime));
        ReadStructJson(cond);
    } else {
        cond.uChannel = ReadInt("通道号(0=第一通道, 回车=-1设备级): ");
        char start[64], end[64];
        GetDefaultTimeRange(start, sizeof(start), end, sizeof(end));
        ReadStringDefault("开始时间", cond.szStartTime, sizeof(cond.szStartTime), start);
        ReadStringDefault("结束时间", cond.szEndTime, sizeof(cond.szEndTime), end);
    }

    NET_RecordFrameStreamInfo_S info;
    memset(&info, 0, sizeof(info));
    info.uSize = sizeof(info);
    g_recordFrameCount = 0;

    BOOL ret = NET_clientStartRecordFrameStream(g_lpUserID[idx], &cond, &info, RecordFrameCallBack, NULL);
    if (!ret) { PrintFailure("[RecFrame] 启动"); return; }
    PrintStructJson("Test_RecordFrameStream", "NET_clientStartRecordFrameStream", "NET_RecordFrameStreamInfo_S", info);
    SafeCopy(g_recordStreamId, sizeof(g_recordStreamId), info.szStreamId);
    g_recordStreamSlot = idx;
    printf("按回车停止录像帧流...\n");
    getchar();

    ret = NET_clientStopRecordFrameStream(g_lpUserID[idx], g_recordStreamId);
    printf("[RecFrame] 停止 %s (共收到 %u 帧)\n", ret ? "成功" : "失败", g_recordFrameCount);
    g_recordStreamId[0] = '\0';
    g_recordStreamSlot = -1;
    g_recordFrameCount = 0;
}

/* 净化时间文本: 全角空格(U+3000=E3 80 80)/制表符等替换为半角空格。
 * 全角空格 %s 打印看不出差异, 但服务端 strlen!=19 严格校验会判 102。 */
static void SanitizeTimeText(char* szText, size_t size)
{
    if (!szText || szText[0] == '\0') return;
    char szOut[128];
    size_t j = 0;
    for (size_t i = 0; szText[i] != '\0' && j < sizeof(szOut) - 1; ) {
        if ((unsigned char)szText[i] == 0xE3 && (unsigned char)szText[i+1] == 0x80 && (unsigned char)szText[i+2] == 0x80) {
            szOut[j++] = ' ';   /* 全角空格 -> 半角 */
            i += 3;
        } else if (szText[i] == '\t') {
            szOut[j++] = ' ';   /* 制表符 -> 半角空格 */
            i++;
        } else {
            szOut[j++] = szText[i++];
        }
    }
    szOut[j] = '\0';
    SafeCopy(szText, size, szOut);
}

/* 归一化查询时间: 时间段须为 "YYYY-MM-DD HH:MM:SS" 完整格式 (config demo 190 已验证打通的格式)。
 * SDK接口内部裁剪: StartTime+EndTime 都非空时会清空 Date;
 * 服务端校验要求时间段必须含日期, 纯 "HH:MM:SS" 会被判 102。
 * Date 非空则作基准日期, 否则用 szToday; 时间为空补全天, 纯时间则拼上日期。
 * 特例: 只填了 Date 且未填任何时间时保留单日模式(服务端单日分支兼容性更好, 也可用于验证服务端版本)。 */
static void NormalizeRecordQueryTime(NET_RecordFindCond_S& stFind, const char* szToday)
{
    if (stFind.szStartTime[0] == '\0' && stFind.szEndTime[0] == '\0' && stFind.szDate[0] != '\0') {
        SanitizeTimeText(stFind.szDate, sizeof(stFind.szDate));
        return;  /* 单日模式, 保持 Date 原样下发 */
    }
    SanitizeTimeText(stFind.szDate, sizeof(stFind.szDate));
    SanitizeTimeText(stFind.szStartTime, sizeof(stFind.szStartTime));
    SanitizeTimeText(stFind.szEndTime, sizeof(stFind.szEndTime));
    const char* szBase = (stFind.szDate[0] != '\0') ? stFind.szDate : szToday;
    char szFull[80];

    if (stFind.szStartTime[0] == '\0') {
        snprintf(szFull, sizeof(szFull), "%s 00:00:00", szBase);
    } else if (strlen(stFind.szStartTime) <= 8) {  /* 纯 "HH:MM:SS" */
        snprintf(szFull, sizeof(szFull), "%s %s", szBase, stFind.szStartTime);
    } else {
        SafeCopy(szFull, sizeof(szFull), stFind.szStartTime);
    }
    SafeCopy(stFind.szStartTime, sizeof(stFind.szStartTime), szFull);

    if (stFind.szEndTime[0] == '\0') {
        snprintf(szFull, sizeof(szFull), "%s 23:59:59", szBase);
    } else if (strlen(stFind.szEndTime) <= 8) {
        snprintf(szFull, sizeof(szFull), "%s %s", szBase, stFind.szEndTime);
    } else {
        SafeCopy(szFull, sizeof(szFull), stFind.szEndTime);
    }
    SafeCopy(stFind.szEndTime, sizeof(stFind.szEndTime), szFull);

    stFind.szDate[0] = '\0';  /* 条件互斥: 切换为完整时间段后清空 Date */
}

/** 35. 分页查询录像文件 (NET_clientQueryRecordFiles, 无48条上限) */
static void Test_QueryRecordFiles()
{
    int idx = SelectClient("分页查询录像文件");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) { printf("[QueryRec] 客户端%d未登录\n", idx+1); return; }

    NET_RecordFileQuery_S stQuery;
    NET_RecordFindResult_S astResults[16];
    memset(&stQuery, 0, sizeof(stQuery));
    memset(astResults, 0, sizeof(astResults));

    /* 默认查询范围: 当天全天 (Web/CLI 共用) */
    time_t now = time(NULL);
    struct tm* tmNow = localtime(&now);
    char dateBuf[32];
    strftime(dateBuf, sizeof(dateBuf), "%Y-%m-%d", tmNow);

    if (g_bWebMode) {
        /* Web 模式: JSON key 为大写首字母(SDK映射), 时间段须为含日期的完整格式, 如
         * {"Find":{"ChnId":0,"StartTime":"2026-09-18 00:00:00","EndTime":"2026-09-18 23:59:59"},"Page":{"CurPage":1}}
         * 注意: 纯 "HH:MM:SS" 会被设备判 102; 结构体成员名(stFind/szDate等)会被 SDK 忽略 */
        ReadStructJson(stQuery);
        if (stQuery.stFind.szDate[0] == '\0' && stQuery.stFind.szStartTime[0] == '\0' &&
            stQuery.stFind.szEndTime[0] == '\0') {
            printf("[QueryRec] 提示: 未识别到查询条件(JSON key 应为 Find/ChnId/Date/StartTime/EndTime/Page/CurPage), 已回退当天全天\n");
        }
    } else {
        stQuery.stFind.nChnId = ReadInt("通道号(0=第一通道, 回车=-1设备级): ");
        ReadStringDefault("日期(YYYY-MM-DD)", stQuery.stFind.szDate, sizeof(stQuery.stFind.szDate), dateBuf);
        ReadStringDefault("开始时间(HH:MM:SS)", stQuery.stFind.szStartTime, sizeof(stQuery.stFind.szStartTime), "00:00:00");
        ReadStringDefault("结束时间(HH:MM:SS)", stQuery.stFind.szEndTime, sizeof(stQuery.stFind.szEndTime), "23:59:59");
        stQuery.stPage.nCurPage = ReadInt("页码(从1开始, 默认1): ");
        if (stQuery.stPage.nCurPage <= 0) stQuery.stPage.nCurPage = 1;
    }

    /* 统一归一化: 拼成设备认可的 "YYYY-MM-DD HH:MM:SS" 完整时间段 */
    NormalizeRecordQueryTime(stQuery.stFind, dateBuf);

    if (stQuery.stPage.nCurPage <= 0) stQuery.stPage.nCurPage = 1;
    stQuery.stPage.nPageSize = (INT32)(sizeof(astResults) / sizeof(astResults[0]));
    stQuery.pResults = astResults;
    stQuery.nResultCapacity = (INT32)(sizeof(astResults) / sizeof(astResults[0]));

    printf("[QueryRec] 调用 NET_clientQueryRecordFiles, channel=%d, range=[%s ~ %s], page=%d, size=%d\n",
           stQuery.stFind.nChnId, stQuery.stFind.szStartTime,
           stQuery.stFind.szEndTime,
           stQuery.stPage.nCurPage, stQuery.stPage.nPageSize);

    INT32 nRetBytes = 0;
    BOOL ret = NET_clientQueryRecordFiles(g_lpUserID[idx], &stQuery, &nRetBytes);
    if (!ret) { PrintFailure("[QueryRec] 分页查询"); return; }

    printf("[QueryRec] 分页查询成功! ResultCount=%d, DataTotal=%d, PageTotal=%d, HasMore=%d\n",
           stQuery.nResultCount, stQuery.stPage.nDataTotal,
           stQuery.stPage.nPageTotal, stQuery.stPage.bHasMore);

    /* 结果以 JSON 输出(与SDK序列化格式一致, pResults自动转Infos数组), 便于网页端/程序直接解析 */
    printf("[QueryRec] ResultJSON:\n%s\n", SDKConvert::to_string(stQuery).c_str());
}

/** 34. 语音对讲 (Start/Send/Stop VoiceCom) */
static void Test_VoiceCom()
{
    int idx = SelectClient("语音对讲");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) { printf("[Voice] 客户端%d未登录\n", idx+1); return; }

    printf("测试接口：Test_VoiceCom\n");
    if (g_voiceComSlot >= 0) {
        printf("当前有活动语音对讲(槽位%d)\n", g_voiceComSlot + 1);
        int stop = ReadInt("停止? (1=停止, 0=取消): ");
        if (stop == 1) {
            printf("实际调用：NET_clientStopVoiceCom\n");
            BOOL ret = NET_clientStopVoiceCom(g_lpUserID[g_voiceComSlot]);
            printf("  -> %s (接收 %u 字节)\n", ret ? "成功" : "失败", g_voiceRecvBytes);
            g_voiceComSlot = -1;
            g_voiceRecvBytes = 0;
        }
        return;
    }

    NET_VoiceComStartInfo_S startInfo;
    memset(&startInfo, 0, sizeof(startInfo));
    startInfo.uAudioPort = 9006;
    startInfo.stAudioParam.enFormat = NET_AUDIO_FORMAT_G711A;
    startInfo.stAudioParam.uSampleRate = 8000;
    startInfo.stAudioParam.uBitDepth = 8;
    startInfo.stAudioParam.uChannels = 1;
    startInfo.stAudioParam.uFrameIntervalMs = 40;
    /* G711: 8000Hz * 0.04s * 8bit / 8 = 320 bytes/frame */
    startInfo.stAudioParam.uFrameBytes =
        startInfo.stAudioParam.uSampleRate * startInfo.stAudioParam.uFrameIntervalMs
        * startInfo.stAudioParam.uBitDepth / 8000;
    g_voiceComFrameBytes = (UINT32)startInfo.stAudioParam.uFrameBytes;

    g_voiceRecvBytes = 0;
    printf("实际调用：NET_clientStartVoiceCom\n");
    BOOL ret = NET_clientStartVoiceCom(g_lpUserID[idx], &startInfo, VoiceComCallBack, NULL);
    printf("  -> %s\n", ret ? "成功" : "失败");
    if (!ret) { PrintFailure("[Voice] 启动"); return; }
    g_voiceComSlot = idx;
    printf("[Voice] 启动成功! 帧大小=%u字节, 发送静音帧中...\n", g_voiceComFrameBytes);
    printf("按回车停止语音对讲...\n");

    /* 发送静音帧，每40ms一帧 */
    char* pSilence = (char*)calloc(1, g_voiceComFrameBytes);
    if (pSilence) {
        int sent = 0;
        while (getchar() == EOF) {
            NET_clientVoiceComSendData(g_lpUserID[idx], pSilence, g_voiceComFrameBytes);
            sent++;
            usleep(startInfo.stAudioParam.uFrameIntervalMs * 1000);
            if (sent > 10000) break; /* 安全上限 */
        }
        free(pSilence);
    }

    printf("实际调用：NET_clientStopVoiceCom\n");
    ret = NET_clientStopVoiceCom(g_lpUserID[idx]);
    printf("  -> %s (接收 %u 字节)\n", ret ? "成功" : "失败", g_voiceRecvBytes);
    g_voiceComSlot = -1;
    g_voiceRecvBytes = 0;
}

/** 300. 综合流程测试 (全自动, 无交互, 默认参数) */
static void PrintMenu();  /* 前向声明: 综合流程命令0用于展示菜单 */

/* ===== 综合流程辅助宏: 获取设备配置/能力并打印JSON, 记录结果 ===== */
#define FLOW_GET_CFG(menuCmd, label, StructType, apiCmd, ch) \
    do { \
        StructType _cfg; \
        memset(&_cfg, 0, sizeof(_cfg)); \
        BOOL _ret = NET_clientGetDevConfig(lpUser, ch, apiCmd, &_cfg, sizeof(_cfg), &nRetBytes); \
        if (_ret) { \
            std::string _json = SDKConvert::to_string(_cfg); \
            printf("%s\n", _json.c_str()); \
            snprintf(buf, sizeof(buf), "[%d] %s: 成功", (int)(menuCmd), label); \
        } else { \
            snprintf(buf, sizeof(buf), "[%d] %s: 失败 (Error=%d)", (int)(menuCmd), label, NET_clientGetLastError()); \
        } \
        results.push_back(buf); \
        printf("    -> %s\n", _ret ? "成功" : "失败"); \
    } while (0)

#define FLOW_GET_CAP(cmdNum, label, StructType, capEnum) \
    do { \
        StructType _cap; \
        memset(&_cap, 0, sizeof(_cap)); \
        BOOL _ret = NET_clientGetDeviceCapability(lpUser, 0, capEnum, &_cap, sizeof(_cap), &nRetBytes); \
        if (_ret) { \
            std::string _json = SDKConvert::to_string(_cap); \
            printf("%s\n", _json.c_str()); \
            snprintf(buf, sizeof(buf), "[%d] %s: 成功", (int)(cmdNum), label); \
        } else { \
            snprintf(buf, sizeof(buf), "[%d] %s: 失败 (Error=%d)", (int)(cmdNum), label, NET_clientGetLastError()); \
        } \
        results.push_back(buf); \
        printf("    -> %s\n", _ret ? "成功" : "失败"); \
    } while (0)

#define FLOW_ERR(tag) \
    do { \
        INT32 _err = NET_clientGetLastError(); \
        const char* _msg = NET_clientGetErrorMsg(); \
        snprintf(buf, sizeof(buf), "[4] 最后错误码(%s): Code=%d, Msg=%s", tag, _err, _msg ? _msg : "(null)"); \
        results.push_back(buf); \
        printf("    %s\n", buf); \
        sleep(3); /* 每个接口调用后间隔3秒, 错误码获取本身不sleep */ \
    } while (0)

static void Test_FullFlow()
{
    printf("\n=== 综合流程测试 (全自动) ===\n");
    printf("目标: %s:%d 用户: %s\n", g_serverIp, g_serverPort, g_username);
    printf("按命令顺序覆盖获取相关接口, 每测完一项自动打印最后错误码(命令4)\n\n");

    std::vector<std::string> results;
    char buf[8192];
    INT32 nRetBytes = 0;
    LPVOID lpUser = NULL;
    NET_DeviceInfo_S devInfoResult;
    memset(&devInfoResult, 0, sizeof(devInfoResult));

    /* ========== 组1: 基础与登录 ========== */

    /* 命令0: 显示菜单 */
    printf("[0] 显示菜单...\n");
    PrintMenu();
    results.push_back("[0] 显示菜单: 成功");
    FLOW_ERR("显示菜单");

    /* 命令1: 初始化SDK */
    printf("[1] 初始化SDK...\n");
    if (g_bSdkInit) {
        results.push_back("[1] 初始化SDK: 已初始化(跳过)");
        printf("    已初始化, 跳过\n");
    } else {
        BOOL ret = NET_clientInit();
        if (ret) {
            g_bSdkInit = true;
            results.push_back("[1] 初始化SDK: 成功");
            printf("    成功\n");
        } else {
            snprintf(buf, sizeof(buf), "[1] 初始化SDK: 失败! Error=%d", NET_clientGetLastError());
            results.push_back(buf);
            printf("    失败, 终止测试\n");
            goto print_summary;
        }
    }
    FLOW_ERR("初始化SDK");

    /* 命令3: SDK版本 */
    printf("[3] SDK版本...\n");
    {
        INT32 ver = NET_clientGetSdkVersion();
        snprintf(buf, sizeof(buf), "[3] SDK版本: %d.%d.%d (0x%08X)",
                 (ver >> 24) & 0xFF, (ver >> 16) & 0xFF, ver & 0xFFFF, ver);
        results.push_back(buf);
        printf("    %s\n", buf);
    }
    FLOW_ERR("SDK版本");

    /* 命令7: 日志设置(默认, 仅设置一次, 重复设置会崩溃) */
    printf("[7] 日志设置...\n");
    {
        BOOL ret = FALSE;
        if (g_bLogInit) {
            snprintf(buf, sizeof(buf), "[7] 日志设置: 已设置(跳过)");
            printf("    已设置, 跳过\n");
        } else {
            char szLogDir[64];
            SafeCopy(szLogDir, sizeof(szLogDir), "/opt/course/log");
            ret = NET_clientSetLogToFile(2, szLogDir, 10 * 1024 * 1024, 5);
            if (ret) g_bLogInit = true;
            snprintf(buf, sizeof(buf), "[7] 日志设置: %s (level=2, dir=/opt/course/log)",
                     ret ? "成功" : "失败");
            printf("    %s\n", buf);
        }
        results.push_back(buf);
    }
    FLOW_ERR("日志设置");

    /* 命令8: 异常回调 */
    printf("[8] 异常回调...\n");
    {
        BOOL ret = NET_clientSetExceptionCallBack(ExceptionCallBack, NULL);
        snprintf(buf, sizeof(buf), "[8] 异常回调: %s", ret ? "成功" : "失败");
        results.push_back(buf);
        printf("    %s\n", buf);
    }
    FLOW_ERR("异常回调");

    /* 命令9: 登录 */
    printf("[9] 登录 (%s:%d)...\n", g_serverIp, g_serverPort);
    {
        NET_DeviceLoginInfo_S loginInfo;
        memset(&loginInfo, 0, sizeof(loginInfo));
        SafeCopy(loginInfo.szIPAddr, sizeof(loginInfo.szIPAddr), g_serverIp);
        loginInfo.uPort = g_serverPort;
        SafeCopy(loginInfo.szUserName, sizeof(loginInfo.szUserName), g_username);
        SafeCopy(loginInfo.szPassword, sizeof(loginInfo.szPassword), g_password);
        lpUser = NET_clientLogin(&loginInfo, &devInfoResult);
        if (!lpUser) {
            snprintf(buf, sizeof(buf), "[9] 登录: 失败! Error=%d, Msg=%s",
                     NET_clientGetLastError(), NET_clientGetErrorMsg());
            results.push_back(buf);
            printf("    失败, 终止测试\n");
            goto print_summary;
        }
        snprintf(buf, sizeof(buf), "[9] 登录: 成功 (UserID=%p)", lpUser);
        results.push_back(buf);
        printf("    %s\n", buf);
        /* 打印登录返回的设备信息结构体 */
        std::string devJson = SDKConvert::to_string(devInfoResult);
        printf("%s\n", devJson.c_str());
    }
    FLOW_ERR("登录");

    /* ========== 组2: 能力集 ========== */

    /* 命令105: 系统能力总表 */
    printf("[105] 系统能力总表...\n");
    FLOW_GET_CAP(105, "系统能力总表", NET_SysCapability_S, NET_CAP_SYS);
    FLOW_ERR("系统能力总表");

    /* 命令18: 视频编码能力 */
    printf("[18] 视频编码能力...\n");
    FLOW_GET_CAP(18, "视频编码能力", NET_VideoEncodeCap_S, NET_CAP_VIDEO_ENCODE);
    FLOW_ERR("视频编码能力");

    /* 命令19: 音频能力 */
    printf("[19] 音频能力...\n");
    FLOW_GET_CAP(19, "音频能力", NET_AudioCap_S, NET_CAP_AUDIO);
    FLOW_ERR("音频能力");

    /* 命令20: OSD能力 */
    printf("[20] OSD能力...\n");
    FLOW_GET_CAP(20, "OSD能力", NET_OsdCap_S, NET_CAP_OSD);
    FLOW_ERR("OSD能力");

    /* ========== 组3: 设备/通道配置查询 (按SDK依赖: 先设备级, 再通道级) ========== */

    /* 命令21: 设备信息 */
    printf("[21] 设备信息...\n");
    FLOW_GET_CFG(21, "设备信息", NET_DeviceBasicInfo_S, NET_GET_DEVICECFG, -1);
    FLOW_ERR("设备信息");

    /* 命令109: 设备状态 */
    printf("[109] 设备状态...\n");
    FLOW_GET_CFG(109, "设备状态", NET_DeviceStatusInfo_S, NET_GET_DEVICE_STATUS, -1);
    FLOW_ERR("设备状态");

    /* 命令23: 存储信息 */
    printf("[23] 存储信息...\n");
    FLOW_GET_CFG(23, "存储信息", NET_DeviceStorageInfo_S, NET_GET_STORAGE_INFO, -1);
    FLOW_ERR("存储信息");

    /* 命令28: NTP查询 */
    printf("[28] NTP查询...\n");
    FLOW_GET_CFG(28, "NTP查询", NET_SystemNtpInfo_S, NET_GET_NTPCFG, -1);
    FLOW_ERR("NTP查询");

    /* 命令113: 网络查询 */
    printf("[113] 网络查询...\n");
    FLOW_GET_CFG(113, "网络查询", NET_NetworkCfgList_S, NET_GET_NETWORKCFG, -1);
    FLOW_ERR("网络查询");

    /* 命令27: RTSP地址(手写, uChannel=0, uStreamIndex=0) */
    printf("[27] RTSP地址...\n");
    {
        NET_RtspUrlInfo_S rtsp;
        memset(&rtsp, 0, sizeof(rtsp));
        rtsp.uChannel = 0;
        rtsp.uStreamIndex = 0;
        BOOL ret = NET_clientGetDevConfig(lpUser, rtsp.uChannel, NET_GET_RTSPURLCFG, &rtsp, sizeof(rtsp), &nRetBytes);
        if (ret) {
            std::string json = SDKConvert::to_string(rtsp);
            printf("%s\n", json.c_str());
            snprintf(buf, sizeof(buf), "[27] RTSP地址: 成功");
        } else {
            snprintf(buf, sizeof(buf), "[27] RTSP地址: 失败 (Error=%d)", NET_clientGetLastError());
        }
        results.push_back(buf);
        printf("    -> %s\n", ret ? "成功" : "失败");
    }
    FLOW_ERR("RTSP地址");

    /* 命令111: 查询通道名称(ch=0, 打印 szChannelName) */
    printf("[111] 查询通道名称...\n");
    {
        NET_ChannelList_S list;
        memset(&list, 0, sizeof(list));
        BOOL ret = NET_clientGetDevConfig(lpUser, 0, NET_GET_CHANNEL_INFO, &list, sizeof(list), &nRetBytes);
        if (ret) {
            UINT32 count = list.uChannelCount;
            if (count > NET_MAX_CHANNEL_NUM) count = NET_MAX_CHANNEL_NUM;
            printf("通道数: %u\n", list.uChannelCount);
            for (UINT32 i = 0; i < count; i++) {
                printf("  通道%u: %s\n", list.stChannels[i].uChannel, list.stChannels[i].szChannelName);
            }
            snprintf(buf, sizeof(buf), "[111] 查询通道名称: 成功");
        } else {
            snprintf(buf, sizeof(buf), "[111] 查询通道名称: 失败 (Error=%d)", NET_clientGetLastError());
        }
        results.push_back(buf);
        printf("    -> %s\n", ret ? "成功" : "失败");
    }
    FLOW_ERR("查询通道名称");

    /* 命令22: 通道列表(calloc, ch=-1 全通道) */
    printf("[22] 通道列表...\n");
    {
        NET_ChannelList_S* pList = (NET_ChannelList_S*)calloc(1, sizeof(NET_ChannelList_S));
        if (pList) {
            BOOL ret = NET_clientGetDevConfig(lpUser, -1, NET_GET_CHANNEL_INFO, pList, sizeof(*pList), &nRetBytes);
            if (ret) {
                std::string json = SDKConvert::to_string(*pList);
                printf("%s\n", json.c_str());
                snprintf(buf, sizeof(buf), "[22] 通道列表: 成功");
            } else {
                snprintf(buf, sizeof(buf), "[22] 通道列表: 失败 (Error=%d)", NET_clientGetLastError());
            }
            free(pList);
        } else {
            snprintf(buf, sizeof(buf), "[22] 通道列表: 内存分配失败");
        }
        results.push_back(buf);
        printf("    -> %s\n", buf);
    }
    FLOW_ERR("通道列表");

    /* 命令12: 设备发现(手写, 默认网卡NULL, 超时3000ms) */
    printf("[12] 设备发现...\n");
    {
        NET_DiscoveryDeviceInfo_S devices[MAX_DEVICES];
        int count = 0;
        BOOL ret = NET_clientSearchDiscovery(NULL, 3000, devices, MAX_DEVICES, &count);
        if (ret) {
            snprintf(buf, sizeof(buf), "[12] 设备发现: 成功, 发现 %d 台设备", count);
            results.push_back(buf);
            printf("    %s\n", buf);
            for (int i = 0; i < count; i++) {
                const NET_DiscoveryDeviceInfo_S& d = devices[i];
                printf("  ┌─ 设备 #%d ──────────────────────────\n", i + 1);
                printf("  │ 名称:     %s\n", d.strDeviceName);
                printf("  │ ID:       %s\n", d.strDeviceID);
                printf("  │ 类型:     %s\n", d.strDeviceType);
                printf("  │ IP:       %s\n", d.strIPv4Address);
                printf("  │ 子网掩码: %s\n", d.strIPv4SubnetMask);
                printf("  │ 网关:     %s\n", d.strIPv4Gateway);
                printf("  │ MAC:      %s\n", d.strMACAddress);
                printf("  │ 固件:     %s\n", d.strFirmwareVersion);
                printf("  │ HTTP端口: %u\n", d.uHttpPort);
                printf("  │ 厂商:     %s\n", d.strManufacturer);
                printf("  └────────────────────────────────────\n");
            }
        } else {
            snprintf(buf, sizeof(buf), "[12] 设备发现: 失败 (Error=%d)", NET_clientGetLastError());
            results.push_back(buf);
            printf("    %s\n", buf);
        }
    }
    FLOW_ERR("设备发现");

    /* ========== 组4: 升级状态/版本 ========== */

    /* 命令204: 获取升级状态 */
    printf("[204] 获取升级状态...\n");
    FLOW_GET_CFG(204, "获取升级状态", NET_UpgradeStatus_S, NET_GET_UPGRADESTATUS, -1);
    FLOW_ERR("获取升级状态");

    /* 命令206: 获取升级版本 */
    printf("[206] 获取升级版本...\n");
    FLOW_GET_CFG(206, "获取升级版本", NET_UpgradeVersion_S, NET_GET_UPGRADEVERSION, -1);
    FLOW_ERR("获取升级版本");

    /* ========== 组5: 回调注册 ========== */

    /* 命令14: 报警监听 */
    printf("[14] 报警监听...\n");
    {
        BOOL r1 = NET_clientSetAlarmCallBack(lpUser, AlarmCallBack, NULL);
        BOOL r2 = NET_clientStartListen(lpUser);
        snprintf(buf, sizeof(buf), "[14] 报警监听: 注册回调=%s, 开始监听=%s",
                 r1 ? "成功" : "失败", r2 ? "成功" : "失败");
        results.push_back(buf);
        printf("    %s\n", buf);
    }
    FLOW_ERR("报警监听");

    /* 命令16: 通道状态回调 */
    printf("[16] 通道状态回调...\n");
    {
        BOOL ret = NET_clientSetChannelStatusCallBack(lpUser, ChannelStatusCallBack, NULL);
        snprintf(buf, sizeof(buf), "[16] 通道状态回调: %s", ret ? "成功" : "失败");
        results.push_back(buf);
        printf("    %s\n", buf);
    }
    FLOW_ERR("通道状态回调");

    /* ========== 组6: 人脸 ========== */

    /* 命令100: 获取人脸 */
    printf("[100] 获取人脸...\n");
    {
        NET_FaceInfoList_S cfg;
        memset(&cfg, 0, sizeof(cfg));
        BOOL ret = NET_clientGetDevConfig(lpUser, -1, NET_GET_FACE_INFO, &cfg, sizeof(cfg), &nRetBytes);
        if (ret) {
            std::string json = SDKConvert::to_string(cfg);
            printf("%s\n", json.c_str());
            snprintf(buf, sizeof(buf), "[100] 获取人脸: 成功");
        } else {
            snprintf(buf, sizeof(buf), "[100] 获取人脸: 失败 (Error=%d)", NET_clientGetLastError());
        }
        results.push_back(buf);
        printf("    -> %s\n", ret ? "成功" : "失败");
    }
    FLOW_ERR("获取人脸");

    /* 登出用户: 结束综合流程, 释放登录句柄 */
    printf("[登出] 登出用户...\n");
    if (lpUser) {
        BOOL ret = NET_clientLogout(lpUser);
        if (ret) {
            snprintf(buf, sizeof(buf), "[登出] 登出用户: 成功");
            lpUser = NULL;
        } else {
            snprintf(buf, sizeof(buf), "[登出] 登出用户: 失败 (Error=%d)", NET_clientGetLastError());
        }
        results.push_back(buf);
        printf("    %s\n", buf);
    } else {
        results.push_back("[登出] 登出用户: 跳过(未登录)");
        printf("    未登录, 跳过\n");
    }
    FLOW_ERR("登出用户");

print_summary:
    printf("\n");
    printf("========================================\n");
    printf("  综合流程测试结果汇总\n");
    printf("  目标: %s:%d  用户: %s\n", g_serverIp, g_serverPort, g_username);
    printf("========================================\n");
    for (size_t i = 0; i < results.size(); i++) {
        printf("%s\n", results[i].c_str());
    }
    printf("========================================\n");
    printf("  共执行 %zu 步, 测试完成\n", results.size());
    printf("========================================\n");
}

/* ===================== 通用 Get/Set 配置测试模板 ===================== */

#define TEST_GENERIC_GET(T, name, cmd, chDefault)  Test_GenericGet<T>(name, cmd, chDefault, #T)
#define TEST_GENERIC_SET(T, name, getCmd, setCmd, chDefault)  Test_GenericSet<T>(name, getCmd, setCmd, chDefault, #T)
#define TEST_DIRECT_SET(T, name, cmd, chDefault)  Test_DirectSet<T>(name, cmd, chDefault, #T)

/** 通用 Get 配置测试模板: 选择客户端 → 读取通道 → GetDevConfig → 打印 JSON */
template <typename T>
static void Test_GenericGet(const char* name, INT32 cmd, INT32 chDefault, const char* structName)
{
    int idx = SelectClient(name);
    if (idx < 0) return;
    if (!g_lpUserID[idx]) { printf("[%s] 客户端%d未登录\n", name, idx+1); return; }
    /* chDefault<0 表示设备级命令, 无需通道号, 跳过输入等待 */
    INT32 nChannel = (chDefault < 0) ? chDefault : ReadChannelParam(chDefault);
    T cfg;
    memset(&cfg, 0, sizeof(cfg));
    INT32 nRetBytes = 0;
    printf("发送参数: {\"channel\":%d,\"command\":%d} (GET请求无body)\n", nChannel, cmd);
    BOOL ret = NET_clientGetDevConfig(g_lpUserID[idx], nChannel, cmd, &cfg, sizeof(cfg), &nRetBytes);
    if (!ret) { PrintFailure(name); return; }
    char apiCall[128];
    snprintf(apiCall, sizeof(apiCall), "NET_clientGetDevConfig【%d】", cmd);
    PrintStructJson(name, apiCall, structName, cfg);
}

/** 通用 Set 配置测试模板: 先 Get 当前配置 → 可选 JSON 修改 → SetDevConfig */
template <typename T>
static void Test_GenericSet(const char* name, INT32 getCmd, INT32 setCmd, INT32 chDefault, const char* structName)
{
    int idx = SelectClient(name);
    if (idx < 0) return;
    if (!g_lpUserID[idx]) { printf("[%s] 客户端%d未登录\n", name, idx+1); return; }
    /* chDefault<0 表示设备级命令, 无需通道号, 跳过输入等待 */
    INT32 nChannel = (chDefault < 0) ? chDefault : ReadChannelParam(chDefault);
    T cfg;
    memset(&cfg, 0, sizeof(cfg));
    INT32 nRetBytes = 0;
    char apiCall[128];
    snprintf(apiCall, sizeof(apiCall), "NET_clientGetDevConfig【%d】", getCmd);
    BOOL ret = NET_clientGetDevConfig(g_lpUserID[idx], nChannel, getCmd, &cfg, sizeof(cfg), &nRetBytes);
    if (!ret) { PrintFailure(name); return; }
    printf("当前配置:\n");
    PrintStructJson(name, apiCall, structName, cfg);
    if (g_bWebMode) { ReadStructJson(cfg); }
    else { printf("输入JSON修改(空行=原样写回):\n"); ReadStructJson(cfg); }
    printf("发送JSON:\n%s\n", SDKConvert::to_string(cfg).c_str());
    snprintf(apiCall, sizeof(apiCall), "NET_clientSetDevConfig【%d】", setCmd);
    ret = NET_clientSetDevConfig(g_lpUserID[idx], nChannel, setCmd, &cfg, sizeof(cfg), &nRetBytes);
    printf("实际调用：%s -> %s\n", apiCall, ret ? "成功" : "失败");
    if (ret) {
        printf("接收JSON: {\"return\":0,\"message\":\"success\"}\n");
    } else {
        printf("接收JSON: {\"return\":%d,\"message\":\"%s\"}\n", NET_clientGetLastError(), NET_clientGetErrorMsg());
        PrintFailure(name);
    }
}

/** 通用直写 Set 测试模板: 直接读取 JSON 输入 → SetDevConfig, 不做无意义的 GET 前置
 *  适用于 GET 与 SET 结构体类型不同(如人脸/目标库 GET 返回 List, SET 用单项)的命令:
 *  若复用 Test_GenericSet 先以单项结构 GET, 服务端会按 List 大结构填充 → 缓冲区太小(Error=12),
 *  且 GET 失败会提前 return, 导致永远到不了 JSON 输入提示。直写模板规避该问题并保证提示出现。 */
template <typename T>
static void Test_DirectSet(const char* name, INT32 setCmd, INT32 chDefault, const char* structName)
{
    int idx = SelectClient(name);
    if (idx < 0) return;
    if (!g_lpUserID[idx]) { printf("[%s] 客户端%d未登录\n", name, idx+1); return; }
    /* chDefault<0 表示设备级命令, 无需通道号, 跳过输入等待 */
    INT32 nChannel = (chDefault < 0) ? chDefault : ReadChannelParam(chDefault);
    T cfg;
    memset(&cfg, 0, sizeof(cfg));
    ReadStructJson(cfg);
    printf("发送JSON:\n%s\n", SDKConvert::to_string(cfg).c_str());
    char apiCall[128];
    snprintf(apiCall, sizeof(apiCall), "NET_clientSetDevConfig【%d】", setCmd);
    INT32 nRetBytes = 0;
    BOOL ret = NET_clientSetDevConfig(g_lpUserID[idx], nChannel, setCmd, &cfg, sizeof(cfg), &nRetBytes);
    printf("实际调用：%s -> %s\n", apiCall, ret ? "成功" : "失败");
    if (ret) {
        printf("接收JSON: {\"return\":0,\"message\":\"success\"}\n");
    } else {
        printf("接收JSON: {\"return\":%d,\"message\":\"%s\"}\n", NET_clientGetLastError(), NET_clientGetErrorMsg());
        PrintFailure(name);
    }
}

/* ===================== 新增配置测试项 (36-86) ===================== */

/** 36. 流编码配置查询 (NET_GET_STREAMCFG=120) */
static void Test_GetStreamCfg() { TEST_GENERIC_GET(NET_VideoEncodeOption_S, "GetStreamCfg", NET_GET_STREAMCFG, 0); }
/** 37. 流编码配置设置 (NET_SET_STREAMCFG=121) */
static void Test_SetStreamCfg() { TEST_GENERIC_SET(NET_VideoEncodeOption_S, "SetStreamCfg", NET_GET_STREAMCFG, NET_SET_STREAMCFG, 0); }
/** 38. OSD配置查询 (NET_GET_OSDCAPCFG=140) */
static void Test_GetOsdCapCfg() { TEST_GENERIC_GET(NET_VideoOsdCfg_S, "GetOsdCapCfg", NET_GET_OSDCAPCFG, 0); }
/** 39. OSD配置设置 (NET_SET_OSDCAPCFG=141) */
static void Test_SetOsdCapCfg() { TEST_GENERIC_SET(NET_VideoOsdCfg_S, "SetOsdCapCfg", NET_GET_OSDCAPCFG, NET_SET_OSDCAPCFG, 0); }
/** 40. 图像配置查询 (NET_GET_IMAGECFG=160) */
static void Test_GetImageCfg() { TEST_GENERIC_GET(NET_ImageSetting_S, "GetImageCfg", NET_GET_IMAGECFG, 0); }
/** 41. 图像配置设置 (NET_SET_IMAGECFG=161) */
static void Test_SetImageCfg() { TEST_GENERIC_SET(NET_ImageSetting_S, "SetImageCfg", NET_GET_IMAGECFG, NET_SET_IMAGECFG, 0); }
/** 42. 网络配置设置 (NET_SET_NETWORKCFG=171) */
static void Test_SetNetworkCfg() { TEST_GENERIC_SET(NET_NetworkCfgList_S, "SetNetworkCfg", NET_GET_NETWORKCFG, NET_SET_NETWORKCFG, -1); }
/** 113. 网络配置查询 (NET_GET_NETWORKCFG=170) */
static void Test_GetNetworkCfg() { TEST_GENERIC_GET(NET_NetworkCfgList_S, "GetNetworkCfg", NET_GET_NETWORKCFG, -1); }
/** 43. 隐私遮盖查询 (NET_GET_PRIVACYMASKCFG=180) */
static void Test_GetPrivacyMask() { TEST_GENERIC_GET(NET_PrivacyMaskCfg_S, "GetPrivacyMask", NET_GET_PRIVACYMASKCFG, 0); }
/** 44. 隐私遮盖设置 (NET_SET_PRIVACYMASKCFG=181) */
static void Test_SetPrivacyMask() { TEST_GENERIC_SET(NET_PrivacyMaskCfg_S, "SetPrivacyMask", NET_GET_PRIVACYMASKCFG, NET_SET_PRIVACYMASKCFG, 0); }
/** 45. 声音报警查询 (NET_GET_AUDIBLE_ALARM_INFO=494) */
static void Test_GetAudibleAlarm() { TEST_GENERIC_GET(NET_AudibleAlarmInfo_S, "GetAudibleAlarm", NET_GET_AUDIBLE_ALARM_INFO, 0); }
/** 46. 声音报警设置 (NET_SET_AUDIBLE_ALARM_INFO=495) */
static void Test_SetAudibleAlarm() { TEST_GENERIC_SET(NET_AudibleAlarmInfo_S, "SetAudibleAlarm", NET_GET_AUDIBLE_ALARM_INFO, NET_SET_AUDIBLE_ALARM_INFO, 0); }
/** 47. 报警输入查询 (NET_GET_ALARM_INPUT=496) */
static void Test_GetAlarmInput() { TEST_GENERIC_GET(NET_AlarmInputInfoList_S, "GetAlarmInput", NET_GET_ALARM_INPUT_INFO, 0); }
/** 48. 报警输入设置 (NET_SET_ALARM_INPUT=497)
 *  GET 496 返回 List 列表结构, SET 497 写单项结构, 两者缓冲区类型不同:
 *  先取全量列表, 按报警通道号取出单项, 修改后按通道号写回 */
static void Test_SetAlarmInput()
{
    int idx = SelectClient("SetAlarmInput");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) { printf("[SetAlarmInput] 客户端%d未登录\n", idx+1); return; }
    NET_AlarmInputInfoList_S lst;
    memset(&lst, 0, sizeof(lst));
    INT32 nRetBytes = 0;
    if (!NET_clientGetDevConfig(g_lpUserID[idx], 0, NET_GET_ALARM_INPUT_INFO, &lst, sizeof(lst), &nRetBytes)) {
        PrintFailure("SetAlarmInput"); return;
    }
    INT32 nAlarmCh = ReadChannelParam(0);
    NET_AlarmInputInfo_S cfg;
    memset(&cfg, 0, sizeof(cfg));
    if (nAlarmCh >= 0 && nAlarmCh < lst.nAlarmInputCount) {
        cfg = lst.astAlarmInputs[nAlarmCh];
    } else {
        printf("报警输入列表共 %d 路, 通道 %d 越界, 使用零值单项继续\n", lst.nAlarmInputCount, nAlarmCh);
        cfg.nAlarmNumber = nAlarmCh;
    }
    printf("当前配置:\n");
    PrintStructJson("SetAlarmInput", "NET_clientGetDevConfig【496】", "NET_AlarmInputInfo_S", cfg);
    if (g_bWebMode) { ReadStructJson(cfg); }
    else { printf("输入JSON修改(空行=原样写回):\n"); ReadStructJson(cfg); }
    BOOL ret = NET_clientSetDevConfig(g_lpUserID[idx], nAlarmCh, NET_SET_ALARM_INPUT_INFO, &cfg, sizeof(cfg), &nRetBytes);
    printf("实际调用：NET_clientSetDevConfig【497】 -> %s\n", ret ? "成功" : "失败");
    if (!ret) PrintFailure("SetAlarmInput");
}
/** 49. 报警输出查询 (NET_GET_ALARM_OUTPUT=498) */
static void Test_GetAlarmOutput() { TEST_GENERIC_GET(NET_AlarmOutputInfoList_S, "GetAlarmOutput", NET_GET_ALARM_OUTPUT_INFO, 0); }
/** 50. 报警输出设置 (NET_SET_ALARM_OUTPUT=499)
 *  GET 498 返回 List 列表结构, SET 499 写单项结构, 缓冲区类型不同:
 *  先取全量列表, 按报警通道号取出单项, 修改后按通道号写回 */
static void Test_SetAlarmOutput()
{
    int idx = SelectClient("SetAlarmOutput");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) { printf("[SetAlarmOutput] 客户端%d未登录\n", idx+1); return; }
    NET_AlarmOutputInfoList_S lst;
    memset(&lst, 0, sizeof(lst));
    INT32 nRetBytes = 0;
    if (!NET_clientGetDevConfig(g_lpUserID[idx], 0, NET_GET_ALARM_OUTPUT_INFO, &lst, sizeof(lst), &nRetBytes)) {
        PrintFailure("SetAlarmOutput"); return;
    }
    INT32 nAlarmCh = ReadChannelParam(0);
    NET_AlarmOutputInfo_S cfg;
    memset(&cfg, 0, sizeof(cfg));
    if (nAlarmCh >= 0 && nAlarmCh < lst.nAlarmOutputCount) {
        cfg = lst.astAlarmOutputs[nAlarmCh];
    } else {
        printf("报警输出列表共 %d 路, 通道 %d 越界, 使用零值单项继续\n", lst.nAlarmOutputCount, nAlarmCh);
        cfg.nAlarmNumber = nAlarmCh;
    }
    printf("当前配置:\n");
    PrintStructJson("SetAlarmOutput", "NET_clientGetDevConfig【498】", "NET_AlarmOutputInfo_S", cfg);
    if (g_bWebMode) { ReadStructJson(cfg); }
    else { printf("输入JSON修改(空行=原样写回):\n"); ReadStructJson(cfg); }
    BOOL ret = NET_clientSetDevConfig(g_lpUserID[idx], nAlarmCh, NET_SET_ALARM_OUTPUT_INFO, &cfg, sizeof(cfg), &nRetBytes);
    printf("实际调用：NET_clientSetDevConfig【499】 -> %s\n", ret ? "成功" : "失败");
    if (!ret) PrintFailure("SetAlarmOutput");
}
/** 51. 闪光报警查询 (NET_GET_FLASHING_LIGHT=500) */
static void Test_GetFlashingLight() { TEST_GENERIC_GET(NET_FlashingLightAlarmInfo_S, "GetFlashingLight", NET_GET_FLASHING_LIGHT_ALARM_INFO, 0); }
/** 52. 闪光报警设置 (NET_SET_FLASHING_LIGHT=501) */
static void Test_SetFlashingLight() { TEST_GENERIC_SET(NET_FlashingLightAlarmInfo_S, "SetFlashingLight", NET_GET_FLASHING_LIGHT_ALARM_INFO, NET_SET_FLASHING_LIGHT_ALARM_INFO, 0); }
/** 53. PIR报警查询 (NET_GET_PIR_ALARM=502) */
static void Test_GetPirAlarm() { TEST_GENERIC_GET(NET_PirAlarmInfo_S, "GetPirAlarm", NET_GET_PIR_ALARM_INFO, 0); }
/** 54. PIR报警设置 (NET_SET_PIR_ALARM=503) */
static void Test_SetPirAlarm() { TEST_GENERIC_SET(NET_PirAlarmInfo_S, "SetPirAlarm", NET_GET_PIR_ALARM_INFO, NET_SET_PIR_ALARM_INFO, 0); }
/** 55. 安全服务查询 (NET_GET_SECURITY_SERVICES_INFO=465) */
static void Test_GetSecuritySvc() { TEST_GENERIC_GET(NET_SecurityServicesInfo_S, "GetSecuritySvc", NET_GET_SECURITY_SERVICES_INFO, -1); }
/** 56. 安全服务设置 (NET_SET_SECURITY_SERVICES_INFO=466) */
static void Test_SetSecuritySvc() { TEST_GENERIC_SET(NET_SecurityServicesInfo_S, "SetSecuritySvc", NET_GET_SECURITY_SERVICES_INFO, NET_SET_SECURITY_SERVICES_INFO, -1); }
/** 57. SSH倒计时 (NET_GET_SSH_COUNTDOWN=467) */
static void Test_GetSshCountdown() { TEST_GENERIC_GET(NET_SshCountdownInfo_S, "GetSshCountdown", NET_GET_SSH_COUNTDOWN, -1); }
/** 58. 日志服务器查询 (NET_GET_LOG_SERVER=470) */
static void Test_GetLogServer() { TEST_GENERIC_GET(NET_LogServerInfo_S, "GetLogServer", NET_GET_LOG_SERVER, -1); }
/** 59. 日志服务器设置 (NET_SET_LOG_SERVER=471) */
static void Test_SetLogServer() { TEST_GENERIC_SET(NET_LogServerInfo_S, "SetLogServer", NET_GET_LOG_SERVER, NET_SET_LOG_SERVER, -1); }
/** 60. 测试日志服务器 (NET_TEST_LOG_SERVER=472) */
static void Test_TestLogServer() { TEST_GENERIC_SET(NET_LogServerInfo_S, "TestLogServer", NET_GET_LOG_SERVER, NET_TEST_LOG_SERVER, -1); }
/** 61. 录像计划查询 (NET_GET_RECORD_SCHEDULE=475) */
static void Test_GetRecordSchedule() { TEST_GENERIC_GET(NET_RecordSchedule_S, "GetRecordSchedule", NET_GET_RECORD_SCHEDULE, 0); }
/** 62. 录像计划设置 (NET_SET_RECORD_SCHEDULE=476) */
static void Test_SetRecordSchedule() { TEST_GENERIC_SET(NET_RecordSchedule_S, "SetRecordSchedule", NET_GET_RECORD_SCHEDULE, NET_SET_RECORD_SCHEDULE, 0); }
/** 63. 录像高级参数查询 (NET_GET_RECORD_ADVANCED_PARAM=477) */
static void Test_GetRecordAdvParam() { TEST_GENERIC_GET(NET_RecordAdvancedParam_S, "GetRecordAdvParam", NET_GET_RECORD_ADVANCED_PARAM, 0); }
/** 64. 录像高级参数设置 (NET_SET_RECORD_ADVANCED_PARAM=478) */
static void Test_SetRecordAdvParam() { TEST_GENERIC_SET(NET_RecordAdvancedParam_S, "SetRecordAdvParam", NET_GET_RECORD_ADVANCED_PARAM, NET_SET_RECORD_ADVANCED_PARAM, 0); }
/** 65. 查找日志 (NET_FIND_LOG=468) */
static void Test_FindLog() { TEST_GENERIC_GET(NET_LogList_S, "FindLog", NET_FIND_LOG, -1); }
/** 66. 导出日志 (NET_EXPORT_LOG=469) */
static void Test_ExportLog() { TEST_GENERIC_GET(NET_LogList_S, "ExportLog", NET_EXPORT_LOG, -1); }
/** 67. 查找录像文件 (NET_FIND_RECORD_FILE_INFO=479) */
static void Test_FindRecordFile() { TEST_GENERIC_GET(NET_RecordFileList_S, "FindRecordFile", NET_FIND_RECORD_FILE_INFO, 0); }

/* ===================== 异步下载任务管理 (参考 config demo 192/194/195) ===================== */
struct DemoDownloadTask {
    int         id;         /* demo 任务编号, 从1递增 */
    LPVOID      handle;     /* SDK 任务句柄 */
    INT32       channelId;
    std::string date;
    std::string savePath;
};
static std::vector<DemoDownloadTask> g_downloadTasks;
static std::mutex                    g_downloadTaskMutex;
static int                           g_nextDownloadTaskId = 1;

static const char* DemoDownloadStatusName(INT32 status)
{
    switch (status) {
        case NET_DOWNLOAD_TASK_CREATED:   return "CREATED";
        case NET_DOWNLOAD_TASK_RUNNING:   return "RUNNING";
        case NET_DOWNLOAD_TASK_SUCCEEDED: return "SUCCEEDED";
        case NET_DOWNLOAD_TASK_FAILED:    return "FAILED";
        case NET_DOWNLOAD_TASK_CANCELED:  return "CANCELED";
        default: return "UNKNOWN";
    }
}

/* 下载进度回调: 每增加10%打印一次, lpUserData 为该任务 lastPercent 指针(由监控线程回收)。
 * 注意: totalBytes 是服务端估算值(可能偏小, 实测可超), 进度按实际/估算封顶 100% */
static void NET_STDCALL DownloadProgressCallBack(UINT64 dwDownloadedBytes, UINT64 dwTotalBytes, LPVOID lpUserData)
{
    int* pnLastPercent = (int*)lpUserData;
    if (!pnLastPercent || dwTotalBytes == 0) return;
    const UINT64 total = (dwDownloadedBytes > dwTotalBytes) ? dwDownloadedBytes : dwTotalBytes;
    int nPercent = (int)(dwDownloadedBytes * 100 / total);
    if (nPercent > 100) nPercent = 100;
    if (nPercent - *pnLastPercent >= 10) {
        *pnLastPercent = nPercent;
        printf("[Download] 进度: %d%% (%llu/%llu 字节, total为估算值)\n", nPercent,
               (unsigned long long)dwDownloadedBytes, (unsigned long long)dwTotalBytes);
        fflush(stdout);
    }
}

/* 解析时间输入: "HH:MM:SS" 或纯秒数(0~86399), 返回当天0点起的秒数, -1表示无效 */
static INT32 ParseTimeToSeconds(const char* szText)
{
    int h = 0, m = 0, s = 0;
    if (sscanf(szText, "%d:%d:%d", &h, &m, &s) == 3) {
        if (h < 0 || h > 23 || m < 0 || m > 59 || s < 0 || s > 59) return -1;
        return h * 3600 + m * 60 + s;
    }
    int nSec = atoi(szText);
    if (nSec <= 0 || nSec > 86399) return -1;
    return nSec;
}

/** 68. 异步下载录像文件 (NET_clientStartDownloadRecordFile, 功能同 config demo 192) */
static void Test_DownloadRecord()
{
    int idx = SelectClient("异步下载录像文件");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) { printf("[Download] 客户端%d未登录\n", idx+1); return; }

    INT32 nChannel = (INT32)ReadInt("通道号(用35查询结果的ChnId/设备在线IPC通道, 如9; 默认0): ");
    if (nChannel < 0) nChannel = 0;

    char szDate[32] = "";
    char szStart[32] = "";
    char szEnd[32] = "";
    char szSavePath[512] = "";
    ReadString("日期(YYYY-MM-DD, 如2026-09-18): ", szDate, sizeof(szDate));
    ReadString("开始时间(HH:MM:SS或秒): ", szStart, sizeof(szStart));
    ReadString("结束时间(HH:MM:SS或秒, 例23:00:00): ", szEnd, sizeof(szEnd));
    ReadStringDefault("保存路径(含文件名)", szSavePath, sizeof(szSavePath), "/tmp/download.mkv");

    INT32 nStartTime = ParseTimeToSeconds(szStart);
    INT32 nEndTime = ParseTimeToSeconds(szEnd);
    if (nStartTime < 0 || nEndTime < 0 || nEndTime <= nStartTime) {
        printf("[Download] 时间范围无效: 格式HH:MM:SS或秒数, 且结束时间必须晚于开始时间\n");
        return;
    }

    DemoDownloadTask task;
    task.channelId = nChannel;
    task.date = szDate;
    task.savePath = szSavePath;
    int* pnLastPercent = new int(0);  /* 进度回调节流状态, 由监控线程回收 */

    LPVOID hTask = NULL;
    if (!NET_clientStartDownloadRecordFile(g_lpUserID[idx], nChannel, szDate, nStartTime, nEndTime,
                                           szSavePath, DownloadProgressCallBack, pnLastPercent, &hTask)) {
        delete pnLastPercent;
        PrintFailure("[Download] 异步下载启动");
        return;
    }

    {
        std::lock_guard<std::mutex> lock(g_downloadTaskMutex);
        task.id = g_nextDownloadTaskId++;
        task.handle = hTask;
        g_downloadTasks.push_back(task);
    }
    printf("[Download] 异步任务已启动: 下载#%d (channel=%d, %s [%d~%d秒], save=%s)\n",
           task.id, nChannel, szDate, nStartTime, nEndTime, szSavePath);
    printf("[Download] 本命令只负责启动; 输入 221 查看进度, 222 停止最近任务。\n");
    printf("[Download] StartJSON: {\"taskId\":%d,\"channel\":%d,\"date\":\"%s\",\"startTime\":%d,\"endTime\":%d,\"savePath\":\"%s\",\"status\":\"STARTED\"}\n",
           task.id, nChannel, szDate, nStartTime, nEndTime, szSavePath);

    /* 监控线程: 独占等待该句柄结束后回收SDK资源 (与 config demo 192 一致) */
    const int demoTaskId = task.id;
    std::thread([demoTaskId, hTask, pnLastPercent]() {
        const BOOL waited = NET_clientWaitDownloadRecordFile(hTask, 0xFFFFFFFFU);
        INT32 status = NET_DOWNLOAD_TASK_FAILED;
        INT32 taskError = NET_E_TRANSFILE_FAIL;
        UINT64 downloaded = 0, total = 0;
        if (waited) {
            NET_clientGetDownloadRecordFileStatus(hTask, &status, &downloaded, &total);
            NET_clientGetDownloadRecordFileError(hTask, &taskError);
        }
        NET_clientReleaseDownloadRecordFile(hTask);
        delete pnLastPercent;
        std::string strSavePath;
        {
            std::lock_guard<std::mutex> lock(g_downloadTaskMutex);
            for (std::vector<DemoDownloadTask>::iterator it = g_downloadTasks.begin();
                 it != g_downloadTasks.end(); ++it) {
                if (it->id == demoTaskId && it->handle == hTask) {
                    strSavePath = it->savePath;
                    g_downloadTasks.erase(it);
                    break;
                }
            }
        }
        printf("[Download] 下载#%d已结束: %s, error=%d; SDK句柄已自动回收。\n",
               demoTaskId, waited ? DemoDownloadStatusName(status) : "WAIT_FAILED", taskError);
        if (!waited || status == NET_DOWNLOAD_TASK_FAILED) {
            printf("[Download] 失败常见原因: 1)通道号无录像/IPC不在线(在线通道见服务端日志CHANNEL_STATUS)\n"
                   "                  2)该时间段无录像(服务端RFS返回-20=无录像, result=-20)\n"
                   "                  3)未先用35查询到实际录像时间段\n");
        }
        printf("[Download] ResultJSON: {\"taskId\":%d,\"status\":%d,\"statusName\":\"%s\",\"error\":%d,\"downloaded\":%llu,\"totalBytes\":%llu,\"savePath\":\"%s\"}\n",
               demoTaskId, status, waited ? DemoDownloadStatusName(status) : "WAIT_FAILED",
               taskError, (unsigned long long)downloaded, (unsigned long long)total,
               strSavePath.c_str());
        fflush(stdout);
    }).detach();
}

/** 221. 查看下载任务进度 (NET_clientGetDownloadRecordFileStatus, 功能同 config demo 194) */
static void Test_ListDownloadTasks()
{
    std::lock_guard<std::mutex> lock(g_downloadTaskMutex);
    if (g_downloadTasks.empty()) {
        printf("[Download] 当前没有进行中的下载任务。\n");
        printf("[Download] TasksJSON: []\n");
        return;
    }
    std::string strJson;
    for (size_t i = 0; i < g_downloadTasks.size(); ++i) {
        const DemoDownloadTask& task = g_downloadTasks[i];
        INT32 status = -1;
        UINT64 downloaded = 0, total = 0;
        const BOOL bQueryOk = NET_clientGetDownloadRecordFileStatus(task.handle, &status, &downloaded, &total);
        const UINT64 percent = (total > 0) ? downloaded * 100 / total : 0;
        /* 过程文本打印(保留) */
        if (bQueryOk) {
            printf("[Download] 下载#%d: %s %llu%% (%llu/%llu字节), chn=%d, %s -> %s\n",
                   task.id, DemoDownloadStatusName(status),
                   (unsigned long long)percent,
                   (unsigned long long)downloaded, (unsigned long long)total,
                   task.channelId, task.date.c_str(), task.savePath.c_str());
        } else {
            printf("[Download] 下载#%d: 句柄查询失败(可能已结束待回收)\n", task.id);
        }
        /* 收集 JSON 片段, 最后统一输出 */
        char szItem[512];
        snprintf(szItem, sizeof(szItem),
                 "%s{\"taskId\":%d,\"queryOk\":%s,\"status\":%d,\"statusName\":\"%s\",\"percent\":%llu,\"downloaded\":%llu,\"totalBytes\":%llu,\"channel\":%d,\"date\":\"%s\",\"savePath\":\"%s\"}",
                 (i ? "," : ""), task.id, bQueryOk ? "true" : "false", status,
                 DemoDownloadStatusName(status),
                 (unsigned long long)percent,
                 (unsigned long long)downloaded, (unsigned long long)total,
                 task.channelId, task.date.c_str(), task.savePath.c_str());
        strJson += szItem;
    }
    printf("[Download] TasksJSON: [%s]\n", strJson.c_str());
}

/** 222. 停止最近启动的下载任务 (NET_clientCancelDownloadRecordFile, 功能同 config demo 195) */
static void Test_CancelDownloadTask()
{
    LPVOID hTask = NULL;
    int taskId = 0;
    {
        std::lock_guard<std::mutex> lock(g_downloadTaskMutex);
        if (g_downloadTasks.empty()) {
            printf("[Download] 当前没有可停止的下载任务。\n");
            return;
        }
        taskId = g_downloadTasks.back().id;
        hTask = g_downloadTasks.back().handle;
    }
    /* 只发停止请求; 句柄由启动时的监控线程独占等待并回收, 避免与其竞争 */
    const BOOL bCancelOk = NET_clientCancelDownloadRecordFile(hTask);
    if (!bCancelOk) {
        printf("[Download] 下载#%d停止请求失败, error=%d\n", taskId, NET_clientGetLastError());
    } else {
        printf("[Download] 下载#%d已提交停止请求; 结束后SDK资源会自动回收。\n", taskId);
    }
    printf("[Download] CancelJSON: {\"taskId\":%d,\"cancelRequested\":%s,\"errorCode\":%d}\n",
           taskId, bCancelOk ? "true" : "false", bCancelOk ? 0 : NET_clientGetLastError());
}

/** 218. 设备端抓图 JPEG (NET_GET_CAPTURE_PICTURE=580, 参考 config demo 580) */
static void Test_CapturePicture()
{
    int idx = SelectClient("设备端抓图(JPEG)");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) { printf("[Capture] 客户端%d未登录\n", idx+1); return; }

    INT32 nChannel = (INT32)ReadInt("通道号(从0开始, 默认0): ");
    if (nChannel < 0) nChannel = 0;

    /* 结构体含1MB图片缓冲, static避免栈溢出 */
    static NET_CapturePictureInfo_S stInfo;
    memset(&stInfo, 0, sizeof(stInfo));
    stInfo.nChannel = nChannel;
    stInfo.nStreamType = NET_CUSTOM_STREAM_MAIN;
    stInfo.nPicFormat = NET_CAPTURE_PICTURE_FORMAT_JPEG;
    stInfo.nOutputType = NET_CAPTURE_OUTPUT_MEMORY;
    SafeCopy(stInfo.szFilePath, sizeof(stInfo.szFilePath), "/tmp/sdk_capture.jpg");

    INT32 dwBytesReturned = 0;
    printf("[Capture] 调用 NET_clientGetDevConfig 抓图, channel=%d, JPEG, 内存返回...\n", nChannel);
    BOOL ret = NET_clientGetDevConfig(g_lpUserID[idx], nChannel, NET_GET_CAPTURE_PICTURE,
                                      &stInfo, (INT32)sizeof(stInfo), &dwBytesReturned);
    if (!ret) {
        PrintFailure("[Capture] 抓图");
        printf("[Capture] ResultJSON: {\"success\":false,\"channel\":%d,\"errorCode\":%d,\"errorMsg\":\"%s\"}\n",
               nChannel, NET_clientGetLastError(), NET_clientGetErrorMsg());
        return;
    }

    printf("[Capture] 抓图成功! PicLen=%u\n", stInfo.uPicLen);
    BOOL bSaved = FALSE;
    if (stInfo.uPicLen > 0 && stInfo.abyPicData[0] != 0) {
        printf("[Capture] 图片数据首字节: 0x%02X (JPEG应为 0xFF)\n", stInfo.abyPicData[0]);
        FILE* fp = fopen("/tmp/sdk_capture.jpg", "wb");
        if (fp) {
            fwrite(stInfo.abyPicData, 1, stInfo.uPicLen, fp);
            fclose(fp);
            bSaved = TRUE;
            printf("[Capture] 图片已落盘: /tmp/sdk_capture.jpg\n");
        }
    }
    printf("[Capture] ResultJSON: {\"success\":true,\"channel\":%d,\"picFormat\":\"JPEG\",\"outputType\":\"MEMORY\",\"picLen\":%u,\"firstByte\":\"0x%02X\",\"saved\":%s,\"savedPath\":\"/tmp/sdk_capture.jpg\"}\n",
           nChannel, stInfo.uPicLen,
           stInfo.uPicLen > 0 ? stInfo.abyPicData[0] : 0, bSaved ? "true" : "false");
}
/** 69. 音频异常侦测查询 (NET_GET_AUDIOANOMALYALARM=222) */
static void Test_GetAudioAnomaly() { TEST_GENERIC_GET(NET_AudioAnomalyAlarmInfo_S, "GetAudioAnomaly", NET_GET_AUDIOANOMALYALARM, 0); }
/** 70. 音频异常侦测设置 (NET_SET_AUDIOANOMALYALARM=223) */
static void Test_SetAudioAnomaly() { TEST_GENERIC_SET(NET_AudioAnomalyAlarmInfo_S, "SetAudioAnomaly", NET_GET_AUDIOANOMALYALARM, NET_SET_AUDIOANOMALYALARM, 0); }
/** 71. 预览信息查询 (NET_GET_PREVIEW_INFO=224) */
static void Test_GetPreviewInfo() { TEST_GENERIC_GET(NET_PreviewInfo_S, "GetPreviewInfo", NET_GET_PREVIEW_INFO, 0); }
/** 72. 预览信息设置 (NET_SET_PREVIEW_INFO=225) */
static void Test_SetPreviewInfo() { TEST_GENERIC_SET(NET_PreviewInfo_S, "SetPreviewInfo", NET_GET_PREVIEW_INFO, NET_SET_PREVIEW_INFO, 0); }
/** 73. 抓图计划查询 (NET_GET_CAPTURE_PLAN_INFO=208) */
static void Test_GetCapturePlan() { TEST_GENERIC_GET(NET_CapturePlanInfo_S, "GetCapturePlan", NET_GET_CAPTURE_PLAN_INFO, 0); }
/** 74. 抓图计划设置 (NET_SET_CAPTURE_PLAN_INFO=209) */
static void Test_SetCapturePlan() { TEST_GENERIC_SET(NET_CapturePlanInfo_S, "SetCapturePlan", NET_GET_CAPTURE_PLAN_INFO, NET_SET_CAPTURE_PLAN_INFO, 0); }
/** 75. 抓图参数查询 (NET_GET_CAPTURE_PARAM_INFO=210) */
static void Test_GetCaptureParam() { TEST_GENERIC_GET(NET_CaptureParamInfo_S, "GetCaptureParam", NET_GET_CAPTURE_PARAM_INFO, 0); }
/** 76. 抓图参数设置 (NET_SET_CAPTURE_PARAM_INFO=211) */
static void Test_SetCaptureParam() { TEST_GENERIC_SET(NET_CaptureParamInfo_S, "SetCaptureParam", NET_GET_CAPTURE_PARAM_INFO, NET_SET_CAPTURE_PARAM_INFO, 0); }
/** 77. 人脸抓拍查询 (NET_GET_FACECAPTUREINFO=246) */
static void Test_GetFaceCapture() { TEST_GENERIC_GET(NET_FaceCaptureInfo_S, "GetFaceCapture", NET_GET_FACECAPTUREINFO, 0); }
/** 78. 人脸抓拍设置 (NET_SET_FACECAPTUREINFO=247) */
static void Test_SetFaceCapture() { TEST_GENERIC_SET(NET_FaceCaptureInfo_S, "SetFaceCapture", NET_GET_FACECAPTUREINFO, NET_SET_FACECAPTUREINFO, 0); }
/** 79. 人脸比对设置 (NET_SET_FACE_COMPARE_INFO=482) */
static void Test_SetFaceCompare() { TEST_GENERIC_SET(NET_FaceCompareInfo_S, "SetFaceCompare", NET_GET_FACECAPTUREINFO, NET_SET_FACE_COMPARE_INFO, 0); }
/** 80. 人流统计查询 (NET_GET_PEOPLE_FLOW_STATISTICS_CFG=408) */
static void Test_GetPeopleFlow() { TEST_GENERIC_GET(NET_PeopleFlowStatisticsCfg_S, "GetPeopleFlow", NET_GET_PEOPLE_FLOW_STATISTICS_CFG, 0); }
/** 81. 人流统计设置 (NET_SET_PEOPLE_FLOW_STATISTICS_CFG=409) */
static void Test_SetPeopleFlow() { TEST_GENERIC_SET(NET_PeopleFlowStatisticsCfg_S, "SetPeopleFlow", NET_GET_PEOPLE_FLOW_STATISTICS_CFG, NET_SET_PEOPLE_FLOW_STATISTICS_CFG, 0); }
/** 82. 人员密度查询 (NET_GET_PEOPLE_DENSITY_DETECTION_CFG=411) */
static void Test_GetPeopleDensity() { TEST_GENERIC_GET(NET_PeopleDensityDetectionCfg_S, "GetPeopleDensity", NET_GET_PEOPLE_DENSITY_DETECTION_CFG, 0); }
/** 83. 人员密度设置 (NET_SET_PEOPLE_DENSITY_DETECTION_CFG=412) */
static void Test_SetPeopleDensity() { TEST_GENERIC_SET(NET_PeopleDensityDetectionCfg_S, "SetPeopleDensity", NET_GET_PEOPLE_DENSITY_DETECTION_CFG, NET_SET_PEOPLE_DENSITY_DETECTION_CFG, 0); }
/** 84. 音频配置设置 (NET_SET_AUDIOCFG=131) */
static void Test_SetAudioCfg() { TEST_GENERIC_SET(NET_AudioCfg_S, "SetAudioCfg", NET_GET_AUDIOCFG, NET_SET_AUDIOCFG, 0); }
/** 85. 对讲音频配置查询 (NET_GET_VOICECOM_AUDIO_CFG=491) */
static void Test_GetVoiceComCfg() { TEST_GENERIC_GET(NET_VoiceComAudioCfg_S, "GetVoiceComCfg", NET_GET_VOICECOM_AUDIO_CFG, 0); }
/** 86. 对讲音频配置设置 (NET_SET_VOICECOM_AUDIO_CFG=492) */
static void Test_SetVoiceComCfg() { TEST_GENERIC_SET(NET_VoiceComAudioCfg_S, "SetVoiceComCfg", NET_GET_VOICECOM_AUDIO_CFG, NET_SET_VOICECOM_AUDIO_CFG, 0); }

/* ===================== IPC图像参数 (87-96) ===================== */

/** 87. 曝光查询 (NET_GET_EXPOSURE_INFO=212) */
static void Test_GetExposure() { TEST_GENERIC_GET(NET_ExposureInfo_S, "GetExposure", NET_GET_EXPOSURE_INFO, 0); }
/** 88. 曝光设置 (NET_SET_EXPOSURE_INFO=213) */
static void Test_SetExposure() { TEST_GENERIC_SET(NET_ExposureInfo_S, "SetExposure", NET_GET_EXPOSURE_INFO, NET_SET_EXPOSURE_INFO, 0); }
/** 89. 日夜转换查询 (NET_GET_DAYNIGHT_INFO=214) */
static void Test_GetDayNight() { TEST_GENERIC_GET(NET_DayNightInfo_S, "GetDayNight", NET_GET_DAYNIGHT_INFO, 0); }
/** 90. 日夜转换设置 (NET_SET_DAYNIGHT_INFO=215) */
static void Test_SetDayNight() { TEST_GENERIC_SET(NET_DayNightInfo_S, "SetDayNight", NET_GET_DAYNIGHT_INFO, NET_SET_DAYNIGHT_INFO, 0); }
/** 91. 背光查询 (NET_GET_BACKLIGHT_INFO=216) */
static void Test_GetBackLight() { TEST_GENERIC_GET(NET_BackLightInfo_S, "GetBackLight", NET_GET_BACKLIGHT_INFO, 0); }
/** 92. 背光设置 (NET_SET_BACKLIGHT_INFO=217) */
static void Test_SetBackLight() { TEST_GENERIC_SET(NET_BackLightInfo_S, "SetBackLight", NET_GET_BACKLIGHT_INFO, NET_SET_BACKLIGHT_INFO, 0); }
/** 93. 降噪查询 (NET_GET_DENOISE_INFO=218) */
static void Test_GetDenoise() { TEST_GENERIC_GET(NET_DenoiseInfo_S, "GetDenoise", NET_GET_DENOISE_INFO, 0); }
/** 94. 降噪设置 (NET_SET_DENOISE_INFO=219) */
static void Test_SetDenoise() { TEST_GENERIC_SET(NET_DenoiseInfo_S, "SetDenoise", NET_GET_DENOISE_INFO, NET_SET_DENOISE_INFO, 0); }
/** 95. 白平衡查询 (NET_GET_WHITEBALANCE_INFO=220) */
static void Test_GetWhiteBalance() { TEST_GENERIC_GET(NET_WhiteBalanceInfo_S, "GetWhiteBalance", NET_GET_WHITEBALANCE_INFO, 0); }
/** 96. 白平衡设置 (NET_SET_WHITEBALANCE_INFO=221) */
static void Test_SetWhiteBalance() { TEST_GENERIC_SET(NET_WhiteBalanceInfo_S, "SetWhiteBalance", NET_GET_WHITEBALANCE_INFO, NET_SET_WHITEBALANCE_INFO, 0); }

/** 223. 音频异常实时音量 (NET_GET_AUDIO_ANOMALY_CURRENT_DB=505) */
static void Test_GetAudioAnomalyCurrentDb() { TEST_GENERIC_GET(NET_AudioAnomalyCurrentDb_S, "GetAudioAnomalyCurrentDb", NET_GET_AUDIO_ANOMALY_CURRENT_DB, 0); }

/* ===================== 云台与预置位 (528/529) ===================== */

/** 528. 云台控制 (NET_CONTROL_CAMERA=528, 设备级: 通道由结构体 ID 字段携带)
 *  Type: 1上 2下 3左 4右 5放大 6缩小 7设置预置位 8调用预置位 9停止转动 10停止拉伸聚焦 */
static void Test_ControlCamera() { TEST_DIRECT_SET(NET_CameraControlInfo_S, "ControlCamera", NET_CONTROL_CAMERA, -1); }
/** 529. 查询预置位列表 (NET_CONTROL_PRESET_BIT=529 GET) */
static void Test_GetPresetBit() { TEST_GENERIC_GET(NET_PresetBitInfo_S, "GetPresetBit", NET_CONTROL_PRESET_BIT, 0); }
/** 529. 预置位控制 (NET_CONTROL_PRESET_BIT=529 SET, 设备级: 通道由结构体 CameraId 字段携带)
 *  Type: 0-设置 1-删除 2-调用 3-改名(NVR 端暂不支持) */
static void Test_ControlPresetBit() { TEST_DIRECT_SET(NET_PresetBitCtrl_S, "ControlPresetBit", NET_CONTROL_PRESET_BIT, -1); }

/* ===================== 人脸目标库/人脸信息 (97-104) ===================== */

/** 97. 添加目标库 (NET_ADD_TARGET_LIB=483) */
static void Test_AddTargetLib() { TEST_DIRECT_SET(NET_FaceLibInfo_S, "AddTargetLib", NET_ADD_TARGET_LIB, -1); }
/** 98. 删除目标库 (NET_DEL_TARGET_LIB=484) */
static void Test_DelTargetLib() { TEST_DIRECT_SET(NET_FaceLibInfo_S, "DelTargetLib", NET_DEL_TARGET_LIB, -1); }
/** 99. 修改目标库 (NET_SET_TARGET_LIB=485) */
static void Test_SetTargetLib() { TEST_DIRECT_SET(NET_FaceLibInfo_S, "SetTargetLib", NET_SET_TARGET_LIB, -1); }
/** 100. 获取目标库 (NET_GET_TARGET_LIB=486) */
static void Test_GetTargetLib() { TEST_GENERIC_GET(NET_FaceLibList_S, "GetTargetLib", NET_GET_TARGET_LIB, -1); }
/** 101. 添加人脸 (NET_ADD_FACE_INFO=487) */
static void Test_AddFaceInfo() { TEST_DIRECT_SET(NET_FaceInfo_S, "AddFaceInfo", NET_ADD_FACE_INFO, -1); }
/** 102. 删除人脸 (NET_DEL_FACE_INFO=488) */
static void Test_DelFaceInfo() { TEST_DIRECT_SET(NET_FaceIdInfo_S, "DelFaceInfo", NET_DEL_FACE_INFO, -1); }
/** 103. 修改人脸 (NET_SET_FACE_INFO=489) */
static void Test_SetFaceInfo() { TEST_DIRECT_SET(NET_FaceInfo_S, "SetFaceInfo", NET_SET_FACE_INFO, -1); }
/** 104. 获取人脸 (NET_GET_FACE_INFO=490)
 *  需先输入 LibId(JSON: {"LibId":"6666"})，通过 WithBody 机制带入 GET 请求体，
 *  由服务端解析后作为人脸库ID下发给设备。 */
static void Test_GetFaceInfo()
{
    int idx = SelectClient("GetFaceInfo");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) { printf("[GetFaceInfo] 客户端%d未登录\n", idx+1); return; }
    /* 设备级命令, 无需通道号 */
    INT32 nChannel = -1;
    NET_FaceInfoList_S cfg;
    memset(&cfg, 0, sizeof(cfg));
    ReadStructJson(cfg);
    INT32 nRetBytes = 0;
    BOOL ret = NET_clientGetDevConfig(g_lpUserID[idx], nChannel, NET_GET_FACE_INFO, &cfg, sizeof(cfg), &nRetBytes);
    if (!ret) { PrintFailure("GetFaceInfo"); return; }
    PrintStructJson("GetFaceInfo", "NET_clientGetDevConfig【490】", "NET_FaceInfoList_S", cfg);
}

/* ===================== 系统能力 (105) ===================== */

/** 105. 系统能力总表 (NET_CAP_SYS=100) */
static void Test_GetSysCap()
{
    int idx = SelectClient("系统能力总表");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) { printf("[SysCap] 客户端%d未登录\n", idx+1); return; }

    /* 系统能力总表为设备级能力, 回调忽略 channel 值, 但服务端要求 channel>=0,
     * 故固定使用 0 静默查询, 不再提示输入 channel */
    INT32 nChannel = 0;
    NET_SysCapability_S cap;
    memset(&cap, 0, sizeof(cap));
    INT32 nRetBytes = 0;
    BOOL ret = NET_clientGetDeviceCapability(g_lpUserID[idx], nChannel, NET_CAP_SYS, &cap, sizeof(cap), &nRetBytes);
    if (!ret) { PrintFailure("[SysCap] 查询"); return; }
    PrintStructJson("Test_GetSysCap", "NET_clientGetDeviceCapability【NET_CAP_SYS=100】", "NET_SysCapability_S", cap);
}

/* ===================== 新增测试函数 (106-121) ===================== */

/** 106. 远程重启 (NET_CONTROL_REBOOT=535) */
static void Test_ControlReboot()
{
    int idx = SelectClient("远程重启");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) { printf("[Reboot] 客户端%d未登录\n", idx+1); return; }

    NET_RebootInfo_S info;
    memset(&info, 0, sizeof(info));

    printf("[Reboot] 即将重启设备...\n");
    printf("测试接口：Test_ControlReboot\n");
    INT32 nRetBytes = 0;
    BOOL ret = NET_clientSetDevConfig(g_lpUserID[idx], -1, NET_CONTROL_REBOOT, &info, sizeof(info), &nRetBytes);
    printf("实际调用：NET_clientSetDevConfig【535】 -> %s\n", ret ? "成功" : "失败");
    if (!ret) PrintFailure("[Reboot]");
}

/** 107. 远程关机 (NET_CONTROL_SHUTDOWN=540) */
static void Test_ControlShutdown()
{
    int idx = SelectClient("远程关机");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) { printf("[Shutdown] 客户端%d未登录\n", idx+1); return; }

    NET_ShutdownInfo_S info;
    memset(&info, 0, sizeof(info));
    if (g_bWebMode) {
        ReadStructJson(info);
    } else {
        info.nDelaySeconds = ReadInt("延迟关机秒数(0=立即): ");
        if (info.nDelaySeconds < 0) info.nDelaySeconds = 0;
    }

    INT32 nRetBytes = 0;
    printf("测试接口：Test_ControlShutdown\n");
    BOOL ret = NET_clientSetDevConfig(g_lpUserID[idx], -1, NET_CONTROL_SHUTDOWN, &info, sizeof(info), &nRetBytes);
    printf("实际调用：NET_clientSetDevConfig【540】 -> %s\n", ret ? "成功" : "失败");
    if (!ret) PrintFailure("Shutdown");
}

/** 108. 格式化硬盘 (NET_CONTROL_FORMAT_DISK=541) */
static void Test_ControlFormatDisk()
{
    int idx = SelectClient("格式化硬盘");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) { printf("[FormatDisk] 客户端%d未登录\n", idx+1); return; }

    NET_FormatDiskInfo_S info;
    memset(&info, 0, sizeof(info));
    if (g_bWebMode) {
        ReadStructJson(info);
    } else {
        info.nDiskIndex = ReadInt("硬盘索引(-1=全部): ");
        info.nFormatType = ReadInt("格式化类型(0=快速,1=完全): ");
    }

    INT32 nRetBytes = 0;
    printf("测试接口：Test_ControlFormatDisk\n");
    BOOL ret = NET_clientSetDevConfig(g_lpUserID[idx], -1, NET_CONTROL_FORMAT_DISK, &info, sizeof(info), &nRetBytes);
    printf("实际调用：NET_clientSetDevConfig【541】 -> %s\n", ret ? "成功" : "失败");
    if (!ret) PrintFailure("FormatDisk");
}

/** 109. 获取设备状态 (NET_GET_DEVICE_STATUS=542) */
static void Test_GetDeviceStatus()
{
    int idx = SelectClient("设备状态");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) { printf("[DevStatus] 客户端%d未登录\n", idx+1); return; }

    NET_DeviceStatusInfo_S status;
    memset(&status, 0, sizeof(status));
    INT32 nRetBytes = 0;
    BOOL ret = NET_clientGetDevConfig(g_lpUserID[idx], -1, NET_GET_DEVICE_STATUS, &status, sizeof(status), &nRetBytes);
    if (!ret) { PrintFailure("[DevStatus] 查询"); return; }
    PrintStructJson("Test_GetDeviceStatus", "NET_clientGetDevConfig【542】", "NET_DeviceStatusInfo_S", status);
}

/** 110. 恢复默认参数 (NET_CONTROL_RESET=544) */
static void Test_ResetDefault()
{
    int idx = SelectClient("恢复默认");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) { printf("[Reset] 客户端%d未登录\n", idx+1); return; }

    NET_ResetInfo_S info;
    memset(&info, 0, sizeof(info));
    info.nResetType = NET_RESET_CTRL_SIMPLE;  /* 默认简单恢复(不含IP/用户名/密码) */
    if (g_bWebMode) {
        /* Web 模式从预填 JSON 示例读取重置类型, 未提供时保留默认 SIMPLE;
         * 必须读取一行输入, 否则该 JSON 行会残留在 stdin 影响下一条命令 */
        ReadStructJson(info);
    } else {
        printf("  1=完全恢复(含IP) 2=简单恢复\n");
        info.nResetType = ReadInt("选择重置类型: ");
    }

    printf("[Reset] 即将恢复默认参数...\n");
    printf("测试接口：Test_ResetDefault\n");
    INT32 nRetBytes = 0;
    BOOL ret = NET_clientSetDevConfig(g_lpUserID[idx], -1, NET_CONTROL_RESET, &info, sizeof(info), &nRetBytes);
    printf("实际调用：NET_clientSetDevConfig【544】 -> %s\n", ret ? "成功" : "失败");
    printf("类型=%d\n", info.nResetType);
    if (!ret) PrintFailure("[Reset]");
}

/** 111. 查询通道名称 (复用 NET_GET_CHANNEL_INFO=300, 读 szChannelName) */
static void Test_GetChannelName()
{
    int idx = SelectClient("查询通道名称");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) { printf("[ChName] 客户端%d未登录\n", idx+1); return; }

    INT32 nChannel = ReadChannelParam(0);
    NET_ChannelList_S list;
    memset(&list, 0, sizeof(list));
    INT32 nRetBytes = 0;
    BOOL ret = NET_clientGetDevConfig(g_lpUserID[idx], nChannel, NET_GET_CHANNEL_INFO, &list, sizeof(list), &nRetBytes);
    if (!ret) { PrintFailure("[ChName] 查询"); return; }
    printf("测试接口：Test_GetChannelName\n");
    printf("实际调用：NET_clientGetDevConfig【300】\n");
    printf("通道数: %u\n", list.uChannelCount);
    UINT32 count = list.uChannelCount;
    if (count > NET_MAX_CHANNEL_NUM) count = NET_MAX_CHANNEL_NUM;
    for (UINT32 i = 0; i < count; i++) {
        printf("  通道%u: %s\n", list.stChannels[i].uChannel, list.stChannels[i].szChannelName);
    }
}

/** 112. 设置通道名称 (NET_SET_CHANNEL_NAME=543)
 *  GET 300 返回通道列表, SET 543 写单项, 先 GET 取当前名称再修改 */
static void Test_SetChannelName()
{
    int idx = SelectClient("设置通道名称");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) { printf("[ChName] 客户端%d未登录\n", idx+1); return; }

    INT32 nChannel = ReadChannelParam(0);

    /* GET 当前通道列表, 取出目标通道的现有名称 */
    NET_ChannelList_S* pList = (NET_ChannelList_S*)calloc(1, sizeof(NET_ChannelList_S));
    if (!pList) { printf("[ChName] 内存分配失败\n"); return; }
    INT32 nRetBytes = 0;
    BOOL ret = NET_clientGetDevConfig(g_lpUserID[idx], nChannel, NET_GET_CHANNEL_INFO, pList, sizeof(*pList), &nRetBytes);
    if (!ret) { PrintFailure("[ChName] 查询当前配置"); free(pList); return; }

    NET_ChannelNameInfo_S info;
    memset(&info, 0, sizeof(info));
    info.uChannel = (UINT32)nChannel;
    /* 从列表中查找目标通道 */
    UINT32 count = pList->uChannelCount;
    if (count > NET_MAX_CHANNEL_NUM) count = NET_MAX_CHANNEL_NUM;
    for (UINT32 i = 0; i < count; i++) {
        if ((INT32)pList->stChannels[i].uChannel == nChannel) {
            SafeCopy(info.szChannelName, sizeof(info.szChannelName), pList->stChannels[i].szChannelName);
            break;
        }
    }
    printf("当前配置:\n");
    PrintStructJson("Test_SetChannelName", "NET_clientGetDevConfig【300】", "NET_ChannelNameInfo_S", info);
    free(pList);

    /* 修改通道名称 */
    if (g_bWebMode) {
        ReadStructJson(info);
    } else {
        char name[64];
        ReadStringDefault("通道名称", name, sizeof(name), info.szChannelName);
        SafeCopy(info.szChannelName, sizeof(info.szChannelName), name);
    }

    printf("测试接口：Test_SetChannelName\n");
    nRetBytes = 0;
    ret = NET_clientSetDevConfig(g_lpUserID[idx], 0, NET_SET_CHANNEL_NAME, &info, sizeof(info), &nRetBytes);
    printf("实际调用：NET_clientSetDevConfig【543】 -> %s\n", ret ? "成功" : "失败");
    printf("通道%u\n", info.uChannel);
    if (!ret) PrintFailure("SetChannelName");
}

/** 115. 建立透明通道 (NET_OPEN_TRANSPARENT_CHANNEL=550) */
static void Test_OpenTransparentChannel()
{
    int idx = SelectClient("建立透明通道");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) { printf("[TransCh] 客户端%d未登录\n", idx+1); return; }

    NET_TransparentChannel_S info;
    memset(&info, 0, sizeof(info));
    if (g_bWebMode) {
        ReadStructJson(info);
    } else {
        info.nChannelIndex = ReadInt("透明通道索引: ");
        info.nSerialPortIndex = ReadInt("关联串口索引: ");
        info.nBaudRate = ReadInt("波特率(默认9600): ");
        if (info.nBaudRate <= 0) info.nBaudRate = 9600;
    }

    printf("测试接口：Test_OpenTransparentChannel\n");
    INT32 nRetBytes = 0;
    BOOL ret = NET_clientSetDevConfig(g_lpUserID[idx], 0, NET_OPEN_TRANSPARENT_CHANNEL, &info, sizeof(info), &nRetBytes);
    printf("实际调用：NET_clientSetDevConfig【550】 -> %s\n", ret ? "成功" : "失败");
    printf("通道%d\n", info.nChannelIndex);
    if (!ret) PrintFailure("OpenTransparentChannel");
}

/** 116. 断开透明通道 (NET_CLOSE_TRANSPARENT_CHANNEL=551) */
static void Test_CloseTransparentChannel()
{
    int idx = SelectClient("断开通道");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) { printf("[TransCh] 客户端%d未登录\n", idx+1); return; }

    NET_TransparentChannel_S info;
    memset(&info, 0, sizeof(info));
    if (g_bWebMode) {
        ReadStructJson(info);
    } else {
        info.nChannelIndex = ReadInt("透明通道索引: ");
    }

    printf("测试接口：Test_CloseTransparentChannel\n");
    INT32 nRetBytes = 0;
    BOOL ret = NET_clientSetDevConfig(g_lpUserID[idx], 0, NET_CLOSE_TRANSPARENT_CHANNEL, &info, sizeof(info), &nRetBytes);
    printf("实际调用：NET_clientSetDevConfig【551】 -> %s\n", ret ? "成功" : "失败");
    printf("通道%d\n", info.nChannelIndex);
    if (!ret) PrintFailure("CloseTransparentChannel");
}

/** 117. 发送透明通道数据 (NET_SEND_TRANSPARENT_DATA=552) */
static void Test_SendTransparentData()
{
    int idx = SelectClient("发送通道数据");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) { printf("[TransData] 客户端%d未登录\n", idx+1); return; }

    NET_TransparentData_S data;
    memset(&data, 0, sizeof(data));
    if (g_bWebMode) {
        ReadStructJson(data);
    } else {
        data.nChannelIndex = ReadInt("透明通道索引: ");
        char strData[1024];
        ReadString("数据(字符串)", strData, sizeof(strData));
        size_t len = strlen(strData);
        std::memcpy(data.byData, strData, len);
        data.uDataLen = static_cast<UINT32>(len);
    }

    printf("测试接口：Test_SendTransparentData\n");
    INT32 nRetBytes = 0;
    BOOL ret = NET_clientSetDevConfig(g_lpUserID[idx], 0, NET_SEND_TRANSPARENT_DATA, &data, sizeof(data), &nRetBytes);
    printf("实际调用：NET_clientSetDevConfig【552】 -> %s\n", ret ? "成功" : "失败");
    printf("通道%d, %u字节\n", data.nChannelIndex, data.uDataLen);
    if (!ret) PrintFailure("SendTransparentData");
}

/** 118. 查询串口参数 (NET_GET_SERIAL_PORT_PARAM=560) */
static void Test_GetSerialPortParam()
{
    int idx = SelectClient("查询串口参数");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) { printf("[Serial] 客户端%d未登录\n", idx+1); return; }

    /* 设备级命令, 无需通道号 */
    INT32 nChannel = -1;
    NET_SerialPortParam_S param;
    memset(&param, 0, sizeof(param));
    if (!g_bWebMode) {
        param.nPortIndex = ReadInt("串口号(0=COM1): ");
    }
    INT32 nRetBytes = 0;
    BOOL ret = NET_clientGetDevConfig(g_lpUserID[idx], nChannel, NET_GET_SERIAL_PORT_PARAM, &param, sizeof(param), &nRetBytes);
    if (!ret) { PrintFailure("[Serial] 查询"); return; }
    PrintStructJson("Test_GetSerialPortParam", "NET_clientGetDevConfig【560】", "NET_SerialPortParam_S", param);
}

/** 119. 发送串口数据 (NET_SEND_SERIAL_DATA=561) */
static void Test_SendSerialData()
{
    int idx = SelectClient("发送串口数据");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) { printf("[SerialData] 客户端%d未登录\n", idx+1); return; }

    NET_SerialData_S data;
    memset(&data, 0, sizeof(data));
    if (g_bWebMode) {
        ReadStructJson(data);
    } else {
        data.nPortIndex = ReadInt("串口号(0=COM1): ");
        char strData[1024];
        ReadString("数据(字符串)", strData, sizeof(strData));
        size_t len = strlen(strData);
        std::memcpy(data.byData, strData, len);
        data.uDataLen = static_cast<UINT32>(len);
    }

    printf("测试接口：Test_SendSerialData\n");
    INT32 nRetBytes = 0;
    BOOL ret = NET_clientSetDevConfig(g_lpUserID[idx], 0, NET_SEND_SERIAL_DATA, &data, sizeof(data), &nRetBytes);
    printf("实际调用：NET_clientSetDevConfig【561】 -> %s\n", ret ? "成功" : "失败");
    printf("COM%d, %u字节\n", data.nPortIndex, data.uDataLen);
    if (!ret) PrintFailure("SendSerialData");
}

/** 120. 直接发送232数据 (NET_SEND_RS232_DATA=562) */
static void Test_SendRs232Data()
{
    int idx = SelectClient("直接发232");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) { printf("[RS232] 客户端%d未登录\n", idx+1); return; }

    NET_SerialData_S data;
    memset(&data, 0, sizeof(data));
    if (g_bWebMode) {
        ReadStructJson(data);
    } else {
        data.nPortIndex = ReadInt("串口号(0=COM1): ");
        char strData[1024];
        ReadString("数据(字符串)", strData, sizeof(strData));
        size_t len = strlen(strData);
        std::memcpy(data.byData, strData, len);
        data.uDataLen = static_cast<UINT32>(len);
    }

    INT32 nRetBytes = 0;
    printf("测试接口：Test_SendRs232Data\n");
    BOOL ret = NET_clientSetDevConfig(g_lpUserID[idx], 0, NET_SEND_RS232_DATA, &data, sizeof(data), &nRetBytes);
    printf("实际调用：NET_clientSetDevConfig【562】 -> %s\n", ret ? "成功" : "失败");
    printf("COM%d, %u字节\n", data.nPortIndex, data.uDataLen);
    if (!ret) PrintFailure("SendRs232Data");
}

/** 121. 直接发送串口数据 (NET_SEND_SERIAL_DIRECT_DATA=563) */
static void Test_SendSerialDirectData()
{
    int idx = SelectClient("直接发串口");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) { printf("[SerialDirect] 客户端%d未登录\n", idx+1); return; }

    NET_SerialData_S data;
    memset(&data, 0, sizeof(data));
    if (g_bWebMode) {
        ReadStructJson(data);
    } else {
        data.nPortIndex = ReadInt("串口号(0=COM1): ");
        char strData[1024];
        ReadString("数据(字符串)", strData, sizeof(strData));
        size_t len = strlen(strData);
        std::memcpy(data.byData, strData, len);
        data.uDataLen = static_cast<UINT32>(len);
    }

    INT32 nRetBytes = 0;
    printf("测试接口：Test_SendSerialDirectData\n");
    BOOL ret = NET_clientSetDevConfig(g_lpUserID[idx], 0, NET_SEND_SERIAL_DIRECT_DATA, &data, sizeof(data), &nRetBytes);
    printf("实际调用：NET_clientSetDevConfig【563】 -> %s\n", ret ? "成功" : "失败");
    printf("COM%d, %u字节\n", data.nPortIndex, data.uDataLen);
    if (!ret) PrintFailure("SendSerialDirectData");
}

/* ===================== NVR 报警命令测试 (122-145) ===================== */
static void Test_GetMotionAlarm() { TEST_GENERIC_GET(NET_MotionAlarmInfo_S, "GetMotionAlarm", NET_GET_MOTIONALARM, 0); }
static void Test_SetMotionAlarm() { TEST_GENERIC_SET(NET_MotionAlarmInfo_S, "SetMotionAlarm", NET_GET_MOTIONALARM, NET_SET_MOTIONALARM, 0); }
static void Test_GetTamperAlarm() { TEST_GENERIC_GET(NET_TamperAlarmInfo_S, "GetTamperAlarm", NET_GET_TAMPERALARM, 0); }
static void Test_SetTamperAlarm() { TEST_GENERIC_SET(NET_TamperAlarmInfo_S, "SetTamperAlarm", NET_GET_TAMPERALARM, NET_SET_TAMPERALARM, 0); }
static void Test_GetCrossLineAlarm() { TEST_GENERIC_GET(NET_CrossLineAlarmInfo_S, "GetCrossLineAlarm", NET_GET_CROSSLINEALARM, 0); }
static void Test_SetCrossLineAlarm() { TEST_GENERIC_SET(NET_CrossLineAlarmInfo_S, "SetCrossLineAlarm", NET_GET_CROSSLINEALARM, NET_SET_CROSSLINEALARM, 0); }
static void Test_GetIntrusionAlarm() { TEST_GENERIC_GET(NET_IntrusionAlarmInfo_S, "GetIntrusionAlarm", NET_GET_INTRUSIONALARM, 0); }
static void Test_SetIntrusionAlarm() { TEST_GENERIC_SET(NET_IntrusionAlarmInfo_S, "SetIntrusionAlarm", NET_GET_INTRUSIONALARM, NET_SET_INTRUSIONALARM, 0); }
static void Test_GetEnterRegionAlarm() { TEST_GENERIC_GET(NET_EnterRegionAlarmInfo_S, "GetEnterRegionAlarm", NET_GET_ENTERREGIONALARM, 0); }
static void Test_SetEnterRegionAlarm() { TEST_GENERIC_SET(NET_EnterRegionAlarmInfo_S, "SetEnterRegionAlarm", NET_GET_ENTERREGIONALARM, NET_SET_ENTERREGIONALARM, 0); }
static void Test_GetLeaveRegionAlarm() { TEST_GENERIC_GET(NET_LeaveRegionAlarmInfo_S, "GetLeaveRegionAlarm", NET_GET_LEAVEREGIONALARM, 0); }
static void Test_SetLeaveRegionAlarm() { TEST_GENERIC_SET(NET_LeaveRegionAlarmInfo_S, "SetLeaveRegionAlarm", NET_GET_LEAVEREGIONALARM, NET_SET_LEAVEREGIONALARM, 0); }
static void Test_GetLoiteringAlarm() { TEST_GENERIC_GET(NET_LoiteringAlarmInfo_S, "GetLoiteringAlarm", NET_GET_LOITERINGALARM, 0); }
static void Test_SetLoiteringAlarm() { TEST_GENERIC_SET(NET_LoiteringAlarmInfo_S, "SetLoiteringAlarm", NET_GET_LOITERINGALARM, NET_SET_LOITERINGALARM, 0); }
static void Test_GetSceneChangeAlarm() { TEST_GENERIC_GET(NET_SceneChangeAlarmInfo_S, "GetSceneChangeAlarm", NET_GET_SCENECHANGEALARM, 0); }
static void Test_SetSceneChangeAlarm() { TEST_GENERIC_SET(NET_SceneChangeAlarmInfo_S, "SetSceneChangeAlarm", NET_GET_SCENECHANGEALARM, NET_SET_SCENECHANGEALARM, 0); }
static void Test_GetCrowdGatheringAlarm() { TEST_GENERIC_GET(NET_CrowdGatheringAlarmInfo_S, "GetCrowdGatheringAlarm", NET_GET_CROWDGATHERINGALARM, 0); }
static void Test_SetCrowdGatheringAlarm() { TEST_GENERIC_SET(NET_CrowdGatheringAlarmInfo_S, "SetCrowdGatheringAlarm", NET_GET_CROWDGATHERINGALARM, NET_SET_CROWDGATHERINGALARM, 0); }
static void Test_GetParkingAlarm() { TEST_GENERIC_GET(NET_ParkingAlarmInfo_S, "GetParkingAlarm", NET_GET_PARKINGALARM, 0); }
static void Test_SetParkingAlarm() { TEST_GENERIC_SET(NET_ParkingAlarmInfo_S, "SetParkingAlarm", NET_GET_PARKINGALARM, NET_SET_PARKINGALARM, 0); }
static void Test_GetUnattendedObjectAlarm() { TEST_GENERIC_GET(NET_UnattendedObjectAlarmInfo_S, "GetUnattendedObjectAlarm", NET_GET_UNATTENDEDOBJECTALARM, 0); }
static void Test_SetUnattendedObjectAlarm() { TEST_GENERIC_SET(NET_UnattendedObjectAlarmInfo_S, "SetUnattendedObjectAlarm", NET_GET_UNATTENDEDOBJECTALARM, NET_SET_UNATTENDEDOBJECTALARM, 0); }
static void Test_GetObjectRemovalAlarm() { TEST_GENERIC_GET(NET_ObjectRemovalAlarmInfo_S, "GetObjectRemovalAlarm", NET_GET_OBJECTREMOVALALARM, 0); }
static void Test_SetObjectRemovalAlarm() { TEST_GENERIC_SET(NET_ObjectRemovalAlarmInfo_S, "SetObjectRemovalAlarm", NET_GET_OBJECTREMOVALALARM, NET_SET_OBJECTREMOVALALARM, 0); }

/* ===================== 扩展AI与附加配置测试 (147-208) ===================== */
/* 垃圾满溢 (406/407) */
static void Test_GetGarbageOverflow() { TEST_GENERIC_GET(NET_GarbageOverflowCfg_S, "GetGarbageOverflow", NET_GET_GARBAGE_OVERFLOW_CFG, 0); }
static void Test_SetGarbageOverflow() { TEST_GENERIC_SET(NET_GarbageOverflowCfg_S, "SetGarbageOverflow", NET_GET_GARBAGE_OVERFLOW_CFG, NET_SET_GARBAGE_OVERFLOW_CFG, 0); }
/* 井盖异常 (413/414) */
static void Test_GetManholeCoverAbnormal() { TEST_GENERIC_GET(NET_ManholeCoverAbnormalCfg_S, "GetManholeCoverAbnormal", NET_GET_MANHOLE_COVER_ABNORMAL_CFG, 0); }
static void Test_SetManholeCoverAbnormal() { TEST_GENERIC_SET(NET_ManholeCoverAbnormalCfg_S, "SetManholeCoverAbnormal", NET_GET_MANHOLE_COVER_ABNORMAL_CFG, NET_SET_MANHOLE_COVER_ABNORMAL_CFG, 0); }
/* 睡岗 (415/416) */
static void Test_GetSleepOnDuty() { TEST_GENERIC_GET(NET_SleepOnDutyCfg_S, "GetSleepOnDuty", NET_GET_SLEEP_ON_DUTY_CFG, 0); }
static void Test_SetSleepOnDuty() { TEST_GENERIC_SET(NET_SleepOnDutyCfg_S, "SetSleepOnDuty", NET_GET_SLEEP_ON_DUTY_CFG, NET_SET_SLEEP_ON_DUTY_CFG, 0); }
/* 电瓶车进电梯 (417/418) */
static void Test_GetElectricVehicleInElevator() { TEST_GENERIC_GET(NET_ElectricVehicleInElevatorCfg_S, "GetElectricVehicleInElevator", NET_GET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG, 0); }
static void Test_SetElectricVehicleInElevator() { TEST_GENERIC_SET(NET_ElectricVehicleInElevatorCfg_S, "SetElectricVehicleInElevator", NET_GET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG, NET_SET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG, 0); }
/* 人员倒地 (419/420) */
static void Test_GetPersonFallDown() { TEST_GENERIC_GET(NET_PersonFallDownCfg_S, "GetPersonFallDown", NET_GET_PERSON_FALL_DOWN_CFG, 0); }
static void Test_SetPersonFallDown() { TEST_GENERIC_SET(NET_PersonFallDownCfg_S, "SetPersonFallDown", NET_GET_PERSON_FALL_DOWN_CFG, NET_SET_PERSON_FALL_DOWN_CFG, 0); }
/* 施工占道 (421/422) */
static void Test_GetConstructionOccupyRoad() { TEST_GENERIC_GET(NET_ConstructionOccupyRoadCfg_S, "GetConstructionOccupyRoad", NET_GET_CONSTRUCTION_OCCUPY_ROAD_CFG, 0); }
static void Test_SetConstructionOccupyRoad() { TEST_GENERIC_SET(NET_ConstructionOccupyRoadCfg_S, "SetConstructionOccupyRoad", NET_GET_CONSTRUCTION_OCCUPY_ROAD_CFG, NET_SET_CONSTRUCTION_OCCUPY_ROAD_CFG, 0); }
/* 拥堵 (423/424) */
static void Test_GetCongestion() { TEST_GENERIC_GET(NET_CongestionCfg_S, "GetCongestion", NET_GET_CONGESTION_CFG, 0); }
static void Test_SetCongestion() { TEST_GENERIC_SET(NET_CongestionCfg_S, "SetCongestion", NET_GET_CONGESTION_CFG, NET_SET_CONGESTION_CFG, 0); }
/* 车牌识别 (425/426) */
static void Test_GetLicensePlateRecognition() { TEST_GENERIC_GET(NET_LicensePlateRecognitionCfg_S, "GetLicensePlateRecognition", NET_GET_LICENSE_PLATE_RECOGNITION_CFG, 0); }
static void Test_SetLicensePlateRecognition() { TEST_GENERIC_SET(NET_LicensePlateRecognitionCfg_S, "SetLicensePlateRecognition", NET_GET_LICENSE_PLATE_RECOGNITION_CFG, NET_SET_LICENSE_PLATE_RECOGNITION_CFG, 0); }
/* 高空安全带 (427/428) */
static void Test_GetHighAltitudeSeatbelt() { TEST_GENERIC_GET(NET_HighAltitudeSeatbeltCfg_S, "GetHighAltitudeSeatbelt", NET_GET_HIGH_ALTITUDE_SEATBELT_CFG, 0); }
static void Test_SetHighAltitudeSeatbelt() { TEST_GENERIC_SET(NET_HighAltitudeSeatbeltCfg_S, "SetHighAltitudeSeatbelt", NET_GET_HIGH_ALTITUDE_SEATBELT_CFG, NET_SET_HIGH_ALTITUDE_SEATBELT_CFG, 0); }
/* 安全帽 (429/430) */
static void Test_GetSafetyHelmet() { TEST_GENERIC_GET(NET_SafetyHelmetCfg_S, "GetSafetyHelmet", NET_GET_SAFETY_HELMET_CFG, 0); }
static void Test_SetSafetyHelmet() { TEST_GENERIC_SET(NET_SafetyHelmetCfg_S, "SetSafetyHelmet", NET_GET_SAFETY_HELMET_CFG, NET_SET_SAFETY_HELMET_CFG, 0); }
/* 摔倒 (431/432) */
static void Test_GetPersonFall() { TEST_GENERIC_GET(NET_PersonFallCfg_S, "GetPersonFall", NET_GET_PERSON_FALL_CFG, 0); }
static void Test_SetPersonFall() { TEST_GENERIC_SET(NET_PersonFallCfg_S, "SetPersonFall", NET_GET_PERSON_FALL_CFG, NET_SET_PERSON_FALL_CFG, 0); }
/* 玩手机 (433/434) */
static void Test_GetPhoneUsage() { TEST_GENERIC_GET(NET_PhoneUsageCfg_S, "GetPhoneUsage", NET_GET_PHONE_USAGE_CFG, 0); }
static void Test_SetPhoneUsage() { TEST_GENERIC_SET(NET_PhoneUsageCfg_S, "SetPhoneUsage", NET_GET_PHONE_USAGE_CFG, NET_SET_PHONE_USAGE_CFG, 0); }
/* 明火 (437/438) */
static void Test_GetOpenFlame() { TEST_GENERIC_GET(NET_OpenFlameCfg_S, "GetOpenFlame", NET_GET_OPEN_FLAME_CFG, 0); }
static void Test_SetOpenFlame() { TEST_GENERIC_SET(NET_OpenFlameCfg_S, "SetOpenFlame", NET_GET_OPEN_FLAME_CFG, NET_SET_OPEN_FLAME_CFG, 0); }
/* 黄土裸露 (439/440) */
static void Test_GetBareSoil() { TEST_GENERIC_GET(NET_BareSoilCfg_S, "GetBareSoil", NET_GET_BARE_SOIL_CFG, 0); }
static void Test_SetBareSoil() { TEST_GENERIC_SET(NET_BareSoilCfg_S, "SetBareSoil", NET_GET_BARE_SOIL_CFG, NET_SET_BARE_SOIL_CFG, 0); }
/* 洞口防护栏 (441/442) */
static void Test_GetHoleProtectionBar() { TEST_GENERIC_GET(NET_HoleProtectionBarCfg_S, "GetHoleProtectionBar", NET_GET_HOLE_PROTECTION_BAR_CFG, 0); }
static void Test_SetHoleProtectionBar() { TEST_GENERIC_SET(NET_HoleProtectionBarCfg_S, "SetHoleProtectionBar", NET_GET_HOLE_PROTECTION_BAR_CFG, NET_SET_HOLE_PROTECTION_BAR_CFG, 0); }
/* 反光衣 (443/444) */
static void Test_GetReflectiveClothing() { TEST_GENERIC_GET(NET_ReflectiveClothingCfg_S, "GetReflectiveClothing", NET_GET_REFLECTIVE_CLOTHING_CFG, 0); }
static void Test_SetReflectiveClothing() { TEST_GENERIC_SET(NET_ReflectiveClothingCfg_S, "SetReflectiveClothing", NET_GET_REFLECTIVE_CLOTHING_CFG, NET_SET_REFLECTIVE_CLOTHING_CFG, 0); }
/* 宠物识别 (445/446) */
static void Test_GetPetRecognition() { TEST_GENERIC_GET(NET_PetRecognitionInfo_S, "GetPetRecognition", NET_GET_PET_RECOGNITION_INFO, 0); }
static void Test_SetPetRecognition() { TEST_GENERIC_SET(NET_PetRecognitionInfo_S, "SetPetRecognition", NET_GET_PET_RECOGNITION_INFO, NET_SET_PET_RECOGNITION_INFO, 0); }
/* 翻越围栏 (447/448) */
static void Test_GetClimbFence() { TEST_GENERIC_GET(NET_ClimbFenceInfo_S, "GetClimbFence", NET_GET_CLIMB_FENCE_INFO, 0); }
static void Test_SetClimbFence() { TEST_GENERIC_SET(NET_ClimbFenceInfo_S, "SetClimbFence", NET_GET_CLIMB_FENCE_INFO, NET_SET_CLIMB_FENCE_INFO, 0); }
/* 离岗 (449/450) */
static void Test_GetDimission() { TEST_GENERIC_GET(NET_DimissionInfo_S, "GetDimission", NET_GET_DIMISSION_INFO, 0); }
static void Test_SetDimission() { TEST_GENERIC_SET(NET_DimissionInfo_S, "SetDimission", NET_GET_DIMISSION_INFO, NET_SET_DIMISSION_INFO, 0); }
/* 违规变道 (451/452) */
static void Test_GetIllegalLane() { TEST_GENERIC_GET(NET_IllegalLaneInfo_S, "GetIllegalLane", NET_GET_ILLEGAL_LANE_INFO, 0); }
static void Test_SetIllegalLane() { TEST_GENERIC_SET(NET_IllegalLaneInfo_S, "SetIllegalLane", NET_GET_ILLEGAL_LANE_INFO, NET_SET_ILLEGAL_LANE_INFO, 0); }
/* 逆行 (453/454) */
static void Test_GetRetrograde() { TEST_GENERIC_GET(NET_RetrogradeInfo_S, "GetRetrograde", NET_GET_RETROGRADE_INFO, 0); }
static void Test_SetRetrograde() { TEST_GENERIC_SET(NET_RetrogradeInfo_S, "SetRetrograde", NET_GET_RETROGRADE_INFO, NET_SET_RETROGRADE_INFO, 0); }
/* 非机动车闯入 (455/456) */
static void Test_GetNonmotorVehicleIntrusion() { TEST_GENERIC_GET(NET_NonmotorVehicleIntrusionInfo_S, "GetNonmotorVehicleIntrusion", NET_GET_NONMOTOR_VEHICLE_INTRUSION_INFO, 0); }
static void Test_SetNonmotorVehicleIntrusion() { TEST_GENERIC_SET(NET_NonmotorVehicleIntrusionInfo_S, "SetNonmotorVehicleIntrusion", NET_GET_NONMOTOR_VEHICLE_INTRUSION_INFO, NET_SET_NONMOTOR_VEHICLE_INTRUSION_INFO, 0); }
/* 应急车道占用 (457/458) */
static void Test_GetOccupationEmergency() { TEST_GENERIC_GET(NET_OccupationEmergencyInfo_S, "GetOccupationEmergency", NET_GET_OCCUPATION_EMERGENCY_INFO, 0); }
static void Test_SetOccupationEmergency() { TEST_GENERIC_SET(NET_OccupationEmergencyInfo_S, "SetOccupationEmergency", NET_GET_OCCUPATION_EMERGENCY_INFO, NET_SET_OCCUPATION_EMERGENCY_INFO, 0); }
/* 行人闯入 (459/460) */
static void Test_GetPedestrianIntrusion() { TEST_GENERIC_GET(NET_PedestrianIntrusionInfo_S, "GetPedestrianIntrusion", NET_GET_PEDESTRIAN_INTRUSION_INFO, 0); }
static void Test_SetPedestrianIntrusion() { TEST_GENERIC_SET(NET_PedestrianIntrusionInfo_S, "SetPedestrianIntrusion", NET_GET_PEDESTRIAN_INTRUSION_INFO, NET_SET_PEDESTRIAN_INTRUSION_INFO, 0); }
/* 烟火 (461/462) */
static void Test_GetSmokeFire() { TEST_GENERIC_GET(NET_SmokeFireCfg_S, "GetSmokeFire", NET_GET_SMOKE_FIRE_CFG, 0); }
static void Test_SetSmokeFire() { TEST_GENERIC_SET(NET_SmokeFireCfg_S, "SetSmokeFire", NET_GET_SMOKE_FIRE_CFG, NET_SET_SMOKE_FIRE_CFG, 0); }
/* 道路积水 (463/464) */
static void Test_GetRoadPonding() { TEST_GENERIC_GET(NET_RoadPondingCfg_S, "GetRoadPonding", NET_GET_ROAD_PONDING_CFG, 0); }
static void Test_SetRoadPonding() { TEST_GENERIC_SET(NET_RoadPondingCfg_S, "SetRoadPonding", NET_GET_ROAD_PONDING_CFG, NET_SET_ROAD_PONDING_CFG, 0); }
/* 人脸抓拍叠加 (249/250) */
static void Test_GetFaceCaptureOverlay() { TEST_GENERIC_GET(NET_FaceCaptureOverlayInfo_S, "GetFaceCaptureOverlay", NET_GET_FACECAPTUREOVERLAYINFO, 0); }
static void Test_SetFaceCaptureOverlay() { TEST_GENERIC_SET(NET_FaceCaptureOverlayInfo_S, "SetFaceCaptureOverlay", NET_GET_FACECAPTUREOVERLAYINFO, NET_SET_FACECAPTUREOVERLAYINFO, 0); }
/* 垃圾暴露 (404/405) */
static void Test_GetGarbageExposure() { TEST_GENERIC_GET(NET_GarbageExposureCfg_S, "GetGarbageExposure", NET_GET_GARBAGE_EXPOSURE_CFG, 0); }
static void Test_SetGarbageExposure() { TEST_GENERIC_SET(NET_GarbageExposureCfg_S, "SetGarbageExposure", NET_GET_GARBAGE_EXPOSURE_CFG, NET_SET_GARBAGE_EXPOSURE_CFG, 0); }
/* 修改用户密码 (113) —— 专用客户端 API */
static void Test_SetUserPassword()
{
    int idx = SelectClient("修改用户密码");
    if (idx < 0) return;
    if (!g_lpUserID[idx]) { printf("[SetUserPassword] 客户端%d未登录\n", idx+1); return; }
    NET_UserPasswordInfo_S info;
    memset(&info, 0, sizeof(info));
    ReadStructJson(info);
    printf("测试接口：Test_SetUserPassword\n");
    BOOL ret = NET_clientSetUserPassword(g_lpUserID[idx], &info);
    printf("实际调用：NET_clientSetUserPassword -> %s\n", ret ? "成功" : "失败");
    if (!ret) PrintFailure("SetUserPassword");
}
/* 设备升级 (102/103/104) —— 设备级命令 */
static void Test_GetUpgradeStatus() { TEST_GENERIC_GET(NET_UpgradeStatus_S, "GetUpgradeStatus", NET_GET_UPGRADESTATUS, -1); }
static void Test_SetUpgrade() { TEST_DIRECT_SET(NET_UpgradeInfo_S, "SetUpgrade", NET_SET_UPGRADE, -1); }
static void Test_GetUpgradeVersion() { TEST_GENERIC_GET(NET_UpgradeVersion_S, "GetUpgradeVersion", NET_GET_UPGRADEVERSION, -1); }

/* ===================== 预留接口(客户端暂未实现发送) (212-220) ===================== */
/* 这些接口 NVR 端已注册回调(真实或预留 stub), 但 SDK 客户端未暴露对应 API,
 * 无法经网页发送, 此处仅作按钮占位与说明。 */
static void Test_NotSupported(const char* name, INT32 nvrCmd)
{
    printf("测试接口：%s\n", name);
    printf("⚠ SDK客户端未提供此命令的发送接口 (NVR端已注册回调 cmd=%d)\n", nvrCmd);
    printf("   需先在 SDK 客户端补充对应 API (如 NET_clientXXX), 方可经网页发送。\n");
}

/* ===================== 菜单与主函数 ===================== */
static void PrintMenu()
{
    printf("\n");
    printf("====================================================================================================\n");
    printf("                              SDK 客户端接口测试菜单 (%d项)\n", 218);
    printf("====================================================================================================\n");

    printf("--- 基础与SDK ---\n");
    printf("  0.显示菜单         1.初始化SDK        2.清理SDK          3.SDK版本          4.最后错误码\n");
    printf("  5.连接超时         6.接收超时         7.日志设置         8.异常回调\n");

    printf("--- 设备与会话 ---\n");
    printf("  9.登录            10.登出            11.自动重连        13.上传文件        300.综合流程测试\n");

    printf("--- 能力集 ---\n");
    printf("105.系统能力总表    18.视频编码能力     19.音频能力        20.OSD能力\n");

    printf("--- 设备控制 ---\n");
    printf(" 17.设备控制       205.设置升级       209.云台控制       210.查询预置位     211.设置预置位\n");
    printf("106.远程重启       107.远程关机       108.格式化硬盘     110.恢复默认       212.外部设备控制\n");

    printf("--- 系统与网络 ---\n");
    printf(" 28.NTP查询         29.NTP设置         146.设置系统时间   113.网络查询       42.网络设置\n");

    printf("--- 用户与设备管理 ---\n");
    printf(" 12.设备发现        21.设备信息        109.设备状态       23.存储信息        27.RTSP地址\n");
    printf(" 22.通道列表       111.查询通道名称   112.设置通道名称   203.修改用户密码   204.获取升级状态\n");
    printf("206.获取升级版本\n");

    printf("--- 报警监听与联动 ---\n");
    printf(" 14.报警监听       15.停止监听        16.通道状态回调\n");

    printf("--- 音频与对讲 ---\n");
    printf(" 26.音频配置查询    84.音频设置        85.对讲音频查询    86.对讲音频设置    34.语音对讲\n");
    printf("213.对讲状态设置   214.设置对讲输出   215.获取对讲源     216.回放对讲\n");

    printf("--- 预览与抓图 ---\n");
    printf(" 71.预览信息查询    72.预览信息设置    33.录像帧流        73.抓图计划查询    74.抓图计划设置\n");
    printf(" 75.抓图参数查询    76.抓图参数设置    36.流编码查询      37.流编码设置      218.抓图(JPEG)\n");

    printf("--- 录像与回放 ---\n");
    printf(" 25.录像状态       30.录像时间段      61.录像计划查询    62.录像计划设置    63.录像高级参数查询\n");
    printf(" 64.录像高级参数设置 67.查找录像文件   68.下载录像文件    31.回放地址        32.回放控制\n");
    printf("217.查询录像文件   219.获取录像文件地址\n");

    printf("--- IPC相关参数 ---\n");
    printf(" 87.曝光查询        88.曝光设置        89.日夜转换查询    90.日夜转换设置    91.背光查询\n");
    printf(" 92.背光设置        93.降噪查询        94.降噪设置        95.白平衡查询      96.白平衡设置\n");
    printf(" 43.隐私遮盖查询    44.隐私遮盖设置    38.OSD配置查询     39.OSD配置设置     40.图像查询\n");
    printf(" 41.图像设置        45.声音报警查询    46.声音报警设置    47.报警输入查询    48.报警输入设置\n");
    printf(" 49.报警输出查询    50.报警输出设置    51.闪光报警查询    52.闪光报警设置    53.PIR报警查询\n");
    printf(" 54.PIR报警设置     69.音频异常查询    70.音频异常设置    223.音频实时音量\n");
    printf("220.声光报警联动\n");

    printf("--- AI智能事件配置 ---\n");
    printf("122.移动侦测查询  123.移动侦测设置  124.遮挡查询      125.遮挡设置      126.越界查询\n");
    printf("127.越界设置      128.入侵查询      129.入侵设置      130.进入区域查询  131.进入区域设置\n");
    printf("132.离开区域查询  133.离开区域设置  134.徘徊查询      135.徘徊设置      136.场景变更查询\n");
    printf("137.场景变更设置  138.人员聚集查询  139.人员聚集设置  140.停车查询      141.停车设置\n");
    printf("142.物品遗留查询  143.物品遗留设置  144.物品拿取查询  145.物品拿取设置\n");
    printf(" 77.人脸抓拍查询    78.人脸抓拍设置    79.人脸比对设置    80.人流统计查询     81.人流统计设置\n");
    printf(" 82.人员密度查询    83.人员密度设置\n");

    printf("--- 扩展AI事件配置 ---\n");
    printf("147.垃圾满溢查询  148.垃圾满溢设置  149.井盖异常查询  150.井盖异常设置  151.睡岗查询\n");
    printf("152.睡岗设置      153.电瓶车进电梯查询 154.电瓶车进电梯设置 155.人员倒地查询 156.人员倒地设置\n");
    printf("157.施工占道查询  158.施工占道设置  159.拥堵查询      160.拥堵设置      161.车牌识别查询\n");
    printf("162.车牌识别设置  163.高空安全带查询 164.高空安全带设置 165.安全帽查询   166.安全帽设置\n");
    printf("167.摔倒查询      168.摔倒设置      169.玩手机查询    170.玩手机设置    171.明火查询\n");
    printf("172.明火设置      173.黄土裸露查询  174.黄土裸露设置  175.洞口防护栏查询 176.洞口防护栏设置\n");
    printf("177.反光衣查询    178.反光衣设置    179.宠物识别查询  180.宠物识别设置  181.翻越围栏查询\n");
    printf("182.翻越围栏设置  183.离岗查询      184.离岗设置      185.违规变道查询  186.违规变道设置\n");
    printf("187.逆行查询      188.逆行设置      189.非机动车闯入查询 190.非机动车闯入设置 191.应急车道占用查询\n");
    printf("192.应急车道占用设置 193.行人闯入查询 194.行人闯入设置  195.烟火查询      196.烟火设置\n");
    printf("197.道路积水查询  198.道路积水设置  199.人脸抓拍叠加查询 200.人脸抓拍叠加设置 201.垃圾暴露查询\n");
    printf("202.垃圾暴露设置\n");

    printf("--- 人脸库管理 ---\n");
    printf(" 97.添加目标库      98.删除目标库       99.修改目标库     100.获取目标库     101.添加人脸\n");
    printf("102.删除人脸       103.修改人脸       104.获取人脸\n");

    printf("--- 串口与日志 ---\n");
    printf("115.建立透明通道   116.断开透明通道   117.发送通道数据   118.查询串口参数   119.发送串口数据\n");
    printf("120.直接发送232    121.直接发送串口   58.日志服务器查询  59.日志服务器设置  60.测试日志服务器\n");
    printf(" 65.查找日志        66.导出日志       55.安全服务查询    56.安全服务设置    57.SSH倒计时\n");

    printf("====================================================================================================\n");
}

/** 命令分发 (CLI 与 Web 共用) */
static void RunTest(int cmd)
{
    switch (cmd) {
    case 0:  PrintMenu(); break;
    case 1:  Test_Init(); break;
    case 2:  Test_Cleanup(); break;
    case 3:  Test_GetVersion(); break;
    case 4:  Test_GetLastError(); break;
    case 5:  Test_SetConnectTime(); break;
    case 6:  Test_SetRevTimeOut(); break;
    case 7:  Test_SetLogToFile(); break;
    case 8:  Test_SetExceptionCb(); break;
    case 9:  Test_Login(); break;
    case 10: Test_Logout(); break;
    case 11: Test_SetAutoReconnect(); break;
    case 12: Test_SearchDiscovery(); break;
    case 13: Test_UploadFile(); break;
    case 14: Test_StartListen(); break;
    case 15: Test_StopListen(); break;
    case 16: Test_SetChannelStatusCb(); break;
    case 17: Test_DeviceControl(); break;
    case 18: Test_GetVideoCap(); break;
    case 19: Test_GetAudioCap(); break;
    case 20: Test_GetOsdCap(); break;
    case 21: Test_GetDeviceInfo(); break;
    case 22: Test_GetChannelList(); break;
    case 23: Test_GetStorageInfo(); break;
    case 25: Test_GetRecordStatus(); break;
    case 26: Test_GetAudioCfg(); break;
    case 27: Test_GetRtspUrl(); break;
    case 28: Test_GetNtpConfig(); break;
    case 29: Test_SetNtpConfig(); break;
    case 30: Test_GetReplayRecordList(); break;
    case 31: Test_GetReplayUrl(); break;
    case 32: Test_ControlReplay(); break;
    case 33: Test_RecordFrameStream(); break;
    case 35: Test_QueryRecordFiles(); break;
    case 34: Test_VoiceCom(); break;
    case 300: Test_FullFlow(); break;
    case 36: Test_GetStreamCfg(); break;
    case 37: Test_SetStreamCfg(); break;
    case 38: Test_GetOsdCapCfg(); break;
    case 39: Test_SetOsdCapCfg(); break;
    case 40: Test_GetImageCfg(); break;
    case 41: Test_SetImageCfg(); break;
    case 42: Test_SetNetworkCfg(); break;
    case 43: Test_GetPrivacyMask(); break;
    case 44: Test_SetPrivacyMask(); break;
    case 45: Test_GetAudibleAlarm(); break;
    case 46: Test_SetAudibleAlarm(); break;
    case 47: Test_GetAlarmInput(); break;
    case 48: Test_SetAlarmInput(); break;
    case 49: Test_GetAlarmOutput(); break;
    case 50: Test_SetAlarmOutput(); break;
    case 51: Test_GetFlashingLight(); break;
    case 52: Test_SetFlashingLight(); break;
    case 53: Test_GetPirAlarm(); break;
    case 54: Test_SetPirAlarm(); break;
    case 55: Test_GetSecuritySvc(); break;
    case 56: Test_SetSecuritySvc(); break;
    case 57: Test_GetSshCountdown(); break;
    case 58: Test_GetLogServer(); break;
    case 59: Test_SetLogServer(); break;
    case 60: Test_TestLogServer(); break;
    case 61: Test_GetRecordSchedule(); break;
    case 62: Test_SetRecordSchedule(); break;
    case 63: Test_GetRecordAdvParam(); break;
    case 64: Test_SetRecordAdvParam(); break;
    case 65: Test_FindLog(); break;
    case 66: Test_ExportLog(); break;
    case 67: Test_FindRecordFile(); break;
    case 68: Test_DownloadRecord(); break;
    case 69: Test_GetAudioAnomaly(); break;
    case 70: Test_SetAudioAnomaly(); break;
    case 71: Test_GetPreviewInfo(); break;
    case 72: Test_SetPreviewInfo(); break;
    case 73: Test_GetCapturePlan(); break;
    case 74: Test_SetCapturePlan(); break;
    case 75: Test_GetCaptureParam(); break;
    case 76: Test_SetCaptureParam(); break;
    case 77: Test_GetFaceCapture(); break;
    case 78: Test_SetFaceCapture(); break;
    case 79: Test_SetFaceCompare(); break;
    case 80: Test_GetPeopleFlow(); break;
    case 81: Test_SetPeopleFlow(); break;
    case 82: Test_GetPeopleDensity(); break;
    case 83: Test_SetPeopleDensity(); break;
    case 84: Test_SetAudioCfg(); break;
    case 85: Test_GetVoiceComCfg(); break;
    case 86: Test_SetVoiceComCfg(); break;
    case 87: Test_GetExposure(); break;
    case 88: Test_SetExposure(); break;
    case 89: Test_GetDayNight(); break;
    case 90: Test_SetDayNight(); break;
    case 91: Test_GetBackLight(); break;
    case 92: Test_SetBackLight(); break;
    case 93: Test_GetDenoise(); break;
    case 94: Test_SetDenoise(); break;
    case 95: Test_GetWhiteBalance(); break;
    case 96: Test_SetWhiteBalance(); break;
    case 97: Test_AddTargetLib(); break;
    case 98: Test_DelTargetLib(); break;
    case 99: Test_SetTargetLib(); break;
    case 100: Test_GetTargetLib(); break;
    case 101: Test_AddFaceInfo(); break;
    case 102: Test_DelFaceInfo(); break;
    case 103: Test_SetFaceInfo(); break;
    case 104: Test_GetFaceInfo(); break;
    case 105: Test_GetSysCap(); break;
    case 106: Test_ControlReboot(); break;
    case 107: Test_ControlShutdown(); break;
    case 108: Test_ControlFormatDisk(); break;
    case 109: Test_GetDeviceStatus(); break;
    case 110: Test_ResetDefault(); break;
    case 111: Test_GetChannelName(); break;
    case 112: Test_SetChannelName(); break;
    case 113: Test_GetNetworkCfg(); break;
    case 115: Test_OpenTransparentChannel(); break;
    case 116: Test_CloseTransparentChannel(); break;
    case 117: Test_SendTransparentData(); break;
    case 118: Test_GetSerialPortParam(); break;
    case 119: Test_SendSerialData(); break;
    case 120: Test_SendRs232Data(); break;
    case 121: Test_SendSerialDirectData(); break;
    case 122: Test_GetMotionAlarm(); break;
    case 123: Test_SetMotionAlarm(); break;
    case 124: Test_GetTamperAlarm(); break;
    case 125: Test_SetTamperAlarm(); break;
    case 126: Test_GetCrossLineAlarm(); break;
    case 127: Test_SetCrossLineAlarm(); break;
    case 128: Test_GetIntrusionAlarm(); break;
    case 129: Test_SetIntrusionAlarm(); break;
    case 130: Test_GetEnterRegionAlarm(); break;
    case 131: Test_SetEnterRegionAlarm(); break;
    case 132: Test_GetLeaveRegionAlarm(); break;
    case 133: Test_SetLeaveRegionAlarm(); break;
    case 134: Test_GetLoiteringAlarm(); break;
    case 135: Test_SetLoiteringAlarm(); break;
    case 136: Test_GetSceneChangeAlarm(); break;
    case 137: Test_SetSceneChangeAlarm(); break;
    case 138: Test_GetCrowdGatheringAlarm(); break;
    case 139: Test_SetCrowdGatheringAlarm(); break;
    case 140: Test_GetParkingAlarm(); break;
    case 141: Test_SetParkingAlarm(); break;
    case 142: Test_GetUnattendedObjectAlarm(); break;
    case 143: Test_SetUnattendedObjectAlarm(); break;
    case 144: Test_GetObjectRemovalAlarm(); break;
    case 145: Test_SetObjectRemovalAlarm(); break;
    case 146: Test_SetSystemTime(); break;
    /* 扩展AI与附加配置 (147-208) */
    case 147: Test_GetGarbageOverflow(); break;
    case 148: Test_SetGarbageOverflow(); break;
    case 149: Test_GetManholeCoverAbnormal(); break;
    case 150: Test_SetManholeCoverAbnormal(); break;
    case 151: Test_GetSleepOnDuty(); break;
    case 152: Test_SetSleepOnDuty(); break;
    case 153: Test_GetElectricVehicleInElevator(); break;
    case 154: Test_SetElectricVehicleInElevator(); break;
    case 155: Test_GetPersonFallDown(); break;
    case 156: Test_SetPersonFallDown(); break;
    case 157: Test_GetConstructionOccupyRoad(); break;
    case 158: Test_SetConstructionOccupyRoad(); break;
    case 159: Test_GetCongestion(); break;
    case 160: Test_SetCongestion(); break;
    case 161: Test_GetLicensePlateRecognition(); break;
    case 162: Test_SetLicensePlateRecognition(); break;
    case 163: Test_GetHighAltitudeSeatbelt(); break;
    case 164: Test_SetHighAltitudeSeatbelt(); break;
    case 165: Test_GetSafetyHelmet(); break;
    case 166: Test_SetSafetyHelmet(); break;
    case 167: Test_GetPersonFall(); break;
    case 168: Test_SetPersonFall(); break;
    case 169: Test_GetPhoneUsage(); break;
    case 170: Test_SetPhoneUsage(); break;
    case 171: Test_GetOpenFlame(); break;
    case 172: Test_SetOpenFlame(); break;
    case 173: Test_GetBareSoil(); break;
    case 174: Test_SetBareSoil(); break;
    case 175: Test_GetHoleProtectionBar(); break;
    case 176: Test_SetHoleProtectionBar(); break;
    case 177: Test_GetReflectiveClothing(); break;
    case 178: Test_SetReflectiveClothing(); break;
    case 179: Test_GetPetRecognition(); break;
    case 180: Test_SetPetRecognition(); break;
    case 181: Test_GetClimbFence(); break;
    case 182: Test_SetClimbFence(); break;
    case 183: Test_GetDimission(); break;
    case 184: Test_SetDimission(); break;
    case 185: Test_GetIllegalLane(); break;
    case 186: Test_SetIllegalLane(); break;
    case 187: Test_GetRetrograde(); break;
    case 188: Test_SetRetrograde(); break;
    case 189: Test_GetNonmotorVehicleIntrusion(); break;
    case 190: Test_SetNonmotorVehicleIntrusion(); break;
    case 191: Test_GetOccupationEmergency(); break;
    case 192: Test_SetOccupationEmergency(); break;
    case 193: Test_GetPedestrianIntrusion(); break;
    case 194: Test_SetPedestrianIntrusion(); break;
    case 195: Test_GetSmokeFire(); break;
    case 196: Test_SetSmokeFire(); break;
    case 197: Test_GetRoadPonding(); break;
    case 198: Test_SetRoadPonding(); break;
    case 199: Test_GetFaceCaptureOverlay(); break;
    case 200: Test_SetFaceCaptureOverlay(); break;
    case 201: Test_GetGarbageExposure(); break;
    case 202: Test_SetGarbageExposure(); break;
    case 203: Test_SetUserPassword(); break;
    case 204: Test_GetUpgradeStatus(); break;
    case 205: Test_SetUpgrade(); break;
    case 206: Test_GetUpgradeVersion(); break;
    /* 云台与预置位 (528/529, 客户端分发已补齐) */
    case 209: Test_ControlCamera(); break;
    case 210: Test_GetPresetBit(); break;
    case 211: Test_ControlPresetBit(); break;
    /* 预留接口(客户端暂未实现发送) (212-220) */
    case 212: Test_NotSupported("外部设备控制", 530); break;
    case 213: Test_NotSupported("对讲状态设置", 400); break;
    case 214: Test_NotSupported("设置对讲输出", 401); break;
    case 215: Test_NotSupported("获取对讲源", 402); break;
    case 216: Test_NotSupported("回放对讲", 403); break;
    case 217: Test_NotSupported("查询录像文件", 0); break;
    case 218: Test_CapturePicture(); break;
    case 219: Test_NotSupported("获取录像文件地址", 0); break;
    case 220: Test_NotSupported("声光报警联动", 574); break;
    case 221: Test_ListDownloadTasks(); break;
    case 222: Test_CancelDownloadTask(); break;
    case 223: Test_GetAudioAnomalyCurrentDb(); break;
    default: printf("无效选项\n"); break;
    }
}

int main(int argc, char* argv[])
{
    if (argc >= 5) {
        SafeCopy(g_serverIp, sizeof(g_serverIp), argv[1]);
        g_serverPort = atoi(argv[2]);
        SafeCopy(g_username, sizeof(g_username), argv[3]);
        SafeCopy(g_password, sizeof(g_password), argv[4]);
    }
    printf("SDK Client Test Demo\n");
    printf("目标: %s:%d 用户: %s\n", g_serverIp, g_serverPort, g_username);

    /* --web 模式: 启动 HTTP 服务器, 通过网页按钮驱动测试 */
    g_bWebMode = (argc >= 2 && strcmp(argv[1], "--web") == 0);
    if (g_bWebMode) {
        int webPort = 8080;
        if (argc >= 3) webPort = atoi(argv[2]);
        if (webPort <= 0) webPort = 8080;

        if (!g_webServer.start(webPort)) {
            fprintf(stderr, "[Web] 启动 HTTP 服务器失败 (端口 %d)\n", webPort);
            return 1;
        }
        printf("\n========================================\n");
        printf("  Web 测试界面: http://0.0.0.0:%d\n", webPort);
        printf("  Ctrl+C 退出\n");
        printf("========================================\n\n");

        while (true) {
            if (g_webServer.hasCommand()) {
                int cmd = g_webServer.popCommand();
                fflush(stdout);
                if (cmd == 999 || cmd == -1) {   /* 999=网页退出按钮专用退出码, 避免与命令 99(修改目标库) 冲突 */
                    if (g_bSdkInit) Test_Cleanup();
                    break;
                }
                /* 启动前排空 stdin 管道中滞留的输入(quit/clear等),
                 * 保证命令的第一个 fgets 只读到命令启动后的新输入 */
                g_webServer.drainIdleInput();
                printf("\n--- Web 命令: %d ---\n", cmd);
                fflush(stdout);
                g_webServer.setBusy(true);
                if (setjmp(g_quitJmp) == 0) {
                    RunTest(cmd);
                    printf("\n--- 命令 %d 完成 ---\n\n", cmd);
                } else {
                    g_webServer.drainQueue();
                    printf("\n--- 命令 %d 已中止 ---\n\n", cmd);
                }
                g_webServer.setBusy(false);
                fflush(stdout);
            } else {
                /* 空闲期消费滞留在 stdin 管道的输入(quit/clear等),
                 * 防止被下一条命令的第一个 fgets 误读 */
                g_webServer.drainIdleInput();
            }
            usleep(100000); /* 100ms */
        }
        g_webServer.stop();
        return 0;
    }

    /* CLI 模式: 标准交互 */
    printf("输入 0 查看菜单\n\n");

    while (true) {
        int choice = ReadInt("\n请输入选项 (0=菜单, 999=退出, quit=中止, clear=清屏): ");
        if (choice == -1 || choice == 999) {   /* 999=退出码, 避免与命令 99(修改目标库) 冲突 */
            if (g_bSdkInit) { Test_Cleanup(); }
            printf("退出\n");
            return 0;
        }
        if (setjmp(g_quitJmp) == 0) {
            RunTest(choice);
        }
    }
    return 0;
}
