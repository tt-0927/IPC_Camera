/**
 * @FilePath     : event_linkage_action_async.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-15 16:29:58
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-03 16:09:15
 * @Description  : 事件联动异步动作执行器实现
 */

#include "event_linkage_action_async.h"

#include <cerrno>
#include <chrono>
#include <cstring>
#include <future>
#include <thread>

#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>

#include "av_configure.h"
#include "capture_ctrl.h"
#include "email_manage.h"
#include "event_configure.h"
#include "event_linkage_dict.h"
#include "gpio_ctrl.h"
#include "light_manager.h"
#include "onvif_SubscriptionManager.hpp"
#include "preview_manage.h"
#include "time_utils.h"

/* 音频块大小，每次读取的长度 */
#if CAP_EVENT_AUDIO_PLAYBACK_V2
#define AUDIO_CHUNK_SIZE 2048
#else
#define AUDIO_CHUNK_SIZE 320
#endif

/* 总淡入淡出步数 */
#define TOTAL_FADE_STEPS (2)

void EventLinkageAsyncAction::execute(const LinkageTask_S &stTask, std::atomic<bool> &bRunningFlag)
{
    /* 根据联动类型分发到对应执行函数，运行标志用于抢占和中断控制 */
    switch (stTask.enLinkageType)
    {
    case LinkageType_E::EMAIL:
        bRunningFlag.store(true);
        execute_email(stTask, bRunningFlag);
        bRunningFlag.store(false);
        break;
    case LinkageType_E::SOUND:
        execute_audio(stTask, bRunningFlag);
        break;
    case LinkageType_E::FLASHING_LIGHT:
        bRunningFlag.store(true);
        execute_warning_light(bRunningFlag);
        bRunningFlag.store(false);
        break;
    case LinkageType_E::ALARM_IO:
        bRunningFlag.store(true);
        execute_alarm_io(stTask, bRunningFlag);
        bRunningFlag.store(false);
        break;
    case LinkageType_E::LOG:
        bRunningFlag.store(true);
        execute_log(stTask);
        bRunningFlag.store(false);
        break;
    default:
        dlog_warn("未知的异步联动类型: %d", static_cast<int>(stTask.enLinkageType));
        break;
    }
}

