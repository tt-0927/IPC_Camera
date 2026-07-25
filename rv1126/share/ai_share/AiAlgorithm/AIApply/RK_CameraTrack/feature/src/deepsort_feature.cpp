/*
 *  File Name: rk_face_feature.cpp
 *  Created on: 2023年7月20日
 *  Author: wcp
 *  description : 获取输入的视频流中的128个特征点
 *  Modify date: 2023年7月21日
 */

#include <chrono>
#include "deepsort_feature.h"

/* 构造函数 -- 初始化变量 */
RK_DEEPSORT_FEATURE::RK_DEEPSORT_FEATURE()
{
    /* 模型的位置 */
    m_pModelName = "./weights/Deepsort_facenet.rknn";
    /* 输入视频流的大小和通道 */
    nImgWidth = 64;
    nImgHeight = 128;
    nImgChannel = 3;
    /* 模型需要输入的大小和通道 */
    nModelWidth = 0;
    nModelHeight = 0;
    nModelChannel = 3;
    uCtx = 0;

    RknnDetectInit();
}
RK_DEEPSORT_FEATURE::RK_DEEPSORT_FEATURE(char *pModelPath)
{
    /* 模型的位置 */
    m_pModelName = pModelPath;
    /* 输入视频流的大小和通道 */
    nImgWidth = 64;
    nImgHeight = 128;
    nImgChannel = 3;
    /* 模型需要输入的大小和通道 */
    nModelWidth = 0;
    nModelHeight = 0;
    nModelChannel = 3;
    uCtx = 0;

    RknnDetectInit();
}

/* 销毁创建的模型 */
RK_DEEPSORT_FEATURE::~RK_DEEPSORT_FEATURE()
{

    if (pModelData)
    {
        free(pModelData);
    }
}

