/**
 * @FilePath     : llm_analysis.cpp
 * @Author       : leiyy
 * @Date         : 2025-09-10
 * @LastEditors  : leiyy
 * @LastEditTime : 2025-09-10
 * @Description  : 多模态LLM推理任务(支持文本和图像输入)
 */

#include "llm_analysis.hpp"
#include <fstream>
#include <chrono>

// 数据队列大小
#define LLM_QUEUE_MAX (2)

CLLmInference::CLLmInference()
    : m_dataQueue(LLM_QUEUE_MAX)
    , m_pLlmEngine(nullptr)
    , m_bEnable(false)
    , m_inferenceHistory(MAX_HISRY_NUM, RES_HISRY_PATH)
{
    m_bRunning.store(true);
    m_thread = std::thread(&CLLmInference::run, this);
}

CLLmInference::~CLLmInference()
{
    /* 通知线程停止 */
    m_bRunning.store(false);
    m_condition.notify_all();
    m_dataQueue.shutdown();
    
    if (m_thread.joinable()) {
        m_thread.join();
    }
    
    m_dataQueue.clear();
    unInit();
}

void CLLmInference::recvMediaData(MediaData_S stMediaData)
{

    if (m_RecvManager.handleEvent(stMediaData.stMediaParam.nChannel))
    {
        if (m_dataQueue.size() >= LLM_QUEUE_MAX)
        {
            dlog_error("场景智能分析-数据队列满了 [%d]", m_dataQueue.size());
        }
        // m_dataQueue.pushOrReplace(stMediaData);
    }
}

void CLLmInference::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
    m_stImageAnalysisCfg.bEnable = stAlgoConfig.nEnLLmInference;

    if (m_stImageAnalysisCfg.bEnable)
    {
        Alarm::LLMImageAnalysis_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }

}

void CLLmInference::setAlgoParamCfg(const Alarm::LLMImageAnalysis_S &stAlgoCfg)
{
     dlog_debug("ai_app: 设置LLM模型推理参数");
     m_stImageAnalysisCfg = stAlgoCfg;

    /*定时画面分析开启*/
     if(m_stImageAnalysisCfg.bScheduleEnable)
     {
        /* 单次分析*/
        if(m_stImageAnalysisCfg.enAnalysisScheduleMode == Alarm::AnalysisSchedule_E::SINGLE)
        {

        }
        /* 重复分析*/
        else if(m_stImageAnalysisCfg.enAnalysisScheduleMode == Alarm::AnalysisSchedule_E::REPEATED)
        {

        }
        /* 间隔分析*/
        else if(m_stImageAnalysisCfg.enAnalysisScheduleMode == Alarm::AnalysisSchedule_E::INTERVAL)
        {

        }
     }
     else
     {
        /*输入分析的内容*/
        m_stImageAnalysisCfg.strAnalysisInputText;
        /*输入分析的图片路径*/
        m_stImageAnalysisCfg.strAnalysisInputImagePath;
     }


 }


void CLLmInference::StartLLMInferenceTask()
{
    m_bEnable = true;
    
     if (m_bEnable && m_pLlmEngine) {
         // 重新初始化引擎
         //unInit();
         init();
     }
}

InferenceHistory& CLLmInference::getInferenceHistory()
{
    return m_inferenceHistory;
}

std::vector<InferenceRecord> CLLmInference::getAllRecords() const
{
    return m_inferenceHistory.getAllRecords();
}

InferenceRecord CLLmInference::getRecord(int id) const
{
    return m_inferenceHistory.getRecord(id);
}

std::vector<InferenceRecord> CLLmInference::getRecordsByTag(const std::string& tag) const
{
    return m_inferenceHistory.getRecordsByTag(tag);
}

bool CLLmInference::exportRecordsToCSV(const std::string& file_path) const
{
    return m_inferenceHistory.exportToCSV(file_path);
}

void CLLmInference::clearAllRecords()
{
    m_inferenceHistory.clearAll();
}


bool CLLmInference::sendTextQuery(const std::string& text, int channel)
{
    if (!m_bEnable || text.empty()) {
        dlog_error("LLM推理未启用或文本为空");
        return false;
    }
        LlmQueryData queryData;
        queryData.text = text;
        queryData.channel = channel;
        queryData.use_image = false;

        if (m_dataQueue.size() >= LLM_QUEUE_MAX) {
            dlog_warn("LLM推理数据队列已满 [%d]", m_dataQueue.size());
        }
        
        m_dataQueue.pushOrReplace(queryData);
        dlog_debug("接收到文本查询: %s, 通道: %d", text.c_str(), channel);
        return true;
    
    return false;
}

bool CLLmInference::sendImageQuery(const cv::Mat& image, const std::string& text, int channel)
{
    if (!m_bEnable || text.empty() || image.empty()) {
        dlog_error("LLM推理未启用或输入数据无效");
        return false;
    }
        LlmQueryData queryData;
        queryData.text = text;
        queryData.image = image.clone();
        queryData.channel = channel;
        queryData.use_image = true;

        if (m_dataQueue.size() >= LLM_QUEUE_MAX) {
            dlog_warn("LLM推理数据队列已满 [%d]", m_dataQueue.size());
        }
        
        m_dataQueue.pushOrReplace(queryData);
        dlog_debug("接收到图像查询: %s, 通道: %d", text.c_str(), channel);
        return true;
    
    return false;
}

