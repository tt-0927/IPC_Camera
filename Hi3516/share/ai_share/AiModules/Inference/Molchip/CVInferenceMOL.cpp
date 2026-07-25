/**
 * @file CVInferenceMol.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-03-06
 *
 * @brief 眸芯芯片的部署代码框架
 */
#include "CVInferenceMOL.hpp"
#include <unordered_map>

#include <cstring>

Inference_NS::CCVInferenceMOL::CCVInferenceMOL(std::string strConfigPath)
    : m_strConfigPath(strConfigPath)
{
}

Inference_NS::CCVInferenceMOL::~CCVInferenceMOL()
{
    unInit();
}

/* 初始化 */
bool Inference_NS::CCVInferenceMOL::init()
{
    if (!m_pModel)
    {
        /* 解析json模型参数 */
        if (! checkModelConfig())
        {
            return false;
        };
        
        /* 创建模型操作类 */
        m_pModel = new CModelOpt(m_stModelInitInfo.strModelPath);
        if (m_pModel)
        {
            if (!m_pModel->init())
            {
                return false;
            }

            /* 初始化值 */
            m_pModel->getModelDesc(m_stModelDesc);

            m_nInputNum = m_stModelDesc.ioDesc.inputNum;
            m_nOutputNum = m_stModelDesc.ioDesc.outputNum;
            /* 初始化输入参数 */
            if (m_nInputNum > 0)
            {
                m_vInputs.resize(m_nInputNum);
            }

            /* 初始化输出参数 */
            if (m_nOutputNum > 0)
            {
                m_vOutputs.resize(m_nOutputNum);
            }

            /* 打印相关的信息 */
            if (m_stModelInitInfo.showNpuInfo)
            {
                /* 打印npu信息 */
                m_pModel->showModelNpu();
            }
            if (m_stModelInitInfo.showOpInfo)
            {
                /* 打印算子情况 */
                m_pModel->showModelOpt();
            }

            /* 初始化参数 */
            return initParams();
        }
    }

    return false;
}

/* 反初始化 */
bool Inference_NS::CCVInferenceMOL::unInit()
{
    if (m_vInputs.size() > 0)
    {
        for (int i = 0; i < m_stModelDesc.ioDesc.inputNum; i++)
        {
            if (m_vInputs[i].dataIn.size > 0)
            {
                FY_MPI_SYS_MmzFree(m_vInputs[i].dataIn.phyAddr, reinterpret_cast<void *>(m_vInputs[i].dataIn.virAddr));
                m_vInputs[i].dataIn.size = 0;
            }
        }
        m_vInputs.clear();
        m_nInputNum = 0;
    }
    if (m_vOutputs.size() > 0)
    {
        for (int i = 0; i < m_stModelDesc.ioDesc.outputNum; i++)
        {
            if (m_vOutputs[i].dataOut.size > 0)
            {
                FY_MPI_SYS_MmzFree(m_vOutputs[i].dataOut.phyAddr, reinterpret_cast<void *>(m_vOutputs[i].dataOut.virAddr));
                m_vOutputs[i].dataOut.size = 0;
            }
        }
        m_vOutputs.clear();
        m_nOutputNum = 0;
    }

    if (m_pModel)
    {
        delete m_pModel;
        m_pModel = nullptr;
    }

    return true;
}

