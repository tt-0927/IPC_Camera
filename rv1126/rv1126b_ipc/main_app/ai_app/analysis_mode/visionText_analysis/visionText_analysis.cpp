/*** 
 * @FilePath     : visionText_analysis.cpp
 * @Author       : cyc
 * @Date         : 2025-09-22 13:43:33
 * @LastEditors  : cyc
 * @LastEditTime : 2025-10-20 19:55:47
 * @Description  : 画面分析跟文字输入模型
 */

#include "visionText_analysis.hpp"
#include "event_vlm_manage.hpp"
#include "event_linkage.h"
#include "algorithm.hpp"
#include "share_data.h"
#include <fstream>
#include <chrono>

/*推理结果显示到osd*/
#define OSD_ENABLE 0

/* 数据队列大小 */ 
#define LLM_QUEUE_MAX (1)

/* 判断用户提问是否包含分析画面的意图 */
static bool containsImageIntent(const std::string& text) {
    if (text.empty()) return false;
    const std::vector<std::string> keywords = {
        "画面", "图片", "照片", "看看", "分析", "图", "场景"
    };
    for (const auto& kw : keywords) {
        if (text.find(kw) != std::string::npos) {
            return true;
        }
    }
    return false;
}

/*生成唯一任务ID*/
static std::string generateTaskId()
{
    /* 使用时间戳 + 随机数生成唯一ID */
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            
    /* 添加随机数确保唯一性 */
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);
    int random_num = dis(gen);
            
    return "TASK_" + std::to_string(timestamp) + "_" + std::to_string(random_num);
}

/*检查是否有效分析文本*/
static int checkString(const std::string& abc) {
    if (abc == "<image>\n") return -1;
    if (std::all_of(abc.begin(), abc.end(), [](unsigned char c){ return std::isspace(c); })) {
        return -1;
    }
    return 0;
}

/* 辅助函数，将配置中的频率枚举转换为毫秒 */
long long get_frequency_ms(Alarm::DetectionFrequency_E freq) {
    switch (freq) {
        case Alarm::DETECTION_FREQ_10S:  return 10000;
        case Alarm::DETECTION_FREQ_20S:  return 20000;
        case Alarm::DETECTION_FREQ_1MIN: return 60000;
        case Alarm::DETECTION_FREQ_5MIN: return 300000;
        default:                         return 30000; // 默认30秒
    }
}

/* 安全校验并修正 ROI 函数*/
static cv::Rect CheckAndFixRect(const Common::Rect_S &srcRect, int imgWidth, int imgHeight)
{
    cv::Rect outRect;
    outRect.x = srcRect.nX;
    outRect.y = srcRect.nY;
    outRect.width = srcRect.nWidth;
    outRect.height = srcRect.nHeight;

    // 左上角校验
    if (outRect.x < 0) { outRect.x = 0; }
    if (outRect.y < 0) { outRect.y = 0; }

    // 宽高越界校验
    if (outRect.x + outRect.width > imgWidth)
    {
        outRect.width = imgWidth - outRect.x;
    }
    if (outRect.y + outRect.height > imgHeight)
    {
        outRect.height = imgHeight - outRect.y;
    }

    // 最小尺寸保护
    if (outRect.width <= 0) outRect.width = 1;
    if (outRect.height <= 0) outRect.height = 1;

    return outRect;
}

CVisionText::CVisionText()
    : m_dataQueue(LLM_QUEUE_MAX),
    m_RecvManager(DETECT_FREENCY_TIME),
    m_lastTextPresetTime(std::chrono::steady_clock::time_point::min())
{
}

CVisionText::~CVisionText()
{
    stop();
}

std::string CVisionText::updateCreateTime()
{
    /* 获取当前时间 */
    std::time_t now = std::time(nullptr);
    std::tm* localTime = std::localtime(&now);
    
    /* 格式化时间字符串：年月日_时分秒 */
    std::ostringstream timeStream;
    timeStream << std::put_time(localTime, "%Y%m%d_%H%M%S");

    return timeStream.str();
}

void CVisionText::recvMediaData(MediaData_S stMediaData)
{

    if (!m_stLLmImageCfg.bEnable && !m_stTextPreseCfg.bEnable)
    {
        return;
    }
    
    m_nChannelId = stMediaData.stMediaParam.nChannel;

    if (m_RecvManager.handleEvent(stMediaData.stMediaParam.nChannel))
    {
        if (m_dataQueue.size() >= LLM_QUEUE_MAX)
        {
            // dlog_error("场景智能分析-数据队列满了 [%d]", m_dataQueue.size());
        }
        m_dataQueue.pushOrReplace(stMediaData);
    }
}

void CVisionText::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
    /* 场景智能分析总开关 */
    bool bAISceneAnalysis = stAlgoConfig.nEnAISceneAnalysis; 
     
    bool bImageAnalysisEnable = stAlgoConfig.nEnLLmInference;
    bool bTextPresetEnable = stAlgoConfig.nEnTextPreset;
    
    dlog_info("接收到算法配置更新 - 场景智能分析总开关: %s, 画面分析: %s, 文字预设: %s", 
              bAISceneAnalysis ? "启用" : "禁用",
              bImageAnalysisEnable ? "启用" : "禁用",
              bTextPresetEnable ? "启用" : "禁用");        

    if(bAISceneAnalysis == false)
    {
        dlog_info("关闭场景智能分析");
        stop();
        return;
    }

    /*初始化work*/
    if(start() != true)
    dlog_error("开启场景智能分析失败");
    
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        /*更新任务的启用状态*/
        m_stLLmImageCfg.bEnable = bImageAnalysisEnable;
        m_stTextPreseCfg.bEnable = bTextPresetEnable;
        /*如果文字预设任务和画面分析任务同时到来，画面分析优先级最高，禁用文字预设*/
        if(bImageAnalysisEnable && bTextPresetEnable)
        {
            m_stTextPreseCfg.bEnable = false;
        }
    }

     /* 检查是否需要初始化模型 */
     if (bAISceneAnalysis && !m_bAiInitialized.load())
     {
         dlog_info("检测到需要启用大模型，准备初始化");
         /* 通知工作线程初始化LLM */
         m_condition.notify_all();
     }

     /*加载场景智能分析总控制参数*/
    if (bAISceneAnalysis)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        Alarm::LLMAISceneAnalysis_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        m_stAISceneAnalysisCfg = stInfo;

        m_bAnalysisStop.store(m_stAISceneAnalysisCfg.bAnalysisStop);
        m_bNewDialogue.store(m_stAISceneAnalysisCfg.bNewDialogue);

        if(m_stAISceneAnalysisCfg.bNewDialogue || m_stAISceneAnalysisCfg.bAnalysisStop){
            m_stAISceneAnalysisCfg.bNewDialogue = false;
            m_stAISceneAnalysisCfg.bAnalysisStop = false;
            /*更新到配置文件*/
            CEventConfigure::instance()->set_configure(m_stAISceneAnalysisCfg);
        }
    }

    /*新对话，清除大模型推理上下文*/
    if(m_bNewDialogue.load())
    {
        if(m_newSessionType.load() != POSTPONED_SESSION){
            //此次新会话：对话前点击，或输出完成后点击
            m_newSessionType.store(THIS_SESSION);
        }
        if(m_pLlmHandle){
        /*清除模型输入缓存*/
        dlog_debug("新对话-清除模型输入缓存");
        m_pLlmHandle->clearModelInputCache(0);
        }
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
    
        /* 加载具体配置 */
        if (bImageAnalysisEnable)
        {
            Alarm::LLMImageAnalysis_S stInfo;
            CEventConfigure::instance()->get_configure(stInfo);
            m_stLLmImageCfg = stInfo;
            dlog_debug("ai_app: 设置画面分析参数");
        }

        /*使能画面截图，在定时分析关闭下，判断文字预设开，返回错误码*/
        if(m_stLLmImageCfg.bScreenshotEnable && m_stTextPreseCfg.bEnable && !m_stLLmImageCfg.bScheduleEnable)
        {
            /*返回错误给网页*/
            TaskPublish::instance()->message(AC_RETURN_IMAGE_ANALYSIS_RESULT, "-1");
            m_stLLmImageCfg.bScreenshotEnable = false; 
        }
    }

    if (bTextPresetEnable)
    {
        dlog_debug("ai_app: 文字预设算法已启用, 当前任务启用状态: %s", 
                m_stTextPreseCfg.bEnable ? "启用" : "禁用");
    }

}

