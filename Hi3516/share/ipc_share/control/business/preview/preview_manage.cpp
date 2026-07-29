/**
 * @FilePath     : preview_manage.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-09-11 21:15:53
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-11-12 10:41:59
 * @Description  : 预览管理
 */

#include "preview_manage.h"

#include "dlog.h"

#include "GbDefine.h"
#include "av_configure.h"
#include "rtsp_server.h"
#include "gpio_ctrl.h"
#include "isp_configure.h"
#include "event_linkage.h"
#include "light_manager.h"

namespace
{
const char *kTvSdkVoiceComSdp = "tvsdk_voicecom";
constexpr int kTvSdkAlarmLightControlType = 2;
constexpr int kTvSdkAlarmLightStart = 1;
constexpr int kTvSdkAlarmLightStop = 2;
constexpr int kTvSdkAlarmLightSetMode = 3;
constexpr int kAlarmLightMinDurationSec = 1;
constexpr int kAlarmLightMaxDurationSec = 300;
constexpr int kFlashFrequencyMin = static_cast<int>(Alarm::FlashFrequency_E::FLASH_STEADY_ON);
constexpr int kFlashFrequencyMax = static_cast<int>(Alarm::FlashFrequency_E::FLASH_HIGH_FREQ);
}

CPreviewManage::CPreviewManage()
{
}

CPreviewManage::~CPreviewManage()
{
    {
        std::lock_guard<std::mutex> timerLock(m_alarmLightTimerMutex);
        m_alarmLightTimerArmed = false;
        m_alarmLightTimerExit = true;
    }
    m_alarmLightTimerCv.notify_all();
    if (m_alarmLightTimerThread.joinable())
    {
        m_alarmLightTimerThread.join();
    }
}

IpcRet_E CPreviewManage::init()
{
	if (OK != Convert::read_file(PREVIEW_CONFIG_FILE, m_stPreviewInfo))
	{	
		Audio_NS::AudioConfig_S stAudioConfig;
		if (0 != CAVConfigure::instance()->get_configure(stAudioConfig))
		{
			dlog_error("获取音频配置失败");
			return ERR;
		}

        m_stPreviewInfo.stRtspUrl.strRtspMainUrl = CRtspServer::instance()->getRtspUrl(RTSP_CHN_MAIN, true);
        m_stPreviewInfo.stRtspUrl.strRtspSubUrl = CRtspServer::instance()->getRtspUrl(RTSP_CHN_SUB, true);

        ISP::ImageParam_S stImageParam;
        if (0 != CIspConfigure::instance()->get_configure(stImageParam))
		{
			dlog_error("获取图像参数失败");
			return ERR;
		}
		m_stPreviewInfo.stImageParam.nBrightness = stImageParam.nBrightness;
		m_stPreviewInfo.stImageParam.nContrast = stImageParam.nContrast;
		m_stPreviewInfo.stImageParam.nSaturation = stImageParam.nSaturation;
		m_stPreviewInfo.stImageParam.nSharpness = stImageParam.nSharpness;

		Convert::write_file(PREVIEW_CONFIG_FILE, m_stPreviewInfo);
	}

	{
        std::lock_guard<std::mutex> lock(m_alarmLightTimerMutex);
        if (!m_alarmLightTimerThread.joinable())
        {
            m_alarmLightTimerExit = false;
            m_alarmLightTimerThread = std::thread(&CPreviewManage::alarm_light_timer_loop, this);
        }
    }

	return OK;
}

IpcRet_E CPreviewManage::deinit()
{
	{
        std::lock_guard<std::mutex> operationLock(m_alarmLightOperationMutex);
        cancel_alarm_light_timer();
        stop_alarm_light_output();
    }

    {
        std::lock_guard<std::mutex> timerLock(m_alarmLightTimerMutex);
        m_alarmLightTimerExit = true;
    }
    m_alarmLightTimerCv.notify_all();
    if (m_alarmLightTimerThread.joinable())
    {
        m_alarmLightTimerThread.join();
    }

	return OK;
}

