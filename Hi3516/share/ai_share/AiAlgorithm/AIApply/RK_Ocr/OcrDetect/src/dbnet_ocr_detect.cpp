/*
*  File Name: dbnet_ocr_detect.cpp
*  Created on: 2023年12月16日
*  Author: wcp
*  description : 获取输入CV::Mat格式的视频流中的文字位置	
*  Modify date: 2023年12月18日
*/
 
#include <chrono>
#include "dbnet_ocr_detect.h"

/* 构造函数 -- 初始化变量 */
RK_OCR_DETECT::RK_OCR_DETECT()
{
    /* 模型的位置 */
    m_pModelName = "./weights/DBnetOcrDetect.rknn";
    /* 输入视频流的大小和通道 */
    nImgWidth = 160;
    nImgHeight = 160;
    nImgChannel = 3;
    /* 模型需要输入的大小和通道 */
    nModelWidth = 0;
    nModelHeight = 0;
    nModelChannel = 3;
    uCtx = 0;

    RknnDetectInit();
}
RK_OCR_DETECT::RK_OCR_DETECT(char* pModelPath)
{
    /* 模型的位置 */
    m_pModelName = pModelPath;
    /* 输入视频流的大小和通道 */
    nImgWidth = 160;
    nImgHeight = 160;
    nImgChannel = 3;
    /* 模型需要输入的大小和通道 */
    nModelWidth = 0;
    nModelHeight = 0;
    nModelChannel = 3;
    uCtx = 0;

    RknnDetectInit();
}

/* 销毁创建的模型 */
RK_OCR_DETECT::~RK_OCR_DETECT()
{

    if (pModelData)
    {
        free(pModelData);
    }
}

/* 载入模型地址， 读取模型数据 */
unsigned char *RK_OCR_DETECT::LoadModel(const char *filename, int *model_size)
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

int RK_OCR_DETECT::RknnDetectInit()
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

int RK_OCR_DETECT::RknnDetectQueryInoutIo()
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
int RK_OCR_DETECT::RknnDetectDestory()
{
    m_nRet = rknn_outputs_release(uCtx, stIoNum.n_output, aOutputs);
    m_nRet = rknn_destroy(uCtx);
    return m_nRet;
}

/* 模型预处理 */
void RK_OCR_DETECT::Precond(cv::Mat& aInputImg,cv::Size expected_size)
{
	int ih = aInputImg.rows;
    int iw = aInputImg.cols;
    int ew = expected_size.width;
    int eh = expected_size.height;
    // scale = min((eh/ih),(ew/iw))
    double scale = std::min(static_cast<double>(eh) / ih, static_cast<double>(ew) / iw);
    
    int nh = static_cast<int>(ih * scale);
    int nw = static_cast<int>(iw * scale);
    cv::resize(aInputImg, aInputImg, cv::Size(nw, nh), 0, 0, cv::INTER_CUBIC);
    
    int top = 0;
    int bottom = eh - nh;
    int left = 0;
    int right = ew - nw;

    cv::copyMakeBorder(aInputImg, aInputImg, top, bottom, left, right, cv::BORDER_CONSTANT, cv::Scalar(114, 114, 114));
}

/* 调整图像 */
void RK_OCR_DETECT::PerspectiveTransform(cv::Mat aImg, std::vector<cv::Point2f>& vBoxes,cv::Mat& aRepImg)
{
    // 计算透视变换后的目标区域，目标图像的宽度和高度
    float width = std::max(cv::norm(vBoxes[0] - vBoxes[1]), cv::norm(vBoxes[2] - vBoxes[3]));
    float height = std::max(cv::norm(vBoxes[1] - vBoxes[2]), cv::norm(vBoxes[3] - vBoxes[0]));
    std::vector<cv::Point2f> dst_points;
    dst_points.push_back(cv::Point2f(0, 0)); 
    dst_points.push_back(cv::Point2f(width - 1, 0)); 
    dst_points.push_back(cv::Point2f(width - 1, height - 1)); 
    dst_points.push_back(cv::Point2f(0, height - 1)); 
    cv::Mat matrix = cv::getPerspectiveTransform(vBoxes, dst_points);
    // 进行透视变换
    cv::Mat aResult;
    cv::warpPerspective(aImg, aResult, matrix, cv::Size(width, height));	
    aRepImg = aResult.clone();
}

/* rgb格式视频流的识别 */
int RK_OCR_DETECT::DetectOcrRgb(cv::Mat aInputImg,std::vector<std::vector<cv::Point2f>>& vBoxes,std::vector<float>& vScores)
{                                                      
    if (!aInputImg.empty())
    {
		DestWidth=aInputImg.cols;
		DestHeight=aInputImg.rows;
    	Precond(aInputImg);
        /* 计算函数使用的时间 */
        auto start_time = std::chrono::high_resolution_clock::now();
        aInputs[0].buf = (void*) aInputImg.data;
        rknn_inputs_set(uCtx, stIoNum.n_input, aInputs);
        m_nRet = rknn_run(uCtx, NULL);
        m_nRet = rknn_outputs_get(uCtx, stIoNum.n_output, aOutputs, NULL);
        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration = end_time - start_time;
        std::cout << "模型推理一次使用的时间：" << duration.count() << "毫秒--"<< std::endl;
       
        // 后处理
		post_process((float*)aOutputs[0].buf,DestWidth,DestHeight,fThreshold,fBoxThresh,nMaxCandidates,fUnclipRatio,vBoxes,vScores);
        return 1;
    }
    else
    {
        std::cout << "data_buf error!!!" << std::endl;
    }

    return -1;
}
