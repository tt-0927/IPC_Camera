/**
 * @FilePath     : svp_ive.c
 * @Author       : cyc
 * @Date         : 2025-07-22 15:17:17
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-07-24 09:34:09
 * @Description  : 海思ive模块封装
 */

#include "svp_ive.h"
#include "ss_mpi_sys_mem.h"
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/ioctl.h>

/* 把任何指针直接变成 64 位无符号整数 */
#define ive_convert_addr_to_ptr(type, addr) ((type *)(td_uintptr_t)(addr)) 
/* 把 64 位无符号整数还原成指定类型的指针 */
#define ive_convert_ptr_to_addr(type, addr) ((type)(td_uintptr_t)(addr))
#define OT_IVE_IMAGE_CHN_TWO             2
#define OT_IVE_IMAGE_CHN_THREE           3
#define OT_ADDR_IDX_ZERO                 0
#define OT_ADDR_IDX_ONE                  1
#define OT_ADDR_IDX_TWO                  2
#define OT_IVE_ALIGN                     16
#define OT_IVE_DIV_TWO                   2
#define OT_MAX_LOOP_IMG_H                3

/**
 * @brief 计算 stride（行字节数），按指定对齐向上取整
 * @param[in] nWidth  每行像素（或元素）个数
 * @param[in] align   对齐字节数，必须是 2 的幂
 * @return    对齐后的行字节数
 */
 static uint32_t ive_calc_stride(uint32_t nWidth, uint8_t align)
{
    /* 向上取整对齐 */
    return (nWidth + (align - nWidth % align) % align);
}

/**
 * @brief 根据图像类型填充 loop_info，用于后续内存布局计算
 * @param[in]  pImg  输入图像描述
 * @param[out] pInfo 输出的遍历信息结构体
 */
static void ive_get_loop_info(const ot_svp_img* pImg, ot_ive_image_loop_info *pInfo)
{
    pInfo->ele_size = 1;
    pInfo->loop_c = 1;
    pInfo->loop_h[0] = pImg->height;
    switch (pImg->type) 
    {
        case OT_SVP_IMG_TYPE_U8C1:
        case OT_SVP_IMG_TYPE_S8C1:
            break;
        case OT_SVP_IMG_TYPE_YUV420SP:
            pInfo->ele_size = 1;
            pInfo->loop_c = OT_IVE_IMAGE_CHN_TWO;
            pInfo->loop_h[1] = pImg->height / OT_IVE_DIV_TWO;
            break;
        case OT_SVP_IMG_TYPE_YUV422SP:
            pInfo->loop_c = OT_IVE_IMAGE_CHN_TWO;
            pInfo->loop_h[1] = pImg->height;
            break;
        case OT_SVP_IMG_TYPE_U8C3_PACKAGE:
            pInfo->ele_size = (td_u32)(sizeof(td_u8) + sizeof(td_u16));
            break;
        case OT_SVP_IMG_TYPE_U8C3_PLANAR:
            pInfo->loop_c = OT_IVE_IMAGE_CHN_THREE;
            pInfo->loop_h[1] = pImg->height;
            pInfo->loop_h[OT_IVE_IMAGE_CHN_TWO] = pImg->height;
            break;
        case OT_SVP_IMG_TYPE_S16C1:
        case OT_SVP_IMG_TYPE_U16C1:
            pInfo->ele_size = (td_u32)sizeof(td_u16);
            break;
        case OT_SVP_IMG_TYPE_U32C1:
        case OT_SVP_IMG_TYPE_S32C1:
            pInfo->ele_size = (td_u32)sizeof(td_u32);
            break;
        case OT_SVP_IMG_TYPE_S64C1:
        case OT_SVP_IMG_TYPE_U64C1:
            pInfo->ele_size = (td_u32)sizeof(td_u64);
            break;
        default:
            break;
    }
}

/**
 * @brief 为图像各 plane 分配 MMZ 物理/虚拟地址
 * @param[in,out] pImg   图像结构体（已填 type/width/height/stride[0]）
 * @param[in]     pInfo  由 ive_get_loop_info 计算得到
 * @return 0 成功；-1 失败
 */