/* 图片的输入格式转换 */
bool chooseType(const std::string stInputType, E_FY_PixelFormat &enPicFormat)
{
    static const std::unordered_map<std::string, int> formatMap = {
        {"YUV_400", E_FY_PIXEL_FORMAT_YUV_400},
        {"YUV_SEMIPLANAR_420", E_FY_PIXEL_FORMAT_YUV_SEMIPLANAR_420},
        {"YVU_SEMIPLANAR_420", E_FY_PIXEL_FORMAT_YVU_SEMIPLANAR_420},
        {"YUYV_PACKED_422", E_FY_PIXEL_FORMAT_YUYV_PACKED_422},
        {"UYVY_PACKED_422", E_FY_PIXEL_FORMAT_UYVY_PACKED_422},
        {"RGB_888_PACKED", E_FY_PIXEL_FORMAT_RGB_888_PACKED},
        {"BGR_888_PACKED", E_FY_PIXEL_FORMAT_BGR_888_PACKED},
        {"RGB_888_PLANAR", E_FY_PIXEL_FORMAT_RGB_888_PLANAR},
        {"BGR_888_PLANAR", E_FY_PIXEL_FORMAT_BGR_888_PLANAR},
        {"S8C1", E_FY_PIXEL_FORMAT_S8C1},
        {"S8C2_PACKAGE", E_FY_PIXEL_FORMAT_S8C2_PACKAGE},
        {"S8C2_PLANAR", E_FY_PIXEL_FORMAT_S8C2_PLANAR},
        {"S16C1", E_FY_PIXEL_FORMAT_S16C1},
        {"U8C1", E_FY_PIXEL_FORMAT_U8C1},
        {"U16C1", E_FY_PIXEL_FORMAT_U16C1},
        {"S32C1", E_FY_PIXEL_FORMAT_S32C1},
        {"U32C1", E_FY_PIXEL_FORMAT_U32C1},
        {"U64C1", E_FY_PIXEL_FORMAT_U64C1},
        {"S64C1", E_FY_PIXEL_FORMAT_S64C1},
        {"YVU_PLANAR_444", E_FY_PIXEL_FORMAT_YVU_PLANAR_444},
        {"YUV_PLANAR_444", E_FY_PIXEL_FORMAT_YUV_PLANAR_444},
        {"BUTT", E_FY_PIXEL_FORMAT_BUTT}};

    auto it = formatMap.find(stInputType);
    if (it != formatMap.end())
    {
        enPicFormat = E_FY_PixelFormat(it->second);
        return true;
    }
    else
    {
        return false;
    }
}

/* 校验模型配置文件的公共信息 */
bool Inference_NS::CCVInferenceMOL::checkModelConfig()
{
    /* 读取json文件 */
    std::ifstream File(m_strConfigPath);
    if (!File)
    {
        printf("[无法打开json文件]: %s\n", m_strConfigPath.c_str());
        return false;
    }
    /* 读取文件内容到 std::string */
    std::string strJson((std::istreambuf_iterator<char>(File)),
                        std::istreambuf_iterator<char>());
    const char *pchJson = strJson.c_str();
    /* 解析Json数据-获取错误 */
    if (NULL == pchJson)
    {
        printf("传入参数异常\n");
        return false;
    }

    Json::Object *pJsonHandle = NULL;
    pJsonHandle = Json::init(pchJson);
    bool bRet = true;

    /* 获取模型地址 */
    bRet = Json::get(pJsonHandle, "model_path", m_stModelInitInfo.strModelPath);
    if (!bRet)
    {
        printf("解析model_path字段失败\n");
        goto EXIT;
    }

    if (!checkModelPreConfig())
    {
        printf("json配置文件[%s], 预处理部分解析异常\n", m_strConfigPath.c_str());
        bRet = false;
        goto EXIT;
    }

    if (!checkModelProConfig())
    {
        printf("json配置文件[%s], 后处理部分解析异常\n", m_strConfigPath.c_str());
        bRet = false;
        goto EXIT;
    }

    /* 将json数据赋值给模型内部 */
    m_stModelInitInfo.bAipp = (1==m_nAIPP);
    if(m_stModelInitInfo.bAipp)
    {
        InputInfo_S stInputInfo;
        stInputInfo.nPicWidth = m_vImageInputSize[0];
        stInputInfo.nPicHeight = m_vImageInputSize[1];
        stInputInfo.nRoiWidth = m_vImageInputSize[0];
        stInputInfo.nRoiHeigh = m_vImageInputSize[1];
        /* 设置模型输入大小 */
        m_nLimitWidth = m_vModelInputSize[0];
        m_nLimitHeight = m_vModelInputSize[1];
        m_nLimitChannel = m_nChannel;
        if (!chooseType(strType, stInputInfo.enPicFormat))
        {
            printf("输入的图片格式json-type[%s]设置失败\n", strType.c_str());
            goto EXIT;
        }
        m_stModelInitInfo.vInputInfo.push_back(stInputInfo);
    }

EXIT:
    if (pJsonHandle)
    {
        Json::deinit(pJsonHandle);
        pJsonHandle = NULL;
    }
    return bRet;
}

