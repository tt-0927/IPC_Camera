/**
 * @file ZipFormerJoiner.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-06-06
 * 
 * @brief 
 */
#include "ZipFormerJoiner.hpp"
#include <memory>
#include <cstring>

Inference_NS::CZipFormerJoiner::CZipFormerJoiner(std::string strConfigPath)
    : CAVInferenceRK(strConfigPath)
{
}

Inference_NS::CZipFormerJoiner::~CZipFormerJoiner()
{
}

/* 推理数据 */
bool Inference_NS::CZipFormerJoiner::inference()
{
    if (!m_pModel)
    {
        return false;
    }
    
    /* 运行 */
    if (!m_pModel->run(m_pInputs,
                       m_vInputAttrs.size(),
                       m_pOutputs,
                       m_vOutputAttrs.size()))
    {
        printf("推理失败-运行模型失败\n");
        return false;
    }

    return true;
}
