/**
 * @file face_detect.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-11-19
 * 
 * @brief 人脸检测相关
 */

#include "face_detect.hpp"
#include "algo_stream_deal.h"
#include "StatisticsTimer.hpp"
#include "SaveImage.hpp"
#include "storage_manage.h"
#include "capture_database.h"
#include "time_utils.h"
#include "email_manage.h"
#include "action_code.h"
#ifdef ENABLE_GAT1400_SRC
#include "gat1400.h"
#endif
#include <cstring>
#ifdef ENABLE_TVSDK_SRC
#include "convert/tvsdk_convert.h"
#include "control_manage.h"
#endif

/* 数据队列 */
#define QUEUE_MAX (2)
#define DELETE_QUEUE_MAX (150)
// 检测帧数阈值
#define DETECT_FRAME_THRESHOLD (5) 

/* 人脸检测框置信度 */
#define FACE_DETECT_THRESHOLD 0.5

/* 人脸质量评分阈值 */
#define FACE_FQASOUCE_THRESHOLD 0.4

/**
 * @brief 根据灵敏度计算质量阈值
 * @param nSensitivity 
 * @return float 
 */
float mapSensitToThrld(int nSensitivity)
{
    if (nSensitivity == 0)
    {
        return FACE_FQASOUCE_THRESHOLD;
    }
    else if (nSensitivity > 20 && nSensitivity < 80)
    {
        return 0.8f - ((nSensitivity - 20) / 60.0f) * FACE_FQASOUCE_THRESHOLD;
    }
    else if (nSensitivity >= 80 && nSensitivity <= 100)
    {
        return FACE_FQASOUCE_THRESHOLD - ((nSensitivity - 80) / 20.0f) * 0.25f;
    }
    else if (nSensitivity <= 20)
    {
        return 0.8f;
    }
    else
    {
        return FACE_FQASOUCE_THRESHOLD;
    }
}

cv::Scalar hexToScalar(const std::string& hexStr) 
{
    std::string hex = hexStr;
    
    // 移除开头的'#'（如果存在）
    if (!hex.empty() && hex[0] == '#') {
        hex = hex.substr(1);
    }
    
    // 检查长度
    if (hex.length() != 6 && hex.length() != 3) {
        std::cerr << "Error: Invalid hex color format. Expected #RRGGBB or #RGB" << std::endl;
        return cv::Scalar(0, 0, 0);  // 返回黑色作为默认值
    }
    
    unsigned int r, g, b;
    
    if (hex.length() == 6) {
        // 格式: RRGGBB
        std::stringstream ss;
        ss << std::hex << hex;
        
        unsigned int rgb;
        ss >> rgb;
        
        r = (rgb >> 16) & 0xFF;
        g = (rgb >> 8) & 0xFF;
        b = rgb & 0xFF;
    } else {
        // 格式: RGB (简写形式，每个字符重复一次)
        std::stringstream ss;
        ss << std::hex 
           << hex[0] << hex[0] 
           << hex[1] << hex[1] 
           << hex[2] << hex[2];
        
        unsigned int rgb;
        ss >> rgb;
        
        r = (rgb >> 16) & 0xFF;
        g = (rgb >> 8) & 0xFF;
        b = rgb & 0xFF;
    }
    
    // OpenCV使用BGR顺序，所以需要交换R和B
    return cv::Scalar(b, g, r);
}

CFaceDetect::CFaceDetect()
    : m_dateQueue(QUEUE_MAX),m_deleteQueue(DELETE_QUEUE_MAX)
{
    m_bRunning.store(true);
    m_thread = std::thread(&CFaceDetect::run, this);

    m_bDeleteRunning.store(true);
    m_deleteThread = std::thread(&CFaceDetect::deleteRun, this);
}

CFaceDetect::~CFaceDetect()
{
    /* 通知线程停止 */
    m_bRunning.store(false);
    m_condition.notify_all();
    MediaData_S stMediaData;
    m_dateQueue.pushOrReplace(stMediaData);
    if (m_thread.joinable())
    {
        m_thread.join();
    }

    m_bDeleteRunning.store(false);
    m_deleteCondition.notify_all();
    std::string strPath;
    m_deleteQueue.pushOrReplace(strPath);
    if (m_deleteThread.joinable())
    {
        m_deleteThread.join();
    }

    unInit();
}

/* 接受媒体数据 */
void CFaceDetect::recvMediaData(MediaData_S stMediaData)
{
    if (!m_stAlgoFaceDetectCfg.bEnable && !m_stAlgoFaceCapCfg.bEnable && !m_bFaceAttribute.load())
    {
        dlog_debug("ai_app:  人脸侦测-开关未启用");
        return;
    }

    m_nChannelId = stMediaData.stMediaParam.nChannel;
	 
    if (m_RecvManager.handleEvent(stMediaData.stMediaParam.nChannel))
    {
        if (m_dateQueue.size() >= QUEUE_MAX)
        {
            dlog_error(" 人脸侦测-数据队列满了 [%d]" ,m_dateQueue.size());
        }
        m_dateQueue.pushOrReplace(stMediaData);
    }
}
/* 判断是否在人脸规则区域内 */
bool CFaceDetect::is_in_region(const Alarm::Region_S &stRegion, const FaceDetect_NS::Result_S &stResult)
{
    Common::PosF_S stResultPoint;
    /* 取检测框的中心点 */
    stResultPoint.fX = (stResult.fX1 + stResult.fX2) / 2.0f;
    stResultPoint.fY = (stResult.fY1 + stResult.fY2) / 2.0f;

    int nCount = 0;
    const float EPSILON = 1e-6f; /* 浮点数比较精度阈值 */

    for (size_t i = 0; i < stRegion.aPoint.size(); ++i)
    {
        const Common::PosF_S &Point1 = stRegion.aPoint[i];
        const Common::PosF_S &Point2 = stRegion.aPoint[(i + 1) % stRegion.aPoint.size()];

        /* 跳过水平边 */
        float fDeltaY = Point2.fY - Point1.fY;
        if (std::abs(fDeltaY) < EPSILON)
        {
            continue;
        }

        /* 判断点是否在边的Y范围内 */
        if (((Point1.fY > stResultPoint.fY) != (Point2.fY > stResultPoint.fY)))
        {
            float fXIntersect = (Point2.fX - Point1.fX) * (stResultPoint.fY - Point1.fY) / fDeltaY + Point1.fX;
            if (stResultPoint.fX < fXIntersect)
            {
                nCount++;
            }
        }
    }

    return (nCount % 2 == 1);
}

bool CFaceDetect::init()
{
    if (!m_pFaceDetectHandle)
    {
        FaceDetect_NS::InParam_S stInParam;
        stInParam.strModelPath = "/opt/cam/model/FaceDetect.json";
        stInParam.bDebug = false;

        m_pFaceDetectHandle = new FaceDetect_NS::CFaceDetectV3_0(stInParam);
        if (m_pFaceDetectHandle)
        {
            if (m_pFaceDetectHandle->init())
            {
                dlog_debug("ai_app:  人脸侦测算法初始化成功, %s", stInParam.strModelPath.c_str());
              
                return true;
            }
            else
            {
                delete m_pFaceDetectHandle;
                m_pFaceDetectHandle = nullptr;
                dlog_debug(" 人脸侦测算法初始化失败");
            }
        }
    }

     if (!m_pFaceQuaHandle)
    {
        FaceQualityAssessment_NS::InParam_S stInParam;
        stInParam.strModelPath = "/opt/cam/model/FaceEvaluate.json";
        stInParam.bDebug = false;

        m_pFaceQuaHandle = new FaceQualityAssessment_NS::CFaceQualityAssessmentV1_0(stInParam);
        if (m_pFaceQuaHandle)
        {
            if (m_pFaceQuaHandle->init())
            {
                dlog_debug("ai_app:  人脸质量算法初始化成功, %s", stInParam.strModelPath.c_str());
              
                return true;
            }
            else
            {
                delete m_pFaceQuaHandle;
                m_pFaceQuaHandle = nullptr;
                dlog_debug(" 人脸质量算法初始化失败");
            }
        }
    }

    if (!m_pFaceAttribute)
    {
        FaceAttribute_NS::InParam_S stInParam;
        stInParam.strModelPath = "/opt/cam/model/FaceAttribute.json";
        stInParam.bDebug = false;

        m_pFaceAttribute = new FaceAttribute_NS::CFaceAttributeV1_0(stInParam);
        if (m_pFaceAttribute)
        {
            if (m_pFaceAttribute->init())
            {
                dlog_debug("ai_app:  人脸属性算法初始化成功, %s", stInParam.strModelPath.c_str());
              
                return true;
            }
            else
            {
                delete m_pFaceAttribute;
                m_pFaceAttribute = nullptr;
                dlog_debug(" 人脸属性算法初始化失败");
            }
        }
    }


    return false;
}

