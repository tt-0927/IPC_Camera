/*
 * @FilePath     : FaceRecExtern.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-09-18 15:18:38
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-09-23 17:20:19
 * @Description  :
 */
#pragma once

#include <iostream>
#include <list>
#include <vector>

namespace Ai0630_NS
{
    /* 名单库人脸数据结构，包含特征和相关信息 */
    struct FaceLibsInfo_S
    {
        int                nId;
        int                nClassId;         /* 班级ID */
        int                nMemberId;        /* 成员ID */
        std::string        strName;          /* 名字 */
        std::string        strPicName;       /* 图片名称 */
        std::string        strLocalPicPath;  /* 图片路径 */
        std::string        strRemotePicPath; /* 图片路径 */
        std::string        strPicMd5;        /* 图片MD5 */
        std::string        strPicDate;       /* 图片日期 */
        int                nIdentity;        /* 身份：0学生 1老师 */
        float              fConfidence;      /* 人脸置信度 */
        std::vector<float> vfData1;          /* 左右镜像的特征向量（以BLOB形式存储）*/
        std::vector<float> vfData2;          /* 左右镜像的特征向量（以BLOB形式存储）*/

        void clear()
        {
            nId       = 0;
            nClassId  = 0;
            nMemberId = 0;
            strName.clear();
            strPicName.clear();
            strLocalPicPath.clear();
            strRemotePicPath.clear();
            strPicMd5.clear();
            strPicDate.clear();
            nIdentity   = 0;
            fConfidence = 0.0f;
            vfData1.clear();
            vfData2.clear();
        }

        void print() const
        {
            std::cout << "\n名单库人脸信息:=============" << std::endl;
            std::cout << "ID: " << nId << std::endl;
            std::cout << "班级ID: " << nClassId << std::endl;
            std::cout << "成员ID: " << nMemberId << std::endl;
            std::cout << "名字: " << strName << std::endl;
            std::cout << "图片名称: " << strPicName << std::endl;
            std::cout << "本地图片路径: " << strLocalPicPath << std::endl;
            std::cout << "远端图片路径: " << strRemotePicPath << std::endl;
            std::cout << "图片MD5: " << strPicMd5 << std::endl;
            std::cout << "图片日期: " << strPicDate << std::endl;
            std::cout << "身份: " << nIdentity << std::endl;
            std::cout << "置信度: " << fConfidence << std::endl;
            std::cout << "特征向量: vfData1=" << vfData1.size() << std::endl;
            std::cout << "特征向量: vfData2=" << vfData2.size() << std::endl;
            std::cout << "end:=============" << std::endl;
        }
    };

    /* 名单库人脸数据结构，包含特征和相关信息 */
    struct HumanLibsInfo_S
    {
        int                nId;
        int                nClassId;      /* 班级ID */
        int                nEmoType;      /* 表情类型 */
        int                nBehaviorType; /* 行为类型-学生的 */
        int                nPostureType;  /* 姿态类型-老师的 */
        int                nClassTime;    /* 当前录制时长 */
        long long          lTimestamp;    /* 当前时间戳 */
        int                nModelState;   /* 模型状态 0: 未建模 1:建模成功 2:建模失败 */
        float              fConfidence;   /* 置信度 */
        std::vector<float> vfFaceData;    /* 特征向量（以BLOB形式存储）*/
        std::vector<float> vfHumanData;   /* 特征向量（以BLOB形式存储）*/

        void clear()
        {
            nId           = 0;
            nClassId      = 0;
            nEmoType      = 0;
            nBehaviorType = -1;
            nPostureType  = -1;
            nClassTime    = 0;
            lTimestamp    = 0;
            nModelState   = 1;
            fConfidence   = 0.0f;
            vfFaceData.clear();
            vfHumanData.clear();
        }

        void print() const
        {
            std::cout << "\n人类库信息:=============" << std::endl;
            std::cout << "ID: " << nId << std::endl;
            std::cout << "班级ID: " << nClassId << std::endl;
            std::cout << "表情类型: " << nEmoType << std::endl;
            std::cout << "行为类型: " << nBehaviorType << std::endl;
            std::cout << "姿态类型: " << nPostureType << std::endl;
            std::cout << "课堂时间: " << nClassTime << std::endl;
            std::cout << "时间戳: " << lTimestamp << std::endl;
            std::cout << "模型状态: " << nModelState << std::endl;
            std::cout << "置信度: " << fConfidence << std::endl;
            std::cout << "特征向量: vfFaceData=" << vfFaceData.size() << std::endl;
            std::cout << "特征向量: vfHumanData=" << vfHumanData.size() << std::endl;

            // std::cout << "特征向量: " << std::endl;
            // for (size_t i = 0; i < vfData.size(); i++)
            // {
            //     std::cout << vfData.at(i) << ", ";
            // }
            // std::cout << std::endl;
            std::cout << "end:=============" << std::endl;
        }
    };

}    // namespace Ai0630_NS
