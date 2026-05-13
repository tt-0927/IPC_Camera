/**
 * @FilePath     : mpp_vgs.c
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-07-28 09:43:31
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-09-26 10:43:50
 * @Description  : 海思 vgs 模块封装
 */

#include "mpp_vgs.h"

/**
 * @brief   : 
 * @param    {ot_vb_calc_cfg} *vb_cal_config：视频图像帧各部分数据大小配置信息结构体指针
 * @param    {ot_pic_buf_attr} *buf_attr：图片属性指针
 * @param    {ot_vb_blk} vb_blk：缓存块句柄
 * @param    {td_void} *pVirtAddr：图像数据虚拟地址
 * @param    {td_phys_addr_t} phys_addr：图像数据物理地址
 * @param    {ot_video_frame_info} *frame_info：视频图像帧信息结构体指针
 */
static void mpp_vgs_set_frame_info(const ot_vb_calc_cfg *vb_cal_config, const ot_pic_buf_attr *buf_attr, const ot_vb_blk vb_blk, td_void *pVirtAddr, td_phys_addr_t phys_addr, ot_video_frame_info *frame_info)
{
    frame_info->mod_id = OT_ID_VGS;
    // frame_info->pool_id = ss_mpi_vb_handle_to_pool_id(vb_blk);

    frame_info->video_frame.width = buf_attr->width;
    frame_info->video_frame.height = buf_attr->height;
    frame_info->video_frame.field = OT_VIDEO_FIELD_FRAME;
    frame_info->video_frame.pixel_format = buf_attr->pixel_format;
    frame_info->video_frame.video_format = buf_attr->video_format;
    frame_info->video_frame.compress_mode = buf_attr->compress_mode;
    frame_info->video_frame.dynamic_range = OT_DYNAMIC_RANGE_SDR8;
    frame_info->video_frame.color_gamut = OT_COLOR_GAMUT_BT601;

    frame_info->video_frame.header_stride[0] = vb_cal_config->head_stride;
    frame_info->video_frame.header_stride[1] = vb_cal_config->head_stride;
    frame_info->video_frame.header_phys_addr[0] = phys_addr;
    frame_info->video_frame.header_phys_addr[1] = frame_info->video_frame.header_phys_addr[0] + vb_cal_config->head_y_size;
    frame_info->video_frame.header_virt_addr[0] = pVirtAddr;
    frame_info->video_frame.header_virt_addr[1] = (char *) frame_info->video_frame.header_virt_addr[0] + vb_cal_config->head_y_size;

    frame_info->video_frame.stride[0] = vb_cal_config->main_stride;
    frame_info->video_frame.stride[1] = vb_cal_config->main_stride;
    frame_info->video_frame.phys_addr[0] = frame_info->video_frame.header_phys_addr[0] + vb_cal_config->head_size;
    frame_info->video_frame.phys_addr[1] = frame_info->video_frame.phys_addr[0] + vb_cal_config->main_y_size;
    frame_info->video_frame.virt_addr[0] = (char *) frame_info->video_frame.header_virt_addr[0] + vb_cal_config->head_size;
    frame_info->video_frame.virt_addr[1] = (char *) frame_info->video_frame.virt_addr[0] + vb_cal_config->main_y_size;

    return;
}