bool CFaceDetect::unInit()
{
    if (m_pFaceDetectHandle)
    {
        delete m_pFaceDetectHandle;
        m_pFaceDetectHandle = nullptr;
    }

    if (m_pFaceQuaHandle)
    {
        delete m_pFaceQuaHandle;
        m_pFaceQuaHandle = nullptr;
    }

    if (m_pFaceAttribute)
    {
        delete m_pFaceAttribute;
        m_pFaceAttribute = nullptr;
    }

    dlog_info("人脸侦测相关模型去初始化"); 
    
    return true;
}



/**
 * @brief 更新算法配置参数
 * @param stAlgoConfig 
 */
void CFaceDetect::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
    m_stAlgoFaceDetectCfg.bEnable = stAlgoConfig.nEnFaceDetect;
    m_stAlgoFaceCapCfg.bEnable = stAlgoConfig.nEnFaceCapture;
    
	
	if(m_stAlgoFaceDetectCfg.bEnable)
	{
		Alarm::FaceDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
	}

    if(m_stAlgoFaceCapCfg.bEnable)
	{
		Alarm::FaceCapture_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
	}
    
    Alarm::AttributeDetectSwitch_S stAttributeDetectSwitch;
    CEventConfigure::instance()->get_configure(stAttributeDetectSwitch);
    m_bFaceAttribute.store(stAttributeDetectSwitch.bFaceAttribute);
    
}

void CFaceDetect::setAlgoParamCfg(const Alarm::FaceDetection_S &stAlgoCfg,Event::Type_E enType)
{
    dlog_debug("ai_app: 设置人脸侦测参数");
    m_stAlgoFaceDetectCfg = stAlgoCfg;
    /* 转换规则区域坐标 */
    auto &region = m_stAlgoFaceDetectCfg.stRegion;
    region.ConvertResolution(PIXEL_WIDTH_1920, PIXEL_HEIGHT_1080, m_nWidth, m_nHeight);
}

void CFaceDetect::setAlgoParamCfg(const Alarm::FaceCapture_S &stAlgoCfg,Event::Type_E enType)
{
    dlog_debug("ai_app: 设置人脸抓拍参数");
    m_stAlgoFaceCapCfg = stAlgoCfg;

    /* 设置检测间隔 */
    m_RecvManager.setTimeWindow(stAlgoCfg.stRule.nInterval * 1000); /* 转换为毫秒 */
    /* 转换规则区域坐标 */
    auto &region = m_stAlgoFaceCapCfg.stRule.stRegion;
    region.ConvertResolution(PIXEL_WIDTH_1920, PIXEL_HEIGHT_1080, m_nWidth, m_nHeight);
    /* 转换最小瞳距区域坐标 */
    auto &rect = m_stAlgoFaceCapCfg.stRule.stMinIpdRect;
    rect.ConvertResolution(PIXEL_WIDTH_1920, PIXEL_HEIGHT_1080, m_nWidth, m_nHeight);
    for (auto &shieldedRegion : m_stAlgoFaceCapCfg.stRule.vstShieldedRegion)
    {
        /* 转换屏蔽区域坐标 */
        shieldedRegion.ConvertResolution(PIXEL_WIDTH_1920, PIXEL_HEIGHT_1080, m_nWidth, m_nHeight);
    }

    /* 判断联动 */
    m_bIsLinkageFacePanoramicImage = false;
    m_bIsLinkageFaceImage = false;
    m_bEmail = false;
    m_bUploadSdCard = false;
    for (auto &type : m_stAlgoFaceCapCfg.stLinkageList.tradition)
    {
        if (type == int(Alarm::LinkageType::UPLOAD_PANORAMIC_IMAGE))
        {
            m_bIsLinkageFacePanoramicImage = true;
        }
        if (type == int(Alarm::LinkageType::UPLOAD_TARGET_IMAGE))
        {
            m_bIsLinkageFaceImage = true;
        }
        else if (type == int(Alarm::LinkageType::SEND_EMAIL))
        {
            m_bEmail = true;
        }
        else if (type == int(Alarm::LinkageType::UPLOAD_SD_CARD))
        {
            m_bUploadSdCard = true;
        }
    }
}

static bool isSameTarget(std::vector<FaceDetect_NS::Result_S> vstLastFrameResult, FaceDetect_NS::Result_S stCurFrameResult)
{
    float fDx                  = 0;
    float fDy                  = 0;
    float fCenterPointDistance = 0;
    float fDistance1           = 0;
    float fDistance2           = 0;

    float fCurCenterPointX  = 0;
    float fCurCenterPointY  = 0;
    float fLastCenterPointX = 0;
    float fLastCenterPointY = 0;
    float fDiagonalLen      = 0;

    for(auto &stLastFrameResult : vstLastFrameResult)
    {
        fDx          = stLastFrameResult.fX2 - stLastFrameResult.fX1;
        fDy          = stLastFrameResult.fY2 - stLastFrameResult.fY1;
        fDiagonalLen = sqrt(fDx * fDx + fDy * fDy);

        fDx        = stCurFrameResult.fX1 - stLastFrameResult.fX1;
        fDy        = stCurFrameResult.fY1 - stLastFrameResult.fY1;
        fDistance1 = sqrt(fDx * fDx + fDy * fDy);

        fDx        = stCurFrameResult.fX2 - stLastFrameResult.fX2;
        fDy        = stCurFrameResult.fY2 - stLastFrameResult.fY2;
        fDistance2 = sqrt(fDx * fDx + fDy * fDy);

        fCurCenterPointX     = (stCurFrameResult.fX1 + stCurFrameResult.fX2) * 0.5f;
        fCurCenterPointY     = (stCurFrameResult.fY1 + stCurFrameResult.fY2) * 0.5f;
        fLastCenterPointX    = (stLastFrameResult.fX1 + stLastFrameResult.fX2) * 0.5f;
        fLastCenterPointY    = (stLastFrameResult.fY1 + stLastFrameResult.fY2) * 0.5f;
        fDx                  = fCurCenterPointX - fLastCenterPointX;
        fDy                  = fCurCenterPointY - fLastCenterPointY;
        fCenterPointDistance = sqrt(fDx * fDx + fDy * fDy);

        /* 两个中心点的距离大于上一帧目标框对角线的0.3倍，判定为不是同一个目标 */
        if (fCenterPointDistance > fDiagonalLen * 0.3f)
        {
            continue;
        }

        /* 两个中心点的距离小于等于上一帧目标框对角线的0.3倍，判定为是同一个目标 */
        if (fCenterPointDistance <= fDiagonalLen * 0.25f)
        {
            return true;
        }

        /* 中心点很近，且至少一个角点接近 */
        if (fCenterPointDistance <= fDiagonalLen * 0.30f && (fDistance1 < fDiagonalLen * 0.3f || fDistance2 < fDiagonalLen * 0.3f))
        {
            return true;
        }
    }
    return false;
}

