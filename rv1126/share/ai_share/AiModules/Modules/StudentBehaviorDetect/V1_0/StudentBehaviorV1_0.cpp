/*
 * @Author: zhengxh zhengxh@kfb.cn
 * @Date: 2026-03-30 19:19
 * @LastEditors: zhengxh zhengxh@kfb.cn
 * @LastEditTime: 2026-03-30 19:19
 * @FilePath: ai_share/AiModules/Modules/StudentBehaviorDetect/V1_0/StudentBehaviorV1_0.cpp
 * @Description: 学生行为分析
 */

#include "StudentBehaviorV1_0.hpp"

#include <chrono>
#include <string>

#include "StudentBehaviorExt.hpp"
#include "dlog.h"
#include "SaveImage.hpp"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/persistence.hpp"
#include "opencv2/core/types.hpp"
#include "rk_comm_vo.h"


/* 一组数据的大小 */
#define DATA_BOX_GROUP_SIZE 6
#define DATA_KEY_GROUP_SIZE 3
#define DATA_KEY_SIZE       26

const std::string DEBUG_HEAD_STR = "head";
const std::string DEBUG_BODY_STR = "body";

using namespace StudentBehavior_NS;

StudentBehavior_NS::CStudentBehaviorV1_0::CStudentBehaviorV1_0(InParam_S stInParam)
    : m_stInParam(stInParam)
    , m_pHInference(nullptr)
    , m_pBInference(nullptr)
{

}

StudentBehavior_NS::CStudentBehaviorV1_0::~CStudentBehaviorV1_0()
{
    unInit();
}

/* 初始化 */
bool StudentBehavior_NS::CStudentBehaviorV1_0::init()
{
    do {
        /* 人头检测初始化 */
        if (m_stInParam.strHModelPath.empty() || m_stInParam.strBModelPath.empty()) break;
        m_pHInference = new Inference_NS::CYolov5(m_stInParam.strHModelPath);
        if (!m_pHInference) {
            dlog_error("人头检测创建失败");
            break;
        }

        if (!m_pHInference->init()) {
            dlog_error("人头检测初始化失败");
            break;
        }
        
        /* 学生行为分析初始化 */
        m_pBInference = new Inference_NS::CImageFeature(m_stInParam.strBModelPath);
        if (!m_pBInference) {
            dlog_error("学生行为检测创建失败");
            break;
        }

        if (!m_pBInference->init()) {
            dlog_error("学生行为检测初始化失败");
            break;
        }

        if (!m_pBInference->getSizeLimit(0, m_nFastPoseLimitWidth, m_nFastPoseLimitHeight, m_nFastPosetLimitChannel)) {
            dlog_error("学生行为检测 模型的目标尺寸获取失败");
            break;
        }

        if (m_nFastPoseLimitWidth <= 0 || m_nFastPoseLimitHeight <= 0) {
            dlog_error("学生行为检测 模型的目标尺寸获取异常");
            break;
        }

        return true;
    } while (0);
    unInit();
    return false;
}

/* 反初始化 */
bool StudentBehavior_NS::CStudentBehaviorV1_0::unInit()
{
    if (m_pHInference)
    {
        delete m_pHInference;
        m_pHInference = nullptr;
        return true;
    }
    if (m_pBInference)
    {
        delete m_pBInference;
        m_pBInference = nullptr;
        return true;
    }
    return false;
}

