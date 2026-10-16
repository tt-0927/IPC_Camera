/**
 * @FilePath     : svp_ive_sharpness.c
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-20
 * @Description  : IVE 硬件清晰度评分封装实现。
 */

#include "svp_ive_sharpness.h"

#define _POSIX_C_SOURCE 199309L

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mpi_common.h"
#include "ss_mpi_sys_mem.h"

/* IVE 图像地址/跨度对齐（对齐 ot_common_ive.h 约束） */
#define IVE_SHARP_ALIGN       (16)
/* ROI 最小尺寸（mag_and_ang 硬件约束 64x64） */
#define IVE_SHARP_MIN_SIZE    (64)
/* 二值化结果像素值：显著梯度=1，其余=0（配合 integ SUM 计数） */
#define IVE_SHARP_BIN_ON      (1)
#define IVE_SHARP_BIN_OFF     (0)
/* query 等待上限（亚毫秒任务，200ms 防死等） */
#define IVE_SHARP_QUERY_RETRY (200)

struct _HiIveSharp_S
{
    HiIveSharpParam_S stParam;
    ot_svp_img stSrc;   /* U8C1，ROI 视图（外部帧 Y 平面，不持有内存） */
    ot_svp_img stMag;   /* U16C1 梯度幅值（当前评分窗档位） */
    ot_svp_img stBin;   /* U8C1 二值化（当前评分窗档位） */
    ot_svp_img stInteg; /* U32C1 积分图（当前评分窗档位） */
    td_u32 nCurW;       /* 当前中间图档位宽（0=未分配；IVE 要求算子输入输出同尺寸） */
    td_u32 nCurH;       /* 当前中间图档位高 */
};

/* 按 16 字节对齐向上取整 */
static td_u32 ive_sharp_align_up(td_u32 v)
{
    return (v + IVE_SHARP_ALIGN - 1) / IVE_SHARP_ALIGN * IVE_SHARP_ALIGN;
}

/* 分配单个独立图像内存（stride 16 对齐；U8C1/U16C1/U32C1 单平面） */
static int ive_sharp_alloc_img(ot_svp_img *pImg, ot_svp_img_type type, td_u32 w, td_u32 h)
{
    const td_u32 ele = (type == OT_SVP_IMG_TYPE_U16C1) ? 2 : ((type == OT_SVP_IMG_TYPE_U32C1) ? 4 : 1);
    pImg->type = type;
    pImg->width = w;
    pImg->height = h;
    pImg->stride[0] = ive_sharp_align_up(w * ele);
    const td_u32 size = pImg->stride[0] * h;
    td_void *virt = TD_NULL;
    td_phys_addr_t phys = 0;
    const td_s32 mmzRet = ss_mpi_sys_mmz_alloc(&phys, &virt, "ive_sharp", TD_NULL, size);
    if (mmzRet != TD_SUCCESS)
    {
        mpi_ive_log("alloc 失败: mmz type=%d %ux%u size=%u ret=0x%08x", (int) type, w, h, size, mmzRet);
        return -1;
    }
    pImg->phys_addr[0] = (td_u64) phys;
    pImg->virt_addr[0] = (td_u64) (uintptr_t) virt;
    return 0;
}

static void ive_sharp_free_img(ot_svp_img *pImg)
{
    if (pImg->phys_addr[0] != 0)
    {
        ss_mpi_sys_mmz_free((td_phys_addr_t) pImg->phys_addr[0], (const td_void *) (uintptr_t) pImg->virt_addr[0]);
        memset(pImg, 0, sizeof(*pImg));
    }
}

/* 中间图按评分窗档位重分配（IVE 算子要求输入输出同尺寸，档位复用避免逐帧分配） */
static int ive_sharp_realloc_imgs(HiIveSharp_S *pHandle, td_u32 w, td_u32 h)
{
    if (pHandle->nCurW == w && pHandle->nCurH == h)
    {
        return 0;
    }
    ive_sharp_free_img(&pHandle->stInteg);
    ive_sharp_free_img(&pHandle->stBin);
    ive_sharp_free_img(&pHandle->stMag);
    pHandle->nCurW = 0;
    pHandle->nCurH = 0;
    if (ive_sharp_alloc_img(&pHandle->stMag, OT_SVP_IMG_TYPE_U16C1, w, h) != 0 ||
        ive_sharp_alloc_img(&pHandle->stBin, OT_SVP_IMG_TYPE_U8C1, w, h) != 0 ||
        ive_sharp_alloc_img(&pHandle->stInteg, OT_SVP_IMG_TYPE_U32C1, w, h) != 0)
    {
        mpi_ive_log("eval 失败: 中间图 %ux%u 重分配失败", w, h);
        ive_sharp_free_img(&pHandle->stInteg);
        ive_sharp_free_img(&pHandle->stBin);
        ive_sharp_free_img(&pHandle->stMag);
        return -1;
    }
    pHandle->nCurW = w;
    pHandle->nCurH = h;
    return 0;
}

