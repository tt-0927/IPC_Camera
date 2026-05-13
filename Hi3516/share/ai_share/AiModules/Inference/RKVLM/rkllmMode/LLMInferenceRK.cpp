/**
 * @file LLMInferenceRK.cpp
 * @author leiyy (leiyy@kfb.cn)
 * @date 2025-09-08 15:35:49
 * 
 * @brief 
 */
#include "LLMInferenceRK.hpp"

#include <cstring>

Inference_NS::CLLMInferenceRK::CLLMInferenceRK(std::string &strConfigPath)
    : m_strConfigPath(strConfigPath)
{

}

Inference_NS::CLLMInferenceRK::~CLLMInferenceRK()
{
    unInit();
}

/* 初始化 */
bool Inference_NS::CLLMInferenceRK::init()
{
    if (!m_pModel)
    {
        /* 解析json模型参数 */
        if (checkModelConfig())
        {
            return true;
        }

        /* 创建模型操作类 */
        m_pModel = new CLLmModelOpt(m_RkLLMParam,m_strImg_Start,m_strImg_End,m_strImg_Content);
        if (m_pModel)
        {
            /*初始化模型*/
            if (m_pModel->init())
            {
                return true;
            }
        }

        if (initParams())
        {
            return true;
        }

        return false;

    }

    return true;
}

/* 反初始化 */
bool Inference_NS::CLLMInferenceRK::unInit()
{
    bool bRet = true;

    if (m_pModel)
    {
        // 检查反初始化是否成功
        if (!m_pModel->unInit())
        {
            bRet = false;
        }

        delete m_pModel;
        m_pModel = nullptr;
    }

    return bRet;
}

/* 校验模型配置文件的公共信息 */
bool Inference_NS::CLLMInferenceRK::checkModelConfig()
{
    /* 读取json文件 */
    std::ifstream File(m_strConfigPath);
    if (!File)
    {
        printf("[无法打开json文件]: %s\n", m_strConfigPath.c_str());
        return true;
    }

    /* 读取文件内容到 std::string */
    std::string strJson((std::istreambuf_iterator<char>(File)),
                        std::istreambuf_iterator<char>());
    const char *pchJson = strJson.c_str();
    /* 解析Json数据-获取错误 */
    if (NULL == pchJson)
    {
        printf("传入参数异常\n");
        return true;
    }

    Json::Object *pJsonHandle = NULL;
    pJsonHandle = Json::init(pchJson);
    bool bRet;

    /* 获取模型地址 */
    bRet = Json::get(pJsonHandle, "model_path", m_strModelPath);
    if (!bRet)
    {
        printf("解析model_path字段失败\n");
        if (pJsonHandle){
            Json::deinit(pJsonHandle);
            pJsonHandle = NULL;
        }
        return true;
    }
    m_RkLLMParam.model_path  = m_strModelPath.c_str();

    /*Token生成的Top-K采样参数*/
    bRet = Json::get(pJsonHandle, "top_k", m_RkLLMParam.top_k);
    if (!bRet)
    {
        printf("解析top_k字段失败\n");
        if (pJsonHandle){
            Json::deinit(pJsonHandle);
            pJsonHandle = NULL;
        }
        return true;
    }

    /*温度参数 - 控制生成随机性的程度*/
    float m_fTemperature = 0.8f;    // 默认值
    bRet = Json::get(pJsonHandle, "temperature", m_fTemperature);
    if (!bRet)
    {
        printf("解析temperature字段失败\n");
        if (pJsonHandle){
            Json::deinit(pJsonHandle);
            pJsonHandle = NULL;
        }
        return true;
    }
    m_RkLLMParam.temperature = m_fTemperature;

    /*要生成的新token的最大数量*/
    bRet = Json::get(pJsonHandle, "max_new_tokens",m_RkLLMParam.max_new_tokens);
    if (!bRet)
    {
        printf("解析max_new_tokens字段失败\n");
        if (pJsonHandle){
            Json::deinit(pJsonHandle);
            pJsonHandle = NULL;
        }
        return true;
    }

    /*上下文窗口中的最大token数量*/
    bRet = Json::get(pJsonHandle, "max_context_len", m_RkLLMParam.max_context_len);
    if (!bRet)
    {
        printf("解析max_context_len字段失败\n");
        if (pJsonHandle){
            Json::deinit(pJsonHandle);
            pJsonHandle = NULL;
        }
        return true;
    }

    /*生成过程中是否跳过特殊符号*/
    int skip_special_token;
    bRet = Json::get(pJsonHandle, "skip_special_token", skip_special_token);
    if (!bRet)
    {
        printf("解析skip_special_token字段失败\n");
        if (pJsonHandle){
            Json::deinit(pJsonHandle);
            pJsonHandle = NULL;
        }
        return true;
    }
    if(skip_special_token)
        m_RkLLMParam.skip_special_token = true;
    else
        m_RkLLMParam.skip_special_token = false;

    /* 多模态输入中图像的起始位置 */
    bRet = Json::get(pJsonHandle, "img_start", m_strImg_Start);
    if (!bRet)
    {
        printf("解析img_start字段失败\n");
        if (pJsonHandle){
            Json::deinit(pJsonHandle);
            pJsonHandle = NULL;
        }
        return true;
    }

    /* 多模态输入中图像的结束位置 */
    bRet = Json::get(pJsonHandle, "img_end", m_strImg_End);
    if (!bRet)
    {
        printf("解析img_end字段失败\n");
        if (pJsonHandle){
            Json::deinit(pJsonHandle);
            pJsonHandle = NULL;
        }
        return true;
    }

    /* 指向图像内容 */
    bRet = Json::get(pJsonHandle, "img_content", m_strImg_Content);
    if (!bRet)
    {
        printf("解析img_content字段失败\n");
        if (pJsonHandle){
            Json::deinit(pJsonHandle);
            pJsonHandle = NULL;
        }
        return true;
    }

    /*基础领域ID*/
    bRet = Json::get(pJsonHandle, "base_domain_id", m_RkLLMParam.extend_param.base_domain_id);
    if (!bRet)
    {
        printf("解析base_domain_id字段失败\n");
        if (pJsonHandle){
            Json::deinit(pJsonHandle);
            pJsonHandle = NULL;
        }
        return true;
    }

    /*是否保存推理上下文记忆纪录*/
    bRet = Json::get(pJsonHandle, "keep_history", m_nKeep_History);
    if (!bRet)
    {
        printf("解析base_domain_id字段失败\n");
        if (pJsonHandle){
            Json::deinit(pJsonHandle);
            pJsonHandle = NULL;
        }
        return true;
    }

    /*输入图像高*/
    bRet = Json::get(pJsonHandle, "image_height", m_nImageHeight);
    if (!bRet)
    {
        printf("解析image_height字段失败\n");
        if (pJsonHandle){
            Json::deinit(pJsonHandle);
            pJsonHandle = NULL;
        }
        return true;
    }

    /*输入图像宽*/
    bRet = Json::get(pJsonHandle, "image_width", m_nImageWidth);
    if (!bRet)
    {
        printf("解析image_width字段失败\n");
        if (pJsonHandle){
            Json::deinit(pJsonHandle);
            pJsonHandle = NULL;
        }
        return true;
    }

    /*输入图像TOKEN_NUM*/
    bRet = Json::get(pJsonHandle, "token_num", m_nImageTokenNum);
    if (!bRet)
    {
        printf("解析token_num字段失败\n");
        if (pJsonHandle){
            Json::deinit(pJsonHandle);
            pJsonHandle = NULL;
        }
        return true;
    }

    /* llm模板的System_Prompt参数 */
    bRet = Json::get(pJsonHandle, "system_prompt", m_strSystemPrompt);
    if (!bRet)
    {
        printf("解析system_prompt字段失败\n");
        if (pJsonHandle){
            Json::deinit(pJsonHandle);
            pJsonHandle = NULL;
        }
        return true;
    }

    return false;
}


