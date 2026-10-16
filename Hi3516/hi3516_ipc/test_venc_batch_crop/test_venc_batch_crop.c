/**
 * @FilePath     : test_venc_batch_crop.c
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-20
 * @Description  : VENC JPEG 用户态批量抠图可用性 PoC（Hi3516CV610 / MPP V6.0）
 *
 * 验证目标（对应《MPP 媒体处理软件 V6.0 开发参考》6.3.17 节约束）：
 *   A. valid=false 只编原图：用户态帧 -> 1 张全景 JPEG；
 *   B. valid=true 批量抠图：一次送 X 个 rect -> 一帧码流 X 个 pack，
 *      每个 pack is_frame_end=true、各为独立 JPEG、按数组下标顺序；
 *   C. 非法参数错误码：rect.x 非 8 对齐 / 宽高 < 32x32 / (x+w,y+h) 越界，
 *      并验证报错后通道仍可正常编码；
 *   D. 编码耗时基线：全景与 3 路抠图各连续 10 次的 send->收流完成耗时；
 *   E. batch_crop_en 使能方式：头文件无显式设置接口，编码前后对比
 *      /proc/umap/jpege 的 batch_crop_en/batch_crop_num/pic_recv/pic_coded/pic_droped。
 *
 * 输出文件（供目视核对裁剪位置与内容）：
 *   /tmp/poc_full.jpg        —— Test A 全景
 *   /tmp/poc_B_crop_N.jpg    —— Test B 各抠图（N 按下发标顺序）
 *   /tmp/poc_C_ok_N.jpg      —— Test C 报错后的恢复性编码
 *   /tmp/poc_D_full_N.jpg / poc_D_crop_N_M.jpg —— Test D 性能轮次
 *
 * 用法：./test_venc_batch_crop [width height]   （默认 1920 1080，需 16 对齐宽）
 */

#define _POSIX_C_SOURCE 199309L /* clock_gettime / CLOCK_MONOTONIC */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "ot_buffer.h"
#include "ot_common.h"
#include "ot_common_vb.h"
#include "ot_common_venc.h"
#include "ot_common_video.h"
#include "ss_mpi_sys.h"
#include "ss_mpi_sys_mem.h"
#include "ss_mpi_vb.h"
#include "ss_mpi_venc.h"
#include "ss_mpi_sys_bind.h"

/* ---------------- PoC 参数 ---------------- */

#define POC_VENC_CHN          0    /* 独立进程独占 VENC，通道 0 即可 */
#define POC_VB_BLK_CNT        4    /* 测试帧缓存块数 */
#define POC_QFACTOR           90   /* JPEG 品质因数 [1,99] */
#define POC_STREAM_TIMEOUT_MS 3000 /* 收流 select 超时 */
#define POC_PERF_ROUNDS       10   /* 性能测试轮数 */

#define POC_CHECK(ret, msg)                                                                                                                \
    do                                                                                                                                     \
    {                                                                                                                                      \
        td_s32 _r = (ret);                                                                                                                 \
        if (_r != TD_SUCCESS)                                                                                                              \
        {                                                                                                                                  \
            printf("[POC][FAIL] %s ret=0x%08x\n", (msg), _r);                                                                              \
            goto cleanup;                                                                                                                  \
        }                                                                                                                                  \
    }                                                                                                                                      \
    while (0)

static td_u32 g_width = 1920;
static td_u32 g_height = 1080;

/* ---------------- 工具函数 ---------------- */

static long long poc_now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long) ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

/* 打印 /proc/umap/jpege 全文（表头行与数值行成对出现，数值行无关键字无法单独过滤） */
static void poc_dump_jpege_proc(const char *tag)
{
    char line[512];
    FILE *fp = fopen("/proc/umap/jpege", "r");
    if (fp == NULL)
    {
        printf("[POC][PROC:%s] 打开 /proc/umap/jpege 失败: %s\n", tag, strerror(errno));
        return;
    }
    printf("[POC][PROC:%s] ---- /proc/umap/jpege 全文 ----\n", tag);
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        printf("  %s", line);
    }
    fclose(fp);
}

/*
 * 构造可目视核对裁剪位置的测试图案（Y 平面）：
 * 每 64x64 块灰度 = (列号*16 + 行号*32) % 256，UV 平面填 128（灰）。
 * 裁剪出的 JPEG 与下标位置目视对照即可确认 crop 正确性与对齐偏移。
 */