bool CLLmInference::sendLocalImageQuery(const std::string& imagePath, const std::string& text, int channel)
{
    if (!m_bEnable || text.empty() || imagePath.empty()) {
        dlog_error("LLM推理未启用或输入数据无效");
        return false;
    }
        LlmQueryData queryData;
        queryData.text = text;
        queryData.image_path = imagePath;
        queryData.channel = channel;
        queryData.use_image = true;

        if (m_dataQueue.size() >= LLM_QUEUE_MAX) {
            dlog_warn("LLM推理数据队列已满 [%d]", m_dataQueue.size());
        }
        
        m_dataQueue.pushOrReplace(queryData);
        dlog_debug("接收到本地图像查询: %s, 图像路径: %s, 通道: %d", 
                  text.c_str(), imagePath.c_str(), channel);
        return true;
    
    return false;
}

bool CLLmInference::init()
{
    if (m_pLlmEngine) {
        return true;
    }

    dlog_info("初始化init_imgenc");
    std::chrono::high_resolution_clock::time_point t_load_end_us;
    std::chrono::high_resolution_clock::time_point t_start_us;
    t_start_us = std::chrono::high_resolution_clock::now();

    /*RKNN视觉模型配置*/
    std::string strRknnConfigPath = RKNN_MODEL_CONFIG_PATH;
    /*LLM推理模型配置*/
    std::string strRkllmConfigPath = RKLLM_MODEL_CONFIG_PATH;

    /* 初始化RKNN */ 
    m_pImageEncoder = std::make_unique<CImageEnoderInferenceRK>(strRknnConfigPath);
    if (m_pImageEncoder->init()) 
    {
        std::cerr << "RKNN初始化失败" << std::endl;
        return false;
    }
    t_load_end_us = std::chrono::high_resolution_clock::now();

    auto load_time = std::chrono::duration_cast<std::chrono::microseconds>(t_load_end_us - t_start_us);
    dlog_info("%s: ImgEnc Model loaded in %8.2f ms\n", __func__, load_time.count() / 1000.0);

    dlog_info("init_imgenc初始化成功");


    dlog_info("初始化LLM推理任务");

    // 创建LLM推理引擎
    m_pLlmEngine = new Inference_NS::CLLMInferenceRK(strRkllmConfigPath);
    if (!m_pLlmEngine) {
        dlog_error("创建LLM推理引擎失败");
        return false;
    }

    // 初始化引擎
    if (m_pLlmEngine->init() == true) {
        dlog_error("LLM推理引擎初始化失败");
        delete m_pLlmEngine;
        m_pLlmEngine = nullptr;
        return false;
    }

    dlog_info("LLM推理任务初始化成功");

    return true;

}

bool CLLmInference::unInit()
{
    if (m_pLlmEngine) {
        m_pLlmEngine->unInit();
        delete m_pLlmEngine;
        m_pLlmEngine = nullptr;
    }

    if (m_pImageEncoder) {
        m_pImageEncoder->release();
        m_pImageEncoder.reset();
    }

    dlog_info("LLM推理任务反初始化成功");
    return true;
}

void CLLmInference::run()
{
    pthread_setname_np(pthread_self(), "LlmInferenceTask");

    LlmQueryData queryData;

    size_t n_image_tokens = IMAGE_TOKEN_NUM;
    size_t image_embed_len = EMBED_SIZE;
    int rkllm_image_embed_len = n_image_tokens * image_embed_len;
    float img_vec[rkllm_image_embed_len];

    while (m_bRunning.load()) {
        if (!m_pLlmEngine) {
            if (!m_bEnable) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }

            if (!init()) {
                dlog_error("等待LLM推理初始化");
                std::unique_lock<std::mutex> lock(m_mutex);
                m_condition.wait_for(lock, std::chrono::seconds(1), [this] {
                    return !m_bRunning.load();
                });
            }
            continue;
        }

        /* 阻塞获取数据 */
        if (!m_dataQueue.pop(queryData, TIMEOUT_1000_MS) || queryData.text.empty()) {
            continue;
        }

        bool has_image = false;
        // 处理图像数据
        cv::Mat image;
        // 处理原始大小图像数据
        cv::Mat src_image;

        //是否推理图片
        //isText  是否纯文本
        bool isText = (queryData.text.find("<image>") == std::string::npos) ? true : false;
        if(isText == false)
        {
            if (queryData.use_image) {
                if (!queryData.image.empty()) {
                    // 使用直接传入的图像
                    image = queryData.image;
                    has_image = true;
                } else if (!queryData.image_path.empty()) {
                    // 从本地加载图像
                    loadEncodeImage(queryData.image_path, image);
                    has_image = true;
                } else if (queryData.vpss_channel >= 0) {
                    // // 从VPSS通道获取图像
                    // if (getImageFromVpss(queryData.vpss_channel, image, src_image)) {
                    //     has_image = true;
                    // } else {
                    //     dlog_error("从VPSS通道获取图像失败: %d", queryData.vpss_channel);
                    //     continue;
                    // }
                }
            }
        }

        // 执行推理
        std::string result;
        
        // 调用视觉引擎进行推理
        bool success = false;
        if (has_image) {
                int ret =0;
                ret = m_pImageEncoder->run(image.data, img_vec);
                if (ret != 0) {
                    dlog_debug("run_imgenc fail! ret=%d\n", ret);
                    continue;
                }
        } 

        // 调用LLM引擎进行推理
        dlog_debug("执行多模态推理: %s", queryData.text.c_str());
        if (!m_pLlmEngine->run(isText, queryData.text, img_vec,result)) {
                success = true; 
        } else {
                success = false; 
        }

        if (success) {
            // 处理推理结果
             handleResult(result, queryData.channel, queryData.text, has_image ? src_image : cv::Mat());
        } else {
            dlog_error("LLM推理失败");
        }
    }
}