void CVisionText::setAlgoParamCfg(const Alarm::LLMImageAnalysis_S &stAlgoCfg)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    dlog_debug("ai_app: 设置画面分析参数");
    m_stLLmImageCfg = stAlgoCfg;
}

void CVisionText::setAlgoParamCfg(const Alarm::TextPreset_S &stAlgoCfg)
{
    std::lock_guard<std::mutex> lock(m_mutex); 
    dlog_debug("ai_app: 设置文字预设任务参数, 任务名称: %s", stAlgoCfg.strTaskName.c_str());
    m_stTextPreseCfg = stAlgoCfg;
    
    if (m_stTextPreseCfg.bEnable)
    {
        long long task_freq_ms = get_frequency_ms(m_stTextPreseCfg.enDetectFrequency);
        m_textPresetIntervalMs.store(task_freq_ms);
        dlog_debug("文字预设任务检测频率已设置为: %lld ms", task_freq_ms);

        m_bIsCrop = false;
        if (m_stTextPreseCfg.stRect.IsValid())
        {
            /* 转换区域坐标分辨率至算法分辨率 */
            m_stTextPreseCfg.stRect.ConvertResolution(PIXEL_WIDTH_1920, PIXEL_HEIGHT_1080, PIXEL_WIDTH_AI, PIXEL_HEIGHT_AI);
            if (m_stTextPreseCfg.stRect.nWidth != 0 && m_stTextPreseCfg.stRect.nHeight != 0)
            {
                    m_bIsCrop = true;
            }
        }
    }
    else
    {
        dlog_info("任务'%s'禁用，恢复默认数据接收间隔", m_stTextPreseCfg.strTaskName.c_str());
        m_RecvManager.setTimeWindow(DETECT_FREENCY_TIME); 
    }
}


int CVisionText::onStreamOutput(const std::string& text, bool is_finished) 
{
    Alarm::AnalysisRecords_S stRecordInfo;
    stRecordInfo.strInputText = strAnalysisInputText;
    /* 更新输出结果的时间 */
    if(m_strOutputCreateTime.empty())
    {    
        m_strOutputCreateTime = updateCreateTime();
        stRecordInfo.strOutputCreateTime = CmProcess::instance().convertTimeFormat(m_strOutputCreateTime);
    }

    /*中断推理*/
    if(m_bAnalysisStop.load())
    {
        dlog_warn("中断推理!!!");
        m_bAnalysisStop.store(false);

        /*定时分析开启，更新配置*/
        std::lock_guard<std::mutex> lock(m_mutex);
        if(m_stLLmImageCfg.bScheduleEnable) {
            m_stLLmImageCfg.bEnable = true;
            CEventConfigure::instance()->set_configure(m_stLLmImageCfg);
        }
        m_stLLmImageCfg.bEnable = false;
        return 1;
    }

    if (!text.empty()) {

        stRecordInfo.strOutputText = text;
        stRecordInfo.strInputImagePath = m_currentStreamImagePath;
        /*未触发新对话才进行返回推理信息*/
        if(!m_bNewDialogue.load())
        {
            /*实时返回记录给网页*/
            TaskPublish::instance()->message(AC_RETURN_IMAGE_ANALYSIS_RESULT, Convert::to_string(stRecordInfo));
        }

        std::cout << text << std::flush;
    }
    if (is_finished) {

        stRecordInfo.strOutputText = text + "OVER!!!";
        /*未触发新对话才进行返回推理信息*/
        if(!m_bNewDialogue.load())
        {
            /*实时返回结束*/
            TaskPublish::instance()->message(AC_RETURN_IMAGE_ANALYSIS_RESULT, Convert::to_string(stRecordInfo));
        }
        std::cout << "\n[推理完成]" << std::endl;
    }

    return 0;
}

bool CVisionText::init()
{
    if(!init_Rknn())
    {
        m_bAiInitialized.store(false);
        return false;
    }

    if(!init_Llm())
    {
        unInit_Rknn();
        m_bAiInitialized.store(false);
        return false;
    }

    m_bAiInitialized.store(true);

    return true;
}

bool CVisionText::uninit()
{

    if(!unInit_Llm())
    {
        return false;
    }

    if(!unInit_Rknn())
    {
        return false;
    }

    m_bAiInitialized.store(false);

    return true;
    
}

bool CVisionText::init_Llm()
{
    std::string strRkllmConfigPath = AI_RKLLM_CONFIG_FILE;
    dlog_info("正在初始化LLM模型，配置文件: %s", strRkllmConfigPath.c_str()); 
    m_pLlmHandle = std::make_unique<Inference_NS::CLLMInferenceRK>(strRkllmConfigPath);
    
    if (!m_pLlmHandle->init()) 
    {
        // 流式输出回调
        m_callback = [this](const std::string& text, bool is_finished)-> int {
         return this->onStreamOutput(text, is_finished);
        };
        dlog_info("LLM模型初始化成功");
        /*清除模型输入缓存*/
        m_pLlmHandle->clearModelInputCache(0);
        return true;
    }
    else 
    {
        dlog_error("LLM模型初始化失败");
        m_pLlmHandle->unInit();
        m_pLlmHandle.reset();
        m_pLlmHandle = nullptr;
        return false;
    }

    return true;
}

bool CVisionText::unInit_Llm()
{
    if (m_pLlmHandle) 
    {
        m_pLlmHandle->unInit();
        m_pLlmHandle.reset();
        m_pLlmHandle = nullptr;
        dlog_debug("LLM模型已反初始化");
        return true;
    }
    return false;
}

bool CVisionText::init_Rknn()
{
    if (m_pRknnHandle)
    {
        dlog_warn("RKNN已存在，先清理");
        unInit_Rknn();
    }

    std::string strRknnConfigPath = AI_RKNN_DETECTION_CONFIG_FILE;
    dlog_info("正在初始化RKNN模型，配置文件: %s", strRknnConfigPath.c_str()); 
    
    m_pRknnHandle = std::make_unique<CImageEnoderInferenceRK>(strRknnConfigPath);
    
    if (m_pRknnHandle->init() != OK)
    {
        dlog_error("RKNN模型初始化失败");
        m_pRknnHandle->release();
        m_pRknnHandle.reset();
        m_pRknnHandle = nullptr;
        return false;
    }

    dlog_info("RKNN模型初始化成功");
    return true;
}

