/**
 * @file LoiteringDetectV1_0.cpp
 * @author xiejh (xiejh@kfb.cn)
 * @date 2025-03-05
 * 
 * @brief 徘徊检测
 */
#include "LoiteringDetectV1_0.hpp"
#include "SaveImage.hpp"
#include "StatisticsTimer.hpp"

using namespace LoiteringDetect_NS;

LoiteringDetect_NS::CLoiteringDetectV1_0::CLoiteringDetectV1_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

LoiteringDetect_NS::CLoiteringDetectV1_0::~CLoiteringDetectV1_0()
{
    unInit();
}

/* 初始化 */
bool LoiteringDetect_NS::CLoiteringDetectV1_0::init()
{
    CStatisticsTimer runTime("徘徊检测初始化耗时");
    bool bRet = false;

    m_pYoloUltralytics = new Inference_NS::CYoloUltralytics(m_stInParam.strModelPath);
    if (m_pYoloUltralytics && m_pYoloUltralytics->init())
    {
        bRet = true;
    }

    if (!bRet)
    {
        printf("模型初始化失败 [%s]\n", m_stInParam.strModelPath.c_str());
        goto FAIL;
    }

    m_pByteTracker = new Inference_NS::cBYTETracker();
    if (m_pByteTracker)
    {
        m_pByteTracker->setValue(m_fTrackThresh, m_fHighThresh, m_fMatchThresh, m_nFrameId, m_nMaxTimeLost);
        bRet = true;
    }

    if (!bRet)
    {
        printf("跟踪算法初始化失败\n");
        goto FAIL;
    }

    return bRet;

FAIL:
    unInit();
    return false;
}

/* 反初始化 */
bool LoiteringDetect_NS::CLoiteringDetectV1_0::unInit()
{
    CStatisticsTimer runTime("徘徊检测反初始化耗时");
    if (m_pYoloUltralytics)
    {
        delete m_pYoloUltralytics;
        m_pYoloUltralytics = nullptr;
    }

    if (m_pByteTracker)
    {
        delete m_pByteTracker;
        m_pByteTracker = nullptr;
    }

    return true;
}


