#pragma once

#include <atomic>
#include <list>
#include <thread>

#include "0630AppExtern.hpp"
#include "BlError.h"
#include "CallQueue.hpp"
#include "SignalSlot.h"

namespace Ai0630_NS
{
    class GetClassInfo
    {
    public:

        GetClassInfo();
        ~GetClassInfo();

        template<typename CONTEXT, typename... Args>
        bool bindSlot(
            CONTEXT*                          context,
            signal_function<CONTEXT, Args...> slot)
        {
            /* 关联信号与槽 */
            connect(&sig_sendFaceData,
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
            disconnect(&sig_sendFaceData);
            return true;
        }

        /**
         * @brief 设置平台IP地址
         * @param strIp IP地址
         * @return BlError_E
         */
        BlError_E setPlatformIp(std::string strIp);

        /**
         * @brief 获取并更新班级信息
         * @return BlError_E
         */
        BlError_E update_classInfo();

        /**
         * @brief 获取班级信息
         * @return m_stClassInfo
         */
        ClassInfo_S getClassInfo()
        {
            return m_stClassInfo;
        }

        /**
         * @brief 获取班级信息
         * @return m_stClassInfo
         */
        void setClassInfo(ClassInfo_S stInfo)
        {
            m_stClassInfo = stInfo;
        }

    private:

        /**
         * @brief 解析Json数据-获取错误
         * @param pchJson
         * @param nError
         * @param strError
         * @return BlError_E
         */
        BlError_E parsePlatform(const char* pchJson, int& nError, std::string& strError);

        /**
         * @brief 解析Json数据-获取token值
         * @param pchJson
         * @param strToken
         * @return BlError_E
         */
        BlError_E parseToken(const char* pchJson, std::string& strToken);

        /**
         * @brief 解析Json数据-获取班级成员信息
         * @param pchJson
         * @param stClassInfo
         * @return BlError_E
         */
        BlError_E parseClassInfo(const char* pchJson, ClassInfo_S& stClassInfo);

        /**
         * @brief 获取文件后缀
         * @param strFilePath
         * @return std::string
         */
        std::string getFileExtension(const std::string& strFilePath);

        /**
         * @brief 判断md5值是否存在
         * @param list
         * @param strMd5ToFind
         * @return true
         * @return false
         */
        bool Md5Exists(const std::list<FaceLibsInfo_S>& list,
                       const std::string&               strMd5ToFind);

        /**
         * @brief 校验是否需要更新
         * @param stNewInfo
         * @param stOldInfo
         * @return true
         * @return false
         */
        bool needsUpdate(const ClassInfo_S& stNewInfo, const ClassInfo_S& stOldInfo);

        /**
         * @brief 平台-获取Token值
         * @param pArgv
         * @return BlError_E
         */
        BlError_E getToken(void* pArgv = nullptr);

        /**
         * @brief 获取并更新班级信息
         * @return BlError_E
         */
        BlError_E updateClassInfo();

        /**
         * @brief 获取班级信息
         * @param stClassInfo
         * @return BlError_E
         */
        BlError_E getClassInfo(ClassInfo_S& stClassInfo);

        /**
         * @brief 下载人脸图片信息
         * @param stClassInfo
         * @return BlError_E
         */
        BlError_E downloadPicInfo(ClassInfo_S& stClassInfo);

        /**
         * @brief ping 服务器
         * @return [*]
         * @note
         */
        bool ping();

        /**
         * @brief 发送请求线程
         */
        void senderLoop();

    private:

        /* 参数 */
        std::mutex m_paramMutex;

        std::string m_strPlatformIp = "172.16.25.79";
        std::string m_strToken;


        std::atomic<bool> m_bRunning;
        CallQueue         m_callQueue;
        std::thread       m_sendThread;
        std::string       m_strPathBase = "/";

        ClassInfo_S m_stClassInfo;

        TanSignal<ClassInfo_S> sig_sendFaceData;
    };

}    // namespace Ai0630_NS
