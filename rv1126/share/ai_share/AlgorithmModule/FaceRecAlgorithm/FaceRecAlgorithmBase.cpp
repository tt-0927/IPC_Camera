/*
 * @FilePath     : FaceRecAlgorithmBase.cpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2024-01-08 15:57:13
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-07-19 09:46:07
 * @Description  : 人脸识别模块基类
 */
#include "FaceRecAlgorithmBase.hpp"

#include <filesystem>

#include "dlog.h"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"

namespace fs = std::filesystem;

using namespace FR_NS;

static void freeMediaData(MediaDataInfo_S stMediaDataInfo)
{
    stMediaDataInfo.free();
}

CFaceRecAlgorithmBase::CFaceRecAlgorithmBase(FaceRecInParam_S stInParam)
    : m_stInParam(stInParam)
{
    if (nullptr == m_pFaceDataDB)
    {
        m_pFaceDataDB = new CFaceDataDB();
    }

    if (nullptr == m_pDataQueue)
    {
        m_pDataQueue = new CDataQueue<MediaDataInfo_S, std::list<FaceRecognitionResult_S>>(freeMediaData, nullptr, stInParam.stExParam.nMaxQueue);
    }

    /* 创建线程 */
    m_threadObj = std::thread(&CFaceRecAlgorithmBase::run, this);
}

CFaceRecAlgorithmBase::~CFaceRecAlgorithmBase()
{
    /* 结束线程 */
    m_bRunning.store(false);
    m_threadObj.join();

    if (m_pFaceDataDB)
    {
        delete m_pFaceDataDB;
        m_pFaceDataDB = nullptr;
    }

    if (m_pDataQueue)
    {
        delete m_pDataQueue;
        m_pDataQueue = nullptr;
    }
}

/* 发送分析数据 */
BlError_E CFaceRecAlgorithmBase::send_dataAnalysis(MediaDataInfo_S stMediaDataInfo)
{
    if (nullptr == stMediaDataInfo.pchData)
    {
        dlog(LOG_ERROR, "传入参数为空");
        return ERR_IN_PARAM_NULL;
    }

    if (nullptr == m_pDataQueue)
    {
        dlog(LOG_ERROR, "未初始化队列操作");
        return ERR_UNINIT;
    }

    BlError_E enRetCode = OK;

    enRetCode = m_pDataQueue->push_pendingQueue(stMediaDataInfo);
    return enRetCode;
}

/* 读取分析数据 */
BlError_E CFaceRecAlgorithmBase::read_analysisResult(std::list<FaceRecognitionResult_S>& listOutInfo)
{
    if (nullptr == m_pDataQueue)
    {
        dlog(LOG_ERROR, "未初始化队列操作");
        return ERR_UNINIT;
    }

    BlError_E enRetCode = OK;

    enRetCode = m_pDataQueue->pop_resultQueue(listOutInfo);
    return enRetCode;
}

/* 实时分析数据 */
BlError_E CFaceRecAlgorithmBase::realTime_dataAnalysis(MediaDataInfo_S stMediaDataInfo, std::list<FaceRecognitionResult_S>& listOutInfo)
{
    return dataAnalysis(stMediaDataInfo, listOutInfo);
}