void CFaceDetect::run()
{
    MediaData_S      stMediaData;   
    std::vector<FaceDetect_NS::Result_S> vecResult;
     /* 存储单次人脸抓拍的图片路径 */
    std::vector<std::string> vecImageFile;
    /* 人脸抓拍报警标志位 */
    bool bFaceCap = false;

    while (m_bRunning.load())
    {
        if (!m_pFaceDetectHandle)
        {
            if (!init())
            {
                dlog_error("等待人脸侦测初始化");
                /* 延迟等待 1s */
                std::unique_lock<std::mutex> lock(m_mutex);
                m_condition.wait_for(lock, std::chrono::seconds(1), [this] {
                    return !m_bRunning.load();
                });
            }
            continue;
        }

        if (!m_pFaceQuaHandle)
        {
            if (!init())
            {
                dlog_error("等待人脸质量初始化");
                /* 延迟等待 1s */
                std::unique_lock<std::mutex> lock(m_mutex);
                m_condition.wait_for(lock, std::chrono::seconds(1), [this] {
                    return !m_bRunning.load();
                });
            }
            continue;
        }

        if (!m_pFaceAttribute)
        {
            if (!init())
            {
                dlog_error("等待人脸属性初始化");
                /* 延迟等待 1s */
                std::unique_lock<std::mutex> lock(m_mutex);
                m_condition.wait_for(lock, std::chrono::seconds(1), [this] {
                    return !m_bRunning.load();
                });
            }
            continue;
        }

        /* 阻塞获取 */
        m_dateQueue.pop(stMediaData, -1);
        if (stMediaData.nSize == 0)
        {
            /* 数据为空 */
            continue;
        }
      
        CStatisticsTimer runTime(" 人脸侦测完整耗时");
        bFaceCap = false;
        FcaeEventStatus_t stFcaeEventStatus; 
        /* 送分析 */
        if (1)
        {
            frameRate(" 人脸侦测-分析数据", 5);

            FaceDetect_NS::InData_S stFDInData;
            FaceDetect_NS::OutData_S stFDOutData;
            
            stFDInData.stParam.fBoxThreshold = FACE_DETECT_THRESHOLD;
            stFDOutData.validResult = false;
            
            cv::Mat i420Mat(
                stMediaData.stMediaParam.nVideoHeight * 3 / 2,
                stMediaData.stMediaParam.nVideoWidth,
                CV_8UC1,
                stMediaData.pData.get()
            );

            /* 缓存 4K 全分辨率帧: 人脸特写图裁剪源 (与主帧同 PTS) */
            m_pFullNv12    = stMediaData.pFullData;
            m_nFullWidth   = stMediaData.nFullWidth;
            m_nFullHeight  = stMediaData.nFullHeight;

            /* RGA 硬件: NV12 -> RGB 1080p, 替代 CPU cvtColor */
            const int nFrameW = stMediaData.stMediaParam.nVideoWidth;
            const int nFrameH = stMediaData.stMediaParam.nVideoHeight;
            if (m_fullRgbMat.cols != nFrameW || m_fullRgbMat.rows != nFrameH || m_fullRgbMat.type() != CV_8UC3)
            {
                m_fullRgbMat.create(nFrameH, nFrameW, CV_8UC3);
            }
            if (!rga_image_transform(stMediaData.pData.get(), nFrameW, nFrameH, RK_FORMAT_YCbCr_420_SP,
                                     m_fullRgbMat.data, nFrameW, nFrameH, RK_FORMAT_RGB_888,
                                     0, 0, nFrameW, nFrameH, 0))
            {
                /* RGA 失败: CPU 回退 */
                cv::cvtColor(i420Mat, m_fullRgbMat, cv::COLOR_YUV2RGB_NV12);
            }

            /* 缩小到模型输入尺寸 (CPU, 千级像素, <1ms) */
            cv::resize(m_fullRgbMat, stFDInData.inMat,
                       cv::Size(m_nWidth, m_nHeight), 0, 0, cv::INTER_LINEAR);

            // cv::rotate(stFDInData.inMat, stFDInData.inMat, cv::ROTATE_180);
            // cv::rotate(bgrMat, bgrMat, cv::ROTATE_180);

            if (!stFDInData.inMat.empty())
            {
                if (access("/Face_debugImage", F_OK) == 0)
                {
                    dlog_debug("============>debugImage");
                    cv::imwrite("/opt/algo/Image/test_algo.jpg", stFDInData.inMat);
                }

                /* 分析数据 */
                {
                    CStatisticsTimer runTime(" 人脸侦测算法耗时");
                    m_pFaceDetectHandle->process(stFDInData, vecResult, &stFDOutData);

                    const FaceDetect_NS::Result_S* pDetectResult = nullptr;
                    const FaceDetect_NS::Result_S* pCaptureResult = nullptr;

                    if (stFDOutData.validResult)
                    {
                        std::vector<Common::RectInfo_S> vstRectInfo;
                        for (const auto& result : vecResult)
                        {
                            /* 人脸质量评估 */
                            FaceQualityAssessment_NS::InData_S stFQInData;
                            FaceQualityAssessment_NS::Result_S stFQOneRes;

                            double scaleFactor = 1.5;
                        
                            /* 人脸裁剪比例放大 */
                            
                            int x1 = std::min(static_cast<float>(stFDInData.inMat.cols), std::max(0.0f, result.fX1));
                            int y1 = std::min(static_cast<float>(stFDInData.inMat.rows), std::max(0.0f, result.fY1));
                            int x2 = std::min(static_cast<float>(stFDInData.inMat.cols), std::max(0.0f, result.fX2));
                            int y2 = std::min(static_cast<float>(stFDInData.inMat.rows), std::max(0.0f, result.fY2));

                            pointScaleUp(x1, y1, x2, y2, stFDInData.inMat.cols, stFDInData.inMat.rows, scaleFactor);

                            // if ((x2 - x1) < 64 || (y2 - y1) < 64)
                            // {
                            //     /* 过滤掉低分辨率的人脸结果 */
                            //     continue;
                            // }
                            
                            cv::Rect roi(x1, y1, x2 - x1, y2 - y1);
                            if (roi.x < 0 || roi.y < 0 || 
                                roi.x + roi.width > stFDInData.inMat.cols || 
                                roi.y + roi.height > stFDInData.inMat.rows)
                            {
                                dlog_error("ai_app: ROI越界: x=%d,y=%d,w=%d,h=%d", roi.x,roi.y,roi.width,roi.height);
                                continue;
                            }

                            /* 预计算原图坐标映射：仅当 640 ROI < 112(模型输入)时走原图,
                               大脸直接用 640 图省内存(4K 下满屏脸走原图会多占 ~25MB OOM) */
                            if (m_fullRgbMat.empty())
                            {
                                dlog_error("ai_app: 原图缓存为空, 跳过质量评估");
                                continue;
                            }
                            const double dScaleX = static_cast<double>(m_fullRgbMat.cols)
                                                 / static_cast<double>(stFDInData.inMat.cols);
                            const double dScaleY = static_cast<double>(m_fullRgbMat.rows)
                                                 / static_cast<double>(stFDInData.inMat.rows);
                            bool bUseFullRes = false;
                            cv::Rect fullRoi;
                            if (dScaleX > 0.0 && dScaleY > 0.0)
                            {
                                const int fx1 = std::max(0, static_cast<int>(x1 * dScaleX));
                                const int fy1 = std::max(0, static_cast<int>(y1 * dScaleY));
                                const int fx2 = std::min(m_fullRgbMat.cols, static_cast<int>(x2 * dScaleX));
                                const int fy2 = std::min(m_fullRgbMat.rows, static_cast<int>(y2 * dScaleY));
                                if (fx2 > fx1 && fy2 > fy1)
                                {
                                    fullRoi = cv::Rect(fx1, fy1, fx2 - fx1, fy2 - fy1);
                                    bUseFullRes = (roi.width < 112 || roi.height < 112);
                                }
                            }

                            //printf("\033[34m %s:%d ] x=%d,y=%d,w=%d,h=%d \033[m\n",__func__,__LINE__,roi.x,roi.y,roi.width,roi.height);
                            
                            /* 将ROI等比填充到stFQInData.inMat (112x112 黑底) */
                            stFQInData.inMat = cv::Mat(112, 112, CV_8UC3, cv::Scalar(0, 0, 0));
                            if (bUseFullRes)
                            {
                                if (!fillRGBToCenter(m_fullRgbMat, fullRoi, stFQInData.inMat))
                                {
                                    dlog_error("ai_app: Failed to fill ROI to center!");
                                    continue;
                                }
                            }
                            else
                            {
                                if (!fillRGBToCenter(stFDInData.inMat, roi, stFQInData.inMat))
                                {
                                    dlog_error("ai_app: Failed to fill ROI to center!");
                                    continue;
                                }
                            }

                            stFQOneRes.fX1 = x1;
                            stFQOneRes.fY1 = y1;
                            stFQOneRes.fX2 = x2;
                            stFQOneRes.fY2 = y2;
                            stFQOneRes.fBoxConfidence = result.fBoxConfidence;
                            stFQOneRes.vPoint = result.vPoint;

                            if (access("/SnapImage", F_OK) == 0)
                            {
                                std::string savedFileName; Modules_NS::saveImage(stFQInData.inMat, "/mnt/bin/SnapImage", 0, 0, savedFileName);
                                dlog_debug("ai_app: 保存转换后的人脸图片 [通道号: 0, 文件名: %s]", savedFileName.c_str());
                            }
                            

                            /* 质量评估 */
                            m_pFaceQuaHandle->process(stFQInData, stFQOneRes.fFqaSouce);
                            dlog_debug("ai_app:  当前人脸质量评估得分 [%.2f] 置信度[%.2f]", stFQOneRes.fFqaSouce,stFQOneRes.fBoxConfidence);
                            const cv::Rect2f faceRect(
                                static_cast<float>(roi.x),
                                static_cast<float>(roi.y),
                                static_cast<float>(roi.width),
                                static_cast<float>(roi.height)
                            );
                            Common::RectInfo_S rectInfo;
                            rectInfo.nX1 = result.fX1;
                            rectInfo.nY1 = result.fY1;
                            rectInfo.nX2 = result.fX2;
                            rectInfo.nY2 = result.fY2;
                            vstRectInfo.emplace_back(rectInfo);
                            /*  人脸侦测 */
                            if(m_stAlgoFaceDetectCfg.bEnable)
                            {
                                if (stFQOneRes.fFqaSouce >= 0.05 || result.fBoxConfidence >= 0.5)
                                {
                                    /* 区域判断 */
                                    if (!is_in_region(m_stAlgoFaceDetectCfg.stRegion, result))
                                    {
                                        /* 人脸检测结果在检测规则外，进行下一个目标的处理 */
                                        continue;
                                    }

                                    /* 上报事件 */
                                    dlog_debug("ai_app:  人脸侦测报警触发 " );
                                    stFcaeEventStatus.bFaceDetect = true;
                                    pDetectResult = &result;
                                    // CEventLinkage::instance()->handleEvent(Event::Type_E::FACE_DETECT, false);
#ifdef ENABLE_GAT1400_SRC
                                    dlog_debug("人脸上传GAT1400处理");
                                    Network::Gat1400Client_S config;
                                    GAT1400::CGAT1400::instance()->getGat1400Config(config);
                                    if (config.enableGat1400 && config.enableFace)
                                    {
                                        /* 人脸属性识别：bUseFullRes 时从原图 letterbox, 否则回退 112→192 旧方案 */
                                        FaceAttribute_NS::InData_S stFAInData;
                                        std::vector<FaceAttribute_NS::Result_S> vecFARes;
                                        if (bUseFullRes)
                                        {
                                            stFAInData.inMat = cv::Mat(m_nFAWidth, m_nFAHeight, CV_8UC3, cv::Scalar(0, 0, 0));
                                            if (fillRGBToCenter(m_fullRgbMat, fullRoi, stFAInData.inMat))
                                            {
                                                m_pFaceAttribute->process(stFAInData, vecFARes);
                                            }
                                            else
                                            {
                                                dlog_error("ai_app: 属性识别-原图ROI填充失败");
                                            }
                                        }
                                        else
                                        {
                                            cv::resize(
                                                stFQInData.inMat,
                                                stFAInData.inMat,
                                                cv::Size(m_nFAWidth, m_nFAHeight),
                                                0, 0,
                                                cv::INTER_LINEAR
                                            );
                                            m_pFaceAttribute->process(stFAInData, vecFARes);
                                        }

                                        cv::Mat imageMat, faceMat;
                                        // 人脸全景图片
                                        cv::cvtColor(i420Mat, imageMat, cv::COLOR_YUV2BGR_NV12);

                                        // 人脸图片
                                        const cv::Rect2f faceRawRect(
                                            result.fX1, result.fY1,
                                            result.fX2 - result.fX1,
                                            result.fY2 - result.fY1);
                                        if (!cropFaceCloseup(faceRawRect,
                                                             stFDInData.inMat.size(),
                                                             faceMat))
                                        {
                                            dlog_error("ai_app: 裁剪人脸目标小图失败");
                                        }

                                        pushFaceImageToGat1400(imageMat, faceMat, vecFARes, result);
                                    }
#endif
                                }

                            }

                            /*  人脸抓拍 */
                            if(m_stAlgoFaceCapCfg.bEnable || m_bFaceAttribute.load())
                            {
                                /* m_nFrameCount 降低人脸属性检测频率 */
                                if(!m_stAlgoFaceCapCfg.bEnable && m_nFrameCount < DETECT_FRAME_THRESHOLD)
                                {
                                    continue;
                                }

                                if (stFQOneRes.fFqaSouce >= 0.05 || result.fBoxConfidence >= 0.5)
                                {
                                    /* 判断是否进行抓拍 */
                                    if (!processFaceCapture(result) && !m_bFaceAttribute.load())
                                    {
                                        dlog_debug("当前人脸不满足抓拍条件");
                                        /* 不满足抓拍条件,进行下一个目标的处理 */
                                        continue;
                                    }
                                    bFaceCap = true;

                                    /* 人脸属性识别：bUseFullRes 时从原图 letterbox, 否则回退 112→192 旧方案 */
                                    FaceAttribute_NS::InData_S stFAInData;
                                    std::vector<FaceAttribute_NS::Result_S> vecFARes;
                                    if (bUseFullRes)
                                    {
                                        stFAInData.inMat = cv::Mat(m_nFAWidth, m_nFAHeight, CV_8UC3, cv::Scalar(0, 0, 0));
                                        if (fillRGBToCenter(m_fullRgbMat, fullRoi, stFAInData.inMat))
                                        {
                                            /* 属性识别 */
                                            m_pFaceAttribute->process(stFAInData, vecFARes);
                                        }
                                        else
                                        {
                                            dlog_error("ai_app: 属性识别-原图ROI填充失败");
                                        }
                                    }
                                    else
                                    {
                                        cv::resize(
                                            stFQInData.inMat,
                                            stFAInData.inMat,
                                            cv::Size(m_nFAWidth, m_nFAHeight),
                                            0, 0,
                                            cv::INTER_LINEAR
                                        );
                                        /* 属性识别 */
                                        m_pFaceAttribute->process(stFAInData, vecFARes);
                                    }
                                   
                                    if(m_stAlgoFaceCapCfg.bEnable)
                                    {
                                        dlog_info("人脸抓拍事件开始");
                                        /* 触发对应的报警事件 */
                                        // CEventLinkage::instance()->handleEvent(Event::Type_E::FACE_CAPTURE, false);
                                        stFcaeEventStatus.bFaceCapture = true;
                                        pCaptureResult = &result;
                                    }

                                    std::string strCurrentDate = TimeUtils_NS::get_currentDateWithDash();
                                    std::string strCurrentTime = TimeUtils_NS::get_currentTimeWithColon();
                                    std::string strFacePicture;                     
                                    std::string strCurrentPicture;  

                                    bool bIsSameTarget = true;
                                    if(m_bFaceAttribute.load())
                                    {
                                        bIsSameTarget = isSameTarget(m_vstLastFrameResult, result);
                                    }
           
                                    /* 是否联动保存人脸全景图片 */
                                    if(m_bIsLinkageFacePanoramicImage || !bIsSameTarget)
                                    {
                                        cv::Mat faceMat;
                                        cv::cvtColor(i420Mat, faceMat, cv::COLOR_YUV2BGR_NV12);
                                        // cv::rotate(faceMat, faceMat, cv::ROTATE_180);

                                        /* 保存人脸全景图片 */
                                        strCurrentPicture = saveFacePanoramicImage(faceMat,"");
                                        if(!strCurrentPicture.empty())
                                        {
                                            vecImageFile.emplace_back(strCurrentPicture);
                                        }
                                    }
                                    /* 是否联动保存人脸图片 */
                                    if(m_bIsLinkageFaceImage || !bIsSameTarget)
                                    {
                                        cv::Mat faceMat;
                                        const cv::Rect2f faceRawRect(
                                            result.fX1, result.fY1,
                                            result.fX2 - result.fX1,
                                            result.fY2 - result.fY1);
                                        if (cropFaceCloseup(faceRawRect,
                                                           stFDInData.inMat.size(),
                                                           faceMat))
                                        {
                                            /* 保存人脸图片 */
                                            strFacePicture = saveFaceImage(faceMat,"");
                                            if(!strFacePicture.empty())
                                            {
                                                vecImageFile.emplace_back(strFacePicture);
                                            }
                                        }
                                        else
                                        {
                                            dlog_error("ai_app: 裁剪人脸目标小图失败");
                                        }
                                        
                                    }

                                    if(m_bFaceAttribute.load() && !vecFARes.empty() && (!strCurrentPicture.empty() || !strFacePicture.empty()))
                                    {
                                        /* 人脸抓拍信息推送 */
                                        pushFaceCaptureInfo(vecFARes[0],strCurrentPicture,strFacePicture);
                                    }
#ifdef ENABLE_TVSDK_SRC
                                    if(m_bFaceAttribute.load() && !vecFARes.empty())
                                    {
                                        /* 人脸抓拍信息 TVSDK 二进制直推 */
                                        cv::Mat panoramaMat;
                                        cv::cvtColor(i420Mat, panoramaMat, cv::COLOR_YUV2BGR_NV12);
                                        const cv::Rect2f faceRawRect(
                                            result.fX1, result.fY1,
                                            result.fX2 - result.fX1,
                                            result.fY2 - result.fY1);
                                        pushFaceCaptureInfoToTvSdk(panoramaMat, faceRect, faceRawRect, stFDInData.inMat.size(), vecFARes[0]);
                                    }
#endif
                                    if((m_bUploadSdCard && SD_CARD_STATUS_E::NORMAL == CStorageManage::instance()->get_SdCardStatus()) || (m_bFaceAttribute.load() && SD_CARD_STATUS_E::NORMAL == CStorageManage::instance()->get_SdCardStatus()))
                                    {
                                        if(!strCurrentPicture.empty())
                                        {
                                            /* 保存信息至抓图图片数据库 */
                                            saveToDatebase(strCurrentPicture, strCurrentDate, strCurrentTime);
                                        }

                                        if(!strFacePicture.empty())
                                        {
                                            /* 保存信息至抓图图片数据库 */
                                            saveToDatebase(strFacePicture, strCurrentDate, strCurrentTime);
                                        }
                                       
                                    }
                                    else
                                    {
                                        if (m_deleteQueue.size() >= DELETE_QUEUE_MAX)
                                        {
                                            dlog_error(" 删除人脸抓拍图片-数据队列满了 [%d]" ,m_deleteQueue.size());
                                        }
                                        /* 加入图片删除队列中 */
                                        if(!strCurrentPicture.empty())
                                        {
                                            m_deleteQueue.pushOrReplace(strCurrentPicture);
                                        }
                                        
                                        if(!strFacePicture.empty())
                                        {
                                            m_deleteQueue.pushOrReplace(strFacePicture);
                                        }
                                    }

                                    if(m_stAlgoFaceCapCfg.bEnable)
                                    {
#ifdef ENABLE_GAT1400_SRC
                                        dlog_debug("人脸上传GAT1400处理");
                                        Network::Gat1400Client_S config;
                                        GAT1400::CGAT1400::instance()->getGat1400Config(config);
                                        if (config.enableGat1400 && config.enableFace)
                                        {
                                            cv::Mat imageMat, faceMat;
                                            // 人脸全景图片
                                            cv::cvtColor(i420Mat, imageMat, cv::COLOR_YUV2BGR_NV12);
    
                                            // 人脸图片
                                            const cv::Rect2f faceRawRect(
                                                result.fX1, result.fY1,
                                                result.fX2 - result.fX1,
                                                result.fY2 - result.fY1);
                                            if (!cropFaceCloseup(faceRawRect,
                                                                 stFDInData.inMat.size(),
                                                                 faceMat))
                                            {
                                                dlog_error("ai_app: 裁剪人脸目标小图失败");
                                            }
    
                                            pushFaceImageToGat1400(imageMat, faceMat, vecFARes, result);
                                        }
#endif

                                        /* 触发对应的报警事件 */
                                        // CEventLinkage::instance()->handleEvent(Event::Type_E::FACE_CAPTURE, true);
                                        dlog_info("人脸抓拍事件结束");
                                    }
                                }
                            }

                        }
                        
                        if(m_stAlgoFaceDetectCfg.bEnable || m_stAlgoFaceCapCfg.bEnable)
                        {
                            /* 是否联动邮件,需上传图片附件 */
                            if (m_bEmail && m_bUploadSdCard && bFaceCap)
                            {
                                Network::EmailEventInfo_S stEventInfo;
                                stEventInfo.strSubject = "人脸抓拍";
                
                                std::ostringstream oss;
                                oss << "事件类型: " << stEventInfo.strSubject << "\n"
                                    << "日期: " << TimeUtils_NS::get_currentDateWithDash() << "\n"
                                    << "时间: " << TimeUtils_NS::get_currentTimeWithColon();
                                stEventInfo.strMessage = oss.str();
                                stEventInfo.vecImageFile = vecImageFile;
                                CEmailManage::instance()->HandleEmail(stEventInfo);
                            }
                            vecImageFile.clear();
                        }

                        /* 动态分析 */
                        if(m_stAlgoFaceDetectCfg.bDynamicAnalysisEnable || m_bFaceAttribute.load())
                        {
                            if(!vstRectInfo.empty())
                            {
                                send_detectionResult_to_osd(m_nWidth, m_nHeight, vstRectInfo);
                            }
                        }
                        
                    }

                    processFaceEvent(stFcaeEventStatus, pDetectResult, pCaptureResult); 
                    if(m_nFrameCount >= DETECT_FRAME_THRESHOLD)
                    {
                        m_nFrameCount = 0;
                        m_vstLastFrameResult = vecResult; 
                    }
                    m_nFrameCount++;
                }
            }
            else
            {
                dlog_error("ai_app: 图片数据为空");
            }
        }
        else
        {
            dlog_error("ai_app:  人脸侦测-获取虚拟地址失败");
        }
    }
}