static td_void poc_fill_test_pattern(td_u8 *virt, const ot_vb_calc_cfg *calc, td_u32 w, td_u32 h)
{
    td_u32 x, y;
    td_u8 *y_plane = virt;
    td_u8 *uv_plane = virt + calc->main_y_size;

    for (y = 0; y < h; y++)
    {
        td_u8 *row = y_plane + (td_ulong) y * calc->main_stride;
        for (x = 0; x < w; x++)
        {
            row[x] = (td_u8) (((x / 64) * 16 + (y / 64) * 32) % 256);
        }
        /* 行尾 padding 保持 0，避免 stride 差异影响 */
    }
    memset(uv_plane, 128, (td_ulong) calc->main_size - calc->main_y_size);
}

/* ---------------- JPEG 用户态编码通道 ---------------- */

static td_s32 poc_create_jpeg_chn(ot_venc_chn chn, td_u32 w, td_u32 h)
{
    ot_venc_chn_attr attr;
    ot_venc_start_param start_param;
    ot_venc_jpeg_param jpeg_param;

    memset(&attr, 0, sizeof(attr));
    /* 通道属性：与 mppVenc_init JPEG 分支一致的填法（见 hi_pipeline/mpp/venc/mpp_venc.c） */
    attr.venc_attr.type = OT_PT_JPEG;
    attr.venc_attr.max_pic_width = w;
    attr.venc_attr.max_pic_height = h;
    /* 码流 buffer 按 16 对齐宽高 * 2（q90 的 1080p JPEG 上限约 1MB，留足余量避免多包） */
    attr.venc_attr.buf_size = ((w + 15) / 16 * 16) * ((h + 15) / 16 * 16) * 2;
    attr.venc_attr.is_by_frame = TD_TRUE;
    attr.venc_attr.pic_width = w;
    attr.venc_attr.pic_height = h;
    attr.venc_attr.jpeg_attr.dcf_en = TD_FALSE;
    attr.venc_attr.jpeg_attr.mpf_cfg.large_thumbnail_num = 0;
    attr.venc_attr.jpeg_attr.recv_mode = OT_VENC_PIC_RECV_SINGLE;
    /* JPEG 无码率控制，rc_mode 保持 0（枚举无 NONE 值，现有固件 JPEG 通道同为 0） */
    attr.gop_attr.gop_mode = OT_VENC_GOP_MODE_NORMAL_P;
    attr.gop_attr.normal_p.ip_qp_delta = 0;

    td_s32 ret = ss_mpi_venc_create_chn(chn, &attr);
    if (ret != TD_SUCCESS)
    {
        printf("[POC][FAIL] ss_mpi_venc_create_chn ret=0x%08x\n", ret);
        return ret;
    }

    /* 品质因数：先 get 再改，保持量化表等默认值 */
    memset(&jpeg_param, 0, sizeof(jpeg_param));
    ret = ss_mpi_venc_get_jpeg_param(chn, &jpeg_param);
    if (ret != TD_SUCCESS)
    {
        printf("[POC][FAIL] ss_mpi_venc_get_jpeg_param ret=0x%08x\n", ret);
        return ret;
    }
    jpeg_param.qfactor = POC_QFACTOR;
    ret = ss_mpi_venc_set_jpeg_param(chn, &jpeg_param);
    if (ret != TD_SUCCESS)
    {
        printf("[POC][FAIL] ss_mpi_venc_set_jpeg_param ret=0x%08x\n", ret);
        return ret;
    }

    /* 常驻接收（用户态按需 send_frame_batch_crop） */
    memset(&start_param, 0, sizeof(start_param));
    start_param.recv_pic_num = -1;
    ret = ss_mpi_venc_start_chn(chn, &start_param);
    if (ret != TD_SUCCESS)
    {
        printf("[POC][FAIL] ss_mpi_venc_start_chn ret=0x%08x\n", ret);
        return ret;
    }
    return TD_SUCCESS;
}

static td_s32 poc_send_batch_crop(ot_venc_chn chn, const ot_video_frame_info *frame, const ot_rect *rects, td_u32 rect_num)
{
    ot_venc_frame_batch_crop_info info;
    memset(&info, 0, sizeof(info));
    info.frame = *frame;
    info.batch_crop_info.valid = (rect_num > 0) ? TD_TRUE : TD_FALSE;
    info.batch_crop_info.crop_num = rect_num;
    for (td_u32 i = 0; i < rect_num; i++)
    {
        info.batch_crop_info.crop_rect[i] = rects[i];
    }
    return ss_mpi_venc_send_frame_batch_crop(chn, &info, POC_STREAM_TIMEOUT_MS);
}

