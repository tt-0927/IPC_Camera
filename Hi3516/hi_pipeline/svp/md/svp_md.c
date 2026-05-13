/**
 * @FilePath     : svp_md.c
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-07-14 14:08:09
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-08-08 10:48:34
 * @Description  : 海思移动侦测封装 
 */

#include "svp_md.h"
#include "mpi_common.h"

#define OT_SAMPLE_IVE_MD_ADD_X_VAL          32768
#define OT_SAMPLE_IVE_MD_ADD_Y_VAL          32768
#define OT_SAMPLE_IVE_MD_AREA_THR_STEP      8
#define OT_SAMPLE_IVE_MD_NUM_TWO            2
#define OT_SAMPLE_IVE_SAD_THRESHOLD         100

/* 初始化次数 */
static int gs_nInited = 0;

/**
 * @brief       : 移动侦测初始化
 * @author      : zhouzirui
 * @param        {HiMd_S} *pHandle：句柄
 * @return       {*}成功返回0,失败返回-1
 */
static int svpMd_init(HiMd_S *pHandle)
{
    if (NULL == pHandle || pHandle->bInited)
    {
        return TD_FAILURE;
    }
    HiMdNeedParam_S *pNeedParam = &pHandle->stNeedParam;
    HiMdExParam_S *pExParam = &pHandle->stExParam;
    
    /* 通道号 */
    ot_md_chn nChn = pNeedParam->nChn;
    /* 避免创建两通道时，重复初始化 */
    if(gs_nInited == 0)
    {
        /* 移动侦测初始化 */
        CHECK_API_RETURN(ss_ivs_md_init());
    }
    gs_nInited++;

    /* 设置MD属性 */
    ot_md_attr stMdAttr = (ot_md_attr){
        .alg_mode = pExParam->enAlgMode,
        .sad_mode = pExParam->enSadMode,
        .sad_out_ctrl = pExParam->enSadOutCtrl,
        .width = pNeedParam->u32Width,
        .height = pNeedParam->u32Height,
        .sad_threshold= pExParam->u16SadThreshold,
        .ccl_ctrl = pExParam->stCclCtrl,
        .add_ctrl = pExParam->stAddCtrl,
    };

    /* 创建MD通道 */
    CHECK_API_RETURN(ss_ivs_md_create_chn(nChn, &stMdAttr));

    /* 创建图像内存 */
    CHECK_API_RETURN(sample_common_ive_create_image(&pHandle->stRefFrame, OT_SVP_IMG_TYPE_U8C1, pNeedParam->u32Width, pNeedParam->u32Height));

    CHECK_API_RETURN(sample_common_ive_create_image(&pHandle->stCurFrame, OT_SVP_IMG_TYPE_U8C1, pNeedParam->u32Width, pNeedParam->u32Height));

    /* 创建blob输出内存 */
    td_u32 u32BlobSize = sizeof(ot_ive_ccblob);
    CHECK_API_RETURN(sample_common_ive_create_mem_info(&pHandle->stBlob, u32BlobSize));

    /* 分配SAD输出图像内存 */
    if (pExParam->enSadOutCtrl != OT_IVE_SAD_OUT_CTRL_THRESHOLD)
    {
        pHandle->stSadImg.type = (pExParam->enSadOutCtrl == OT_IVE_SAD_OUT_CTRL_16BIT_BOTH ||
                                  pExParam->enSadOutCtrl == OT_IVE_SAD_OUT_CTRL_16BIT_SAD)
                                     ? OT_SVP_IMG_TYPE_U16C1
                                     : OT_SVP_IMG_TYPE_U8C1;

        // 根据sad_mode计算输出尺寸 [参考资料显示高、宽分别为cur的1/4、1/8、1/16]
        td_u32 u32SadWidth = pNeedParam->u32Width;
        td_u32 u32SadHeight = pNeedParam->u32Height;
        switch (pExParam->enSadMode)
        {
        case OT_IVE_SAD_MODE_MB_4X4:
            u32SadWidth /= 4;
            u32SadHeight /= 4;
            break;
        case OT_IVE_SAD_MODE_MB_8X8:
            u32SadWidth /= 8;
            u32SadHeight /= 8;
            break;
        case OT_IVE_SAD_MODE_MB_16X16:
            u32SadWidth /= 16;
            u32SadHeight /= 16;
            break;
        default:
            break;
        }

        CHECK_API_RETURN(sample_common_ive_create_image(&pHandle->stSadImg, pHandle->stSadImg.type, u32SadWidth, u32SadHeight));
    }

    /* 初始化标志 */
    pHandle->bFirstFrame = TD_TRUE;
    pHandle->bInited = TD_TRUE;

    mpi_md_log("MD init success, chn: %d, size: %dx%d\n", pNeedParam->nChn, pNeedParam->u32Width, pNeedParam->u32Height);
    return TD_SUCCESS;
}

