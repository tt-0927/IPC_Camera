/*
 * @Author: leiyy leiyy@kfb.cn
 * @Date: 2025-11-28 09:55:45
 * @LastEditors: leiyy leiyy@kfb.cn
 * @LastEditTime: 2026-06-10 16:33:39
 * @FilePath: /rv1126_8814T/rv1126b_ipc/main_app/ai_app/common/common_cv.hpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
/**
 * @FilePath     : common_vlm.hpp
 * @Author       : zhouzirui
 * @Date         : 2025-11-28 9:02:10
 * @LastEditors  : leiyy
 * @LastEditTime : 2025-11-28 14:40:56
 * @Description  : visionText 公共处理函数(OpenCV处理相关)
 */

#pragma once

#include <iostream>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <ctime>
#include <filesystem>
#include <unistd.h>
#include <sys/resource.h>
#include <malloc.h>
#include "Json.h"
#include "event_configure.h"
#include "action_code.h"
#include "event_configure.h"
#include "convert_interface.h"
#include "result_manager.hpp"
#include <opencv2/opencv.hpp>
#include "common_process.h"

namespace fs = std::filesystem;

// 将录像克隆到备份目录,空间不足清理阈值
//低水位线（300M 触发清理）
#define MIN_FREE_SPACE_MB 300
//高水位线（清理到 500M 停止）
#define SAFE_FREE_SPACE_MB 500
constexpr uintmax_t MIN_FREE_SPACE_BYTES = MIN_FREE_SPACE_MB * 1024ULL * 1024ULL;
constexpr uintmax_t SAFE_FREE_SPACE_BYTES = SAFE_FREE_SPACE_MB * 1024ULL * 1024ULL;

/* 智能场景分析大小 */
#define MODEL_INPUT_WIDTH 448
#define MODEL_INPUT_HEIGHT 448

class CmProcess
{

public:

    // 获取单例的静态方法
    static CmProcess& instance()
    {
        // 保证线程安全的，且只初始化一次
        static CmProcess s_instance; 
        return s_instance;
    }

    // 禁止拷贝和赋值
    CmProcess(const CmProcess&) = delete;
    void operator=(const CmProcess&) = delete;


    /**
    * @brief 保存yuv数据为指定格式图片
    * @param pSrcData [in] 指向YUV(NV12)数据的指针
    * @param filename [in] 图片文件名+路径
    * @return 0表示成功, -1表示失败
    */
    int saveImage(const unsigned char* pSrcData,std::string& filename);

    /**
    * @brief YUV数据转换为模型所需的RGB输入格式
    * @param pSrcData [in] 指向YUV(NV12)数据的指针
    * @param Desframe [out] 经过预处理后的目标cv::Mat，可直接用于模型推理
    * @return 0表示成功, -1表示失败
    */
    int yuv_convert_rgb(const unsigned char* pSrcData, cv::Mat& Desframe);

    /**
    * @brief 本地图片转换为模型所需的RGB输入格式
    * @param path [in] 本地图片路径
    * @param Desframe [out] 经过预处理后的目标cv::Mat，可直接用于模型推理
    * @return 0表示成功, -1表示失败
    */
    int loadEncodeImage(const std::string& path, cv::Mat& image);

    /**
    * @brief 查找并物理复制最近的视频片段到AI备份目录
    * @param recordDir 录制原始根目录路径（通常为 RECORD_PATH）
    * @param createTime 目标时间点字符串（格式：YYYYMMDD_HHMMSS）
    * @param targetDir AI专用备份目标目录（通常为 AI_VIDEO_BACKUP_PATH）
    * @return std::string 成功则返回物理备份后的新文件完整路径，失败则返回空字符串
    */
    std::string copyClosestTSFile(const std::string& recordDir, const std::string& createTime, const std::string& targetDir);

    /**
    * @brief 物理删除磁盘上的图像文件及关联的AI备份视频文件
    * @param imagePath 待删除的图像文件完整路径
    * @param videoPath 待删除的备份视频文件完整路径
    * @note 函数内部会执行安全检查，仅允许删除包含关键词 "ai_video_backup" 的视频文件
    */
    void removePhysicalFiles(const std::string& imagePath, const std::string& videoPath);

    /**
    * @brief 将时间字符串从 "YYYYMMDD_HHMMSS" 格式转换为 "YYYY/MM/DD HH:MM:SS" 格式
    */
    std::string convertTimeFormat(const std::string& strRecordCreateTime);

    /**
    * @brief 保存检测到报警时的图片
    * @param pImageData 图片数据
    * @param nImageSize 图片大小
    * @param bTextSave true-保存图片到文字预设，false-保存到画面分析路径
    * @return 保存的图片路径，失败返回空字符串
    */
    std::string save_Detection_Image(const unsigned char* pImageData, size_t nImageSize,bool bTextSave);

    /**
    * @brief 安全地将 JPEG 内存数据保存到磁盘
    * @param jpegData [in] JPEG 字节流
    * @param bTextSave [in] true-保存到文字预设路径，false-保存到画面分析路径
    * @return 保存后的完整路径，失败返回空字符串
    */
    std::string save_Jpeg_Buffer_Safe(const std::vector<unsigned char>& jpegData, bool bTextSave);


    
private:

    // 私有构造函数
    CmProcess() = default; 

    /**
    * @brief 填充正方形 + 缩放，且复用内存
    * @param src 输入图像
    * @param dst 输入图像
    * @return 返回一个填充后的正方形图像
    */
    int make_square_and_resize(const cv::Mat& src, cv::Mat& dst);


private:

    std::mutex m_processMutex;

    //  内存缓存池
    cv::Mat m_cacheNv12;      // 用于 saveImage
    cv::Mat m_cacheRgb;       // 通用 RGB 缓存
    cv::Mat m_cacheBgr;       // 通用 BGR 缓存
    cv::Mat m_cacheSquare;    // 通用正方形缓存
    cv::Mat m_cacheLoaded;    // 用于 loadEncodeImage 读取文件
    cv::Mat m_cacheRgbMat;
    cv::Mat m_cacheSquareMat;

};