/* 添加图片 */
BlError_E CFaceRecAlgorithmBase::addFaceImage(std::string strFilePath, std::string strName, int nUserId)
{
    if (nullptr == m_pFaceDataDB)
    {
        dlog(LOG_ERROR, "未初始化");
        return ERR_UNINIT;
    }

    BlError_E      enRetCode = OK;
    FaceDataInfo_S stInfo;
    stInfo.clear();

    /* 判断图片路径是否存在 */
    if (!fs::exists(strFilePath))
    {
        dlog(LOG_ERROR, "文件路径不存在 [%s]", strFilePath.c_str());
        return ERR_NOT_EXIST;
    }

    /* 判断文件是否为常规文件 */
    if (!fs::is_regular_file(strFilePath))
    {
        dlog(LOG_ERROR, "该文件不是常规文件 [%s]", strFilePath.c_str());
        return ERR_FILE_ERR;
    }

    /* 人名 */
    stInfo.strName    = strName;
    /* 图片路径 */
    stInfo.strPicPath = strFilePath;
    /* 获取图片文件名 */
    stInfo.strPicName = fs::path(strFilePath).filename().string();

    /*读取图像文件*/
    cv::Mat image = cv::imread(stInfo.strPicPath);
    if (image.empty())
    {
        /*图像为空的处理逻辑*/
        dlog(LOG_ERROR, "文件无法被读取，可能不是一张图片[%s]", stInfo.strPicPath.c_str());
        return ERR_FILE_ERR;
    }
    if (image.cols <= 0 || image.rows <= 0)
    {
        /*图像尺寸不符合条件的处理逻辑*/
        dlog(LOG_ERROR, "输入图像尺寸不符合条件[%s]", stInfo.strPicPath.c_str());
        return ERR_FILE_ERR;
    }

    stInfo.nCardId = nUserId;

    std::list<std::vector<float>> listOutData;
    listOutData.clear();

    MediaDataInfo_S stMediaDataInfo;
    stMediaDataInfo.pchData   = (char*)image.data;
    stMediaDataInfo.nDataSize = image.total();

    /* 提取特征数据 */
    enRetCode = get_facialFeatures(image, listOutData);
    if (enRetCode < OK)
    {
        dlog(LOG_ERROR, "提取特征数据失败");
        return enRetCode;
    }

    /*将新的人脸数据插入总库*/
    if (listOutData.size() <= 0)
    {
        dlog(LOG_ERROR, "该图片解析不到人脸[%s]", stInfo.strPicPath.c_str());
        return ERR_FILE_ERR;
    }

    if (listOutData.size() > 1)
    {
        dlog(LOG_ERROR, "该图片解析到多个[%ld]人脸[%s]", listOutData.size(), stInfo.strPicPath.c_str());
        return ERR_FILE_ERR;
    }

    stInfo.vfData = listOutData.front();

    /* 插入数据库 */
    enRetCode = m_pFaceDataDB->insertData(stInfo);
    if (enRetCode < OK)
    {
        dlog(LOG_ERROR, "插入数据库失败");
        return enRetCode;
    }

    return enRetCode;
}

/* 更新图片 */
BlError_E CFaceRecAlgorithmBase::uploadFaceImage(int nId, std::string strFilePath, std::string strName)
{
    if (nullptr == m_pFaceDataDB)
    {
        dlog(LOG_ERROR, "未初始化");
        return ERR_UNINIT;
    }

    BlError_E      enRetCode = OK;
    FaceDataInfo_S stInfo;
    stInfo.clear();

    /* 判断图片路径是否存在 */
    if (!fs::exists(strFilePath))
    {
        dlog(LOG_ERROR, "文件路径不存在 [%s]", strFilePath.c_str());
        return ERR_NOT_EXIST;
    }

    /* 判断文件是否为常规文件 */
    if (!fs::is_regular_file(strFilePath))
    {
        dlog(LOG_ERROR, "该文件不是常规文件 [%s]", strFilePath.c_str());
        return ERR_FILE_ERR;
    }

    /* 人名 */
    stInfo.strName    = strName;
    /* 图片路径 */
    stInfo.strPicPath = strFilePath;
    /* 获取图片文件名 */
    stInfo.strPicName = fs::path(strFilePath).filename().string();

    /*读取图像文件*/
    cv::Mat image = cv::imread(stInfo.strPicPath);
    if (image.empty())
    {
        /*图像为空的处理逻辑*/
        dlog(LOG_ERROR, "文件无法被读取，可能不是一张图片[%s]", stInfo.strPicPath.c_str());
        return ERR_FILE_ERR;
    }
    if (image.cols <= 0 || image.rows <= 0)
    {
        /*图像尺寸不符合条件的处理逻辑*/
        dlog(LOG_ERROR, "输入图像尺寸不符合条件[%s]", stInfo.strPicPath.c_str());
        return ERR_FILE_ERR;
    }

    /* 调整图片大小 */
    cv::resize(image, image, cv::Size(1920, 1024));

    std::list<std::vector<float>> listOutData;
    listOutData.clear();

    MediaDataInfo_S stMediaDataInfo;
    stMediaDataInfo.pchData   = (char*)image.data;
    stMediaDataInfo.nDataSize = image.total();

    /* 提取特征数据 */
    enRetCode = get_facialFeatures(stMediaDataInfo, listOutData);
    if (enRetCode < OK)
    {
        dlog(LOG_ERROR, "提取特征数据失败");
        return enRetCode;
    }
    /*将新的人脸数据插入总库*/
    if (listOutData.size() <= 0)
    {
        dlog(LOG_ERROR, "该图片解析不到人脸[%s]", stInfo.strPicPath.c_str());
        return ERR_FILE_ERR;
    }

    if (listOutData.size() > 1)
    {
        dlog(LOG_ERROR, "该图片解析到多个[%ld]人脸[%s]", listOutData.size(), stInfo.strPicPath.c_str());
        return ERR_FILE_ERR;
    }

    stInfo.vfData = listOutData.front();

    /* 更新数据库 */
    enRetCode = m_pFaceDataDB->updateData(nId, stInfo);
    if (enRetCode < OK)
    {
        dlog(LOG_ERROR, "更新数据库失败 [%d]", nId);
        return enRetCode;
    }

    return enRetCode;
}

