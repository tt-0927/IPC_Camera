/**
 * @FilePath     : svp_ld.c
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-10-27 09:14:03
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-10-28 16:13:14
 * @Description  : 物品检测封装（遗留/拿取）
 */

#include "svp_ld.h"
#include "mpi_common.h"

/* IVE 物品检测比较阈值 */
#define OT_SAMPLE_IVE_LD_DIFF_THRESHOLD 20

/**
 * @brief   : 判断点是否在多边形内(射线法)
 * @param    {td_float} fX 点横坐标
 * @param    {td_float} fY 点纵坐标
 * @param    {HiLdPolygon_S} *pPolygon 多边形区域结果参数
 * @return   {td_bool} 成功返回TD_TRUE,失败返回TD_FALSE
 */
static td_bool svpLd_pointInPolygon(td_float fX, td_float fY, const HiLdPolygon_S *pPolygon)
{
    td_u32 i, j;
    td_bool bInside = TD_FALSE;
    td_float fDeltaY;

    /* 参数校验 */
    if (pPolygon == TD_NULL || pPolygon->u32PointNum < 3)
    {
        return TD_FALSE;
    }

    for (i = 0, j = pPolygon->u32PointNum - 1; i < pPolygon->u32PointNum; j = i++)
    {
        fDeltaY = pPolygon->aPoints[j].fY - pPolygon->aPoints[i].fY;
        /* 跨越测试：点的Y坐标是否在边的Y坐标范围内 */
        if (((pPolygon->aPoints[i].fY > fY) != (pPolygon->aPoints[j].fY > fY)))
        {
            /* 显式检查避免除零（虽然理论上不会发生） */
            if (fDeltaY != 0.0f)
            {
                /* 计算射线与边的交点X坐标 */
                td_float fIntersectX = (pPolygon->aPoints[j].fX - pPolygon->aPoints[i].fX) * 
                                       (fY - pPolygon->aPoints[i].fY) / fDeltaY + 
                                       pPolygon->aPoints[i].fX;
                
                /* 交点在测试点右侧，射线与边相交 */
                if (fX < fIntersectX)
                {
                    bInside = !bInside;
                }
            }
        }
    }

    return bInside;
}

/**
 * @brief   : 创建区域mask图像(多边形)
 * @param    {HiLd_S} *pHandle 句柄
 * @param    {td_u32} u32RegionIdx 区域索引号
 * @return   {int} 成功返回0,失败返回-1
 */
static int svpLd_createMask(HiLd_S *pHandle, td_u32 u32RegionIdx)
{
    if (u32RegionIdx >= pHandle->stNeedParam.u32RegionNum)
    {
        return TD_FAILURE;
    }

    HiLdPolygon_S *pPolygon = &pHandle->stNeedParam.stRegions[u32RegionIdx];
    if (!pPolygon->bEnable || pPolygon->u32PointNum < 3)
    {
        return TD_SUCCESS;
    }

    /* 创建mask图像 */
    CHECK_API_RETURN(sample_common_ive_create_image(&pHandle->stMaskFrame[u32RegionIdx],
                                                    OT_SVP_IMG_TYPE_U8C1,
                                                    pHandle->stNeedParam.u32Width,
                                                    pHandle->stNeedParam.u32Height));

    /* 填充mask：区域内为255，区域外为0 */
    td_u8 *pMask = sample_svp_convert_addr_to_ptr(td_u8, pHandle->stMaskFrame[u32RegionIdx].virt_addr[0]);
    td_u32 stride = pHandle->stMaskFrame[u32RegionIdx].stride[0];

    memset(pMask, 0, stride * pHandle->stNeedParam.u32Height);

    /* 遍历图像每个像素，判断是否在多边形内 */
    for (td_u32 y = 0; y < pHandle->stNeedParam.u32Height; y++)
    {
        for (td_u32 x = 0; x < pHandle->stNeedParam.u32Width; x++)
        {
            if (svpLd_pointInPolygon((td_float) x, (td_float) y, pPolygon))
            {
                pMask[y * stride + x] = 255;
            }
        }
    }

    return TD_SUCCESS;
}

