/*
 * @FilePath     : ParseJson.cpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2023-06-19 11:22:11
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-05-20 16:23:46
 * @Description  : 解析数据-Json
 */
#include "ConvertJson.hpp"

#include "Convert.h"

using namespace Ai0630_NS;

/* 转换函数 */

void Ai0630_NS::dealJson(Json::Object* pRootJson, CommInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert convert(bOutStruct);
    convert.field(pRootJson, "ActionCode", stInfo.nCode);
    convert.field(pRootJson, "OptType", stInfo.nOptType);
    int nUserIntParam = 0;
    convert.field(pRootJson, "UserIntParam", nUserIntParam);
    convert.field(pRootJson, "UserName", stInfo.strUserName);
    convert.field(pRootJson, stInfo.strData);
    convert.field(pRootJson, "Return", stInfo.nReturn);
}

void Ai0630_NS::dealJson(Json::Object* pRootJson, HeaderInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert convert(bOutStruct);
    convert.field(pRootJson, "Code", stInfo.nCode);
    convert.field(pRootJson, "AiFlags", stInfo.nAiFlags);
    convert.field(pRootJson, "NeedPostProcess", stInfo.bNeedPostProcess);
}

void Ai0630_NS::dealJson(Json::Object* pRootJson, AiParamInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert convert(bOutStruct);
    convert.field(pRootJson, "Id", stInfo.nId);
}

void Ai0630_NS::dealJson(Json::Object* pRootJson, Inference_NS::Cls_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert convert(bOutStruct);
    convert.field(pRootJson, "Label", stInfo.nLabel);
    convert.field(pRootJson, "Confidence", stInfo.fConfidence);
}

void Ai0630_NS::dealJson(Json::Object* pRootJson, std::vector<Inference_NS::Cls_S>& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert convert(bOutStruct);
    convert.structure(pRootJson, "Cls", stInfo);
}

void Ai0630_NS::dealJson(Json::Object* pRootJson, Inference_NS::Box_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert convert(bOutStruct);
    convert.field(pRootJson, "X1", stInfo.nX1);
    convert.field(pRootJson, "Y1", stInfo.nY1);
    convert.field(pRootJson, "Y1", stInfo.nX2);
    convert.field(pRootJson, "Y2", stInfo.nY2);
}

void Ai0630_NS::dealJson(Json::Object* pRootJson, std::vector<Inference_NS::Box_S>& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert convert(bOutStruct);
    convert.structure(pRootJson, "Box", stInfo);
}

void Ai0630_NS::dealJson(Json::Object* pRootJson, Inference_NS::Point_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert convert(bOutStruct);
    convert.field(pRootJson, "X", stInfo.nX);
    convert.field(pRootJson, "Y", stInfo.nY);
    convert.field(pRootJson, "Show", stInfo.nShow);
}

void Ai0630_NS::dealJson(Json::Object* pRootJson, std::vector<Inference_NS::Point_S>& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert convert(bOutStruct);
    convert.structure(pRootJson, "Point", stInfo);
}

void Ai0630_NS::dealJson(Json::Object* pRootJson, Inference_NS::ClsData_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert convert(bOutStruct);

    convert.field(pRootJson, "Feature", stInfo.vFeature);
    convert.structure(pRootJson, stInfo.stCls);
    convert.structure(pRootJson, stInfo.vCls);
}

void Ai0630_NS::dealJson(Json::Object* pRootJson, std::vector<Inference_NS::ClsData_S>& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert convert(bOutStruct);
    convert.structure(pRootJson, "ClsData", stInfo);
}

void Ai0630_NS::dealJson(Json::Object* pRootJson, Inference_NS::BoxData_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert convert(bOutStruct);
    convert.field(pRootJson, "Label", stInfo.nLabel);
    convert.field(pRootJson, "Confidence", stInfo.fConfidence);
    convert.structure(pRootJson, stInfo.stBoxs);
}