/* 删除图片 */
BlError_E CFaceRecAlgorithmBase::deleteFaceImage()
{
    if (nullptr == m_pFaceDataDB)
    {
        dlog(LOG_ERROR, "未初始化");
        return ERR_UNINIT;
    }

    BlError_E enRetCode = OK;

    /* 删除数据 */
    enRetCode = m_pFaceDataDB->deleteData();
    if (enRetCode < OK)
    {
        dlog(LOG_ERROR, "删除数据失败");
        return enRetCode;
    }

    return enRetCode;
}

/* 删除图片 */
BlError_E CFaceRecAlgorithmBase::deleteFaceImage(int nId)
{
    if (nullptr == m_pFaceDataDB)
    {
        dlog(LOG_ERROR, "未初始化");
        return ERR_UNINIT;
    }

    BlError_E enRetCode = OK;

    /* 删除数据 */
    enRetCode = m_pFaceDataDB->deleteData(nId);
    if (enRetCode < OK)
    {
        dlog(LOG_ERROR, "删除数据失败 [%d]", nId);
        return enRetCode;
    }

    return enRetCode;
}

/* 获取图片信息 */
BlError_E CFaceRecAlgorithmBase::getFaceImageInfo(int nId, FaceDataInfo_S& stOutInfo)
{
    if (nullptr == m_pFaceDataDB)
    {
        dlog(LOG_ERROR, "未初始化");
        return ERR_UNINIT;
    }

    BlError_E enRetCode = OK;

    stOutInfo.clear();

    /* 查询数据 */
    enRetCode = m_pFaceDataDB->searchData(nId, stOutInfo);
    if (enRetCode < OK)
    {
        dlog(LOG_ERROR, "查询数据失败 [%d]", nId);
        return enRetCode;
    }

    return enRetCode;
}

/* 分页获取图片信息 */
BlError_E CFaceRecAlgorithmBase::getFaceImageInfo(
    int                        nCurPageNum,
    int                        nPageSize,
    std::list<FaceDataInfo_S>& listOutInfo,
    int&                       nOutTotal)
{
    if (nullptr == m_pFaceDataDB)
    {
        dlog(LOG_ERROR, "未初始化");
        return ERR_UNINIT;
    }

    BlError_E enRetCode = OK;

    listOutInfo.clear();

    /* 获取数据总数 */
    enRetCode = m_pFaceDataDB->getDataTotal(nOutTotal);
    if (enRetCode < OK)
    {
        dlog(LOG_ERROR, "查询数据库，数据总数失败");
        return enRetCode;
    }

    /* 查询数据 */
    enRetCode = m_pFaceDataDB->searchData(nCurPageNum, nPageSize, listOutInfo);
    if (enRetCode < OK)
    {
        dlog(LOG_ERROR, "查询数据失败 页码[%d] 页数[%d]", nCurPageNum, nPageSize);
        return enRetCode;
    }

    return enRetCode;
}

