#include "ResultModule.hpp"

#include <cmath>

#include "Behavior.hpp"
#include "ConvertInterface.h"
#include "ConvertJson.hpp"

using namespace Ai0630_NS;

static std::tuple<int, float> softmax(const std::vector<float>& x)
{
    if (x.empty())
    {
        /* 空输入，返回无效 */
        return { -1, 0.0f };
    }

    /* 1. 找最大值（为了数值稳定） */
    float m = *std::max_element(x.begin(), x.end());

    /* 2. 计算分母 */
    double sum = 0.0;
    for (float v : x)
    {
        sum += std::exp(double(v) - double(m));
    }

    /* 如果 sum == 0（极端情况） */
    if (sum == 0.0)
    {
        return { -1, 0.0f };
    }

    /* 3. 找最大 softmax 值及其 index */
    float fMax   = -1.0f;
    int   nIndex = -1;

    for (int i = 0; i < (int)x.size(); i++)
    {
        float fTemp = float(std::exp(double(x[i]) - double(m)) / sum);
        if (fTemp > fMax)
        {
            fMax   = fTemp;
            nIndex = i;
        }
    }

    return { nIndex, fMax };
}

ResultModule::ResultModule()
{
    m_pTeacherSummary      = std::make_shared<TeacherSummary>();
    m_pAttendanceManager   = std::make_shared<AttendanceManager>();
    m_pClassStudentSummary = std::make_shared<ClassStudentSummary>();
    m_pClassroomSummary    = std::make_shared<ClassroomSummary>();
}

ResultModule::~ResultModule()
{
}

/* 处理算法分析数据 */
void ResultModule::recvData(
    HeaderInfo_S     stHeader,
    UserHeaderInfo_S stUserHeader,
    FaceResult_S     stResult)
{
    switch (stHeader.nCode)
    {
        /* 人脸识别 */
        case toInt(CommCode_E::AI_COM_FACE):
        {
            /* 应用1 判断是否在板书 */
            if (m_bCurTsPodium.load())
            {
                /* 老师在讲台 */
                for (const auto& item : stResult.vstAIFaceDetectResult)
                {
                    if (item.vstPointData.size() <= 0)
                    {
                        /* 但是没有人脸 */
                        if (m_pTeacherSummary)
                        {
                            m_pTeacherSummary->addBehavior(
                                stUserHeader,
                                TeacherBehavior_E::WRITING_BOARD);
                        }
                    }
                }
            }
            break;
        }
        /* 人头识别 */
        case toInt(CommCode_E::AI_COM_HEAD):
        {
            /* 应用1 人数统计 */
            for (const auto& item : stResult.vstAIHeadDetectResult)
            {
                int nNumber = item.vstBoxData.size();
                if (m_pClassStudentSummary)
                {
                    m_pClassStudentSummary->addPersonCount(nNumber, stUserHeader.nClassTime);
                }
            }

            break;
        }
        /* 班级表情识别 */
        case toInt(CommCode_E::AI_COM_CLASS_EMO):
        {
            /* 应用1 表情统计 */
            if (m_pClassStudentSummary)
            {
                std::unordered_map<Emotion_E, int> mapCnt;
                for (const auto& stResult : stResult.vstAIFaceEmotionResult)
                {
                    for (const auto& stClsData : stResult.vstClsData)
                    {
                        auto [nEmoType, fConfidence] = softmax(stClsData.vFeature);
                        mapCnt[m_pClassStudentSummary->toEmotion(nEmoType)]++;
                    }
                }
                m_pClassStudentSummary->addClassEmotionBatch(stUserHeader.nClassTime, mapCnt);
            }

            break;
        }
        /* 班级行为分析 */
        case toInt(CommCode_E::AI_COM_CLASS_BEHAVIOR):
        {
            /* 应用1 行为统计 */
            if (m_pClassStudentSummary && m_pTeacherSummary)
            {
                std::vector<Inference_NS::Point_S>         vPoints; /* 点信息 */
                std::unordered_map<StudentBehavior_E, int> mapCnt;
                std::vector<Inference_NS::Box_S>           vstCurTurnBoxInfo;
                for (const auto& stResult : stResult.vstAIFastPoseResult)
                {
                    for (const auto& stPointData : stResult.vstPointData)
                    {
                        int nBehavior = Behavior::getType(stPointData.vPoints, BehaviorParam_S());
                        mapCnt[m_pClassStudentSummary->toBehavior(nBehavior)]++;

                        if (!m_bCurTsPodium.load())
                        {
                            if (m_pClassStudentSummary->toBehavior(nBehavior) == StudentBehavior_E::DEMO ||
                                m_pClassStudentSummary->toBehavior(nBehavior) == StudentBehavior_E::DISCUSS)
                            {
                                /* 将框往中心点缩小半 */
                                Inference_NS::Box_S stHalfBox;

                                /* 计算框的中心点坐标 */
                                int nCenterX = (stPointData.stBoxs.nX1 + stPointData.stBoxs.nX2) / 2;
                                int nCenterY = (stPointData.stBoxs.nY1 + stPointData.stBoxs.nY2) / 2;

                                /* 将左上角和右下角的坐标向中心点移动一半的距离 */
                                int nHalfWidth  = (stPointData.stBoxs.nX2 - stPointData.stBoxs.nX1) / 2;
                                int nHalfHeight = (stPointData.stBoxs.nY2 - stPointData.stBoxs.nY1) / 2;
                                stHalfBox.nX1   = nCenterX - nHalfWidth;
                                stHalfBox.nY1   = nCenterY - nHalfHeight;
                                stHalfBox.nX2   = nCenterX + nHalfWidth;
                                stHalfBox.nY2   = nCenterY + nHalfHeight;

                                vstCurTurnBoxInfo.push_back(stHalfBox);
                            }
                        }
                    }
                }
                m_pClassStudentSummary->addClassBehaviorBatch(stUserHeader.nClassTime, mapCnt);
                m_pTeacherSummary->addPosition(stUserHeader, vstCurTurnBoxInfo);
            }
            break;
        }
        /* 课堂纪律 */
        case toInt(CommCode_E::AI_COM_DISCIPLINE):
        {
            break;
        }
        default:
        {
            break;
        }
    }
}