int mppVgs_create_video_frame_info(td_u32 u32Width, td_u32 u32Height, ot_pixel_format enPixelFormat, ot_video_frame_info *pFrameInfo)
{
    ot_vb_calc_cfg calc_cfg;
    ot_vb_blk vb_blk;
    td_phys_addr_t phys_addr;
    td_void *pVirtAddr = TD_NULL;

    if (pFrameInfo == NULL)
    {
        mpi_vgs_log("无效的输入参数");
        return TD_FAILURE;
    }

    /* 计算VB配置 */
    ot_vb_pool_cfg vb_pool_cfg;
    ot_pic_buf_attr stBufAttr;
    stBufAttr.width = u32Width;
    stBufAttr.height = u32Height;
    stBufAttr.align = OT_DEFAULT_ALIGN;
    stBufAttr.bit_width = OT_DATA_BIT_WIDTH_8;
    stBufAttr.pixel_format = enPixelFormat;
    stBufAttr.compress_mode = OT_COMPRESS_MODE_NONE;
    stBufAttr.video_format = OT_VIDEO_FORMAT_LINEAR;

    ot_common_get_pic_buf_cfg(&stBufAttr, &calc_cfg);
    // mpi_vgs_log("%d %d %d %d %d %d %d", calc_cfg.head_stride, calc_cfg.head_y_size, calc_cfg.head_size, calc_cfg.main_stride, calc_cfg.main_y_size, calc_cfg.main_size, calc_cfg.vb_size);

    vb_pool_cfg.blk_size = calc_cfg.vb_size;
    vb_pool_cfg.blk_cnt = 1;
    memcpy(vb_pool_cfg.mmz_name, MMZ_NAME, sizeof(MMZ_NAME));
    vb_pool_cfg.remap_mode = OT_VB_REMAP_MODE_NONE;

    pFrameInfo->pool_id = ss_mpi_vb_create_pool(&vb_pool_cfg);

    /* 获取VB块 */
    vb_blk = ss_mpi_vb_get_blk(pFrameInfo->pool_id, calc_cfg.vb_size, TD_NULL);
    if (vb_blk == OT_VB_INVALID_HANDLE)
    {
        mpi_vgs_log("获取VB块失败");
        return TD_FAILURE;
    }

    /* 获取物理地址 */
    phys_addr = ss_mpi_vb_handle_to_phys_addr(vb_blk);
    if (phys_addr == 0)
    {
        mpi_vgs_log("获取物理地址失败");
        ss_mpi_vb_release_blk(vb_blk);
        return TD_FAILURE;
    }

    /* 映射虚拟地址 */
    pVirtAddr = ss_mpi_sys_mmap(phys_addr, calc_cfg.vb_size);
    if (pVirtAddr == TD_NULL)
    {
        mpi_vgs_log("映射虚拟地址失败");
        ss_mpi_vb_release_blk(vb_blk);
        return TD_FAILURE;
    }

    mpp_vgs_set_frame_info(&calc_cfg, &stBufAttr, vb_blk, pVirtAddr, phys_addr, pFrameInfo);

    return TD_SUCCESS;
}

int mppVgs_destroy_video_frame_info(ot_video_frame_info *pFrameInfo)
{
    ot_vb_blk vb_blk;
    ot_vb_calc_cfg calc_cfg;

    if (pFrameInfo == NULL)
    {
        return TD_SUCCESS;
    }

    /* 获取VB相关信息用于释放 */
    ot_pic_buf_attr buf_attr;
    buf_attr.width = pFrameInfo->video_frame.width;
    buf_attr.height = pFrameInfo->video_frame.height;
    buf_attr.pixel_format = pFrameInfo->video_frame.pixel_format;
    buf_attr.bit_width = OT_DATA_BIT_WIDTH_8;
    buf_attr.compress_mode = pFrameInfo->video_frame.compress_mode;
    buf_attr.align = OT_DEFAULT_ALIGN;
    buf_attr.video_format = pFrameInfo->video_frame.video_format;
    ot_common_get_pic_buf_cfg(&buf_attr, &calc_cfg);

    /* 取消虚拟地址映射 */
    if (pFrameInfo->video_frame.header_virt_addr[0] != 0)
    {
        CHECK_API_RETURN_PRINT(ss_mpi_sys_munmap(pFrameInfo->video_frame.header_virt_addr[0], calc_cfg.vb_size), "取消虚拟地址映射失败");
    }

    /* 释放VB块 */
    vb_blk = ss_mpi_vb_phys_addr_to_handle(pFrameInfo->video_frame.header_phys_addr[0]);
    if (vb_blk != OT_VB_INVALID_HANDLE)
    {
        CHECK_API_RETURN_PRINT(ss_mpi_vb_release_blk(vb_blk), "释放VB块失败");
    }

    /*销毁一个视频缓存池*/
    CHECK_API_RETURN(ss_mpi_vb_destroy_pool(pFrameInfo->pool_id));

    return TD_SUCCESS;
}