/* 处理数据 */
bool StudentBehavior_NS::CStudentBehaviorV1_0::process(
    StudentBehavior_NS::InData_S stInData,
    std::vector<StudentBehavior_NS::Result_S> &vecResult,
    StudentBehavior_NS::OutData_S *stOutData)
{
    if (stInData.inMat.empty())
    {
        dlog(LOG_ERROR, "传入图片为空");
        return false;
    }

    if (stOutData == nullptr) {
        dlog(LOG_ERROR, "检测结果指针为空");
        return false;
    }

    if (!m_pHInference)
    {
        dlog(LOG_ERROR, "未初始化人头检测算法类");
        return false;
    }

    if (!m_pBInference)
    {
        dlog(LOG_ERROR, "未初始化人体特征提取算法类");
        return false;
    }

    bool bRet = true;
    if (m_stInParam.bDebug)
    {
        /* 保存图片 */
        if (!stInData.inMat.empty() && !m_stInParam.strOriginalDataPath.empty())
        {
            if (!Modules_NS::saveImage(stInData.inMat, m_stInParam.strOriginalDataPath))
            {
                dlog(LOG_ERROR, "Debug-保存图片失败[%s]", m_stInParam.strOriginalDataPath.c_str());
            }
        }
    }

    Inference_NS::InputData_S stInputData;
    std::vector<Inference_NS::BoxData_S> vBoxDatas;
    stInputData.pData              = (float *)stInData.inMat.data;
    stInputData.nDataSize          = static_cast<size_t>(stInData.inMat.total() * stInData.inMat.elemSize() * sizeof(float));

    /* 人头检测推理+后处理 */
    bRet = m_pHInference->inference(stInputData, vBoxDatas);
    if (!bRet) {
        dlog(LOG_ERROR, "算法分析失败");
        return false;
    }

    // debug
    for (int i = 0; i < vBoxDatas.size(); i++)
    {
        StudentBehavior_NS::Result_S stResult;
        stResult.fBoxConfidence = vBoxDatas[i].fConfidence;
        stResult.fX1            = vBoxDatas[i].stBoxs.nX1;
        stResult.fX2            = vBoxDatas[i].stBoxs.nX2;
        stResult.fY1            = vBoxDatas[i].stBoxs.nY1;
        stResult.fY2            = vBoxDatas[i].stBoxs.nY2;
        stResult.nClassId       = vBoxDatas[i].nLabel;
        stResult.nId            = 0;

        if (m_stInParam.bDebug) {
            cv::Mat save = stInData.inMat.clone();
            /* 画框 */
            cv::rectangle(
                save,
                cv::Point(stResult.fX1, stResult.fY1),
                cv::Point(stResult.fX2, stResult.fY2),
                cv::Scalar(255,0,0),
                2
            );
            /* 保存图片 */
            if (!save.empty() && !m_stInParam.strHeadDataPath.empty())
            {
                if (!Modules_NS::saveImage(save, m_stInParam.strHeadDataPath))
                {
                    dlog(LOG_ERROR, "Debug-保存图片失败[%s]", m_stInParam.strHeadDataPath.c_str());
                }
            }
        }
        vecResult.push_back(stResult);
    }

    cv::Mat aImage = stInData.inMat.clone();
    int imgW = stInData.inMat.cols;
    int imgH = stInData.inMat.rows;
    int                     nHeadW      = 0;
    int                     nHeadH      = 0;
    int                     nWidthSize  = 2;
    int                     nHeightSize = 3;

    for (auto &box : vBoxDatas)
    {
        nHeadW = box.stBoxs.nX2 - box.stBoxs.nX1;
        nHeadH = box.stBoxs.nY2 - box.stBoxs.nY1;

        /* 扩充坐标到全身 */
        int nBodyX1 = box.stBoxs.nX1 - nHeadW;
        int nBodyY1 = box.stBoxs.nY1;
        int nBodyX2 = box.stBoxs.nX2 + nHeadW;
        int nBodyY2 = box.stBoxs.nY2 + nHeadH * 3;

        /* 限制图片范围内 */
        nBodyX1 = std::max(0, nBodyX1);
        nBodyY1 = std::max(0, nBodyY1);
        nBodyX2 = std::min(imgW, nBodyX2);
        nBodyY2 = std::min(imgH, nBodyY2);

        /* 检查框是否还能成立 */
        if (nBodyX2 <= nBodyX1 || nBodyY2 <= nBodyY1)
        {
            dlog_debug("检查框不成立")
            continue;
        }

        /* 裁剪出人体图片 */
        cv::Rect cropRect(
            cv::Point(nBodyX1, nBodyY1),
            cv::Point(nBodyX2, nBodyY2));
        cv::Mat bodyMat = aImage(cropRect).clone();

        /* 人体关键点提取前处理 */
        if (bodyMat.channels() != m_nFastPosetLimitChannel)
        {
            dlog_error( "模型需要的通道数和输入图片的通道数不一致 inMat[%d] != m_nHeadDetectLimitChannel[%d]", bodyMat.channels(), m_nFastPosetLimitChannel);
            return false;
        }

        if (bodyMat.cols != m_nFastPoseLimitWidth || bodyMat.rows != m_nFastPoseLimitHeight) {
            /* 修改图片大小 */
            cv::resize(bodyMat, bodyMat, cv::Size(m_nWidth, m_nHeight));
        }
        
        if (m_stInParam.bDebug) {
            if (!bodyMat.empty() && !m_stInParam.strBodyDataPath.empty()) {
                if (!Modules_NS::saveImage(bodyMat, m_stInParam.strBodyDataPath))
                {
                    dlog(LOG_ERROR, "Debug-保存图片失败[%s]", m_stInParam.strBodyDataPath.c_str());
                }
            }
        }


        /* 人体输出数据 */
        std::vector<Inference_NS::ClsData_S> vClsDatas;
        Inference_NS::InputData_S stBInputData;
        stBInputData.pData = (float*)bodyMat.data;
        stBInputData.nDataSize = static_cast<size_t>(bodyMat.total() * bodyMat.elemSize() * sizeof(float));

        /* 算法分析 */
        bRet = m_pBInference->inference(stBInputData, vClsDatas);
        if (!bRet) {
            dlog(LOG_ERROR, "人体关键点提取算法分析失败");
            continue;
        }

        for (auto &data : vClsDatas) {
            float fPWMultiple = (box.stBoxs.nX2 - box.stBoxs.nX1) * 1.0 / m_nFastPoseLimitWidth;
            float fPHMultiple = (box.stBoxs.nY2 - box.stBoxs.nY1) * 1.0 / m_nFastPoseLimitHeight;

            std::vector<KeyPosInfo_S> vKeyPosInfo;
            vKeyPosInfo.clear();

            for (int nP = 0; nP < data.vFeature.size() / DATA_KEY_GROUP_SIZE; nP++) {
                data.vFeature[nP * DATA_KEY_GROUP_SIZE + 0] = box.stBoxs.nX1 + data.vFeature[nP * DATA_KEY_GROUP_SIZE + 0] * fPWMultiple;
                data.vFeature[nP * DATA_KEY_GROUP_SIZE + 1] = box.stBoxs.nY1 + data.vFeature[nP * DATA_KEY_GROUP_SIZE + 1] * fPHMultiple;

                KeyPosInfo_S stKeyPosInfo;
                stKeyPosInfo.dX     = data.vFeature[nP * DATA_KEY_GROUP_SIZE + 0];
                stKeyPosInfo.dY     = data.vFeature[nP * DATA_KEY_GROUP_SIZE + 1];
                stKeyPosInfo.dScore = data.vFeature[nP * DATA_KEY_GROUP_SIZE + 2];
                vKeyPosInfo.push_back(stKeyPosInfo);
            }

            int nBehavioralType = getBehavioralType(stInData.stParam, vKeyPosInfo);
            if (nBehavioralType != -1) {
                Behavior_S stBehavior;
                stBehavior.nBehavior = nBehavioralType;
                stBehavior.vFeature = data.vFeature;
                stOutData->vStBehavior.push_back(stBehavior);
            }
        }
    }
    return true;

}