bool CVisionText::unInit_Rknn()
{
    if (m_pRknnHandle)
    {
        m_pRknnHandle->release();
        m_pRknnHandle.reset();
        m_pRknnHandle = nullptr;
        dlog_debug("RKNN模型已清理");
        return true;
    }
    return false;
}

bool CVisionText::stop()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_bRunning.load()) {
            return true;
        }
        m_bRunning.store(false);
    }

    /* 通知线程停止 */
    m_condition.notify_all();
    m_ioCondition.notify_all();

    if (m_thread.joinable()) {
        m_thread.join();
    }

    /* 清空 m_imgVec 并释放内存*/
    std::vector<float>().swap(m_imgVec); 
    
    if (m_ioThread.joinable()){
         m_ioThread.join();
    }

    return uninit();
 
}

bool CVisionText::start()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_bRunning.load()) {
        return true;  
    }

    /*分配 m_imgVec 内存并初始化为 0*/
    m_imgVec.resize(IMAGE_TOKEN_NUM * EMBED_SIZE, 0.0f);   

    m_dataQueue.~CBlockingQueue();
    new (&m_dataQueue) BQ_NS::CBlockingQueue<MediaData_S>(LLM_QUEUE_MAX);

    m_bRunning.store(true);
    m_thread = std::thread(&CVisionText::run, this);
    m_ioThread = std::thread(&CVisionText::ioWorker, this);

    return true;  
}

bool CVisionText::check_TextPresetTask()
{
    auto now = std::chrono::steady_clock::now();
    
    /* 如果是第一次执行，直接允许 */
    if (m_lastTextPresetTime == std::chrono::steady_clock::time_point::min())
    {
        m_lastTextPresetTime = now;
        return true;
    }
    
    /* 检查时间间隔 */
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastTextPresetTime);
    long long intervalMs = m_textPresetIntervalMs.load();
    
    if (elapsed.count() >= intervalMs)
    {
        m_lastTextPresetTime = now;
        return true;
    }
    
    return false;
}

void CVisionText::execute_ImageAnalysis_Task(bool isText, Alarm::LLMImageAnalysis_S cfg, const std::string& timeStr, 
                                             const std::string& imagePath, bool isUpload, float* pImgVec)
{
    /*是否开启定时分析，开启则使用预设提问*/
    if(cfg.bScheduleEnable) 
    {
        cfg.strAnalysisInputText = IMAGR_ANALYSIS_SCFEDULE_INPUTTEXT;
    }
    
    if(m_bAnalysisStop.load()) 
    {
        dlog_info("结束执行画面分析任务");
        Alarm::AnalysisRecords_S RecordInfo;
        RecordInfo.strCreateTime = CmProcess::instance().convertTimeFormat(timeStr);
        RecordInfo.strInputText = cfg.strAnalysisInputText;
        RecordInfo.strOutputCreateTime = RecordInfo.strCreateTime;
        RecordInfo.strOutputText = " ";
        if(!isText) 
        RecordInfo.strInputImagePath = imagePath; 

        /*点击新对话，不进行返回*/
        if(!m_bNewDialogue.load()) 
        {
            /*返回空信息给网页*/
            TaskPublish::instance()->message(AC_RETURN_IMAGE_ANALYSIS_RESULT, Convert::to_string(RecordInfo));
        } 
        else 
        {
            m_bNewDialogue.store(false);
        }

        m_bAnalysisStop.store(false);
        /*定时分析开启，更新配置*/
        if(cfg.bScheduleEnable) 
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_stLLmImageCfg.bEnable = true; 
            CEventConfigure::instance()->set_configure(m_stLLmImageCfg);
        }
        return;
    }

    /*开启定时分析或者开启新会话,其他推理任务关闭的下一次对话时，清除模型输入缓存*/
    if(cfg.bScheduleEnable || m_newSessionType.load() == THIS_SESSION || m_newSessionType.load() == POSTPONED_SESSION) 
    {
        dlog_info("清除模型输入缓存\n");
        if(m_pLlmHandle) 
        {
            /*清除模型输入缓存*/
            m_pLlmHandle->clearModelInputCache(0);
        }
    }

    dlog_info("开始执行画面分析任务");
    std::string strQueryText = build_image_analysis_prompt(isText, cfg.strAnalysisInputText);
    if (strQueryText.empty() || checkString(strQueryText)) 
    {
        dlog_error("构建画面分析 prompt 失败");
        return;
    }

    dlog_debug("画面分析 Prompt: %s", strQueryText.c_str());

    /*定时分析开启，返回输入文本和图片信息到网页*/
    if(cfg.bScheduleEnable) 
    {
        Alarm::AnalysisRecords_S stRecordInfo;
        /*定时分析固定提问,返回给网页*/
        stRecordInfo.strCreateTime = CmProcess::instance().convertTimeFormat(timeStr);
        stRecordInfo.strInputText = IMAGR_ANALYSIS_SCFEDULE_INPUTTEXT;
        stRecordInfo.strInputImagePath = imagePath; 
        /*定时分析，返回信息给网页*/
        TaskPublish::instance()->message(AC_RETURN_IMAGE_ANALYSIS_RESULT, Convert::to_string(stRecordInfo));
    }
    strAnalysisInputText = cfg.strAnalysisInputText;
    /* LLM推理 */ 
    std::string strResult;
    if(m_pLlmHandle->run(isText, strQueryText, pImgVec, strResult, m_callback) != OK) 
    {
        return;
    }

    dlog_info("画面分析 LLM 推理结果: %s", strResult.c_str());
    /* 处理结果 */ 
    /* 清理一下结果字符串，去除前后空格和换行符 */ 
    strResult.erase(0, strResult.find_first_not_of(" \t\n\r"));
    strResult.erase(strResult.find_last_not_of(" \t\n\r") + 1);

    #if OSD_ENABLE
    /*结果发送到osd*/
    std::string reason = "【模型分析】：" + strResult;
    COsdManage::instance()->send_subtitle_info(reason, 20000); // 显示20秒
    #endif

    /*识别到预设任务或者定时分析关闭后的第一次实时分析，加入新对话*/
    if(!cfg.bScheduleEnable && m_newSessionType.load() == POSTPONED_SESSION) 
    {
        m_newSessionType.store(THIS_SESSION);
    } 
    else if(m_bNewDialogue.load()) 
    {
        //下次新会话：推理/输出过程中点击
        m_newSessionType.store(NEXT_SESSION);
    }

    /*画面分析任务处理*/
    IoTask t; 
    t.type = IoTask::IMAGE_ANALYSIS;
    t.timeKey = timeStr;
    t.outputTimeKey = m_strOutputCreateTime;
    t.prompt = strQueryText; 
    t.result = strResult;
    t.imagePath = imagePath; 
    t.isText = isText;
    t.isUploadImage = isUpload; 
    t.bScheduleEnable = cfg.bScheduleEnable;
    t.imagePath = imagePath; 
    { 
        std::lock_guard<std::mutex> lk(m_ioMutex); 
        m_ioQueue.push(t); 
        m_ioCondition.notify_one(); 
    }

    /*清除输出结果的时间*/
    m_strOutputCreateTime.clear();
}