int mppVgs_crop(ot_video_frame_info *pSrcFrame, ot_video_frame_info *pDstFrame, ot_rect *pstCropRect)
{
    ot_vgs_handle handle = -1;
    ot_vgs_task_attr task_attr;
    ot_vgs_online stOnline;

    if (pSrcFrame == NULL || pDstFrame == NULL || pstCropRect == NULL)
    {
        mpi_vgs_log("无效的输入参数");
        return TD_FAILURE;
    }

    /* 检查裁剪区域参数合法性 宽高和坐标必须2对齐，裁剪区域宽高不能小于6 */
    if ((pstCropRect->x % 2 != 0) || (pstCropRect->y % 2 != 0) || (pstCropRect->width % 2 != 0) || (pstCropRect->height % 2 != 0) || (pstCropRect->width < 6) || (pstCropRect->height < 6))
    {
        mpi_vgs_log("裁剪参数必须是 2 的倍数且大小 >= 6");
        return TD_FAILURE;
    }

    /* 启动一个job */
    CHECK_API_RETURN(ss_mpi_vgs_begin_job(&handle));

    /* 配置VGS任务属性 */
    memset(&task_attr, 0, sizeof(ot_vgs_task_attr));

    /* 设置输入图像信息 */
    memcpy(&task_attr.img_in, pSrcFrame, sizeof(ot_video_frame_info));

    /* 设置输出图像信息 */
    memcpy(&task_attr.img_out, pDstFrame, sizeof(ot_video_frame_info));

    /* 配置在线处理参数 */
    memset(&stOnline, 0, sizeof(ot_vgs_online));

    /* 配置裁剪参数 */
    stOnline.crop_en = TD_TRUE;
    stOnline.crop_rect.x = pstCropRect->x;
    stOnline.crop_rect.y = pstCropRect->y;
    stOnline.crop_rect.width = pstCropRect->width;
    stOnline.crop_rect.height = pstCropRect->height;

    /* 添加VGS任务 往一个已经启动的job里添加在线任务 */
    td_s32 nRet = ss_mpi_vgs_add_online_task(handle, &task_attr, &stOnline);
    if (nRet != TD_SUCCESS)
    {
        mpi_vgs_log("mpi_vgs_add_online_task failed, ret:0x%x", nRet);
        CHECK_API_RETURN(ss_mpi_vgs_cancel_job(handle));
        return TD_FAILURE;
    }

    /* 提交并执行VGS任务 */
    if (ss_mpi_vgs_end_job(handle) != TD_SUCCESS)
    {
        CHECK_API_RETURN(ss_mpi_vgs_cancel_job(handle));
    }

    return TD_SUCCESS;
}

int mppVgs_scale(ot_video_frame_info *pSrcFrame, ot_video_frame_info *pDstFrame)
{
    ot_vgs_handle handle = -1;
    ot_vgs_task_attr task_attr;

    if (pSrcFrame == NULL || pDstFrame == NULL)
    {
        mpi_vgs_log("无效的输入参数");
        return TD_FAILURE;
    }

    /* 启动一个job */
    CHECK_API_RETURN(ss_mpi_vgs_begin_job(&handle));

    /* 配置VGS任务属性 */
    memset(&task_attr, 0, sizeof(ot_vgs_task_attr));

    /* 设置输入图像信息 */
    memcpy(&task_attr.img_in, pSrcFrame, sizeof(ot_video_frame_info));

    /* 设置输出图像信息 */
    memcpy(&task_attr.img_out, pDstFrame, sizeof(ot_video_frame_info));

    /* 添加VGS任务 往一个已经启动的job里添加缩放task */
    CHECK_API_RETURN(ss_mpi_vgs_add_scale_task(handle, &task_attr, OT_VGS_SCALE_COEF_NORM));

    /* 提交并执行VGS任务 */
    if (ss_mpi_vgs_end_job(handle) != TD_SUCCESS)
    {
        CHECK_API_RETURN(ss_mpi_vgs_cancel_job(handle));
    }

    return TD_SUCCESS;
}
