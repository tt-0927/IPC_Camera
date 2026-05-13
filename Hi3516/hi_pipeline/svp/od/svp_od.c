/*
 * @FilePath     : svp_od.c
 * @Author       : cyc
 * @Date         : 2025-07-25 09:23:12
 * @LastEditors  : cyc
 * @LastEditTime : 2025-07-30 20:36:44
 * @Description  : 海思遮挡侦测封装
 */

#include "svp_od.h"
#include "mpi_common.h"

/*------------------------ 常量定义 ------------------------*/
#define OT_SAMPLE_IVE_RIGHT_SHIFT_TWENTY_EIGHT  28
#define OT_IVE_CHAR_CALW                        8
#define OT_IVE_CHAR_CALH                        8
#define OT_IVE_CHAR_NUM                         (OT_IVE_CHAR_CALW * OT_IVE_CHAR_CALH)
#define OT_SAMPLE_IVE_OD_POINT_NUM              10
#define OT_SAMPLE_IVE_OD_LINEAR_NUM             2
#define OT_SAMPLE_IVE_OD_LINEAR_POINT0_X        80
#define OT_SAMPLE_IVE_OD_LINEAR_POINT0_Y        0
#define OT_SAMPLE_IVE_OD_LINEAR_POINT1_X        80
#define OT_SAMPLE_IVE_OD_LINEAR_POINT1_Y        20
#define OT_SAMPLE_IVE_OD_NUM_TWO                2

/**
 * @brief       : 遮挡侦测初始化
 * @author      : cyc
 * @param        {HiMd_S} *pHandle：句柄
 * @return       {*}成功返回0,失败返回-1
 */
 static int svpOd_init(HiOd_S *pHandle)
 {
     if (NULL == pHandle || pHandle->bInited)
     {
         return TD_FAILURE;
     }
     HiOdNeedParam_S *pNeedParam = &pHandle->stNeedParam;
     
     /* 创建图像内存 */
     CHECK_API_RETURN(sample_common_ive_create_image(&pHandle->stSrcFrame, OT_SVP_IMG_TYPE_U8C1, pNeedParam->nWidth, pNeedParam->nHeight));
 
     CHECK_API_RETURN(sample_common_ive_create_image(&pHandle->stIntegFrame, OT_SVP_IMG_TYPE_U64C1, pNeedParam->nWidth, pNeedParam->nHeight));

     pHandle->stIntegCtrl.out_ctrl = OT_IVE_INTEG_OUT_CTRL_COMBINE;

     pHandle->nWidth = pNeedParam->nWidth / OT_IVE_CHAR_CALW;
     pHandle->nHeight = pNeedParam->nHeight / OT_IVE_CHAR_CALH;
 
 
     /* 初始化标志 */
     pHandle->bInited = TD_TRUE;
 
     mpi_md_log("OD init success, size: %dx%d\n",  pNeedParam->nWidth, pNeedParam->nHeight);
     return TD_SUCCESS;
 }
 
 /**
  * @brief       : 遮挡侦测去初始化
  * @author      : cyc
  * @param        {HiMd_S} *pHandle：句柄
  * @return       {*}成功返回0,失败返回-1
  */
 static int svpOd_uninit(HiOd_S *pHandle)
 {
     if (NULL == pHandle)
     {
         return TD_FAILURE;
     }
 
     /* 释放图像内存 */
     if (pHandle->stSrcFrame.phys_addr[0] != 0)
     {
         sample_svp_mmz_free(pHandle->stSrcFrame.phys_addr[0], pHandle->stSrcFrame.virt_addr[0]);
         memset(&pHandle->stSrcFrame, 0, sizeof(ot_svp_src_img));
     }
  
     if (pHandle->stIntegFrame.phys_addr[0] != 0)
     {
         sample_svp_mmz_free(pHandle->stIntegFrame.phys_addr[0], pHandle->stIntegFrame.virt_addr[0]);
         memset(&pHandle->stIntegFrame, 0, sizeof(ot_svp_dst_img));
     }
 
     return TD_SUCCESS;
 }

 /**
 * @brief  准备 DMA 描述符，用于把 VPSS 帧拷入 IVE 输入缓冲
 * @param[in]  pHandle   句柄
 * @param[in]  pFrameInfo VPSS 帧信息
 * @param[out] pSrcData  源 DMA 描述符
 * @param[out] pDstData  目的 DMA 描述符
 */