void CVisionText::execute_TextPreset_Task(Alarm::TextPreset_S cfg, const std::string& timeStr, float* pImgVec , cv::Mat& Frame)
{
    /* 检查是否可以执行（基于时间间隔控制） */
    if (!check_TextPresetTask()) 
    {
        dlog_debug("文字预设任务 '%s' 未到执行时间，跳过", cfg.strTaskName.c_str());
        return;
    }
    
    if(m_pLlmHandle) 
    {
        /*清除模型输入缓存*/
        dlog_debug("清除模型输入缓存");
        m_pLlmHandle->clearModelInputCache(0);
    }

    dlog_info("开始执行文字预设任务: %s", cfg.strTaskName.c_str());     
    /* 构建Prompt */ 
    std::string strQueryText_tmp = build_text_preset_prompt(cfg);
    if (strQueryText_tmp.empty()) 
    {
        dlog_error("为任务 '%s' 构建 prompt 失败。", cfg.strTaskName.c_str());
        return;
    }
    /*文字预设返回提问，图片数据任务*/
    IoTask t1; 
    t1.type = IoTask::TEXT_PRESET_RETURN_QUESTION;
    t1.timeKey = timeStr;
    t1.prompt = strQueryText_tmp; 

    std::vector<uchar> buf;
    std::vector<int> params = { cv::IMWRITE_JPEG_QUALITY, 75 };          
    if (!Frame.empty()) 
    {
        cv::imencode(".jpg", Frame, buf, params);
        t1.jpegData = std::move(buf); 
    }

    { 
        std::lock_guard<std::mutex> lk(m_ioMutex); 
        m_ioQueue.push(t1); 
        m_ioCondition.notify_one(); 
    }

    m_currentStreamImagePath = "";
    strAnalysisInputText = strQueryText_tmp ;
    std::string strQueryText = "<image>" + strQueryText_tmp;
    dlog_debug("任务 '%s' 的 Prompt: %s", cfg.strTaskName.c_str(), strQueryText.c_str());
    /* LLM推理 */ 
    std::string strResult;
    if(m_pLlmHandle->run(false, strQueryText, pImgVec, strResult, m_callback) != OK)
    {
        dlog_error("文字预设任务 '%s' LLM推理失败。", cfg.strTaskName.c_str());
        return;
    } 

    dlog_info("任务 '%s' LLM 推理结果: %s", cfg.strTaskName.c_str(), strResult.c_str());
    bool bContainsAffirmative = false;
    std::regex affirmative_regex(R"((\bYes\b|是|有))", std::regex::icase);
    std::regex negative_regex(R"((\bNo\b|不|没|否))", std::regex::icase);
    if (std::regex_search(strResult, negative_regex)) 
    {
        bContainsAffirmative = false;
    } 
    else if (std::regex_search(strResult, affirmative_regex)) 
    {
        bContainsAffirmative = true;
    }

    /*文字预设结果更新任务*/
    IoTask t2; t2.type = IoTask::TEXT_PRESET_RESULT_UPDATE;
    t2.timeKey = timeStr;
    t2.outputTimeKey = m_strOutputCreateTime;
    t2.prompt = strQueryText_tmp; 
    t2.result = strResult; 
    t2.bSkipDelete = bContainsAffirmative; 
    t2.bDetailedAnalysis = false;
    { 
        std::lock_guard<std::mutex> lk(m_ioMutex); 
        m_ioQueue.push(t2); 
        m_ioCondition.notify_one(); 
    }

    /*清除输出结果的时间*/
    m_strOutputCreateTime.clear();

    /*符合条件则详细分析*/
    if (bContainsAffirmative) 
    {
        /*推理时符合条件期间点击新对话，会清除大模型上下文，不进行详细分析*/
        if(m_bNewDialogue.load())
        {
            m_bNewDialogue.store(false);
            return;
        }

#ifdef ENABLE_TVSDK_SRC
        {
            EventTriggerContext_S stContext;
            stContext.enEventType = Event::Type_E::TEXT_PRESET;
            stContext.nChnId = m_nChannelId;
            stContext.llTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            if (!Frame.empty()) {
                auto pPayload = std::make_shared<EventTvSdkPayload_S>();
                pPayload->enType = get_tvsdk_payload_type(stContext.enEventType);
                if (encode_mat_to_tvsdk_image(Frame, pPayload->stPanoramaImage, 85, false)) {
                    stContext.pTvSdkPayload = pPayload;
                }
            }
            CEventLinkage::instance()->handleEvent(stContext);
        }
#else
        CEventLinkage::instance()->handleEvent(Event::Type_E::TEXT_PRESET);
#endif
        dlog_warn("文字预设任务告警: [%s] 检测到符合条件!", cfg.strTaskName.c_str());
        std::string currentTimeStr = updateCreateTime();
        /* 进一步分析图片 - 生成详细描述 */
        strResult = perform_DetailedImage_Analysis(cfg, currentTimeStr, pImgVec);
        /*预警推送记录任务*/
        IoTask t3; 
        t3.type = IoTask::ALERT_PUSH_RECORD_UPDATE;
        t3.result = strResult; 
        t3.timeKey = timeStr;
        { 
            std::lock_guard<std::mutex> lk(m_ioMutex); 
            m_ioQueue.push(t3); 
            m_ioCondition.notify_one(); 
        }
    }
}

std::string CVisionText::perform_DetailedImage_Analysis(const Alarm::TextPreset_S& cfg, const std::string& timeStr, float* pImgVec) 
{
    dlog_info("开始进行详细图片分析");
    /* 构建详细分析的Prompt */
    std::string strDetailPrompt_tmp = "分析一下这张图片中所包含的物体和状态，进行结构化分析";
    strAnalysisInputText = strDetailPrompt_tmp;

    Alarm::AnalysisRecords_S stRecordInfo; 
    stRecordInfo.strCreateTime = CmProcess::instance().convertTimeFormat(timeStr);
    stRecordInfo.strInputText = strDetailPrompt_tmp;
    /*返回信息给网页*/
    TaskPublish::instance()->message(AC_RETURN_IMAGE_ANALYSIS_RESULT, Convert::to_string(stRecordInfo));

    std::string strDetailPrompt = "<image>" + strDetailPrompt_tmp + ",要求使用中文回答,控制在200个字以内";
    std::string strDetailedResult;
    if (m_pLlmHandle->run(false, strDetailPrompt, pImgVec, strDetailedResult, m_callback) != OK) 
    {
        dlog_error("详细图片分析推理失败");
        return "图片分析失败";
    }
    /* 清理结果字符串 */
    strDetailedResult.erase(0, strDetailedResult.find_first_not_of(" \t\n\r"));
    strDetailedResult.erase(strDetailedResult.find_last_not_of(" \t\n\r") + 1);
    dlog_info("详细图片分析结果: %s", strDetailedResult.c_str());

    #if OSD_ENABLE
    /*结果发送到osd*/
    std::string reason = "【模型分析】：" + strDetailedResult;
    COsdManage::instance()->send_subtitle_info(reason, 20000); // 显示20秒
    #endif

    /*文字预设详细分析结果更新*/
    IoTask t; t.type = IoTask::TEXT_PRESET_DETAIL_UPDATE; 
    t.timeKey = timeStr;
    t.outputTimeKey = m_strOutputCreateTime;
    t.prompt = strDetailPrompt_tmp; 
    t.result = strDetailedResult; 
    t.bDetailedAnalysis = true;
    { 
        std::lock_guard<std::mutex> lk(m_ioMutex); 
        m_ioQueue.push(t); 
        m_ioCondition.notify_one(); 
    }

    /*清除输出结果的时间*/
    m_strOutputCreateTime.clear();

    return strDetailedResult;
}

