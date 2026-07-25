/**
 * @FilePath     : record_file.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-28 10:36:11
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-04 09:02:02
 * @Description  : 通道通讯录制
 */

#include <sys/stat.h>
#include "record_file.h"
#include "dlog.h"
#include "Json.h"
#include "action_code.h"
#include "time_tools.h"
#include "record_convert.h"
#include "record_client.h"
#include "convert_interface.h"
#include "path_define.h"
#include <cerrno>
#include <cstring>

CRecordFile::CRecordFile(int nChnId)
	: m_nChnId(nChnId)
#if defined(DEVICE_TV_3882TI) || defined(DEVICE_TV_3881T)
	, m_mediaDataQueue(1000)
#else
	, m_mediaDataQueue(500)
#endif
{
	m_nRecordStatus.store(Record_NS::NO_OPERATION);
	m_bFirstInit.store(false);
	m_bDisconnect.store(false);

	m_bRunning.store(true);
	m_recordTd = std::thread(&CRecordFile::thread_record, this);

	/*读取视频配置文件*/
	Convert::read_file(VIDEO_CONFIG_FILE, m_vstVideoConfig);
	/*判断视频配置对应的录制的视频配置信息*/
	Record_NS::VideoConfigInfo_S stVideoConfigInfo;
#if CAP_RECORD_USE_MAIN_STREAM
	/* 录制使用主码流 */
	stVideoConfigInfo.nVencWidth = m_vstVideoConfig[0].stVideoResolution.nWidth;
	stVideoConfigInfo.nVencHeight = m_vstVideoConfig[0].stVideoResolution.nHeight;
	stVideoConfigInfo.nFps = m_vstVideoConfig[0].getFrameRateAsInt();
	stVideoConfigInfo.nVideoCodeID = AV_CODEC_ID_H264;
	if (m_vstVideoConfig[0].enVideoCodec == Video_NS::VideoCodec_E::H264)
	{
		stVideoConfigInfo.nVideoCodeID = AV_CODEC_ID_H264;
	}
	else if (m_vstVideoConfig[0].enVideoCodec == Video_NS::VideoCodec_E::H265)
	{
		stVideoConfigInfo.nVideoCodeID = AV_CODEC_ID_H265;
	}
#else
	/* 录制使用子码流 */
	stVideoConfigInfo.nVencWidth = m_vstVideoConfig[1].stVideoResolution.nWidth;
	stVideoConfigInfo.nVencHeight = m_vstVideoConfig[1].stVideoResolution.nHeight;
	stVideoConfigInfo.nFps = m_vstVideoConfig[1].getFrameRateAsInt();
	stVideoConfigInfo.nVideoCodeID = AV_CODEC_ID_H264;
	if (m_vstVideoConfig[1].enVideoCodec == Video_NS::VideoCodec_E::H264)
	{
		stVideoConfigInfo.nVideoCodeID = AV_CODEC_ID_H264;
	}
	else if (m_vstVideoConfig[1].enVideoCodec == Video_NS::VideoCodec_E::H265)
	{
		stVideoConfigInfo.nVideoCodeID = AV_CODEC_ID_H265;
	}
#endif
	/*设置录制的视频配置信息*/
	set_videoInfo(stVideoConfigInfo);

	Record_NS::AudioConfigInfo_S stAudioConfigInfo;
	stAudioConfigInfo.nSampleRate = 16000;
	stAudioConfigInfo.nAudioCodeID = AV_CODEC_ID_AAC;
	stAudioConfigInfo.nSampleFmt = AV_SAMPLE_FMT_S16;
	stAudioConfigInfo.nChannel = 1;
	/*设置录制的音频配置信息*/
	set_audioInfo(stAudioConfigInfo);
}

CRecordFile::~CRecordFile()
{
	/* 停止录制 */
	m_nRecordStatus.store(Record_NS::STOP_OPERATION);
	m_stSliceInfo.nVideoFlag = 0;
	m_stSliceInfo.nAudioFlag = 0;

	/* 结束线程 */
	m_bRunning.store(false);
	m_recordTd.join();

	m_ffmpegRecord.deinit();
	/* 释放掉存储媒体数据 */
	clear_mediaDataQueue();
}

