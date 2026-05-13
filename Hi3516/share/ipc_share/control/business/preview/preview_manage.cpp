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

CPreviewManage::CPreviewManage()
{
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
	return OK;
}

IpcRet_E CPreviewManage::deinit()
{
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

bool CPreviewManage::get_intercom_status()
{
    return m_bIntercomStatus;
}

std::string CPreviewManage::get_intercom_ip()
{
	return m_strIp;
}