int CVisionText::UpdateImageAnalysisRecord(Alarm::AnalysisRecords_S& RecordInfo, bool bNewSession)
{
    std::lock_guard<std::mutex> lk(CEventConfigure::instance()->imageAnalysisRecordMutex());
    
    if (RecordInfo.strInputText.empty() || RecordInfo.strOutputText.empty()) 
    {
        dlog_error("结果参数为空");
        /*清除新对话标识*/
        if(m_bNewDialogue.load())
        {
            m_bNewDialogue.store(false);
        }
        return -1;
    }

    std::stringstream ssid;
    Alarm::AnalysisRecords_S stRecordInfo;
    Alarm::AnalysisAllRecordIndexItem_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);

    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    ssid << "r" << millis;
    /*去掉字符串开头的"<image>"和换行符"\n*/
    size_t pos = RecordInfo.strInputText.find("<image>\n");
    if (pos != std::string::npos) 
    {
        RecordInfo.strInputText.erase(pos, 8);
    }

    stRecordInfo.strId = ssid.str();
    stRecordInfo.strCreateTime = RecordInfo.strCreateTime;
    stRecordInfo.strOutputCreateTime = RecordInfo.strOutputCreateTime;
    stRecordInfo.strInputText = RecordInfo.strInputText;
    stRecordInfo.strOutputText = RecordInfo.strOutputText;
    stRecordInfo.strInputImagePath = RecordInfo.strInputImagePath;
    stRecordInfo.strVideoPath = RecordInfo.strVideoPath;
    stRecordInfo.bSkipDelete = RecordInfo.bSkipDelete;

    if(!m_bNewDialogue.load())
    {
        TaskPublish::instance()->message(AC_RETURN_IMAGE_ANALYSIS_RESULT, Convert::to_string(stRecordInfo));
    } 
    else 
    {
        m_bNewDialogue.store(false);
    }

    // 如果是新对话模式，或者当前没有任何记录，则创建一个新会话
    if (bNewSession || stInfo.Allrecords.empty()) 
    {
        Alarm::AnalysisRecordIndexItem_S newSession;
        // 使用时间戳作为 Key，保证唯一性
        newSession.indexKey = "session_" + std::to_string(millis);
        // 新会话总是插入到最前面 (Index 0)
        stInfo.Allrecords.insert(stInfo.Allrecords.begin(), newSession);

        if ((int)stInfo.Allrecords.size() > MAX_SESSION_SIZE) 
        {
            Alarm::AnalysisRecordIndexItem_S& oldSession = stInfo.Allrecords.back();
            for (auto &oldRec : oldSession.records) 
            {
                if(!oldRec.bSkipDelete)
                {
                    CmProcess::instance().removePhysicalFiles(oldRec.strInputImagePath, oldRec.strVideoPath);
                }
            }
            stInfo.Allrecords.pop_back();
        }
        stInfo.current_session_index = 0;
    }
    auto& currentSession = stInfo.Allrecords[0];
    currentSession.records.push_back(stRecordInfo);

    if ((int)currentSession.records.size() > MAX_RECORD_INFO_SIZE) 
    {
        const auto& oldestRecord = currentSession.records.front();
        if(!oldestRecord.bSkipDelete)
        {
            CmProcess::instance().removePhysicalFiles(oldestRecord.strInputImagePath, oldestRecord.strVideoPath);
        }
        currentSession.records.erase(currentSession.records.begin());
    }
    stInfo.total_sessions = static_cast<int>(stInfo.Allrecords.size());
    stInfo.current_session_index = 0;

    return CEventConfigure::instance()->set_configure(stInfo);
}

void CVisionText::save_realAlarmPush_record(const Alarm::TextPreset_S& taskConfig, const std::string& strResult, const std::string& timeStr, const std::string& imagePath) 
{
    /* 创建实时预警推送记录 */
    Alarm::RealAlarmPushRecord_S stRecord;
    /* 基本信息 */
    stRecord.strTaskId = generateTaskId();
    stRecord.strTaskName = taskConfig.strTaskName;
    stRecord.strObjectName = taskConfig.strObjectName;
    stRecord.strConditionName = taskConfig.strConditionName;
    stRecord.enDealStatus = Alarm::PUSH_DEAL_STATUS_NONE; 
    stRecord.strDescription = strResult;
    stRecord.strImagePath = imagePath;
    stRecord.strVideoPath = taskConfig.strVideoPath;

    /*时间格式如：20231027_143005*/
    try {
        if (timeStr.length() >= 15) 
        {
            stRecord.stAlarmTime.stDate.nYear   = std::stoi(timeStr.substr(0, 4));
            stRecord.stAlarmTime.stDate.nMonth  = std::stoi(timeStr.substr(4, 2));
            stRecord.stAlarmTime.stDate.nDay    = std::stoi(timeStr.substr(6, 2));
            stRecord.stAlarmTime.stTime.nHour   = std::stoi(timeStr.substr(9, 2));
            stRecord.stAlarmTime.stTime.nMinute = std::stoi(timeStr.substr(11, 2));
            stRecord.stAlarmTime.stTime.nSecond = std::stoi(timeStr.substr(13, 2));
        }
    } catch (const std::exception& e) {
        dlog_warn("无效时间格式: %s\n", e.what());
    }
    
    /* 调用事件VLM管理器保存记录 */
    CEventVlmManager::instance()->addRealAlarmPushRecord(stRecord);
}


