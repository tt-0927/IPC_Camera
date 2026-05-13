/*
 * @FilePath     : LLMModelOpt.cpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-05-29 17:30:28
 * @LastEditors: leiyy leiyy@kfb.cn
 * @LastEditTime: 2026-01-19 16:13:18
 * @Description  : RK模型操作
 */
#include "LLMModelOpt.hpp"

#include <fstream>


// 初始化静态成员
Inference_NS::CLLmModelOpt* Inference_NS::CLLmModelOpt::s_currentInstance = nullptr;

Inference_NS::CLLmModelOpt::CLLmModelOpt(RKLLMParam RkLLMParam,std::string& strImg_Start,std::string&  strImg_End, std::string&  strImg_Content)
    : m_RkLLMParam(RkLLMParam), m_streamCallback(nullptr)
{

    memset(&m_Rkllm_InPut, 0, sizeof(RKLLMInput));
    memset(&m_RkLlm_InFer_Params, 0, sizeof(RKLLMInferParam));

    /* 多模态输入中图像的起始位置 */
    m_RkLLMParam.img_start    = strImg_Start.c_str();
     /*多模态输入中图像的结束位置*/
    m_RkLLMParam.img_end      = strImg_End.c_str();
    /* 指向图像内容 */
    m_RkLLMParam.img_content  = strImg_Content.c_str();

}

Inference_NS::CLLmModelOpt::~CLLmModelOpt()
{
    unInit();
}

// 静态回调函数
int Inference_NS::CLLmModelOpt::staticCallback(RKLLMResult *result, void *userdata, LLMCallState state)
{
    if (s_currentInstance) {
        return s_currentInstance->handleCallback(result, state);
    }
    return 0;
}

// 实例回调处理
int Inference_NS::CLLmModelOpt::handleCallback(RKLLMResult *result, LLMCallState state)
{
    if (state == RKLLM_RUN_FINISH)
    {
        // 推理完成，通知回调
        if (m_streamCallback) {
            return m_streamCallback("", true);
        }
    }
    else if (state == RKLLM_RUN_ERROR)
    {
        printf("运行错误\n");
        // 错误时也通知完成
        if (m_streamCallback) {
            return m_streamCallback("", true);
        }
    }
    else if (state == RKLLM_RUN_NORMAL)
    {
        // 累积完整文本
        m_currentOutputText += result->text;
        
        // 实时回调输出
        if (m_streamCallback) {
            return m_streamCallback(result->text, false);
        }
    }
    
    return 0;
}

/* 初始化模型 */
bool Inference_NS::CLLmModelOpt::init()
{
    if (m_bInitialized)
    {
        /* 模型已被初始化 */
        return true;
    }
    bool bRet = true;
    int  nRet = 0;


    //设置llm参数及初始化
    RKLLMParam Param;
    Param  = rkllm_createDefaultParam();

    Param.model_path                    = m_RkLLMParam.model_path;
    Param.top_k                         = m_RkLLMParam.top_k;
    Param.temperature                   = m_RkLLMParam.temperature;
    Param.max_new_tokens                = m_RkLLMParam.max_new_tokens;
    Param.max_context_len               = m_RkLLMParam.max_context_len;
    Param.skip_special_token            = m_RkLLMParam.skip_special_token;
    Param.img_start                     = m_RkLLMParam.img_start;
    Param.img_end                       = m_RkLLMParam.img_end;
    Param.img_content                   = m_RkLLMParam.img_content;
    Param.extend_param.base_domain_id   = m_RkLLMParam.extend_param.base_domain_id;

    std::chrono::high_resolution_clock::time_point t_start_us = std::chrono::high_resolution_clock::now();

    // 设置当前实例
    s_currentInstance = this;

    nRet = rkllm_init(&m_LlmHandle, &Param, staticCallback);
    if (nRet == 0){
        printf("rkllm init success\n");
    } else {
        printf("rkllm init failed\n");
        if (m_LlmHandle != nullptr)
        {
            {
                LLMHandle _tmp = m_LlmHandle;
                m_LlmHandle = nullptr;
                rkllm_destroy(_tmp);
            }
        }
        return bRet;

    }
    std::chrono::high_resolution_clock::time_point t_load_end_us = std::chrono::high_resolution_clock::now();

    auto load_time = std::chrono::duration_cast<std::chrono::microseconds>(t_load_end_us - t_start_us);
    printf("%s: LLM Model loaded in %8.2f ms\n", __func__, load_time.count() / 1000.0);

    /* 初始化成功 */
    m_bInitialized = true;

    bRet = false;
    
    return bRet;
}

/* 反初始化模型 */
bool Inference_NS::CLLmModelOpt::unInit()
{
    if (m_bInitialized)
    {
         // 清除当前实例
        if (s_currentInstance == this) {
            s_currentInstance = nullptr;
        }

        rkllm_destroy(m_LlmHandle);
        m_bInitialized = false;
        m_currentOutputText.clear();

        return false;
    }

    return true;
}

/* 设置模型输入属性 */
bool Inference_NS::CLLmModelOpt::setInputAttrs(std::string& strSystem_Prompt,RKLLMInferParam &rkllm_infer_params)
{
    if (m_bInitialized)
    {
        int nRet = 0;
        std::string strInfer_System_Prompt;
        /*llm infer 参数结构体*/
        m_RkLlm_InFer_Params = rkllm_infer_params;

        strInfer_System_Prompt = "<|im_start|>system\n"+ strSystem_Prompt +"<|im_end|>\n";

        nRet = rkllm_set_chat_template(m_LlmHandle, strInfer_System_Prompt.c_str(), "<|im_start|>user\n", "<|im_end|>\n<|im_start|>assistant\n");
        if (nRet != 0)
        {
            printf("rkllm_set_chat_template!\n");
        }
        return false;
    }
    return true;
}

/* 设置模型输入要推理的内容参数 */
bool Inference_NS::CLLmModelOpt::setInput(RKLLMInput &rkllm_input)
{
    if (m_bInitialized)
    {
        /*llm输入参数*/
        m_Rkllm_InPut = rkllm_input;
        
        return false;
    }
    return true;
}

/* 获取输出结果文本 */
bool Inference_NS::CLLmModelOpt::getOutputResultText(std::string& output)
{
    if (m_bInitialized)
    {
        output = m_currentOutputText;
        m_currentOutputText.clear();
        
        return false ;
    }
    return true ;
}


/* clear模型输入缓存 */
bool Inference_NS::CLLmModelOpt::ClearInputCache(int keep_system_prompt)
{
    if (m_bInitialized)
    {
        int nRet = 0;
        nRet = rkllm_clear_kv_cache(m_LlmHandle, keep_system_prompt, nullptr, nullptr);
        if (nRet != 0)
        {
            printf("clear kv cache failed!\n");
            return true;
        }
        return false;
    }
    return true;
}

/* 设置流式输出回调函数 */
void Inference_NS::CLLmModelOpt::setStreamCallback(StreamCallback callback)
{
    m_streamCallback = callback;
}

/* 运行模型 */
bool Inference_NS::CLLmModelOpt::run(StreamCallback callback)
{
    int nRet = 0;
    if (m_bInitialized)
    {

        // 设置回调函数
        m_streamCallback = callback;
        
        // 清空之前的输出
        m_currentOutputText.clear();

        /* 运行 (会阻塞，推理完毕返回)*/
        nRet = rkllm_run(m_LlmHandle, &m_Rkllm_InPut, &m_RkLlm_InFer_Params, NULL);
        if (nRet != 0)
        {
            return true;
        }

        return false;
    }

    return true;
}
