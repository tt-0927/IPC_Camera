/*
 * @FilePath     : LLMModelOpt.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-05-29 17:30:28
 * @LastEditors: leiyy leiyy@kfb.cn
 * @LastEditTime: 2025-10-13 15:38:33
 * @Description  : RK模型操作
 */
#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <chrono>
#include <string>
#include <functional>
#include <map>
#include <mutex>

#include "rkllm.h"


namespace Inference_NS
{
   // 定义流式输出回调函数类型
    using StreamCallback = std::function<int(const std::string& text, bool is_finished)>;
    class CLLmModelOpt
    {
    public:

        CLLmModelOpt(RKLLMParam RkLLMParam,std::string& strImg_Start,std::string&  strImg_End, std::string&  strImg_Content);
        ~CLLmModelOpt();

        /**
         * @brief 初始化模型
         * @return [*]
         * @note
         */
        bool init();

        /**
         * @brief 反初始化模型
         * @return [*]
         * @note
         */
        bool unInit();

        /**
         * @brief 设置模型输入属性
         * @param std::string& strSystem_Prompt: 输入的System字段参数
         * @param RKLLMInferParam &rkllm_infer_params: 输入参数
         * @return [*]
         * @note
         */
        bool setInputAttrs(std::string& strSystem_Prompt,RKLLMInferParam &rkllm_infer_params);

        /**
         * @brief 设置模型输入要推理的内容参数
         * @param RKLLMInput &rkllm_input: 输入参数
         * @return [*]
         * @note
         */
        bool setInput(RKLLMInput &rkllm_input);

        /**
         * @brief 获取输出结果文本
         * @param std::string& output: 输出结果内容
         * @return [*]
         * @note
         */
        bool getOutputResultText(std::string& output);

        /**
         * @brief  clear模型输入缓存 
         * @param keep_system_prompt: 1 to retain, 0 to clear
         * @return [*]
         * @note
         */
        bool ClearInputCache(int keep_system_prompt);
        
        /**
         * @brief 运行模型
         * @param StreamCallback callback: 流式输出回调函数
         * @return [*]
         * @note
         */
        bool run(StreamCallback callback = nullptr);

        /**
         * @brief 设置流式输出回调函数
         * @param StreamCallback callback: 回调函数
         * @return [*]
         * @note
         */
        void setStreamCallback(StreamCallback callback);


    public:
        /* 模型句柄 */
        LLMHandle m_LlmHandle = nullptr;
         /*llm参数*/
        RKLLMParam m_RkLLMParam;
         /*llm输入参数*/
        RKLLMInput m_Rkllm_InPut;
        /*llm infer 参数结构体*/
        RKLLMInferParam m_RkLlm_InFer_Params;
        /*llm 推理结果*/
        RKLLMResult* m_Resultl;

    private:

        // 流式输出回调函数
        StreamCallback m_streamCallback;
        // 当前推理的完整文本
        std::string m_currentOutputText;
        
        bool   m_bInitialized = false;

        // 静态实例指针（用于回调）
        static CLLmModelOpt* s_currentInstance;

        // 静态回调函数
        static int staticCallback(RKLLMResult *result, void *userdata, LLMCallState state);
        
        // 实例回调处理
        int handleCallback(RKLLMResult *result, LLMCallState state);

    };

}    // namespace std