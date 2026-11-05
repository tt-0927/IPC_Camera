/*
 * @FilePath: rv1126b_ipc/main_app/ai_app/algorithm_mode/group4_detect/group4_detect.cpp
 * @Description: cigarette(香烟)、sleep(睡觉)、phone(玩手机)、fall(摔跤)、falling(摔跤中)
 *
 * 全盘移植自 share/ai_share/AiModules/Modules/Group4Detect/V3_0/Group4DetectV3_0.cpp
 */

#include "BYTETracker.h"
#include "group4_detect.hpp"

#include <unistd.h>

#include "dlog.h"

using namespace Group4Detect_NS;

static void pointScaleUpV3(float& x1, float& y1, float& x2, float& y2, int max_w, int max_h, double scaleFactor)
{
    float width = x2 - x1;
    float height = y2 - y1;

    float newWidth = static_cast<float>(width * scaleFactor);
    float newHeight = static_cast<float>(height * scaleFactor);

    /* 新的左上角坐标, 确保新矩形不会超出图像边界 */
    x1 = std::max(0.0f, x1 - (newWidth - width) / 2);
    y1 = std::max(0.0f, y1 - (newHeight - height) / 2);

    x2 = std::min(x1 + newWidth, (float)max_w);
    y2 = std::min(y1 + newHeight, (float)max_h);
}

/* 内联实现坐标还原（替代 Modules_NS::OffsetScaRestor） */
static void offsetScaRestor(std::vector<Inference_NS::BoxData_S>& vecBox, int scaWidth, int scaHeight, int oriWidth, int oriHeight)
{
    float fResizeScale = std::min(
        static_cast<float>(scaWidth) / static_cast<float>(oriWidth),
        static_cast<float>(scaHeight) / static_cast<float>(oriHeight));

    int newWidth  = static_cast<int>(oriWidth * fResizeScale);
    int newHeight = static_cast<int>(oriHeight * fResizeScale);

    int nXOffset = (scaWidth - newWidth) / 2;
    int nYOffset = (scaHeight - newHeight) / 2;

    for (auto &stBox : vecBox)
    {
        stBox.stBoxs.nX1 = static_cast<float>(std::max(0, ((int)stBox.stBoxs.nX1 - nXOffset)) / fResizeScale);
        stBox.stBoxs.nY1 = static_cast<float>(std::max(0, ((int)stBox.stBoxs.nY1 - nYOffset)) / fResizeScale);
        stBox.stBoxs.nX2 = static_cast<float>(std::max(0, ((int)stBox.stBoxs.nX2 - nXOffset)) / fResizeScale);
        stBox.stBoxs.nY2 = static_cast<float>(std::max(0, ((int)stBox.stBoxs.nY2 - nYOffset)) / fResizeScale);
    }
}