/**
 * @brief       : 移动侦测去初始化
 * @author      : zhouzirui
 * @param        {HiMd_S} *pHandle：句柄
 * @return       {*}成功返回0,失败返回-1
 */
static int svpMd_uninit(HiMd_S *pHandle)
{
    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }

    HiMdNeedParam_S *pNeedParam = &pHandle->stNeedParam;
    // HiMdExParam_S *pExParam = &pHandle->stExParam;

    /* 通道号 */
    ot_md_chn nChn = pNeedParam->nChn;

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
 
    if (pHandle->stSadImg.phys_addr[0] != 0)
    {
        sample_svp_mmz_free(pHandle->stSadImg.phys_addr[0], pHandle->stSadImg.virt_addr[0]);
        memset(&pHandle->stSadImg, 0, sizeof(ot_svp_dst_img));
    }
 
    if (pHandle->stBlob.phys_addr != 0)
    {
        sample_svp_mmz_free(pHandle->stBlob.phys_addr, pHandle->stBlob.virt_addr);
        memset(&pHandle->stBlob, 0, sizeof(ot_svp_dst_mem_info));
    }

    /* 销毁MD通道 */
    CHECK_API_RETURN(ss_ivs_md_destroy_chn(nChn));

    /* 避免创建两通道时，重复退出 */
    gs_nInited--;
    if(gs_nInited == 0)
    {
        /* 移动侦测退出 */
        ss_ivs_md_exit();
    }

    return TD_SUCCESS;
}

/**
* @brief       : 送帧给MD进行检测处理
* @author      : zhouzirui
* @param        {HiMd_S} *pHandle：句柄
* @param        {ot_video_frame_info} *pFrameInfo：输入帧信息
* @param        {ot_svp_dst_mem_info} **ppResult：移动侦测结果输出
* @return       {*}成功返回0,失败返回-1
*/
static int svpMd_sendFrame(HiMd_S *pHandle, ot_video_frame_info *pFrameInfo, ot_svp_dst_mem_info **ppResult)
{
    if (NULL == pHandle || NULL == pFrameInfo || NULL == ppResult || !pHandle->bInited)
    {
        mpi_md_log("Invalid parameter or not initialized\n");
        return TD_FAILURE;
    }

    td_bool is_instant = TD_TRUE;

    /* 检查帧尺寸 */
    if (pFrameInfo->video_frame.width != pHandle->stNeedParam.u32Width ||
        pFrameInfo->video_frame.height != pHandle->stNeedParam.u32Height)
    {
        mpi_md_log("Frame size mismatch: expected %dx%d, got %dx%d\n",
                pHandle->stNeedParam.u32Width, pHandle->stNeedParam.u32Height,
                pFrameInfo->video_frame.width, pFrameInfo->video_frame.height);
        return TD_FAILURE;
    }

    /* 使用IVE DMA将输入帧拷贝到当前帧 */
    CHECK_API_RETURN(sample_common_ive_dma_image(pFrameInfo, &pHandle->stCurFrame, is_instant));

    /* 如果是第一帧，仅保存为参考帧，不进行检测 */
    if (pHandle->bFirstFrame)
    {
        /* 将当前帧复制到参考帧 */
        CHECK_API_RETURN(sample_common_ive_dma_image(pFrameInfo, &pHandle->stRefFrame, is_instant));

        pHandle->bFirstFrame = TD_FALSE;
        
        /* 清空结果 */
        memset(sample_svp_convert_addr_to_ptr(td_void, pHandle->stBlob.virt_addr), 0, pHandle->stBlob.size);
        *ppResult = &pHandle->stBlob;
        
        mpi_md_log("First frame processed, reference frame initialized\n");
        return TD_SUCCESS;
    }

    /* 准备SAD输出指针 */
    ot_svp_dst_img *pSad = (pHandle->stExParam.enSadOutCtrl != OT_IVE_SAD_OUT_CTRL_THRESHOLD) ? 
                        &pHandle->stSadImg : NULL;

    /* 调用移动侦测处理函数 */
    CHECK_API_RETURN(ss_ivs_md_proc(pHandle->stNeedParam.nChn, &pHandle->stCurFrame, 
                        &pHandle->stRefFrame, pSad, &pHandle->stBlob));

    /**
     * 根据 bUpdateRef 参数决定参考帧更新策略：
     * - bUpdateRef = TD_FALSE: 自动更新模式（移动侦测功能），每帧自动更新参考帧
     * - bUpdateRef = TD_TRUE:  手动更新模式（场景变更功能），仅通过 svpMd_updateRef 接口手动更新
     */
    if (pHandle->stExParam.bUpdateRef == TD_FALSE)
    {
        /* 更新参考帧：将当前帧复制到参考帧 */
        CHECK_API_RETURN(sample_common_ive_dma_image(pFrameInfo, &pHandle->stRefFrame, is_instant));
    }

    *ppResult = &pHandle->stBlob;
    return TD_SUCCESS;
}