static bool symlink_exists(const char* path)
{
    struct stat st;
    if (lstat(path, &st) == 0) /* 路径存在 */
	{
		return S_ISLNK(st.st_mode);      /* 并且是软链接 */ 
	}
    else if (errno == ENOENT) /* 路径不存在 */
	{
		return false;            
	}
    else
	{
		dlog_error("lstat error: %s", std::strerror(errno))
        return false;                   /* 按不存在处理 */ 
    }
}

void CRecordFile::start(Record_NS::Info_S &stRecordInfos)
{
	if (m_nRecordStatus.load() == Record_NS::RECORD_OPERATION)
	{
		return;
	}

	while(symlink_exists(RECORD_PATH) == false)
	{
		// dlog_debug("未检测到软链接");
		usleep(10*1000);
	}
	/* 每次重新开始录制前清楚上一次可能遗留下来的的帧数据 */
	clear_mediaDataQueue();

	m_stSliceInfo.nEventFlag = (Record_NS::Event_E)stRecordInfos.nEventType;
	m_ffmpegRecord.reset_lastPts();
	m_ffmpegRecord.init(m_stSliceInfo);
	m_stRecordInfo = stRecordInfos;
	m_strRecvStratTime = Time::get_hhmmss();
	m_nRecordStatus.store(Record_NS::RECORD_OPERATION);

	dlog_info("启动录制");
}
void CRecordFile::pause()
{
	m_nRecordStatus.store(Record_NS::PAUSE_OPERATION);
	dlog_info("暂停录制");
}

void CRecordFile::stop()
{
	m_nRecordStatus.store(Record_NS::STOP_OPERATION);
	if (m_ffmpegRecord.is_init())
	{
		clear_mediaDataQueue();
		m_ffmpegRecord.deinit();
		m_m3u8.add_ts(m_ffmpegRecord.get_mediaInfo());
		m_ffmpegRecord.reset();
		m_bFirstInit.store(false);
	}
	dlog_info("停止录制");
}

void CRecordFile::set_videoInfo(Record_NS::VideoConfigInfo_S stVideoConfigInfo)
{
	/*检测视频配置是否发生变化*/
	bool bConfigChanged = false;
	
	/*检查分辨率是否变化*/
	if (m_stSliceInfo.nVencWidth != stVideoConfigInfo.nVencWidth || 
		m_stSliceInfo.nVencHeight != stVideoConfigInfo.nVencHeight) {
		dlog(LOG_FAULT, "分辨率变化: %dx%d ==> %dx%d", 
			m_stSliceInfo.nVencWidth, m_stSliceInfo.nVencHeight,
			stVideoConfigInfo.nVencWidth, stVideoConfigInfo.nVencHeight);
		bConfigChanged = true;
	}
	
	/*检查帧率是否变化*/
	if (m_stSliceInfo.nFps != stVideoConfigInfo.nFps) {
		dlog(LOG_FAULT, "帧率变化: %d ==> %d", m_stSliceInfo.nFps, stVideoConfigInfo.nFps);
		bConfigChanged = true;
	}
	
	/*检查视频编码格式是否变化*/
	if (m_stSliceInfo.nVideoCodeID != stVideoConfigInfo.nVideoCodeID) {
		dlog(LOG_FAULT, "视频格式变化: %d ==> %d", m_stSliceInfo.nVideoCodeID, stVideoConfigInfo.nVideoCodeID);
		bConfigChanged = true;
	}
	
	/*如果配置发生变化，设置切片标志*/
	if (bConfigChanged)
	{
		m_bHandleSlice.store(true);
		dlog(LOG_FAULT, "视频配置变化，将触发切片");
	}
	
	/*更新视频配置信息*/
	m_stSliceInfo.nVencWidth = stVideoConfigInfo.nVencWidth;
	m_stSliceInfo.nVencHeight = stVideoConfigInfo.nVencHeight;
	m_stSliceInfo.nFps = stVideoConfigInfo.nFps;
	m_stSliceInfo.nVideoCodeID = stVideoConfigInfo.nVideoCodeID;
	m_stSliceInfo.nVideoFlag = 1;
	if (m_stSliceInfo.nRealFrameRate == 0)
	{
		m_stSliceInfo.nRealFrameRate = m_stSliceInfo.nFps;
	}
	dlog_debug("%dx%d fps:%d nVideoCodeID:%d", m_stSliceInfo.nVencWidth, m_stSliceInfo.nVencHeight, m_stSliceInfo.nFps, m_stSliceInfo.nVideoCodeID);
}

