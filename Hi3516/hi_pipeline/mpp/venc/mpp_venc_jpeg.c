/**
 * @FilePath     : mpp_venc_jpeg.c
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-20
 * @Description  : VENC JPEG 用户态按需编码封装实现。
 *
 * PoC 板端验证结论（TV-3852TL / HI3516CV610_MPP_V1.0.2.0，2026-09-20）：
 *   - valid=false 整帧 1080p q90 约 10ms；valid=true 3 框抠图约 4~6ms；
 *   - 驱动对非法 rect（非对齐/小于 32/越界）返回 OT_ERR_VENC_ILLEGAL_PARAM
 *     并可继续正常编码，本封装在送帧前再做一层防御性校验并输出日志；
 *   - batch_crop_en 无显式设置接口，由首次调用自动置位（/proc/umap/jpege 可观测）。
 */

#include "mpp_venc_jpeg.h"

#include <stdlib.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

#include "mpi_common.h"
#include "securec.h"

/* 抠图框约束常量（CV610，见 MPP V6.0 开发参考 6.3.17） */
#define MPP_VENC_JPEG_CROP_X_ALIGN        (8)
#define MPP_VENC_JPEG_CROP_Y_ALIGN        (2)
#define MPP_VENC_JPEG_CROP_MIN_SIZE       (32)
/* 码流 buffer 按 16 对齐宽高 * 2 字节预估（q99 的 1080p JPEG 上限约 1MB，4MB 余量充足） */
#define MPP_VENC_JPEG_BUF_BYTES_PER_PIXEL (2)

static td_bool mpp_venc_jpeg_check_rects(const ot_rect *rects, td_u32 rect_num, td_u32 width, td_u32 height)
{
    for (td_u32 i = 0; i < rect_num; i++)
    {
        const ot_rect *r = &rects[i];
        if ((r->x % MPP_VENC_JPEG_CROP_X_ALIGN) != 0 || (r->y % MPP_VENC_JPEG_CROP_Y_ALIGN) != 0)
        {
            mpi_venc_log("crop[%u] 起点(%d,%d) 不满足 %d/%d 像素对齐",
                         i,
                         r->x,
                         r->y,
                         MPP_VENC_JPEG_CROP_X_ALIGN,
                         MPP_VENC_JPEG_CROP_Y_ALIGN);
            return TD_FALSE;
        }
        if (r->width < MPP_VENC_JPEG_CROP_MIN_SIZE || r->height < MPP_VENC_JPEG_CROP_MIN_SIZE)
        {
            mpi_venc_log("crop[%u] 宽高(%u,%u) 小于最小 %dx%d",
                         i,
                         r->width,
                         r->height,
                         MPP_VENC_JPEG_CROP_MIN_SIZE,
                         MPP_VENC_JPEG_CROP_MIN_SIZE);
            return TD_FALSE;
        }
        if (r->x < 0 || r->y < 0 || (td_u32) r->x + r->width > width || (td_u32) r->y + r->height > height)
        {
            mpi_venc_log("crop[%u] 区域(%d+%u, %d+%u) 越界(%ux%u)", i, r->x, r->width, r->y, r->height, width, height);
            return TD_FALSE;
        }
    }
    return TD_TRUE;
}

