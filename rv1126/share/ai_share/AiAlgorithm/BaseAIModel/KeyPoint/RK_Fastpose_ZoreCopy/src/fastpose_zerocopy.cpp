/*
*  File Name: fastpose_zerocopy.cpp
*  Created on: 2024年7月18日
*  Author: wcp
*  description : 人体26个关键点识别
*  Modify date: 2024年7月18日
*/
 
#include <chrono>
#include "fastpose_zerocopy.h"
#include "fastpose_process.h"
#include <algorithm>

namespace Fastpose_NS{
/* 构造函数 -- 初始化变量 */
cFastpose::cFastpose()
{
    /* 模型的位置 */
    m_pModelName = "./weights/Fastpose.rknn";
    /* 输入视频流的大小和通道 */
    nImgWidth = 192;
    nImgHeight = 256;
    nImgChannel = 3;
    /* 模型需要输入的大小和通道 */
    nModelWidth = 0;
    nModelHeight = 0;
    nModelChannel = 3;
    uCtx = 0;
    RknnDetectInit();
}
cFastpose::cFastpose(char* pModelPath)
{
    /* 模型的位置 */
    m_pModelName = pModelPath;
    /* 输入视频流的大小和通道 */
    nImgWidth = 192;
    nImgHeight = 256;
    nImgChannel = 3;
    /* 模型需要输入的大小和通道 */
    nModelWidth = 0;
    nModelHeight = 0;
    nModelChannel = 3;
    uCtx = 0;
    RknnDetectInit();
}

/* 销毁创建的模型 */
cFastpose::~cFastpose()
{

    if (pModelData)
    {
        free(pModelData);
    }

}

/* 载入模型地址， 读取模型数据 */
unsigned char *cFastpose::LoadModel(const char *filename, int *model_size)
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

int cFastpose::RknnDetectInit()
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
    
    rknn_core_mask CoreMask = RKNN_NPU_CORE_0_1_2;
    m_nRet = rknn_set_core_mask(uCtx, CoreMask);
    if (m_nRet < 0)
    {
        std::cout << "rknn_init error m_nRet=" << m_nRet;
        return -1;
    }

    RknnDetectQueryInoutIo();
    return 0;
}

int cFastpose::RknnDetectQueryInoutIo()
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
        
		/* 申请输入数据内存 */
    	pInputMen[i] = rknn_create_mem(uCtx, stInputAttrs[i].size_with_stride);
    	/* 输出数据的相关配置 */
    	// 如果设置为 uint8，将在 NPU 中进行归一化和量化处理
		stInputAttrs[i].type = InputType;
		// 默认格式为 NHWC，NPU 仅支持在零拷贝模式下使用 NHWC 格式
		stInputAttrs[i].fmt = InputLayout;
		/* 调用rknn_set_io_mem 让NPU使用上面申请到的内存 */
    	rknn_set_io_mem(uCtx, pInputMen[i], &stInputAttrs[i]);
    }
	printf("-----------------------模型输入形状--------------------------\n");
	for (int i = 0; i < stIoNum.n_input; i++)
	{
		nModelHeight = stInputAttrs[i].dims[1];
		nModelWidth = stInputAttrs[i].dims[2];
		nModelChannel = stInputAttrs[i].dims[3];
		std::cout << "model input"<< i <<" nModelHeight=" << nModelHeight << ", nModelWidth=" << nModelWidth << ", nModelChannel=" << nModelChannel << std::endl;
	}
	printf("-------------------------------------------------\n");
    /* 获取输出tensor的属性信息 */
    memset(stOutputAttrs, 0, sizeof(stOutputAttrs));
    for (int i = 0; i < stIoNum.n_output; i++)
    {
        stOutputAttrs[i].index = i;
        m_nRet = rknn_query(uCtx, RKNN_QUERY_OUTPUT_ATTR, &(stOutputAttrs[i]), sizeof(rknn_tensor_attr));
        
		/* 申请输入数据内存 */
    	pOutputMem[i] = rknn_create_mem(uCtx, stOutputAttrs[i].n_elems*sizeof(float));
    	/* 输出数据的相关配置 */
    	stOutputAttrs[i].type = OutputType;
    	/* 调用rknn_set_io_mem 让NPU使用上面申请到的内存 */
    	rknn_set_io_mem(uCtx, pOutputMem[i], &stOutputAttrs[i]);
    }
    
    return 0;
}