/**
 * @brief   : 物品检测初始化
 * @param    {HiLd_S} *pHandle 句柄
 * @return   {int} 成功返回0,失败返回-1
 */
static int svpLd_init(HiLd_S *pHandle)
{
    if (NULL == pHandle || pHandle->bInited)
    {
        return TD_FAILURE;
    }

    HiLdNeedParam_S *pNeedParam = &pHandle->stNeedParam;

    /* 创建图像内存 */
    CHECK_API_RETURN(sample_common_ive_create_image(&pHandle->stRefFrame,
                                                    OT_SVP_IMG_TYPE_U8C1,
                                                    pNeedParam->u32Width,
                                                    pNeedParam->u32Height));

    CHECK_API_RETURN(sample_common_ive_create_image(&pHandle->stCurFrame,
                                                    OT_SVP_IMG_TYPE_U8C1,
                                                    pNeedParam->u32Width,
                                                    pNeedParam->u32Height));

    CHECK_API_RETURN(sample_common_ive_create_image(&pHandle->stDiffFrame,
                                                    OT_SVP_IMG_TYPE_U8C1,
                                                    pNeedParam->u32Width,
                                                    pNeedParam->u32Height));

    CHECK_API_RETURN(sample_common_ive_create_image(&pHandle->stIntegFrame,
                                                    OT_SVP_IMG_TYPE_U32C1,
                                                    pNeedParam->u32Width,
                                                    pNeedParam->u32Height));

    CHECK_API_RETURN(sample_common_ive_create_image(&pHandle->stMaskedImg,
                                                    OT_SVP_IMG_TYPE_U8C1,
                                                    pNeedParam->u32Width,
                                                    pNeedParam->u32Height));

    /* 创建各区域的mask和积分图 */
    for (td_u32 i = 0; i < pNeedParam->u32RegionNum && i < SVP_LD_MAX_REGION_NUM; i++)
    {
        if (pNeedParam->stRegions[i].bEnable)
        {
            CHECK_API_RETURN(svpLd_createMask(pHandle, i));

            CHECK_API_RETURN(sample_common_ive_create_image(&pHandle->stRegionInteg[i],
                                                            OT_SVP_IMG_TYPE_U32C1,
                                                            pNeedParam->u32Width,
                                                            pNeedParam->u32Height));
        }
    }

    /* 初始化控制参数 */
    pHandle->stSubCtrl.mode = OT_IVE_SUB_MODE_ABS;
    pHandle->stIntegCtrl.out_ctrl = OT_IVE_INTEG_OUT_CTRL_SUM;

    /* 初始化结果 */
    memset(pHandle->stResults, 0, sizeof(pHandle->stResults));

    /* 初始化标志 */
    pHandle->bFirstFrame = TD_TRUE;
    pHandle->bInited = TD_TRUE;

    mpi_ld_log("物品检测初始化成功, 尺寸: %dx%d, 有效区域数: %u",
               pNeedParam->u32Width,
               pNeedParam->u32Height,
               pNeedParam->u32RegionNum);
    return TD_SUCCESS;
}