void CRecordFile::set_audioInfo(Record_NS::AudioConfigInfo_S stAudioConfigInfo)
{
	m_stSliceInfo.nSampleRate = stAudioConfigInfo.nSampleRate;
	m_stSliceInfo.nAudioCodeID = stAudioConfigInfo.nAudioCodeID;
	m_stSliceInfo.nSampleFmt = stAudioConfigInfo.nSampleFmt;
	m_stSliceInfo.nChannel = stAudioConfigInfo.nChannel;
	m_stSliceInfo.nAudioFlag = 1;
	dlog_debug("nSampleRate:%d nSampleFmt:%d nChannel:%d nAudioCodeID:%d", m_stSliceInfo.nSampleRate, m_stSliceInfo.nSampleFmt, m_stSliceInfo.nChannel, m_stSliceInfo.nAudioCodeID);
}

void CRecordFile::push(const void *pData, int nLen, Record_NS::MediaDataType_E enType)
{
	if (!m_bRunning)
	{
		return;
	}

	Record_NS::MediaData_S stRecvData;
	stRecvData.enType = enType;
	stRecvData.nSize = nLen;
	stRecvData.pData = std::shared_ptr<char[]>(new char[nLen]);
	if (stRecvData.pData == nullptr)
	{
		return;
	}
	memcpy(stRecvData.pData.get(), pData, nLen);
	switch (stRecvData.enType)
	{
	case Record_NS::MediaDataType::VIDEO_DATA:
		if (!m_stSliceInfo.nVideoFlag)
		{
			return;
		}

		/* 判断是否为I帧 */
		if (m_stSliceInfo.nVideoCodeID == AV_CODEC_ID_H264)
		{
			stRecvData.bIFrame = CDataTools::isIFrame_h264(static_cast<const char *>(pData), nLen);
		}
		else if (m_stSliceInfo.nVideoCodeID == AV_CODEC_ID_H265)
		{
			stRecvData.bIFrame = CDataTools::isIFrame_h265(static_cast<const char *>(pData), nLen);
		}
		else
		{
			return;
		}
		break;
	case Record_NS::MediaDataType::AUDIO_DATA:
		if (!m_stSliceInfo.nAudioFlag)
		{
			return;
		}
		if (m_stSliceInfo.nChannel == 1)
		{
			m_stSliceInfo.nChannel = AV_CH_LAYOUT_MONO;
		}
		else if (m_stSliceInfo.nChannel == 2)
		{
			m_stSliceInfo.nChannel = AV_CH_LAYOUT_STEREO;
		}
		break;
	default:
		break;
	}
	push_mediaDataQueue(stRecvData);
}

