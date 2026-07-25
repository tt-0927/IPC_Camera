/*
 * @FilePath     : ResultsSaveJson.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-03-28 09:25:55
 * @LastEditors: weigl 308241951@qq.com
 * @LastEditTime: 2024-12-20 15:26:30
 * @Description  :
 */
#pragma once

#include "ResultsBase.hpp"

namespace ResultsModule_NS
{

    class CResultsSaveJson : public CResultsBase
    {
    public:

        CResultsSaveJson(InParam_S stInfo);
        ~CResultsSaveJson();

    protected:

        /**
         * @brief 结束处理-热词提取信息
         * @param [void*] pParam: 不定参数，不同子类可能需要不同的参数类型
         * @return [*] 
         */
        BlError_E endDeal_hotwordExtInfo(const void* pParam);

        /**
         * @brief 结束处理-课堂信息
         * @param [void*] pParam: 不定参数，不同子类可能需要不同的参数类型
         * @return [*]
         * @note
         */
        BlError_E endDeal_classSummaryInfo(const void* pParam);

        /**
         * @brief 结束处理-教师信息
         * @param [void*] pParam: 不定参数，不同子类可能需要不同的参数类型
         * @return [*]
         * @note
         */
        BlError_E endDeal_teacherInfo(const void* pParam);

        /**
         * @brief 结束处理-学生信息
         * @param [void*] pParam: 不定参数，不同子类可能需要不同的参数类型
         * @return [*]
         * @note
         */
        BlError_E endDeal_studentInfo(const void* pParam);

        /**
         * @brief 结束处理-考勤信息
         * @param [void*] pParam: 不定参数，不同子类可能需要不同的参数类型
         * @return [*]
         * @note
         */
        BlError_E endDeal_attendanceInfo(const void* pParam);

         /**
         * @brief 处理学生人脸识别分析数据的处理
         * @param [FaceInfo_S] stInfo: 分析后的数据
         * @return [*]
         * @note 
         */
        BlError_E endDeal_stFaceDetecrDetecr(AiManage_NS::FaceInfo_S stInfo);

        /**
         * @brief 处理学生回答问题人脸识别数据的处理
         * @param [FaceInfo_S] stInfo: 分析后的数据
         * @return [*]
         * @note 默认什么都不处理，如果子类需要，直接重写函数
         */
        BlError_E endDeal_stAsFaceDetecrDetecr(AiManage_NS::FaceInfo_S stInfo);

        /**
         * @brief 处理教师人脸识别分析数据的处理
         * @param [FaceInfo_S] stInfo: 分析后的数据
         * @return [*]
         * @note 
         */
        BlError_E endDeal_teFaceDetecrDetecr(AiManage_NS::FaceInfo_S stInfo);

        /**
         * @brief 读取置信度阈值
         * @param [AiConfidenceTh_S&] stAiConfidenceTh: 需要读取的结构体
         * @return [*]
         * @note 
         */
        BlError_E readConfidenceTh(char *pchFilePath, AiConfidenceTh_S& stAiConfidenceTh);

    private:

        /**
         * @brief 写文件
         * @param [char*] pchFilePath: 文件路径
         * @param [char*] pchJsonData: 文件内容
         * @return [*]
         * @note
         */
        BlError_E write_toFile(const char* pchFilePath, const char* pchJsonData);

        /**
         * @brief 读取文件中的Json数据
         * @param [char*] pchFilePath: 文件路径
         * @return [char*] pchJsonData: 文件内容
         * @note
         */
        char* readJson_from_file(const char* pchFilePath);
    };

}    // namespace ResultsModule_NS
