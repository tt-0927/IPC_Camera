#pragma once

#include "0630AppExtern.hpp"
#include "CommBase.hpp"
#include "DevManageExtern.hpp"
#include "Intern.hpp"
#include "SignalSlot.h"

namespace Ai0630_NS
{
    class CtrlComm
    {

    public:

        CtrlComm();
        ~CtrlComm();

        /**
         * @brief 初始化TCP通讯
         * @return BlError_E
         */
        void init(std::string strIp);

        /**
         * @brief 获取AI服务器信息
         * @return AiServerInfo_S
         */
        AiServerInfo_S getAiServerInfo()
        {
            return m_stAiServerInfo;
        }

    private:

        /**
         * @brief 发送数据
         * @param pHandle
         * @param stInfo
         * @return BlError_E
         */
        BlError_E send(CommInfo_S stInfo, void* pHandle = nullptr);

        /**
         * @brief 获取设备信息
         * @param stDevInfo
         * @return BlError_E
         */
        BlError_E get_device_info(DevDataInfo_S& stDevInfo);

        /**
         * @brief 回调函数-数据返回
         * @param stInfo 参数
         * @return BlError_E
         */
        BlError_E dataCallback(COMM_NS::DataParam_S stInfo);

        /**
         * @brief 回调函数-链接状态
         * @param stInfo 参数
         * @return BlError_E
         */
        BlError_E statusCallback(COMM_NS::StatusParam_S stInfo);

    private:

        COMM_NS::CommInParam_S m_stParam;

        COMM_NS::CCommBase* m_pComm = nullptr;

        AiServerInfo_S m_stAiServerInfo;
    };
}    // namespace Ai0630_NS