void EventLinkageAsyncAction::play_audio(const std::string &strAudioPath,
                                         int nTimes,
                                         std::atomic<bool> &bRunningFlag)
{
#if CAP_EVENT_AUDIO_PLAYBACK_V2
    /* 同一时刻只允许一条声音联动占用扬声器 */
    if (bRunningFlag.load())
    {
        dlog_warn("音频已在播放中");
        return;
    }

    bRunningFlag.store(true);
    CAVConfigure::instance()->setAudioAoSampleRate(Audio_NS::AudioSamprate_E::AUDIO_SAMPRATE_16000);

    uint8_t zero_buffer[AUDIO_CHUNK_SIZE];
    memset(zero_buffer, 0, sizeof(zero_buffer));

    Audio_NS::AoInfo_S stSilenceInfo;
    stSilenceInfo.nChannel = 0;
    stSilenceInfo.pData = zero_buffer;
    stSilenceInfo.nLen = AUDIO_CHUNK_SIZE;
    stSilenceInfo.enAudioFormat = Audio_NS::AudioFormat_E::PCM;

    for (int playCount = 0; playCount < nTimes; ++playCount)
    {
        if (!bRunningFlag.load())
        {
            break;
        }

        /* 每轮播放前先补一小段静音，减轻切换音频时的突兀感 */
        for (int i = 0; i < 5; ++i)
        {
            CAVConfigure::instance()->setAoSpeakInfo(stSilenceInfo);
        }
        int fd = open(strAudioPath.c_str(), O_RDONLY);
        if (fd == -1)
        {
            break;
        }

        unsigned char header[44] = {0};
        read(fd, header, 44);

        uint32_t audio_data_len = static_cast<unsigned char>(header[40]) |
                                  (static_cast<unsigned char>(header[41]) << 8) |
                                  (static_cast<unsigned char>(header[42]) << 16) |
                                  (static_cast<unsigned char>(header[43]) << 24);
        if (audio_data_len == 0 || audio_data_len > 50 * 1024 * 1024)
        {
            audio_data_len = 0xFFFFFFFF;
        }

        uint32_t total_read_bytes = 0;
        unsigned char buffer[AUDIO_CHUNK_SIZE] = {0};
        while (total_read_bytes < audio_data_len)
        {
            if (!bRunningFlag.load())
            {
                break;
            }

            const uint32_t remaining = audio_data_len - total_read_bytes;
            const int nToRead = remaining > AUDIO_CHUNK_SIZE ? AUDIO_CHUNK_SIZE : static_cast<int>(remaining);
            const int nBytesRead = read(fd, buffer, nToRead);
            if (nBytesRead <= 0)
            {
                break;
            }

            total_read_bytes += nBytesRead;
            if (nBytesRead < AUDIO_CHUNK_SIZE)
            {
                /* 最后一块不足整帧时做补零和淡出，减少播放尾音杂音 */
                memset(buffer + nBytesRead, 0, AUDIO_CHUNK_SIZE - nBytesRead);

                int fade_len = nBytesRead / 2 > 80 ? 80 : nBytesRead / 2;
                short *pData = reinterpret_cast<short *>(buffer);
                int nSamples = nBytesRead / 2;
                for (int k = 0; k < fade_len; ++k)
                {
                    const int idx = nSamples - 1 - k;
                    pData[idx] = static_cast<short>(pData[idx] * k / fade_len);
                }
            }

            Audio_NS::AoInfo_S stAoInfo;
            stAoInfo.nChannel = 0;
            stAoInfo.pData = reinterpret_cast<uint8_t *>(buffer);
            stAoInfo.nLen = AUDIO_CHUNK_SIZE;
            stAoInfo.enAudioFormat = Audio_NS::AudioFormat_E::PCM;
            CAVConfigure::instance()->setAoSpeakInfo(stAoInfo);
            memset(buffer, 0, sizeof(buffer));
        }

        close(fd);
        /* 文件播放结束后继续送几帧静音，保证尾部音频完整输出 */
        for (int i = 0; i < 5; ++i)
        {
            CAVConfigure::instance()->setAoSpeakInfo(stSilenceInfo);
        }

        if (playCount < nTimes - 1)
        {
            /* 多次播报之间保留短暂间隔，避免听感过于紧凑 */
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
    }

    /* 播放结束，立即关闭功放消除结尾噗声（比 watchdog 2秒超时更快） */
    CAVConfigure::instance()->muteAudioOutput();
    bRunningFlag.store(false);
#else
    /* 旧播放链路按块读取PCM并同步睡眠控制节奏 */
    if (bRunningFlag.load())
    {
        dlog_warn("音频已在播放中，忽略本次请求");
        return;
    }

    bRunningFlag.store(true);
    dlog_info("开始音频播放: 文件=%s, 次数=%d", strAudioPath.c_str(), nTimes);
    CAVConfigure::instance()->setAudioAoSampleRate(Audio_NS::AudioSamprate_E::AUDIO_SAMPRATE_16000);

    /* 静音帧 */
    uint8_t zero_buffer[AUDIO_CHUNK_SIZE] = {0};
    Audio_NS::AoInfo_S stSilenceInfo;
    stSilenceInfo.nChannel = 0;
    stSilenceInfo.pData = zero_buffer;
    stSilenceInfo.nLen = AUDIO_CHUNK_SIZE;
    stSilenceInfo.enAudioFormat = Audio_NS::AudioFormat_E::PCM;

    /* 非首次播放的淡入步数：约 8 帧 × 10ms ≈ 80ms 的线性淡入 */
    const int FADE_IN_FRAMES = 8;

    for (int playCount = 0; playCount < nTimes; ++playCount)
    {
        if (!bRunningFlag.load())
        {
            break;
        }

        /* 每次循环都重新打开文件，从头播放一遍完整音频 */
        int fd = open(strAudioPath.c_str(), O_RDONLY);
        if (fd == -1)
        {
            dlog_error("打开文件失败: %s (错误: %d: %s)", strAudioPath.c_str(), errno, strerror(errno));
            break;
        }

        /* 解析 WAV 头部获取数据长度，以便精准抓取尾部进行淡出 */
        unsigned char header[44] = { 0 };
        read(fd, header, 44);
        uint32_t audio_data_len = static_cast<uint32_t>(header[40]) |
                                  (static_cast<uint32_t>(header[41]) << 8) |
                                  (static_cast<uint32_t>(header[42]) << 16) |
                                  (static_cast<uint32_t>(header[43]) << 24);
        if (audio_data_len == 0 || audio_data_len > 50U * 1024U * 1024U)
        {
            audio_data_len = 0xFFFFFFFFU;
        }

        uint32_t total_read_bytes = 0;
        unsigned char buffer[AUDIO_CHUNK_SIZE] = {0};
        /* 是否淡出 */
        bool bFadingOut = false;
        /* 剩余淡出步数 */
        int nFadeStepsRemaining = 0;
        /* 剩余淡入部分 */
        int nFadeInRemaining = (playCount > 0) ? FADE_IN_FRAMES : 0;

        while (total_read_bytes < audio_data_len)
        {
            if (!bRunningFlag.load() && !bFadingOut)
            {
                bFadingOut = true;
                nFadeStepsRemaining = TOTAL_FADE_STEPS;
            }

            if (bFadingOut && nFadeStepsRemaining <= 0)
            {
                break;
            }

            const uint32_t nRemaining = audio_data_len - total_read_bytes;
            const int nToRead = (nRemaining > AUDIO_CHUNK_SIZE) ? AUDIO_CHUNK_SIZE : static_cast<int>(nRemaining);
            const ssize_t nBytesRead = read(fd, buffer, nToRead);
            if (nBytesRead <= 0)
            {
                break;
            }
            total_read_bytes += static_cast<uint32_t>(nBytesRead);

            if (bFadingOut)
            {
                /* 打断淡出：逐帧线性衰减音量，从当前增益递减至 0，避免突然中断产生爆音 */
                short *pData = reinterpret_cast<short *>(buffer);
                const int nSamples = nBytesRead / 2;
                for (int idx = 0; idx < nSamples; ++idx)
                {
                    pData[idx] = static_cast<short>(pData[idx] * nFadeStepsRemaining / TOTAL_FADE_STEPS);
                }
                --nFadeStepsRemaining;
            }
            else if (nFadeInRemaining > 0)
            {
                /* 非首轮播放时执行淡入：增益从 0 线性升至满幅，防止多次循环衔接处出现爆音 */
                const int nGain = FADE_IN_FRAMES - nFadeInRemaining;
                short *pData = reinterpret_cast<short *>(buffer);
                const int nSamples = nBytesRead / 2;
                for (int idx = 0; idx < nSamples; ++idx)
                {
                    pData[idx] = static_cast<short>(pData[idx] * nGain / FADE_IN_FRAMES);
                }
                --nFadeInRemaining;
            }
            else if (nBytesRead < AUDIO_CHUNK_SIZE || total_read_bytes >= audio_data_len)
            {
                /* 最后一帧：补零到完整帧，并做尾部最多 80 个样本的线性淡出（消除噗声） */
                if (nBytesRead < AUDIO_CHUNK_SIZE)
                {
                    memset(buffer + nBytesRead, 0, AUDIO_CHUNK_SIZE - nBytesRead);
                }
                const int nSamples = nBytesRead / 2;
                const int fade_len = (nSamples > 80) ? 80 : nSamples;
                short *pData = reinterpret_cast<short *>(buffer);
                for (int k = 0; k < fade_len; ++k)
                {
                    const int idx = nSamples - 1 - k;
                    pData[idx] = static_cast<short>(pData[idx] * k / fade_len);
                }
            }

            Audio_NS::AoInfo_S stAoInfo;
            stAoInfo.nChannel = 0;
            stAoInfo.pData = reinterpret_cast<uint8_t *>(buffer);
            stAoInfo.nLen = AUDIO_CHUNK_SIZE;
            stAoInfo.enAudioFormat = Audio_NS::AudioFormat_E::PCM;
            CAVConfigure::instance()->setAoSpeakInfo(stAoInfo);

            memset(buffer, 0, sizeof(buffer));
#if 0
            /* 根据采样率和帧大小计算等时播放间隔，保证数据推送速率与实际播放速率匹配 */
            int sleep_ms = AUDIO_CHUNK_SIZE * 1000 / (static_cast<int>(Audio_NS::AudioSamprate_E::AUDIO_SAMPRATE_16000) * 2);
#if CAP_AUDIO_PLAYBACK_SLEEP_HALF
            /* 部分平台播放速率偏快，缩短等待时间以补偿节奏差异 */
            sleep_ms /= 2;
#endif
            usleep(sleep_ms * 1000);
#else
            /* 等待 AO 硬件缓冲区排空后再推送下一帧，确保数据推送速率与实际播放速率匹配 */
            CAVConfigure::instance()->waitAoDrained(0, -1);
#endif
        }

        if (close(fd) != 0)
        {
            dlog_error("关闭文件错误: %s", strerror(errno));
        }

        if (playCount < nTimes - 1)
        {
            /* 多次播报之间保留短暂间隔，以推送静音帧代替单纯Sleep，防止声卡断流爆音 */
            for (int i = 0; i < 50 && bRunningFlag.load(); ++i)
            {
                CAVConfigure::instance()->setAoSpeakInfo(stSilenceInfo);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    }

    bRunningFlag.store(false);
#endif
    /* 等待尾帧播放完成后关闭功放，V1/V2播放路径均生效 */
    CAVConfigure::instance()->waitAoDrained(0, 200);
    CAVConfigure::instance()->muteAudioOutput();
}

void EventLinkageAsyncAction::execute_email(const LinkageTask_S &stTask, std::atomic<bool> &bRunningFlag)
{
    pthread_setname_np(pthread_self(), "EventLinkEmail");

    /* 邮件正文使用当前事件快照，避免异步执行时再访问外部共享状态 */
    ::Network::EmailEventInfo_S stEventInfo;
    stEventInfo.strSubject = EventLinkageDict::get_event_name(stTask.stContext.enEventType);
    stEventInfo.strMessage = std::string("事件类型: ") + stEventInfo.strSubject + "\n" +
                             "日期: " + stTask.stEventInfo.strDate + "\n" +
                             "时间: " + stTask.stEventInfo.strTime;

    Capture_NS::CaptureParam_S stCaptureParams;
    CCaptureCtrl::instance()->get_captureParam(stCaptureParams);
    if (stTask.bUploadSdCard && stCaptureParams.stCaptureEventConfig.bEnable)
    {
        /* 若事件同时配置了抓图，则优先等待首张图片，便于邮件带图发送 */
        const int CHECK_INTERVAL_MS = 500;
        const int TIMEOUT_MS = 3000;
        const long long llStartTime = TimeUtils_NS::get_currentTimestampMs();

        while (bRunningFlag.load())
        {
            std::string strImageFile;
            if (CCaptureCtrl::instance()->get_event_first_capture_status(stTask.stContext.enEventType, strImageFile))
            {
                stEventInfo.vecImageFile.emplace_back(strImageFile);
                break;
            }

            if (TimeUtils_NS::get_currentTimestampMs() - llStartTime >= TIMEOUT_MS)
            {
                dlog_warn("等待事件类型[%d]首张图片超时", static_cast<int>(stTask.stContext.enEventType));
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(CHECK_INTERVAL_MS));
        }
    }

    if (bRunningFlag.load())
    {
        CEmailManage::instance()->HandleEmail(stEventInfo);
    }
}

void EventLinkageAsyncAction::execute_audio(const LinkageTask_S &stTask, std::atomic<bool> &bRunningFlag)
{
    (void)stTask;
    pthread_setname_np(pthread_self(), "EventLinkAudio");

    /* 对讲占用音频输出时，不再叠加声音联动，避免互相干扰 */
    if (CPreviewManage::instance()->get_intercom_status())
    {
        dlog_info("正在进行对讲，不进行声音联动");
        return;
    }

    std::string strAudioPath;
    int nTimes = 0;
    /* 先从配置中选择要播报的音频文件和次数 */
    if (select_audio_file(strAudioPath, nTimes) != OK)
    {
        return;
    }
    if(stTask.stContext.enEventType == Event::Type_E::FACE_COMPARE_SUCCESS)
    {
        strAudioPath = AUDIO_CONFIG_PATH "face_compare_success.wav";
        dlog_error("音频文件人脸比对成功");
    }
    if (access(strAudioPath.c_str(), F_OK) != 0)
    {
        dlog_error("音频文件不存在: %s", strAudioPath.c_str());
        return;
    }

    /* 记录音频路径 */
    {
        std::lock_guard<std::mutex> lock(m_audioPathMutex);
        m_strPlayingAudioPath = strAudioPath;
    }
    play_audio(strAudioPath, nTimes, bRunningFlag);
    {
        std::lock_guard<std::mutex> lock(m_audioPathMutex);
        m_strPlayingAudioPath.clear();
    }
}

int EventLinkageAsyncAction::get_audio_file_path(std::string &strAudioPath)
{
    int nTimes = 0;
    return select_audio_file(strAudioPath, nTimes);
}

std::string EventLinkageAsyncAction::get_playing_audio_path()
{
    std::lock_guard<std::mutex> lock(m_audioPathMutex);
    return m_strPlayingAudioPath;
}

void EventLinkageAsyncAction::execute_warning_light(std::atomic<bool> &bRunningFlag)
{
    pthread_setname_np(pthread_self(), "EventLinkLight");

    bool bIsFlashing = false;
    int nRemainTime = 0;
    /* 如果白灯已经在闪烁，则沿用当前动作，不重复发起新的闪烁请求 */
    if (CLightManager::instance()->get_flashing_status(LIGHT_TYPE_WHITE, bIsFlashing, nRemainTime) == IpcRet_E::OK &&
        bIsFlashing)
    {
        dlog_info("闪光灯已经在闪烁中，剩余时间: %d秒", nRemainTime);
        return;
    }

    Alarm::FlashInfo_S stFlashAlarm;
    if (CEventConfigure::instance()->get_configure(stFlashAlarm) != 0)
    {
        stFlashAlarm.nFlashTime = 3;
        stFlashAlarm.enFalshFrequency = Alarm::FlashFrequency_E::FLASH_MID_FREQ;
    }

    if (stFlashAlarm.nFlashTime < 1)
    {
        stFlashAlarm.nFlashTime = 1;
    }
    else if (stFlashAlarm.nFlashTime > 300)
    {
        stFlashAlarm.nFlashTime = 300;
    }

    const int nRet = CLightManager::instance()->start_flashing(LIGHT_TYPE_WHITE,
                                                               stFlashAlarm.nFlashTime,
                                                               stFlashAlarm.enFalshFrequency);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("启动闪光灯闪烁失败，错误码: %d", nRet);
        return;
    }

    /* 线程只负责等待闪烁时长结束，便于中途被高优先级任务打断 */
    for (int i = 0; i < stFlashAlarm.nFlashTime && bRunningFlag.load(); ++i)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    /* 恢复补光配置（重新应用） */
    CLightManager::instance()->apply_peripheral_config();
}

void EventLinkageAsyncAction::execute_alarm_io(const LinkageTask_S &stTask, std::atomic<bool> &bRunningFlag)
{
    pthread_setname_np(pthread_self(), "EventLinkAlmIO");

    std::map<int, int> mapAlarmOutput;
    for (const auto &nIoNum : stTask.vecAlarmOutputNum)
    {
        /* 先校验输出口编号，再读取每个IO独立的保持时长配置 */
        if (nIoNum < 0 || nIoNum >= GPIO_OUTPUT_COUNT)
        {
            dlog_error("无效的IO序号: %d", nIoNum);
            continue;
        }

        Alarm::IoOutputInfo_S stIoOutputInfo;
        stIoOutputInfo.nIoNumer = nIoNum;
        CEventConfigure::instance()->get_configure(stIoOutputInfo);
        mapAlarmOutput[nIoNum] = stIoOutputInfo.nDelayTime;
        CGpioCtrl::instance()->alarm_output_on(nIoNum);
    }

    std::vector<std::future<void>> futures;
    for (const auto &item : mapAlarmOutput)
    {
        /* 每个输出口独立计时，互不阻塞，时间到后自动关闭 */
        futures.emplace_back(std::async(std::launch::async, [item, &bRunningFlag]() {
            int nWaitSec = 0;
            while (nWaitSec < item.second && bRunningFlag.load())
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                ++nWaitSec;
            }
            CGpioCtrl::instance()->alarm_output_off(item.first);
        }));
    }

    for (auto &future : futures)
    {
        future.wait();
    }
}

void EventLinkageAsyncAction::execute_log(const LinkageTask_S &stTask)
{
    pthread_setname_np(pthread_self(), "EventLinkLog");
    /* ONVIF订阅事件使用统一推送入口，开始事件传true，结束事件传false */
    COnvifSubscriptionManager::instance()->pushEventToAll(stTask.stContext.enEventType, !stTask.stContext.bEventEnded);
}

int EventLinkageAsyncAction::select_audio_file(std::string &strAudioPath, int &nTimes)
{
    Alarm::SoundOutputAlarm_S stSoundInfo;
    CEventConfigure::instance()->get_configure(stSoundInfo);
    /* 声音联动次数由声音告警配置直接决定 */
    nTimes = stSoundInfo.nTimes;

    switch (stSoundInfo.enSoundType)
    {
    case Alarm::SoundType_E::WARN:
        switch (stSoundInfo.enAlertSound)
        {
        case Alarm::AlertSoundType_E::WARNING_ZONE_LEAVE_IMMEDIATELY:
            strAudioPath = AUDIO_CONFIG_PATH "warning_zone_leave_immediately.wav";
            break;
        case Alarm::AlertSoundType_E::DANGER_ZONE_DO_NOT_APPROACH:
            strAudioPath = AUDIO_CONFIG_PATH "danger_zone_do_not_approach.wav";
            break;
        case Alarm::AlertSoundType_E::NO_PARKING_ZONE:
            strAudioPath = AUDIO_CONFIG_PATH "no_parking_zone.wav";
            break;
        case Alarm::AlertSoundType_E::ENTERING_SURVEILLANCE_ZONE:
            strAudioPath = AUDIO_CONFIG_PATH "entering_surveillance_zone.wav";
            break;
        case Alarm::AlertSoundType_E::WELCOME_GREETING:
            strAudioPath = AUDIO_CONFIG_PATH "welcome_greeting.wav";
            break;
        case Alarm::AlertSoundType_E::DO_NOT_TOUCH_VALUABLES:
            strAudioPath = AUDIO_CONFIG_PATH "do_not_touch_valuables.wav";
            break;
        case Alarm::AlertSoundType_E::PRIVATE_PROPERTY_NO_ENTRY:
            strAudioPath = AUDIO_CONFIG_PATH "private_property_no_entry.wav";
            break;
        case Alarm::AlertSoundType_E::DEEP_WATER_WARNING:
            strAudioPath = AUDIO_CONFIG_PATH "deep_water_warning.wav";
            break;
        case Alarm::AlertSoundType_E::HIGH_PLACE_DANGER:
            strAudioPath = AUDIO_CONFIG_PATH "high_place_danger.wav";
            break;
        case Alarm::AlertSoundType_E::SHRIEK_ALARM:
            strAudioPath = AUDIO_CONFIG_PATH "shriek_alarm.wav";
            break;
        case Alarm::AlertSoundType_E::GENERAL_WARNING_TONE:
            strAudioPath = AUDIO_CONFIG_PATH "general_warning_tone.wav";
            break;
        default:
            dlog_error("未知的警戒音类型: %d", static_cast<int>(stSoundInfo.enAlertSound));
            return ERR;
        }
        break;
    case Alarm::SoundType_E::ALERT:
        strAudioPath = AUDIO_CONFIG_PATH "tip.wav";
        break;
    case Alarm::SoundType_E::CUSTOM:
        for (const auto &customAudio : stSoundInfo.aCustomAudio)
        {
            if (customAudio.bChoose && !customAudio.strPath.empty())
            {
                strAudioPath = customAudio.strPath;
                break;
            }
        }
        if (strAudioPath.empty())
        {
            dlog_error("未找到有效的自定义音频文件");
            return ERR;
        }
        break;
    default:
        dlog_error("未知的音频类型: %d", static_cast<int>(stSoundInfo.enSoundType));
        return ERR;
    }

    return OK;
}
