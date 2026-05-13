/**
 * @FilePath     : audio_detect.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-07-29 20:10:37
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-12-29 17:02:02
 * @Description  : 音频异常侦测
 */

#include "audio_detect.hpp"
#include "video_frame_jpeg_encoder.hpp"

/* 数据队列 */
#define QUEUE_MAX (2)

/* 静音阈值(dB) */
#define SILENCE_DB_THRESHOLD (10.0f)

CAudioDetect::CAudioDetect()
    : m_dateQueue(QUEUE_MAX)
{
    /* 启动线程 */
    m_bRunning.store(true);
    m_thread = std::thread(&CAudioDetect::run, this);
}

CAudioDetect::~CAudioDetect()
{
    /* 通知线程停止 */
    m_bRunning.store(false);
    // m_condition.notify_all();
    // note 调用 shutdown() 来唤醒可能阻塞在 pop() 的线程
    m_dateQueue.shutdown();
    if (m_thread.joinable())
    {
        m_thread.join();
    }
    m_dateQueue.clear();
    unInit();
}

void CAudioDetect::recvMediaData(MediaData_S stMediaData)
{
    if (!m_stAudioAnomalyCfg.bEnable)
    {
        return;
    }

    if (m_RecvManager.handleEvent(stMediaData.stMediaParam.nChannel))
    {
        if (m_dateQueue.size() >= QUEUE_MAX)
        {
            dlog_error("音频异常侦测-数据队列满了 [%d]", m_dateQueue.size());
        }
        m_dateQueue.pushOrReplace(stMediaData);
    }
}

void CAudioDetect::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
    m_stAudioAnomalyCfg.bEnable = stAlgoConfig.nEnAudioAnomaly;

    if (m_stAudioAnomalyCfg.bEnable)
    {
        Alarm::AudioAnomaly_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }
}

void CAudioDetect::setAlgoParamCfg(const Alarm::AudioAnomaly_S &stAlgoCfg)
{
    dlog_debug("ai_app: 设置音频异常侦测参数");
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stAudioAnomalyCfg = stAlgoCfg;
}

float CAudioDetect::getCurrentDb() const
{
    return m_fCurrentDB.load();
}

bool CAudioDetect::init()
{
    dlog_info("音频异常侦测初始化成功");
    /* 清空历史数据 */
    m_queueDBHistory.clear();
    m_fCurrentDB.store(0.0f);
    m_nSilenceFrameCount = 0;
    m_bInit = true;
    return true;
}

/* 反初始化 */
bool CAudioDetect::unInit()
{
    dlog_info("音频异常侦测反初始化");
    m_queueDBHistory.clear();
    m_bInit = false;
    return true;
}

bool CAudioDetect::reboot()
{
    if(!unInit())
    {
        return false;
    }
    if(!init())
    {
        return false;
    }

    return true;
}

void CAudioDetect::run()
{
    pthread_setname_np(pthread_self(), "AudioDetect");

    /* 媒体信息 */
    MediaData_S stMediaData;

    while (m_bRunning.load())
    {
        if(!m_bInit)
        {
            /* 没有算法使能，不进行算法初始化 */
            if (!m_stAudioAnomalyCfg.bEnable)
            {
                sleep(1);
                continue;
            }
            /* 初始化 */
            init();
        }

        /* 阻塞获取 */
        if (!m_dateQueue.pop(stMediaData, TIMEOUT_1000_MS) || stMediaData.pData == nullptr
            || stMediaData.pData.get() == nullptr)
        {
            continue;
        }

        /* 直接使用 stMediaData.pAudioFrame，避免内存拷贝 */
        char *pData = stMediaData.pData.get();
        if (!pData)
        {
            dlog_error("原始数据帧为空");
            continue;
        }

        /* 音频异常侦测 */
        if (m_stAudioAnomalyCfg.bEnable)
        {
            // double time = time_get_ms();
            /* 音频异常侦测后处理函数 */
            SEventProcessContext stCtx;
            stCtx.nChnId = stMediaData.stMediaParam.nChannel;
            stCtx.llTimestamp = TimeUtils_NS::get_currentTimestampMs();
            stCtx.pFrameInfo = nullptr; // 音频检测无视频帧
            processAudioAnomaly(pData, stMediaData.nSize, stCtx);
            // dlog_debug("音频异常侦测后处理函数耗时：%f", time_get_ms() - time);
        }
    }
}

// info /*----------------------- 算法后处理 -----------------------*/