/*核心工作线程--------------------------------------------------------------------------------------------------------------------*/
void CVisionText::run()
{
    pthread_setname_np(pthread_self(), "CVisionText");
    bool rga_ok = false;
    MediaData_S stMediaData;
    cv::Mat croppedMat;
    cv::Mat Desframe(PIXEL_HEIGHT_AI, PIXEL_WIDTH_AI, CV_8UC3);
    /*rknn模型使用*/
    cv::Mat ResizedFrame(MODEL_INPUT_HEIGHT, MODEL_INPUT_WIDTH, CV_8UC3);

    while (m_bRunning.load()) 
    {
        bool bNeed = false;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            bNeed = m_stAISceneAnalysisCfg.bEnable;
        }

        if (bNeed && !m_bAiInitialized.load()) 
        {
            // 设置当前线程为标准优先级
            setpriority(PRIO_PROCESS, 0, 0); 
            dlog_info("正在初始化rkllm rknn模型");
            if (init()) 
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                /*初始化后避免，未清理的旧配置触发*/
                m_stLLmImageCfg.bEnable = false;
                /*模型加载完成，清理临时内存碎片*/
                malloc_trim(0);
            } 
            else 
            {
                dlog_error("rkllm rknn模型初始化失败，将在1秒后重试");
                uninit();
                std::unique_lock<std::mutex> ulock(m_mutex);
                m_condition.wait_for(ulock, std::chrono::seconds(1), [this] { return !m_bRunning.load(); });
                continue;
            }
        }
        /* 如果没有启用的算法或LLM未初始化，等待 */
        if (!bNeed || !m_bAiInitialized.load()) 
        {
            sleep(1); 
            continue;
        } 
        /* 阻塞获取数据 */
        if (!m_dataQueue.pop(stMediaData, TIMEOUT_5_MS) || stMediaData.nSize == 0)
        {
            continue;
        }

        Alarm::LLMImageAnalysis_S local_img_cfg;
        Alarm::TextPreset_S local_text_cfg;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            local_img_cfg = m_stLLmImageCfg;
            local_text_cfg = m_stTextPreseCfg;
        }

        /* 如果收到中断消息，跳过 */
        if (m_bAnalysisStop.load()) 
        {
           m_bAnalysisStop.store(false);
           std::lock_guard<std::mutex> lock(m_mutex);
           m_stLLmImageCfg.bEnable = false;
           continue;
        } 

        bool isText = false;
        bool isUpload = false;
        std::string currentTimeStr = updateCreateTime();
        std::string currentImagePath = ""; 

        // 实时分析判断用户是否有隐式“画面分析”的意图
        bool bImplicitImageIntent = false;
        if (local_img_cfg.bEnable && !local_img_cfg.strAnalysisInputText.empty() && !local_img_cfg.bScheduleEnable) 
        {
            if(local_img_cfg.strAnalysisInputImagePath.empty())
            {
                /*非上传图片实时对话才进行判断*/
                bImplicitImageIntent = containsImageIntent(local_img_cfg.strAnalysisInputText);
            }
        }

        /*纯文本提问*/
        if (local_img_cfg.bEnable && !local_text_cfg.bEnable &&
            !local_img_cfg.bScreenshotEnable && 
            local_img_cfg.strAnalysisInputImagePath.empty() && 
            !local_img_cfg.bScheduleEnable &&
            !bImplicitImageIntent) 
        {
            isText = true;
            m_currentStreamImagePath.clear();
        } 
        else 
        {
            // YUV 转 BGR，抓图
            if ((local_img_cfg.bEnable && local_img_cfg.strAnalysisInputImagePath.empty())  || local_text_cfg.bEnable) 
            {
                //RK_FORMAT_YCbCr_420_SP = NV12
                rga_ok = rga_image_transform(stMediaData.pData.get(), stMediaData.stMediaParam.nVideoWidth, stMediaData.stMediaParam.nVideoHeight,
                                                                RK_FORMAT_YCbCr_420_SP,Desframe.data, PIXEL_WIDTH_AI, PIXEL_HEIGHT_AI, RK_FORMAT_BGR_888);
                if(!rga_ok) 
                {
                    dlog_error("RGA-YUV 转 BGR error");
                    continue;
                }
            }

            isText = false;
            // 手动截图与定时分析的抓图逻辑
            if (local_img_cfg.bEnable && local_img_cfg.strAnalysisInputImagePath.empty()) 
            {
                /*事件时间管理调度赋值 定时分析strAnalysisInputText为"[*]预设提问"，此时才进行推理执行*/
                if(local_img_cfg.bScheduleEnable && local_img_cfg.strAnalysisInputText.empty()) 
                {
                    continue;
                }

                // 手动纯截图：交由IO线程生成返回路径，然后退出等待用户提问
                IoTask t; 
                t.type = IoTask::SCREENSHOT_ONLY;
                t.timeKey = currentTimeStr;
                t.prompt = local_img_cfg.strAnalysisInputText;
                t.bScheduleEnable = local_img_cfg.bScheduleEnable; 
                t.bImplicitImageIntent = bImplicitImageIntent;

                //将 BGR 转成 JPEG 内存块
                std::vector<uchar> buf;
                std::vector<int> params = { cv::IMWRITE_JPEG_QUALITY, 75 };

                if (rga_ok) 
                {
                    cv::imencode(".jpg", Desframe, buf, params);
                    t.jpegData = std::move(buf); 
                }

                { 
                    std::lock_guard<std::mutex> lk(m_ioMutex); 
                    m_ioQueue.push(t);
                    m_ioCondition.notify_one(); 
                }

                if(!bImplicitImageIntent && !local_img_cfg.bScheduleEnable && local_img_cfg.bScreenshotEnable )
                {
                    /*实时分析，先返回截图文件路径给网页，后待网页提问后一起传入，使用本地图片*/
                    std::lock_guard<std::mutex> lock(m_mutex);
                    m_stLLmImageCfg.bEnable = false; 
                    m_stLLmImageCfg.bScreenshotEnable = false;
                    continue; 
                }
            }
            /*画面分析开启，传入图片文件路径时*/
            else if (local_img_cfg.bEnable && !local_img_cfg.bScheduleEnable && !local_img_cfg.strAnalysisInputImagePath.empty()) 
            {
                currentImagePath = local_img_cfg.strAnalysisInputImagePath;
                /*文件如果在UPLOAD_PATH路径下，则判断为上传的图片文件*/
                isUpload = isFileInDirectory(currentImagePath, UPLOAD_PATH);

                dlog_info("使用本地图片:%s\n",currentImagePath.c_str());
                if(CmProcess::instance().loadEncodeImage(currentImagePath, ResizedFrame)) 
                {
                    dlog_error("loadEncodeImage error");
                    /*返回错误信息给网页*/
                    onStreamOutput("图片加载错误!", false); 
                    std::lock_guard<std::mutex> lock(m_mutex); 
                    m_stLLmImageCfg.bEnable = false;
                    continue;
                }
                /*移动图片任务*/
                IoTask t; 
                t.type = IoTask::MOVE_IMAGE; 
                t.imagePath = currentImagePath;
                { 
                    
                    std::lock_guard<std::mutex> lk(m_ioMutex); 
                    m_ioQueue.push(t); 
                    m_ioCondition.notify_one(); 
                }
            }

            if (local_img_cfg.bEnable || local_text_cfg.bEnable) 
            {
                if (!m_bIsCrop && local_img_cfg.strAnalysisInputImagePath.empty()) 
                {
                    // 确保目标 Mat 内存已分配（MODEL_INPUT_WIDTH x MODEL_INPUT_HEIGHT）
                    if (ResizedFrame.cols != MODEL_INPUT_WIDTH || ResizedFrame.rows != MODEL_INPUT_HEIGHT || ResizedFrame.type() != CV_8UC3) 
                    {
                        ResizedFrame.create(MODEL_INPUT_HEIGHT, MODEL_INPUT_WIDTH, CV_8UC3);
                    }

                    // 利用 RGA  YUV 转 RGB，供模型推理
                    bool ai_rga_ok = rga_image_transform(
                        stMediaData.pData.get(), 
                        stMediaData.stMediaParam.nVideoWidth, 
                        stMediaData.stMediaParam.nVideoHeight,
                        RK_FORMAT_YCbCr_420_SP, //NV12
                        ResizedFrame.data, 
                        MODEL_INPUT_WIDTH, MODEL_INPUT_HEIGHT, 
                        RK_FORMAT_RGB_888  
                    );
                    //  RGA 失败，用 CPU 处理
                    if (!ai_rga_ok) 
                    {
                        dlog_error("RGA 转换失败,使用CPU 处理!");
                        
                        if (!Desframe.empty()) 
                        {
                            try {
                                // CPU 缩放
                                cv::resize(Desframe, ResizedFrame, 
                                        cv::Size(MODEL_INPUT_WIDTH, MODEL_INPUT_HEIGHT), 0, 0, cv::INTER_LINEAR);
                                // CPU 换色
                                cv::cvtColor(ResizedFrame, ResizedFrame, cv::COLOR_BGR2RGB);
                            } 
                            catch (const cv::Exception& e) {
                                dlog_error("CPU Full-Frame Fallback failed: %s", e.what());
                            }
                        }
                        else {
                            dlog_error("Desframe is empty, cannot fallback!");
                        }
                    }
                }
                /*文字预设设置了分析区域,进行裁剪*/
                else if(local_text_cfg.bEnable && m_bIsCrop)
                {
                        
                    int sw = stMediaData.stMediaParam.nVideoWidth;
                    int sh = stMediaData.stMediaParam.nVideoHeight;
                    //边界安全校验（确保矩形在图像内）
                    cv::Rect safeCrop = CheckAndFixRect(local_text_cfg.stRect, sw, sh);
                    //源端：cx, cy, cw, ch 必须是偶数 (2对齐)
                    int cx = (safeCrop.x >> 1) << 1;
                    int cy = (safeCrop.y >> 1) << 1;
                    int cw = (safeCrop.width >> 1) << 1; 
                    int ch = (safeCrop.height >> 1) << 1;

                    // dw 必须是 4 的倍数 (4对齐) ，高度偶数
                    int dw = MODEL_INPUT_WIDTH;
                    int dh = MODEL_INPUT_HEIGHT;

                    // 如果尚未分配或尺寸不对,预分配目标 Mat 的内存
                    if (ResizedFrame.cols != dw || ResizedFrame.rows != dh || ResizedFrame.type() != CV_8UC3) 
                    {
                        ResizedFrame.create(dh, dw, CV_8UC3);
                    }

                    // 利用 RGA  YUV 转 RGB，裁剪,供模型推理
                    bool ai_rga_ok = rga_image_transform(
                        stMediaData.pData.get(), sw, sh, RK_FORMAT_YCbCr_420_SP, 
                        ResizedFrame.data, dw, dh, RK_FORMAT_RGB_888,
                                cx, cy, cw, ch,   // 裁剪坐标
                       0                // 旋转角度
                    );
        
                    //  RGA 失败，用 CPU 处理
                    if (!ai_rga_ok) 
                    {
                        dlog_error("RGA 转换失败,使用CPU 处理!");

                        // 情况 1: 如果第一路 RGA 成功生成了全帧 Desframe (AI 通道分辨率 BGR)，基于它进行处理
                        if (rga_ok && !Desframe.empty()) 
                        {
                            try {
                                // 裁剪 ROI
                                cv::Mat croppedMat = Desframe(safeCrop); 
                                // 缩放至模型尺寸
                                cv::resize(croppedMat, ResizedFrame, 
                                        cv::Size(MODEL_INPUT_WIDTH, MODEL_INPUT_HEIGHT), 0, 0, cv::INTER_LINEAR);
                                // 颜色转换 BGR -> RGB
                                cv::cvtColor(ResizedFrame, ResizedFrame, cv::COLOR_BGR2RGB);
                                
                                dlog_info("CPU Fallback A (from Desframe) successful.");
                            } 
                            catch (const cv::Exception& e) {
                                dlog_error("CPU Fallback A failed: %s", e.what());
                            }
                        } 
                        // 情况 2: 第一路也失败了，从最原始的 NV12 数据重新转换
                        else 
                        {
                            try {
                                // 包装原始 NV12 数据
                                cv::Mat nv12_mat(stMediaData.stMediaParam.nVideoHeight * 3 / 2, 
                                                stMediaData.stMediaParam.nVideoWidth, CV_8UC1, 
                                                stMediaData.pData.get());
                                
                                // 整体转 BGR
                                cv::Mat full_bgr;
                                cv::cvtColor(nv12_mat, full_bgr, cv::COLOR_YUV2BGR_NV12);
                                // 裁剪 + 缩放
                                cv::Mat cropped = full_bgr(safeCrop);
                                cv::resize(cropped, ResizedFrame, 
                                        cv::Size(MODEL_INPUT_WIDTH, MODEL_INPUT_HEIGHT), 0, 0, cv::INTER_LINEAR);
                                
                                // 转 RGB
                                cv::cvtColor(ResizedFrame, ResizedFrame, cv::COLOR_BGR2RGB);
                                
                                dlog_info("CPU Fallback B (from raw NV12) successful.");
                            } 
                            catch (const cv::Exception& e) {
                                dlog_error("Total failure: All processing paths failed! %s", e.what());
                            }
                        }
                    }
                }
                
                /* 画面编码，获取图像向量 */
                if(m_pRknnHandle->run(ResizedFrame.data, m_imgVec.data()) != OK) 
                {
                    dlog_error("RKNN模型推理失败");
                    continue;
                }
            }
        }
   
        if (local_img_cfg.bEnable || m_bAnalysisStop.load()) 
        {
            {
                std::lock_guard<std::mutex> lock(m_mutex); 
                m_stLLmImageCfg.bEnable = false; 
            }

            currentImagePath = m_currentStreamImagePath;
            execute_ImageAnalysis_Task(isText, local_img_cfg, currentTimeStr, currentImagePath, isUpload,  m_imgVec.data());
        }
        else if (local_text_cfg.bEnable) 
        {
            execute_TextPreset_Task(local_text_cfg, currentTimeStr, m_imgVec.data(),Desframe);
        }
    }
}