Group4Detect_NS::CGroup4Detect::CGroup4Detect(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

Group4Detect_NS::CGroup4Detect::~CGroup4Detect()
{
    unInit();
}

/* 初始化 */
bool Group4Detect_NS::CGroup4Detect::init()
{
    bool bRet = false;

    m_pYoloUltralytics = new Inference_NS::CYoloUltralytics(m_stInParam.strModelPath);
    if (m_pYoloUltralytics && m_pYoloUltralytics->init())
    {
        m_pYoloUltralytics->getSizeLimit(
                0,
                m_nLimitWidth,
                m_nLimitHeight,
                m_nLimitChannel);
        bRet = true;
    }

    if (!bRet)
    {
        dlog_error("Group4Detect: 模型初始化失败 [%s]", m_stInParam.strModelPath.c_str());
        goto FAIL;
    }

    bRet = false;
    m_pByteTracker = new Inference_NS::cBYTETracker();
    if (m_pByteTracker)
    {
        m_pByteTracker->setValue(m_fTrackThresh, m_fHighThresh, m_fMatchThresh, m_nFrameId, m_nMaxTimeLost);
        bRet = true;
    }

    return bRet;

FAIL:
    unInit();
    return false;
}

/* 反初始化 */
bool Group4Detect_NS::CGroup4Detect::unInit()
{
    if (m_pYoloUltralytics)
    {
        delete m_pYoloUltralytics;
        m_pYoloUltralytics = nullptr;
    }

    if(m_pByteTracker)
    {
        delete m_pByteTracker;
        m_pByteTracker = nullptr;
    }

    return true;
}

/* 处理数据 */
bool Group4Detect_NS::CGroup4Detect::process(
    InData_S               stInData,
    std::vector<Result_S> &vecResult,
    OutData_S             *stOutData)
{
    OutData_S defaultOutData;

    // 如果传入的指针为空，则使用默认对象
    if (stOutData == nullptr)
    {
        stOutData = &defaultOutData;
    }

    vecResult.clear();

    if (stInData.inMat.empty())
    {
        dlog_error("Group4Detect: 传入图片为空");
        return false;
    }

    if (!m_pYoloUltralytics)
    {
        dlog_error("Group4Detect: 未初始化算法类");
        return false;
    }

    if(!m_pByteTracker)
    {
        dlog_error("Group4Detect: 未初始化追踪");
        return false;
    }

    int clipFlag = 0;
    clipFlag |= stInData.stParam.stCigaretteDetectParam.bEnable;
    clipFlag |= stInData.stParam.stPhoneParam.bEnable;

    /* 人员 ROI 推理结果，只用于抽烟和玩手机。 */
    std::vector<Inference_NS::BoxData_S> vecPersonBoxDatas;
    /* 整帧推理结果，只用于睡觉和摔倒。 */
    std::vector<Inference_NS::BoxData_S> vecFullFrameBoxDatas;

    if(clipFlag)
    {
        for(const auto &area : stInData.vecArea)
        {
            // 裁剪人物图片，并填充至算法所需大小
            // 使用 & 运算符自动裁剪越界区域
            auto rect = area;
            pointScaleUpV3(rect.fX1, rect.fY1, rect.fX2, rect.fY2, stInData.inMat.cols, stInData.inMat.rows, 1.3);

            cv::Rect roi_rect(rect.fX1, rect.fY1, rect.fX2 - rect.fX1, rect.fY2 - rect.fY1);
            cv::Rect img_rect(0, 0, stInData.inMat.cols, stInData.inMat.rows);
            cv::Rect safe_rect = roi_rect & img_rect;  // 自动裁剪交集

            if (safe_rect.width <= 0 || safe_rect.height <= 0) {
                dlog_error("Group4Detect: No valid ROI overlap with image!");
                continue;
            }

            cv::Mat clipMat = stInData.inMat(safe_rect);
            resizeAndPadImage(clipMat, clipMat);
            Inference_NS::InputData_S stInputData;
            stInputData.pData              = (float *)clipMat.data;
            stInputData.nDataSize          = static_cast<size_t>(clipMat.total() * clipMat.elemSize() * sizeof(float));
            stInputData.stBoxs.fConfidence = stInData.stParam.fBoxThreshold;
            stInputData.stBoxs.fNms        = stInData.stParam.fNmsThreshold;

            std::vector<Inference_NS::BoxData_S> vTmpDatas;
            {
                bool bRet = false;
                if (m_pYoloUltralytics)
                {
                    bRet = m_pYoloUltralytics->inference(stInputData, vTmpDatas);
                    if (!bRet)
                    {
                        dlog_error("Group4Detect: 算法分析失败");
                        return false;
                    }
                }
                else
                {
                    dlog_error("Group4Detect: m_pYoloUltralytics 为空，算法分析失败");
                    return false;
                }
            }
            Result_S stResult;
            stResult.fX1            = rect.fX1;
            stResult.fX2            = rect.fX2;
            stResult.fY1            = rect.fY1;
            stResult.fY2            = rect.fY2;
            for (const auto &box : vTmpDatas)
            {
                const bool bCigaretteResult =
                    box.nLabel == CIGARETTE &&
                    stInData.stParam.stCigaretteDetectParam.bEnable;
                const bool bPhoneResult =
                    box.nLabel == PHONE &&
                    stInData.stParam.stPhoneParam.bEnable;

                if ((!bCigaretteResult && !bPhoneResult) ||
                    box.fConfidence <= 0.0f)
                {
                    continue;
                }

                vecPersonBoxDatas.push_back(box);

                /* 抽烟和玩手机对外输出当前 ROI 对应的整图人员框。 */
                stResult.fBoxConfidence = box.fConfidence;
                stResult.nClassId       = box.nLabel;
                vecResult.push_back(stResult);
            }


            // vecResult.push_back(stResult);
        }
    }
    int noClipFlag = 0;
    noClipFlag |= stInData.stParam.stSleepParam.bEnable;
    noClipFlag |= stInData.stParam.stFallParam.bEnable;

    if(noClipFlag)
    {
        /* 推理+后处理 */
        Inference_NS::InputData_S stInputData;
        cv::Mat reMat;
        // 如果图像尺寸与限制尺寸不同，需要缩放和填充
        if(stInData.inMat.cols != m_nLimitWidth || stInData.inMat.rows != m_nLimitHeight)
        {
            resizeAndPadImage(stInData.inMat,reMat);
            stInputData.pData              = (float *)reMat.data;
            stInputData.nDataSize          = static_cast<size_t>(reMat.total() * reMat.elemSize() * sizeof(float));
        }
        else
        {
            stInputData.pData              = (float *)stInData.inMat.data;
            stInputData.nDataSize          = static_cast<size_t>(stInData.inMat.total() * stInData.inMat.elemSize() * sizeof(float));
        }
        stInputData.stBoxs.fConfidence = stInData.stParam.fBoxThreshold;
        stInputData.stBoxs.fNms        = stInData.stParam.fNmsThreshold;

        std::vector<Inference_NS::BoxData_S> vTmpDatas;
        {
            bool bRet = false;
            if (m_pYoloUltralytics)
            {
                bRet = m_pYoloUltralytics->inference(stInputData, vTmpDatas);
                if (!bRet)
                {
                    dlog_error("Group4Detect: 算法分析失败");
                    return false;
                }
            }
            else
            {
                dlog_error("Group4Detect: m_pYoloUltralytics 为空，算法分析失败");
                return false;
            }
        }

        offsetScaRestor(vTmpDatas, m_nLimitWidth, m_nLimitHeight, stInData.inMat.cols, stInData.inMat.rows);

        for (const auto &box : vTmpDatas)
        {
            const bool bSleepResult =
                box.nLabel == SLEEP &&
                stInData.stParam.stSleepParam.bEnable;
            const bool bFallResult =
                (box.nLabel == FALL || box.nLabel == FALLING) &&
                stInData.stParam.stFallParam.bEnable;

            if (bSleepResult || bFallResult)
            {
                vecFullFrameBoxDatas.push_back(box);
            }
        }
    }

    int isSTrackFlag = 0;
    isSTrackFlag |= stInData.stParam.stSleepParam.bEnable;


    if (isSTrackFlag)
    {
        std::vector<DetectResult_S> vecBoxs;/* 只有睡岗需要跟踪 */
        for (const auto &box : vecFullFrameBoxDatas)
        {
            if(box.nLabel == SLEEP)
            {
                DetectResult_S vecBox;
                vecBox.nClassId    = box.nLabel;
                vecBox.fConfidence = box.fConfidence;
                vecBox.vfBox       = cv::Rect_<float>(box.stBoxs.nX1, box.stBoxs.nY1, box.stBoxs.nX2 - box.stBoxs.nX1, box.stBoxs.nY2 - box.stBoxs.nY1);

                vecBoxs.push_back(vecBox);
            }
        }

        std::vector<cSTrack> stStracks;
        stStracks = m_pByteTracker->update(vecBoxs);

        auto curTime = time(NULL);

        for (int nIndex = 0; nIndex < (int)stStracks.size(); nIndex++)
        {
            Target_S          *pstTarget      = nullptr;
            int                nTargetId      = stStracks[nIndex].track_id;
            float              fBoxConfidence = stStracks[nIndex].score;
            std::vector<float> vectlwh        = stStracks[nIndex].tlwh;
            if (vectlwh.size() < 4)
            {
                continue;
            }

            /* 判断id在在表里 */
            if (m_mapPenson.count(nTargetId))
            {
                m_mapPenson[nTargetId].isUsed = true;
                m_mapPenson[nTargetId].ndwellTime = 0;

                pstTarget = &m_mapPenson[nTargetId];
            }
            else
            {
                Target_S newPerson;
                newPerson.nId = nTargetId;
                newPerson.nFirstTime = curTime;
                newPerson.isUsed = true;

                m_mapPenson[nTargetId] = newPerson;
                pstTarget = &m_mapPenson[nTargetId];
            }


            Result_S stResult;
            stResult.nClassId       = SLEEP;
            stResult.fX1            = vectlwh[0];
            stResult.fY1            = vectlwh[1];
            stResult.fX2            = vectlwh[0] + vectlwh[2];
            stResult.fY2            = vectlwh[1] + vectlwh[3];
            stResult.fBoxConfidence = fBoxConfidence;


            /* 睡岗 */
            {
                if (fBoxConfidence > stInData.stParam.stSleepParam.fConfidence)
                {
                    // vecResult.push_back(stResult);
                    if (curTime - pstTarget->nFirstTime >= stInData.stParam.stSleepParam.nDetectDuration)
                    {
                        vecResult.push_back(stResult);
                        stOutData->bSleep  = true;
                    }
                }
            }
        }
    }


    /* 标记当前帧是否识别到了 香烟 */
    bool bDetectCigarette = false;
    /* 标记当前帧是否识别到了 玩手机 */
    bool bDetectPhone = false;
    /* 标记当前帧是否识别到了 摔跤 */
    bool bDetectFall = false;


    for (const auto &box : vecPersonBoxDatas)
    {
        if (box.nLabel == CIGARETTE)
        {
            dlog_info("Group4Detect: 触发抽烟事件 置信度: %f", box.fConfidence);

            if (box.fConfidence > stInData.stParam.stCigaretteDetectParam.fConfidence)
            {
                bDetectCigarette = true;
            }
        }
        else if (box.nLabel == PHONE)
        {
            dlog_info("Group4Detect: 触发玩手机事件 置信度: %f", box.fConfidence);

            if (box.fConfidence > stInData.stParam.stPhoneParam.fConfidence)
            {
                bDetectPhone = true;
            }
        }
    }

    for (const auto &box : vecFullFrameBoxDatas)
    {
        if (box.nLabel != FALL && box.nLabel != FALLING)
        {
            continue;
        }

        dlog_info("Group4Detect: 触发摔倒事件 置信度: %f", box.fConfidence);

        if (box.fConfidence <= stInData.stParam.stFallParam.fConfidence)
        {
            continue;
        }

        bDetectFall = true;

        Result_S stResult;
        stResult.nClassId       = box.nLabel;
        stResult.fX1            = box.stBoxs.nX1;
        stResult.fY1            = box.stBoxs.nY1;
        stResult.fX2            = box.stBoxs.nX2;
        stResult.fY2            = box.stBoxs.nY2;
        stResult.fBoxConfidence = box.fConfidence;
        vecResult.push_back(stResult);
    }

    int64_t llCurTimestamp = time(NULL);
    /* 检测到吸烟 */
    if (bDetectCigarette)
    {
        /* 已经开始计时，并且在时间窗口内达标 */
        if (m_llCigaretteTimestamp != 0 && llCurTimestamp - m_llCigaretteTimestamp >= stInData.stParam.stCigaretteDetectParam.nDetectDuration)
        {
            stOutData->bCigarette = true;
            m_llCigaretteTimestamp = 0;
        }
        /* 第一次检测到吸烟，记录时间 */
        if (m_llCigaretteTimestamp == 0)
        {
            m_llCigaretteTimestamp = llCurTimestamp;
        }
    }
    else
    {
        /* 已经开始计时，但在整个时间窗口内都没再检测到吸烟 */
        if (m_llCigaretteTimestamp != 0 && (llCurTimestamp - m_llCigaretteTimestamp > stInData.stParam.stCigaretteDetectParam.nDetectDuration))
        {
            m_llCigaretteTimestamp = 0;
        }
    }

    if (bDetectPhone)
    {
        /* 已经开始计时，并且在时间窗口内达标 */
        if (m_llPhoneTimestamp != 0 && llCurTimestamp - m_llPhoneTimestamp >= stInData.stParam.stPhoneParam.nDetectDuration)
        {
            stOutData->bPhone = true;
            m_llPhoneTimestamp = 0;
        }
        /* 第一次检测到玩手机，记录时间 */
        if (m_llPhoneTimestamp == 0)
        {
            m_llPhoneTimestamp = llCurTimestamp;
        }
    }
    else
    {
        /* 已经开始计时，但在整个时间窗口内都没再检测到玩手机 */
        if (m_llPhoneTimestamp != 0 && (llCurTimestamp - m_llPhoneTimestamp > stInData.stParam.stPhoneParam.nDetectDuration))
        {
            m_llPhoneTimestamp = 0;
        }
    }



    if (bDetectFall)
    {
       if (m_llFallTimestamp != 0 && llCurTimestamp - m_llFallTimestamp >= stInData.stParam.stFallParam.nDetectDuration)
        {
            stOutData->bFall  = true;
            m_llFallTimestamp = 0;
        }

        if (m_llFallTimestamp == 0)
        {
            m_llFallTimestamp = llCurTimestamp;
        }
    }
    else
    {
        if (m_llFallTimestamp != 0 && (llCurTimestamp - m_llFallTimestamp > stInData.stParam.stFallParam.nDetectDuration))
        {
            m_llFallTimestamp = 0;
        }
    }


    for (auto pair = m_mapPenson.begin(); pair != m_mapPenson.end();)
    {
        int id = pair->first;
        if (m_mapPenson[id].isUsed)
        {
            m_mapPenson[id].isUsed = false;
        }
        else
        {
            m_mapPenson[id].ndwellTime++;
        }

        if (m_mapPenson[id].ndwellTime >= m_nMaxTimeLost)
        {
            pair = m_mapPenson.erase(pair);
        }
        else
        {
            ++pair;
        }
    }

    return true;
}

/* 处理数据 */
/**
 * @brief 等比例缩放并填充图片至固定尺寸
 * @param inputImage  输入图像（任意尺寸）
 * @param outputImage 输出图像（固定尺寸 m_nLimitWidth × m_nLimitHeight）
 * @return true 始终返回 true
 * @details 先按比例缩放(支持放大/缩小)，使图像适应限制尺寸，然后居中填充黑色边框。
 *          缩放比例 = min(限制宽度/原宽度, 限制高度/原高度)，保证缩放后不超出限制尺寸。
 *          例：417×585 → scale=min(640/417, 384/585)=0.656, 缩放后 274×384, 居中偏移(183,0)
 */
bool Group4Detect_NS::CGroup4Detect::resizeAndPadImage(cv::Mat inputImage, cv::Mat &outputImage)
{
    if (inputImage.empty()) {
        dlog_error("Group4Detect: Input image is empty!");
        return false;
    }

    int imageWidth  = inputImage.cols;
    int imageHeight = inputImage.rows;

    // 计算缩放比例：取宽高两个方向的较小值，保证缩放后两边都不超出限制尺寸
    // scale > 1 表示放大，scale < 1 表示缩小
    m_fResizeScale = std::min(
        static_cast<float>(m_nLimitWidth) / static_cast<float>(imageWidth),
        static_cast<float>(m_nLimitHeight) / static_cast<float>(imageHeight));

    // 缩放后的尺寸
    int newWidth  = static_cast<int>(imageWidth * m_fResizeScale);
    int newHeight = static_cast<int>(imageHeight * m_fResizeScale);

    // 执行缩放
    cv::Mat resizedImage;
    cv::resize(inputImage, resizedImage, cv::Size(newWidth, newHeight));

    // 居中偏移量（保证非负：newWidth/newHeight 由 min 保证 ≤ 限制尺寸）
    m_nXOffset = (m_nLimitWidth - newWidth) / 2;
    m_nYOffset = (m_nLimitHeight - newHeight) / 2;

    // 创建固定尺寸黑色背景，将缩放后的图像居中放置
    cv::Mat output = cv::Mat::zeros(m_nLimitHeight, m_nLimitWidth, inputImage.type());
    resizedImage.copyTo(output(cv::Rect(m_nXOffset, m_nYOffset, newWidth, newHeight)));

    outputImage = output;
    return true;
}