/* 接受人脸处理结果 */
void ResultModule::recvFaceData(
    HeaderInfo_S     stHeader,
    UserHeaderInfo_S stUserHeader,
    FaceLibsInfo_S   syFaceLibsInfo,
    HumanLibsInfo_S  stHumanLibsInfo)
{

    if (m_pAttendanceManager)
    {
        /* 添加考勤信息 */
        m_pAttendanceManager->add(syFaceLibsInfo, stUserHeader);
    }

    if (m_pClassStudentSummary && syFaceLibsInfo.nIdentity == 0)
    {
        /* 添加个人行为 */
        m_pClassStudentSummary->addStudentBehavior(
            syFaceLibsInfo.nMemberId,
            stUserHeader.nClassTime,
            stHumanLibsInfo.nBehaviorType);

        /* 添加个人表情 */
        m_pClassStudentSummary->addStudentEmotion(
            syFaceLibsInfo.nMemberId,
            stUserHeader.nClassTime,
            stHumanLibsInfo.nEmoType);
    }


    if (m_pTeacherSummary && syFaceLibsInfo.nIdentity == 1)
    {
        /* 行为是转身 */
        if (stHumanLibsInfo.nBehaviorType == 5)
        {
            m_pTeacherSummary->addBehavior(
                stUserHeader,
                TeacherBehavior_E::WRITING_BOARD);
        }

        /* 姿态是板书 */
        if (stHumanLibsInfo.nPostureType == 9)
        {
            m_pTeacherSummary->addBehavior(
                stUserHeader,
                TeacherBehavior_E::WRITING_BOARD);
        }

        m_pTeacherSummary->addPosture(
            stUserHeader,
            m_pTeacherSummary->toPosture(stHumanLibsInfo.nPostureType));
    }
}

