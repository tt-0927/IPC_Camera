/**
 * @FilePath     : mpp_venc_jpeg.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-20
 * @Description  : VENC JPEG 用户态按需编码封装（全景图 + 批量抠图小图）。
 *
 * 基于 ss_mpi_venc_send_frame_batch_crop（MPP V6.0 开发参考 6.3.17）实现：
 *   - valid=false：只编码整帧原图，用于全景图抓拍；
 *   - valid=true ：只编码 crop_rect 数组指定的抠图区域，一次最多 60 张，
 *     收流按"一帧码流 X 个 pack、每 pack is_frame_end=true、各自独立成图、
 *     输出顺序与数组下标一致"的硬件语义拆分。
 *
 * 约束（Hi3516CV610，调用方送入前须满足，封装内亦做防御性校验）：
 *   - 原图大小必须等于编码通道大小（batch_crop 硬性要求）；
 *   - 帧为非压缩 NV21/NV12 等半平面格式；
 *   - rect.x 8 像素对齐、rect.y 2 像素对齐；
 *   - rect 宽高不小于 32x32，且 (x+w, y+h) 不超出原图范围。
 *
 * 线程模型：同一通道的编码接口不保证并发安全，调用方（上层编码服务）
 * 需保证对同一通道串行调用；不同通道之间可并发。
 */

#ifndef _MPP_VENC_JPEG_H_
#define _MPP_VENC_JPEG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ot_common_video.h"
#include "ot_common_venc.h"
#include "ss_mpi_venc.h"

/* 单次批量抠图上限（OT_VENC_MAX_BATCH_CROP_RECT_NUM，CV610 硬件限制 60） */
#define MPP_VENC_JPEG_MAX_CROP_NUM (60)

/* JPEG 品质因数默认值 [1,99] */
#define MPP_VENC_JPEG_QFACTOR_DEFAULT (90)

/* 用户态 JPEG 编码通道属性 */
typedef struct _mpp_venc_jpeg_attr
{
    ot_venc_chn chn; /* 编码通道号，由业务统一规划，须避开已占用通道 */
    td_u32 width;    /* 编码通道宽，必须等于后续送入帧宽 */
    td_u32 height;   /* 编码通道高，必须等于后续送入帧高 */
    td_u32 qfactor;  /* 品质因数 [1,99]，填 0 取默认值 90 */
} mpp_venc_jpeg_attr;

/* 单张输出 JPEG 的缓冲描述（调用方分配，封装内只写入） */
typedef struct _mpp_venc_jpeg_out
{
    td_u8 *buf; /* 输出缓冲，调用方分配 */
    td_u32 cap; /* 缓冲容量（字节） */
    td_u32 len; /* 输出：实际 JPEG 字节数 */
} mpp_venc_jpeg_out;

/**
 * @brief   创建用户态 JPEG 编码通道（不绑定 VPSS，业务通过 encode 接口送帧）。
 * @param   attr        通道属性
 * @return  0 成功；非 0 失败
 */
int mppVencJpeg_create(const mpp_venc_jpeg_attr *attr);

/**
 * @brief   销毁用户态 JPEG 编码通道。
 * @param   chn         通道号
 * @return  0 成功；非 0 失败
 */
int mppVencJpeg_destroy(ot_venc_chn chn);

/**
 * @brief   动态调整品质因数（下一次编码生效）。
 * @param   chn         通道号
 * @param   qfactor     [1,99]
 * @return  0 成功；非 0 失败
 */
int mppVencJpeg_set_qfactor(ot_venc_chn chn, td_u32 qfactor);

/**
 * @brief   整帧编码为单张 JPEG（batch_crop valid=false 路径，用于全景图）。
 * @param   chn         通道号
 * @param   frame       用户态帧（非压缩半平面 YUV，宽高须等于通道宽高）
 * @param   buf         输出缓冲
 * @param   cap         输出缓冲容量
 * @param   out_len     输出实际字节数
 * @param   timeout_ms  送帧/收流超时（毫秒，-1 阻塞）
 * @return  0 成功；非 0 失败
 */
int mppVencJpeg_encode_frame(ot_venc_chn chn, const ot_video_frame_info *frame, td_u8 *buf, td_u32 cap, td_u32 *out_len, int timeout_ms);

/**
 * @brief   批量抠图编码（batch_crop valid=true 路径，用于目标小图）。
 *          一次送入 rect_num 个抠图框，硬件一次完成全部编码；
 *          outs[i] 与 rects[i] 按下标一一对应。
 * @param   chn         通道号
 * @param   frame       用户态帧（非压缩半平面 YUV，宽高须等于通道宽高）
 * @param   rects       抠图框数组（x 8 对齐 / y 2 对齐 / 宽高 >= 32x32 / 不越界）
 * @param   rect_num    抠图框数量 [1, 60]
 * @param   outs        输出缓冲数组（长度 = rect_num，调用方分配）
 * @param   timeout_ms  送帧/收流超时（毫秒，-1 阻塞）
 * @return  0 成功；非 0 失败（容量不足时对应 out.len 不写，整体返回失败）
 */
int mppVencJpeg_encode_crops(ot_venc_chn chn,
                             const ot_video_frame_info *frame,
                             const ot_rect *rects,
                             td_u32 rect_num,
                             mpp_venc_jpeg_out *outs,
                             int timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* _MPP_VENC_JPEG_H_ */