/* 校验模型配置文件中的预处理信息 */
bool Inference_NS::CCVInferenceMOL::checkModelPreConfig()
{
    /* 读取json文件 */
    std::ifstream File(m_strConfigPath);
    if (!File)
    {
        printf("[无法打开json文件]: %s\n", m_strConfigPath.c_str());
        return false;
    }
    /* 读取文件内容到 std::string */
    std::string strJson((std::istreambuf_iterator<char>(File)),
                        std::istreambuf_iterator<char>());
    const char *pchJson = strJson.c_str();
    /* 解析Json数据-获取错误 */
    if (NULL == pchJson)
    {
        printf("传入参数异常\n");
        return false;
    }

    Json::Object *pJsonHandle = NULL;
    Json::Object *pJsonData = NULL;
    Json::Object *pJsonDataItem = NULL;
    Json::Object *pItemObject = NULL;
    bool bRet = true;
    int nSize = 0;
    int i;
    int nSizeItem, nMean, nStd;

    pJsonHandle = Json::init(pchJson);

    pJsonData = Json::get(pJsonHandle, "pre_process");
    if (!pJsonData)
    {
        printf("解析[data]字段失败\n");
        bRet = false;
        goto EXIT;
    }

    /* 0、获取AIPP是否开启 */
    bRet = Json::get(pJsonData, "aipp", m_nAIPP);
    if (!bRet && ((m_nAIPP != 0) || (m_nAIPP != 1)))
    {
        printf("解析aipp字段失败\n");
        goto EXIT;
    }

    /* 1、获取模型输入大小限制 */
    pJsonDataItem = Json::get(pJsonData, "model_size");
    if (!pJsonDataItem)
    {
        printf("解析[model_size]字段失败\n");
        bRet = false;
        goto EXIT;
    }
    nSize = Json::Array::size(pJsonDataItem);
    if (nSize <= 0)
    {
        printf("解析[数组大小异常]\n");
        bRet = false;
        goto EXIT;
    }
    m_vModelInputSize.clear();
    for (i = 0; i < nSize; i++)
    {
        /* 获取数组的节点 */
        pItemObject = Json::Array::get(pJsonDataItem, i);
        if (NULL == pItemObject)
        {
            printf("获取数组节点失败\n");
            bRet = false;   
            goto EXIT;
        }

        bRet = Json::Value::get(pItemObject, nSizeItem);
        if (!bRet)
        {
            printf("解析[image_size]字段失败\n");
            goto EXIT;
        }
        m_vModelInputSize.push_back(nSizeItem);
    }
    if (m_vModelInputSize.size() != 2)
    {
        printf("输入图片的[model_size]参数，必须为[高,宽] 两个参数\n");
        bRet = false;
        goto EXIT;
    }
    pJsonDataItem = Json::get(pJsonData, "image_size");
    if (!pJsonDataItem)
    {
        printf("解析[image_size]字段失败\n");
        bRet = false;
        goto EXIT;
    }
    nSize = Json::Array::size(pJsonDataItem);
    if (nSize <= 0)
    {
        printf("解析[数组大小异常]\n");
        bRet = false;
        goto EXIT;
    }
    m_vImageInputSize.clear();
    for (i = 0; i < nSize; i++)
    {
        /* 获取数组的节点 */
        pItemObject = Json::Array::get(pJsonDataItem, i);
        if (NULL == pItemObject)
        {
            printf("获取数组节点失败\n");
            bRet = false;
            goto EXIT;
        }

        bRet = Json::Value::get(pItemObject, nSizeItem);
        if (!bRet)
        {
            printf("解析[image_size]字段失败\n");
            goto EXIT;
        }
        m_vImageInputSize.push_back(nSizeItem);
    }
    if (m_vImageInputSize.size() != 2)
    {
        printf("输入图片的[image_size]参数，必须为[高,宽] 两个参数\n");
        bRet = false;
        goto EXIT;
    }
    /* 2、图片通道 */
    bRet = Json::get(pJsonData, "channel", m_nChannel);
    if (!bRet)
    {
        printf("解析channel字段失败\n");
        goto EXIT;
    }
    /* 3、输入是数据的格式 */
    bRet = Json::get(pJsonData, "type", strType);
    if (!bRet)
    {
        printf("解析type字段失败\n");
        goto EXIT;
    }
    /* 4、归一化-均值 */
    pJsonDataItem = Json::get(pJsonData, "mean");
    if (!pJsonDataItem)
    {
        printf("解析[pJsonData]字段失败\n");
        bRet = false;
        goto EXIT;
    }
    nSize = Json::Array::size(pJsonDataItem);
    if (nSize <= 0)
    {
        printf("解析[数组大小异常]\n");
        bRet = false;
        goto EXIT;
    }
    for (int i = 0; i < nSize; i++)
    {
        /* 获取数组的节点 */
        pItemObject = Json::Array::get(pJsonDataItem, i);
        if (NULL == pItemObject)
        {
            printf("获取数组节点失败\n");
            bRet = false;
            goto EXIT;
        }

        bRet = Json::Value::get(pItemObject, nMean);
        if (!bRet)
        {
            printf("解析[mean]字段失败\n");
            goto EXIT;
        }
        m_vMean.push_back(nMean);
    }
    /* 5、归一化-方差 */
    pJsonDataItem = Json::get(pJsonData, "std");
    if (!pJsonDataItem)
    {
        printf("解析[pJsonData]字段失败\n");
        bRet = false;
        goto EXIT;
    }
    nSize = Json::Array::size(pJsonDataItem);
    if (nSize <= 0)
    {
        printf("解析[数组大小异常]\n");
        bRet = false;
        goto EXIT;
    }
    for (int i = 0; i < nSize; i++)
    {
        /* 获取数组的节点 */
        pItemObject = Json::Array::get(pJsonDataItem, i);
        if (NULL == pItemObject)
        {
            printf("获取数组节点失败\n");
            bRet = false;
            goto EXIT;
        }

        bRet = Json::Value::get(pItemObject, nStd);
        if (!bRet)
        {
            printf("解析[std]字段失败\n");
            goto EXIT;
        }
        m_vStd.push_back(nStd);
    }
    /* 6、图片缩放时，是否填充 */
    bRet = Json::get(pJsonData, "padding", m_nPadding);
    if (!bRet)
    {
        printf("解析padding字段失败\n");
        goto EXIT;
    }

EXIT:
    if (pJsonHandle)
    {
        Json::deinit(pJsonHandle);
        pJsonHandle = NULL;
    }
    return bRet;
}

