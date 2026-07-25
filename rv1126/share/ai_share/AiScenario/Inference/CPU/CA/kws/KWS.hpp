/*
 * @FilePath     : KWS.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-06-05 10:39:26
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-06-07 09:23:43
 * @Description  :
 */
#pragma once

#include <algorithm>
#include <cstdint>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

#include "CAInferenceCPU.hpp"
#include "kws/display.h"
#include "kws/keyword-spotter.h"
#include "kws/parse-options.h"

namespace InferenceV1_0_NS
{
    class CKWS : public CCAInferenceCPU
    {
    public:

        /**
         * @brief 构造
         * @param [std::string] strTokensPath: Tokens值文件路径
         * @param [std::string] strEncoderPath: 编码模型文件路径
         * @param [std::string] strDecoderPath: 解码模型文件路径
         * @param [std::string] strJoinerPath: 拼接模型文件路径
         * @param [std::string] strKeywordsFilePath: 唤醒词文件路径
         * @note
         */
        CKWS(std::string strTokens,
             std::string strEncoder,
             std::string strDecoder,
             std::string strJoiner,
             std::string strKeywordsFile);


        /**
         * @brief 初始化
         * @return [*]
         * @note
         */
        bool init() override;

        /**
         * @brief 反初始化
         * @return [*]
         * @note
         */
        bool unInit() override;

        /**
         * @brief 推理数据
         * @param [Mat] inMat:传入的图片数据
         * @param [std::string&] strOutData: 输出的处理结果
         * @return [*]
         * @note
         */
        bool inference(AiScenario_NS::CAData_S stInData, std::string& strOutData) override;

    private:

        /**
         * @brief 辅助函数：将给定位深度的样本转换为浮点数
         * @param [const int8_t*] pchSampleData: 样本数据
         * @param [int] nBitDepth: 位深
         * @return [*]
         * @note
         */
        float convertSampleToFloat(const int8_t* pchSampleData, int nBitDepth);

        /**
         * @brief 主转换函数：将PCM数据转换为浮点数据
         * @param [const std::vector<int8_t>&] vfPcmData: 需要转换的PCM数据
         * @param [int] nBitDepth: 位深
         * @param [int] nChannels: 通道数
         * @param [bool] bToMono: 是否转换成单通道
         * @param [std::vector<float>&] vfOutData: 转换后的输出数据
         * @return [*]
         * @note
         */
        bool convertPCMToFloat(
            const std::vector<int8_t>& vfPcmData,
            int                        nBitDepth,
            int                        nChannels,
            bool                       bToMono,
            std::vector<float>&        vfOutData);


    private:

        sherpa_onnx::ParseOptions                    m_po;
        sherpa_onnx::KeywordSpotterConfig            m_config;
        std::unique_ptr<sherpa_onnx::KeywordSpotter> m_pSpotter;
        std::unique_ptr<sherpa_onnx::OnlineStream>   m_pStream;


        std::string m_strTokens;
        std::string m_strEncoder;
        std::string m_strDecoder;
        std::string m_strJoiner;
        std::string m_strKeywordsFile;
    };

}    // namespace InferenceV1_0_NS