static int ive_set_image_addr(ot_svp_img *pImg, ot_ive_image_loop_info *pInfo)
{
    td_u32 size = 0;
    int nRet;
    int i = 0;
    td_void *virt_addr = TD_NULL;
    for (i = 0; (i < pInfo->loop_c) && (i < OT_MAX_LOOP_IMG_H) && (i < OT_SVP_IMG_STRIDE_NUM); i++) 
    {
        size += pImg->stride[0] * pInfo->loop_h[i] * pInfo->ele_size;
        pImg->stride[i] = pImg->stride[0];
    }
    
    /* 在用户态分配MMZ内存 */
    nRet = ss_mpi_sys_mmz_alloc((td_phys_addr_t *)&pImg->phys_addr[0], (td_void **)&virt_addr,
        TD_NULL, TD_NULL, size);
    if(nRet != 0)
    {
        printf("ss_mpi_sys_mmz_alloc error!nRet:%u\n",nRet);
        return -1;
    }

    pImg->virt_addr[OT_ADDR_IDX_ZERO] = ive_convert_ptr_to_addr(td_u64, virt_addr);

    if (pImg->type != OT_SVP_IMG_TYPE_U8C3_PACKAGE) 
    {
        for (i = 1; (i < pInfo->loop_c) && (i < OT_MAX_LOOP_IMG_H) && (i < OT_SVP_IMG_STRIDE_NUM); i++) 
        {
            pImg->phys_addr[i] = pImg->phys_addr[i - 1] + pImg->stride[i - 1] * pImg->height;
            pImg->virt_addr[i] = pImg->virt_addr[i - 1] + pImg->stride[i - 1] * pImg->height;
        }
    } 
    else 
    {
        pImg->virt_addr[OT_ADDR_IDX_ONE] = pImg->virt_addr[OT_ADDR_IDX_ZERO] + 1;
        pImg->virt_addr[OT_ADDR_IDX_TWO] = pImg->virt_addr[OT_ADDR_IDX_ONE] + 1;
        pImg->phys_addr[OT_ADDR_IDX_ONE] = pImg->phys_addr[OT_ADDR_IDX_ZERO] + 1;
        pImg->phys_addr[OT_ADDR_IDX_TWO] = pImg->phys_addr[OT_ADDR_IDX_ONE] + 1;
    }
    return TD_SUCCESS;
}

 /**
  * @description : 创建IVE图像内存
  * @author      : cyc
  * @param        {HiIve_S} *pHandle
  * @param        {ot_svp_img} *pImg
  * @return       成功返回0,失败返回-1
  */ 
 static int create_ive_image(HiIve_S *pHandle,ot_svp_img *pImg)
 {
    int nRet = -1;
    ot_ive_image_loop_info stLoopInfo;
    memset(&stLoopInfo,0,sizeof(ot_ive_image_loop_info));

    pImg->type = pHandle->stNeedParam.enType;
    pImg->width = pHandle->stNeedParam.nWidth;
    pImg->height = pHandle->stNeedParam.nHeight;
    pImg->stride[0] = ive_calc_stride(pImg->width,OT_IVE_ALIGN);
    switch(pImg->type)
    {
        case OT_SVP_IMG_TYPE_U8C1:
        case OT_SVP_IMG_TYPE_S8C1:
        case OT_SVP_IMG_TYPE_YUV420SP:
        case OT_SVP_IMG_TYPE_YUV422SP:
        case OT_SVP_IMG_TYPE_S16C1:
        case OT_SVP_IMG_TYPE_U16C1:
        case OT_SVP_IMG_TYPE_U8C3_PACKAGE:
        case OT_SVP_IMG_TYPE_S32C1:
        case OT_SVP_IMG_TYPE_U32C1:
        case OT_SVP_IMG_TYPE_S64C1:
        case OT_SVP_IMG_TYPE_U64C1:
        {
            ive_get_loop_info(pImg,&stLoopInfo);
            ive_set_image_addr(pImg,&stLoopInfo);
            break;
        }
        case OT_SVP_IMG_TYPE_YUV420P:
            break;
        case OT_SVP_IMG_TYPE_YUV422P:
            break;
        case OT_SVP_IMG_TYPE_S8C2_PACKAGE:
            break;
        case OT_SVP_IMG_TYPE_S8C2_PLANAR:
            break;
        case OT_SVP_IMG_TYPE_U8C3_PLANAR:
            break;
        default:
            break;
    }
    return nRet;
 }

 
 
 /**
  * @description : 销毁 IVE 图像内存
  * @author      : cyc
  * @param        {ot_svp_img} *pImg
  * @return       {*}
  */
 static int destroy_ive_image(ot_svp_img *pImg)
 {
    if (NULL == pImg)
    {
        return TD_FAILURE;
    }

    int nRet = -1;

    nRet = ss_mpi_sys_mmz_free(pImg->phys_addr[0],(void *)(td_uintptr_t)pImg->virt_addr[0]);
    if(nRet != 0)
    {
        printf("ss_mpi_sys_mmz_free error!nRet:%u\n",nRet);
        return -1;
    }
 
     return TD_SUCCESS;
 }
 
 
 HiIve_S *svpIve_alloc(HiIveNeedParam_S stNeedParam)
 {
    HiIve_S *pHandle = (HiIve_S *)malloc(sizeof(HiIve_S));
    memset(pHandle, 0, sizeof(HiIve_S));
 
    /**********************功能参数***************************/
     pHandle->stNeedParam.enType       = stNeedParam.enType;
     pHandle->stNeedParam.nWidth       = stNeedParam.nWidth;
     pHandle->stNeedParam.nHeight      = stNeedParam.nHeight;
 
    /**********************函数列表***************************/
     pHandle->create_ive_image         = create_ive_image;
     pHandle->destroy_ive_image        = destroy_ive_image;
 
     return pHandle;
 }
 
 void svpIve_release(HiIve_S *pHandle)
 {
     if (pHandle)
     {
         free(pHandle);
         pHandle = NULL;
     }
 }

