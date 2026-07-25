/*
 * @FilePath     : ParseBase.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-03-22 16:03:14
 * @LastEditors: weigl 308241951@qq.com
 * @LastEditTime: 2024-12-26 15:57:44
 * @Description  : 解析数据基类
 */
#pragma once

#include "AiManageExtern.hpp"
#include "BlError.h"
#include "ParseDataExtern.hpp"
#include "PlatformExtern.hpp"

namespace ParseData_NS
{
    class CParseBase
    {
    public:

        CParseBase(InParam_S stInfo)
            : m_stParam(stInfo)
        {
        }

        virtual ~CParseBase()
        {
        }

        /* 解析 */
        /**
        * @description: 解析Json数据-错误状态
        * @param [const char *] pchJson: json数据
        * @param [string] &strToken: token值
        * @return [*] BlError_E::OK 成功  其他失败
        * @others:
        */
        virtual BlError_E parsePlatform(const char* pchJson, int& nError, std::string &strError) = 0;

        /**
        * @description: 解析Json数据-获取token值
        * @param [const char *] pchJson: json数据
        * @param [string] &strToken: token值
        * @return [*] BlError_E::OK 成功  其他失败
        * @others:
        */
        virtual BlError_E parseToken(const char* pchJson, std::string& strToken) = 0;

        /**
        * @description: 解析Json数据-获取班级成员信息
        * @param [const char *] pchJson: json数据
        * @param [PlatformManage_NS::DataInfo_S&] stDataInfo: 成员信息结构体
        * @return [*] BlError_E::OK 成功  其他失败
        * @others:
        */
        virtual BlError_E parseClassInfo(const char* pchJson, PlatformManage_NS::DataInfo_S& stDataInfo) = 0;

        /**
         * @brief 解析数据-解析板书识别信息
         * @param [char] *pData: 需要解析的数据
         * @param [AiManage_NS::BoardInfo_S&] stInfo: 板书识别信息
         * @return [*] BlError_E::OK 成功  其他失败
         * @note
         */
        virtual BlError_E parse(char* pData, AiManage_NS::BoardInfo_S& stInfo) = 0;

        /**
         * @brief 解析数据-解析表情识别信息
         * @param [char] *pData: 需要解析的数据
         * @param [AiManage_NS::EmoInfo_S&] stInfo: 表情识别信息
         * @return [*] BlError_E::OK 成功  其他失败
         * @note
         */
        virtual BlError_E parse(char* pData, AiManage_NS::EmoInfo_S& stInfo) = 0;

        /**
         * @brief 解析数据-解析人脸识别信息
         * @param [char] *pData: 需要解析的数据
         * @param [AiManage_NS::FaceInfo_S&] stInfo: 人脸识别信息
         * @return [*] BlError_E::OK 成功  其他失败
         * @note
         */
        virtual BlError_E parse(char* pData, AiManage_NS::FaceInfo_S& stInfo) = 0;

        /**
         * @brief 解析数据-解析轨迹识别信息
         * @param [char] *pData: 需要解析的数据
         * @param [AiManage_NS::TrackInfo_S&] stInfo: 轨迹识别信息
         * @return [*] BlError_E::OK 成功  其他失败
         * @note
         */
        virtual BlError_E parse(char* pData, AiManage_NS::TrackInfo_S& stInfo) = 0;

        /**
         * @brief 解析数据-解析人数识别信息
         * @param [char] *pData: 需要解析的数据
         * @param [AiManage_NS::NumberInfo_S&] stInfo: 人数识别信息
         * @return [*] BlError_E::OK 成功  其他失败
         * @note
         */
        virtual BlError_E parse(char* pData, AiManage_NS::NumberInfo_S& stInfo) = 0;

        /**
         * @brief 解析数据-解析行为识别信息
         * @param [char] *pData: 需要解析的数据
         * @param [AiManage_NS::BehaviorInfo_S&] stInfo: 行为识别信息
         * @return [*] BlError_E::OK 成功  其他失败
         * @note
         */
        virtual BlError_E parse(char* pData, AiManage_NS::BehaviorInfo_S& stInfo) = 0;

        /**
         * @brief *课堂纪律解析
         * @param [char] *pData: 需要解析的数据
         * @param [MoveProbability_S&] stMoveProbability: 混乱度
         * @return [*] BlError_E::OK 成功  其他失败
         * @note
         */
        /*课堂纪律解析*/
        virtual BlError_E parse(char* pData, AiManage_NS::MoveProbability_S& stMoveProbability) = 0;

        /**
         * @brief 解析数据-解析行为识别信息
         * @param [char] *pData: 需要解析的数据
         * @param [int&] nReturn: 返回值
         * @return [*] BlError_E::OK 成功  其他失败
         * @note
         */
        virtual BlError_E parse(char* pData, int& nReturn) = 0;

