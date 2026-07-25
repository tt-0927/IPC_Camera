#pragma once

#include "CommBase.hpp"
#include "DevManageExtern.hpp"
#include "Intern.hpp"
#include "SignalSlot.h"

namespace Ai0630_NS
{
    class DevComm
    {

    public:

        DevComm();
        ~DevComm();

        /**
         * @brief 绑定槽函数
         * @tparam CONTEXT 槽函数所在实例指针
         * @param slot 槽函数指针
         * @return true
         * @return false
         */
        template<typename CONTEXT, typename... Args>
        bool bindSlot(
            CONTEXT*                          context,
            signal_function<CONTEXT, Args...> slot)
        {
            /* 关联信号与槽 */
            connect(&sig_setServerIp,
                    context,
                    slot,
                    false);

            return true;
        }

        /**
         * @brief 解绑定信号
         * @return true
         * @return false
         */
        bool unbindSig()
        {
            /* 关联信号与槽 */
            disconnect(&sig_setServerIp);
            return true;
        }

    private:

        /**
         * @brief 初始化
         * @return BlError_E
         */
        BlError_E init();

        /**
         * @brief 发送数据
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
         * @brief 通讯数据回调函数
         * @param stInfo
         * @return * BlError_E
         */
        BlError_E dataCallback(COMM_NS::DataParam_S stInfo);

        /**
         * @brief 通讯状态回调函数
         * @param stInfo
         * @return BlError_E
         */
        BlError_E statusCallback(COMM_NS::StatusParam_S stInfo);

    private:

        /* 设备通讯类 */
        COMM_NS::CCommBase* m_pTcpComm = nullptr;

        TanSignal<std::string> sig_setServerIp;
    };

}    // namespace Ai0630_NS