/**
* @brief    : 物品检测去初始化
* @param     {HiLd_S} *pHandle 句柄
* @return    {int} 成功返回0,失败返回-1
*/
static int svpLd_uninit(HiLd_S *pHandle)
{
    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }

    /* 释放图像内存 */
    if (pHandle->stRefFrame.phys_addr[0] != 0)
    {
        sample_svp_mmz_free(pHandle->stRefFrame.phys_addr[0], pHandle->stRefFrame.virt_addr[0]);
        memset(&pHandle->stRefFrame, 0, sizeof(ot_svp_src_img));
    }

    if (pHandle->stCurFrame.phys_addr[0] != 0)
    {
        sample_svp_mmz_free(pHandle->stCurFrame.phys_addr[0], pHandle->stCurFrame.virt_addr[0]);
        memset(&pHandle->stCurFrame, 0, sizeof(ot_svp_src_img));
    }

    if (pHandle->stDiffFrame.phys_addr[0] != 0)
    {
        sample_svp_mmz_free(pHandle->stDiffFrame.phys_addr[0], pHandle->stDiffFrame.virt_addr[0]);
        memset(&pHandle->stDiffFrame, 0, sizeof(ot_svp_dst_img));
    }

    if (pHandle->stIntegFrame.phys_addr[0] != 0)
    {
        sample_svp_mmz_free(pHandle->stIntegFrame.phys_addr[0], pHandle->stIntegFrame.virt_addr[0]);
        memset(&pHandle->stIntegFrame, 0, sizeof(ot_svp_dst_img));
    }

    if (pHandle->stMaskedImg.phys_addr[0] != 0)
    {
        sample_svp_mmz_free(pHandle->stMaskedImg.phys_addr[0], pHandle->stMaskedImg.virt_addr[0]);
        memset(&pHandle->stMaskedImg, 0, sizeof(ot_svp_dst_img));
    }

    /* 释放各区域内存 */
    for (td_u32 i = 0; i < SVP_LD_MAX_REGION_NUM; i++)
    {
        if (pHandle->stMaskFrame[i].phys_addr[0] != 0)
        {
            sample_svp_mmz_free(pHandle->stMaskFrame[i].phys_addr[0], pHandle->stMaskFrame[i].virt_addr[0]);
            memset(&pHandle->stMaskFrame[i], 0, sizeof(ot_svp_dst_img));
        }

        if (pHandle->stRegionInteg[i].phys_addr[0] != 0)
        {
            sample_svp_mmz_free(pHandle->stRegionInteg[i].phys_addr[0], pHandle->stRegionInteg[i].virt_addr[0]);
            memset(&pHandle->stRegionInteg[i], 0, sizeof(ot_svp_dst_img));
        }
    }

    return TD_SUCCESS;
}

/**
 * @brief   : 计算积分图的全图和
 * @param    {ot_svp_dst_img} *pInteg 积分图
 * @return   {td_u64} 全图和
 */
static td_u64 svpLd_calcIntegFullSum(ot_svp_dst_img *pInteg)
{
    td_u32 *pData = sample_svp_convert_addr_to_ptr(td_u32, pInteg->virt_addr[0]);
    td_u32 stride = pInteg->stride[0];
    /* 返回右下角的值即为全图和 */
    return pData[(pInteg->height - 1) * stride + (pInteg->width - 1)];
}