int CStudentBehaviorV1_0::getBehavioralType(BehaviorParam_S stBehaviorParam, std::vector<KeyPosInfo_S> vecInfo)
{
#if 0
    for (auto &info : vecInfo) {
        printf("%f %f %f\n", info.dX, info.dY, info.dScore);
    }
#endif

    if (vecInfo.size() != 26)
    {
        return -1;
    }

    KeyPosInfo_S& stNose      = vecInfo[0];  /* 鼻子 */
    KeyPosInfo_S& stREye      = vecInfo[1];  /* 右眼 */
    KeyPosInfo_S& stLEye      = vecInfo[2];  /* 左眼 */
    KeyPosInfo_S& stREar      = vecInfo[3];  /* 右耳朵 */
    KeyPosInfo_S& stLEar      = vecInfo[4];  /* 左耳朵 */
    KeyPosInfo_S& stRShoulder = vecInfo[5];  /* 右肩 */
    KeyPosInfo_S& stLShoulder = vecInfo[6];  /* 左肩 */
    KeyPosInfo_S& stRElbow    = vecInfo[7];  /* 右手肘 */
    KeyPosInfo_S& stLElbow    = vecInfo[8];  /* 左手肘 */
    KeyPosInfo_S& stRWrist    = vecInfo[9];  /* 右手腕 */
    KeyPosInfo_S& stLWrist    = vecInfo[10]; /* 左手腕 */
    KeyPosInfo_S& stRHip      = vecInfo[11]; /* 右臀 */
    KeyPosInfo_S& stLHip      = vecInfo[12]; /* 左臀 */
    KeyPosInfo_S& stRKnee     = vecInfo[13]; /* 右膝盖 */
    KeyPosInfo_S& stLKnee     = vecInfo[14]; /* 左膝盖 */
    KeyPosInfo_S& stRAnkle    = vecInfo[15]; /* 右脚踝 */
    KeyPosInfo_S& stLAnkle    = vecInfo[16]; /* 左脚踝 */
    KeyPosInfo_S& stHead      = vecInfo[17]; /* 头顶 */
    KeyPosInfo_S& stNeck      = vecInfo[18]; /* 颈部 */
    KeyPosInfo_S& stHip       = vecInfo[19]; /* 臀部 */
/* 数据优化 */
#if 1
    double dScore = 0.8;
    /* 手臂特征点只要有二个超过阈值，全部设置为阈值以上 */
    /* 计算大于阈值的数的数量 */
    int    nCount = (stLShoulder.dScore > dScore) +
        (stLElbow.dScore > dScore) +
        (stLWrist.dScore > dScore);
    if (nCount >= 2)
    {
        stLShoulder.dScore = (dScore + 0.1);
        stLElbow.dScore    = (dScore + 0.1);
        stLWrist.dScore    = (dScore + 0.1);
    }

    nCount = (stRShoulder.dScore > dScore) +
        (stRElbow.dScore > dScore) +
        (stRWrist.dScore > dScore);
    if (nCount >= 2)
    {
        stRShoulder.dScore = (dScore + 0.1);
        stRElbow.dScore    = (dScore + 0.1);
        stRWrist.dScore    = (dScore + 0.1);
    }

    /* 耳朵特征点超过阈值，眼睛特征点设置为阈值以上 */
    if (stREar.dScore > dScore && stLEar.dScore > dScore)
    {
        stREye.dScore = (dScore + 0.1);
        stLEye.dScore = (dScore + 0.1);
    }

#endif

    /* 趴桌判断 */
    if (isLyingOnDesk(vecInfo, stBehaviorParam.stLyingOnDesk))
    {
        return LyingOnDesk;
    }

    /* 转身判断 */
    if (isTurningBody(vecInfo, stBehaviorParam.stTurningBody))
    {
        return TurningBody;
    }

    /* 站立判断 */
    if (isStanding(vecInfo, stBehaviorParam.stStanding))
    {
        return Standing;
    }

    /* 举手判断 */
    if (isRaisingHand(vecInfo, stBehaviorParam.stRaisingHand))
    {
        return RaisingHand;
    }

    /* 低头判断 */
    if (isBendingHead(vecInfo, stBehaviorParam.stBendingHead))
    {
        return BendingHead;
    }
    /* 转头判断 */
    else if (isTurningHead(vecInfo, stBehaviorParam.stTurningHead))
    {
        return TurningHead;
    }
    /* 抬头判断 */
    else if (isRaisingHead(vecInfo, stBehaviorParam.stRaisingHead))
    {
        return RaisingHead;
    }

    return -1;
}