int mppVencJpeg_create(const mpp_venc_jpeg_attr *attr)
{
    if (attr == NULL || attr->width == 0 || attr->height == 0)
    {
        mpi_venc_log("非法参数");
        return OT_ERR_VENC_ILLEGAL_PARAM;
    }

    ot_venc_chn_attr chn_attr;
    ot_venc_start_param start_param;
    (td_void) memset_s(&chn_attr, sizeof(chn_attr), 0, sizeof(chn_attr));
    (td_void) memset_s(&start_param, sizeof(start_param), 0, sizeof(start_param));

    chn_attr.venc_attr.type = OT_PT_JPEG;
    chn_attr.venc_attr.max_pic_width = attr->width;
    chn_attr.venc_attr.max_pic_height = attr->height;
    /* qfactor 上限下按 2 字节/像素预留，保证单张完整 JPEG 不拆包 */
    chn_attr.venc_attr.buf_size = MPI_ALIGN_UP(attr->width, 16) * MPI_ALIGN_UP(attr->height, 16) * MPP_VENC_JPEG_BUF_BYTES_PER_PIXEL;
    chn_attr.venc_attr.is_by_frame = TD_TRUE;
    chn_attr.venc_attr.pic_width = attr->width;
    chn_attr.venc_attr.pic_height = attr->height;
    /* batch_crop 不支持 DCF/MPF 缩略图；单源接收模式 */
    chn_attr.venc_attr.jpeg_attr.dcf_en = TD_FALSE;
    chn_attr.venc_attr.jpeg_attr.mpf_cfg.large_thumbnail_num = 0;
    chn_attr.venc_attr.jpeg_attr.recv_mode = OT_VENC_PIC_RECV_SINGLE;
    /* JPEG 无码率控制（rc_mode 保持 0）；CV610 不支持 SNAP 模式，默认 ALL 即可 */
    chn_attr.gop_attr.gop_mode = OT_VENC_GOP_MODE_NORMAL_P;

    td_s32 ret = ss_mpi_venc_create_chn(attr->chn, &chn_attr);
    if (ret != TD_SUCCESS)
    {
        mpi_venc_log("创建通道失败 chn=%d ret=%#x", attr->chn, ret);
        return ret;
    }

    ret = mppVencJpeg_set_qfactor(attr->chn, attr->qfactor == 0 ? MPP_VENC_JPEG_QFACTOR_DEFAULT : attr->qfactor);
    if (ret != TD_SUCCESS)
    {
        ss_mpi_venc_destroy_chn(attr->chn);
        return ret;
    }

    /* 常驻接收，编码请求由用户态按需送入 */
    start_param.recv_pic_num = -1;
    ret = ss_mpi_venc_start_chn(attr->chn, &start_param);
    if (ret != TD_SUCCESS)
    {
        mpi_venc_log("启动通道失败 chn=%d ret=%#x", attr->chn, ret);
        ss_mpi_venc_destroy_chn(attr->chn);
        return ret;
    }
    return TD_SUCCESS;
}

int mppVencJpeg_destroy(ot_venc_chn chn)
{
    td_s32 ret = ss_mpi_venc_stop_chn(chn);
    if (ret != TD_SUCCESS)
    {
        mpi_venc_log("停止通道失败 chn=%d ret=%#x", chn, ret);
        return ret;
    }
    ret = ss_mpi_venc_destroy_chn(chn);
    if (ret != TD_SUCCESS)
    {
        mpi_venc_log("销毁通道失败 chn=%d ret=%#x", chn, ret);
    }
    return ret;
}

int mppVencJpeg_set_qfactor(ot_venc_chn chn, td_u32 qfactor)
{
    if (qfactor < 1 || qfactor > 99)
    {
        mpi_venc_log("qfactor %u 超出 [1,99]", qfactor);
        return OT_ERR_VENC_ILLEGAL_PARAM;
    }
    ot_venc_jpeg_param jpeg_param;
    (td_void) memset_s(&jpeg_param, sizeof(jpeg_param), 0, sizeof(jpeg_param));
    td_s32 ret = ss_mpi_venc_get_jpeg_param(chn, &jpeg_param);
    if (ret != TD_SUCCESS)
    {
        mpi_venc_log("获取 jpeg 参数失败 chn=%d ret=%#x", chn, ret);
        return ret;
    }
    jpeg_param.qfactor = qfactor;
    ret = ss_mpi_venc_set_jpeg_param(chn, &jpeg_param);
    if (ret != TD_SUCCESS)
    {
        mpi_venc_log("设置 jpeg 参数失败 chn=%d ret=%#x", chn, ret);
    }
    return ret;
}

/*
 * 等待并收走一帧码流，按 is_frame_end 拆分写入 outs。
 * batch_crop 语义下每个 pack 即一张独立 JPEG；全景模式允许单张图跨多 pack
 * （缓冲充足时不会发生，仍做累积兜底）。
 */