HiIveSharp_S *svpIveSharp_alloc(HiIveSharpParam_S stParam)
{
    if (stParam.width < IVE_SHARP_MIN_SIZE || stParam.height < IVE_SHARP_MIN_SIZE || stParam.width > 1920 || stParam.height > 1024)
    {
        mpi_ive_log("alloc 失败: 参数越界 %ux%u", stParam.width, stParam.height);
        return TD_NULL;
    }
    /* 中间图由 eval 按评分窗档位懒分配，此处仅持有参数 */
    HiIveSharp_S *pHandle = (HiIveSharp_S *) calloc(1, sizeof(HiIveSharp_S));
    /* cppcheck 对 TD_NULL 宏不敏感，判空用 NULL 以正确识别后续解引用安全 */
    if (pHandle == NULL)
    {
        mpi_ive_log("alloc 失败: calloc 句柄内存失败");
        return TD_NULL;
    }
    pHandle->stParam = stParam;
    return pHandle;
}

void svpIveSharp_release(HiIveSharp_S *pHandle)
{
    if (pHandle == TD_NULL)
    {
        return;
    }
    ive_sharp_free_img(&pHandle->stInteg);
    ive_sharp_free_img(&pHandle->stBin);
    ive_sharp_free_img(&pHandle->stMag);
    free(pHandle);
}

/* 阻塞等待已提交任务完成（ROI 小图亚毫秒级，轮询带上限防死等） */
static int ive_sharp_wait(ot_ive_handle handle)
{
    td_bool is_done = TD_FALSE;
    for (int i = 0; i < IVE_SHARP_QUERY_RETRY; i++)
    {
        if (ss_mpi_ive_query(handle, &is_done, 1) != TD_SUCCESS)
        {
            return -1;
        }
        if (is_done == TD_TRUE)
        {
            return 0;
        }
        usleep(1000);
    }
    return -1;
}