/* 载入模型地址， 读取模型数据 */
unsigned char *RK_DEEPSORT_FEATURE::LoadModel(const char *filename, int *model_size)
{
    FILE *fp;
    unsigned char *data = NULL;

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

    data = (unsigned char *)malloc(size);
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

int RK_DEEPSORT_FEATURE::RknnDetectInit()
{
    cout << "Loading mode...\n";
    m_nModelDataSize = 0;
    /* 载入模型，并转为二进制格式 */
    pModelData = LoadModel(m_pModelName, &m_nModelDataSize);

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

int RK_DEEPSORT_FEATURE::RknnDetectQueryInoutIo()
{
    /* 获取输入tensor的属性信息 */
    memset(stInputAttrs, 0, sizeof(stInputAttrs));
    for (int i = 0; i < stIoNum.n_input; i++)
    {
        stInputAttrs[i].index = i;
        m_nRet = rknn_query(uCtx, RKNN_QUERY_INPUT_ATTR, &(stInputAttrs[i]), sizeof(rknn_tensor_attr));
        if (m_nRet != RKNN_SUCC)
        {
            std::cout << "rknn_init error m_nRet=" << m_nRet;
            return -1;
        }
    }

    if (stInputAttrs[0].fmt == RKNN_TENSOR_NCHW)
    {
        nModelChannel = stInputAttrs[0].dims[1];
        nModelHeight = stInputAttrs[0].dims[2];
        nModelWidth = stInputAttrs[0].dims[3];
    }
    else
    {
        nModelHeight = stInputAttrs[0].dims[1];
        nModelWidth = stInputAttrs[0].dims[2];
        nModelChannel = stInputAttrs[0].dims[3];
    }
    std::cout << "model input nModelHeight=" << nModelHeight << ", nModelWidth=" << nModelWidth << ", nModelChannel=" << nModelChannel << std::endl;

    /* 获取输出tensor的属性信息， 此face = 3 */
    memset(stOutputAttrs, 0, sizeof(stOutputAttrs));
    for (int i = 0; i < stIoNum.n_output; i++)
    {
        stOutputAttrs[i].index = i;
        m_nRet = rknn_query(uCtx, RKNN_QUERY_OUTPUT_ATTR, &(stOutputAttrs[i]), sizeof(rknn_tensor_attr));
    }

    memset(aInputs, 0, sizeof(aInputs));
    /* 该输入的索引位置 */
    aInputs[0].index = 0;
    /* 输入数据的类型 */
    aInputs[0].type = RKNN_TENSOR_UINT8;
    /* 输入数据所占内存大小 */
    aInputs[0].size = nModelWidth * nModelHeight * nModelChannel;
    /* 输入数据的格式 */
    aInputs[0].fmt = RKNN_TENSOR_NHWC;
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
int RK_DEEPSORT_FEATURE::RknnDetectDestory()
{
    m_nRet = rknn_outputs_release(uCtx, stIoNum.n_output, aOutputs);
    m_nRet = rknn_destroy(uCtx);
    return m_nRet;
}

void randomCrop(cv::Mat &image, int cropWidth, int cropHeight)
{
    int imageWidth = image.cols;
    int imageHeight = image.rows;

    // 检查图像尺寸是否小于裁剪大小
    if (imageWidth < cropWidth || imageHeight < cropHeight)
    {
        // 使用copyMakeBorder函数进行边缘填充，将图像调整为裁剪大小
        int padWidth = std::max(cropWidth - imageWidth, 0);
        int padHeight = std::max(cropHeight - imageHeight, 0);
        cv::copyMakeBorder(image, image, 0, padHeight, 0, padWidth, cv::BORDER_CONSTANT, cv::Scalar(128, 128, 128));
    }
    else
    {
        // 计算中心裁剪的起始位置
        int startX = std::floor((imageWidth - cropWidth) / 2);
        int startY = std::floor((imageHeight - cropHeight) / 2);
        cv::Rect cropRect(startX, startY, cropWidth, cropHeight);
        image = image(cropRect);
    }
}

int resizeAndPad(cv::Mat &image, int targetWidth, int targetHeight)
{
    int imageWidth = image.cols;
    int imageHeight = image.rows;

    if(imageWidth == 0 || imageHeight == 0)
    {
        return -1;
    }

    // 计算图像的缩放比例
    double scale = std::min((double)targetWidth / imageWidth, (double)targetHeight / imageHeight);

    // 计算等比缩放后的目标尺寸
    int resizedWidth = std::round(imageWidth * scale);
    int resizedHeight = std::round(imageHeight * scale);

    // 使用cv::resize函数进行等比缩放
    cv::resize(image, image, cv::Size(resizedWidth, resizedHeight));

    // 计算填充的边界大小
    int padWidth = targetWidth - resizedWidth;
    int padHeight = targetHeight - resizedHeight;

    // 使用cv::copyMakeBorder函数进行填充
    cv::copyMakeBorder(image, image, 0, padHeight, 0, padWidth, cv::BORDER_CONSTANT, cv::Scalar(128, 128, 128));

    return 0;
}

/* bgr格式视频流的识别 */
bool RK_DEEPSORT_FEATURE::GetFeatures(cv::Mat aInputImg, std::vector<float> vPoints, float *vFeatures)
{
    if (!aInputImg.empty())
    {
        cv::Rect aRc = cv::Rect(int(vPoints[0]), int(vPoints[1]), int(vPoints[2] - vPoints[0]), int(vPoints[3] - vPoints[1]));
        cv::Mat aRoiImg = aInputImg.clone()(aRc);
        /*if (aRoiImg.cols != nImgWidth || aRoiImg.rows != nImgHeight) {
            // 计算缩放比例
            double scale = std::min(nImgWidth*1.0 / aRoiImg.cols, nImgHeight*1.0 / aRoiImg.rows);
            // 根据缩放比例调整图像大小
            cv::resize(aRoiImg, aRoiImg, cv::Size(), scale, scale, cv::INTER_LINEAR);
         }*/
        /*cv::resize(aRoiImg, aRoiImg,
                   cv::Size(nImgWidth,nImgHeight),
                   cv::InterpolationFlags::INTER_CUBIC);*/
        
        if(resizeAndPad(aRoiImg, nImgWidth, nImgHeight) == -1)
        {
            return false;
        }
        // cv::cvtColor(aRoiImg, aRoiImg, cv::COLOR_RGB2BGR); // 转换颜色空间
        // cv::imwrite("feature2.jpg", aRoiImg);

        /* 计算函数使用的时间 */
        auto start_time = std::chrono::high_resolution_clock::now();
        aInputs[0].buf = aRoiImg.data;
        rknn_inputs_set(uCtx, stIoNum.n_input, aInputs);
        m_nRet = rknn_run(uCtx, NULL);
        m_nRet = rknn_outputs_get(uCtx, stIoNum.n_output, aOutputs, NULL);
        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration = end_time - start_time;
        // std::cout << "Feature模型推理一次使用的时间:" << duration.count() << "毫秒--"<< std::endl;

        /* 返回结果 */
        float *f = static_cast<float *>(aOutputs[0].buf);
        for (int i = 0; i < 512; i++)
        {
            vFeatures[i] = f[i];
        }

        rknn_outputs_release(uCtx, stIoNum.n_output, aOutputs);
        return true;
    }
    else
    {
        std::cout << "data_buf error!!!" << std::endl;
    }

    return false;
}


