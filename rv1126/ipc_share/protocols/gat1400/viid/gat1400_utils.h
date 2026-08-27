/**
 * @File     gat1400-utils.h
 * @brief
 * @DateTime 2018/8/1 12:06:15
 * @Author   Nanuns
 */
#ifndef __GAT1400_UTILS_H_
#define __GAT1400_UTILS_H_

#include "dlog.h"
#include "time_utils.h"
#include <cstdint>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <functional>

using namespace std;

#if defined(WIN32)

//#define WIN32_LEAN_AND_MEAN
#include <time.h>
#include <sys/timeb.h>
#include <windows.h>

static int gettimeofday(struct timeval *tp, void *tz)
{
    struct _timeb timebuffer;

    _ftime(&timebuffer);
    tp->tv_sec = (long)timebuffer.time;
    tp->tv_usec = timebuffer.millitm * 1000;
    return 0;
}
#else
#include <sys/time.h>
#endif

// https://blog.csdn.net/qiuchangyong/article/details/68945482
// http://blog.csdn.net/linyt/article/details/52728910 ����Linux2038������
// http ://wuzhiwei.net/one_overflow_issue/ ȡ������������
inline static int64_t gettime_ms() {
    struct timeval now;
    gettimeofday(&now, 0);
    return (((int64_t)now.tv_sec) * 1000 + now.tv_usec / 1000);
}

static const char* num2String(int port) {
    static char buff[8] = { 0 };
    sprintf(buff, "%d", port);
    return buff;
}

/**
 * @Method   checkAddress
 * @Brief
 * @DateTime 2018-08-04T14:15:16+0800
 * @Modify   2018-08-04T14:15:16+0800
 * @Author   Nanuns
 * @param    ip [description]
 * @param    port [description]
 * @return   [description]
 */
static int checkAddress(const char* ip, int port) {
    if (!ip || strlen(ip) < 5) {
        printf("IP address invalid.\n");
        return -1;
    }
    if (port < 1024 || port > 65536) {
        printf("port invalid.\n");
        return -2;
    }
    return 0;
}

/**
 * @Method   addressPrefix
 * @Brief
 * @DateTime 2018-08-04T14:15:16+0800
 * @Modify   2018-08-04T14:15:16+0800
 * @Author   Nanuns
 * @param    ip [description]
 * @param    port [description]
 * @return   [description]
 */
static string addressPrefix(const char* ip, int port) {
    if (checkAddress(ip, port) != 0) {
        return "";
    }
    string suri("http://");
    suri.append(ip);
    suri.append(":");
    suri.append(num2String(port));
    return suri;
}

// 上报状态机
// 主要处理一些状态维持的上报任务
class CUploadStateMachine
{
public:
    CUploadStateMachine() {};
    ~CUploadStateMachine() {};

    bool handleAlarmState(bool bAlarm) {
        long long now = TimeUtils_NS::get_currentTimestampMs();
        bool bUpload = false;
        if (bAlarm) {
            if (!m_bActive) {
                dlog_debug("事件上传");
                m_bActive = true;
                bUpload = true;
            } else {
                dlog_debug("事件维持中，无需上传");
            }
            m_lLasttime = now;
        } else {
            if (m_bActive) {
                long long l_duration = now - m_lLasttime;
                if (l_duration > 3 * 1000) {
                    dlog_debug("事件结束");
                    m_bActive = false;
                }
            }
        }
        return bUpload;
    }

private:

    bool m_bActive = false;
    long long m_lLasttime = 0;
};

/* 处理上传间隔，避免某些事件一直触发上报 */
class CUploadInterval
{
public:
    CUploadInterval() {}
    ~CUploadInterval() {}

    /**
     * @description  : 是否需要上传
     * @return        {*}
     */
    bool isUpload() {
        bool bUpload = false;
        auto now = TimeUtils_NS::get_currentTimestampMs();
        auto l_duration = now - m_lLastTime;
        if (l_duration >= m_lInterval) {
            bUpload = true;
            m_lLastTime = now;
        };
        return bUpload;
    }

private:
    /* 上传上传时间 */
    long long m_lLastTime = 0;
    /* 上传间隔 */
    long long m_lInterval = 10 * 1000;
};

/* 坐标点处理工具类 */
class PointUtils {
public:
    PointUtils (int x, int y):mX(x), mY(y) {}

    /**
     * @description  : 像素坐标转归一化坐标
     * @param         {int} srcX
     * @param         {int} srcY
     * @param         {float} &dstX
     * @param         {float} &dstY
     * @return        {*}
     */    
    void toNormalize(int srcX, int srcY, float &dstX, float &dstY) {
        dstX = static_cast<float>(srcX) / (mX - 1);
        dstY = static_cast<float>(srcY) / (mY - 1);
        if (dstX == 0) dstX = 0.001;
        if (dstY == 0) dstY = 0.001;
    }

    /**
     * @description  : 归一化坐标转像素坐标
     * @param         {float} srcX
     * @param         {float} srcY
     * @param         {int} &dstX
     * @param         {int} &dstY
     * @return        {*}
     */
    void toPixel(float srcX, float srcY, int &dstX, int &dstY) {
        dstX = srcX * (mX - 1);
        dstY = srcY * (mY - 1);
    }

    /**
     * @description  : 像素坐标转万分比坐标
     * @param         {int} srcX
     * @param         {int} srcY
     * @param         {int} &dstX
     * @param         {int} &dstY
     * @return        {*}
     */    
    void toPermyriad(int srcX, int srcY, int &dstX, int &dstY) {
        dstX = (srcX * 10000) / mX;
        dstY = (srcY * 10000) / mY;
    }

private:
    /* 图片原本大小 */
    int mX, mY;
};

#endif