/**
* @brief    : 送帧给LD进行检测处理
* @param     {HiLd_S} *pHandle 句柄
* @param     {ot_video_frame_info} *pFrameInfo 输入帧信息
* @return    {int} 成功返回0,失败返回-1
*/
static int svpLd_sendFrame(HiLd_S *pHandle, ot_video_frame_info *pFrameInfo)
{
    if (NULL == pHandle || NULL == pFrameInfo || !pHandle->bInited)
    {
        mpi_ld_log("无效参数或未初始化");
        return TD_FAILURE;
    }

    td_bool is_instant = TD_TRUE;
    ot_ive_handle handle;

    /* 检查帧尺寸 */
    if (pFrameInfo->video_frame.width != pHandle->stNeedParam.u32Width ||
        pFrameInfo->video_frame.height != pHandle->stNeedParam.u32Height)
    {
        mpi_ld_log("帧大小不匹配：期望 %dx%d,实际 %dx%d\n",
                   pHandle->stNeedParam.u32Width,
                   pHandle->stNeedParam.u32Height,
                   pFrameInfo->video_frame.width,
                   pFrameInfo->video_frame.height);
        return TD_FAILURE;
    }

    /* 使用IVE DMA将输入帧拷贝到当前帧 */
    CHECK_API_RETURN(sample_common_ive_dma_image(pFrameInfo, &pHandle->stCurFrame, is_instant));

    /* 如果是第一帧，仅保存为参考帧 */
    if (pHandle->bFirstFrame)
    {
        CHECK_API_RETURN(sample_common_ive_dma_image(pFrameInfo, &pHandle->stRefFrame, is_instant));
        pHandle->bFirstFrame = TD_FALSE;

        /* 清空结果 */
        memset(pHandle->stResults, 0, sizeof(pHandle->stResults));

        mpi_ld_log("第一帧已处理，参考帧已初始化");
        return TD_SUCCESS;
    }

    /* 计算差分图像：当前帧 - 参考帧 */
    CHECK_API_RETURN(ss_mpi_ive_sub(&handle,
                                    &pHandle->stCurFrame,
                                    &pHandle->stRefFrame,
                                    &pHandle->stDiffFrame,
                                    &pHandle->stSubCtrl,
                                    is_instant));

    /* 计算全图积分图(用于计算ST) */
    CHECK_API_RETURN(
        ss_mpi_ive_integ(&handle, &pHandle->stDiffFrame, &pHandle->stIntegFrame, &pHandle->stIntegCtrl, is_instant));

    /* 计算全图变化量ST */
    td_u64 st = svpLd_calcIntegFullSum(&pHandle->stIntegFrame);

    /* 处理各个检测区域 */
    for (td_u32 i = 0; i < pHandle->stNeedParam.u32RegionNum && i < SVP_LD_MAX_REGION_NUM; i++)
    {
        HiLdPolygon_S *pPolygon = &pHandle->stNeedParam.stRegions[i];
        if (!pPolygon->bEnable || pPolygon->u32PointNum < 3)
        {
            pHandle->stResults[i].bValid = TD_FALSE;
            continue;
        }

        /* 使用AND操作应用mask */
        CHECK_API_RETURN(
            ss_mpi_ive_and(&handle, &pHandle->stDiffFrame, &pHandle->stMaskFrame[i], &pHandle->stMaskedImg, is_instant));

        /* 计算区域积分图 */
        CHECK_API_RETURN(
            ss_mpi_ive_integ(&handle, &pHandle->stMaskedImg, &pHandle->stRegionInteg[i], &pHandle->stIntegCtrl, is_instant));

        /* 计算区域变化量S1 */
        td_u64 s1 = svpLd_calcIntegFullSum(&pHandle->stRegionInteg[i]);

        /* 计算灵敏度：sensitivity = 100 - (S1/ST)*100 */
        td_u32 sensitivity = 100;
        if (st > 0 && s1 > 0)
        {
            sensitivity = 100 - (td_u32) ((s1 * 100) / st);
            /* 防止溢出 */
            if (sensitivity < 0)
            {
                sensitivity = 0;
            }
        }

        pHandle->stResults[i].bValid = TD_TRUE;
        pHandle->stResults[i].u32Sensitivity = sensitivity;
        pHandle->stResults[i].u64ST = st;
        pHandle->stResults[i].u64S1 = s1;
    }

    /* 根据配置决定是否自动更新参考帧 */
    if (pHandle->stExParam.bManualUpdate == TD_FALSE)
    {
        CHECK_API_RETURN(sample_common_ive_dma_image(pFrameInfo, &pHandle->stRefFrame, is_instant));
    }

    return TD_SUCCESS;
}