/* 整合 */
void ResultModule::finalize(const void* pParam)
{
    ClassParamParam_S stClassParamParam;
    stClassParamParam.nClassTime = m_nRecordTime;
    if (m_pAttendanceManager)
    {
        m_pAttendanceManager->finalize(pParam);
    }

    if (m_pClassStudentSummary)
    {
        m_pClassStudentSummary->finalize(pParam);
        m_pClassStudentSummary->getInfo(stClassParamParam);
    }

    if (m_pTeacherSummary)
    {
        m_pTeacherSummary->finalize(pParam);
        m_pTeacherSummary->getInfo(stClassParamParam);
    }

    if (m_pClassroomSummary)
    {
        m_pClassroomSummary->addClassParam(stClassParamParam);
        m_pClassroomSummary->finalize(pParam);
    }

    /* 整合完，清空 */
    if (m_pAttendanceManager)
    {
        m_pAttendanceManager->reset();
    }

    if (m_pClassStudentSummary)
    {
        m_pClassStudentSummary->reset();
    }

    if (m_pTeacherSummary)
    {
        m_pTeacherSummary->reset();
    }

    if (m_pClassroomSummary)
    {
        m_pClassroomSummary->reset();
    }
}

/* PPT切换 */
BlError_E Ai0630_NS::ResultModule::pptSwitch(long long lTimestamp, std::string strJpgName)
{
    if (m_pClassroomSummary)
    {
        m_pClassroomSummary->addPPTSwitch(m_nRecordTime, lTimestamp, strJpgName);
    }
    return OK;
}

/* 切换老师画面 */
BlError_E ResultModule::switchTeacherScreen()
{
    m_bCurStScreen.store(false);
    if (m_pClassroomSummary)
    {
        m_pClassroomSummary->addTeacherBehavior();
    }
    return OK;
}

/* 切换学生画面 */
BlError_E ResultModule::switchStudentScreen()
{
    m_bCurStScreen.store(true);
    if (m_pClassroomSummary)
    {
        m_pClassroomSummary->addStudentBehavior();
    }
    return OK;
}

/* 切换学生特写 */
BlError_E ResultModule::switchStudentCloseUp(UserHeaderInfo_S stUserHeader, bool bValue)
{
    m_bStudentCloseUp.store(bValue);
    /* 添加班级学生站立行为 */
    if (m_pClassStudentSummary)
    {
        std::unordered_map<StudentBehavior_E, int> mapCnt;
        mapCnt[StudentBehavior_E::DEMO] = 1;
        m_pClassStudentSummary->addClassBehaviorBatch(stUserHeader.nClassTime, mapCnt);
    }
    return OK;
}

/* 切换老师是否在讲台 */
BlError_E ResultModule::switchTeacherPodium(bool bValue)
{
    m_bCurTsPodium.store(bValue);
    if (m_pTeacherSummary)
    {
        m_pTeacherSummary->switchTeacherPodium(bValue);
    }
    return OK;
}

/* 开始互动 */
BlError_E ResultModule::startInteraction(UserHeaderInfo_S stUserHeader)
{
    if (m_pTeacherSummary)
    {
        m_pTeacherSummary->addBehavior(stUserHeader, TeacherBehavior_E::INTERACTION);
    }
    return OK;
}

/* 开始巡视行为 */
BlError_E ResultModule::startTour(UserHeaderInfo_S stUserHeader)
{
    if (m_pTeacherSummary)
    {
        m_pTeacherSummary->addBehavior(stUserHeader, TeacherBehavior_E::WALKING_AROUND);
    }
    return OK;
}

/* 开始教授行为 */
BlError_E ResultModule::startTaught(UserHeaderInfo_S stUserHeader)
{
    if (m_pTeacherSummary)
    {
        m_pTeacherSummary->addBehavior(stUserHeader, TeacherBehavior_E::TEACHING);
    }
    return OK;
}

/* 老师板书切换 */
BlError_E ResultModule::teacherBoard(UserHeaderInfo_S stUserHeader, bool bValue)
{
    if (m_pTeacherSummary)
    {
        m_pTeacherSummary->addBehavior(stUserHeader, TeacherBehavior_E::WRITING_BOARD);
    }
    return OK;
}

/* 设置录制时间 */
BlError_E ResultModule::setRecordTime(int nRecordTime)
{
    m_nRecordTime = nRecordTime;
    return OK;
}

/* 添加课堂分贝值 */
void Ai0630_NS::ResultModule::addClassDb(int nDb)
{
    if (m_pClassroomSummary)
    {
        m_pClassroomSummary->addClassDb(m_nRecordTime, nDb);
    }
}