void Ai0630_NS::dealJson(Json::Object* pRootJson, std::vector<Inference_NS::BoxData_S>& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert convert(bOutStruct);
    convert.structure(pRootJson, "BoxData", stInfo);
}

void Ai0630_NS::dealJson(Json::Object* pRootJson, Inference_NS::PointData_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert convert(bOutStruct);
    convert.field(pRootJson, "Label", stInfo.nLabel);
    convert.field(pRootJson, "Confidence", stInfo.fConfidence);
    convert.structure(pRootJson, stInfo.stBoxs);
    convert.structure(pRootJson, stInfo.vPoints);
}

void Ai0630_NS::dealJson(Json::Object* pRootJson, std::vector<Inference_NS::PointData_S>& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert convert(bOutStruct);
    convert.structure(pRootJson, "PointData", stInfo);
}

void Ai0630_NS::dealJson(Json::Object* pRootJson, AIResult_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert convert(bOutStruct);
    convert.field(pRootJson, "Id", stInfo.nId);
    convert.structure(pRootJson, "vstClsData", stInfo.vstClsData);
    convert.structure(pRootJson, "vstBoxData", stInfo.vstBoxData);
    convert.structure(pRootJson, "vstPointData", stInfo.vstPointData);
}

void Ai0630_NS::dealJson(Json::Object* pRootJson, std::vector<AIResult_S>& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert convert(bOutStruct);
    convert.structure(pRootJson, "AIResult", stInfo);
}

void Ai0630_NS::dealJson(Json::Object* pRootJson, HeadResult_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert convert(bOutStruct);
    convert.structure(pRootJson, "AIHeadDetectResult", stInfo.vstAIHeadDetectResult);
    convert.structure(pRootJson, "AIFastPoseResult", stInfo.vstAIFastPoseResult);
}

void Ai0630_NS::dealJson(Json::Object* pRootJson, FaceResult_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert convert(bOutStruct);
    convert.structure(pRootJson, "AIFaceDetectResult", stInfo.vstAIFaceDetectResult);
    convert.structure(pRootJson, "AIHeadDetectResult", stInfo.vstAIHeadDetectResult);
    convert.structure(pRootJson, "AIFastPoseResult", stInfo.vstAIFastPoseResult);
    convert.structure(pRootJson, "AIFaceFeatureResult", stInfo.vstAIFaceFeatureResult);
    convert.structure(pRootJson, "AIHumanFeatureResult", stInfo.vstAIHumanFeatureResult);
    convert.structure(pRootJson, "AIFaceEmotionResult", stInfo.vstAIFaceEmotionResult);
}

void Ai0630_NS::dealJson(Json::Object* pRootJson, FaceLibsInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert convert(bOutStruct);
    convert.field(pRootJson, "Id", stInfo.nId);
    convert.field(pRootJson, "ClassId", stInfo.nClassId);
    convert.field(pRootJson, "MemberId", stInfo.nMemberId);
    convert.field(pRootJson, "Name", stInfo.strName);
    convert.field(pRootJson, "PicName", stInfo.strPicName);
    convert.field(pRootJson, "LocalPicPath", stInfo.strLocalPicPath);
    convert.field(pRootJson, "RemotePicPath", stInfo.strRemotePicPath);
    convert.field(pRootJson, "PicMd5", stInfo.strPicMd5);
    convert.field(pRootJson, "PicDate", stInfo.strPicDate);
    convert.field(pRootJson, "Identity", stInfo.nIdentity);
    convert.field(pRootJson, "Confidence", stInfo.fConfidence);
}

void Ai0630_NS::dealJson(Json::Object* pRootJson, ClassInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert convert(bOutStruct);
    convert.field(pRootJson, "ClassId", stInfo.nClassId);
    convert.field(pRootJson, "ClassName", stInfo.strClassName);
    convert.structure(pRootJson, "TeaInfo", stInfo.listTeaInfo);
    convert.structure(pRootJson, "StuInfo", stInfo.listStuInfo);
}