void CFaceDetect::deleteRun()
{
    std::string strDeleteFile;
    while (m_bDeleteRunning.load())
    {
        /* 阻塞获取 */
        m_deleteQueue.pop(strDeleteFile, -1);
        {
            /* 延迟等待 0.5s */
            std::unique_lock<std::mutex> lock(m_mutex);
            m_deleteCondition.wait_for(lock, std::chrono::milliseconds(500), [this] 
            {
                return !m_bDeleteRunning.load();
            });
        }

        dlog_debug("删除抓拍文件：[%s]",strDeleteFile.c_str());
        /* 删除文件 */
        std::remove(strDeleteFile.c_str());
    }
}

std::string CFaceDetect::saveFacePanoramicImage(const cv::Mat &image,std::string strFaceAttribute)
{
     if(image.empty() || SD_CARD_STATUS_E::NORMAL != CStorageManage::instance()->get_SdCardStatus())
    {
        return "";
    }

    std::string strCurrentDate = TimeUtils_NS::get_currentDate();

    std::string fullPath = std::string(CAPTURE_PATH) + "/" + strCurrentDate;

    struct stat info;
    /* 目录不存在 */
    if (stat(fullPath.c_str(), &info) != 0)
    {
        std::string strCmd = "mkdir -p \"" + fullPath + "\"";
        int         nRet   = system(strCmd.c_str());

        if (nRet != 0)
        {
            dlog_error("[%s]命令执行失败", strCmd.c_str());
            return "";
        }
    }

    /* 添加叠加数据 */
    addFaceOverlayInfo(image);

    /* 抓图路径 + 日期（YYYYMMDD）+ 时间（HHMMSS）+ 事件类型 + 计数值.jpg */
    std::string strFilename = std::string(CAPTURE_PATH) + "/" + strCurrentDate + "/" + strCurrentDate + "_" +
                            TimeUtils_NS::get_currentTimeMs() + "_" +
                            std::to_string(static_cast<int>(Event::Type_E::FACE_CAPTURE)) + "_" +
                            std::to_string(int(Alarm::LinkageType::UPLOAD_PANORAMIC_IMAGE)) + ".jpg";

    dlog_debug("[人脸全景大图] 保存人脸图片[%s]",strFilename.c_str());
    if( cv::imwrite(strFilename, image))
    {
        return strFilename;
    }
    else
    {
        return "";
    }
}

