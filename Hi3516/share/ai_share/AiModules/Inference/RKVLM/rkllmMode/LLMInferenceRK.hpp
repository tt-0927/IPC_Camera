/*
 * @Author: leiyy leiyy@kfb.cn
 * @Date: 2025-09-19 17:03:34
 * @LastEditors: leiyy leiyy@kfb.cn
 * @LastEditTime: 2025-10-13 15:46:18
 * @FilePath: /rv1126new/share/ai_share/AiModules/Inference/RKVLM/rkllmMode/LLMInferenceRK.hpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
/**
 * @file LLMInferenceRK.hpp
 * @author leiyy (leyy@kfb.cn)
 * @date 2025-09-08 15:35:49
 * 
 * @brief 
 */
#pragma once

#include <string.h>
#include <unistd.h>
#include <string>
#include <fstream>
#include <iostream>
#include <csignal>
#include <vector>
#include <chrono>
#ifdef JSON_ENABLE
#include "JsonInterfase.h"
#else
#include "Json.h"
#endif
#include "LLMModelOpt.hpp"

#include "rkllm.h"


namespace Inference_NS
{
    class CLLMInferenceRK
    {
    public:
        CLLMInferenceRK(std::string &strConfigPath);

        ~CLLMInferenceRK();

        /**
         * @brief 初始化
         * @return [*]
         * @note
         */
        bool init();

        /**
         * @brief 反初始化
         * @return [*]
         * @note
         */
        bool unInit();

        /**
         * @brief 初始化参数
         * @return [*]
         * @note
         */
        bool initParams();

        /**
         * @brief 清除模型的输入缓存
         * @param  keep_system_prompt :0-清除，1保持
         * @return [*]
         * @note
         */
        bool clearModelInputCache(int keep_system_prompt);

        /**
         * @brief 推理执行，可循环调用
         * @param  isText :是否纯文本模式
         * @param [std::string &Text: 传入的文本内容
         * @param  float *Img_Vec :要推理的图片向量
         * @param std::string& OutText 结果的文本内容
         * @param callback: 流式输出回调函数
         
         * @return [*]
         * @note
         */
        bool run( bool isText,std::string &InputText, float *Img_Vec,std::string& OutText, 
                StreamCallback callback = nullptr);

        /**
         * @brief 设置流式输出回调函数
         * @param callback: 回调函数
         * @return [*]
         * @note
         */
        void setStreamCallback(StreamCallback callback);

        /**
         * @brief 校验模型配置文件的公共信息
         * @return [*]
         */
        bool checkModelConfig();

    protected:
        std::string m_strConfigPath;             /* 模型json配置路径 */
        std::string m_strModelPath;              /* 模型路径 */
        CLLmModelOpt *m_pModel = nullptr;        /* 模型操作句柄 */
        RKLLMParam m_RkLLMParam;                 /*llm参数*/

        int m_nImageHeight;                     /*输入图像高*/
        int m_nImageWidth;                      /*输入图像宽*/
        int m_nImageTokenNum;                  /*输入图像TOKEN_NUM*/

        std::string  m_strImg_Start;            /* 多模态输入中图像的起始位置 */
        std::string  m_strImg_End;              /*多模态输入中图像的结束位置*/
        std::string  m_strImg_Content;          /* 指向图像内容 */

        int m_nKeep_History;                    /* 是否保存推理上下文记忆纪录 */ 
        
        std::string m_strSystemPrompt;        /*llm模板的System_Prompt参数*/

    };

} // namespace std

