/*
 *  File Name: rk_human_count_detect.cpp
 *  Created on: 2023年7月13日
 *  Author: wcp
 *  description : 统计输入的视频流中的人数
 *  Modify date: 2023年7月18日
 */

#include "rk_human_count_detect.h"

#include <chrono>

/* 构造函数 -- 初始化变量 */
RK_COUNT_DETECT::RK_COUNT_DETECT()
{
    /* 模型的位置 */
    m_pModelName  = "./weights/human768x640.rknn";
    /* 图片中统计到的人数 */
    nPepleNum     = 0;
    /* 输入视频流的大小和通道 */
    nImgWidth     = 768;
    nImgHeight    = 640;
    nImgChannel   = 3;
    /* 模型需要输入的大小和通道 */
    nModelWidth   = 0;
    nModelHeight  = 0;
    nModelChannel = 3;
    uCtx          = 0;
    /* 初始化需要操作的MAT图片 */
    aImgNew.create(nImgHeight, nImgWidth, CV_8UC1);
    RknnDetectInit();
}

RK_COUNT_DETECT::RK_COUNT_DETECT(char* pModelPath)
{
    /* 模型的位置 */
    m_pModelName  = pModelPath;
    /* 图片中统计到的人数 */
    nPepleNum     = 0;
    /* 输入视频流的大小和通道 */
    nImgWidth     = 768;
    nImgHeight    = 640;
    nImgChannel   = 3;
    /* 模型需要输入的大小和通道 */
    nModelWidth   = 0;
    nModelHeight  = 0;
    nModelChannel = 3;
    uCtx          = 0;
    /* 初始化需要操作的MAT图片 */
    aImgNew.create(nImgHeight, nImgWidth, CV_8UC1);
    RknnDetectInit();
}

RK_COUNT_DETECT::RK_COUNT_DETECT(char* pModelPath, int uInitImgWidth, int uInitImgHeight, int uInitImgChannel)
{
    /* 模型的位置 */
    m_pModelName  = pModelPath;
    /* 图片中统计到的人数 */
    nPepleNum     = 0;
    /* 输入视频流的大小和通道 */
    nImgWidth     = uInitImgWidth;
    nImgHeight    = uInitImgHeight;
    nImgChannel   = uInitImgChannel;
    /* 模型需要输入的大小和通道 */
    nModelWidth   = 0;
    nModelHeight  = 0;
    nModelChannel = 3;
    uCtx          = 0;
    /* 初始化需要操作的MAT图片 */
    aImgNew.create(nImgHeight, nImgWidth, CV_8UC1);
    RknnDetectInit();
}

/* 销毁创建的模型 */
RK_COUNT_DETECT::~RK_COUNT_DETECT()
{

    if (pModelData)
    {
        free(pModelData);
    }
}

/* 载入模型地址， 读取模型数据 */
unsigned char* RK_COUNT_DETECT::LoadModel(const char* filename, int* model_size)
{
    FILE*          fp;
    unsigned char* data = NULL;

    fp = fopen(filename, "rb");
    if (NULL == fp)
    {
        cout << "Open file " << filename << "failed." << endl;
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);

    // data = load_data(fp, 0, size);
    if (NULL == fp)
    {
        return NULL;
    }
    m_nRet = fseek(fp, 0, SEEK_SET);
    if (m_nRet != 0)
    {
        cout << "blob seek failure.\n";
        return NULL;
    }

    data = (unsigned char*)malloc(size);
    if (data == NULL)
    {
        cout << "buffer malloc failure.\n";
        return NULL;
    }

    m_nRet = fread(data, 1, size, fp);
    fclose(fp);
    *model_size = size;

    return data;
}

int RK_COUNT_DETECT::RknnDetectInit()
{
    cout << "Loading mode...\n";
    m_nModelDataSize = 0;
    /* 载入模型，并转为二进制格式 */
    pModelData       = LoadModel(m_pModelName, &m_nModelDataSize);

    m_nRet = rknn_init(&uCtx, pModelData, m_nModelDataSize, 0, NULL);
    if (m_nRet != RKNN_SUCC)
    {
        std::cout << "rknn_init error m_nRet=" << m_nRet;
        return -1;
    }

    rknn_sdk_version version;
    m_nRet = rknn_query(uCtx, RKNN_QUERY_SDK_VERSION, &version, sizeof(rknn_sdk_version));
    if (m_nRet < 0)
    {
        std::cout << "rknn_init error m_nRet=" << m_nRet;
        return -1;
    }
    /* SDK 的版本信息。SDK 所基于的驱动版本信息 */
    std::cout << "\tRKNN sdk version: " << version.api_version << "driver version: " << version.drv_version << std::endl;

    m_nRet = rknn_query(uCtx, RKNN_QUERY_IN_OUT_NUM, &stIoNum, sizeof(stIoNum));
    if (m_nRet != RKNN_SUCC)
    {
        std::cout << "rknn_init error m_nRet=" << m_nRet;
        return -1;
    }

    RknnDetectQueryInoutIo();
    return 0;
}