/* 获取预览信息 */
int CPreviewManage::get_preview_info(Preview::PreviewInfo_S &stInfo)
{
	stInfo.stRtspUrl.strRtspMainUrl = CRtspServer::instance()->getRtspUrl(RTSP_CHN_MAIN, true);
    stInfo.stRtspUrl.strRtspSubUrl = CRtspServer::instance()->getRtspUrl(RTSP_CHN_SUB, true);

    ISP::ImageParam_S stImageParam;
	if (0 != CIspConfigure::instance()->get_configure(stImageParam))
	{
		dlog_error("获取图像参数失败");
		return ERR;
	}
	stInfo.stImageParam.nBrightness = stImageParam.nBrightness;
	stInfo.stImageParam.nContrast = stImageParam.nContrast;
	stInfo.stImageParam.nSaturation = stImageParam.nSaturation;
	stInfo.stImageParam.nSharpness = stImageParam.nSharpness;

	m_stPreviewInfo = stInfo;
	Convert::write_file(PREVIEW_CONFIG_FILE, m_stPreviewInfo);

	return OK;
}

/* 设置预览信息 */
int CPreviewManage::set_preview_info(Preview::PreviewInfo_S stInfo)
{
	m_stPreviewInfo = stInfo;
	return Convert::write_file(PREVIEW_CONFIG_FILE, m_stPreviewInfo);
}

/* 获取采集音频信息 */
int CPreviewManage::get_collect_audio_info(Preview::CollectAudioInfo_S &stInfo)
{
	stInfo.nChn = 1;
	stInfo.nCodec = (int)Audio_NS::AudioFormat_E::G711A;
	stInfo.nBitRate = (int)Audio_NS::AudioBitrate_E::AUDIO_BITRATE_16K;
	stInfo.nSampleRate = (int)Audio_NS::AudioSamprate_E::AUDIO_SAMPRATE_16000;
	stInfo.nFormat = (int)GB28181::SampleFormat_E::S16_LE;
	enCurFormat = Audio_NS::AudioFormat_E::G711A;

	return OK;
}

/* 回调接收到的音频 */
void CPreviewManage::audioDataCallback(const uint8_t* pData, size_t length) 
{
	Audio_NS::AoInfo_S stAoInfo;
	stAoInfo.nChannel = 0; 
	stAoInfo.nLen = length;
	stAoInfo.pData = (uint8_t*)pData;
	stAoInfo.enAudioFormat = enCurFormat;
#if CAP_EVENT_AUDIO_PLAYBACK_V2
	stAoInfo.enSource = Audio_NS::AoSource_E::AO_SOURCE_REALTIME;  /* 对讲/广播 */
#endif
	CAVConfigure::instance()->setAoSpeakInfo(stAoInfo);
}

/* 设置对讲信息 */
int CPreviewManage::set_intercom_info(Preview::IntercomInfo_S stInfo)
{
	if(stInfo.bEnable)
	{
		dlog_debug("开始对讲");
        /* 停止事件联动中声音联动 */
        if (!CEventLinkage::instance()->stop_play_audio())
        {
            /* 如果正在联动，关闭后睡眠一会 */
            usleep(200 * 1000);
        }

        m_bIntercomStatus = true;
		m_strIp = stInfo.strLocalIp;

        if (stInfo.strSdp == kTvSdkVoiceComSdp && stInfo.strUrl.empty())
        {
            dlog_info("TVSDK VoiceCom对讲开启, 跳过RTP接收器初始化");
            return OK;
        }

		/* 先停止旧的 */ 
		if (m_intercomReceiver && m_intercomReceiver->isRunning())
		{
			m_intercomReceiver->stop();
		}

		/* 然后再新建 */ 
		m_intercomReceiver.reset(new RtpAudioReceiver());
		/* 初始化Rtp */
		if(m_intercomReceiver->init(stInfo.strUrl) != OK)
		{
			dlog_error("m_intercomReceiver->init error");
			m_bIntercomStatus = false;
			return ERR;
		}
		/* 开启Rtp接收 */
		m_intercomReceiver->start();
		/* 设置音频回调 */
		m_intercomReceiver->setDataCallback([this](const uint8_t* pData, size_t len) {
    		this->audioDataCallback(pData, len);
		});

	}
	else
	{
		m_bIntercomStatus = false;
		m_strIp = "";
		dlog_debug("停止对讲");
		if (m_intercomReceiver)
        {
            m_intercomReceiver->stop();
        }

#if CAP_EVENT_AUDIO_PLAYBACK_V2
        /* 对讲结束，立即关闭功放消除结尾噗声 */
        CAVConfigure::instance()->muteAudioOutput();
#endif
#if CAP_IO_EXTERNAL_DDR_00S
        CAVConfigure::instance()->waitAoDrained(0, 200);
        CAVConfigure::instance()->muteAudioOutput();
#endif
    }

	return OK;
}

