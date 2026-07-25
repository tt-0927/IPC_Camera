/**
 * @file face_detect.hpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-11-19
 * 
 * @brief 人脸检测相关
 */

 #pragma once

 #include <atomic>
 #include <chrono>
 #include <condition_variable>
 #include <mutex>
 #include <thread>
 #include <algorithm>
 #include <sys/time.h>
 #include "common_process.h"
 #include "blocking_queue.hpp"
 #include "stream_video.h"
 #include "stream_vpss.h"
 #include <opencv2/opencv.hpp>
 #include "event_manager.hpp"
 #include "stream_process_ext.hpp"
#include "algorithm.hpp"
#include "task_publish.h"
#include "event_define.h"
#include "FaceDetectV3_0.hpp"
#include "FaceQualityAssessmentV1_0.hpp"
#include "FaceAttributeV1_0.hpp"

typedef struct FcaeEventStatus
{
    bool bFaceDetect = false;
    bool bFaceCapture = false;
}FcaeEventStatus_t;

class CFaceDetect : public CAlgorithm
{
public:

    CFaceDetect();
    ~CFaceDetect();

    /**
     * @brief 接受媒体数据
     * @param [MediaData_S] stMediaData:
     * @return [*]
     * @note
     */
    void recvMediaData(MediaData_S stMediaData) override;
    
    /**
     * @brief 更新算法配置参数
     * @param stAlgoConfig 
     */
    void setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig);
    void setAlgoParamCfg(const Alarm::FaceDetection_S &stAlgoCfg,Event::Type_E enType = Event::Type_E::FACE_DETECT);
    void setAlgoParamCfg(const Alarm::FaceCapture_S &stAlgoCfg,Event::Type_E enType = Event::Type_E::FACE_CAPTURE);

private:

    /**
     * @brief 初始化
     * @return [*]
     * @note
     */
    bool init();

    /**
     * @brief 反初始化
     * @return [*]
     * @note
     */
    bool unInit();
    /**
     * @brief 线程函数
     * @return [*]
     * @note
     */
    void run();

    void deleteRun();
    /**
     * @brief   : 检查目标是否在区域内
     * @param   : stRegion 区域定义
     * @param   : stResult 目标框
     * @return  : bool 是否在区域内
     */
    bool is_in_region(const Alarm::Region_S &stRegion, const FaceDetect_NS::Result_S &stResult);

    /**
     * @brief 人脸抓拍处理函数
     * @param stResult 
     * @return true 
     * @return false 
     */
    bool processFaceCapture(const FaceDetect_NS::Result_S &stResult);

     /**
     * @brief   : 保存人脸全景大图
     * @param    {cv::Mat} *image：当前检测帧
     * @param    {std::string} strFaceAttribute：人脸属性
     * @return   {std::string} 图片路径
     */
    std::string saveFacePanoramicImage(const cv::Mat &image,std::string strFaceAttribute);

    /**
     * @brief   : 保存人脸目标小图
     * @param    {cv::Mat} *image：当前检测帧
     * @param    {std::string} strFaceAttribute: 人脸属性
     * @return   {std::string} 图片路径
     */
    std::string saveFaceImage(const cv::Mat &image,std::string strFaceAttribute);
    /**
     * @brief   : 保存信息至抓图图片数据库
     * @note    : 保存了图片需要更新信息至数据库
     * @param    {string} &strFilename 图片名
     * @param    {string} &strCurrentDate 图片当前日期
     * @param    {string} &strCurrentTime 图片当前时间
     * @return   {int} 0：成功 非零：失败
     */
    int saveToDatebase(const std::string &strFilename,
                       const std::string &strCurrentDate,
                       const std::string &strCurrentTime);
    /**
     * @brief 推送人脸抓拍信息
     * @param stFDResult 人脸检测信息
     * @param stFAResult 人脸属性信息
     */
    void pushFaceCaptureInfo(FaceAttribute_NS::Result_S stFAResult,std::string strCurrentPicture,std::string strFacePicture);

    /**
     * @brief 添加人脸叠加信息
     * @param image 
     */
    void addFaceOverlayInfo(const cv::Mat &image);

    void processFaceEvent(const FcaeEventStatus_t &stFcaeEventStatus);