        /**
         * @brief 解析数据-解析获取Ai服务器IP
         * @param [char] *pData: 需要解析的数据
         * @param [std::string&] strAiServerIp: AI服务器IP
         * @return [*] BlError_E::OK 成功  其他失败
         * @note
         */
        virtual BlError_E parse(char* pData, std::string& strAiServerIp) = 0;

        /**
         * @brief 解析数据-解析获取Ai服务器IP
         * @param [char] *pData: 需要解析的数据
         * @param [AiManage_NS::VodHeartInfo_S&] stVodHeartInfo: Ai心跳信息
         * @return [*] BlError_E::OK 成功  其他失败
         * @note
         */
        virtual BlError_E parse(char* pData, AiManage_NS::VodHeartInfo_S& stVodHeartInfo) = 0;


    public:

        /* 转换 */
        /**
         * @brief 转换数据-转换设备信息
         * @param [DevInfo_S] stDevInfo: 设备信息
         * @param [std::string&] strOutJson: 输出json数据
         * @param [int] nCode: 通信码
         * @return [*] BlError_E::OK 成功  其他失败
         * @note
         */
        virtual BlError_E convert(AiManage_NS::DevInfo_S stDevInfo, std::string& strOutJson, int nCode) = 0;

        /**
         * @brief 转换数据-转换返回信息
         * @param [int] nReturn: 返回值
         * @param [std::string&] strOutJson: 输出json数据
         * @param [int] nCode: 通信码
         * @return [*] BlError_E::OK 成功  其他失败
         * @note
         */
        virtual BlError_E convert(int nReturn, std::string& strOutJson, int nCode) = 0;

        /**
         * @brief 转换数据-截图命令
         * @param [string] strPicPath: 截图保存文件名称
         * @param [int] nChannelNumber: 截图通道号
         * @param [string&] strOutJson:  输出json数据
         * @param [int] nCode: 通信码
         * @return [*] BlError_E::OK 成功  其他失败
         * @note
         */
        virtual BlError_E convert(std::string strPicPath, int nChannelNumber, std::string& strOutJson, int nCode) = 0;

        /**
         * @brief 转换数据-班级人脸信息
         * @param [int] nClassId: 班级ID
         * @param [std::string] strTarPath: 压缩包路径
         * @param [string&] strOutJson:  输出json数据
         * @param [int] nCode: 通信码
         * @return [*] BlError_E::OK 成功  其他失败
         * @note
         */
        virtual BlError_E convert(int nClassId, std::string strTarPath, std::string& strOutJson, int nCode) = 0;

        /**
         * @brief 转换数据-班级人脸信息
         * @param [PlatformManage_NS::DataInfo_S] stDataInfo: 班级数据信息
         * @param [std::string] strTarPath: 压缩包路径
         * @param [string&] strOutJson:  输出json数据
         * @param [int] nCode: 通信码
         * @return [*] BlError_E::OK 成功  其他失败
         * @note
         */
        virtual BlError_E convert(PlatformManage_NS::DataInfo_S stDataInfo, std::string strFilePath, std::string& strOutJson, int nCode) = 0;


        // /**
        //  * @brief 转换数据-转换 心跳结构体
        //  * @param [RecordHeartbit_S] stHeartbitInfo: 心跳结构体
        //  * @param [char*&] pData: 转换成的数据
        //  * @return [*] BlError_E::OK 成功  其他失败
        //  * @note
        //  */
        // virtual BlError_E convert(RecordHeartbit_S stHeartbitInfo, char*& pData) = 0;

        // /**
        //  * @brief 转换数据-转换 录制状态信息结构体
        //  * @param [RecordStatusInfo_S] stRecordStatusInfo: 录制状态信息结构体
        //  * @param [char*&] pData: 转换成的数据
        //  * @return [*] BlError_E::OK 成功  其他失败
        //  * @note
        //  */
        // virtual BlError_E convert(RecordStatusInfo_S stRecordStatusInfo, char*& pData) = 0;

        // /**
        //  * @brief 转换数据-生成通讯返回数据
        //  * @param [char*] pMessage: 需要返回的信息
        //  * @param [int] nCode: 命令码
        //  * @param [BlError_E] enReturn: 返回值
        //  * @param [char*&] pData: 转换数据
        //  * @return [*] BlError_E::OK 成功  其他失败
        //  * @note
        //  */
        // virtual BlError_E convert(char* pMessage, int nCode, BlError_E enReturn, char*& pData) = 0;

        // /**
        //  * @brief 转换数据-转换 编码信息
        //  * @param [RecordEncodeParam_S] stInfo: 编码信息
        //  * @param [char*&] pData: 转换成的数据
        //  * @return [*] BlError_E::OK 成功  其他失败
        //  * @note
        //  */
        // virtual BlError_E convert(RecordEncodeParam_S stInfo, char*& pData) = 0;

    protected:

        InParam_S m_stParam;
    };



}    // namespace ParseData_NS