int RK_COUNT_DETECT::RknnDetectQueryInoutIo()
{
    /* 获取输入tensor的属性信息 */
    memset(stInputAttrs, 0, sizeof(stInputAttrs));
    for (int i = 0; i < stIoNum.n_input; i++)
    {
        stInputAttrs[i].index = i;
        m_nRet                = rknn_query(uCtx, RKNN_QUERY_INPUT_ATTR, &(stInputAttrs[i]), sizeof(rknn_tensor_attr));
        if (m_nRet != RKNN_SUCC)
        {
            std::cout << "rknn_init error m_nRet=" << m_nRet;
            return -1;
        }
    }

    if (stInputAttrs[0].fmt == RKNN_TENSOR_NCHW)
    {
        nModelChannel = stInputAttrs[0].dims[1];
        nModelHeight  = stInputAttrs[0].dims[2];
        nModelWidth   = stInputAttrs[0].dims[3];
    }
    else
    {
        nModelHeight  = stInputAttrs[0].dims[1];
        nModelWidth   = stInputAttrs[0].dims[2];
        nModelChannel = stInputAttrs[0].dims[3];
    }
    std::cout << "model input nModelHeight=" << nModelHeight << ", nModelWidth=" << nModelWidth << ", nModelChannel=" << nModelChannel << std::endl;

    /* 获取输出tensor的属性信息， 此face = 3 */
    memset(stOutputAttrs, 0, sizeof(stOutputAttrs));
    for (int i = 0; i < stIoNum.n_output; i++)
    {
        stOutputAttrs[i].index = i;
        m_nRet                 = rknn_query(uCtx, RKNN_QUERY_OUTPUT_ATTR, &(stOutputAttrs[i]), sizeof(rknn_tensor_attr));
    }

    memset(aInputs, 0, sizeof(aInputs));
    /* 该输入的索引位置 */
    aInputs[0].index        = 0;
    /* 输入数据的类型 */
    aInputs[0].type         = RKNN_TENSOR_UINT8;
    /* 输入数据所占内存大小 */
    aInputs[0].size         = nModelWidth * nModelHeight * nModelChannel;
    /* 输入数据的格式 */
    aInputs[0].fmt          = RKNN_TENSOR_NHWC;
    /* 设置为 1 时会将 buf 存放的输入数据直接设置给模型的输入节点，不做任何预处理。 */
    /* 注意，变换过程在 rknn api 内部自动处理 */
    aInputs[0].pass_through = 0;

    memset(aOutputs, 0, sizeof(aOutputs));
    for (int i = 0; i < stIoNum.n_output; i++)
    {
        /* uint8_t 标识是否需要将输出数据转为 float 类型输出 */
        aOutputs[i].want_float = 1;
    }

    return 0;
}

/* 释放模型定义的相关变量内存 */
int RK_COUNT_DETECT::RknnDetectDestory()
{
    m_nRet = rknn_outputs_release(uCtx, stIoNum.n_output, aOutputs);
    m_nRet = rknn_destroy(uCtx);
    return m_nRet;
}

/* 计算欧氏距离的函数 */
float RK_COUNT_DETECT::CalculateEuclideanDistance(float x1, float y1, float x2, float y2)
{
    float distance = sqrt(pow(sqrt(x2) - sqrt(x1), 2) + pow(sqrt(y2) - sqrt(y1), 2));
    return distance;
}