#ifdef ENABLE_GAT1400_SRC
    /**
     * @description  : 推送人脸图片到Gat1400平台
     * @param         {vector<cv::Mat>} &vImageList
     * @param         {vector<FaceAttribute_NS::Result_S>} &stFAResult
     * @param         {Result_S} &stFADetectResult
     * @return        {*}
     */
    void pushFaceImageToGat1400(const cv::Mat &image, const cv::Mat &smallImage,
        const std::vector<FaceAttribute_NS::Result_S> &stFAResult,
        const FaceDetect_NS::Result_S &stFADetectResult);
#endif

private:
    /* 人脸检测句柄 */
    FaceDetect_NS::CFaceDetectV3_0* m_pFaceDetectHandle = nullptr;
    /* 人脸质量句柄 */
    FaceQualityAssessment_NS::CFaceQualityAssessmentV1_0* m_pFaceQuaHandle = nullptr;
    /* 人脸属性句柄 */
    FaceAttribute_NS::CFaceAttributeV1_0* m_pFaceAttribute = nullptr;
    /* 队列 */
    BQ_NS::CBlockingQueue<MediaData_S> m_dateQueue;

    /* 用于控制线程的运行 */
    std::atomic<bool>       m_bRunning;
    /* 互斥锁保护指针访问 */
    std::mutex              m_mutex;
    /* 条件变量 */
    std::condition_variable m_condition;
    /* 数据获取线程 */
    std::thread             m_thread;

    
    /**
     * @brief 删除抓拍图片队列相关
     */
     BQ_NS::CBlockingQueue<std::string> m_deleteQueue;
    /* 用于控制线程的运行 */
    std::atomic<bool>       m_bDeleteRunning;
    /* 互斥锁保护指针访问 */
    std::mutex              m_deleteMutex;
    /* 条件变量 */
    std::condition_variable m_deleteCondition;
    /* 数据获取线程 */
    std::thread             m_deleteThread;

    /* 人脸侦测相关配置 */
    Alarm::FaceDetection_S m_stAlgoFaceDetectCfg;
    /* 人脸抓拍相关配置 */
    Alarm::FaceCapture_S m_stAlgoFaceCapCfg;

    /* 是否开启人脸属性分析 */
    std::atomic<bool> m_bFaceAttribute;

    /* 检测频率控制 */
    EventManager m_RecvManager{1000 * 2};

    /* 报警状态管理 */
    CAlarmStateMachine m_FaceDetectStateMachine;         /* 人脸识别 */
    CAlarmStateMachine m_FaceCapturectStateMachine;      /* 人脸抓拍 */

    /* 算法默认最小瞳距 */
    int m_nMinIpd = 20;    
    /* 算法最小瞳距范围 */
    const int MIN_IPD = 0;
    const int MAX_IPD = 75;
    /* 人脸识别算法默认分辨率 */
    int m_nWidth = 640;
    int m_nHeight = 640;

    /* 人脸质量算法分辨率 */
    int m_nFQWidth = 112;
    int m_nFQHeight = 112;

    /* 人脸属性算法分辨率 */
    int m_nFAWidth = 192;
    int m_nFAHeight = 192;

    /* 人脸保存分辨率 */
    int m_nFaceSaveWidth = 212;
    int m_nFaceSaveHeight = 212;

    /* 是否联动保存人脸全景图片 */
    bool m_bIsLinkageFacePanoramicImage = false;
    /* 是否联动保存人脸图片 */
    bool m_bIsLinkageFaceImage = false;
    /* 是否联动邮件 */
    bool m_bEmail = false;
    /* 是否联动上传SD卡 */
    bool m_bUploadSdCard = false;
    /* 人脸抓拍叠加信息 */
    Alarm::OverlayInfo_S m_overlayInfo;

    /* 上一帧人脸检测结果 */
    std::vector<FaceDetect_NS::Result_S> m_vstLastFrameResult;
    
    int m_nFrameCount = 0;
    // CAlarmStateMachine m_FaceDetectStateMachine;         /* 人脸侦测 */
};