void CLLmInference::handleResult(const std::string& result, int channel, 
                                   const std::string& input, const cv::Mat& image)
{
    if (result.empty()) {
        dlog_warn("通道 %d: 推理结果为空", channel);
        return;
    }

    dlog_info("通道 %d: LLM推理结果: %s", channel, result.c_str());
    
    //保存结果到文件
    // 创建推理记录
    InferenceRecord record(input, result, image, "LLM-1.0");

    // 根据输入类型添加标签
    if (!image.empty()) {
        record.addTag("多模态");
        record.addTag("图像分析");
    } else {
        record.addTag("文本");
    }

    // 添加通道信息标签
    record.addTag("通道" + std::to_string(channel));

    // 保存记录
    if (!m_inferenceHistory.addRecord(record)) {
        dlog_error("保存推理记录失败");
    }
}

bool CLLmInference::getImageFromVpss(int channel, cv::Mat& frame ,cv::Mat& src_frame)
{
    #if 0
    unsigned char *vpss_nv12_buffer = nullptr;
    int size = 0;
    
    dlog_debug("从VPSS通道 %d 获取图像", channel);
    CStreamVideo::instance()->Vpss_get_chnFrame(channel,&vpss_nv12_buffer,&size);
    
    if(vpss_nv12_buffer == nullptr)
    {
        dlog_debug("从VPSS通道 %d 获取图像失败", channel);
        return false;
    }

    /*NV12数据包装为OpenCV的Mat结构*/
    cv::Mat nv12_mat(VPSS_AI_IMAGE_HEIGHT * 3 / 2, VPSS_AI_IMAGE_WIDTH, CV_8UC1, vpss_nv12_buffer);

     /* 转换NV12格式到RGB*/
     cv::Mat rgb_mat;
     cv::cvtColor(nv12_mat, rgb_mat, cv::COLOR_YUV2RGB_NV12);  // NV12->RGB转换

    /*旋转图像180度 */
    cv::rotate(rgb_mat, rgb_mat, cv::ROTATE_180);

    /* 缩放到448x448 */
    cv::Mat rgb_448x448;
    // 使用INTER_AREA插值（适合缩小图像，保留细节且减少锯齿）
    cv::resize(rgb_mat, rgb_448x448, cv::Size(IMAGE_HEIGHT, IMAGE_WIDTH), 0, 0, cv::INTER_AREA);

    /*用于模型*/
    frame = rgb_448x448;
    /*用于记录保存*/
    src_frame = rgb_mat;

    CStreamVideo::instance()->Vpss_release_chnFrame();
    #endif

    return true;
}

bool CLLmInference::loadEncodeImage(const std::string& path, cv::Mat& image)
{
    cv::Mat processed_img = cv::imread(path);
    if (processed_img .empty()) {
        dlog_error("无法加载图像: %s", path.c_str());
        return false;
    }

    dlog_debug("从本地加载图像: %s", path.c_str());

    // 图像预处理
    if (processed_img.channels() == 3) {
        cv::cvtColor(processed_img, processed_img, cv::COLOR_BGR2RGB);
    }
    
    // 扩展为正方形并调整大小
    cv::Scalar background_color(127.0, 127.0, 127.0);
    
    // 计算扩展尺寸
    int width = processed_img.cols;
    int height = processed_img.rows;
    int size = std::max(width, height);
    
    cv::Mat square_img(size, size, processed_img.type(), background_color);
    
    int x_offset = (size - width) / 2;
    int y_offset = (size - height) / 2;
    
    cv::Rect roi(x_offset, y_offset, width, height);
    processed_img.copyTo(square_img(roi));
    
    // 调整到模型输入尺寸
    cv::Mat resized_img;
    cv::Size new_size(IMAGE_WIDTH, IMAGE_HEIGHT);
    cv::resize(square_img, resized_img, new_size, 0, 0, cv::INTER_LINEAR);

    image = resized_img;

    return true;
}