int CRecordFile::send_tsFileInfo()
{
	std::string date = Time::get_yyyymmdd();

	Record_NS::FileInfo_S stFileInfo;
	stFileInfo.nChnId = m_nChnId;
	if (stFileInfo.nChnId >= (Record_NS::RECORD_REPLAY_NUM / 2))
	{
		stFileInfo.nChnId -= (Record_NS::RECORD_REPLAY_NUM / 2);
	}

	stFileInfo.path = m_stRecordInfo.path + "/" + date;

	if (!m_stRecordInfo.redunPath.empty())
	{
		stFileInfo.redunPath = m_stRecordInfo.redunPath + "/" + date;
	}
	SliceInfo_S stSliceInfo = m_ffmpegRecord.get_mediaInfo();
	stFileInfo.filename = stSliceInfo.filename;
	/* 截取出文件名 */
	size_t pos = stFileInfo.filename.find_last_of('/');
	if (pos != std::string::npos)
	{
		stFileInfo.filename = stFileInfo.filename.substr(pos + 1);
	}

	stFileInfo.nType = 0;
	stFileInfo.createTime = stSliceInfo.startTime;
	/* 分片文件大小 */
	// stFileInfo.nSize = stSliceInfo.nSize;
	struct stat st;
	std::string strTsFullPath = stFileInfo.path + "/" + stFileInfo.filename;
    if (stat(strTsFullPath.c_str(), &st) == 0) /*获取ts文件的实际大小 */
    {
        stFileInfo.nSize = st.st_size;
    }
    else /* 这个获取的大小只是 编码数据大小，而 TS 文件最终大小 = 编码数据 + TS 封装开销 */
    {
        stFileInfo.nSize = stSliceInfo.nSize;
    }

    /* 算出分片总时长 */
	auto tp1 = std::chrono::system_clock::from_time_t(stSliceInfo.nStartTimeMs);
	auto tp2 = std::chrono::system_clock::from_time_t(stSliceInfo.nEndTimeMs);
	stFileInfo.nDuration = std::chrono::duration_cast<std::chrono::seconds>(tp2 - tp1).count() / 1000;
	stFileInfo.nIndex = stSliceInfo.nIndex;

	/* 转换为tm结构体 */
	stFileInfo.modifyTime = Time::get_curTime();

	std::string strData = Convert::to_string(stFileInfo);
	CRecordClient::instance()->fill_head(strData, AC_NOTICE_RECORD_TS_FILE_INFO);
	CRecordClient::instance()->send(strData, AC_NOTICE_RECORD_TS_FILE_INFO);
	return 0;
}

int CRecordFile::send_m3u8Info(std::string strEventM3u8FileName)
{
	Record_NS::FileInfo_S stFileInfo;
	stFileInfo.nChnId = m_nChnId;
	if (stFileInfo.nChnId >= (Record_NS::RECORD_REPLAY_NUM / 2))
	{
		stFileInfo.nChnId -= (Record_NS::RECORD_REPLAY_NUM / 2);
	}
	std::string date = Time::get_yyyymmdd();
	if (m_stRecordInfo.nEventType == 0)
	{
		stFileInfo.filename = "normal_" + date + ".m3u8";
	}
	else
	{
		stFileInfo.filename = strEventM3u8FileName;
	}

	char achPach[520] = {0};
	snprintf(achPach, sizeof(achPach), "%s/%s", m_stRecordInfo.path.c_str(), date.c_str());
	stFileInfo.path = achPach;

	if (m_stRecordInfo.redunPath.length() > 0)
	{
		snprintf(achPach, sizeof(achPach), "%s/%s", m_stRecordInfo.redunPath.c_str(), date.c_str());
		stFileInfo.redunPath = achPach;
	}
	stFileInfo.nType = 0;
	stFileInfo.createTime = Time::get_curTime();

	stFileInfo.nDuration = 0;
	stFileInfo.modifyTime = Time::get_curTime();

	std::string strData = Convert::to_string(stFileInfo);
	CRecordClient::instance()->fill_head(strData, AC_NOTICE_RECORD_FILE_INFO);
	CRecordClient::instance()->send(strData, AC_NOTICE_RECORD_FILE_INFO);
	return 0;
}

RecordData_S CRecordFile::to_ffmpegData(Record_NS::MediaData_S &stMediaData)
{
	RecordData_S stFfData;
	stFfData.pData = reinterpret_cast<unsigned char *>(stMediaData.pData.get());
	stFfData.nKey = stMediaData.bIFrame;
	stFfData.nSize = stMediaData.nSize;

	/* 判断媒体数据类型 */
	if (Record_NS::MediaDataType_E::VIDEO_DATA == stMediaData.enType)
	{
		stFfData.nType = 0;
	}
	else if (Record_NS::MediaDataType_E::AUDIO_DATA == stMediaData.enType)
	{
		stFfData.nType = 1;
	}
	return stFfData;
}