/* 趴桌检测 */
bool CStudentBehaviorV1_0::isLyingOnDesk(const std::vector<KeyPosInfo_S>& vstPoint, LyingOnDesk_S stLyingOnDesk)
{
    if (vstPoint.size() != 26)
    {
        dlog(LOG_ERROR, "数据大小不对 [%d]", vstPoint.size());
        return false;
    }

    const KeyPosInfo_S& stNose      = vstPoint[0];  /* 鼻子 */
    const KeyPosInfo_S& stREye      = vstPoint[1];  /* 右眼 */
    const KeyPosInfo_S& stLEye      = vstPoint[2];  /* 左眼 */
    const KeyPosInfo_S& stREar      = vstPoint[3];  /* 右耳朵 */
    const KeyPosInfo_S& stLEar      = vstPoint[4];  /* 左耳朵 */
    const KeyPosInfo_S& stRShoulder = vstPoint[5];  /* 右肩 */
    const KeyPosInfo_S& stLShoulder = vstPoint[6];  /* 左肩 */
    const KeyPosInfo_S& stRElbow    = vstPoint[7];  /* 右手肘 */
    const KeyPosInfo_S& stLElbow    = vstPoint[8];  /* 左手肘 */
    const KeyPosInfo_S& stRWrist    = vstPoint[9];  /* 右手腕 */
    const KeyPosInfo_S& stLWrist    = vstPoint[10]; /* 左手腕 */
    const KeyPosInfo_S& stRHip      = vstPoint[11]; /* 右臀 */
    const KeyPosInfo_S& stLHip      = vstPoint[12]; /* 左臀 */
    const KeyPosInfo_S& stRKnee     = vstPoint[13]; /* 右膝盖 */
    const KeyPosInfo_S& stLKnee     = vstPoint[14]; /* 左膝盖 */
    const KeyPosInfo_S& stRAnkle    = vstPoint[15]; /* 右脚踝 */
    const KeyPosInfo_S& stLAnkle    = vstPoint[16]; /* 左脚踝 */
    const KeyPosInfo_S& stHead      = vstPoint[17]; /* 头顶 */
    const KeyPosInfo_S& stNeck      = vstPoint[18]; /* 颈部 */
    const KeyPosInfo_S& stHip       = vstPoint[19]; /* 臀部 */

    /* 趴桌子 : 头顶，有一个低于肩膀。则说明为趴桌子 */
    if (stLShoulder.dScore > stLyingOnDesk.dKeyScore &&
        stRShoulder.dScore > stLyingOnDesk.dKeyScore &&
        stHead.dScore > stLyingOnDesk.dKeyScore)
    {
        double dMinShoulder = std::min(stLShoulder.dY, stRShoulder.dY);
        if ((stHead.dY) >= dMinShoulder)
        {
            return true;
        }
    }

    return false;
}

/* 转身检测 */
bool CStudentBehaviorV1_0::isTurningBody(const std::vector<KeyPosInfo_S>& vstPoint, TurningBody_S stTurningBody)
{
    if (vstPoint.size() != 26)
    {
        dlog(LOG_ERROR, "数据大小不对 [%d]", vstPoint.size());
        return false;
    }

    const KeyPosInfo_S& stNose      = vstPoint[0];  /* 鼻子 */
    const KeyPosInfo_S& stREye      = vstPoint[1];  /* 右眼 */
    const KeyPosInfo_S& stLEye      = vstPoint[2];  /* 左眼 */
    const KeyPosInfo_S& stREar      = vstPoint[3];  /* 右耳朵 */
    const KeyPosInfo_S& stLEar      = vstPoint[4];  /* 左耳朵 */
    const KeyPosInfo_S& stRShoulder = vstPoint[5];  /* 右肩 */
    const KeyPosInfo_S& stLShoulder = vstPoint[6];  /* 左肩 */
    const KeyPosInfo_S& stRElbow    = vstPoint[7];  /* 右手肘 */
    const KeyPosInfo_S& stLElbow    = vstPoint[8];  /* 左手肘 */
    const KeyPosInfo_S& stRWrist    = vstPoint[9];  /* 右手腕 */
    const KeyPosInfo_S& stLWrist    = vstPoint[10]; /* 左手腕 */
    const KeyPosInfo_S& stRHip      = vstPoint[11]; /* 右臀 */
    const KeyPosInfo_S& stLHip      = vstPoint[12]; /* 左臀 */
    const KeyPosInfo_S& stRKnee     = vstPoint[13]; /* 右膝盖 */
    const KeyPosInfo_S& stLKnee     = vstPoint[14]; /* 左膝盖 */
    const KeyPosInfo_S& stRAnkle    = vstPoint[15]; /* 右脚踝 */
    const KeyPosInfo_S& stLAnkle    = vstPoint[16]; /* 左脚踝 */
    const KeyPosInfo_S& stHead      = vstPoint[17]; /* 头顶 */
    const KeyPosInfo_S& stNeck      = vstPoint[18]; /* 颈部 */
    const KeyPosInfo_S& stHip       = vstPoint[19]; /* 臀部 */

    /* 如果左肩大于右肩，则为转身 */
    if (stLShoulder.dScore > stTurningBody.dKeyScore &&
        stRShoulder.dScore > stTurningBody.dKeyScore &&
        stLShoulder.dX > stRShoulder.dX)
    {
        /* 转身 */
        return true;
    }

    return false;
}