std::string CFaceDetect::saveFaceImage(const cv::Mat &image,std::string strFaceAttribute)
{
    if(image.empty() || SD_CARD_STATUS_E::NORMAL != CStorageManage::instance()->get_SdCardStatus())
    {
        return "";
    }
   
    std::string strCurrentDate = TimeUtils_NS::get_currentDate();

    std::string fullPath = std::string(CAPTURE_PATH) + "/" + strCurrentDate;

    struct stat info;
    /* 目录不存在 */
    if (stat(fullPath.c_str(), &info) != 0)
    {
        std::string strCmd = "mkdir -p \"" + fullPath + "\"";
        int         nRet   = system(strCmd.c_str());

        if (nRet != 0)
        {
            dlog_error("[%s]命令执行失败", strCmd.c_str());
            return "";
        }
    }

    /* 抓图路径 + 日期（YYYYMMDD）+ 时间（HHMMSS）+ 事件类型 + 计数值.jpg */
    std::string strFilename = std::string(CAPTURE_PATH) + "/" + strCurrentDate + "/" + strCurrentDate + "_" +
                            TimeUtils_NS::get_currentTimeMs() + "_" +
                            std::to_string(static_cast<int>(Event::Type_E::FACE_CAPTURE)) + "_" +
                            std::to_string(int(Alarm::LinkageType::UPLOAD_TARGET_IMAGE)) + ".jpg";
    
    dlog_debug("[人脸目标小图] 保存人脸图片[%s]",strFilename.c_str());

    if( cv::imwrite(strFilename, image))
    {
        return strFilename;
    }
    else
    {
        return "";
    }
}