void CRecordFile::update_recordDate()
{
	m_curRecordDate = Time::get_yyyymmdd();
}

bool CRecordFile::is_newDay()
{
	if (m_curRecordDate == Time::get_yyyymmdd())
	{
		return false;
	}
	return true;
}

void CRecordFile::slice()
{
	/********************反初始化流程*************/
	/*保存上次计数值*/
	nVptsMs = m_ffmpegRecord.get_videoPts();
	nAptsMs = m_ffmpegRecord.get_audioPts();

	m_ffmpegRecord.deinit();

	/* 获取分片信息（在deinit后获取，包含正确的nIndex和时间戳）*/
	SliceInfo_S stSliceInfo = m_ffmpegRecord.get_mediaInfo();

	/* 连续分片下标自增（在deinit后进行，确保获取的nIndex正确）*/
	m_stSliceInfo.nIndex++;

	/*更新m3u8Path（使用新的nIndex）*/
	if (m_stRecordInfo.nEventType == 0)
	{
		/* 常规录像 */
		m_m3u8Path = m_stRecordInfo.path + "/" + Time::get_yyyymmdd() + "/normal_" + Time::get_yyyymmdd() + ".m3u8";
	}
	else
	{
		if(!m_strRecvStratTime.empty())
		{
			/* 事件录像 */
			std::string strEventM3u8FileName = Time::get_yyyymmdd() + "_" + m_strRecvStratTime + "_" + std::to_string(m_stRecordInfo.nEventType) + "_" + ".m3u8";
			m_m3u8Path = m_stRecordInfo.path + "/" + Time::get_yyyymmdd() + "/" + strEventM3u8FileName;
		}
	}

#if 0
	/*分片总时长*/
	float fSliceTimeMs = m_ffmpegRecord.get_endTimestampMs() - m_ffmpegRecord.get_startTimeStampMs();
	/*计算帧率*/
	float fFrameRate = 0.5 + 1000 * m_ffmpegRecord.get_videoCount() / (float)fSliceTimeMs;
	if(fSliceTimeMs > 0)
	{
		/*判断计算的帧率和发送过来的帧率相差较大时修改帧率*/
		if((abs(m_stSliceInfo.nRealFrameRate - fFrameRate) > 3) && fFrameRate > 20)
		{
			int nNewFrameRate = fFrameRate;
			dlog_debug("帧率改变 %d ==> %d\n",m_stSliceInfo.nRealFrameRate,nNewFrameRate);
			m_stSliceInfo.nRealFrameRate = nNewFrameRate;
		}
	}
#endif
	
	/*判断年月日不等,日期变更*/
	if (is_newDay())
	{
		/*日期变更*/
		dlog_debug("日期变更 %s ==> %s\n", m_curRecordDate.c_str(), Time::get_yyyymmdd().c_str());
		update_recordDate();
		m_ffmpegRecord.reset_lastPts();

		nVptsMs = 0;
		nAptsMs = 0;
		m_stSliceInfo.nIndex = 0;
	}
	
	/*使用更新后的m_stSliceInfo添加到m3u8*/
	m_m3u8.add_ts(std::move(stSliceInfo));
	send_tsFileInfo();
	m_ffmpegRecord.reset();
}

void CRecordFile::redun_backup()
{
	/*判断开启了冗余录像功能，拷贝ts文件和m3u8文件*/
	if (m_stRecordInfo.redunPath.empty())
	{
		return;
	}
	
	/*确保m_redunPath已初始化*/
	if (m_redunPath.empty())
	{
		m_redunPath = m_stRecordInfo.redunPath + "/" + Time::get_yyyymmdd();
	}
	
	std::string cmd;
	if (access(m_redunPath.c_str(), F_OK) != 0)
	{
		cmd = "mkdir -p " + m_redunPath;
		if (system(cmd.c_str()) != 0)
		{
			dlog_error("执行命令[%s]失败", cmd.c_str());
		}
	}

	cmd = "cp " + m_m3u8Path + " " + m_redunPath + " &";
	if (system(cmd.c_str()) != 0)
	{
		dlog_error("执行命令[%s]失败", cmd.c_str());
	}
	auto stSliceInfo = m_ffmpegRecord.get_mediaInfo();
	cmd = "cp " + stSliceInfo.filename + " " + m_redunPath + " &";
	if (system(cmd.c_str()) != 0)
	{
		dlog_error("执行命令[%s]失败", cmd.c_str());
	}
}