/*
 * 等待并收走一帧码流，按 is_frame_end 拆分写文件。
 * batch_crop 语义：一帧 X 个 pack，每 pack is_frame_end=true、各为独立 JPEG；
 * 全景模式：可能 1 个或多个 pack，is_frame_end=true 时写文件。
 * 返回写出的图片张数，失败返回负值。
 */
static int poc_wait_and_dump(ot_venc_chn chn, const char *prefix)
{
    ot_venc_chn_status status;
    ot_venc_stream stream;
    fd_set read_fds;
    struct timeval tv;
    td_s32 ret;
    int img_cnt = 0;
    char path[256];
    FILE *fp = NULL;

    int fd = ss_mpi_venc_get_fd(chn);
    if (fd < 0)
    {
        printf("[POC][FAIL] ss_mpi_venc_get_fd ret=%d\n", fd);
        return -1;
    }

    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);
    tv.tv_sec = POC_STREAM_TIMEOUT_MS / 1000;
    tv.tv_usec = (POC_STREAM_TIMEOUT_MS % 1000) * 1000;
    ret = select(fd + 1, &read_fds, NULL, NULL, &tv);
    if (ret <= 0)
    {
        printf("[POC][FAIL] select 等待码流超时/错误 ret=%d\n", ret);
        return -1;
    }

    memset(&status, 0, sizeof(status));
    ret = ss_mpi_venc_query_status(chn, &status);
    if (ret != TD_SUCCESS || status.cur_packs == 0)
    {
        printf("[POC][FAIL] query_status ret=0x%08x cur_packs=%u\n", ret, status.cur_packs);
        return -1;
    }

    memset(&stream, 0, sizeof(stream));
    stream.pack = (ot_venc_pack *) malloc(sizeof(ot_venc_pack) * status.cur_packs);
    if (stream.pack == NULL)
    {
        return -1;
    }
    stream.pack_cnt = status.cur_packs;
    ret = ss_mpi_venc_get_stream(chn, &stream, POC_STREAM_TIMEOUT_MS);
    if (ret != TD_SUCCESS)
    {
        printf("[POC][FAIL] ss_mpi_venc_get_stream ret=0x%08x\n", ret);
        free(stream.pack);
        return -1;
    }

    for (td_u32 i = 0; i < stream.pack_cnt; i++)
    {
        ot_venc_pack *pack = &stream.pack[i];
        const td_u8 *data = pack->addr + pack->offset;
        td_u32 len = pack->len - pack->offset;

        if (fp == NULL)
        {
            snprintf(path, sizeof(path), "/tmp/%s_%d.jpg", prefix, img_cnt);
            fp = fopen(path, "wb");
            if (fp == NULL)
            {
                printf("[POC][FAIL] 创建 %s 失败: %s\n", path, strerror(errno));
                break;
            }
        }
        fwrite(data, 1, len, fp);
        if (pack->is_frame_end == TD_TRUE)
        {
            fclose(fp);
            fp = NULL;
            printf("[POC]   写出 %s (%u 字节, pack%u)\n", path, len, i);
            img_cnt++;
        }
    }
    if (fp != NULL)
    { /* 兜底：码流异常未出现 frame_end */
        fclose(fp);
        img_cnt++;
    }

    ss_mpi_venc_release_stream(chn, &stream);
    free(stream.pack);
    return img_cnt;
}

/* ---------------- 测试用例 ---------------- */

/* Test A：valid=false 只编原图（全景图模式） */
static td_s32 poc_test_panorama(ot_venc_chn chn, const ot_video_frame_info *frame, const char *prefix)
{
    long long t0 = poc_now_ms();
    td_s32 ret = poc_send_batch_crop(chn, frame, NULL, 0);
    if (ret != TD_SUCCESS)
    {
        printf("[POC][FAIL] TestA send(valid=false) ret=0x%08x\n", ret);
        return ret;
    }
    int imgs = poc_wait_and_dump(chn, prefix);
    printf("[POC] TestA 全景编码: 产出 %d 张, 耗时 %lld ms（预期 1 张）\n", imgs, poc_now_ms() - t0);
    return (imgs == 1) ? TD_SUCCESS : TD_FAILURE;
}