bool CFaceDetect::processFaceCapture(const FaceDetect_NS::Result_S &stResult)
{
    /* 是否报警并进行抓拍 */
    bool bIsAlarm = false;
    /* 是否正确设置检测区域 */
    bool bIsDetect = false;
    /* 是否正确设置屏蔽区域 */
    bool bIsShielded = false;
    /* 最小瞳距 */
    int nMinIpd = m_nMinIpd;

    auto &rule = m_stAlgoFaceCapCfg.stRule;
    /* 判断规则区域是否设置了正确的多边形 */
    if (rule.stRegion.IsValid())
    {
        bIsDetect = true;
    }

    /* 判断最小瞳距区域是否设置了正确的区域 */
    if (rule.stMinIpdRect.IsValid())
    {
        /* 更新最小瞳距 */
        nMinIpd = std::min(std::max(MIN_IPD, rule.stMinIpdRect.nWidth), MAX_IPD);
    }

    /* 判断屏蔽区域是否设置了正确的区域 */
    for (auto &shieldedRegion : rule.vstShieldedRegion)
    {
        if (shieldedRegion.IsValid())
        {
            bIsShielded = true;
        }
    }

    /* 正确设置屏蔽区域才进行处理 */
    if (bIsShielded)
    {
        /* 判断人脸检测结果是否在屏蔽区域内 */
        bool bIsResultInShielded = false;
        for (auto &shieldedRegion : rule.vstShieldedRegion)
        {
            if (is_in_region(shieldedRegion, stResult))
            {
                bIsResultInShielded = true;
            }
        }
        if (bIsResultInShielded)
        {
            /* 人脸检测结果在屏蔽区域内，进行下一个目标的处理 */
            return false;
        }
    }

    /* 正确设置检测规则区域-部分区域判断 */
    if (bIsDetect)
    {
        /* 判断人脸检测结果是否在检测规则区域外 */
        if (!is_in_region(rule.stRegion, stResult))
        {
            /* 人脸检测结果在检测规则外，进行下一个目标的处理 */
            return false;
        }
    }
    
    /* 瞳距判断 */
    unsigned int nIpd = stResult.vPoint[2] - stResult.vPoint[0];
    // dlog_info("[人脸抓拍 瞳距 [%d]  阈值[%d]",  nIpd, nMinIpd);
    // if (nIpd < nMinIpd)
    // {
    //     return false; // 瞳距过小，跳过这个目标在当前区域的检测
    // }

    bIsAlarm = true;
    dlog_info("[人脸抓拍] 置信度度[%.3f]  瞳距 [%d] > [%d]", stResult.fBoxConfidence, nIpd, nMinIpd);

    return bIsAlarm;
}

void CFaceDetect::processFaceEvent(const FcaeEventStatus_t &stFcaeEventStatus,
                                   const FaceDetect_NS::Result_S* pDetectResult,
                                   const FaceDetect_NS::Result_S* pCaptureResult)
{
    if (m_stAlgoFaceDetectCfg.bEnable)
    {
        /* 上报事件 */
#ifdef ENABLE_TVSDK_SRC
        EventTriggerContext_S stCtxFaceDetect;
        stCtxFaceDetect.enEventType = Event::Type_E::FACE_DETECT;
        stCtxFaceDetect.nChnId = m_nChannelId;
        stCtxFaceDetect.llTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                                          std::chrono::system_clock::now().time_since_epoch())
                                          .count();
        if (stFcaeEventStatus.bFaceDetect && !m_fullRgbMat.empty())
        {
            auto pPayload = std::make_shared<EventTvSdkPayload_S>();
            pPayload->enType = get_tvsdk_payload_type(stCtxFaceDetect.enEventType);
            if (encode_mat_to_tvsdk_image(m_fullRgbMat, pPayload->stPanoramaImage))
            {
                stCtxFaceDetect.pTvSdkPayload = pPayload;
            }
            if (pDetectResult)
            {
                const float fScaleX = m_nWidth > 0 ? static_cast<float>(m_fullRgbMat.cols) / static_cast<float>(m_nWidth) : 1.0f;
                const float fScaleY = m_nHeight > 0 ? static_cast<float>(m_fullRgbMat.rows) / static_cast<float>(m_nHeight) : 1.0f;
                /* 模型坐标系下的裁剪区域，与质量评估/人脸属性抓拍路径一致放大1.5倍留边 */
                int nMX1 = std::max(0, std::min(m_nWidth, static_cast<int>(pDetectResult->fX1)));
                int nMY1 = std::max(0, std::min(m_nHeight, static_cast<int>(pDetectResult->fY1)));
                int nMX2 = std::max(0, std::min(m_nWidth, static_cast<int>(pDetectResult->fX2)));
                int nMY2 = std::max(0, std::min(m_nHeight, static_cast<int>(pDetectResult->fY2)));
                pointScaleUp(nMX1, nMY1, nMX2, nMY2, m_nWidth, m_nHeight, 1.5);
                if (nMX2 > nMX1 && nMY2 > nMY1)
                {
                    stCtxFaceDetect.nLeft = std::min(m_fullRgbMat.cols, static_cast<int>(nMX1 * fScaleX));
                    stCtxFaceDetect.nTop = std::min(m_fullRgbMat.rows, static_cast<int>(nMY1 * fScaleY));
                    stCtxFaceDetect.nRight = std::min(m_fullRgbMat.cols, static_cast<int>(nMX2 * fScaleX));
                    stCtxFaceDetect.nBottom = std::min(m_fullRgbMat.rows, static_cast<int>(nMY2 * fScaleY));
                    stCtxFaceDetect.nObjectType = 1;
                    stCtxFaceDetect.fConfidence = pDetectResult->fBoxConfidence;
                    cv::Mat faceMat;
                    const cv::Rect2f faceRawRect(
                        static_cast<float>(pDetectResult->fX1),
                        static_cast<float>(pDetectResult->fY1),
                        static_cast<float>(pDetectResult->fX2 - pDetectResult->fX1),
                        static_cast<float>(pDetectResult->fY2 - pDetectResult->fY1));
                    if (cropFaceCloseup(faceRawRect,
                                        cv::Size(m_nWidth, m_nHeight),
                                        faceMat))
                    {
                        EventTvSdkImage_S stTarget;
                        if (encode_mat_to_tvsdk_image(faceMat, stTarget)) {
                            stCtxFaceDetect.stTargetImage = std::move(stTarget);
                        }
                    }
                }
            }
        }
        m_FaceDetectStateMachine.handleAlarmState(stFcaeEventStatus.bFaceDetect, stCtxFaceDetect);
#else
        m_FaceDetectStateMachine.handleAlarmState(stFcaeEventStatus.bFaceDetect, Event::Type_E::FACE_DETECT);
#endif
    }
    if (m_stAlgoFaceCapCfg.bEnable) 
    {
        /* 上报事件 */
#ifdef ENABLE_TVSDK_SRC
        EventTriggerContext_S stCtxFaceCapture;
        stCtxFaceCapture.enEventType = Event::Type_E::FACE_CAPTURE;
        stCtxFaceCapture.nChnId = m_nChannelId;
        stCtxFaceCapture.llTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                                           std::chrono::system_clock::now().time_since_epoch())
                                           .count();
        if (stFcaeEventStatus.bFaceCapture && !m_fullRgbMat.empty())
        {
            auto pPayload = std::make_shared<EventTvSdkPayload_S>();
            pPayload->enType = get_tvsdk_payload_type(stCtxFaceCapture.enEventType);
            if (encode_mat_to_tvsdk_image(m_fullRgbMat, pPayload->stPanoramaImage))
            {
                stCtxFaceCapture.pTvSdkPayload = pPayload;
            }
            if (pCaptureResult)
            {
                const float fScaleX = m_nWidth > 0 ? static_cast<float>(m_fullRgbMat.cols) / static_cast<float>(m_nWidth) : 1.0f;
                const float fScaleY = m_nHeight > 0 ? static_cast<float>(m_fullRgbMat.rows) / static_cast<float>(m_nHeight) : 1.0f;
                /* 模型坐标系下的裁剪区域，与质量评估/人脸属性抓拍路径一致放大1.5倍留边 */
                int nMX1 = std::max(0, std::min(m_nWidth, static_cast<int>(pCaptureResult->fX1)));
                int nMY1 = std::max(0, std::min(m_nHeight, static_cast<int>(pCaptureResult->fY1)));
                int nMX2 = std::max(0, std::min(m_nWidth, static_cast<int>(pCaptureResult->fX2)));
                int nMY2 = std::max(0, std::min(m_nHeight, static_cast<int>(pCaptureResult->fY2)));
                pointScaleUp(nMX1, nMY1, nMX2, nMY2, m_nWidth, m_nHeight, 1.5);
                if (nMX2 > nMX1 && nMY2 > nMY1)
                {
                    stCtxFaceCapture.nLeft = std::min(m_fullRgbMat.cols, static_cast<int>(nMX1 * fScaleX));
                    stCtxFaceCapture.nTop = std::min(m_fullRgbMat.rows, static_cast<int>(nMY1 * fScaleY));
                    stCtxFaceCapture.nRight = std::min(m_fullRgbMat.cols, static_cast<int>(nMX2 * fScaleX));
                    stCtxFaceCapture.nBottom = std::min(m_fullRgbMat.rows, static_cast<int>(nMY2 * fScaleY));
                    stCtxFaceCapture.fConfidence = pCaptureResult->fBoxConfidence;
                    cv::Mat faceMat;
                    const cv::Rect2f faceRawRect(
                        static_cast<float>(pCaptureResult->fX1),
                        static_cast<float>(pCaptureResult->fY1),
                        static_cast<float>(pCaptureResult->fX2 - pCaptureResult->fX1),
                        static_cast<float>(pCaptureResult->fY2 - pCaptureResult->fY1));
                    if (cropFaceCloseup(faceRawRect,
                                        cv::Size(m_nWidth, m_nHeight),
                                        faceMat))
                    {
                        EventTvSdkImage_S stTarget;
                        if (encode_mat_to_tvsdk_image(faceMat, stTarget)) {
                            stCtxFaceCapture.stTargetImage = std::move(stTarget);
                        }
                    }
                }
            }
        }
        m_FaceCapturectStateMachine.handleAlarmState(stFcaeEventStatus.bFaceCapture, stCtxFaceCapture);