/* 设置广播信息 */
int CPreviewManage::set_broadcast_info(Preview::BroadcastInfo_S stInfo)
{
	if(stInfo.bEnable)
	{
		dlog_debug("开始广播");
        /* 停止事件联动中声音联动 */
        if (!CEventLinkage::instance()->stop_play_audio())
        {
            /* 如果正在联动，关闭后睡眠一会 */
            usleep(200 * 1000);
        }

        m_bIntercomStatus = true;
		m_strIp = stInfo.strLocalIp;
		/* 先停止旧的 */ 
		if (m_broadcastReceiver && m_broadcastReceiver->isRunning())
		{
			m_broadcastReceiver->stop();
		}

		/* 然后再新建 */ 
		m_broadcastReceiver.reset(new RtpAudioReceiver());
		/* 初始化Rtp */
		if(m_broadcastReceiver->init(stInfo.strUrl) != OK)
		{
			dlog_error("m_broadcastReceiver->init error");
			m_bIntercomStatus = false;
			return ERR;
		}
		/* 开启Rtp接收 */
		m_broadcastReceiver->start();
		/* 设置音频回调 */
		m_broadcastReceiver->setDataCallback([this](const uint8_t* pData, size_t len) {
    		this->audioDataCallback(pData, len);
		});
	}
	else
	{
		m_bIntercomStatus = false;
		m_strIp = "";
		dlog_debug("停止广播");
		if (m_broadcastReceiver)
        {
            m_broadcastReceiver->stop();
        }

#if CAP_EVENT_AUDIO_PLAYBACK_V2
        /* 广播结束，立即关闭功放消除结尾噗声 */
        CAVConfigure::instance()->muteAudioOutput();
#endif
#if CAP_IO_EXTERNAL_DDR_00S
        /* 广播结束，立即关闭功放消除结尾噗声 */
        CAVConfigure::instance()->waitAoDrained(0, 200);
        CAVConfigure::instance()->muteAudioOutput();
        #endif
    }

	return OK;
}

/* 设置蜂鸣器报警信息 */
int CPreviewManage::set_beep_alarm(Preview::BeepAlarm_S stInfo)
{
	if(stInfo.bEnable)
	{
		dlog_debug("开启报警");
		CGpioCtrl::instance()->alarm_output_on(0);
	}
	else
	{
		dlog_debug("停止报警");
		CGpioCtrl::instance()->alarm_output_off(0);
	}

	return OK;
}

int CPreviewManage::device_control(const Preview::DeviceControl_S &stInfo)
{
    if (stInfo.nChannelId < 0 || stInfo.nControlType != kTvSdkAlarmLightControlType)
    {
        dlog_error("TVSDK设备控制参数非法: channel[%d], type[%d]",
                   stInfo.nChannelId,
                   stInfo.nControlType);
        return ERR;
    }

#if !CAP_ALARM_IO
    dlog_warn("当前设备不支持声光控制: 未启用 CAP_ALARM_IO");
    return ERR;
#else
    if (stInfo.nCommand == kTvSdkAlarmLightStop)
    {
        std::lock_guard<std::mutex> operationLock(m_alarmLightOperationMutex);
        cancel_alarm_light_timer();
        return stop_alarm_light_output();
    }

    if (stInfo.nCommand != kTvSdkAlarmLightStart && stInfo.nCommand != kTvSdkAlarmLightSetMode)
    {
        dlog_warn("TVSDK声光控制命令不支持: command[%d]", stInfo.nCommand);
        return ERR;
    }

    if (stInfo.nDurationMs < 0 || stInfo.nDurationMs > kAlarmLightMaxDurationSec * 1000 ||
        stInfo.nParam1 < kFlashFrequencyMin || stInfo.nParam1 > kFlashFrequencyMax)
    {
        dlog_error("TVSDK声光控制参数非法: durationMs[%d], frequency[%d]",
                   stInfo.nDurationMs,
                   stInfo.nParam1);
        return ERR;
    }

    int nDurationSec = kAlarmLightMinDurationSec;
    if (stInfo.nDurationMs > 0)
    {
        nDurationSec = (stInfo.nDurationMs + 999) / 1000;
    }

    const auto enFrequency = static_cast<Alarm::FlashFrequency_E>(stInfo.nParam1);
    std::lock_guard<std::mutex> operationLock(m_alarmLightOperationMutex);

    /* 先更新计时器代次，避免上一轮超时动作关闭本次刚开启的声光。 */
    arm_alarm_light_timer(nDurationSec);
    const int nLightRet = CLightManager::instance()->start_flashing(LIGHT_TYPE_WHITE,
                                                                      nDurationSec,
                                                                      enFrequency);
    if (nLightRet != OK)
    {
        cancel_alarm_light_timer();
        dlog_error("TVSDK启动闪光灯失败: ret[%d]", nLightRet);
        return nLightRet;
    }

    CGpioCtrl::instance()->alarm_output_on(0);
    dlog_info("TVSDK声光报警已开启: channel[%d], duration[%d]s, frequency[%d]",
              stInfo.nChannelId,
              nDurationSec,
              stInfo.nParam1);
    return OK;
#endif
}

