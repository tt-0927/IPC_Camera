/*
 * @FilePath     : FaceRecAlgorithmBase.hpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2024-01-08 15:07:12
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-07-19 09:59:29
 * @Description  : 人脸识别模块基类
 */
#pragma once

#include <atomic>
#include <mutex>
#include <queue>
#include <thread>

#include "BlError.h"
#include "DataQueue.hpp"
#include "FaceDataDB.hpp"
#include "FaceRecExtern.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"

namespace FR_NS
{
    class CFaceRecAlgorithmBase
    {
    public:

        CFaceRecAlgorithmBase(FaceRecInParam_S stInParam);

        virtual ~CFaceRecAlgorithmBase();

    public:

        /**
         * @brief 发送分析数据
         * @param [MediaDataInfo_S] stMediaDataInfo: 媒体数据信息
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note 如果设置了stMediaDataInfo.freeDataFunc; 内部使用完会自动释放stMediaDataInfo.pchData。
         * @note 非阻塞调用
         */
        BlError_E send_dataAnalysis(MediaDataInfo_S stMediaDataInfo);

        /**
         * @brief 读取分析数据
         * @param [std::list<FaceRecognitionResult_S>&] listOutInfo: 分析结果链表
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note
         */
        BlError_E read_analysisResult(std::list<FaceRecognitionResult_S>& listOutInfo);

        /**
         * @brief 实时分析数据
         * @param [MediaDataInfo_S] stMediaDataInfo: 媒体数据信息
         * @param [std::list<FaceRecognitionResult_S>&] listOutInfo: 分析结果链表
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note 如果设置了stMediaDataInfo.freeDataFunc; 内部使用完会自动释放stMediaDataInfo.pchData。
         * @note 阻塞调用
         */
        BlError_E realTime_dataAnalysis(MediaDataInfo_S stMediaDataInfo, std::list<FaceRecognitionResult_S>& listOutInfo);

        /**
         * @brief 添加图片
         * @param [string] strFilePath: 图片路径
         * @param [string] strName: 人名
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note
         */
        BlError_E addFaceImage(std::string strFilePath, std::string strName, int nUserId);

        /**
         * @brief 更新图片
         * @param [int] nId: 图片ID
         * @param [string] strFilePath: 图片路径
         * @param [string] strName: 人名
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note
         */
        BlError_E uploadFaceImage(int nId, std::string strFilePath, std::string strName);

        /**
         * @brief 删除图片
         * @param [int] nId: 图片ID
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note
         */
        BlError_E deleteFaceImage(int nId);
        BlError_E deleteFaceImage();

        /**
         * @brief 获取图片信息
         * @param [int] nId: 图片ID
         * @param [FaceDataInfo_S&] stOutInfo: 该图片的人脸信息
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note
         */
        BlError_E getFaceImageInfo(int nId, FaceDataInfo_S& stOutInfo);


        /**
         * @brief 分页获取图片信息
         * @param [int] nCurPageNum: 获取第几页，1开始
         * @param [int] nPageSize: 每页数量
         * @param [list<FaceDataInfo_S>] listOutInfo: 图片的人脸信息链表
         * @param [int&] nOutTotal: 数据库总量
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note
         */
        BlError_E getFaceImageInfo(
            int                        nCurPageNum,
            int                        nPageSize,
            std::list<FaceDataInfo_S>& listOutInfo,
            int&                       nOutTotal);

        /**
         * @brief 分页获取图片信息
         * @param [string] strSearchKey: 搜索关键字
         * @param [int] nCurPageNum: 获取第几页，1开始
         * @param [int] nPageSize: 每页数量
         * @param [list<FaceDataInfo_S>] listOutInfo: 图片的人脸信息链表
         * @param [int&] nOutTotal: 数据库总量
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note
         */
        BlError_E getFaceImageInfo(
            std::string                strSearchKey,
            int                        nCurPageNum,
            int                        nPageSize,
            std::list<FaceDataInfo_S>& listOutInfo,
            int&                       nOutTotal);

        /**
         * @brief 获取所有人脸信息
         * @param [list<FaceDataInfo_S>] listOutInfo: 图片的人脸信息链表
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note
         */
        BlError_E getFaceImageInfo(std::list<FaceDataInfo_S>& listOutInfo);

    protected:

        /**
         * @brief 获取所有人脸信息
         * @param [list<FaceDataInfo_S>] listOutInfo: 图片的人脸信息链表
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note 不能加锁，内部函数调用的，会导致死锁
         */
        BlError_E getAllFaceInfo(std::list<FaceDataInfo_S>& listOutInfo);

        /**
         * @brief 获取图片中的人脸特征信息
         * @param [MediaDataInfo_S] stMediaDataInfo: 媒体数据信息
         * @param [std::list<std::vector<float>>&] listOutData: 解析到的人脸链表
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note 不会释放stMediaDataInfo空间。
         */
        virtual BlError_E get_facialFeatures(MediaDataInfo_S stMediaDataInfo, std::list<std::vector<float>>& listOutData) = 0;
        virtual BlError_E get_facialFeatures(cv::Mat inMat, std::list<std::vector<float>>& listOutData)                   = 0;
        /**
         * @brief 分析数据
         * @param [MediaDataInfo_S] stMediaDataInfo: 媒体数据信息
         * @param [std::list<FaceRecognitionResult_S>&] listOutInfo: 分析后的结果信息
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note 如果设置了stMediaDataInfo.freeDataFunc; 内部使用完会自动释放stMediaDataInfo.pchData。
         */
        virtual BlError_E dataAnalysis(MediaDataInfo_S stMediaDataInfo, std::list<FaceRecognitionResult_S>& listOutInfo)  = 0;

    private:

        /**
         * @brief 线程函数
         * @return [*] 无
         * @note
         */
        void run();


    protected:

        /* 传入参数 */
        FaceRecInParam_S m_stInParam;

    private:

        /* 数据库操作实例 */
        CFaceDataDB* m_pFaceDataDB = nullptr;

        /* 线程相关 */
        std::thread       m_threadObj;
        std::atomic<bool> m_bRunning = { true };

        /* 使用模板类操作队列 */
        CDataQueue<MediaDataInfo_S, std::list<FaceRecognitionResult_S>>* m_pDataQueue = nullptr;
    };

}    // namespace FR_NS