/* 校验模型配置文件中的模型推理信息 */
bool Inference_NS::CCVInferenceMOL::checkModelInferConfig()
{
    return true;
}

/* 校验模型配置文件中的后处理信息 */
bool Inference_NS::CCVInferenceMOL::checkModelProConfig()
{
    return true;
}

/* 获取输入图片限制 */
bool Inference_NS::CCVInferenceMOL::getSizeLimit(int nIndex, int &nWidth, int &nHeight, int &nChannel)
{
    if (m_pModel &&
        m_vInputs.size() > nIndex)
    {
        T_FY_BlobDesc *pBlobDesc = &m_stModelDesc.ioDesc.out[nIndex];
        if (pBlobDesc->tensor.numDims > 3)
        {
            if (pBlobDesc->tensor.format == E_FY_FORMAT_NCHW)
            {
                nChannel = pBlobDesc->tensor.dims[1];
                nHeight = pBlobDesc->tensor.dims[2];
                nWidth = pBlobDesc->tensor.dims[3];
            }
            else if (pBlobDesc->tensor.format == E_FY_FORMAT_NHWC)
            {
                nHeight = pBlobDesc->tensor.dims[1];
                nWidth = pBlobDesc->tensor.dims[2];
                nChannel = pBlobDesc->tensor.dims[3];
            }
            return true;
        }
    }

    return false;
}

int getImageSize(E_FY_PixelFormat fmt, uint32_t width, uint32_t height)
{
    int size = 0;
    switch (fmt)
    {
    case E_FY_PIXEL_FORMAT_U8C1:
        size = width * height * 1;
        break;
    case E_FY_PIXEL_FORMAT_U16C1:
        size = width * height * 2;
        break;
    case E_FY_PIXEL_FORMAT_YUV_SEMIPLANAR_420:
        size = width * height * 3 / 2;
        break;
    case E_FY_PIXEL_FORMAT_YUV_400:
        size = width * height;
        break;
    default:
        size = 0;
        printf("Not supported format %d\n", fmt);
        break;
    }
    return size;
}