void CPreviewManage::arm_alarm_light_timer(int nDurationSec)
{
    std::lock_guard<std::mutex> timerLock(m_alarmLightTimerMutex);
    ++m_alarmLightTimerGeneration;
    m_alarmLightTimerArmed = true;
    m_alarmLightDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(nDurationSec);
    m_alarmLightTimerCv.notify_all();
}

void CPreviewManage::cancel_alarm_light_timer()
{
    std::lock_guard<std::mutex> timerLock(m_alarmLightTimerMutex);
    ++m_alarmLightTimerGeneration;
    m_alarmLightTimerArmed = false;
    m_alarmLightTimerCv.notify_all();
}

int CPreviewManage::stop_alarm_light_output()
{
#if CAP_ALARM_IO
    CGpioCtrl::instance()->alarm_output_off(0);
#endif

    const int nLightRet = CLightManager::instance()->stop_flashing(LIGHT_TYPE_WHITE);
    const int nRestoreRet = CLightManager::instance()->apply_peripheral_config();
    if (nLightRet != OK || nRestoreRet != OK)
    {
        dlog_error("TVSDK停止声光报警失败: stopLight[%d], restoreLight[%d]", nLightRet, nRestoreRet);
        return ERR;
    }

    dlog_info("TVSDK声光报警已停止");
    return OK;
}

void CPreviewManage::alarm_light_timer_loop()
{
    while (true)
    {
        uint64_t nGeneration = 0;
        std::chrono::steady_clock::time_point deadline;

        {
            std::unique_lock<std::mutex> timerLock(m_alarmLightTimerMutex);
            m_alarmLightTimerCv.wait(timerLock, [this]() {
                return m_alarmLightTimerExit || m_alarmLightTimerArmed;
            });

            if (m_alarmLightTimerExit)
            {
                return;
            }

            nGeneration = m_alarmLightTimerGeneration;
            deadline = m_alarmLightDeadline;
            if (m_alarmLightTimerCv.wait_until(timerLock, deadline, [this, nGeneration]() {
                    return m_alarmLightTimerExit || !m_alarmLightTimerArmed ||
                           m_alarmLightTimerGeneration != nGeneration;
                }))
            {
                continue;
            }
        }

        std::lock_guard<std::mutex> operationLock(m_alarmLightOperationMutex);
        bool bNeedStop = false;
        {
            std::lock_guard<std::mutex> timerLock(m_alarmLightTimerMutex);
            if (!m_alarmLightTimerExit && m_alarmLightTimerArmed &&
                m_alarmLightTimerGeneration == nGeneration)
            {
                m_alarmLightTimerArmed = false;
                bNeedStop = true;
            }
        }

        if (bNeedStop)
        {
            dlog_info("TVSDK声光报警时间到，自动停止");
            stop_alarm_light_output();
        }
    }
}

bool CPreviewManage::get_intercom_status()
{
    return m_bIntercomStatus;
}

std::string CPreviewManage::get_intercom_ip()
{
	return m_strIp;
}