/* 初始化参数 */
bool Inference_NS::CLLMInferenceRK::initParams()
{
    RKLLMInferParam rkllm_infer_params;

    memset(&rkllm_infer_params, 0, sizeof(RKLLMInferParam));

    rkllm_infer_params.mode = RKLLM_INFER_GENERATE;
    rkllm_infer_params.keep_history =  m_nKeep_History;

    if (m_pModel)
    {
        /*设置推理模型参数*/
        if (m_pModel->setInputAttrs(m_strSystemPrompt,rkllm_infer_params))
        {
            return true;
        }

        return false;
    }

    return true;
}

/* 清除模型的输入缓存 */
bool Inference_NS::CLLMInferenceRK::clearModelInputCache(int keep_system_prompt)
{
     if(m_pModel->ClearInputCache(keep_system_prompt))
    {
        return true;
    }

    return false;
}

/* 推理执行，可循环调用 */
bool Inference_NS::CLLMInferenceRK::run( bool isText,std::string &InputText, float *Img_Vec,std::string& OutText, StreamCallback callback)
{
    if (!m_pModel || InputText.empty())
    {
        printf("推理失败-模型未初始化或者初始化失败\n");
        return true;
    }

    RKLLMInput rkllm_input; 
    memset(&rkllm_input, 0, sizeof(RKLLMInput));

    if (isText) 
    {
        rkllm_input.input_type = RKLLM_INPUT_PROMPT;
        rkllm_input.role = "user";
        rkllm_input.prompt_input = (char *)InputText.c_str();
    } else {
        rkllm_input.input_type = RKLLM_INPUT_MULTIMODAL;
        rkllm_input.role = "user";
        rkllm_input.multimodal_input.prompt = (char *)InputText.c_str();
        rkllm_input.multimodal_input.image_embed = Img_Vec;
        rkllm_input.multimodal_input.n_image_tokens = m_nImageTokenNum;
        rkllm_input.multimodal_input.n_image = 1;
        rkllm_input.multimodal_input.image_height = m_nImageHeight;
        rkllm_input.multimodal_input.image_width = m_nImageWidth;
    }

    if (m_pModel->setInput(rkllm_input))
    {
         return true;
    }
    if (m_pModel->run(callback))
    {
        return true;
    }
    /* 获取输出结果文本 */
    if(m_pModel->getOutputResultText(OutText))
    {
        return true;
    }

    return false;
}

/* 设置流式输出回调函数 */
void Inference_NS::CLLMInferenceRK::setStreamCallback(StreamCallback callback)
{
    if (m_pModel) {
        m_pModel->setStreamCallback(callback);
    }
}