/**
* @brief       : 获取移动侦测结果
* @author      : zhouzirui
* @param        {HiMd_S} *pHandle：句柄
* @param        {ot_sample_svp_rect_info} *pRectInfo：矩形信息输出
* @return       {*}成功返回0,失败返回-1
*/
static int svpMd_getResult(HiMd_S *pHandle, ot_sample_svp_rect_info *pRectInfo)
{
    if (NULL == pHandle || NULL == pRectInfo || !pHandle->bInited)
    {
        return TD_FAILURE;
    }

    if (pHandle->stBlob.virt_addr == 0)
    {
        return TD_SUCCESS;
    }

    ot_ive_ccblob *pBlob = sample_svp_convert_addr_to_ptr(ot_ive_ccblob, pHandle->stBlob.virt_addr);
    
    /* 构造源目标尺寸信息 */
    ot_sample_src_dst_size src_dst_size = {
        .src = { pHandle->stNeedParam.u32Width, pHandle->stNeedParam.u32Height },
        .dst = { pHandle->stNeedParam.u32Width, pHandle->stNeedParam.u32Height }
    };

    CHECK_API_RETURN(sample_common_ive_blob_to_rect(pBlob, pRectInfo, OT_SVP_RECT_NUM, OT_SAMPLE_IVE_MD_AREA_THR_STEP, &src_dst_size));

    return TD_SUCCESS;
}
  
/**
* @brief       : 清除移动侦测结果
* @author      : zhouzirui
* @param        {HiMd_S} *pHandle：句柄
* @return       {*}
*/
static void svpMd_clearResult(HiMd_S *pHandle)
{
    if (pHandle != NULL && pHandle->stBlob.virt_addr != 0)
    {
        memset(sample_svp_convert_addr_to_ptr(td_void, pHandle->stBlob.virt_addr), 0, pHandle->stBlob.size);
    }
}

/**
 * @brief       : 手动更新参考帧
 * @author      : zhouzirui
 * @param       {HiMd_S} *pHandle：句柄
 * @param       {ot_video_frame_info} *pFrameInfo：新的参考帧信息
 * @return      {int} 成功返回0,失败返回-1
 * @note        : 用于场景变更功能，手动设置新的参考帧
 */