/* Test B：valid=true 批量抠图（含贴边/大框/中框三类位置） */
static td_s32 poc_test_batch_crop(ot_venc_chn chn, const ot_video_frame_info *frame, const char *prefix, td_u32 expect)
{
    ot_rect rects[8];
    long long t0 = poc_now_ms();

    /* rect 约束（CV610）：x 8 对齐、y 2 对齐、宽高 >= 32x32、(x+w,y+h) 不越界 */
    rects[0].x = 0;
    rects[0].y = 0;
    rects[0].width = 512;
    rects[0].height = 384; /* 左上原点 */
    rects[1].x = 520;
    rects[1].y = 386;
    rects[1].width = 640;
    rects[1].height = 480; /* 中部 */
    if (expect >= 3)
    {
        rects[2].x = g_width - 256;
        rects[2].y = g_height - 256;
        rects[2].width = 256;
        rects[2].height = 256; /* 右下贴边 */
    }

    td_s32 ret = poc_send_batch_crop(chn, frame, rects, expect);
    if (ret != TD_SUCCESS)
    {
        printf("[POC][FAIL] TestB send(valid=true, %u rects) ret=0x%08x\n", expect, ret);
        return ret;
    }
    int imgs = poc_wait_and_dump(chn, prefix);
    printf("[POC] TestB 批量抠图(%u 框): 产出 %d 张, 耗时 %lld ms（预期 %u 张，"
           "每张内容应与 /tmp 下标块位置一致）\n",
           expect,
           imgs,
           poc_now_ms() - t0,
           expect);
    return (imgs == (int) expect) ? TD_SUCCESS : TD_FAILURE;
}

/*
 * Test C：非法参数应返回错误码且不产出码流，报错后通道可正常编码。
 * 依次验证：x 非 8 对齐 / 宽 < 32 / 越界。
 */
