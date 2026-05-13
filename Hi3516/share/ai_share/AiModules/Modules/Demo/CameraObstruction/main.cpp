/**
 * @file main.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-10-28
 *
 * @brief 接流或者读取视频
 */
#include "FFmedia.hpp"
#include "CameraObstructionV1_0.hpp"

/* ============================================================= 其他配置 =====================================================================*/
/* 当sMoviePath不为nullptr时，则开始将显示的视频保存成 xx.h264格式*/
char *sMoviePath = nullptr;
// char* sMoviePath = "results.h264";
/* ========================================================================================================================================== */

/*从队列获取视频处理线程*/
static void *videoFrameHandlThr(void *pParam)
{
    /* 全局的图片 */
    cv::Mat aSrcImg(1024, 1920, CV_8UC3);
    /* ---------------------- 模型初始化 ------------------------ */
    CameraObstruction_NS::InParam_S stInParam;
    /* 初始化车牌识别检测 */
    CameraObstruction_NS::CCameraObstructionV1_0 *demo = new CameraObstruction_NS::CCameraObstructionV1_0(stInParam);
    bool bFlag = demo->init();
    if (!bFlag)
    {
        printf("模型初始化失败\n");
        exit(0);
    }

    CameraObstruction_NS::InData_S stInData;
    stInData.stParam.dThres = 0.9;  /* 模糊阈值，[0-1],越大越模糊 */
    /* ---------------------------------------------------------- */

    int nRet;
    Int64 naddr = 0;
    OS_QueHndl *queHndl = (OS_QueHndl *)&g_videoQue;
    RK_U32 u32BuffSize;
    VO_FRAME_INFO_S stFrameInfo;
    RK_VOID *pMblk = NULL;
    /*分配一个用来送显图像的帧*/
    VIDEO_FRAME_INFO_S *pstVFrame = (VIDEO_FRAME_INFO_S *)(malloc(sizeof(VIDEO_FRAME_INFO_S)));
    /*先创建一个图层的framebuffer并且填充数据准备显示*/
    u32BuffSize = RK_MPI_VO_CreateGraphicsFrameBuffer(1920, 1080, TEST_FMT_VPSS, &pMblk);
    if (u32BuffSize == 0)
    {
        printf("RK_MPI_VO_CreateGraphicsFrameBuffer error\n");
    }
    /*获取framebufer的信息*/
    RK_MPI_VO_GetFrameInfo(pMblk, &stFrameInfo);

    while (!g_bExit)
    {
        naddr = 0;
        int nstatus = OS_queGet(queHndl, &naddr, -1);
        if (nstatus == 0 && naddr != 0)
        {
            VideoFrame_S *pFrame = (VideoFrame_S *)naddr;
            //  开始计时
            auto start0 = std::chrono::high_resolution_clock::now();
            unsigned char *output_data_buffer = (unsigned char *)malloc(pFrame->nSize);
            memcpy(aSrcImg.data, pFrame->pData, pFrame->nSize);

            CameraObstruction_NS::Result_S stOutData;
            { /*---------------------------------- 模型推理 ------------------------------------ */
                /* ================================ 开始计时 ===================================== */
                auto start1 = std::chrono::high_resolution_clock::now();
                /* =============================== AI算法 ======================================== */
                stInData.inMat = aSrcImg;
                bFlag = demo->process(stInData, stOutData);
                if (!bFlag)
                {
                    printf("推理失败\n");
                }
                /* =============================== 输出推理事件耗时 ================================== */
                auto end1 = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double, std::milli> duration = end1 - start1;
                std::cout << "InferAllTime: " << duration.count() << " ms" << std::endl;
            }
            { /* ---------------------------------- 显示函数 ----------------------------------  */
                const char* pShow =  stOutData.bBlockFlag? "TRUE":"FALSE";
                cv::putText(aSrcImg, pShow, cv::Point(400, 350), cv::FONT_HERSHEY_SIMPLEX, 1, getColor(0), 2);
            }

            /* 绘制分析的区域 */
            memcpy(stFrameInfo.pData, (void *)aSrcImg.data, pFrame->nSize);

            pstVFrame->stVFrame.pMbBlk = pMblk;
            pstVFrame->stVFrame.u32Width = pFrame->nWidth;
            pstVFrame->stVFrame.u32Height = pFrame->nHeight;
            pstVFrame->stVFrame.u32VirWidth = pFrame->nVirWidth;
            pstVFrame->stVFrame.u32VirHeight = pFrame->nVirHeight;
            pstVFrame->stVFrame.enPixelFormat = pFrame->nPixelFormat;
            nRet = RK_MPI_VO_SendFrame(g_nVoLayer, 0, pstVFrame, 10);
            if (nRet != RK_SUCCESS)
            {
                RK_LOGE("RK_MPI_VO_SendFrame failed with %#x", nRet);
            }

            /*送数据到VENC编码*/
            if (sMoviePath != nullptr)
            {
                RK_MPI_VENC_SendFrame(g_nVencChn, pstVFrame, 10);
            }

            free(output_data_buffer);
            /*释放资源*/
            if (pFrame)
            {
                if (pFrame->pData)
                {
                    free(pFrame->pData);
                }
                free(pFrame);
            }

            // 结束计时
            auto end0 = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> resu = end0 - start0;
            std::cout << "MainTime: " << resu.count() << " ms" << std::endl;
        }
    }
    while (0 == OS_queGet(queHndl, &naddr, 0))
    {
        VideoFrame_S *pFrame = (VideoFrame_S *)naddr;
        if (pFrame)
        {
            if (pFrame->pData)
            {
                free(pFrame->pData);
            }
            free(pFrame);
        }
    }

    if (pMblk)
    {
        RK_MPI_VO_DestroyGraphicsFrameBuffer(pMblk);
    }

    delete demo;
    return NULL;
}