#else
        m_FaceCapturectStateMachine.handleAlarmState(stFcaeEventStatus.bFaceCapture, Event::Type_E::FACE_CAPTURE);
#endif
    }

    return;
}

int CFaceDetect::saveToDatebase(const std::string &strFilename,
                                const std::string &strCurrentDate,
                                const std::string &strCurrentTime)
{
    using namespace Db;
    Capture_NS::CaptureInfo_S stInfo;
    stInfo.nChnId = 0;
    stInfo.strImagePath = strFilename;
    stInfo.strStartTime = strCurrentDate + " " + strCurrentTime;
    stInfo.strEndTime = stInfo.strStartTime;
    stInfo.enType = Event::Type_E::FACE_CAPTURE;

    try
    {
        stInfo.nImageSize = std::filesystem::file_size(strFilename);
    } 
    catch (...)  // 文件不存在或错误
    {
        dlog_warn("文件不存在");
        stInfo.nImageSize = 0;
    }

    /* 添加图片信息至数据库表 */
    CCaptureDatabase::instance()->add(stInfo);

    /* 更新图片数量、总大小至数据库表 */
    Capture_NS::CaptureDirInfo_S stDirInfo;
    stDirInfo.nChnId = 0;
    /* 获取当前信息 */
    int nRet = CCaptureDatabase::instance()->get_itemInfo(stDirInfo);
    stDirInfo.nTotalSize += (long long) stInfo.nImageSize;
    stDirInfo.nCount++;

    if (nRet < 0)
    {
        /* 未创建表数据，进行添加 */
        CCaptureDatabase::instance()->add(stDirInfo);
    }
    else
    {
        /* 已经存在表数据，进行更新 */
        CCaptureDatabase::instance()->update(stDirInfo);
    }

    return 0;
}


void CFaceDetect::pushFaceCaptureInfo(FaceAttribute_NS::Result_S stFAResult,std::string strCurrentPicture,std::string strFacePicture)
{
    Alarm::FaceAlarmInfo_S stFaceAlarmInfo;
    /* 属性获取 */
    stFaceAlarmInfo.stFaceAlarmAttribute.bIsMale        = stFAResult.bIsMale;
    stFaceAlarmInfo.stFaceAlarmAttribute.nAgeLabel      = stFAResult.nAgeLabel;
    stFaceAlarmInfo.stFaceAlarmAttribute.bIsGlasses     = stFAResult.bIsGlasses;
    stFaceAlarmInfo.stFaceAlarmAttribute.bIsBeard       = stFAResult.bIsBeard;
    stFaceAlarmInfo.stFaceAlarmAttribute.bIsMask        = stFAResult.bIsMask;
    stFaceAlarmInfo.stFaceAlarmAttribute.nEmotionLabel  = stFAResult.nEmotionLabel;
    
    stFaceAlarmInfo.strCurrentPicture = strCurrentPicture;
    stFaceAlarmInfo.strFacePicture = strFacePicture;
    stFaceAlarmInfo.strTimeStamp = TimeUtils_NS::get_currentDateAndTimeNoT();

    if(SD_CARD_STATUS_E::NORMAL == CStorageManage::instance()->get_SdCardStatus())
    {
        stFaceAlarmInfo.bIsDownLoad = true;
    }
    else
    {
        stFaceAlarmInfo.bIsDownLoad = false;
    }

    dlog_debug("推送人脸抓拍信息:%s",Convert::to_string(stFaceAlarmInfo).c_str());
    TaskPublish::instance()->message(AC_PUSH_FACE_CAPTURE_INFO, Convert::to_string(stFaceAlarmInfo));
}

#ifdef ENABLE_TVSDK_SRC
static bool encode_capture_image(const cv::Mat &mat, BYTE *pDst, UINT32 &uDstLen, size_t nMaxLen)
{
    EventTvSdkImage_S stImage;
    if (mat.empty() || !encode_mat_to_tvsdk_image(mat, stImage, 85, false))
    {
        return false;
    }
    if (stImage.vecJpeg.empty())
    {
        return true;
    }
    if (stImage.vecJpeg.size() > nMaxLen)
    {
        return false;
    }
    std::memcpy(pDst, stImage.vecJpeg.data(), stImage.vecJpeg.size());
    uDstLen = static_cast<UINT32>(stImage.vecJpeg.size());
    return true;
}

static void fillFaceCaptureRegion(const cv::Rect2f &faceRect, const cv::Size &detectSize, const cv::Size &imageSize, Alarm::Region_S &stRegion)
{
    stRegion.nPointNum = 0;
    stRegion.aPoint.clear();
    if (faceRect.empty() || imageSize.width <= 0 || imageSize.height <= 0 || detectSize.width <= 0 || detectSize.height <= 0)
    {
        return;
    }
    float fScaleX = static_cast<float>(imageSize.width) / static_cast<float>(detectSize.width);
    float fScaleY = static_cast<float>(imageSize.height) / static_cast<float>(detectSize.height);
    float fX = faceRect.x * fScaleX;
    float fY = faceRect.y * fScaleY;
    float fW = faceRect.width * fScaleX;
    float fH = faceRect.height * fScaleY;
    float fNormX = 100.0f / static_cast<float>(imageSize.width);
    float fNormY = 100.0f / static_cast<float>(imageSize.height);
    stRegion.nPointNum = 4;
    stRegion.aPoint = {
        Common::PosF_S{ fX * fNormX, fY * fNormY },
        Common::PosF_S{ (fX + fW) * fNormX, fY * fNormY },
        Common::PosF_S{ (fX + fW) * fNormX, (fY + fH) * fNormY },
        Common::PosF_S{ fX * fNormX, (fY + fH) * fNormY }
    };
}

