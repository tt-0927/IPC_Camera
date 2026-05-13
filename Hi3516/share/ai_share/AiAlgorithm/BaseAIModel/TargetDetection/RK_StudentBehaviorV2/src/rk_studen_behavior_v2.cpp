/*
 *  File Name: RK_STUDENT_BEHAVIOR_V2.cpp
 *  Created on: 2023年7月20日
 *  Author: wcp
 *  description : 获取输入的视频流中的128个特征点
 *  Modify date: 2023年7月21日
 */

#include <chrono>

#include "rk_student_behavior_v2.h"

/* 构造函数 -- 初始化变量 */
RK_STUDENT_BEHAVIOR_V2::RK_STUDENT_BEHAVIOR_V2()
{
    /* 模型的位置 */
    m_pModelName  = "./weights/student_behavior.rknn";
    /* 输入视频流的大小和通道 */
    nImgWidth     = 1920;
    nImgHeight    = 1024;
    nImgChannel   = 3;
    /* 模型需要输入的大小和通道 */
    nModelWidth   = 0;
    nModelHeight  = 0;
    nModelChannel = 3;
    uCtx          = 0;

    /* 初始化需要操作的MAT图片 */
    aSrcImg.create(nImgHeight, nImgWidth, CV_8UC3);

    RknnDetectInit();
}

RK_STUDENT_BEHAVIOR_V2::RK_STUDENT_BEHAVIOR_V2(char* pModelPath)
{
    /* 模型的位置 */
    m_pModelName  = pModelPath;
    /* 输入视频流的大小和通道 */
    nImgWidth     = 1920;
    nImgHeight    = 1024;
    nImgChannel   = 3;
    /* 模型需要输入的大小和通道 */
    nModelWidth   = 0;
    nModelHeight  = 0;
    nModelChannel = 3;
    uCtx          = 0;

    /* 初始化需要操作的MAT图片 */
    aSrcImg.create(nImgHeight, nImgWidth, CV_8UC3);

    RknnDetectInit();
}

RK_STUDENT_BEHAVIOR_V2::RK_STUDENT_BEHAVIOR_V2(char* pModelPath, int nInitImgWidth, int nInitImgHeight, int nInitImgChannel)
{
    /* 模型的位置 */
    m_pModelName  = pModelPath;
    /* 输入视频流的大小和通道 */
    nImgWidth     = nInitImgWidth;
    nImgHeight    = nInitImgHeight;
    nImgChannel   = nInitImgChannel;
    /* 模型需要输入的大小和通道 */
    nModelWidth   = 0;
    nModelHeight  = 0;
    nModelChannel = 3;
    uCtx          = 0;

    /* 初始化需要操作的MAT图片 */
    aSrcImg.create(nImgHeight, nImgWidth, CV_8UC3);

    RknnDetectInit();
}

/* 销毁创建的模型 */
RK_STUDENT_BEHAVIOR_V2::~RK_STUDENT_BEHAVIOR_V2()
{

    if (pModelData)
    {
        free(pModelData);
    }
}

/* 载入模型地址， 读取模型数据 */
unsigned char* RK_STUDENT_BEHAVIOR_V2::LoadModel(const char* filename, int* model_size)
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

int RK_STUDENT_BEHAVIOR_V2::RknnDetectInit()
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

int RK_STUDENT_BEHAVIOR_V2::RknnDetectQueryInoutIo()
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
int RK_STUDENT_BEHAVIOR_V2::RknnDetectDestory()
{
    m_nRet = rknn_outputs_release(uCtx, stIoNum.n_output, aOutputs);
    m_nRet = rknn_destroy(uCtx);
    return m_nRet;
}

/* 模型预处理 -- 缩放使用灰度图填充 */
cv::Mat Pretreatment(cv::Mat aSrcImg, float& fMax)
{
    int   roiw = aSrcImg.cols;
    int   roih = aSrcImg.rows;
    float w_s  = roiw * 1.0 / 640;
    float h_s  = roih * 1.0 / 640;
    fMax       = w_s > h_s ? w_s : h_s;

    cv::Mat aSimgResize;
    cv::Mat aSimgPart(640, 640, CV_8UC3, cv::Scalar(128, 128, 128));
    cv::resize(aSrcImg, aSimgResize, cv::Size(int(roiw / fMax), int(roih / fMax)));
    cv::Rect roi(0, 0, aSimgResize.cols, aSimgResize.rows);
    // 将aSrcImg复制到aSimgPart的感兴趣区域
    aSimgResize.copyTo(aSimgPart(roi));
    return aSimgPart;
}

/* rgb格式视频流的识别 */
int RK_STUDENT_BEHAVIOR_V2::DetectFaceRgb(char* aInputImg, std::vector<float>& vPoints)
{
    /* 图片格式转换 */
    cv::Mat aSrcImg(nImgHeight, nImgWidth, CV_8UC3);
    memcpy(aSrcImg.data, aInputImg, nImgHeight * nImgWidth * 3);    // 使用memcpy将数据复制到aSrcImg.data
    if (!aSrcImg.empty())
    {
        // /* 图片预处理 */
        // float fMax;
        // cv::Mat aInputResizeImg = Pretreatment(aSrcImg,fMax);

        /* 计算函数使用的时间 */
        auto start_time = std::chrono::high_resolution_clock::now();
        aInputs[0].buf  = (void*)aSrcImg.data;
        rknn_inputs_set(uCtx, stIoNum.n_input, aInputs);

        m_nRet        = rknn_run(uCtx, NULL);
        m_nRet        = rknn_outputs_get(uCtx, stIoNum.n_output, aOutputs, NULL);
        auto end_time = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double, std::milli> duration = end_time - start_time;
        // std::cout << "模型推理一次使用的时间：" << duration.count() << "毫秒--" << std::endl;

        // 后处理
        post_process_v2((float*)aOutputs[0].buf, (float*)aOutputs[1].buf, (float*)aOutputs[2].buf, nModelWidth, nModelHeight,
                     fBoxThreshold, fNmsThreshold, vPoints);

        // cv::imwrite("./test.jpg", aSrcImg);
        /* 将目标缩放回原始尺寸 */
        // for (int i = 0; i < vPoints.size()/6; i++)
        // {
        //     vPoints[6*i] = vPoints[6*i]*fMax;
        //     vPoints[6*i+1] = vPoints[6*i+1]*fMax;
        //     vPoints[6*i+2] = vPoints[6*i+2]*fMax;
        //     vPoints[6*i+3] = vPoints[6*i+3]*fMax;
        // }

        rknn_outputs_release(uCtx, stIoNum.n_output, aOutputs);
        return 1;
    }
    else
    {
        std::cout << "data_buf error!!!" << std::endl;
    }

    return -1;
}
