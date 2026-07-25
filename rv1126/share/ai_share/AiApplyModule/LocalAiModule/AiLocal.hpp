/*
 * @FilePath     : AiLocal.hpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2024-01-22 09:06:59
 * @LastEditors: weigl 308241951@qq.com
 * @LastEditTime: 2024-12-18 11:10:05
 * @Description  : 本地AI分析
 */
#pragma once

#include "AiLocalExtern.hpp"
#include "LocalBehavior.hpp"
#include "LocalDisciplineGather.hpp"
#include "LocalHeadCount.hpp"

namespace AiLocal_NS
{
    class CAiLocal
    {
    public:

        CAiLocal(returnDataFunc pHeadCount, returnDataFunc pBehavior, returnDataFunc pDiscipline);
        ~CAiLocal();

        /**
         * @brief 初始化分析功能
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note
         */
        BlError_E initHeadCount();
        BlError_E initBehavior();
        BlError_E initDiscipline();

        /**
         * @brief 反初始化分析功能
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note
         */
        BlError_E uninitHeadCount();
        BlError_E uninitBehavior();
        BlError_E uninitDiscipline();

        /**
         * @brief 添加分析人数统计数据
         * @param [CommDataInfo_S*] *pstData: 数据
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note 内部释放pchData
         */
        BlError_E analyseCountStudent(AiManage_NS::CommDataInfo_S* pstData);

        /**
         * @brief 添加分析学生行为分析数据
         * @param [CommDataInfo_S*] *pstData: 数据
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note 内部释放pchData
         */
        BlError_E analyseStudentBehavior(AiManage_NS::CommDataInfo_S* pstData);

        /**
         * @brief 添加课堂纪律分析数据
         * @param [CommDataInfo_S*] *pstData: 数据
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note 内部释放pchData
         */
        BlError_E analyseDiscipline(AiManage_NS::CommDataInfo_S* pstData);

        /**
         * @brief 清空数据
         * @return [*]
         * @note
         */
        BlError_E clearData();

    private:

        /* 锁 */
        std::mutex m_headCountMutex;
        std::mutex m_behaviorMutex;
        std::mutex m_disciplineMutex;

        returnDataFunc m_pHeadCount  = nullptr;
        returnDataFunc m_pBehavior   = nullptr;
        returnDataFunc m_pDiscipline = nullptr;

        CLocalHeadCount*        m_pLocalHeadCount  = nullptr;
        CLocalBehavior*         m_pLocalBehavior   = nullptr;
        CLocalDisciplineGather* m_pLocalDiscipline = nullptr;
    };

}    // namespace AiLocal_NS