void CFaceDetect::pushFaceCaptureInfoToTvSdk(const cv::Mat &panoramaBgr, const cv::Rect2f &faceRect, const cv::Rect2f &rawFaceRect, const cv::Size &detectCoordinateSize, const FaceAttribute_NS::Result_S &stFAResult)
{
    NET_FaceCapturePushInfo_S stInfo;
    std::memset(&stInfo, 0, sizeof(stInfo));

    Alarm::FaceAlarmInfo_S stFaceAlarmInfo;
    stFaceAlarmInfo.stFaceAlarmAttribute.bIsMale        = stFAResult.bIsMale;
    stFaceAlarmInfo.stFaceAlarmAttribute.nAgeLabel      = stFAResult.nAgeLabel;
    stFaceAlarmInfo.stFaceAlarmAttribute.bIsGlasses     = stFAResult.bIsGlasses;
    stFaceAlarmInfo.stFaceAlarmAttribute.bIsBeard       = stFAResult.bIsBeard;
    stFaceAlarmInfo.stFaceAlarmAttribute.bIsMask        = stFAResult.bIsMask;
    stFaceAlarmInfo.stFaceAlarmAttribute.nEmotionLabel  = stFAResult.nEmotionLabel;
    stFaceAlarmInfo.strTimeStamp = TimeUtils_NS::get_currentDateAndTimeNoT();
    fillFaceCaptureRegion(faceRect, detectCoordinateSize, panoramaBgr.size(), stFaceAlarmInfo.stFaceRegion);
    TvSdkConvert::FillFaceCapturePushInfo(stFaceAlarmInfo, stInfo);

    if (!encode_capture_image(panoramaBgr, stInfo.byPanoramaImg, stInfo.uPanoramaImgLen, sizeof(stInfo.byPanoramaImg)))
    {
        dlog_warn("TVSDK人脸抓拍全景图编码失败或超上限");
        return;
    }

    cv::Mat faceMat;
    if (!cropFaceCloseup(rawFaceRect, detectCoordinateSize, faceMat))
    {
        dlog_warn("TVSDK人脸抓拍特写图裁剪失败");
        return;
    }
    if (!encode_capture_image(faceMat, stInfo.byFaceImg, stInfo.uFaceImgLen, sizeof(stInfo.byFaceImg)))
    {
        dlog_warn("TVSDK人脸抓拍特写图编码失败或超上限");
        return;
    }

    int nRet = ControlManage::instance()->tvsdk_push_alarm(NET_PUSH_FACE_CAPTURE_INFO, &stInfo, sizeof(stInfo));
    if (nRet < 0)
    {
        dlog_warn("TVSDK推送人脸抓拍失败: ret[%d]", nRet);
    }
    else
    {
        dlog_info("TVSDK推送人脸抓拍成功: cmd[%d] face[%u] panorama[%u]", NET_PUSH_FACE_CAPTURE_INFO, stInfo.uFaceImgLen, stInfo.uPanoramaImgLen);
    }
}
#endif

bool CFaceDetect::cropFaceCloseup(const cv::Rect2f &detectRect,
                                  const cv::Size &detectCoordinateSize,
                                  cv::Mat &outBgr)
{
    /* 优先从 4K 全分辨率帧裁剪特写图 */
    if (m_pFullNv12 && m_nFullWidth > 0 && m_nFullHeight > 0)
    {
        if (cropTargetImageFaceNv12(m_pFullNv12.get(), m_nFullWidth, m_nFullHeight,
                                    detectRect, detectCoordinateSize, outBgr))
        {
            return true;
        }
        dlog_warn("ai_app: 4K 特写裁剪失败, 回退 1080p 全景裁剪");
    }
    return cropTargetImageFace(m_fullRgbMat, detectRect, detectCoordinateSize, outBgr);
}

void CFaceDetect::addFaceOverlayInfo(const cv::Mat &image)
{
    /* 获取叠加信息 */
    Alarm::OverlayInfo_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);

    /* 左上角文字 */
    std::vector<std::string> vecUpLeftOverlay; 
    /* 左下角文字 */
    std::vector<std::string> vecBottomLeftOverlay; 
    /* 字体颜色大小 */
    int fontFace = cv::FONT_HERSHEY_SIMPLEX;
    double fontScale = 1.0;
    int thickness = 2;
    cv::Scalar color(0, 0, 0); 
    
    /* 黑色 */
    if(stInfo.enFontColor == Osd::OSD_COLOR_BLACK)
    {
        color = cv::Scalar(0, 0, 0);
    }
    /* 白色 */
    else if(stInfo.enFontColor == Osd::OSD_COLOR_WHITE)
    {
        color = cv::Scalar(255, 255, 255);
    }
    /* 自定义 */
    else
    {
        color = hexToScalar(stInfo.strFontColor);
    }
    
    /* 设备编号 */
    if(stInfo.bOverlayDeviceID)
    {
        vecUpLeftOverlay.push_back(/*"DeviceID:" + */std::to_string(stInfo.nDeviceID));
    }
    /* 监控点编号 */
    if(stInfo.bOverlayMonitoryPointInfo)
    {
        if(!stInfo.strMonitoryPointInfo.empty())
        {
            vecUpLeftOverlay.push_back(/* "MonitoryPoint:" + */stInfo.strMonitoryPointInfo);
        }
    }
    /* 抓拍时间 */
    if(stInfo.bOverlayCaptureTime)
    {
        std::string strTime = /*"CaptureTime:" + */TimeUtils_NS::get_currentDateAndTimeNoT();
        vecBottomLeftOverlay.push_back(strTime);
    }

    if(!vecUpLeftOverlay.empty())
    {   
        int lineHeight = 30; 
        cv::Point startPoint(10, 30);
        
        for (size_t i = 0; i < vecUpLeftOverlay.size(); ++i) 
        {
            cv::Point textPos(startPoint.x, startPoint.y + i * lineHeight);
            cv::putText(image, vecUpLeftOverlay[i], textPos, fontFace, fontScale, color, thickness);
        }
    }

    if(!vecBottomLeftOverlay.empty())
    {   
        int margin = 10;
        cv::Point origin(margin, image.rows - margin);

        cv::putText(image, vecBottomLeftOverlay[0], origin, fontFace, fontScale, color, thickness);
    }

}

#ifdef ENABLE_GAT1400_SRC
void CFaceDetect::pushFaceImageToGat1400(const cv::Mat &image, const cv::Mat &smallImage,
    const std::vector<FaceAttribute_NS::Result_S> &stFAResult,
    const FaceDetect_NS::Result_S &stFADetectResul)
{
    security_faces_t faces;
    security_face_t face;
    face.InfoKind = SecurityInfoType::Auto;
    if (!stFAResult.empty()) {
        /* 性别代码 1-男性、2-女性、0-未知性别、9-未说明性别 */
        face.GenderCode = stFAResult[0].bIsMale ? "1" : "2";
        /* 年龄上限、年龄下限 */
        int nAgeLabel = stFAResult[0].nAgeLabel;
        if (nAgeLabel >= 0 && nAgeLabel < g_vAgeLabelList.size()) {
            face.AgeLowerLimit = g_vAgeLabelList[nAgeLabel].first;
            face.AgeUpLimit = g_vAgeLabelList[nAgeLabel].second;
        }
        /* 眼镜款式 99-未知 */
        face.GlassStyle = stFAResult[0].bIsGlasses ? "99" : "";
        /* 眼睛颜色 99-未知 */
        face.GlassColor = stFAResult[0].bIsGlasses ? "99" : "";
        /* 胡型 */
        face.MustacheStyle = stFAResult[0].bIsBeard ? "留有胡子" : "";
        /* 口罩颜色 99-未知 */
        face.RespiratorColor = stFAResult[0].bIsMask ? "99" : "";
    }
    
    // 坐标点转换
    Common::PosF_S stPosition1 {stFADetectResul.fX1, stFADetectResul.fY1};
    Common::PosF_S stPosition2 {stFADetectResul.fX2, stFADetectResul.fY2};
    bool bConvert1 = stPosition1.ConvertResolution(m_nWidth, m_nHeight, image.cols, image.rows);
    bool bConvert2 = stPosition2.ConvertResolution(m_nWidth, m_nHeight, image.cols, image.rows);
    if (bConvert1 && bConvert2) {
        face.LeftTopX = stPosition1.fX;
        face.LeftTopY = stPosition1.fY;
        face.RightBtmX = stPosition2.fX;
        face.RightBtmY = stPosition2.fY;
    }

    /* 场景图 */
    if (!image.empty()) {
        security_subimage_info_t image_info;
        std::vector<uchar> buffer;
        if (!cv::imencode(".jpg", image, buffer)) {
            dlog_debug("jpeg 编码失败");
            return;
        }
        image_info.Data.resize(buffer.size());
        memcpy(&(image_info.Data)[0], buffer.data(), buffer.size());
        image_info.FileFormat = "Jpeg";
        image_info.Width = image.cols;
        image_info.Height = image.rows;
        image_info.Type = IMAGE_TYPE_SCENE;
        face.SubImageList.push_back(image_info);
    }

    /* 人脸图 */
    if (!smallImage.empty()) {
        security_subimage_info_t image_info;
        std::vector<uchar> buffer;
        if (!cv::imencode(".jpg", smallImage, buffer)) {
            dlog_debug("jpeg 编码失败");
            return;
        }
        image_info.Data.resize(buffer.size());
        memcpy(&(image_info.Data)[0], buffer.data(), buffer.size());
        image_info.FileFormat = "Jpeg";
        image_info.Width = smallImage.cols;
        image_info.Height = smallImage.rows;
        image_info.Type = IMAGE_TYPE_FACE;
        face.SubImageList.push_back(image_info);
    }

    faces.push_back(face);
    int status = GAT1400::CGAT1400::instance()->uploadFaces(faces);
    if (status) {
        dlog_debug("uploadFaces faild %d", status);
    }
}
#endif