/* 处理数据 */
bool LoiteringDetect_NS::CLoiteringDetectV1_0::process(
    InData_S stInData,
    std::vector<Result_S> &vecResult,
    OutData_S *stOutData)
{
    OutData_S defaultOutData;

    /* 如果传入的指针为空，则使用默认对象 */
    if (stOutData == nullptr)
    {
        stOutData = &defaultOutData;
    }

    /* 初始化输出结果 */
    stOutData->validResult = false;
    stOutData->nType = -1;
    vecResult.clear();

    if (stInData.inMat.empty())
    {
        printf("传入图片为空\n");
        return false;
    }

    if (!m_pYoloUltralytics || !m_pByteTracker)
    {
        printf("未初始化算法类\n");
        return false;
    }

    bool bRet = m_pYoloUltralytics->setParam(stInData.stParam.fBoxThreshold, stInData.stParam.fNmsThreshold);
    if (!bRet)
    {
        printf("阈值参数设置错误，应该在0~1之间！！\n");
        return false;
    }
    
    if (m_stInParam.bDebug && !stInData.inMat.empty() && !m_stInParam.strOriginalDataPath.empty())
    {
        if (!Modules_NS::saveImage(stInData.inMat, m_stInParam.strOriginalDataPath))
        {
            printf("Debug-保存图片失败[%s]\n", m_stInParam.strOriginalDataPath.c_str());
        }
    }

    /* 推理+后处理 */
    Inference_NS::InputData_S stInputData;
    stInputData.pData = (float*)stInData.inMat.data;
    stInputData.nDataSize = static_cast<size_t>(stInData.inMat.total() * stInData.inMat.elemSize() * sizeof(float));
    stInputData.stBoxs.fConfidence = stInData.stParam.fBoxThreshold;
    stInputData.stBoxs.fNms = stInData.stParam.fNmsThreshold;
    
    std::vector<Inference_NS::BoxData_S> vBoxDatas;
    {
        CStatisticsTimer runTime("推理耗时");
        bRet = m_pYoloUltralytics->inference(stInputData, vBoxDatas);
        if (!bRet)
        {
            printf("算法分析失败\n");
            return false;
        }
    }

    const float scaleX = static_cast<float>(stInData.inMat.cols) / m_nLimitWidth;
    const float scaleY = static_cast<float>(stInData.inMat.rows * 2 / 3) / m_nLimitHeight;

    /* 跟踪算法 */
    std::vector<DetectResult_S> vecBoxs;
    for (const auto& box : vBoxDatas)
    {
        DetectResult_S result;
        result.fConfidence = box.fConfidence;
        result.nClassId = box.nLabel;
        result.vfBox = cv::Rect_<float>(box.stBoxs.nX1, box.stBoxs.nY1, 
                                       box.stBoxs.nX2 - box.stBoxs.nX1, 
                                       box.stBoxs.nY2 - box.stBoxs.nY1);
        vecBoxs.push_back(result);

        bool bAreaRet = false;
        std::vector<cv::Point> vRectPolygon = {cv::Point(box.stBoxs.nX1, box.stBoxs.nY1),
                                                cv::Point(box.stBoxs.nX2, box.stBoxs.nY1),
                                                cv::Point(box.stBoxs.nX2, box.stBoxs.nY2),
                                                cv::Point(box.stBoxs.nX1, box.stBoxs.nY2)};
        
        if (stInData.stParam.stLoiteringParam.bEnable)
        {
            bAreaRet = isIntersecting(vRectPolygon,
                                        stInData.stParam.stLoiteringParam.vecPoints);
        }

        if (bAreaRet)
        {
            Result_S stResult;
            stResult.nId = 0;
            stResult.fX = (float)box.stBoxs.nX1 * scaleX;
            stResult.fY = (float)box.stBoxs.nY1 * scaleY;
            stResult.fWidth = (float)(box.stBoxs.nX2 - box.stBoxs.nX1) * scaleX;
            stResult.fHeight = (float)(box.stBoxs.nY2 - box.stBoxs.nY1) * scaleY;

            vecResult.push_back(stResult);
        }
    }

    /* 跟踪算法 */
    {
        CStatisticsTimer runTime("跟踪+后处理耗时");
        std::vector<cSTrack> vecStracks = m_pByteTracker->update(vecBoxs);

        /* 获取徘徊检测参数 */
        int nLoiteringTimeThreshold = stInData.stParam.nLoiteringTimeThreshold;

        /* 分析结果 */
        /* 遍历当前帧检测到的目标 */
        bool bLoiteringDetected = false; /* 标记是否检测到徘徊 */
        for (const auto &track : vecStracks)
        {
            int nPersonId = track.track_id;
            cv::Point centerPoint(
                static_cast<int>(track.tlwh[0] + (track.tlwh[2] / 2)),
                static_cast<int>(track.tlwh[1] + (track.tlwh[3] / 2))); /* 目标中心点 */

            /* 检查目标是否已经在跟踪列表中 */
            if (m_mapPenson.count(nPersonId))
            {
                /* 更新目标的位置信息和状态 */
                m_mapPenson[nPersonId].startPoint = m_mapPenson[nPersonId].curPoint;
                m_mapPenson[nPersonId].curPoint = centerPoint;
                m_mapPenson[nPersonId].ndwellTime = 0;
                m_mapPenson[nPersonId].nexistTime++;  /* 增加停留时间 */
                m_mapPenson[nPersonId].isUsed = true; /* 标记为当前帧检测到 */
            }
            else
            {
                /* 新目标，添加到跟踪列表 */
                Penson_S newPenson;
                newPenson.nId = nPersonId;
                newPenson.startPoint = centerPoint;
                newPenson.curPoint = centerPoint;
                newPenson.ndwellTime = 0;
                newPenson.nexistTime = 1; /* 初始停留时间 */
                newPenson.isUsed = true;  /* 标记为当前帧检测到 */
                m_mapPenson[nPersonId] = newPenson;
            }

            /* 判断是否触发徘徊报警 */
            if (m_mapPenson[nPersonId].nexistTime >= nLoiteringTimeThreshold)
            {
                bLoiteringDetected = true; /* 标记检测到徘徊 */
            }
        }

        /* 清除丢失的目标 */
        for (auto it = m_mapPenson.begin(); it != m_mapPenson.end();)
        {
            int nPersonId = it->first;
            if (!m_mapPenson[nPersonId].isUsed)
            {
                m_mapPenson[nPersonId].ndwellTime++; /* 增加丢失时间 */
                if (m_mapPenson[nPersonId].ndwellTime > m_nMaxTimeLost)
                {
                    it = m_mapPenson.erase(it); /* 从跟踪列表中移除 */
                }
                else
                {
                    ++it;
                }
            }
            else
            {
                m_mapPenson[nPersonId].isUsed = false; /* 重置使用状态 */
                ++it;
            }
        }

        /* 如果检测到徘徊事件，设置输出结果 */
        if (bLoiteringDetected)
        {
            stOutData->nChnId = stInData.nChnId;
            stOutData->validResult = true;
            stOutData->nType = 10; /* 徘徊事件 Type_E::LOITER_DET */
        }
    }

    return true;
}