void CAudioDetect::processAudioAnomaly(char *pData, int nLength, const SEventProcessContext &stCtx)
{
    if (!pData || nLength <= 0)
    {
        return;
    }

    /* 获取音频数据 */
    short *pSamples = (short *) pData;
    int nSampleCount = nLength / sizeof(short);

    /* 计算RMS */
    float fRMS = audio_processing_calculateRMS(pSamples, nSampleCount);

    /* 转换为分贝 */
    float fCurrentDB = audio_processing_convertRMSToDecibel(fRMS, m_nBitsPerSample);

    if (!access("testPrint", F_OK))
    {
        dlog_debug("当前RMS:[%f],音量:[%f]db", fRMS, fCurrentDB);
    }
    /* 更新当前音量 */
    m_fCurrentDB.store(fCurrentDB);

    /* 音频输入是否异常 */
    bool bAudioInputAnomaly = false;
    /* 声强是否陡升 */
    bool bAudioSuddenRise = false;
    /* 声强是否陡降 */
    bool bAudioSuddenDrop = false;
    /* 音频输入异常检测是否启用 */
    if (m_stAudioAnomalyCfg.bAudioInputAnomaly)
    {
        if (fCurrentDB < SILENCE_DB_THRESHOLD)
        {
            m_nSilenceFrameCount++;
            if (m_nSilenceFrameCount >= SILENCE_THRESHOLD_FRAMES)
            {
                bAudioInputAnomaly = true;
                dlog_warn("检测到音频输入异常：持续静音");
            }
        }
        else
        {
            /* 静音帧计数清零 */
            m_nSilenceFrameCount = 0;
        }
        /* 判断是否报警 */
        EventTriggerContext_S stInputContext;
        stInputContext.enEventType = Event::Type_E::AUDIO_ANOMALY;
        stInputContext.nChnId = stCtx.nChnId;
        stInputContext.llTimestamp = stCtx.llTimestamp;
        m_inputAlarmStateMachine.handleAlarmState(bAudioInputAnomaly, stInputContext);
    }

    /* 更新历史数据 */
    m_queueDBHistory.push_back(fCurrentDB);
    if (m_queueDBHistory.size() > static_cast<size_t>(m_nWindowSize))
    {
        m_queueDBHistory.pop_front();
    }

    /* 需要足够的历史数据才能进行检测 */
    if (m_queueDBHistory.size() < static_cast<size_t>(m_nWindowSize) / 5 * 4)
    {
        return;
    }

    /* 计算历史平均音量 */
    float fAvgDB = calculateAverageDB();

    /* 声强陡升检测 */
    if (m_stAudioAnomalyCfg.bUpEnable)
    {
        if (detectSuddenRise(fCurrentDB, fAvgDB))
        {
            bAudioSuddenRise = true;
            dlog_warn("检测到声强陡升: 当前%.1fdB, 平均%.1fdB, 差值%.1fdB", fCurrentDB, fAvgDB, fCurrentDB - fAvgDB);
        }
        /* 判断是否报警 */
        EventTriggerContext_S stRiseContext;
        stRiseContext.enEventType = Event::Type_E::AUDIO_SUDDEN_RISE;
        stRiseContext.nChnId = stCtx.nChnId;
        stRiseContext.llTimestamp = stCtx.llTimestamp;
        m_riseAlarmStateMachine.handleAlarmState(bAudioSuddenRise, stRiseContext);
    }

    /* 声强陡降检测 */
    if (m_stAudioAnomalyCfg.bDownEnable)
    {
        if (detectSuddenDrop(fCurrentDB, fAvgDB))
        {
            bAudioSuddenDrop = true;
            dlog_warn("检测到声强陡降: 当前%.1fdB, 平均%.1fdB, 差值%.1fdB", fCurrentDB, fAvgDB, fAvgDB - fCurrentDB);
        }
        /* 判断是否报警 */
        EventTriggerContext_S stDropContext;
        stDropContext.enEventType = Event::Type_E::AUDIO_SUDDEN_DROP;
        stDropContext.nChnId = stCtx.nChnId;
        stDropContext.llTimestamp = stCtx.llTimestamp;
        m_dropAlarmStateMachine.handleAlarmState(bAudioSuddenDrop, stDropContext);
    }
}

float CAudioDetect::calculateAverageDB()
{
    if (m_queueDBHistory.empty())
    {
        return 0.0f;
    }

    float fSum = 0.0f;
    for (const auto &db : m_queueDBHistory)
    {
        fSum += db;
    }

    return fSum / m_queueDBHistory.size();
}

bool CAudioDetect::detectSuddenRise(float fCurrentDB, float fAvgDB)
{
    /* 灵敏度系数: 1-100 映射到 1.0-0.1 */
    float fSensitivity = (101.0f - m_stAudioAnomalyCfg.nUpSensitivity) / 100.0f;

    /* 阈值计算: 基础阈值 = 配置阈值(0-100) * 灵敏度系数 */
    /* 配置阈值范围[1,100]映射为实际分贝阈值[1,35]dB */
    float fBaseThreshold = (m_stAudioAnomalyCfg.nUpThreshold / 100.0f) * 35.0f;
    float fThreshold = fBaseThreshold * fSensitivity;

    /* 最小阈值限制 默认15dB */
    if (fThreshold < 15.0f)
    {
        fThreshold = 15.0f;
    }

    /* 判断是否陡升 */
    float fDiff = fCurrentDB - fAvgDB;
    return (fDiff > fThreshold);
}

bool CAudioDetect::detectSuddenDrop(float fCurrentDB, float fAvgDB)
{
    /* 灵敏度系数: 1-100 映射到 1.0-0.1 */
    float fSensitivity = (101.0f - m_stAudioAnomalyCfg.nDownSensitivity) / 100.0f;

    /* 陡降阈值 默认35dB */
    float fThreshold = 35.0f * fSensitivity;

    /* 最小阈值限制 默认15dB */
    if (fThreshold < 15.0f)
    {
        fThreshold = 15.0f;
    }

    /* 判断是否陡降 */
    float fDiff = fAvgDB - fCurrentDB;
    return (fDiff > fThreshold);
}