static td_void sample_ive_prepare_dma_data(HiOd_S *pHandle, ot_video_frame_info *pFrameInfo,
    ot_svp_data *pSrcData, ot_svp_data *pDstData)
{
    pSrcData->virt_addr = sample_svp_convert_ptr_to_addr(td_u64, pFrameInfo->video_frame.virt_addr[0]);
    pSrcData->phys_addr = pFrameInfo->video_frame.phys_addr[0];
    pSrcData->stride = pFrameInfo->video_frame.stride[0];
    pSrcData->width = pFrameInfo->video_frame.width;
    pSrcData->height = pFrameInfo->video_frame.height;

    pDstData->virt_addr = pHandle->stSrcFrame.virt_addr[0];
    pDstData->phys_addr = pHandle->stSrcFrame.phys_addr[0];
    pDstData->stride = pFrameInfo->video_frame.stride[0];
    pDstData->width = pFrameInfo->video_frame.width;
    pDstData->height = pFrameInfo->video_frame.height;
}

/**
 * @brief  轮询等待 IVE 任务完成
 * @param  handle IVE 任务句柄
 * @return TD_SUCCESS / 错误码
 */
static td_s32 sample_ive_query_task(ot_ive_handle handle)
{
    td_s32 ret;
    td_bool is_block = TD_TRUE;
    td_bool is_finish = TD_FALSE;
    ret = ss_mpi_ive_query(handle, &is_finish, is_block);
    while (ret == OT_ERR_IVE_QUERY_TIMEOUT) {
        usleep(100);
        ret = ss_mpi_ive_query(handle, &is_finish, is_block);
    }
    sample_svp_check_exps_return(ret != TD_SUCCESS, ret, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error(%#x),ss_mpi_ive_query failed!\n", ret);
    return TD_SUCCESS;
}

/**
 * @brief  从积分图提取 8×8 cell 的均值和方差特征
 * @param  pHandle  句柄
 * @param  char_point 输出特征点数组
 * @param  length   数组长度(最大 OT_IVE_CHAR_NUM)
 * @return TD_SUCCESS / 错误码
 */
static td_s32 sample_ive_get_char_point(HiOd_S *pHandle, ot_point char_point[], td_u32 length)
{
    td_u64 *vir_data = TD_NULL;
    td_u32 i, j;
    td_u64 top_left, top_right, btm_left, btm_right;
    td_u64 *top_row_ptr = TD_NULL;
    td_u64 *btm_row_ptr = TD_NULL;
    td_u64 block_sum, block_sqrt;
    td_float sqrt_val;
    vir_data = sample_svp_convert_addr_to_ptr(td_u64, pHandle->stIntegFrame.virt_addr[0]);
    sample_svp_check_exps_return(length > OT_IVE_CHAR_CALW * OT_IVE_CHAR_CALH, OT_ERR_IVE_ILLEGAL_PARAM,
        SAMPLE_SVP_ERR_LEVEL_ERROR, "length(%u) is larger than %u\n", length, OT_IVE_CHAR_CALW * OT_IVE_CHAR_CALH);

    for (j = 0; (j < OT_IVE_CHAR_CALH) && (j < length); j++) {
        top_row_ptr = (0 == j) ? (vir_data) : (vir_data + (j * pHandle->nHeight - 1) * pHandle->stIntegFrame.stride[0]);
        btm_row_ptr = vir_data + ((j + 1) * pHandle->nHeight - 1) * pHandle->stIntegFrame.stride[0];

        for (i = 0; i < OT_IVE_CHAR_CALW; i++) {
            top_left = (0 == j) ? (0) : ((0 == i) ? (0) : (top_row_ptr[i * pHandle->nWidth - 1]));
            top_right = (0 == j) ? (0) : (top_row_ptr[(i + 1) * pHandle->nWidth - 1]);
            btm_left = (0 == i) ? (0) : (btm_row_ptr[i * pHandle->nWidth - 1]);
            btm_right = btm_row_ptr[(i + 1) * pHandle->nWidth - 1];

            block_sum = (top_left & 0xfffffffLL) + (btm_right & 0xfffffffLL) -
                (btm_left & 0xfffffffLL) - (top_right & 0xfffffffLL);

            block_sqrt = (top_left >> OT_SAMPLE_IVE_RIGHT_SHIFT_TWENTY_EIGHT) +
                        (btm_right >> OT_SAMPLE_IVE_RIGHT_SHIFT_TWENTY_EIGHT) -
                        (btm_left >> OT_SAMPLE_IVE_RIGHT_SHIFT_TWENTY_EIGHT) -
                        (top_right >> OT_SAMPLE_IVE_RIGHT_SHIFT_TWENTY_EIGHT);

            /* mean */
            char_point[j * OT_IVE_CHAR_CALW + i].x = (td_s32)(block_sum / (pHandle->nWidth * pHandle->nHeight));
            /* sigma=sqrt(1/(w*h)*sum((x(i,j)-mean)^2)= sqrt(sum(x(i,j)^2)/(w*h)-mean^2) */
            sqrt_val = (td_s64)(block_sqrt / (pHandle->nWidth * pHandle->nHeight)) -
                char_point[j * OT_IVE_CHAR_CALW + i].x * char_point[j * OT_IVE_CHAR_CALW + i].x;
            char_point[j * OT_IVE_CHAR_CALW + i].y = (td_s32)sqrt(sqrt_val);
        }
    }
    return TD_SUCCESS;
}

/**
 * @brief  线性 2D 分类器：判断落在多边形内的特征点个数
 * @param  char_point   特征点数组
 * @param  char_num     特征点数量
 * @param  linear_point 多边形顶点数组
 * @param  linear_num   多边形顶点数量
 * @return 落在多边形内的点数
 */
static td_s32 sample_ive_linear_2d_classifer(ot_point *char_point, td_s32 char_num)
{
    td_s32 result_num;
    td_s32 i;
    td_bool test_flag;
    result_num = 0;
    for (i = 0; i < char_num; i++) 
    {
        test_flag = TD_FALSE;
     
        /* 测试出来的值，暂定 */
        if((char_point[i].x < 60 || char_point[i].x > 200) && char_point[i].y < 20)
        {
            test_flag = TD_TRUE;
        }
        
        if (test_flag == TD_TRUE) {
            result_num++;
        }
    }
    return result_num;
}

/**
 * @brief  送帧给 OD 处理：DMA->积分图->特征提取->分类
 * @param  pHandle   句柄
 * @param  pFrameInfo 输入帧
 * @return TD_SUCCESS-遮挡，TD_FAILURE-未遮挡
 */
 static int svpOd_sendFrame(HiOd_S *pHandle, ot_video_frame_info *pFrameInfo)
 {
     if (NULL == pHandle || NULL == pFrameInfo  || !pHandle->bInited)
     {
         mpi_md_log("Invalid parameter or not initialized\n");
         return TD_FAILURE;
     }
 
     ot_svp_data stSrcData, stDstData;
     ot_ive_handle handle;
     ot_ive_dma_ctrl dma_ctrl = { OT_IVE_DMA_MODE_DIRECT_COPY, 0, 0, 0, 0 };
     ot_point char_point[OT_IVE_CHAR_NUM];
     int nNum;
 
     /* 检查帧尺寸 */
     if (pFrameInfo->video_frame.width != pHandle->stNeedParam.nWidth ||
         pFrameInfo->video_frame.height != pHandle->stNeedParam.nHeight)
     {
         mpi_md_log("Frame size mismatch: expected %dx%d, got %dx%d\n",
                 pHandle->stNeedParam.nWidth, pHandle->stNeedParam.nHeight,
                 pFrameInfo->video_frame.width, pFrameInfo->video_frame.height);
         return TD_FAILURE;
     }
 
     /* 使用IVE DMA将输入帧拷贝到当前帧 */
     sample_ive_prepare_dma_data(pHandle,pFrameInfo, &stSrcData,&stDstData);

     CHECK_API_RETURN(ss_mpi_ive_dma(&handle,&stSrcData,&stDstData,&dma_ctrl, TD_FALSE));

     CHECK_API_RETURN(ss_mpi_ive_integ(&handle, &pHandle->stSrcFrame, &pHandle->stIntegFrame, &pHandle->stIntegCtrl, TD_TRUE));

     CHECK_API_RETURN(sample_ive_query_task(handle));

     CHECK_API_RETURN(sample_ive_get_char_point(pHandle, char_point, OT_IVE_CHAR_NUM));
 
     nNum = sample_ive_linear_2d_classifer(char_point, OT_IVE_CHAR_NUM);
     if(nNum >= 0)
     {
         return nNum;
     }
     return TD_FAILURE;
 }
 
 
 HiOd_S *svpOd_alloc(HiOdNeedParam_S stNeedParam)
 {
    HiOd_S *pHandle = (HiOd_S *)malloc(sizeof(HiOd_S));
     memset(pHandle, 0, sizeof(HiOd_S));
 
     //info /**********************必需参数***************************/
     pHandle->stNeedParam = stNeedParam;
 
     /* 初始化状态 */
     pHandle->bInited = TD_FALSE;
     //info /**********************函数列表***************************/
     pHandle->svpOd_init                     = svpOd_init;
     pHandle->svpOd_uninit                   = svpOd_uninit;
     pHandle->svpOd_sendFrame                = svpOd_sendFrame;
 
     return pHandle;
 }
 
 void svpOd_release(HiOd_S *pHandle)
 {
     if (pHandle)
     {
         if (pHandle->bInited)
         {
             pHandle->svpOd_uninit(pHandle);
         }
         free(pHandle);
         pHandle = NULL;
     }
 }