/* 站立检测 */
bool CStudentBehaviorV1_0::isStanding(const std::vector<KeyPosInfo_S>& vstPoint, Standing_S stStanding)
{
    if (vstPoint.size() != 26)
    {
        dlog(LOG_ERROR, "数据大小不对 [%d]", vstPoint.size());
        return false;
    }

    const KeyPosInfo_S& stNose      = vstPoint[0];  /* 鼻子 */
    const KeyPosInfo_S& stREye      = vstPoint[1];  /* 右眼 */
    const KeyPosInfo_S& stLEye      = vstPoint[2];  /* 左眼 */
    const KeyPosInfo_S& stREar      = vstPoint[3];  /* 右耳朵 */
    const KeyPosInfo_S& stLEar      = vstPoint[4];  /* 左耳朵 */
    const KeyPosInfo_S& stRShoulder = vstPoint[5];  /* 右肩 */
    const KeyPosInfo_S& stLShoulder = vstPoint[6];  /* 左肩 */
    const KeyPosInfo_S& stRElbow    = vstPoint[7];  /* 右手肘 */
    const KeyPosInfo_S& stLElbow    = vstPoint[8];  /* 左手肘 */
    const KeyPosInfo_S& stRWrist    = vstPoint[9];  /* 右手腕 */
    const KeyPosInfo_S& stLWrist    = vstPoint[10]; /* 左手腕 */
    const KeyPosInfo_S& stRHip      = vstPoint[11]; /* 右臀 */
    const KeyPosInfo_S& stLHip      = vstPoint[12]; /* 左臀 */
    const KeyPosInfo_S& stRKnee     = vstPoint[13]; /* 右膝盖 */
    const KeyPosInfo_S& stLKnee     = vstPoint[14]; /* 左膝盖 */
    const KeyPosInfo_S& stRAnkle    = vstPoint[15]; /* 右脚踝 */
    const KeyPosInfo_S& stLAnkle    = vstPoint[16]; /* 左脚踝 */
    const KeyPosInfo_S& stHead      = vstPoint[17]; /* 头顶 */
    const KeyPosInfo_S& stNeck      = vstPoint[18]; /* 颈部 */
    const KeyPosInfo_S& stHip       = vstPoint[19]; /* 臀部 */

    /* 如果左肩大于右肩，则为转身 */
    if (stLShoulder.dScore > stStanding.dKeyScore &&
        stRShoulder.dScore > stStanding.dKeyScore &&
        stLShoulder.dX > stRShoulder.dX)
    {
        if (stLHip.dScore > stStanding.dKeyScore &&
            stRHip.dScore > stStanding.dKeyScore)
        {
            /* 转身的站立 */
            return true;
        }
    }

    /* 站立 */
#if 1
    bool bValue = false;
    /* 站立 小腿大腿竖直20度以内，且能检测到身体 */
    if (stLHip.dScore > stStanding.dKeyScore && stLKnee.dScore > stStanding.dKeyScore)
    {
        if (stLHip.dY < stLKnee.dY)
        {
            /* 膝盖和臀部的点与Y轴夹角，小于一定的角度 */
            double dAxis = getAngleWithYAxis(stLKnee, stLHip);
            if (dAxis < stStanding.dYRaiseAngle)
            {
                bValue = true;
            }
        }
    }
    if (stRHip.dScore > stStanding.dKeyScore && stRKnee.dScore > stStanding.dKeyScore)
    {
        if (stRHip.dY < stRKnee.dY)
        {
            double dAxis = getAngleWithYAxis(stRKnee, stRHip);
            if (dAxis < stStanding.dYRaiseAngle)
            {
                bValue = true;
            }
        }
    }

    /* 满足上述条件后，还要判断 */
    if (bValue)
    {
        /* 必须有一个手的手腕低于手肘 */
        if (stLElbow.dScore > stStanding.dKeyScore &&
            stLWrist.dScore > stStanding.dKeyScore &&
            stLWrist.dY > stLElbow.dY)
        {
            /* 且手肘到手腕和手肘到肩膀的角度必须大于某个角度 */
            double dAxis = getAngleBetweenPoints(stLElbow, stLShoulder, stLWrist);
            if (dAxis > stStanding.dRaiseAngle)
            {
                return true;
            }
        }
        if (stRElbow.dScore > stStanding.dKeyScore &&
            stRWrist.dScore > stStanding.dKeyScore &&
            stRWrist.dY > stRElbow.dY)
        {
            /* 且手肘到手腕和手肘到肩膀的角度必须大于某个角度 */
            double dAxis = getAngleBetweenPoints(stRElbow, stRShoulder, stRWrist);
            if (dAxis > stStanding.dRaiseAngle)
            {
                return true;
            }
        }
    }

#endif
    return false;
}

/* 计算两点之间与y轴的角度 */
double CStudentBehaviorV1_0::getAngleWithYAxis(const KeyPosInfo_S& stPoint1, const KeyPosInfo_S& stPoint2)
{
    /* 确保两点的 x 坐标不同，避免除以零错误 */
    if (stPoint1.dX == stPoint2.dX)
    {
        /* 如果两点的 x 坐标相同，则直线垂直于 x 轴，夹角为 90 度 */
        return 0;
    }

    /* 计算直线的斜率 */
    double dSlope = std::abs((stPoint2.dX - stPoint1.dX) / (stPoint2.dY - stPoint1.dY));

    /* 计算斜率的角度（弧度） */
    double dAngleRadians = std::atan(dSlope);

    /* 将弧度转换为角度 */
    double dAngleDegrees = dAngleRadians * 180.0 / M_PI;    // std::to_degrees(dAngleRadians);

    /* 确保角度在0到90度之间 */
    dAngleDegrees = std::min(dAngleDegrees, 90.0d);
    dAngleDegrees = std::max(dAngleDegrees, 0.0d);

    return dAngleDegrees;
}

/* 计算三角形面积的两倍 */
double CStudentBehaviorV1_0::getTwiceTriangleArea(const KeyPosInfo_S& a, const KeyPosInfo_S& b, const KeyPosInfo_S& c)
{
    return a.dX * (b.dY - c.dY) + b.dX * (c.dY - a.dY) + c.dX * (a.dY - b.dY);
}