static int getBlobSize(T_FY_BlobDesc *pBlob)
{
    uint32_t blobByteSize = 0;
    int byteUnitVec[] = {4, 2, 1, 4, 1, 2, 4, 8, 8, 1}; // 对应 E_FY_DataType
    int unitSize = 0;
    int64_t unitNum = 1;
    switch (pBlob->type)
    {
    case E_FY_BLOB_DATA:
        unitSize = byteUnitVec[pBlob->tensor.dataType];
        for (int idx = 0; idx < pBlob->tensor.numDims; idx++)
        {
            unitNum *= pBlob->tensor.dims[idx];
        }
        blobByteSize = unitSize * unitNum;
        break;
    case E_FY_BLOB_IMAGE:
        unitSize = 1;
        unitNum = getImageSize(pBlob->img.picFormat, pBlob->img.picWidthStride, pBlob->img.picHeightStride);
        blobByteSize = unitSize * unitNum;
        break;
    case E_FY_BLOB_IMAGE_WITH_PRE_PROC:
        perror("BLOB_IMAGE_WITH_PRE_PROC need to be converted as BLOB_IMAGE!");
        break;
    default:
        break;
    }
    return blobByteSize;
}

/* 初始化输入输出参数 */
bool Inference_NS::CCVInferenceMOL::initParams()
{
    int nRet = 0;
    /* 设置输入的相关信息 */
    if (m_vInputs.size() > 0)
    {
        for (int i = 0; i < m_nInputNum; i++)
        {
            T_FY_BlobDesc *pBlobDesc = &m_stModelDesc.ioDesc.in[i];
            int nLimitChannel = 0;
            int nLimitHeight = 0;
            int nLimitWidth = 0;
            /* 判断是否可以获取到模型输入的高宽信息 */
            if (!m_stModelInitInfo.bAipp && (pBlobDesc->type == E_FY_BLOB_IMAGE_WITH_PRE_PROC))
            {
                printf("模型的AIPP功能已开启，但输入配置bAipp为[%d]\n", m_stModelInitInfo.bAipp);
                return false;
            }
            else if (pBlobDesc->type == E_FY_BLOB_DATA)
            {
                if (pBlobDesc->tensor.numDims > 3)
                {
                    if (pBlobDesc->tensor.format == E_FY_FORMAT_NCHW || pBlobDesc->tensor.format == E_FY_FORMAT_UNDEFINED)
                    {
                        nLimitChannel = pBlobDesc->tensor.dims[1];
                        nLimitHeight = pBlobDesc->tensor.dims[2];
                        nLimitWidth = pBlobDesc->tensor.dims[3];
                    }
                    else if (pBlobDesc->tensor.format == E_FY_FORMAT_NHWC)
                    {
                        nLimitHeight = pBlobDesc->tensor.dims[1];
                        nLimitWidth = pBlobDesc->tensor.dims[2];
                        nLimitChannel = pBlobDesc->tensor.dims[3];
                    }
                    else
                    {
                        printf("模型输入[%s],格式非[NCHW]或[NHWC]\n", pBlobDesc->tensor.name);
                        return false;
                    }

                    if (i == 0)
                    {
                        m_nLimitWidth = nLimitWidth;
                        m_nLimitHeight = nLimitHeight;
                        m_nLimitChannel = nLimitChannel;
                    }
                    printf("[%s]输入图片限制 [%d]x[%d]x[%d]\n",
                           pBlobDesc->tensor.name,
                           nLimitWidth,
                           nLimitHeight,
                           nLimitChannel);
                }
                /* 分配内存区域 */
                m_vInputs[i].descIn = m_stModelDesc.ioDesc.in[i]; /* 将模型内部的输入信息，赋值给定义的模型输入 */
                int nSize = getBlobSize(&m_vInputs[i].descIn);
                nRet = m_pModel->allocMmzMemory(&m_vInputs[i].dataIn, nSize, 8, E_FY_MEM_VMM_CACHED);
            }
            else if (pBlobDesc->type == E_FY_BLOB_IMAGE_WITH_PRE_PROC)
            {
                /* 设置模型输入高宽和图片格式 */
                m_vInputs[i].descIn.type = E_FY_BLOB_IMAGE;
                m_vInputs[i].descIn.img.picFormat = m_stModelInitInfo.vInputInfo[i].enPicFormat;
                m_vInputs[i].descIn.img.picWidth = m_stModelInitInfo.vInputInfo[i].nPicWidth;
                m_vInputs[i].descIn.img.picHeight = m_stModelInitInfo.vInputInfo[i].nPicHeight;
                m_vInputs[i].descIn.img.picWidthStride = m_stModelInitInfo.vInputInfo[i].nPicWidth;
                m_vInputs[i].descIn.img.picHeightStride = m_stModelInitInfo.vInputInfo[i].nPicHeight;
                m_vInputs[i].descIn.img.roi.x = m_stModelInitInfo.vInputInfo[i].nRoiX;
                m_vInputs[i].descIn.img.roi.y = m_stModelInitInfo.vInputInfo[i].nRoiY;
                m_vInputs[i].descIn.img.roi.width = m_stModelInitInfo.vInputInfo[i].nRoiWidth;
                m_vInputs[i].descIn.img.roi.height = m_stModelInitInfo.vInputInfo[i].nRoiHeigh;
                /* 分配内存区域 */
                nRet = m_pModel->allocMmzMemory(&m_vInputs[i].dataIn, getBlobSize(&m_vInputs[i].descIn), 8, E_FY_MEM_VMM_CACHED);

                printf("输入图片限制 [%d]x[%d]\n",
                       m_stModelInitInfo.vInputInfo[i].nPicWidth,
                       m_stModelInitInfo.vInputInfo[i].nPicHeight);
            }
        }
    }
    /* 设置输出的相关信息 */
    if (m_vOutputs.size() > 0)
    {
        for (int i = 0; i < m_nOutputNum; i++)
        {
            T_FY_BlobDesc *pBlobDesc = &m_stModelDesc.ioDesc.out[i];

            int nSize = getBlobSize(pBlobDesc);
            nRet = m_pModel->allocMmzMemory(&m_vOutputs[i].dataOut, nSize, 8, E_FY_MEM_VMM_CACHED);

            int nLimitChannel = 0;
            int nLimitHeight = 0;
            int nLimitWidth = 0;
            if (pBlobDesc->type == E_FY_BLOB_DATA)
            {
                printf("模型输出[%s] [", pBlobDesc->tensor.name);
                for (int nI = 0; nI < pBlobDesc->tensor.numDims; nI++)
                {
                    printf("[%ld]", pBlobDesc->tensor.dims[nI]);
                    if (nI < (pBlobDesc->tensor.numDims - 1))
                    {
                        printf("x");
                    }
                }
                printf("\n");
            }
        }
    }

    return true;
}

/* 能否推理的使用前判断 */
bool Inference_NS::CCVInferenceMOL::inferenceInfe(int nImgSize)
{
    if (!m_pModel)
    {
        printf("推理失败-模型未初始化或者初始化失败\n");
        return false;
    }

    if (m_nLimitHeight <= 0 ||
        m_nLimitWidth <= 0 ||
        m_nLimitChannel <= 0)
    {
        printf("推理失败-模型限制数据异常 [%d]x[%d]x[%d]\n",
               m_nLimitWidth,
               m_nLimitHeight,
               m_nLimitChannel);
        return false;
    }

    if (nImgSize <= 0)
    {
        printf("推理失败-传入的数据大小nImgSize[%d]小于0\n", nImgSize);
        return false;
    }

    int nDstSize = m_vInputs[0].dataIn.size;

    /* 图片输入为float字节大小，nDstSize为像素大小 */
    nImgSize /= sizeof(float);

    if (nImgSize != nDstSize)
    {
        printf("模型[%s]需要的大小与输入图片大小不一致 nImgSize[%d] nDstSize[%d]\n",
            m_stModelInitInfo.strModelPath.c_str(),
               nImgSize,
               nDstSize);
        return false;
    }
    return true;
}