static td_s32 poc_test_illegal_params(ot_venc_chn chn, const ot_video_frame_info *frame)
{
    struct
    {
        const char *desc;
        ot_rect rect;
    } cases[] = {
        { "x=4 非 8 对齐",          { 4, 0, 64, 64 } },
        { "y=1 非 2 对齐",          { 0, 1, 64, 64 } },
        {      "宽 16 < 32",          { 0, 0, 16, 64 } },
        {       "x+w 越界", { 0, 0, g_width + 8, 64 } },
    };
    td_s32 ret = TD_SUCCESS;

    for (td_u32 i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
    {
        ret = poc_send_batch_crop(chn, frame, &cases[i].rect, 1);
        printf("[POC] TestC 非法参数[%s]: ret=0x%08x（预期非 0 错误码）\n", cases[i].desc, ret);
        if (ret == TD_SUCCESS)
        {
            /* 驱动未拦截：收走可能产出的码流，保持通道状态干净 */
            (void) poc_wait_and_dump(chn, "poc_C_unexpected");
            printf("[POC][WARN] 驱动未拦截该非法参数，需人工确认约束是否由上层保证\n");
        }
    }

    /* 报错后恢复性验证 */
    ot_rect ok_rect = { 0, 0, 256, 256 };
    ret = poc_send_batch_crop(chn, frame, &ok_rect, 1);
    if (ret != TD_SUCCESS)
    {
        printf("[POC][FAIL] TestC 报错后正常 crop 仍失败 ret=0x%08x\n", ret);
        return ret;
    }
    int imgs = poc_wait_and_dump(chn, "poc_C_ok");
    printf("[POC] TestC 恢复性验证: 产出 %d 张（预期 1 张）\n", imgs);
    return (imgs == 1) ? TD_SUCCESS : TD_FAILURE;
}

/* Test D：性能基线（send -> 收流完成） */
static td_s32 poc_test_perf(ot_venc_chn chn, const ot_video_frame_info *frame)
{
    long long full_min = __INT64_MAX__, full_max = 0, full_sum = 0;
    long long crop_min = __INT64_MAX__, crop_max = 0, crop_sum = 0;
    ot_rect rects[3] = {
        {             0,              0, 512, 384 },
        {           520,            386, 640, 480 },
        { g_width - 256, g_height - 256, 256, 256 },
    };
    char prefix[64];

    for (int i = 0; i < POC_PERF_ROUNDS; i++)
    {
        long long t0 = poc_now_ms();
        td_s32 ret = poc_send_batch_crop(chn, frame, NULL, 0);
        if (ret == TD_SUCCESS && poc_wait_and_dump(chn, "poc_D_full_tmp") == 1)
        {
            long long cost = poc_now_ms() - t0;
            full_min = (cost < full_min) ? cost : full_min;
            full_max = (cost > full_max) ? cost : full_max;
            full_sum += cost;
        }
        else
        {
            printf("[POC][FAIL] TestD 全景第 %d 轮失败\n", i);
            return TD_FAILURE;
        }
    }
    for (int i = 0; i < POC_PERF_ROUNDS; i++)
    {
        long long t0 = poc_now_ms();
        td_s32 ret = poc_send_batch_crop(chn, frame, rects, 3);
        if (ret == TD_SUCCESS && poc_wait_and_dump(chn, "poc_D_crop_tmp") == 3)
        {
            long long cost = poc_now_ms() - t0;
            crop_min = (cost < crop_min) ? cost : crop_min;
            crop_max = (cost > crop_max) ? cost : crop_max;
            crop_sum += cost;
        }
        else
        {
            printf("[POC][FAIL] TestD 三路抠图第 %d 轮失败\n", i);
            return TD_FAILURE;
        }
    }
    (void) prefix;
    (void) snprintf(prefix, sizeof(prefix), "/tmp/poc_D_*_tmp_*.jpg");
    printf("[POC] TestD 性能(%ux%u, q%d, %d 轮): 全景 min/avg/max = %lld/%lld/%lld ms, "
           "3 路抠图 = %lld/%lld/%lld ms（临时文件: %s 可删）\n",
           g_width,
           g_height,
           POC_QFACTOR,
           POC_PERF_ROUNDS,
           full_min,
           full_sum / POC_PERF_ROUNDS,
           full_max,
           crop_min,
           crop_sum / POC_PERF_ROUNDS,
           crop_max,
           prefix);
    return TD_SUCCESS;
}

/* ---------------- 主流程 ---------------- */

int main(int argc, char *argv[])
{
    ot_vb_cfg vb_cfg;
    ot_pic_buf_attr buf_attr;
    ot_vb_calc_cfg calc;
    ot_video_frame_info frame;
    ot_vb_blk blk = OT_VB_INVALID_HANDLE;
    td_phys_addr_t phys = 0;
    td_void *virt = NULL;
    td_s32 ret;
    td_bool chn_created = TD_FALSE;
    td_bool vb_inited = TD_FALSE;
    td_bool sys_inited = TD_FALSE;

    if (argc >= 3)
    {
        g_width = (td_u32) atoi(argv[1]);
        g_height = (td_u32) atoi(argv[2]);
    }
    printf("[POC] VENC batch_crop 可用性测试开始: 分辨率 %ux%u, VENC 通道 %d\n", g_width, g_height, POC_VENC_CHN);

    /* VB/SYS 初始化（独立进程，顺序与 streamSys_init 一致） */
    POC_CHECK(ss_mpi_sys_exit(), "预清理 ss_mpi_sys_exit");
    POC_CHECK(ss_mpi_vb_exit(), "预清理 ss_mpi_vb_exit");

    memset(&buf_attr, 0, sizeof(buf_attr));
    buf_attr.width = g_width;
    buf_attr.height = g_height;
    buf_attr.align = OT_DEFAULT_ALIGN;
    buf_attr.bit_width = OT_DATA_BIT_WIDTH_8;
    buf_attr.pixel_format = OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420; /* NV21，与 VPSS AI 通道一致 */
    buf_attr.compress_mode = OT_COMPRESS_MODE_NONE;
    buf_attr.video_format = OT_VIDEO_FORMAT_LINEAR;
    ot_common_get_pic_buf_cfg(&buf_attr, &calc);
    printf("[POC] 帧缓存: vb_size=%u main_stride=%u main_y_size=%u\n", calc.vb_size, calc.main_stride, calc.main_y_size);

    memset(&vb_cfg, 0, sizeof(vb_cfg));
    vb_cfg.max_pool_cnt = 1;
    vb_cfg.common_pool[0].blk_size = calc.vb_size;
    vb_cfg.common_pool[0].blk_cnt = POC_VB_BLK_CNT;
    vb_cfg.common_pool[0].remap_mode = OT_VB_REMAP_MODE_NONE;
    POC_CHECK(ss_mpi_vb_set_cfg(&vb_cfg), "ss_mpi_vb_set_cfg");
    POC_CHECK(ss_mpi_vb_init(), "ss_mpi_vb_init");
    vb_inited = TD_TRUE;
    POC_CHECK(ss_mpi_sys_init(), "ss_mpi_sys_init");
    sys_inited = TD_TRUE;

    /* 分配测试帧并写入目视图案 */
    blk = ss_mpi_vb_get_blk(OT_VB_INVALID_POOL_ID, calc.vb_size, NULL);
    if (blk == OT_VB_INVALID_HANDLE)
    {
        printf("[POC][FAIL] ss_mpi_vb_get_blk 失败\n");
        goto cleanup;
    }
    phys = ss_mpi_vb_handle_to_phys_addr(blk);
    virt = ss_mpi_sys_mmap(phys, calc.vb_size);
    if (virt == NULL)
    {
        printf("[POC][FAIL] ss_mpi_sys_mmap 失败\n");
        goto cleanup;
    }
    poc_fill_test_pattern(virt, &calc, g_width, g_height);

    memset(&frame, 0, sizeof(frame));
    frame.mod_id = OT_ID_VENC;
    frame.pool_id = ss_mpi_vb_handle_to_pool_id(blk);
    frame.video_frame.phys_addr[0] = phys;
    frame.video_frame.phys_addr[1] = phys + calc.main_y_size;
    frame.video_frame.virt_addr[0] = virt;
    frame.video_frame.virt_addr[1] = (td_void *) ((td_u8 *) virt + calc.main_y_size);
    frame.video_frame.width = g_width;
    frame.video_frame.height = g_height;
    frame.video_frame.stride[0] = calc.main_stride;
    frame.video_frame.stride[1] = calc.main_stride;
    frame.video_frame.pixel_format = buf_attr.pixel_format;
    frame.video_frame.video_format = OT_VIDEO_FORMAT_LINEAR;
    frame.video_frame.compress_mode = OT_COMPRESS_MODE_NONE;
    frame.video_frame.dynamic_range = OT_DYNAMIC_RANGE_SDR8;
    frame.video_frame.field = OT_VIDEO_FIELD_FRAME;
    frame.video_frame.pts = 0;

    /* 编码前 proc 基线 */
    poc_dump_jpege_proc("基线");

    POC_CHECK(poc_create_jpeg_chn(POC_VENC_CHN, g_width, g_height), "创建 JPEG 用户态通道");
    chn_created = TD_TRUE;

    /* Test A/B/C/D 依次执行 */
    ret = poc_test_panorama(POC_VENC_CHN, &frame, "poc_full");
    printf("[POC] ===== TestA 全景模式(valid=false): %s =====\n", ret == TD_SUCCESS ? "PASS" : "FAIL");
    poc_dump_jpege_proc("TestA后");

    ret = poc_test_batch_crop(POC_VENC_CHN, &frame, "poc_B_crop", 3);
    printf("[POC] ===== TestB 批量抠图(valid=true): %s =====\n", ret == TD_SUCCESS ? "PASS" : "FAIL");
    poc_dump_jpege_proc("TestB后");

    ret = poc_test_illegal_params(POC_VENC_CHN, &frame);
    printf("[POC] ===== TestC 非法参数约束: %s =====\n", ret == TD_SUCCESS ? "PASS" : "FAIL");

    ret = poc_test_perf(POC_VENC_CHN, &frame);
    printf("[POC] ===== TestD 性能基线: %s =====\n", ret == TD_SUCCESS ? "PASS" : "FAIL");
    poc_dump_jpege_proc("结束");

cleanup:
    if (chn_created)
    {
        ss_mpi_venc_stop_chn(POC_VENC_CHN);
        ss_mpi_venc_destroy_chn(POC_VENC_CHN);
    }
    if (virt != NULL)
    {
        ss_mpi_sys_munmap(virt, calc.vb_size);
    }
    if (blk != OT_VB_INVALID_HANDLE)
    {
        ss_mpi_vb_release_blk(blk);
    }
    if (sys_inited)
    {
        ss_mpi_sys_exit();
    }
    if (vb_inited)
    {
        ss_mpi_vb_exit();
    }
    printf("[POC] 测试结束，结果文件见 /tmp/poc_*.jpg，请将各 jpg 与 64px 灰度块图案目视比对\n");
    return 0;
}
