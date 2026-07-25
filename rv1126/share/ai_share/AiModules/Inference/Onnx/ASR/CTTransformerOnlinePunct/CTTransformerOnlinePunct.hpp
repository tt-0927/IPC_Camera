/**
 * @file CTTransformerOnlinePunct.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-04-22
 *
 * @brief
 */
#pragma once

#include "TextUtils.hpp"
#include "InputDataEXT.hpp"
#include "OutputDataEXT.hpp"
#include "AVInferenceOnnx.hpp"

namespace Inference_NS
{
    typedef struct _ModelMetaData_
    {
        std::unordered_map<std::string, int32_t> token2id;
        std::unordered_map<std::string, int32_t> punct2id;
        std::unordered_map<std::string, std::string> punct_ch2en;
        std::vector<std::string> id2punct;

        int32_t unk_id;
        int32_t dot_id;
        int32_t comma_id;
        int32_t quest_id;
        int32_t pause_id;
        int32_t underline_id;
        int32_t num_punctuations;
    } ModelMetaData_S;

    class CCTTransformerOnlinePunct : public CAVInferenceOnnx
    {
    public:
        CCTTransformerOnlinePunct(std::string strConfigPath);
        ~CCTTransformerOnlinePunct();

        /**
         * @brief 重写父类的解析json模型后处理数据，用于适配不同类型的模型
         * @return true
         * @return false
         */
        bool checkModelProConfig() override;

        /**
         * @brief 获取模型内部的Metadata
         * @return [*]
         */
        virtual bool initMetadata() override;

        /**
         * @brief 推理数据
         * @param [Inference_NS::InputData_S] stInputData: 传入的结构体
         * @param [std::vector<Inference_NS::ASRData_S>&] stASRData: 推理出来的数据
         * @return [*]
         * @note
         */
        bool inference(Inference_NS::InputData_S stInputData, Inference_NS::ASRData_S &stASRData);

        std::string clearArrCache();

    private:
        /* 字符串转为整形 */
        bool stringToInt(std::string strData, int &nOutData);
        /* 字符串切割 */
        void splitStringToVector(const std::string &strInData,
                                 const char cDelim,
                                 std::vector<std::string> *pOutData);

        void vadMask(int nSize, int vad_pos, std::vector<float> &Result);
        bool detect(std::vector<int32_t> vInputData, int nCacheSize, std::vector<int> &vPunction);

    private:
        ModelMetaData_S stMetaData;

        int32_t nVocabSize = 0;
        std::vector<std::string> m_vArrCache; 
        int32_t nCandidateNum= 6;
        int32_t nSegmentSize = 20;
        int32_t nMaxLen = 200;

        std::string m_strEndRes = "";
    };

} // namespace Inference_NS