/**
* @brief    : 获取物品检测结果(仅灵敏度)
* @param     {HiLd_S} *pHandle 句柄
* @param     {HiLdRegionResult_S} *pResults 结果输出
* @param     {td_u32} u32MaxNum 最大结果数量
* @return    {int} 成功返回0,失败返回-1
*/
static int svpLd_getResult(HiLd_S *pHandle, HiLdRegionResult_S *pResults, td_u32 u32MaxNum)
{
    if (NULL == pHandle || NULL == pResults || !pHandle->bInited)
    {
        return TD_FAILURE;
    }

    td_u32 copy_num = (u32MaxNum < pHandle->stNeedParam.u32RegionNum) ? u32MaxNum : pHandle->stNeedParam.u32RegionNum;

    memcpy(pResults, pHandle->stResults, copy_num * sizeof(HiLdRegionResult_S));

    return TD_SUCCESS;
}

/**
* @brief    : 手动更新参考帧
* @param     {HiLd_S} *pHandle 句柄
* @param     {ot_video_frame_info} *pFrameInfo 新的参考帧信息
* @return    {int} 成功返回0,失败返回-1
*/
static int svpLd_updateRef(HiLd_S *pHandle, ot_video_frame_info *pFrameInfo)
{
    if (NULL == pHandle || NULL == pFrameInfo || !pHandle->bInited)
    {
        mpi_ld_log("无效参数或更新参考帧时未进行初始化");
        return TD_FAILURE;
    }

    /* 检查帧尺寸 */
    if (pFrameInfo->video_frame.width != pHandle->stNeedParam.u32Width ||
        pFrameInfo->video_frame.height != pHandle->stNeedParam.u32Height)
    {
        mpi_ld_log("参考帧尺寸不匹配");
        return TD_FAILURE;
    }

    td_bool is_instant = TD_TRUE;
    CHECK_API_RETURN(sample_common_ive_dma_image(pFrameInfo, &pHandle->stRefFrame, is_instant));

    mpi_ld_log("参考帧已手动更新");
    return TD_SUCCESS;
}

/**
 * @brief   : 打印结果
 * @param    {HiLd_S} *pHandle 句柄
 */
static void svpLd_printResult(HiLd_S *pHandle)
{
    for (int i = 0; i < SVP_LD_MAX_REGION_NUM; i++)
    {
        if (pHandle->stResults[i].bValid)
        {
            mpi_ld_log("区域[%d] 灵敏度:%d 全图变化量:%llu 区域变化量:%llu",
                       i,
                       pHandle->stResults[i].u32Sensitivity,
                       pHandle->stResults[i].u64ST,
                       pHandle->stResults[i].u64S1);
        }
    }
    printf("\n");
}

HiLd_S *svpLd_alloc(HiLdNeedParam_S stNeedParam)
{
    HiLd_S *pHandle = (HiLd_S *)malloc(sizeof(HiLd_S));
    memset(pHandle, 0, sizeof(HiLd_S));

    //info /**********************必需参数***************************/
    pHandle->stNeedParam = stNeedParam;
    
    //info /**********************功能参数***************************/
    pHandle->stExParam.u8DiffThreshold = OT_SAMPLE_IVE_LD_DIFF_THRESHOLD;
    pHandle->stExParam.bManualUpdate = TD_FALSE;  // 默认自动更新模式

    /* 初始化状态 */
    pHandle->bFirstFrame = TD_TRUE;
    pHandle->bInited = TD_FALSE;
    
    //info /**********************函数列表***************************/
    pHandle->svpLd_init         = svpLd_init;
    pHandle->svpLd_uninit       = svpLd_uninit;
    pHandle->svpLd_sendFrame    = svpLd_sendFrame;
    pHandle->svpLd_getResult    = svpLd_getResult;
    pHandle->svpLd_updateRef    = svpLd_updateRef;
    pHandle->svpLd_printResult  = svpLd_printResult;

    return pHandle;
}

void svpLd_release(HiLd_S *pHandle)
{
    if (pHandle)
    {
        if (pHandle->bInited)
        {
            pHandle->svpLd_uninit(pHandle);
        }
        free(pHandle);
        pHandle = NULL;
    }
}