float svpIveSharp_eval(HiIveSharp_S *pHandle, td_phys_addr_t physY, td_u32 frameStride, const ot_rect *pRoi)
{
    if (pHandle == TD_NULL || pRoi == TD_NULL || physY == 0 || frameStride == 0)
    {
        mpi_ive_log("eval 失败: 空参数或无效地址");
        return -1.0f;
    }
    if (frameStride % IVE_SHARP_ALIGN != 0)
    {
        mpi_ive_log("eval 失败: 帧跨度 %u 非 %d 对齐", frameStride, IVE_SHARP_ALIGN);
        return -1.0f;
    }

    /*
     * 评分窗：ROI 超上限时中心截取（清晰度为区域统计量，中心子区域代表性足够）；
     * 尺寸向下对齐 64 像素档位（IVE 硬件下限 64，档位复用中间图避免逐帧重分配）。
     * IVE 算子要求输入输出图像同尺寸，src 视图与中间图统一使用评分窗尺寸。
     */
    td_u32 w = (td_u32) pRoi->width;
    td_u32 h = (td_u32) pRoi->height;
    if (pRoi->x < 0 || pRoi->y < 0 || w < IVE_SHARP_MIN_SIZE || h < IVE_SHARP_MIN_SIZE)
    {
        mpi_ive_log("eval 失败: ROI 越界 %ux%u@(%d,%d)", w, h, pRoi->x, pRoi->y);
        return -1.0f;
    }
    td_s32 x = pRoi->x;
    td_s32 y = pRoi->y;
    if (w > pHandle->stParam.width)
    {
        x += (td_s32) ((w - pHandle->stParam.width) / 2);
        w = pHandle->stParam.width;
    }
    if (h > pHandle->stParam.height)
    {
        y += (td_s32) ((h - pHandle->stParam.height) / 2);
        h = pHandle->stParam.height;
    }
    w = w / IVE_SHARP_MIN_SIZE * IVE_SHARP_MIN_SIZE;
    h = h / IVE_SHARP_MIN_SIZE * IVE_SHARP_MIN_SIZE;
    /* 地址 16 字节对齐：x 向下对齐只会左移，窗仍落在原 ROI 内 */
    x = x / (td_s32) IVE_SHARP_ALIGN * (td_s32) IVE_SHARP_ALIGN;

    if (ive_sharp_realloc_imgs(pHandle, w, h) != 0)
    {
        return -1.0f;
    }

    /* 源视图：外部帧 Y 平面内偏移（不拷贝，IVE 按物理地址访问） */
    ot_svp_img *src = &pHandle->stSrc;
    memset(src, 0, sizeof(*src));
    src->type = OT_SVP_IMG_TYPE_U8C1;
    src->width = w;
    src->height = h;
    src->phys_addr[0] = physY + (td_phys_addr_t) y * frameStride + (td_phys_addr_t) x;
    src->stride[0] = frameStride;

    ot_ive_handle handle;
    td_bool is_instant = TD_TRUE;

    /* 1. 梯度幅值（3x3 sobel 嵌入 5x5 模板，转置模板求垂直分量），内置弱梯度截断 */
    ot_ive_mag_and_ang_ctrl magCtrl;
    memset(&magCtrl, 0, sizeof(magCtrl));
    magCtrl.out_ctrl = OT_IVE_MAG_AND_ANG_OUT_CTRL_MAG;
    magCtrl.threshold = pHandle->stParam.grad_threshold;
    magCtrl.mask[1 * 5 + 1] = -1;
    magCtrl.mask[1 * 5 + 3] = 1;
    magCtrl.mask[2 * 5 + 1] = -2;
    magCtrl.mask[2 * 5 + 3] = 2;
    magCtrl.mask[3 * 5 + 1] = -1;
    magCtrl.mask[3 * 5 + 3] = 1;
    td_s32 ret = ss_mpi_ive_mag_and_ang(&handle,
                                        (const ot_svp_src_img *) src,
                                        (ot_svp_dst_img *) &pHandle->stMag,
                                        TD_NULL,
                                        &magCtrl,
                                        is_instant);
    if (ret != TD_SUCCESS)
    {
        mpi_ive_log("eval 失败: mag_and_ang ret=0x%08x 窗=%ux%u stride=%u", ret, w, h, frameStride);
        return -1.0f;
    }
    if (ive_sharp_wait(handle) != 0)
    {
        mpi_ive_log("eval 失败: mag_and_ang 等待超时");
        return -1.0f;
    }

    /* 2. 显著梯度二值化 0/1（U16 -> U8，mag 已截断弱梯度，此处仅非零映射） */
    ot_ive_threshold_u16_ctrl thCtrl;
    memset(&thCtrl, 0, sizeof(thCtrl));
    thCtrl.mode = OT_IVE_THRESHOLD_U16_MODE_U16_TO_U8_MIN_MID_MAX;
    thCtrl.low_threshold = 1;
    thCtrl.high_threshold = 0xFFFF;
    thCtrl.min_val = IVE_SHARP_BIN_OFF;
    thCtrl.mid_val = IVE_SHARP_BIN_ON;
    thCtrl.max_val = IVE_SHARP_BIN_ON;
    ret = ss_mpi_ive_threshold_u16(&handle,
                                   (const ot_svp_src_img *) &pHandle->stMag,
                                   (ot_svp_dst_img *) &pHandle->stBin,
                                   &thCtrl,
                                   is_instant);
    if (ret != TD_SUCCESS)
    {
        mpi_ive_log("eval 失败: threshold_u16 ret=0x%08x", ret);
        return -1.0f;
    }
    if (ive_sharp_wait(handle) != 0)
    {
        mpi_ive_log("eval 失败: threshold_u16 等待超时");
        return -1.0f;
    }

    /* 3. 积分图（SUM 模式，U32C1，值 = 左上角起累加和） */
    ot_ive_integ_ctrl integCtrl;
    memset(&integCtrl, 0, sizeof(integCtrl));
    integCtrl.out_ctrl = OT_IVE_INTEG_OUT_CTRL_SUM;
    ret = ss_mpi_ive_integ(&handle, (const ot_svp_src_img *) &pHandle->stBin, (ot_svp_dst_img *) &pHandle->stInteg, &integCtrl, is_instant);
    if (ret != TD_SUCCESS)
    {
        mpi_ive_log("eval 失败: integ ret=0x%08x", ret);
        return -1.0f;
    }
    if (ive_sharp_wait(handle) != 0)
    {
        mpi_ive_log("eval 失败: integ 等待超时");
        return -1.0f;
    }

    /*
     * 4. 右下角采样即全 ROI 累计（积分图语义），归一化为占比。
     * MMZ 为 cacheable 映射：硬件 DMA 写完后 CPU 读取前必须 flush，
     * 否则命中 CPU cache 中的陈旧/未初始化数据（板端曾读出 1e4 量级脏值）。
     */
    const td_u32 integSize = pHandle->stInteg.stride[0] * h;
    ret = ss_mpi_sys_flush_cache((td_phys_addr_t) pHandle->stInteg.phys_addr[0],
                                 (td_void *) (uintptr_t) pHandle->stInteg.virt_addr[0],
                                 integSize);
    if (ret != TD_SUCCESS)
    {
        mpi_ive_log("eval 失败: flush_cache ret=0x%08x size=%u", ret, integSize);
        return -1.0f;
    }
    const td_u32 *integ = (const td_u32 *) (uintptr_t) pHandle->stInteg.virt_addr[0];
    const td_u32 stride = pHandle->stInteg.stride[0] / sizeof(td_u32);
    const td_u64 total = integ[(h - 1) * stride + (w - 1)];
    return (float) ((double) total / ((double) w * (double) h));
}