/* 计算由三个点形成的夹角的角度 a是交点 */
double CStudentBehaviorV1_0::getAngleBetweenPoints(const KeyPosInfo_S& a, const KeyPosInfo_S& b, const KeyPosInfo_S& c)
{
    // 计算三角形面积的两倍，检查是否共线
    double area = getTwiceTriangleArea(a, b, c);
    if (area == 0)
    {
        return 180.0;
    }

    // 计算向量AB和向量AC
    double ABx = b.dX - a.dX;
    double ABy = b.dY - a.dY;
    double ACx = c.dX - a.dX;
    double ACy = c.dY - a.dY;

    // 计算向量AB和向量AC的点积
    double dotProduct = ABx * ACx + ABy * ACy;

    // 计算向量AB和向量AC的模
    double magnitudeAB = sqrt(ABx * ABx + ABy * ABy);
    double magnitudeAC = sqrt(ACx * ACx + ACy * ACy);

    // 计算夹角的余弦值
    double cosAngle = dotProduct / (magnitudeAB * magnitudeAC);

    // 将余弦值转换为角度
    double angleRadians = acos(cosAngle);
    double angleDegrees = angleRadians * (180.0 / M_PI);

    return angleDegrees;
}

/* 举手检测 */
bool CStudentBehaviorV1_0::isRaisingHand(const std::vector<KeyPosInfo_S>& vstPoint, RaisingHand_S stRaisingHand)
{
    if (vstPoint.size() != 26)
    {
        dlog(LOG_ERROR, "数据大小不对 [%d]", vstPoint.size());
        return false;
    }

    const KeyPosInfo_S& stNose      = vstPoint[0];  /* 鼻子 */
    const KeyPosInfo_S& stREye      = vstPoint[1];  /* 右眼 */
    const KeyPosInfo_S& stLEye      = vstPoint[2];  /* 左眼 */
    const KeyPosInfo_S& stREar      = vstPoint[3];  /* 右耳朵 */
    const KeyPosInfo_S& stLEar      = vstPoint[4];  /* 左耳朵 */
    const KeyPosInfo_S& stRShoulder = vstPoint[5];  /* 右肩 */
    const KeyPosInfo_S& stLShoulder = vstPoint[6];  /* 左肩 */
    const KeyPosInfo_S& stRElbow    = vstPoint[7];  /* 右手肘 */
    const KeyPosInfo_S& stLElbow    = vstPoint[8];  /* 左手肘 */
    const KeyPosInfo_S& stRWrist    = vstPoint[9];  /* 右手腕 */
    const KeyPosInfo_S& stLWrist    = vstPoint[10]; /* 左手腕 */
    const KeyPosInfo_S& stRHip      = vstPoint[11]; /* 右臀 */
    const KeyPosInfo_S& stLHip      = vstPoint[12]; /* 左臀 */
    const KeyPosInfo_S& stRKnee     = vstPoint[13]; /* 右膝盖 */
    const KeyPosInfo_S& stLKnee     = vstPoint[14]; /* 左膝盖 */
    const KeyPosInfo_S& stRAnkle    = vstPoint[15]; /* 右脚踝 */
    const KeyPosInfo_S& stLAnkle    = vstPoint[16]; /* 左脚踝 */
    const KeyPosInfo_S& stHead      = vstPoint[17]; /* 头顶 */
    const KeyPosInfo_S& stNeck      = vstPoint[18]; /* 颈部 */
    const KeyPosInfo_S& stHip       = vstPoint[19]; /* 臀部 */


    /* 举手，1.手腕高于手肘，并且手肘和手肘连成的直线在纵向小于一定度 */
    if (stLElbow.dScore > stRaisingHand.dKeyScore &&
        stLWrist.dScore > stRaisingHand.dKeyScore)
    {
        if (stLElbow.dY > stLWrist.dY)
        {
            double dAxis = getAngleWithYAxis(stLElbow, stLWrist);
            if (dAxis < stRaisingHand.dYRaiseAngle)
            {
                /* 举左手 */
                return true;
            }
            // dlog(LOG_ERROR, "举左手 [%f] %f(%f,%f)-(%f,%f)",
            //      dAxis,
            //      stRaisingHand.dRaiseAngle,
            //      stLElbow.dX,
            //      stLElbow.dY,
            //      stLWrist.dX,
            //      stLWrist.dY);
        }
    }

    if (stRElbow.dScore > stRaisingHand.dKeyScore &&
        stRWrist.dScore > stRaisingHand.dKeyScore)
    {
        if (stRElbow.dY > stRWrist.dY)
        {
            double dAxis = getAngleWithYAxis(stRElbow, stRWrist);
            if (dAxis < stRaisingHand.dYRaiseAngle)
            {
                /* 举右手 */
                return true;
            }
            // dlog(LOG_ERROR, "举右手 [%f] %f(%f,%f)-(%f,%f)",
            //      dAxis,
            //      stRaisingHand.dRaiseAngle,
            //      stRElbow.dX,
            //      stRElbow.dY,
            //      stRWrist.dX,
            //      stRWrist.dY);
        }
    }

    return false;
}