/*从VENC获取编码视频*/
static void *getStreamThr(void *param)
{
    int nRet = 0;
    void *pData = NULL;
    int i = 0;
    FILE *fp = fopen(sMoviePath, "w+");
    VENC_PACK_S *pPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S));
    VENC_STREAM_S stFrame;

    while (1)
    {
        nRet = g_pEncHandle->rockitVenc_get_stream(g_pEncHandle, &stFrame, pPack, -1);

        if (nRet != RK_SUCCESS)
        {
            printf("venc error %x\n", nRet);
            break;
        }

        pData = g_pEncHandle->rockitVenc_get_streamVirdata(pPack);
        fwrite(pData, 1, stFrame.pstPack->u32Len, fp);
        // fflush(fp);

        printf("u32Len=%d\n", stFrame.pstPack->u32Len);

        g_pEncHandle->rockitVenc_release_stream(g_pEncHandle, &stFrame);
        if (pPack->bStreamEnd == RK_TRUE)
        {
            printf("编码完成\n");
            break;
        }
    }
    if (pPack)
    {
        free(pPack);
    }
    fclose(fp);
    fp = NULL;

    return NULL;
}

int main(int argc, char **argv)
{
    int nRet = 0;
    int nPort = 1;
    VO_DEV nVoDev = 0;
    VO_LAYER_MODE_E nLayerMode = VO_LAYER_MODE_VIDEO;
    VO_MODE_E nScreenMode = VO_MODE_1MUX;
    int nResolution = 0;
    char *pFile = NULL;
    int nVpssGrp = 0;
    int nVdecChnId = 0;
    int nnum = 1;
    RkVdec_S *ahandle[nnum];
    memset(ahandle, 0, sizeof(RkVdec_S *) * nnum);

    printf("ffmpeg version %s\n", av_version_info());

    // 打开网络流
    avformat_network_init();

    /*注意事项：
     *实测当screen mode为单屏模式时，即只开启一个VO通道时，
     *如果enCompressMode设置为压缩模式 VoLayer必须为0~3
     *如果enCompressMode设置为非压缩模式 VoLayer必须为4~7
     *实测HDMI0 VoDev必须为0
     *实测HDMI1 VoDev必须为1
     *实测MIPI0 VoDev必须为2
     *实测MIPI1 VoDev必须为3
     */

    if (argc < 4)
    {
        printf("use %s <port> <VoDev> <VoLayer> <URL或文件路径>\n", argv[0]);
        printf("port: 1:HDMI0 2:HDMI1 3:MIPI0 4:MIPI1 5:VGA 6:LCD 7:LVDS 8:EDP 9:EDP1 10:DP 11:DP1\n");
        printf("VoDev: 0~4\n");
        printf("VoLayer: 0~7\n");
        printf("例如:\n");
        printf("%s 1 0 0 test.mp4\n", argv[0]);
        printf("%s 2 1 0 rtsp://....\n", argv[0]);
        printf("%s 2 3 2 rtsp://....\n", argv[0]);
        return -1;
    }
    else
    {
        nPort = atoi(argv[1]);
        nVoDev = (VO_DEV)atoi(argv[2]);
        g_nVoLayer = (VO_LAYER)atoi(argv[3]);
        pFile = argv[4];
    }

    printf("port:%d\n", nPort);
    printf("VoDev:%d\n", nVoDev);
    printf("VoLayer:%d\n", g_nVoLayer);
    printf("URL:%s\n", pFile);

    /*系统初始化*/
    nRet = RK_MPI_SYS_Init();
    if (nRet != RK_SUCCESS)
    {
        printf("sys init fail %x\n", nRet);
        return -1;
    }

    printf("SYS INIT SUCCESS\n");

    /*初始化vo 设备和图层*/
    if (voPortInit(nPort, nVoDev, g_nVoLayer, nLayerMode, nResolution) != 0)
    {
        return -1;
    }

    /*初始化vo通道*/
    voStartChn(g_nVoLayer, nScreenMode);

    /*初始化VDEC*/
    vdec_init(nVdecChnId);

    /*初始化VPSS*/
    vpssInit(nVpssGrp, 1920, 1080);

    /*VDEC绑定VPSS*/
    rockitVdec_bind_vpss(nVdecChnId, nVpssGrp, VPSS_CHN0);

    /*初始化VENC*/
    vencInit(g_nVencChn);

    /*创建视频队列*/
    nRet = OS_queCreate(&g_MediaQue, 30);
    if (nRet != OS_SOK)
    {
        printf("创建队列失败\n");
    }

    nRet = OS_queCreate(&g_videoQue, 30);
    if (nRet != OS_SOK)
    {
        printf("创建队列失败\n");
    }

    /*创建读取媒体线程*/
    nRet = OS_thrCreate(&g_getMediaThrId, getMediaThr, OS_JOINABLE, OS_THR_STACK_SIZE_DEFAULT, (void *)pFile);
    if (nRet != OS_SOK)
    {
        g_getMediaThrId.hndl = -1;
        printf("create getMediaThr fial\n");
    }

    /*创建送解码线程*/
    nRet = OS_thrCreate(&g_sendDecThrId, sendDecThr, OS_JOINABLE, OS_THR_STACK_SIZE_DEFAULT, NULL);
    if (nRet != OS_SOK)
    {
        g_sendDecThrId.hndl = -1;
        printf("player create sendDecThr fial\n");
    }

    /*创建获取视频送队列线程*/
    nRet = OS_thrCreate(&g_getVideoThrId, getVideoThr, OS_JOINABLE, OS_THR_STACK_SIZE_DEFAULT, NULL);
    if (nRet != OS_SOK)
    {
        g_getVideoThrId.hndl = -1;
        printf("player create getVideoThr fial\n");
    }

    /*创建从队列获取视频处理并送显线程*/
    nRet = OS_thrCreate(&g_videoFrameHandlThrId, videoFrameHandlThr, OS_JOINABLE, OS_THR_STACK_SIZE_DEFAULT, NULL);
    if (nRet != OS_SOK)
    {
        g_videoFrameHandlThrId.hndl = -1;
        printf("player create videoFrameHandlThr fial\n");
    }

    /*创建获取编码视频线程*/
    if (sMoviePath != nullptr)
    {
        nRet = OS_thrCreate(&g_getVencVideoThrId, getStreamThr, OS_JOINABLE, OS_THR_STACK_SIZE_DEFAULT, NULL);
        if (nRet != OS_SOK)
        {
            g_getVencVideoThrId.hndl = -1;
            printf("player create getStreamThr fial\n");
        }
    }

    while (!g_bExit)
    {
        usleep(50000);
    }

    /*删除队列*/
    if (!g_MediaQue.queue)
    {
        OS_queDelete(&g_MediaQue);
    }

    if (!g_videoQue.queue)
    {
        OS_queDelete(&g_videoQue);
    }

    /*VENC去初始化*/
    rockitVenc_release(g_pEncHandle);

    /*VDEC解除绑定VPSS*/
    rockitVdec_unbind_vpss(nVdecChnId, nVpssGrp, VPSS_CHN0);

    /*VPSS去初始化*/
    vpssUnInit();

    /*VDEC去初始化*/
    vdec_uninit();

    /*禁用vo各个通道**/
    voStopChn(g_nVoLayer, nScreenMode);

    /*禁用vo设备和图层*/
    voPortUninit(nVoDev, g_nVoLayer);

    RK_MPI_VO_CloseFd();
    RK_MPI_SYS_Exit();

    return 0;
}
