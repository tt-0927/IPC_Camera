/*
 * @FilePath     : ParseJson.h
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2023-06-19 11:21:50
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-04-18 14:55:57
 * @Description  : 解析数据-Json
 */
#pragma once

#include "0630AppExtern.hpp"
#include "Intern.hpp"
#include "JsonInterfase.h"
#include "OutputDataEXT.hpp"

namespace Ai0630_NS
{
    /**
     * @brief 转换函数
     * @param pRootJson json句柄
     * @param stInfo 设备信息
     * @param bOutStruct 是否转换成结构体，false:将结构体转化成json句柄，true:相反
     */
    void dealJson(Json::Object* pRootJson, CommInfo_S& stInfo, bool bOutStruct);
    void dealJson(Json::Object* pRootJson, HeaderInfo_S& stInfo, bool bOutStruct);
    void dealJson(Json::Object* pRootJson, AiParamInfo_S& stInfo, bool bOutStruct);

    /* AI分析 */
    void dealJson(Json::Object* pRootJson, Inference_NS::Cls_S& stInfo, bool bOutStruct);
    void dealJson(Json::Object* pRootJson, std::vector<Inference_NS::Cls_S>& stInfo, bool bOutStruct);
    void dealJson(Json::Object* pRootJson, Inference_NS::Box_S& stInfo, bool bOutStruct);
    void dealJson(Json::Object* pRootJson, std::vector<Inference_NS::Box_S>& stInfo, bool bOutStruct);
    void dealJson(Json::Object* pRootJson, Inference_NS::Point_S& stInfo, bool bOutStruct);
    void dealJson(Json::Object* pRootJson, std::vector<Inference_NS::Point_S>& stInfo, bool bOutStruct);
    void dealJson(Json::Object* pRootJson, Inference_NS::ClsData_S& stInfo, bool bOutStruct);
    void dealJson(Json::Object* pRootJson, std::vector<Inference_NS::ClsData_S>& stInfo, bool bOutStruct);
    void dealJson(Json::Object* pRootJson, Inference_NS::BoxData_S& stInfo, bool bOutStruct);
    void dealJson(Json::Object* pRootJson, std::vector<Inference_NS::BoxData_S>& stInfo, bool bOutStruct);
    void dealJson(Json::Object* pRootJson, Inference_NS::PointData_S& stInfo, bool bOutStruct);
    void dealJson(Json::Object* pRootJson, std::vector<Inference_NS::PointData_S>& stInfo, bool bOutStruct);
    void dealJson(Json::Object* pRootJson, AIResult_S& stInfo, bool bOutStruct);
    void dealJson(Json::Object* pRootJson, std::vector<AIResult_S>& stInfo, bool bOutStruct);
    void dealJson(Json::Object* pRootJson, HeadResult_S& stInfo, bool bOutStruct);
    void dealJson(Json::Object* pRootJson, FaceResult_S& stInfo, bool bOutStruct);

    /* 班级信息 */
    void dealJson(Json::Object* pRootJson, FaceLibsInfo_S& stInfo, bool bOutStruct);
    void dealJson(Json::Object* pRootJson, ClassInfo_S& stInfo, bool bOutStruct);

}    // namespace Ai0630_NS