/* 低头检测 */
bool CStudentBehaviorV1_0::isBendingHead(const std::vector<KeyPosInfo_S>& vstPoint, BendingHead_S stBendingHead)
{
    if (vstPoint.size() != 26)
    {
        dlog(LOG_ERROR, "数据大小不对 [%d]", vstPoint.size());
        return false;
    }

    const KeyPosInfo_S& stNose      = vstPoint[0];  /* 鼻子 */
    const KeyPosInfo_S& stREye      = vstPoint[1];  /* 右眼 */
    const KeyPosInfo_S& stLEye      = vstPoint[2];  /* 左眼 */
    const KeyPosInfo_S& stREar      = vstPoint[3];  /* 右耳朵 */
    const KeyPosInfo_S& stLEar      = vstPoint[4];  /* 左耳朵 */
    const KeyPosInfo_S& stRShoulder = vstPoint[5];  /* 右肩 */
    const KeyPosInfo_S& stLShoulder = vstPoint[6];  /* 左肩 */
    const KeyPosInfo_S& stRElbow    = vstPoint[7];  /* 右手肘 */
    const KeyPosInfo_S& stLElbow    = vstPoint[8];  /* 左手肘 */
    const KeyPosInfo_S& stRWrist    = vstPoint[9];  /* 右手腕 */
    const KeyPosInfo_S& stLWrist    = vstPoint[10]; /* 左手腕 */
    const KeyPosInfo_S& stRHip      = vstPoint[11]; /* 右臀 */
    const KeyPosInfo_S& stLHip      = vstPoint[12]; /* 左臀 */
    const KeyPosInfo_S& stRKnee     = vstPoint[13]; /* 右膝盖 */
    const KeyPosInfo_S& stLKnee     = vstPoint[14]; /* 左膝盖 */
    const KeyPosInfo_S& stRAnkle    = vstPoint[15]; /* 右脚踝 */
    const KeyPosInfo_S& stLAnkle    = vstPoint[16]; /* 左脚踝 */
    const KeyPosInfo_S& stHead      = vstPoint[17]; /* 头顶 */
    const KeyPosInfo_S& stNeck      = vstPoint[18]; /* 颈部 */
    const KeyPosInfo_S& stHip       = vstPoint[19]; /* 臀部 */

    if (stNeck.dScore > stBendingHead.dKeyScore &&
        stNose.dScore > stBendingHead.dKeyScore)
    {
        if (stNeck.dY < stNose.dY)
        {
            return true;
        }
        else
        {
            if (stLEye.dScore > stBendingHead.dKeyScore &&
                stREye.dScore > stBendingHead.dKeyScore)
            {
                /* 获取stNose到眉心的垂点 */
                KeyPosInfo_S stAjina = getIntersection(stREye, stLEye, stNose);

                double dNumerator   = getDistance(stAjina, stNose);
                double dDenominator = getDistance(stAjina, stNeck);
                if (dDenominator != 0)
                {
                    double dFactor = dNumerator / dDenominator;
                    if (dFactor > stBendingHead.dRatio)
                    {
                        /* 低头 */
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

/* 获取斜率 */
double CStudentBehaviorV1_0::getSlope(const KeyPosInfo_S& p1, const KeyPosInfo_S& p2)
{
    if (p1.dX != p2.dX)
    {    // 检查是否为垂直线
        return (p2.dY - p1.dY) / (p2.dX - p1.dX);
    }
    else
    {
        return 0;    // 垂直线斜率设置为0
    }
}

/* 获取垂直交点 */
KeyPosInfo_S CStudentBehaviorV1_0::getIntersection(const KeyPosInfo_S& A, const KeyPosInfo_S& B, const KeyPosInfo_S& C)
{
    double slopeAB            = getSlope(A, B);
    double slopePerpendicular = -1 / slopeAB;    // Perpendicular slope

    // 垂直线的方程 y - y1 = slopePerpendicular * (x - x1)
    // 线段AB的方程 y = slopeAB * x + (y1 - slopeAB * x1)

    // 交点坐标的计算
    KeyPosInfo_S stOutInfo;

    if (slopeAB == 0)
    {                           // 线段AB水平
        stOutInfo.dX = A.dX;
        stOutInfo.dY = C.dY;    // 垂直线通过C点
    }
    else if (slopePerpendicular == 0)
    {                           // 垂直线水平
        stOutInfo.dY = C.dY;
        stOutInfo.dX = A.dX;    // 线段AB通过A点
    }
    else
    {
        stOutInfo.dX = (C.dY - (slopePerpendicular * C.dX) - (A.dY - slopeAB * A.dX)) / (slopeAB - slopePerpendicular);
        stOutInfo.dY = slopePerpendicular * stOutInfo.dX + (C.dY - slopePerpendicular * C.dX);
    }
    stOutInfo.dScore = 1.0;

    return stOutInfo;
}

/* 计算两点间的距离 */
double CStudentBehaviorV1_0::getDistance(const KeyPosInfo_S& stPoint1, const KeyPosInfo_S& stPoint2)
{
    /* 欧几里得距离公式 */
    return sqrt((stPoint2.dX - stPoint1.dX) * (stPoint2.dX - stPoint1.dX) + (stPoint2.dY - stPoint1.dY) * (stPoint2.dY - stPoint1.dY));
}

/* 转头检测 */
bool CStudentBehaviorV1_0::isTurningHead(const std::vector<KeyPosInfo_S>& vstPoint, TurningHead_S stTurningHead)
{
    if (vstPoint.size() != 26)
    {
        dlog(LOG_ERROR, "数据大小不对 [%d]", vstPoint.size());
        return false;
    }

    const KeyPosInfo_S& stNose      = vstPoint[0];  /* 鼻子 */
    const KeyPosInfo_S& stREye      = vstPoint[1];  /* 右眼 */
    const KeyPosInfo_S& stLEye      = vstPoint[2];  /* 左眼 */
    const KeyPosInfo_S& stREar      = vstPoint[3];  /* 右耳朵 */
    const KeyPosInfo_S& stLEar      = vstPoint[4];  /* 左耳朵 */
    const KeyPosInfo_S& stRShoulder = vstPoint[5];  /* 右肩 */
    const KeyPosInfo_S& stLShoulder = vstPoint[6];  /* 左肩 */
    const KeyPosInfo_S& stRElbow    = vstPoint[7];  /* 右手肘 */
    const KeyPosInfo_S& stLElbow    = vstPoint[8];  /* 左手肘 */
    const KeyPosInfo_S& stRWrist    = vstPoint[9];  /* 右手腕 */
    const KeyPosInfo_S& stLWrist    = vstPoint[10]; /* 左手腕 */
    const KeyPosInfo_S& stRHip      = vstPoint[11]; /* 右臀 */
    const KeyPosInfo_S& stLHip      = vstPoint[12]; /* 左臀 */
    const KeyPosInfo_S& stRKnee     = vstPoint[13]; /* 右膝盖 */
    const KeyPosInfo_S& stLKnee     = vstPoint[14]; /* 左膝盖 */
    const KeyPosInfo_S& stRAnkle    = vstPoint[15]; /* 右脚踝 */
    const KeyPosInfo_S& stLAnkle    = vstPoint[16]; /* 左脚踝 */
    const KeyPosInfo_S& stHead      = vstPoint[17]; /* 头顶 */
    const KeyPosInfo_S& stNeck      = vstPoint[18]; /* 颈部 */
    const KeyPosInfo_S& stHip       = vstPoint[19]; /* 臀部 */

    if (stLEye.dScore > stTurningHead.dKeyScore &&
        stREye.dScore > stTurningHead.dKeyScore &&
        stHead.dScore > stTurningHead.dKeyScore &&
        stNeck.dScore > stTurningHead.dKeyScore)
    {
        /* 转头 以头颈线为基准，左眼在右边，右眼在左边边都为转头 */

        int nLEye = getPointRelativeOfLine(stHead, stNeck, stLEye);
        if (nLEye <= 0)
        {
            /* 转头 */
            return true;
        }
        else
        {
            int nREye = getPointRelativeOfLine(stHead, stNeck, stREye);
            if (nREye >= 0)
            {
                /* 转头 */
                return true;
            }
        }
    }

    return false;
}

/* 计算向量AB和AP的叉积 */
double CStudentBehaviorV1_0::getCrossProduct(const KeyPosInfo_S& A, const KeyPosInfo_S& B, const KeyPosInfo_S& P)
{
    return (B.dX - A.dX) * (P.dY - A.dY) - (B.dY - A.dY) * (P.dX - A.dX);
}

/* 判断点P相对于由A和B定义的直线的位置, 具体返回值为 1 表示在左边，-1 表示在右边，0 表示在直线上 */
int CStudentBehaviorV1_0::getPointRelativeOfLine(const KeyPosInfo_S& A,
                                             const KeyPosInfo_S& B,
                                             const KeyPosInfo_S& P)
{
    double crossProduct = getCrossProduct(A, B, P);

    /* 根据直线方向决定左边的判断 */
    if (A.dY < B.dY)
    {
        /* 如果 A.y < B.y，则交叉积大于等于零在左边 */
        if (crossProduct == 0.0)
        {
            return 0;
        }
        else if (crossProduct > 0.0)
        {
            return 1;
        }
        else
        {
            return -1;
        }
    }
    else
    {
        if (crossProduct == 0.0)
        {
            return 0;
        }
        else if (crossProduct > 0.0)
        {
            return -1;
        }
        else
        {
            return 1;
        }
    }
}

/* 抬头检测 */
bool CStudentBehaviorV1_0::isRaisingHead(
    const std::vector<KeyPosInfo_S>& vstPoint,
    RaisingHead_S                    stRaisingHead)
{
    if (vstPoint.size() != 26)
    {
        dlog(LOG_ERROR, "数据大小不对 [%d]", vstPoint.size());
        return false;
    }

    const KeyPosInfo_S& stNose      = vstPoint[0];  /* 鼻子 */
    const KeyPosInfo_S& stREye      = vstPoint[1];  /* 右眼 */
    const KeyPosInfo_S& stLEye      = vstPoint[2];  /* 左眼 */
    const KeyPosInfo_S& stREar      = vstPoint[3];  /* 右耳朵 */
    const KeyPosInfo_S& stLEar      = vstPoint[4];  /* 左耳朵 */
    const KeyPosInfo_S& stRShoulder = vstPoint[5];  /* 右肩 */
    const KeyPosInfo_S& stLShoulder = vstPoint[6];  /* 左肩 */
    const KeyPosInfo_S& stRElbow    = vstPoint[7];  /* 右手肘 */
    const KeyPosInfo_S& stLElbow    = vstPoint[8];  /* 左手肘 */
    const KeyPosInfo_S& stRWrist    = vstPoint[9];  /* 右手腕 */
    const KeyPosInfo_S& stLWrist    = vstPoint[10]; /* 左手腕 */
    const KeyPosInfo_S& stRHip      = vstPoint[11]; /* 右臀 */
    const KeyPosInfo_S& stLHip      = vstPoint[12]; /* 左臀 */
    const KeyPosInfo_S& stRKnee     = vstPoint[13]; /* 右膝盖 */
    const KeyPosInfo_S& stLKnee     = vstPoint[14]; /* 左膝盖 */
    const KeyPosInfo_S& stRAnkle    = vstPoint[15]; /* 右脚踝 */
    const KeyPosInfo_S& stLAnkle    = vstPoint[16]; /* 左脚踝 */
    const KeyPosInfo_S& stHead      = vstPoint[17]; /* 头顶 */
    const KeyPosInfo_S& stNeck      = vstPoint[18]; /* 颈部 */
    const KeyPosInfo_S& stHip       = vstPoint[19]; /* 臀部 */

    if (stLEye.dScore > stRaisingHead.dKeyScore &&
        stLEar.dScore > stRaisingHead.dKeyScore &&
        stREye.dScore > stRaisingHead.dKeyScore &&
        stREar.dScore > stRaisingHead.dKeyScore)
    {

        if (std::max(stLEye.dY, stREye.dY) < std::min(stLEar.dY, stREar.dY))
        {
            return true;
        }

        double dAngle = getAngleBetweenPoints(stNose, stLEar, stREar);
        if (dAngle > stRaisingHead.dRaiseAngle)
        {
            return true;
        }
    }

    return false;
}