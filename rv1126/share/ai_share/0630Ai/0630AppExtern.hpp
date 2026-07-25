#pragma once

#include <atomic>
#include <functional>
#include <iostream>

#include "BlError.h"
#include "FaceRecExtern.hpp"

/* 考勤图片临时路径 */
#define GANCIAN_PICTURE_TEMP_PATH ("/opt/course/Attendance")

namespace Ai0630_NS
{

    typedef std::function<BlError_E(int)> notifyGetSteamDataFunc;
    typedef std::function<bool(void)>     PTZControlFunc;

#pragma pack(push, 1)

    /* 用户参数，需要原封不动的传递回去，用于区分数据结果 */
    struct UserHeaderInfo_S
    {
        int       nClassId;   /* 班级ID */
        int       nClassTime; /* 当前录制时长 */
        long long lTimestamp; /* 当前时间戳 */

        void clear()
        {
            nClassId   = 0;
            nClassTime = 0;
            lTimestamp = 0;
        }
    };

    struct UserFaceInfo_S
    {
        int   nId;
        int   nClassId;              /* 班级ID */
        int   nMemberId;             /* 成员ID */
        char  achName[64];           /* 名字 */
        char  achPicName[128];       /* 图片名称 */
        char  achLocalPicPath[256];  /* 图片路径 */
        char  achRemotePicPath[256]; /* 图片路径 */
        char  achPicMd5[64];         /* 图片MD5 */
        char  achPicDate[32];        /* 图片日期 */
        int   nIdentity;             /* 身份：0学生 1老师 */
        float fConfidence;           /* 人脸置信度 */
    };

#pragma pack(pop)

    struct ClassInfo_S
    {
        int         nClassId;                  /* 班级Id */
        std::string strClassName;              /* 班级名称 */

        std::list<FaceLibsInfo_S> listTeaInfo; /* 教师人脸结构体 */
        std::list<FaceLibsInfo_S> listStuInfo; /* 学生人脸结构体 */

        void clear()
        {
            nClassId = 0;
            strClassName.clear();
            listTeaInfo.clear();
            listStuInfo.clear();
        }

        void print() const
        {
            std::cout << "\n获取班级信息请求:=============" << std::endl;
            std::cout << "班级Id:" << nClassId << std::endl;
            std::cout << "班级名称:" << strClassName << std::endl;
            std::cout << "老师信息:" << std::endl;
            for (auto item : listTeaInfo)
            {
                item.print();
            }
            std::cout << "学生信息:" << std::endl;
            for (auto item : listStuInfo)
            {
                item.print();
            }
            std::cout << "end:=============" << std::endl;
        }
    };

    static inline void UserFaceInfoToFaceLibsInfo(
        const UserFaceInfo_S& src,
        FaceLibsInfo_S&       dst)
    {
        dst.nId       = src.nId;
        dst.nClassId  = src.nClassId;
        dst.nMemberId = src.nMemberId;

        dst.strName          = src.achName;
        dst.strPicName       = src.achPicName;
        dst.strLocalPicPath  = src.achLocalPicPath;
        dst.strRemotePicPath = src.achRemotePicPath;
        dst.strPicMd5        = src.achPicMd5;
        dst.strPicDate       = src.achPicDate;

        dst.nIdentity   = src.nIdentity;
        dst.fConfidence = src.fConfidence;

        // 注意：UserFaceInfo_S 不含特征，这里只清空
        dst.vfData1.clear();
        dst.vfData2.clear();
    }

    static inline void FaceLibsInfoToUserFaceInfo(
        const FaceLibsInfo_S& src,
        UserFaceInfo_S&       dst)
    {
        dst.nId       = src.nId;
        dst.nClassId  = src.nClassId;
        dst.nMemberId = src.nMemberId;

        // string → char[]（安全拷贝，自动截断）
        std::snprintf(dst.achName, sizeof(dst.achName), "%s", src.strName.c_str());
        std::snprintf(dst.achPicName, sizeof(dst.achPicName), "%s", src.strPicName.c_str());
        std::snprintf(dst.achLocalPicPath, sizeof(dst.achLocalPicPath), "%s", src.strLocalPicPath.c_str());
        std::snprintf(dst.achRemotePicPath, sizeof(dst.achRemotePicPath), "%s", src.strRemotePicPath.c_str());
        std::snprintf(dst.achPicMd5, sizeof(dst.achPicMd5), "%s", src.strPicMd5.c_str());
        std::snprintf(dst.achPicDate, sizeof(dst.achPicDate), "%s", src.strPicDate.c_str());

        dst.nIdentity   = src.nIdentity;
        dst.fConfidence = src.fConfidence;

        // UserFaceInfo_S 不包含特征，所以 vfData 不转换
    }

    /* AI服务器信息 */
    struct AiServerInfo_S
    {
        int         nNetStatus; /* 链接状态 */
        std::string strIp;      /* 服务器IP */

        void clear()
        {
            nNetStatus = 0;
            strIp.clear();
        }

        AiServerInfo_S()
        {
            clear();
        }
    };

    /* AI功能开关 */
    struct AiSwitchInfo_S
    {
        int nAiServerSwitch;  /* AI服务器开关 */
        int nTecBehavSwitch;  /* 教师行为分析开关 */
        int nTecAttendSwitch; /* 教师考勤开关 */
        int nPPTSwitch;       /* PPT开关 */
        int nStuBehavSwitch;  /* 学生行为分析开关 */
        int nStuAttendSwitch; /* 学生考勤开关 */
        int nStuCountSwitch;  /* 人数开关 */
        int nTpCallsSwitch;   /* 接打电话开关 */
        int nEmoSwitch;       /* 人脸表情开关 */

        void clear()
        {
            nAiServerSwitch  = 0;
            nTecBehavSwitch  = 0;
            nTecAttendSwitch = 0;
            nPPTSwitch       = 0;
            nStuBehavSwitch  = 0;
            nStuAttendSwitch = 0;
            nStuCountSwitch  = 0;
            nTpCallsSwitch   = 0;
            nEmoSwitch       = 0;
        }

        AiSwitchInfo_S()
        {
            clear();
        }
    };

}    // namespace Ai0630_NS