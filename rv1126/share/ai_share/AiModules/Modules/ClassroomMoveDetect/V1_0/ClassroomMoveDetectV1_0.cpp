/*
 * @FilePath     : ClassroomMoveDetectV1_0.cpp
 * @Author       : 廖尔涛 liaoet@kfb.cn
 * @Date         : 2024-09-23 20:19:15
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-09-28 11:48:50
 * @Description  : 人少场景
 */
#include "ClassroomMoveDetectV1_0.hpp"

#include "HeadDetect.hpp"
#include "SaveImage.hpp"
#include <unistd.h>

/* 一组数据的大小 */
#define DATA_GROUP_SIZE1 6
#define DATA_GROUP_SIZE2 4

using namespace ClassroomMoveDetect_NS;

ClassroomMoveDetect_NS::CClassroomMoveDetectV1_0::CClassroomMoveDetectV1_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

ClassroomMoveDetect_NS::CClassroomMoveDetectV1_0::~CClassroomMoveDetectV1_0()
{
    unInit();
}

/* 初始化 */
bool ClassroomMoveDetect_NS::CClassroomMoveDetectV1_0::init()
{
    bool bRet = false;

    bRet = false;

    m_pHeadDetect = new Inference_NS::CHeadDetect(m_stInParam.strModelPath);
    if (m_pHeadDetect)
    {
        if (m_pHeadDetect->init())
        {
            bRet = m_pHeadDetect->getSizeLimit(
                0,
                m_nLimitWidth,
                m_nLimitHeight,
                m_nLimitChannel);
        }
    }

    if (!bRet)
    {
        printf("模型初始化失败 [%s]\n",
               m_stInParam.strModelPath.c_str());
        goto FAIL;
    }

    return bRet;

FAIL:

    unInit();

    return false;
}

/* 反初始化 */
bool ClassroomMoveDetect_NS::CClassroomMoveDetectV1_0::unInit()
{
    if (m_pHeadDetect)
    {
        delete m_pHeadDetect;
        m_pHeadDetect = nullptr;
    }

    return true;
}

/* 处理数据 */
bool ClassroomMoveDetect_NS::CClassroomMoveDetectV1_0::process(
    InData_S stInData,
    OutData_S &stOutData)
{
    if (stInData.inMat.empty())
    {
        printf("传入图片为空\n");
        return false;
    }

    bool bRet = true;

    bRet = m_pHeadDetect->setParam(stInData.stParam.fBoxThreshold, stInData.stParam.fNmsThreshold);
    if (!bRet)
    {
        printf("阈值参数设置错误，应该在0~1之间！！\n");
        return false;
    }

    std::vector<float> vecPos;
    vecPos.clear();

    if (m_stInParam.bDebug)
    {
        /* 保存图片 */
        if (!stInData.inMat.empty() && !m_stInParam.strOriginalDataPath.empty())
        {
            if (!Modules_NS::saveImage(stInData.inMat, m_stInParam.strOriginalDataPath))
            {
                printf("Debug-保存图片失败[%s]\n", m_stInParam.strOriginalDataPath.c_str());
            }
        }
    }

    /* 前处理 */
    if (stInData.inMat.channels() != m_nLimitChannel)
    {
        printf("模型需要的通道数和输入图片的通道数不一致 inMat[%d] != m_nLimitChannel[%d]\n",
               stInData.inMat.channels(),
               m_nLimitChannel);
        return false;
    }

    if (stInData.inMat.cols != m_nLimitWidth || stInData.inMat.rows != m_nLimitHeight)
    {
        resizeAndPadImage(stInData.inMat, stInData.inMat);
        // resizeAndPadImage2(stInData.inMat, stInData.inMat);
    }

    /* 推理+后处理 */
    bRet = m_pHeadDetect->inference(stInData.inMat, vecPos);
    if (!bRet)
    {
        printf("算法分析失败\n");
        return false;
    }

    if ((vecPos.size() % DATA_GROUP_SIZE1) != 0)
    {
        bRet = false;
        printf("算法推理结果异常\n");
        return false;
    }

    /* 坐标转换 */
    std::vector<std::vector<int>> v_nfOutputLocations;

    for (int nIndex = 0; nIndex < (int)vecPos.size(); nIndex++)
    {
        if ((int)vecPos.size() >= ((nIndex + 1) * DATA_GROUP_SIZE1))
        {
            int x1 = static_cast<int>((vecPos[nIndex * DATA_GROUP_SIZE1 + 0] - m_nXOffset) / m_fResizeScale);
            int y1 = static_cast<int>((vecPos[nIndex * DATA_GROUP_SIZE1 + 1] - m_nYOffset) / m_fResizeScale);
            int x2 = static_cast<int>((vecPos[nIndex * DATA_GROUP_SIZE1 + 2] - m_nXOffset) / m_fResizeScale);
            int y2 = static_cast<int>((vecPos[nIndex * DATA_GROUP_SIZE1 + 3] - m_nYOffset) / m_fResizeScale);

            std::vector<int> v_nToAdd = {x1, y1, x2, y2};
            v_nfOutputLocations.push_back(v_nToAdd);
        }
    }

    /* 判断上一帧目标框是否为空 */
    if (!v_nForeboxs.empty())
    {
        double fMoveResult = iou_filter(v_nfOutputLocations, v_nForeboxs, stInData.stParam.m_fIouFilterThreshold);
        stOutData.fDegreeMobility = fMoveResult;
    }
    stOutData.nChnId = stInData.nChnId;

    /* 替换上一帧的目标框 */
    v_nForeboxs = v_nfOutputLocations;

    return true;
}