/*异步 IO 处理线程--------------------------------------------------------------------------------------------------------------------*/
void CVisionText::ioWorker() 
{
    pthread_setname_np(pthread_self(), "AI_AsyncIO");
    dlog_info("异步IO处理线程已启动");
    //优先级3
    setpriority(PRIO_PROCESS, 0, 3); 

    std::string lastTextPresetImage = "";  
    std::string lastTextPresetVideo = "";  

    while (m_bRunning) {
        IoTask task;
        {
            // 阻塞等待任务
            std::unique_lock<std::mutex> lock(m_ioMutex);
            m_ioCondition.wait(lock, [this]{ return !m_ioQueue.empty() || !m_bRunning; });
            if (!m_bRunning && m_ioQueue.empty()) break;
            task = std::move(m_ioQueue.front()); 
            m_ioQueue.pop();
            dlog_info("AI_AsyncIO Memory Trace -> IO Queue Size: %d", (int)m_ioQueue.size());
        }
        // --- 开始处理不同类型的IO任务 ---
        try {
            /* 画面分析-截图任务处理*/
            if (task.type == IoTask::SCREENSHOT_ONLY) 
            {                
                dlog_info("执行画面分析-截图任务处理\n");

                bool isTextPreset = (task.type == IoTask::TEXT_PRESET_RETURN_QUESTION);
                std::string savedPath = CmProcess::instance().save_Jpeg_Buffer_Safe(task.jpegData, isTextPreset);
                //std::string savedPath = CmProcess::instance().save_Detection_Image(task.yuvData.data(), task.yuvData.size(), false);
                if(!savedPath.empty()) 
                {
                    Alarm::AnalysisRecords_S stRecordInfo; 
                    stRecordInfo.strCreateTime = CmProcess::instance().convertTimeFormat(task.timeKey);
                    stRecordInfo.strInputImagePath = savedPath;
                    m_currentStreamImagePath = savedPath;
                    /*实时分析，先返回截图文件路径给网页，后待网页提问后一起传入，使用本地图片*/
                    if(!task.bScheduleEnable && !task.bImplicitImageIntent)
                    {
                        // 构造特殊的 JSON 返回给网页，包含 ScreenshotEnable 标识
                        std::string strStringinfo = Convert::to_string(stRecordInfo);
                        size_t pos = strStringinfo.find('{') + 1;
                        std::string strScreenshotinfo = strStringinfo.substr(0, pos) + "\"ScreenshotEnable\": \"true\"," + strStringinfo.substr(pos);
                        // 实时推送给网页，让网页显示预览图
                        TaskPublish::instance()->message(AC_RETURN_IMAGE_ANALYSIS_RESULT, strScreenshotinfo);
                    }
                    /*实时分析，意图分析（抓取画面分析）*/
                    else if(!task.bScheduleEnable && task.bImplicitImageIntent)
                    {
                        stRecordInfo.strInputText = task.prompt;
                        /*实时分析，意图分析，返回信息给网页*/
                        TaskPublish::instance()->message(AC_RETURN_IMAGE_ANALYSIS_RESULT, Convert::to_string(stRecordInfo));
                    }
                }
            }
            /*文字预设返回提问,图片信息任务*/
            else if (task.type == IoTask::TEXT_PRESET_RETURN_QUESTION) 
            {
                dlog_info("执行文字预设返回提问,图片信息任务处理\n");

                bool isTextPreset = (task.type == IoTask::TEXT_PRESET_RETURN_QUESTION);
                std::string savedPath = CmProcess::instance().save_Jpeg_Buffer_Safe(task.jpegData, isTextPreset);
                //std::string savedPath = CmProcess::instance().save_Detection_Image(task.yuvData.data(), task.yuvData.size(), true);
                if(!savedPath.empty()) 
                {
                    lastTextPresetImage = savedPath;
                    Alarm::AnalysisRecords_S stRecordInfo;
                    stRecordInfo.strInputImagePath = savedPath;
                    stRecordInfo.strCreateTime = CmProcess::instance().convertTimeFormat(task.timeKey);
                    stRecordInfo.strInputText = task.prompt;
                    TaskPublish::instance()->message(AC_RETURN_IMAGE_ANALYSIS_RESULT, Convert::to_string(stRecordInfo));
                }
            }
            /* 文字预设结果更新 */
            else if (task.type == IoTask::TEXT_PRESET_RESULT_UPDATE || task.type == IoTask::TEXT_PRESET_DETAIL_UPDATE) 
            {
                dlog_info("执行文字预设结果更新任务处理\n");
                Alarm::AnalysisRecords_S record;
                record.strCreateTime = CmProcess::instance().convertTimeFormat(task.timeKey);
                record.strOutputCreateTime = CmProcess::instance().convertTimeFormat(task.outputTimeKey);
                record.strInputText = task.prompt;
                record.strOutputText = task.result;
                if (task.type == IoTask::TEXT_PRESET_RESULT_UPDATE)
                {
                    record.bSkipDelete = task.bSkipDelete;
                    record.strInputImagePath = lastTextPresetImage; 
                    /*调用物理复制函数：从录像目录拷贝到 AI 备份目录*/
                    record.strVideoPath = CmProcess::instance().copyClosestTSFile(RECORD_PATH, task.timeKey, AI_VIDEO_BACKUP_PATH);
                    lastTextPresetVideo = record.strVideoPath;
                }

                /* 更新记录*/
                /*不属于文字预设详细分析，加入到新会话里*/
                UpdateImageAnalysisRecord(record, !task.bDetailedAnalysis);
                //文字预设关闭后，第一次实时分析加入新会话
                m_newSessionType.store(POSTPONED_SESSION);
            }
            /* 预警推送记录任务 */
            else if (task.type == IoTask::ALERT_PUSH_RECORD_UPDATE) 
            {
                dlog_info("执行预警推送记录任务处理\n");
                Alarm::TextPreset_S current_task_cfg;
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    current_task_cfg = m_stTextPreseCfg; 
                    current_task_cfg.strVideoPath = lastTextPresetVideo;
                }
                save_realAlarmPush_record(current_task_cfg, task.result, task.timeKey, lastTextPresetImage);
            }
            /*画面分析记录更新任务处理 */
            else if (task.type == IoTask::IMAGE_ANALYSIS) 
            {
                dlog_info("执行画面分析记录更新任务处理\n");
                Alarm::AnalysisRecords_S record;
                record.strCreateTime = CmProcess::instance().convertTimeFormat(task.timeKey);
                record.strOutputCreateTime = CmProcess::instance().convertTimeFormat(task.outputTimeKey);
                record.strInputText = task.prompt;
                record.strOutputText = task.result;

                if(!task.isText)
                {
                    record.strInputImagePath = task.imagePath;
                    /*上传的图片文件推理和实时分析，不保存视频片段*/
                    if(!task.isUploadImage && task.bScheduleEnable)
                    {
                         std::string reverseTime = task.timeKey; 
                         record.strVideoPath = CmProcess::instance().copyClosestTSFile(RECORD_PATH, reverseTime, AI_VIDEO_BACKUP_PATH);
                    }
                }
                /*定时分析开启，加入到新会话里*/
                if(task.bScheduleEnable)
                {
                    //定时分析关闭后，第一次实时分析加入新会话
                    m_newSessionType.store(POSTPONED_SESSION);
                    UpdateImageAnalysisRecord(record, true);
                }
                else
                {
                    bool shouldReset = (m_newSessionType.load() == THIS_SESSION);
                    bool moveToNext = (m_newSessionType.load() == NEXT_SESSION);
                    m_newSessionType.store(shouldReset ? NULL_TYPE : (moveToNext ? THIS_SESSION : m_newSessionType.load()));
                    UpdateImageAnalysisRecord(record, shouldReset);
                }
            }
            /* 移动图片文件任务处理 */
            else if (task.type == IoTask::MOVE_IMAGE) 
            {
                dlog_info("执行画面分析移动图片文件任务处理\n");
                std::string targetBasePath = EVENT_IMAGE_ANALYSIS_PIC_PATH;
                size_t pos = targetBasePath.find("image_");
                if (pos != std::string::npos) targetBasePath = targetBasePath.substr(0, pos);
                /*移动至存放目录*/
                moveImageToAnalysisDir(targetBasePath, task.imagePath);
                m_currentStreamImagePath = task.imagePath;
            }
        }
        catch (const std::exception& e) {
            dlog_error("AI_APP: IoWorker 任务处理异常: %s", e.what());
        }
    }
        dlog_info("异步IO处理线程已退出");
}