static int svpMd_updateRef(HiMd_S *pHandle, ot_video_frame_info *pFrameInfo)
{
    if (NULL == pHandle || NULL == pFrameInfo || !pHandle->bInited)
    {
        mpi_md_log("Invalid parameter or not initialized for update reference frame\n");
        return TD_FAILURE;
    }

    /* 检查帧尺寸 */
    if (pFrameInfo->video_frame.width != pHandle->stNeedParam.u32Width || pFrameInfo->video_frame.height != pHandle->stNeedParam.u32Height)
    {
        mpi_md_log("Reference frame size mismatch: expected %dx%d, got %dx%d\n", pHandle->stNeedParam.u32Width, pHandle->stNeedParam.u32Height, pFrameInfo->video_frame.width, pFrameInfo->video_frame.height);
        return TD_FAILURE;
    }

    td_bool is_instant = TD_TRUE;

    /* 使用IVE DMA将输入帧拷贝到参考帧 */
    CHECK_API_RETURN(sample_common_ive_dma_image(pFrameInfo, &pHandle->stRefFrame, is_instant));

    /* 清除移动侦测结果，因为参考帧已更新 */
    svpMd_clearResult(pHandle);

    mpi_md_log("Reference frame updated manually, chn: %d\n", pHandle->stNeedParam.nChn);
    return TD_SUCCESS;
}

HiMd_S *svpMd_alloc(HiMdNeedParam_S stNeedParam)
{
    HiMd_S *pHandle = (HiMd_S *)malloc(sizeof(HiMd_S));
    memset(pHandle, 0, sizeof(HiMd_S));

    //info /**********************必需参数***************************/
    pHandle->stNeedParam = stNeedParam;
    
    //info /**********************功能参数***************************/
    pHandle->stExParam.enAlgMode                        = OT_MD_ALG_MODE_BG;
    pHandle->stExParam.enSadMode                        = OT_IVE_SAD_MODE_MB_4X4;
    pHandle->stExParam.enSadOutCtrl                     = OT_IVE_SAD_OUT_CTRL_THRESHOLD;
    pHandle->stExParam.u16SadThreshold                  = OT_SAMPLE_IVE_SAD_THRESHOLD * (1 << 1);

    /* CCL控制参数 */
    /* 连通区域模式 */
    pHandle->stExParam.stCclCtrl.mode                   = OT_IVE_CCL_MODE_4C;
    td_u32 sad_mode;
    td_u8  wnd_size;
    sad_mode = (td_u32) pHandle->stExParam.enSadMode;
    wnd_size = (1 << (OT_SAMPLE_IVE_MD_NUM_TWO + sad_mode));
    /* 初始面积阈值。取值范围：[0, 65535] 参考取值：4。 */
    pHandle->stExParam.stCclCtrl.init_area_threshold    = wnd_size * wnd_size;
    /* 面积阈值增长步长。取值范围：[1,65535] 参考取值：2。 */
    pHandle->stExParam.stCclCtrl.step                   = wnd_size;

    /* 加权控制参数 */
    /* 加权加“xA+yB”中的权重“x”  0.5对应0x8000*/
    pHandle->stExParam.stAddCtrl.x                      = OT_SAMPLE_IVE_MD_ADD_X_VAL;
    /* 加权加“xA+yB”中的权重“y”  0.5对应0x8000*/
    pHandle->stExParam.stAddCtrl.y                      = OT_SAMPLE_IVE_MD_ADD_Y_VAL;

    /* 动态分析结果比例 */
    pHandle->stExParam.nWRatio                          = 16;
    pHandle->stExParam.nHRatio                          = 9;
    pHandle->stExParam.bUpdateRef                       = TD_FALSE;  // 默认自动更新模式

    /* 初始化状态 */
    pHandle->bFirstFrame = TD_TRUE;
    pHandle->bInited = TD_FALSE;
    //info /**********************函数列表***************************/
    pHandle->svpMd_init                     = svpMd_init;
    pHandle->svpMd_uninit                   = svpMd_uninit;
    pHandle->svpMd_sendFrame                = svpMd_sendFrame;
    pHandle->svpMd_getResult                = svpMd_getResult;
    pHandle->svpMd_clearResult              = svpMd_clearResult;
    pHandle->svpMd_updateRef                = svpMd_updateRef;

    return pHandle;
}

void svpMd_release(HiMd_S *pHandle)
{
    if (pHandle)
    {
        if (pHandle->bInited)
        {
            pHandle->svpMd_uninit(pHandle);
        }
        free(pHandle);
        pHandle = NULL;
    }
}