/* 处理数据 */
bool LoiteringDetect_NS::CLoiteringDetectV1_0::resizeAndPadImage(cv::Mat inputImage, cv::Mat &outputImage)
{
    int imageWidth = inputImage.cols;
    int imageHeight = inputImage.rows;
    m_fResizeScale = static_cast<float>(m_nLimitWidth) / std::max(imageWidth, imageHeight);

    // std::cout << "imageWidth：" << imageWidth << std::endl;
    // std::cout << "imageHeight：" << imageHeight << std::endl;
    // std::cout << "m_nLimitWidth：" << m_nLimitWidth << std::endl;
    // std::cout << "m_fResizeScale" << m_fResizeScale << std::endl;

    int newWidth = static_cast<int>(imageWidth * m_fResizeScale);
    int newHeight = static_cast<int>(imageHeight * m_fResizeScale);

    // std::cout << "计算后的缩放：" << newWidth << "x" << newHeight << std::endl;

    cv::Mat resizedImage;
    cv::resize(inputImage, resizedImage, cv::Size(newWidth, newHeight));

    cv::Mat output = cv::Mat::zeros(m_nLimitWidth, m_nLimitHeight, inputImage.type());

    m_nXOffset = static_cast<int>((m_nLimitWidth - newWidth) / 2);
    m_nYOffset = static_cast<int>((m_nLimitHeight - newHeight) / 2);

    resizedImage.copyTo(output(cv::Rect(m_nXOffset, m_nYOffset, newWidth, newHeight)));
    outputImage = output;

    return true;
}

bool LoiteringDetect_NS::CLoiteringDetectV1_0::resizeAndPadImage2(cv::Mat inputImage, cv::Mat &outputImage)
{
    int imageWidth = inputImage.cols;
    int imageHeight = inputImage.rows;
    m_fResizeScale = static_cast<float>(m_nLimitWidth) / std::max(imageWidth, imageHeight);

    int newWidth = static_cast<int>(imageWidth * m_fResizeScale);
    int newHeight = static_cast<int>(imageHeight * m_fResizeScale);

    cv::Mat resizedImage;
    cv::resize(inputImage, resizedImage, cv::Size(newWidth, newHeight));

    m_nXOffset = static_cast<int>((m_nLimitWidth - newWidth) / 2);
    m_nYOffset = static_cast<int>((m_nLimitHeight - newHeight) / 2);
    int nBottom = m_nLimitHeight - newHeight - m_nYOffset;
    int mRight = m_nLimitWidth - newWidth - m_nXOffset;

    cv::copyMakeBorder(resizedImage, outputImage,
                       m_nYOffset, nBottom, m_nXOffset, mRight, 
                       cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
    return true;
}

/* 判断两个多边形是否有交点 */
bool LoiteringDetect_NS::CLoiteringDetectV1_0::isIntersecting(
    std::vector<cv::Point> rectPolygon,
    std::vector<cv::Point> polygons)
{
    std::vector<cv::Point> intersectionPoints;
    int result = cv::intersectConvexConvex(rectPolygon, polygons, intersectionPoints);
    if (intersectionPoints.empty())
    {
        return false;
    }
    else
    {
        return true;
    }
}