/* 多帧结果融合算法 */
void RK_COUNT_DETECT::MultiFrameResultFusionAlgorithm(std::vector<float> vPoints, std::vector<float>& vResultPoints)
{

    if (bFlag)
    {
        vPoints_Result == vPoints;
        for (int i = 0; i < vPoints_Result.size(); i++)
        {
            vCout.push_back(nControl);    // 表示最多3帧都没有匹配上，不然就去掉
        }
        bFlag = false;
    }

    for (int i = 0; i < vPoints.size() / 2; i++)
    {
        bool bIsC = false;
        for (int j = 0; j < vPoints_Result.size() / 2; j++)
        {
            // 如果两帧对应的点欧氏距离某个值，认为是同一个点，保留一个
            if (CalculateEuclideanDistance(vPoints[i * 2], vPoints[i * 2 + 1], vPoints_Result[j * 2], vPoints_Result[j * 2 + 1]) < fInstance)
            {
                vPoints_Result[j * 2]     = vPoints[i * 2];
                vPoints_Result[j * 2 + 1] = vPoints[i * 2 + 1];

                // 统计替换次数,当做新加入的点
                vCout[j * 2]     = nControl;
                vCout[j * 2 + 1] = nControl;

                bIsC = true;
                break;
            }
        }
        if (!bIsC)
        {
            bool bDel = false;
            for (int z = 0; z < vPoints_remove.size() / 2; z++)
            {
                // 如果两帧对应的点欧氏距离某个值，认为是同一个点，保留一个
                if (CalculateEuclideanDistance(vPoints[i * 2], vPoints[i * 2 + 1], vPoints_remove[z * 2], vPoints_remove[z * 2 + 1]) < fInstance)
                {
                    vPoints_Result.push_back(vPoints[i * 2]);
                    vPoints_Result.push_back(vPoints[i * 2 + 1]);
                    vCout.push_back(nControl);    // 表示最多3帧都没有匹配上，不然就去掉
                    vCout.push_back(nControl);    // 表示最多3帧都没有匹配上，不然就去掉

                    vPoints_remove.erase(vPoints_remove.begin() + z * 2 + 1);
                    vPoints_remove.erase(vPoints_remove.begin() + z * 2);
                    vDelCout.erase(vDelCout.begin() + z * 2 + 1);
                    vDelCout.erase(vDelCout.begin() + z * 2);
                    bDel = true;
                    break;
                }
            }
            if (!bDel)
            {
                vDelCout.push_back(nDelControl);
                vDelCout.push_back(nDelControl);
                vPoints_remove.push_back(vPoints[i * 2]);
                vPoints_remove.push_back(vPoints[i * 2 + 1]);
            }
        }
        /* 将最终结果的容器赋值出去 */
        vResultPoints = vPoints_Result;
    }
    // 移除多次不匹配到的结果
    std::vector<int> vDel;    // 记录需要删除的下标
    /* 获取上一帧识别结果中，需要删除的点；最后循环删除 */
    for (int i = 0; i < vPoints_Result.size(); i++)
    {
        vCout[i] -= 1;
        if (vCout[i] < 0)
        {
            vDel.push_back(i);
        }
    }
    for (size_t num = vDel.size(); num > 0; num--)
    {
        vPoints_Result.erase(vPoints_Result.begin() + vDel[num - 1]);
        vCout.erase(vCout.begin() + vDel[num - 1]);
    }
    vDel.clear();    // 清空删除下标容器
    /* 获取上一帧中需要删除的结果容器中，需要删除的点；最后循环删除 */
    for (int i = 0; i < vPoints_remove.size(); i++)
    {
        vDelCout[i] -= 1;
        if (vDelCout[i] < 0)
        {
            vDel.push_back(i);
        }
    }
    for (size_t num = vDel.size(); num > 0; num--)
    {
        vPoints_remove.erase(vPoints_remove.begin() + vDel[num - 1]);
        vDelCout.erase(vDelCout.begin() + vDel[num - 1]);
    }
}

/* bgr格式视频流的识别 */
int RK_COUNT_DETECT::DetectHumanBgr(char* pDataBuffer, int nLength)
{
    if (nLength != nImgHeight * nImgWidth * 3)
    {
        return -1;
    }

    if (pDataBuffer)
    {
        /* 计算函数使用的时间 */
        aInputs[0].buf = pDataBuffer;
        rknn_inputs_set(uCtx, stIoNum.n_input, aInputs);
        m_nRet = rknn_run(uCtx, NULL);
        m_nRet = rknn_outputs_get(uCtx, stIoNum.n_output, aOutputs, NULL);


        /* 后处理 */
        ctrlNetoutputProcess((float*)aOutputs[0].buf, (float*)aOutputs[1].buf, nImgHeight, nImgWidth, stTunableParam, nPepleNum, vPointsXY);
        //printf("============人数为：%d =============\n",nPepleNum);
        rknn_outputs_release(uCtx, stIoNum.n_output, aOutputs);

        return 0;
    }
    else
    {
        std::cout << "data_buf error!!!" << std::endl;
    }

    return -1;
}
