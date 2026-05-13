/**
 * @file SenceVoice.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-06-06
 *
 * @brief
 */
#pragma once

#include "InputDataEXT.hpp"
#include "OutputDataEXT.hpp"
#include "AVInferenceRK.hpp"
#include <array>

namespace Inference_NS
{
    class CSenceVoice : public CAVInferenceRK
    {
        typedef struct _VocabEntry_
        {
            int index;
            char *token;
        } VocabEntry_S;
    
    public:
        CSenceVoice(std::string strConfigPath);
        ~CSenceVoice();

        /**
         * @brief 重写父类的解析json模型后处理数据，用于适配不同类型的模型
         * @return true
         * @return false
         */
        bool checkModelProConfig() override;

        /**
         * @brief 特征提取
         * @param [Inference_NS::AVInputData_S] stInputData: 传入的结构体
         * @param [std::vector<Inference_NS::ASRData_S>&] stASRData: 推理出来的数据
         * @return [*]
         * @note
         */
        bool inference(
            Inference_NS::AVInputData_S stInputData,
            Inference_NS::ASRData_S &stASRData);

        /**
         * @brief 设置识别的语言
         * @param nLanguage 语言,{"auto": 0, "zh": 3, "en": 4, "yue": 7, "ja": 11, "ko": 12, "nospeech": 13}
         * @param bTextNormalization 是否标准化（数字转为阿拉伯数字，输出添加标点符号）
         * @return true 
         * @return false 
         */
        bool setLanguage(int nLanguage, bool bTextNormalization);
            
    private:
        /* 去除字符串首尾空格（辅助函数） */
        std::string trim(const std::string &strInData);
        /* 字符串切割，转为std::vector<float> */
        bool stringToFloatVecotr(std::string strData, std::vector<float> &vOutData);
        /* 降帧操作 */

        std::vector<float> applyLFR(const std::vector<float> &vInputData);
        /* 读取词表 */
        bool readVocab(const char *fileName);
        /* 读取逗号分隔的 TXT 文件 */
        bool readCommaSeparatedTXT(const std::string strFilename, std::vector<std::vector<float>>& vvOut);

    private:
        /* 词表 */
        std::string m_strVocabPath;
        std::vector<VocabEntry_S> m_vVocab;

        float m_fAlphaThreshold = 1.0;
        /* 模型配置参数 */
        int nChunkSize = 61;
        int nLeftChunkSize = 5;
        int nRightChunkSize = 3;
        int m_nVocabSize;     /* 词表大小 */
        int m_nLfrWindowSize=7; /* 滑动窗口大小 */
        int m_nLfrWindowShift=6;
        int m_nEncoderOutputSize;
        int m_nDecoderNumBlocks;
        int m_nDecoderKernelSize;
        int m_nInFeatDim = 80; 
        std::string m_strNegMean;
        std::string m_strInvStddev;

        /* 语言选择输入,{"auto": 0, "zh": 3, "en": 4, "yue": 7, "ja": 11, "ko": 12, "nospeech": 13} */
        /* 事件输入,{情绪:1, 事件:2} */
        /* 文本输入提示，{文本归一化：14，不文本归一化:15} */
        std::array<int32_t, 4> m_vPrompt{3, 1, 2, 14};
    };

} // namespace Inference_NS