/* 处理数据 */
bool ClassroomMoveDetect_NS::CClassroomMoveDetectV1_0::resizeAndPadImage(cv::Mat inputImage, cv::Mat &outputImage)
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

bool ClassroomMoveDetect_NS::CClassroomMoveDetectV1_0::resizeAndPadImage2(cv::Mat inputImage, cv::Mat &outputImage)
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

double ClassroomMoveDetect_NS::CClassroomMoveDetectV1_0::CalculateOverlap(
    const std::vector<int> &box1, 
    const std::vector<int> &box2)
{
    // 计算交集区域的宽和高
    int inter_width = fmax(0, fmin(box1[2], box2[2]) - fmax(box1[0], box2[0]));
    int inter_height = fmax(0, fmin(box1[3], box2[3]) - fmax(box1[1], box2[1]));
    // 交集面积
    int inter_area = inter_width * inter_height;
    // 计算两个框的面积
    int box1_area = (box1[2] - box1[0]) * (box1[3] - box1[1]);
    int box2_area = (box2[2] - box2[0]) * (box2[3] - box2[1]);
    // 并集面积
    int union_area = box1_area + box2_area - inter_area;

    return (union_area != 0) ? static_cast<double>(inter_area) / union_area : 0.0;
}

double ClassroomMoveDetect_NS::CClassroomMoveDetectV1_0::iou_filter(
    std::vector<std::vector<int>> &boxes1, 
    std::vector<std::vector<int>> &boxes2,
    double IouFilterThreshold)
{
    std::cout << "====================== " << std::endl;
    int size1 = boxes1.size();
    int size2 = boxes2.size();
    // std::cout << "当前帧的人头数: " << size1 << std::endl;
    // std::cout << "上一帧的人头数:: " << size2 << std::endl;
    // 标记数组，用于记录哪些框已经被移除
    std::vector<bool> remove_flag1(size1, false);
    std::vector<bool> remove_flag2(size2, false);
    // 遍历两个列表的矩形框
    for (int i = 0; i < size1; ++i) {
        if (remove_flag1[i]) continue;  // 如果框已经被标记为移除，跳过

        for (int j = 0; j < size2; ++j) {
            if (remove_flag2[j]) continue;  // 如果框已经被标记为移除，跳过

            // 计算IOU
            double iou = CalculateOverlap(boxes1[i], boxes2[j]);
            // std::cout << "iou: " << iou << std::endl;

            if (iou > IouFilterThreshold) {
                // 如果IOU大于阈值，标记这两个框为移除
                remove_flag1[i] = true;
                remove_flag2[j] = true;
                break;  // 找到匹配后，跳出内层循环，避免不必要的计算
            }
        }
    }
    // 计算剩余未被移除的框数量
    int remaining1 = std::count(remove_flag1.begin(), remove_flag1.end(), false);
    int remaining2 = std::count(remove_flag2.begin(), remove_flag2.end(), false);
    // std::cout << "当前帧未被移除的框数量: " << remaining1 << std::endl;
    // std::cout << "上一帧未被移除的框数量: " << remaining2 << std::endl;
    // 返回过滤后的比例
    double fIouFilterReturn = static_cast<double>(remaining1 + remaining2) / (size1 + size2);
    // std::cout << "过滤后的比例: " << fIouFilterReturn << std::endl;

    return fIouFilterReturn;
}