/* 分页获取图片信息 */
BlError_E FR_NS::CFaceRecAlgorithmBase::getFaceImageInfo(
    std::string                strSearchKey,
    int                        nCurPageNum,
    int                        nPageSize,
    std::list<FaceDataInfo_S>& listOutInfo,
    int&                       nOutTotal)
{
    if (nullptr == m_pFaceDataDB)
    {
        dlog(LOG_ERROR, "未初始化");
        return ERR_UNINIT;
    }

    BlError_E enRetCode = OK;

    listOutInfo.clear();

    /* 获取数据总数 */
    enRetCode = m_pFaceDataDB->getDataTotal(nOutTotal);
    if (enRetCode < OK)
    {
        dlog(LOG_ERROR, "查询数据库，数据总数失败");
        return enRetCode;
    }

    /* 查询数据 */
    enRetCode = m_pFaceDataDB->searchData(strSearchKey, nCurPageNum, nPageSize, listOutInfo);
    if (enRetCode < OK)
    {
        dlog(LOG_ERROR, "查询数据失败 页码[%d] 页数[%d]", nCurPageNum, nPageSize);
        return enRetCode;
    }

    return enRetCode;
}

/* 获取所有人脸信息 */
BlError_E FR_NS::CFaceRecAlgorithmBase::getFaceImageInfo(std::list<FaceDataInfo_S>& listOutInfo)
{
    if (nullptr == m_pFaceDataDB)
    {
        dlog(LOG_ERROR, "未初始化");
        return ERR_UNINIT;
    }

    BlError_E enRetCode = OK;

    listOutInfo.clear();

    /* 查询数据 */
    enRetCode = m_pFaceDataDB->getAllData(listOutInfo);
    if (enRetCode < OK)
    {
        dlog(LOG_ERROR, "获取全部数据失败");
        return enRetCode;
    }

    return enRetCode;
}

/* 获取所有人脸信息 不能加锁，内部函数调用的，会导致死锁 */
BlError_E CFaceRecAlgorithmBase::getAllFaceInfo(std::list<FaceDataInfo_S>& listOutInfo)
{
    if (nullptr == m_pFaceDataDB)
    {
        dlog(LOG_ERROR, "未初始化");
        return ERR_UNINIT;
    }

    BlError_E enRetCode = OK;

    listOutInfo.clear();

    /* 查询数据 */
    enRetCode = m_pFaceDataDB->getAllData(listOutInfo);
    if (enRetCode < OK)
    {
        dlog(LOG_ERROR, "获取全部数据失败");
        return enRetCode;
    }

    return enRetCode;
}

/* 线程函数 */
void CFaceRecAlgorithmBase::run()
{
    BlError_E enRetCode = OK;

    std::chrono::milliseconds sleepDuration(10);

    MediaDataInfo_S stPendingInfo;

    std::list<FaceRecognitionResult_S> listResultInfo;

    while (m_bRunning.load())
    {

        if (nullptr == m_pDataQueue)
        {
            /* 没有数据，等待ms */
            std::this_thread::sleep_for(sleepDuration);
            continue;
        }

        /* 取数据 */
        enRetCode = m_pDataQueue->pop_pendingQueue(stPendingInfo);
        if (enRetCode < OK)
        {
            /* 没有数据，等待ms */
            std::this_thread::sleep_for(sleepDuration);
            continue;
        }

        if (m_pDataQueue->getSize_pendingQueue() > 0)
        {
            dlog(LOG_INFO, "人脸识别待分析队列[%d]", m_pDataQueue->getSize_pendingQueue());
        }

        /* 进行分析 */
        enRetCode = dataAnalysis(stPendingInfo, listResultInfo);
        // if (enRetCode < OK)
        // {
        //     continue;
        // }

        /* 保存分析后的数据 */
        m_pDataQueue->push_resultQueue(listResultInfo);
    }
}