int CRecordFile::get_chnId()
{
	return m_nChnId;
}

int CRecordFile::init_record()
{
	if (m_stRecordInfo.path.length() <= 0)
	{
		dlog_error("RecordInfo path length is NULL");
		return -1;
	}

	/* 创建新的句柄 */
	SliceInfo_S stNeedParam = m_stSliceInfo;

	std::string strEventM3u8FileName;

	std::string date = Time::get_yyyymmdd();
	/* 获取当前年月日时分秒 */
	stNeedParam.startTime = Time::get_curTime();

	/* 不在这里设置nStartTimeMs，让ffmpeg_record的init_startTime()处理 */
	/* stNeedParam.nStartTimeMs = Time::get_milliseconds(); */

	/* 获取当前的时分秒 */
	std::string strDateTime = Time::get_hhmmss();

	/* ts文件 */
	std::string path = m_stRecordInfo.path + "/" + date + "/";
	stNeedParam.filename = path + date + "_" + strDateTime + ".ts";
	/* 创建ts文件所在目录 */
	CDataTools::createFile(path);

	/* 冗余文件 */
	m_redunPath = m_stRecordInfo.redunPath + "/" + date;
	if (m_stRecordInfo.nEventType == 0)
	{
		/* 常规录像 */
		m_m3u8Path = m_stRecordInfo.path + "/" + date + "/normal_" + date + ".m3u8";
	}
	else if(m_stRecordInfo.nEventType == 1)
	{
		if(!m_strRecvStratTime.empty())
		{
			/* 事件录像 */
			strEventM3u8FileName = date + "_" + m_strRecvStratTime + "_" + std::to_string(m_stRecordInfo.nEventType) + "_" + ".m3u8";
			m_m3u8Path = m_stRecordInfo.path + "/" + date + "/" + strEventM3u8FileName;
			m_strRecvStratTime = std::string();
		}
	} 

	/* 创建m3u8文件 */
	int nRet = m_m3u8.set_path(m_m3u8Path);
	if (nRet == 0)
	{
		/* 发送给control */
		send_m3u8Info(strEventM3u8FileName);
	}
	/* 设置ffmpeg录像信息 */
	m_ffmpegRecord.set_mediaInfo(stNeedParam);
	nRet = m_ffmpegRecord.init(stNeedParam);
	if(nRet < 0)
	{
		dlog_error("ffmpeg record init error")
		return -1;
	}

	/* 初始化完成后，设置之前保存的PTS值 */
	m_ffmpegRecord.set_audioPts(nAptsMs);
	m_ffmpegRecord.set_videoPts(nVptsMs);

	/* 清空计数，让init_startTime()重新计算 */
	m_ffmpegRecord.clear_count();

	return 0;
}

