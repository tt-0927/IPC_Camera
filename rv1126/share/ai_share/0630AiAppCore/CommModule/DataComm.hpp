#pragma once

#include <opencv2/opencv.hpp>

#include "0630AppExtern.hpp"
#include "CommShareTcp.hpp"
#include "Intern.hpp"
#include "SignalSlot.h"

namespace Ai0630_NS
{
    class DataComm
    {
    public:

        DataComm();
        ~DataComm();

        /**
         * @brief 绑定槽函数
         * @tparam CONTEXT 槽函数所在实例指针
         * @param slot 槽函数指针
         * @return true
         * @return false
         */
        template<typename CONTEXT, typename... Args>
        bool bindFeatureSlot(
            CONTEXT*                          context,
            signal_function<CONTEXT, Args...> slot)
        {
            /* 关联信号与槽 */
            connect(&sig_sendFeatureData,
                    context,
                    slot,
                    false);

            return true;
        }

        template<typename CONTEXT, typename... Args>
        bool bindSlot(
            CONTEXT*                          context,
            signal_function<CONTEXT, Args...> slot)
        {
            /* 关联信号与槽 */
            connect(&sig_sendData,
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
        bool unbindFeatureSig()
        {
            /* 关联信号与槽 */
            disconnect(&sig_sendFeatureData);
            return true;
        }

        bool unbindSig()
        {
            /* 关联信号与槽 */
            disconnect(&sig_sendData);
            return true;
        }

        /**
         * @brief 接受数据
         * @param pchData 数据
         * @param nDataLen 数据长度
         * @param nCode 通讯码
         * @param nRecordTime 录制时长
         * @param nClassId 班级ID
         * @param bSend 是否发送，不发送，就是本地分析
         */
        void recvData(char* pchData,
                      int   nDataLen,
                      int   nCode,
                      int   nRecordTime,
                      int   nClassId,
                      bool  bSend = true);

        /**
         * @brief 接受班级信息，用来发送给服务器进行特诊提取
         * @param stClassInfo 班级信息
         */
        void recvClassData(ClassInfo_S stClassInfo);

        /**
         * @brief 初始化TCP通讯
         * @return BlError_E
         */
        void init(std::string strIp);

        /**
         * @brief 反初始化TCP通讯
         * @return BlError_E
         */
        BlError_E unInit();


    private:

        /**
         * @brief 工具函数：JPEG 编码
         * @param img
         * @param outJpeg
         * @return true
         * @return false
         */
        bool encodeJpeg(const cv::Mat& img, std::vector<uchar>& outJpeg);

        /**
         * @brief 工具函数：构建 CommData_S 包
         * @param strHeader
         * @param pData
         * @param nDataLen
         * @param nUserSize
         * @param pUser
         * @return CommData_S*
         */
        CommData_S* buildCommPacket(const std::string& strHeader,
                                    const void*        pData,
                                    int                nDataLen,
                                    int                nUserSize,
                                    const void*        pUser);

        /**
         * @brief 工具函数：统一发送
         * @param nCode
         * @param pstData
         * @return true
         * @return false
         */
        bool sendPacket(int nCode, CommData_S* pstData);

        /**
         * @brief 发送一张（原图或镜像）
         * @param strHeader
         * @param stUserFaceInfo
         * @param img
         * @param nCode
         * @param logTag
         * @return true
         * @return false
         */
        bool sendFaceFeatureOne(const std::string&    strHeader,
                                const UserFaceInfo_S& stUserFaceInfo,
                                const cv::Mat&        img,
                                int                   nCode,
                                const char*           logTag);

        /**
         * @brief 发送原图 + 镜像
         * @param strHeader
         * @param stUserFaceInfo
         * @param img
         * @param nCode
         */
        void sendFaceFeatureBoth(const std::string&    strHeader,
                                 const UserFaceInfo_S& stUserFaceInfo,
                                 const cv::Mat&        img,
                                 int                   nCode);

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

        /**
         * @brief 回调函数-心跳回调
         * @param stInfo 参数
         * @return BlError_E
         */
        BlError_E heartbeatCallback(COMM_NS::HeartbeatParam_S stInfo);

        /**
         * @brief 解析数据
         * @param nCode
         * @param pCommData
         * @return
         */
        bool parse(int nCode, CommData_S* pCommData);

    private:

        COMM_NS::CommInParam_S m_stParam;

        COMM_NS::CCommBase* m_pComm = nullptr;

        TanSignal<HeaderInfo_S, FaceLibsInfo_S, FaceResult_S>   sig_sendFeatureData;
        TanSignal<HeaderInfo_S, UserHeaderInfo_S, FaceResult_S> sig_sendData;

        std::string    m_strIp;
        AiServerInfo_S m_stAiServerInfo;
    };
}    // namespace Ai0630_NS