/* 释放模型定义的相关变量内存 */
int cFastpose::RknnDetectDestory()
{
    /* 调用rknn_destory_mem接口销毁申请的内存 */
	for (int i = 0; i < stIoNum.n_input; i++)
    {
    	rknn_destroy_mem(uCtx, pInputMen[i]);
    }
	
	for (int i = 0; i < stIoNum.n_output; i++)
    {
    	rknn_destroy_mem(uCtx, pOutputMem[i]);
    }
	/* 调用rknn_destroy销毁context */
	m_nRet = rknn_destroy(uCtx);
    return m_nRet;
}

/* 定义一个数据输入存储函数 */
void cFastpose::setInputDatas(unsigned char* pDataBuffer, int nInputIdex)
{
	// 将输入数据拷贝到输入张量内存
	int nInputWidth  = stInputAttrs[nInputIdex].dims[2];
	int nInputStride = stInputAttrs[nInputIdex].w_stride;
	if (nInputWidth == nInputStride) 
	{
		memcpy(pInputMen[nInputIdex]->virt_addr, pDataBuffer, nInputWidth * stInputAttrs[nInputIdex].dims[1] * stInputAttrs[nInputIdex].dims[3]);
	} 
	else 
	{
		int nInputHeight  = stInputAttrs[nInputIdex].dims[1];
		int nInputChannel = stInputAttrs[nInputIdex].dims[3];
		// 按照跨度从源地址到目标地址进行拷贝
		uint8_t* pSrcPtr = pDataBuffer;
		uint8_t* pDstPtr = (uint8_t*)pInputMen[nInputIdex]->virt_addr;
		// 宽度乘以通道的元素数量
		int nSrcWcElems = nInputWidth * nInputChannel;
		int nDstWcElems = nInputStride * nInputChannel;
		for (int h = 0; h < nInputHeight; ++h) 
		{
			memcpy(pDstPtr, pSrcPtr, nSrcWcElems);
			pSrcPtr += nSrcWcElems;
			pDstPtr += nDstWcElems;
		}
	}
}

/* rgb格式视频流的识别 */
int cFastpose::Infer(cv::Mat aInputImg,std::vector<float>& vPoints)
{
    if (!aInputImg.empty())
    {
    	/* 预处理 */
    	/* 定义一张空的输出图片 */
    	cv::Mat aSrcImg(nImgHeight,nImgWidth, CV_8UC3, cv::Scalar(128));
		// 预处理计时
		auto pre_start = std::chrono::high_resolution_clock::now();
		cv::resize(aInputImg, aSrcImg, cv::Size(nImgWidth,nImgHeight));
		// 结束计时
		auto pre_end = std::chrono::high_resolution_clock::now();
		// 计算执行时间（毫秒）
		std::chrono::duration<double, std::milli> pre_resu = pre_end - pre_start;
		// 输出执行时间
		std::cout << "预处理耗时: " << pre_resu.count() << " ms" << std::endl;
        /* 计算函数使用的时间 */
        auto start_time = std::chrono::high_resolution_clock::now();
        pInputData = (unsigned char*) aSrcImg.data;
        setInputDatas(pInputData, 0);
        m_nRet = rknn_run(uCtx, NULL);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration = end_time - start_time;
        std::cout << "模型推理使用的时间：" << duration.count() << "毫秒--"<< std::endl;
 		
 		float* pOutput = (float*)pOutputMem[0]->virt_addr;
 		
		/* 后处理 */
		start_time = std::chrono::high_resolution_clock::now();
		PostProcessImage(pOutput, vPoints);
		
        end_time = std::chrono::high_resolution_clock::now();
        duration = end_time - start_time;
        std::cout << "后处理使用的时间：" << duration.count() << "毫秒--"<< std::endl;
		return 0;
		
    }
    else
    {
        std::cout << "data_buf error!!!" << std::endl;
    }

    return -1;
}
}