/* 录制的逻辑 */
void CRecordFile::write_record()
{
	/* 媒体数据 */
	Record_NS::MediaData_S stMediaData;
	/* 从队列中拿一个数据出来 */
	int nRet = pop_mediaDataQueue(stMediaData);
	if (nRet < 0)
	{
		/* 等待30ms */
		std::this_thread::sleep_for(std::chrono::milliseconds(30));
		return;
	}

	if(m_nRecordStatus.load() == Record_NS::STOP_OPERATION || m_nRecordStatus.load() == Record_NS::PAUSE_OPERATION)
	{
		return ;
	}

	/* 录制句柄需要的数据参数 */
	RecordData_S stFfData = to_ffmpegData(stMediaData);

	/*开始录制，判断还没初始化，清空变量，初始化录制*/
	if (!m_bFirstInit.load())
	{
		if (init_record() == -1)
		{
			dlog_debug("初始化录制失败");
			return;
		}
		else
		{
			m_bFirstInit.store(true);
			dlog_debug("初始化录制");
			if (is_newDay())
			{
				/*日期变更*/
				dlog_debug("日期变更 %s ==> %s\n", m_curRecordDate.c_str(), Time::get_yyyymmdd().c_str());
				update_recordDate();
				m_ffmpegRecord.reset_lastPts();
			}
		}
	}

	if (!m_ffmpegRecord.is_init())
	{
		dlog_warn("录制句柄未初始化");
		return;
	}

	/*检查视频配置是否变化，如果变化则触发切片*/
	if (m_bHandleSlice.load() && stFfData.nType == AVMEDIA_TYPE_VIDEO && stFfData.nKey)
	{
		dlog_info("检测到视频配置变化，触发切片");
		
		/* 分片 */
		slice();
		/* 冗余备份 */
		redun_backup();
		
		/* 清空队列中的旧数据，避免使用旧配置 */
		clear_mediaDataQueue();
		
		/***********初始化流程*************/
		if(m_nRecordStatus.load() == Record_NS::RECORD_OPERATION)
		{
			if (init_record() == -1)
			{
				dlog_debug("初始化录制失败");
				m_bHandleSlice.store(false);
				return;
			}
			/* init_record()内部已处理PTS设置，无需再次设置 */
			
			/*重置配置变化标志*/
			m_bHandleSlice.store(false);
		}
	}
	
	/*判断当前片段时间有没有 SLICING_TIME 秒,并且是否为关键帧，写尾，分片*/
	
	if( (stFfData.nType == AVMEDIA_TYPE_VIDEO && stFfData.nKey && (m_ffmpegRecord.get_durationMs() / 1000) >= SLICING_TIME && m_ffmpegRecord.get_videoCount() != 0)
		|| (is_newDay() && stFfData.nType == AVMEDIA_TYPE_VIDEO && stFfData.nKey && m_ffmpegRecord.get_videoCount() != 0))
	{
		/* 分片 */
		slice();
		/* 冗余备份 */
		redun_backup();
		// /* 写入分片后清理队列里面旧的媒体数据 */
		// clear_mediaDataQueue();

		/***********初始化流程*************/
		if(m_nRecordStatus.load() == Record_NS::RECORD_OPERATION)
		{
			if (init_record() == -1)
			{
				dlog_debug("初始化录制失败");
				return;
			}
			m_ffmpegRecord.set_audioPts(nAptsMs);
			m_ffmpegRecord.set_videoPts(nVptsMs);
		}
	}
	/* 写入数据 */
	if (m_ffmpegRecord.is_init())
	{
		m_ffmpegRecord.write(stFfData);
	}
	return;
}

bool CRecordFile::is_record()
{
	bool bRet = false;
	switch (m_nRecordStatus.load())
	{
	case Record_NS::NO_OPERATION:
		bRet = false;
		break;
	case Record_NS::RECORD_OPERATION:
		bRet = true;
		break;
	case Record_NS::PAUSE_OPERATION:
		clear_mediaDataQueue();
		bRet = false;
		break;
	case Record_NS::STOP_OPERATION:
		bRet = false;
		// dlog_debug("通道%d停止录制", m_nChnId);
		if (m_ffmpegRecord.is_init())
		{
			m_ffmpegRecord.deinit();
			m_m3u8.add_ts(m_ffmpegRecord.get_mediaInfo());
			m_ffmpegRecord.reset();
			m_bFirstInit.store(false);

			clear_mediaDataQueue();
		}
		break;
	default:
		bRet = false;
		break;
	}
	if (m_bDisconnect.load())
	{
		bRet = false;
		m_ffmpegRecord.deinit();
		m_m3u8.add_ts(m_ffmpegRecord.get_mediaInfo());
		m_ffmpegRecord.reset();
		m_bFirstInit.store(false);

		clear_mediaDataQueue();
	}
	return bRet;
}

/*录制线程*/
void CRecordFile::thread_record()
{
	/*等待一会开始*/
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	while (m_bRunning.load())
	{
		if (!is_record())
		{
			/*延时500毫秒*/
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			continue;
		}
		write_record();
	}
}