static int mpp_venc_jpeg_recv_and_split(ot_venc_chn chn, mpp_venc_jpeg_out *outs, td_u32 out_num, int timeout_ms)
{
    ot_venc_chn_status status;
    ot_venc_stream stream;
    fd_set read_fds;
    struct timeval tv;
    td_s32 ret;
    td_u32 img_idx = 0;
    td_u32 img_offset = 0;

    int fd = ss_mpi_venc_get_fd(chn);
    if (fd < 0)
    {
        mpi_venc_log("获取通道 fd 失败 chn=%d", chn);
        return OT_ERR_VENC_INVALID_CHN_ID;
    }
    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    ret = select(fd + 1, &read_fds, NULL, NULL, &tv);
    if (ret <= 0)
    {
        mpi_venc_log("等待码流超时/错误 chn=%d ret=%d", chn, ret);
        return OT_ERR_VENC_BUF_FULL;
    }

    (td_void) memset_s(&status, sizeof(status), 0, sizeof(status));
    ret = ss_mpi_venc_query_status(chn, &status);
    if (ret != TD_SUCCESS || status.cur_packs == 0)
    {
        mpi_venc_log("查询码流状态失败 chn=%d ret=%#x cur_packs=%u", chn, ret, status.cur_packs);
        return (ret != TD_SUCCESS) ? ret : OT_ERR_VENC_BUF_FULL;
    }

    stream.pack = (ot_venc_pack *) malloc(sizeof(ot_venc_pack) * status.cur_packs);
    if (stream.pack == NULL)
    {
        return OT_ERR_VENC_NO_BUF;
    }
    stream.pack_cnt = status.cur_packs;
    ret = ss_mpi_venc_get_stream(chn, &stream, timeout_ms);
    if (ret != TD_SUCCESS)
    {
        mpi_venc_log("获取码流失败 chn=%d ret=%#x", chn, ret);
        free(stream.pack);
        return ret;
    }

    for (td_u32 i = 0; i < stream.pack_cnt; i++)
    {
        const ot_venc_pack *pack = &stream.pack[i];
        const td_u8 *data = pack->addr + pack->offset;
        td_u32 len = pack->len - pack->offset;

        if (img_idx >= out_num)
        {
            mpi_venc_log("码流图片数超过预期(%u)", out_num);
            ret = OT_ERR_VENC_BUF_FULL;
            break;
        }
        /* 容量不足整体失败，由调用方兜底重试（加大缓冲或降质量） */
        if (img_offset + len > outs[img_idx].cap)
        {
            mpi_venc_log("输出缓冲不足 img=%u need=%u cap=%u", img_idx, img_offset + len, outs[img_idx].cap);
            ret = OT_ERR_VENC_NO_BUF;
            break;
        }
        (td_void) memcpy_s(outs[img_idx].buf + img_offset, outs[img_idx].cap - img_offset, data, len);
        img_offset += len;
        if (pack->is_frame_end == TD_TRUE)
        {
            outs[img_idx].len = img_offset;
            img_idx++;
            img_offset = 0;
        }
    }

    /* memory: 无论成败必须归还码流缓存，避免编码器环形缓存泄漏 */
    (td_void) ss_mpi_venc_release_stream(chn, &stream);
    free(stream.pack);
    return TD_SUCCESS;
}

static td_bool mpp_venc_jpeg_check_frame(const ot_video_frame_info *frame, td_u32 width, td_u32 height)
{
    if (frame == NULL)
    {
        return TD_FALSE;
    }
    /* batch_crop 硬性约束：原图大小必须等于编码通道大小 */
    if (frame->video_frame.width != width || frame->video_frame.height != height)
    {
        mpi_venc_log("帧尺寸(%ux%u)与通道(%ux%u)不一致", frame->video_frame.width, frame->video_frame.height, width, height);
        return TD_FALSE;
    }
    if (frame->video_frame.compress_mode != OT_COMPRESS_MODE_NONE)
    {
        mpi_venc_log("帧为压缩格式，batch_crop 不支持");
        return TD_FALSE;
    }
    return TD_TRUE;
}

int mppVencJpeg_encode_frame(ot_venc_chn chn, const ot_video_frame_info *frame, td_u8 *buf, td_u32 cap, td_u32 *out_len, int timeout_ms)
{
    ot_venc_chn_attr chn_attr;
    if (buf == NULL || out_len == NULL)
    {
        return OT_ERR_VENC_ILLEGAL_PARAM;
    }
    *out_len = 0;
    (td_void) memset_s(&chn_attr, sizeof(chn_attr), 0, sizeof(chn_attr));
    if (ss_mpi_venc_get_chn_attr(chn, &chn_attr) != TD_SUCCESS ||
        !mpp_venc_jpeg_check_frame(frame, chn_attr.venc_attr.pic_width, chn_attr.venc_attr.pic_height))
    {
        return OT_ERR_VENC_ILLEGAL_PARAM;
    }

    /* valid=false：只编码整帧原图 */
    ot_venc_frame_batch_crop_info info;
    (td_void) memset_s(&info, sizeof(info), 0, sizeof(info));
    info.frame = *frame;
    info.batch_crop_info.valid = TD_FALSE;
    info.batch_crop_info.crop_num = 0;
    td_s32 ret = ss_mpi_venc_send_frame_batch_crop(chn, &info, timeout_ms);
    if (ret != TD_SUCCESS)
    {
        mpi_venc_log("全景送帧失败 chn=%d ret=%#x", chn, ret);
        return ret;
    }

    mpp_venc_jpeg_out out = { buf, cap, 0 };
    ret = mpp_venc_jpeg_recv_and_split(chn, &out, 1, timeout_ms);
    *out_len = out.len;
    return ret;
}

int mppVencJpeg_encode_crops(ot_venc_chn chn,
                             const ot_video_frame_info *frame,
                             const ot_rect *rects,
                             td_u32 rect_num,
                             mpp_venc_jpeg_out *outs,
                             int timeout_ms)
{
    ot_venc_chn_attr chn_attr;
    if (rects == NULL || outs == NULL || rect_num == 0 || rect_num > MPP_VENC_JPEG_MAX_CROP_NUM)
    {
        return OT_ERR_VENC_ILLEGAL_PARAM;
    }
    for (td_u32 i = 0; i < rect_num; i++)
    {
        outs[i].len = 0;
    }
    (td_void) memset_s(&chn_attr, sizeof(chn_attr), 0, sizeof(chn_attr));
    if (ss_mpi_venc_get_chn_attr(chn, &chn_attr) != TD_SUCCESS ||
        !mpp_venc_jpeg_check_frame(frame, chn_attr.venc_attr.pic_width, chn_attr.venc_attr.pic_height) ||
        !mpp_venc_jpeg_check_rects(rects, rect_num, chn_attr.venc_attr.pic_width, chn_attr.venc_attr.pic_height))
    {
        return OT_ERR_VENC_ILLEGAL_PARAM;
    }

    /* valid=true：只编码抠图区域，硬件一次完成全部抠图编码 */
    ot_venc_frame_batch_crop_info info;
    (td_void) memset_s(&info, sizeof(info), 0, sizeof(info));
    info.frame = *frame;
    info.batch_crop_info.valid = TD_TRUE;
    info.batch_crop_info.crop_num = rect_num;
    for (td_u32 i = 0; i < rect_num; i++)
    {
        info.batch_crop_info.crop_rect[i] = rects[i];
    }
    td_s32 ret = ss_mpi_venc_send_frame_batch_crop(chn, &info, timeout_ms);
    if (ret != TD_SUCCESS)
    {
        mpi_venc_log("批量抠图送帧失败 chn=%d num=%u ret=%#x", chn, rect_num, ret);
        return ret;
    }
    return mpp_venc_jpeg_recv_and_split(chn, outs, rect_num, timeout_ms);
}
