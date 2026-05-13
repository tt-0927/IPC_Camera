/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#include "sample_common_svp.h"

#include <stdio.h>

#include "ot_common.h"
#include "ot_common_video.h"
#include "ot_common_sys.h"
#include "ot_common_svp.h"
#include "ss_mpi_vb.h"
#include "sample_comm.h"
#include "sample_common_ive.h"

#define SAMPLE_SVP_BLK_CNT           7
#define SMAPLE_SVP_DISPLAY_BUF_LEN   3
#define SAMPLE_SVP_VI_CHN_INTERVAL   4
#define SAMPLE_SVP_VDEC_CHN_0        0
#define SAMPLE_SVP_VDEC_CHN_1        1
#define SAMPLE_SVP_VDEC_CHN_2        2
#define SAMPLE_SVP_VDEC_CHN_NUM      1
#define SAMPLE_SVP_VPSS_BORDER_WIDTH 3
#define SAMPLE_SVP_VO_DIS_BUF_LEN    3
#define SAMPLE_SVP_MAX_WIDTH         32768
#define SAMPLE_SVP_NUM_TWO          2
#define SAMPLE_SVP_DSP_BIN_NUM_PER  4
#define SAMPLE_SVP_DSP_MEM_TYPE_SYS_DDR 0
#define SAMPLE_SVP_DSP_MEM_TYPE_IRAM    1
#define SAMPLE_SVP_DSP_MEM_TYPE_DRAM_0  2
#define SAMPLE_SVP_DSP_MEM_TYPE_DRAM_1  3
#define SAMPLE_SVP_VGS_COLOR_NUM 12
#define SAMPLE_SVP_NPU_RECT_LEFT_TOP     0
#define SAMPLE_SVP_NPU_RECT_RIGHT_TOP    1
#define SAMPLE_SVP_NPU_RECT_RIGHT_BOTTOM 2
#define SAMPLE_SVP_NPU_RECT_LEFT_BOTTOM  3
#define SAMPLE_SVP_VGS_LINE_THICK 4

#define SAMPLE_SVP_RGN_START_HANDLE 32
#define SAMPLE_SVP_MAX_RGN_CNT 32
#define SAMPLE_SVP_VGS_OSD_BG_COLOR 0x00ff00
#define SAMPLE_SVP_VGS_OSD_FONT_COLOR 0X0000
#define SAMPLE_SVP_VGS_OSD_TEXT_FRAME_W 32
#define SAMPLE_SVP_VGS_OSD_TEXT_FRAME_H 32
#define SAMPLE_SVP_VGS_DIGIT_NUM 10
#define SAMPLE_SVP_VGS_OSD_FONT_WIDTH 16
#define SAMPLE_SVP_VGS_OSD_FONT_HEIGHT 32
#define SAMPLE_SVP_VGS_SINGLE_DIGIT_LEFT_SAPCE 8
#define SAMPLE_SVP_VGS_SINGLE_DIGIT_RIGHT_SAPCE 8
#define SAMPLE_SVP_VGS_BYTE_BIT 8
#define SAMPLE_SVP_VGS_ALPHA_MAX 255
#define SAMPLE_SVP_SINGLE_FRAME_VPSS_CHN_NUM 2 /* 2: one image + one show frame is single frame mode */
#define SAMPLE_SVP_VDEC_CHN_NUM 1
#define SAMPLE_SVP_LEFT_RIGHT_STRIDE_HALF 2
#define SAMPLE_SVP_NPU_VPSS_CHN_NUM 3

static td_bool g_sample_svp_init_flag = TD_FALSE;

static td_u8 sample_common_svp_vgs_font_matrixs[640] = { /* 640: 10 digits * 32 bit * 16 bit = 640 B */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xc0, 0x0c, 0x30, 0x18, 0x30, 0x18, 0x18,
    0x38, 0x18, 0x30, 0x1c, 0x30, 0x0c, 0x30, 0x0c, 0x70, 0x0c, 0x70, 0x0c, 0x70, 0x0c, 0x70, 0x0c,
    0x70, 0x0c, 0x30, 0x0c, 0x30, 0x0c, 0x30, 0x1c, 0x30, 0x18, 0x18, 0x18, 0x18, 0x18, 0x0c, 0x30,
    0x06, 0x60, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x03, 0x80, 0x07, 0x80, 0x01, 0x80,
    0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80,
    0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80,
    0x07, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 1 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xe0, 0x18, 0x30, 0x10, 0x18, 0x30, 0x18,
    0x30, 0x1c, 0x38, 0x1c, 0x38, 0x18, 0x00, 0x18, 0x00, 0x18, 0x00, 0x30, 0x00, 0x60, 0x00, 0x40,
    0x00, 0x80, 0x01, 0x00, 0x02, 0x00, 0x04, 0x00, 0x08, 0x04, 0x10, 0x04, 0x30, 0x0c, 0x20, 0x38,
    0x3f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 2 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xc0, 0x18, 0x30, 0x30, 0x30, 0x30, 0x18,
    0x38, 0x18, 0x10, 0x18, 0x00, 0x18, 0x00, 0x30, 0x00, 0x60, 0x03, 0xc0, 0x01, 0xe0, 0x00, 0x30,
    0x00, 0x18, 0x00, 0x18, 0x00, 0x0c, 0x00, 0x0c, 0x30, 0x0c, 0x38, 0x1c, 0x30, 0x18, 0x30, 0x30,
    0x0c, 0x60, 0x03, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 3 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x60, 0x00, 0xe0, 0x00, 0xe0,
    0x01, 0x60, 0x03, 0x60, 0x02, 0x60, 0x04, 0x60, 0x04, 0x60, 0x08, 0x60, 0x10, 0x60, 0x10, 0x60,
    0x20, 0x60, 0x60, 0x60, 0x7f, 0xfe, 0x00, 0x60, 0x00, 0x60, 0x00, 0x60, 0x00, 0x60, 0x00, 0x60,
    0x01, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 4 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xf8, 0x1f, 0xf8, 0x10, 0x00, 0x10, 0x00,
    0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x17, 0xe0, 0x18, 0x30, 0x10, 0x18, 0x00, 0x18,
    0x00, 0x0c, 0x00, 0x0c, 0x00, 0x0c, 0x10, 0x0c, 0x38, 0x0c, 0x30, 0x18, 0x30, 0x18, 0x10, 0x30,
    0x0c, 0x60, 0x03, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 5 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xf0, 0x04, 0x18, 0x08, 0x18, 0x18, 0x18,
    0x10, 0x00, 0x30, 0x00, 0x30, 0x00, 0x30, 0x00, 0x33, 0xf0, 0x74, 0x38, 0x78, 0x18, 0x70, 0x0c,
    0x70, 0x0c, 0x70, 0x0c, 0x30, 0x0c, 0x30, 0x0c, 0x30, 0x0c, 0x30, 0x0c, 0x18, 0x08, 0x1c, 0x18,
    0x0e, 0x70, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 6 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xfc, 0x3f, 0xfc, 0x30, 0x08, 0x20, 0x10,
    0x20, 0x10, 0x00, 0x20, 0x00, 0x20, 0x00, 0x60, 0x00, 0x40, 0x00, 0xc0, 0x00, 0x80, 0x01, 0x80,
    0x01, 0x80, 0x01, 0x80, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x07, 0x00,
    0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 7 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xe0, 0x18, 0x10, 0x30, 0x18, 0x30, 0x0c,
    0x30, 0x0c, 0x30, 0x0c, 0x30, 0x08, 0x38, 0x18, 0x1e, 0x30, 0x0f, 0xe0, 0x07, 0xe0, 0x18, 0xf0,
    0x30, 0x38, 0x30, 0x18, 0x60, 0x0c, 0x60, 0x0c, 0x60, 0x0c, 0x60, 0x0c, 0x30, 0x08, 0x10, 0x18,
    0x0c, 0x70, 0x03, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 8 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xc0, 0x18, 0x30, 0x30, 0x18, 0x30, 0x18,
    0x70, 0x18, 0x60, 0x0c, 0x60, 0x0c, 0x60, 0x0c, 0x70, 0x0c, 0x30, 0x1c, 0x30, 0x2c, 0x38, 0x6c,
    0x0f, 0xcc, 0x00, 0x1c, 0x00, 0x1c, 0x00, 0x18, 0x00, 0x18, 0x10, 0x30, 0x38, 0x30, 0x38, 0x60,
    0x18, 0xc0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; /* 9 */

typedef struct {
    td_u16 *bitmap_data;
    td_u32 stride;
    ot_size rgn_size;
    ot_size font_size;
    td_u32 bg_color;
    td_u16 font_color;
    td_char text[1024];
} sample_common_svp_text_bitmap;

/* System init */
static td_s32 sample_comm_svp_sys_init(td_void)
{
    td_s32 ret;
    ot_vb_cfg vb_cfg;

    ret = ss_mpi_sys_exit();
    sample_svp_check_exps_return(ret != TD_SUCCESS, ret, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error(%#x):ss_mpi_sys_exit failed!\n", ret);
    ret = ss_mpi_vb_exit();
    sample_svp_check_exps_return(ret != TD_SUCCESS, ret, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error(%#x):ss_mpi_vb_exit failed!\n", ret);

    (td_void)memset_s(&vb_cfg, sizeof(ot_vb_cfg), 0, sizeof(ot_vb_cfg));

    vb_cfg.max_pool_cnt = SAMPLE_SVP_VB_POOL_NUM;
    vb_cfg.common_pool[1].blk_size = SAMPLE_SVP_D1_PAL_WIDTH * SAMPLE_SVP_D1_PAL_HEIGHT * SAMPLE_SVP_VB_POOL_NUM;
    vb_cfg.common_pool[1].blk_cnt  = 1;

    ret = ss_mpi_vb_set_cfg((const ot_vb_cfg *)&vb_cfg);
    sample_svp_check_exps_return(ret != TD_SUCCESS, ret, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error(%#x):ss_mpi_vb_set_config failed!\n", ret);

    ret = ss_mpi_vb_init();
    sample_svp_check_exps_return(ret != TD_SUCCESS, ret, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error(%#x):ss_mpi_vb_init failed!\n", ret);

    ret = ss_mpi_sys_init();
    sample_svp_check_exps_return(ret != TD_SUCCESS, ret, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error(%#x):ss_mpi_sys_init failed!\n", ret);

    return ret;
}

/* System exit */
static td_s32 sample_comm_svp_sys_exit(td_void)
{
    td_s32 ret;

    ret = ss_mpi_sys_exit();
    sample_svp_check_exps_return(ret != TD_SUCCESS, ret, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error(%#x):ss_mpi_sys_exit failed!\n", ret);

    ret = ss_mpi_vb_exit();
    sample_svp_check_exps_return(ret != TD_SUCCESS, ret, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error(%#x):ss_mpi_vb_exit failed!\n", ret);

    return TD_SUCCESS;
}

/* System init */
td_s32 sample_common_svp_check_sys_init(td_void)
{
    if (g_sample_svp_init_flag == TD_FALSE) {
        if (sample_comm_svp_sys_init() != TD_SUCCESS) {
            sample_svp_trace_err("Svp mpi init failed!\n");
            return TD_FALSE;
        }
        g_sample_svp_init_flag = TD_TRUE;
    }

    sample_svp_trace_info("Svp mpi init ok!\n");
    return TD_TRUE;
}

/* System exit */
td_void sample_common_svp_check_sys_exit(td_void)
{
    td_s32 ret;

    if (g_sample_svp_init_flag == TD_TRUE) {
        ret = sample_comm_svp_sys_exit();
        if (ret != TD_SUCCESS) {
            sample_svp_trace_err("svp mpi exit failed!\n");
        }
    }
    g_sample_svp_init_flag = TD_FALSE;
    sample_svp_trace_info("Svp mpi exit ok!\n");
}

/* Align */
td_u32 sample_common_svp_align(td_u32 size, td_u16 align)
{
    td_u32 stride;

    sample_svp_check_exps_return(align == 0, 0, SAMPLE_SVP_ERR_LEVEL_ERROR, "align can't be zero!\n");
    sample_svp_check_exps_return((size < 1) || (size > SAMPLE_SVP_MAX_WIDTH), 0, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "size(%u) must be [1, %u]\n", size, SAMPLE_SVP_MAX_WIDTH);
    stride = size + (align - size % align) % align;
    return stride;
}

/* Create mem info */
td_s32 sample_common_svp_create_mem_info(ot_svp_mem_info *mem_info, td_u32 size, td_u32 addr_offset)
{
    td_s32 ret = TD_FAILURE;
    td_u32 size_tmp;

    sample_svp_check_exps_return(mem_info == TD_NULL, ret, SAMPLE_SVP_ERR_LEVEL_ERROR, "mem_info can't be zero\n");

    size_tmp = size + addr_offset;
    mem_info->size = size;
    ret = ss_mpi_sys_mmz_alloc((td_phys_addr_t *)(&mem_info->phys_addr),
        (void **)&mem_info->virt_addr, TD_NULL, TD_NULL, size_tmp);
    sample_svp_check_exps_return(ret != TD_SUCCESS, ret, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error(%#x):ss_mpi_sys_alloc failed!\n", ret);

    mem_info->phys_addr += addr_offset;
    mem_info->virt_addr += addr_offset;

    return ret;
}

/* Destory mem info */
td_void sample_common_svp_destroy_mem_info(ot_svp_mem_info *mem_info, td_u32 addr_offset)
{
    sample_svp_check_exps_return_void(mem_info == TD_NULL, SAMPLE_SVP_ERR_LEVEL_ERROR, "mem_info can't be zero\n");

    if ((mem_info->virt_addr != 0) && (mem_info->phys_addr != 0)) {
        (td_void)ss_mpi_sys_mmz_free(mem_info->phys_addr - addr_offset,
            sample_svp_convert_addr_to_ptr(void, (mem_info->virt_addr - addr_offset)));
    }
    (td_void)memset_s(mem_info, sizeof(*mem_info), 0, sizeof(*mem_info));
}
/* Malloc memory */
td_s32 sample_common_svp_malloc_mem(td_char *mmb, td_char *zone, td_phys_addr_t *phys_addr,
    td_void **virt_addr, td_u32 size)
{
    td_s32 ret = TD_FAILURE;

    sample_svp_check_exps_return(phys_addr == TD_NULL, ret, SAMPLE_SVP_ERR_LEVEL_ERROR, "phys_addr can't be null\n");
    sample_svp_check_exps_return(virt_addr == TD_NULL, ret, SAMPLE_SVP_ERR_LEVEL_ERROR, "virt_addr can't be null\n");
    ret = ss_mpi_sys_mmz_alloc((td_phys_addr_t *)phys_addr, virt_addr, mmb, zone, size);

    return ret;
}

/* Malloc memory with cached */
td_s32 sample_common_svp_malloc_cached(td_char *mmb, td_char *zone, td_phys_addr_t *phys_addr,
    td_void **virt_addr, td_u32 size)
{
    td_s32 ret = TD_FAILURE;

    sample_svp_check_exps_return(phys_addr == TD_NULL, ret, SAMPLE_SVP_ERR_LEVEL_ERROR, "phys_addr can't be null\n");
    sample_svp_check_exps_return(virt_addr == TD_NULL, ret, SAMPLE_SVP_ERR_LEVEL_ERROR, "virt_addr can't be null\n");
    ret = ss_mpi_sys_mmz_alloc_cached((td_phys_addr_t *)phys_addr, virt_addr, mmb, zone, size);

    return ret;
}

/* Fulsh cached */
td_s32 sample_common_svp_flush_cache(td_phys_addr_t phys_addr, td_void *virt_addr, td_u32 size)
{
    td_s32 ret = TD_FAILURE;

    sample_svp_check_exps_return(virt_addr == TD_NULL, ret, SAMPLE_SVP_ERR_LEVEL_ERROR, "virt_addr can't be null\n");
    ret = ss_mpi_sys_flush_cache((td_phys_addr_t)phys_addr, virt_addr, size);
    return ret;
}

/*
 * function : Init Vb
 */
static td_s32 sample_common_svp_vb_init(ot_pic_size *pic_type, ot_size *pic_size,
    td_u32 vpss_chn_num)
{
    td_s32 ret;
    td_u32 i;
    ot_vb_cfg vb_cfg = {0};
    ot_pic_buf_attr pic_buf_attr;
    ot_vb_calc_cfg calc_cfg;
    ot_vi_vpss_mode_type mode_type = OT_VI_ONLINE_VPSS_OFFLINE;
    ot_vi_aiisp_mode aiisp_mode = OT_VI_AIISP_MODE_DEFAULT;

    vb_cfg.max_pool_cnt = OT_SAMPLE_IVE_MAX_POOL_CNT;

    ret = sample_comm_sys_get_pic_size(pic_type[0], &pic_size[0]);
    sample_svp_check_exps_goto(ret != TD_SUCCESS, vb_fail_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "sample_comm_sys_get_pic_size failed,Error(%#x)!\n", ret);
    pic_buf_attr.width = pic_size[0].width;
    pic_buf_attr.height = pic_size[0].height;
    pic_buf_attr.align = OT_DEFAULT_ALIGN;
    pic_buf_attr.bit_width = OT_DATA_BIT_WIDTH_8;
    pic_buf_attr.pixel_format = OT_PIXEL_FORMAT_YVU_SEMIPLANAR_422;
    pic_buf_attr.compress_mode = OT_COMPRESS_MODE_NONE;
    pic_buf_attr.video_format = OT_VIDEO_FORMAT_LINEAR;
    ot_common_get_pic_buf_cfg(&pic_buf_attr, &calc_cfg);

    vb_cfg.common_pool[0].blk_size = calc_cfg.vb_size;
    vb_cfg.common_pool[0].blk_cnt = SAMPLE_SVP_BLK_CNT;

    for (i = 1; (i < vpss_chn_num) && (i < OT_VB_MAX_COMMON_POOLS); i++) {
        ret = sample_comm_sys_get_pic_size(pic_type[i], &pic_size[i]);
        sample_svp_check_exps_goto(ret != TD_SUCCESS, vb_fail_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
            "sample_comm_sys_get_pic_size failed,Error(%#x)!\n", ret);
        pic_buf_attr.width = pic_size[i].width;
        pic_buf_attr.height = pic_size[i].height;
        pic_buf_attr.compress_mode = OT_COMPRESS_MODE_NONE;
        pic_buf_attr.align = OT_DEFAULT_ALIGN;

        ot_common_get_pic_buf_cfg(&pic_buf_attr, &calc_cfg);

        /* comm video buffer */
        vb_cfg.common_pool[i].blk_size = calc_cfg.vb_size;
        vb_cfg.common_pool[i].blk_cnt = SAMPLE_SVP_BLK_CNT;
    }

    ret = sample_comm_sys_init_with_vb_supplement(&vb_cfg, OT_VB_SUPPLEMENT_BNR_MOT_MASK);
    sample_svp_check_exps_goto(ret != TD_SUCCESS, vb_fail_1, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "sample_comm_sys_vb_init failed,Error(%#x)!\n", ret);

    ret = sample_comm_vi_set_vi_vpss_mode(mode_type, aiisp_mode);
    sample_svp_check_exps_goto(ret != TD_SUCCESS, vb_fail_1, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "sample_comm_vi_set_vi_vpss_mode failed!\n");
    return ret;
vb_fail_1:
    sample_comm_sys_exit();
vb_fail_0:
    return ret;
}

td_s32 sample_common_svp_destroy_all_rgn(td_u32 create_rgn_cnt)
{
    for (td_u32 i = 0; i < create_rgn_cnt; i++) {
        td_s32 ret = ss_mpi_rgn_destroy(i + SAMPLE_SVP_RGN_START_HANDLE);
        if (ret != TD_SUCCESS) {
            printf("ss_mpi_rgn_destroy err with %#x\n", ret);
        }
    }
    return TD_SUCCESS;
}

td_s32 sample_common_svp_create_all_rgn(td_void)
{
    ot_rgn_attr rgn_attr;
    rgn_attr.type = OT_RGN_OVERLAYEX;
    rgn_attr.attr.overlayex.pixel_format = OT_PIXEL_FORMAT_ARGB_1555;
    rgn_attr.attr.overlayex.bg_color = SAMPLE_SVP_VGS_OSD_BG_COLOR;
    rgn_attr.attr.overlayex.size.width = SAMPLE_SVP_VGS_OSD_TEXT_FRAME_W;
    rgn_attr.attr.overlayex.size.height = SAMPLE_SVP_VGS_OSD_TEXT_FRAME_H;
    rgn_attr.attr.overlayex.canvas_num = 1;
    for (td_u32 i = 0; i < SAMPLE_SVP_MAX_RGN_CNT; i++) {
        td_s32 ret = ss_mpi_rgn_create(i + SAMPLE_SVP_RGN_START_HANDLE, &rgn_attr);
        if (ret != TD_SUCCESS) {
            printf("create rgn(%u) failed with %#x\n", i + SAMPLE_SVP_RGN_START_HANDLE, ret);
            sample_common_svp_destroy_all_rgn(i);
            return TD_FAILURE;
        }
    }
    return TD_SUCCESS;
}

td_s32 sample_common_svp_check_rect_on_same_line(ot_point *points)
{
    return points[SAMPLE_SVP_NPU_RECT_LEFT_TOP].x == points[SAMPLE_SVP_NPU_RECT_RIGHT_BOTTOM].x ||
        points[SAMPLE_SVP_NPU_RECT_LEFT_TOP].y == points[SAMPLE_SVP_NPU_RECT_RIGHT_BOTTOM].y;
}

td_s32 sample_common_svp_pre_check_no_rect(ot_sample_svp_rect_info *rect)
{
    int cnt = 0;
    for (int i = 0; i < rect->num; i++) {
        if (sample_common_svp_check_rect_on_same_line(rect->rect[i].point) == TD_TRUE) {
            continue;
        }
        cnt++;
    }
    if (cnt == 0) {
        return TD_TRUE;
    } else {
        return TD_FALSE;
    }
}

static td_u16 sample_common_svp_font_fill_in_bit(td_u32 bitmap_row, td_u32 bitmap_col, td_u16 bg_color,
    td_u16 font_color, td_s32 id)
{
    if (id < SAMPLE_SVP_VGS_DIGIT_NUM) {
        int bit = (td_s32)(bitmap_row * SAMPLE_SVP_VGS_OSD_FONT_WIDTH + bitmap_col);
        td_s32 matrix_pos = bit / SAMPLE_SVP_VGS_BYTE_BIT;
        td_s32 matrix_bit = SAMPLE_SVP_VGS_BYTE_BIT - 1 - (bit % SAMPLE_SVP_VGS_BYTE_BIT);
        if (sample_common_svp_vgs_font_matrixs[id * (SAMPLE_SVP_VGS_OSD_FONT_WIDTH * SAMPLE_SVP_VGS_OSD_FONT_HEIGHT /
            SAMPLE_SVP_VGS_BYTE_BIT) + matrix_pos] & (td_u8)(1 << matrix_bit)) {
            return font_color;
        } else {
            return bg_color;
        }
    }
    return bg_color;
}

static td_void sample_common_svp_config_bit_map(int id_bit_high, int id_bit_low, sample_common_svp_text_bitmap *text)
{
    const td_u32 stride_half = text->stride / SAMPLE_SVP_LEFT_RIGHT_STRIDE_HALF; /* 2: left and right divide stride */
    if (id_bit_high == 0) {
        for (td_u32 bitmap_row = 0; bitmap_row < SAMPLE_SVP_VGS_OSD_FONT_HEIGHT; ++bitmap_row) {
            for (td_u32 col = 0; col < SAMPLE_SVP_VGS_SINGLE_DIGIT_LEFT_SAPCE; col++) {
                td_u32 bitmap_data_idx = bitmap_row * stride_half + col;
                text->bitmap_data[bitmap_data_idx] = (td_u16)text->bg_color;
            }
            for (td_u32 bitmap_col = SAMPLE_SVP_VGS_SINGLE_DIGIT_LEFT_SAPCE; bitmap_col <
                SAMPLE_SVP_VGS_SINGLE_DIGIT_LEFT_SAPCE + SAMPLE_SVP_VGS_OSD_FONT_WIDTH; ++bitmap_col) {
                td_u32 bitmap_data_idx = bitmap_row * stride_half + bitmap_col;
                text->bitmap_data[bitmap_data_idx] = sample_common_svp_font_fill_in_bit(bitmap_row, bitmap_col -
                    SAMPLE_SVP_VGS_SINGLE_DIGIT_LEFT_SAPCE, (td_u16)text->bg_color, (td_u16)text->font_color,
                    id_bit_low);
            }
            for (td_u32 col = SAMPLE_SVP_VGS_SINGLE_DIGIT_LEFT_SAPCE + SAMPLE_SVP_VGS_OSD_FONT_WIDTH; col <
                SAMPLE_SVP_VGS_OSD_TEXT_FRAME_W; col++) {
                td_u32 bitmap_data_idx = bitmap_row * stride_half + col;
                text->bitmap_data[bitmap_data_idx] = (td_u16)text->bg_color;
            }
        }
    } else {
        for (td_u32 bitmap_row = 0; bitmap_row < SAMPLE_SVP_VGS_OSD_FONT_HEIGHT; ++bitmap_row) {
            for (td_u32 bitmap_col = 0; bitmap_col < SAMPLE_SVP_VGS_OSD_FONT_WIDTH; ++bitmap_col) {
                td_u32 bitmap_data_idx = bitmap_row * stride_half + bitmap_col;
                text->bitmap_data[bitmap_data_idx] = sample_common_svp_font_fill_in_bit(bitmap_row, bitmap_col,
                    (td_u16)text->bg_color, (td_u16)text->font_color, id_bit_high);
            }
            for (td_u32 bitmap_col = SAMPLE_SVP_VGS_OSD_FONT_WIDTH; bitmap_col < SAMPLE_SVP_VGS_OSD_TEXT_FRAME_W;
                ++bitmap_col) {
                td_u32 bitmap_data_idx = bitmap_row * stride_half + bitmap_col;
                text->bitmap_data[bitmap_data_idx] = sample_common_svp_font_fill_in_bit(bitmap_row, bitmap_col -
                    SAMPLE_SVP_VGS_OSD_FONT_WIDTH, (td_u16)text->bg_color, (td_u16)text->font_color, id_bit_low);
            }
        }
    }
}

static td_void sample_common_svp_tune_notify_box_range(td_s32 *x_rect, td_s32 *y_rect, td_s32 width, td_s32 height,
    ot_size frame_size)
{
    *x_rect = (*x_rect < 0) ? 0 : (*x_rect);
    *y_rect = (*y_rect < 0) ? 0 : (*y_rect);
    *x_rect = (*x_rect + width >= (td_s32)frame_size.width) ? ((td_s32)frame_size.width - width - 1) : (*x_rect);
    *y_rect = (*y_rect + height >= (td_s32)frame_size.height) ? ((td_s32)frame_size.height - height - 1) : (*y_rect);
}

static td_s32 sample_common_svp_config_osd(td_s32 i, ot_vgs_osd *osd, ot_sample_svp_rect_info *rect, td_u32 width,
    td_u32 height)
{
    td_s32 ret;
    ot_rgn_canvas_info canvas_info = {0};
    ret = ss_mpi_rgn_get_canvas_info(SAMPLE_SVP_RGN_START_HANDLE + i, &canvas_info); /* get canvas info virt addr */
    sample_svp_check_exps_return(ret != TD_SUCCESS, ret, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "ss_mpi_rgn_get_canvas_info failed with %#x\n", ret);

    sample_common_svp_text_bitmap bitmap; /* mmz in bitmap is managed by rgn, bitmap is a tool */
    ret = memset_s(&bitmap, sizeof(bitmap), 0, sizeof(bitmap));
    sample_svp_check_exps_return(ret != EOK, TD_FAILURE, SAMPLE_SVP_ERR_LEVEL_ERROR, "clear memory of bit map failed");

    /* config bitmap */
    int id_bit_low = (td_s32)(rect->ids[i] % SAMPLE_SVP_VGS_DIGIT_NUM);
    int id_bit_high = (td_s32)((rect->ids[i] / SAMPLE_SVP_VGS_DIGIT_NUM) % SAMPLE_SVP_VGS_DIGIT_NUM);
    bitmap.bitmap_data = (td_u16 *)(canvas_info.virt_addr);
    bitmap.stride = canvas_info.stride;
    bitmap.font_size.width = SAMPLE_SVP_VGS_OSD_TEXT_FRAME_W;
    bitmap.font_size.height = SAMPLE_SVP_VGS_OSD_TEXT_FRAME_H;
    bitmap.rgn_size.width = SAMPLE_SVP_VGS_OSD_TEXT_FRAME_W;
    bitmap.rgn_size.height = SAMPLE_SVP_VGS_OSD_TEXT_FRAME_H;
    bitmap.bg_color = SAMPLE_SVP_VGS_OSD_BG_COLOR;
    bitmap.font_color = SAMPLE_SVP_VGS_OSD_FONT_COLOR;

    /* fill bitmap */
    sample_common_svp_config_bit_map(id_bit_high, id_bit_low, &bitmap);
    ret = ss_mpi_rgn_update_canvas(SAMPLE_SVP_RGN_START_HANDLE + i);
    sample_svp_check_exps_return(ret != TD_SUCCESS, ret, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "ss_mpi_rgn_update_canvas failed with %#x\n", ret);

    /* rect info of tracker id display */
    td_s32 x_rect = (td_u32)(rect->rect[i].point[SAMPLE_SVP_NPU_RECT_LEFT_TOP].x - SAMPLE_SVP_VGS_OSD_TEXT_FRAME_H) &
        (~1);
    td_s32 y_rect = (td_u32)(rect->rect[i].point[SAMPLE_SVP_NPU_RECT_LEFT_TOP].y - SAMPLE_SVP_VGS_OSD_TEXT_FRAME_W) &
        (~1);
    ot_size frame_si = {width, height};
    sample_common_svp_tune_notify_box_range(&x_rect, &y_rect, SAMPLE_SVP_VGS_OSD_TEXT_FRAME_W,
        SAMPLE_SVP_VGS_OSD_TEXT_FRAME_H, frame_si); /* avoid over range */

    /* fill osd attr */
    osd[i].rect.x = x_rect;
    osd[i].rect.y = y_rect;
    osd[i].rect.width = SAMPLE_SVP_VGS_OSD_TEXT_FRAME_W;
    osd[i].rect.height = SAMPLE_SVP_VGS_OSD_TEXT_FRAME_H;
    osd[i].bg_color = SAMPLE_SVP_VGS_OSD_BG_COLOR;
    osd[i].pixel_format = OT_PIXEL_FORMAT_ARGB_1555;
    osd[i].phys_addr = canvas_info.phys_addr;
    osd[i].stride = canvas_info.stride;
    osd[i].fg_alpha = SAMPLE_SVP_VGS_ALPHA_MAX;
    osd[i].bg_alpha = SAMPLE_SVP_VGS_ALPHA_MAX;
    osd[i].osd_inverted_color = OT_VGS_OSD_INVERTED_COLOR_NONE;
    return ret;
}

td_s32 sample_common_svp_vgs_fill_tracker_id(const ot_video_frame_info *frame_info, ot_sample_svp_rect_info *rect,
    td_u32 width, td_u32 height, ot_vgs_osd *osd)
{
    td_s32 ret = TD_SUCCESS;
    sample_svp_check_exps_return(osd == TD_NULL, TD_FAILURE, SAMPLE_SVP_ERR_LEVEL_ERROR, "osd malloc failed!");

    if (sample_common_svp_pre_check_no_rect(rect) == TD_TRUE) { /* No rect num: vgs skip osd task */
        return TD_SUCCESS;
    }

    ret = sample_common_svp_create_all_rgn(); /* rgn resource init */
    sample_svp_check_exps_return(ret != TD_SUCCESS, TD_FAILURE, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "create_all_rgn failed with %#x\n", ret);

    ot_vgs_handle vgs_handle_osd = -1;
    ret = ss_mpi_vgs_begin_job(&vgs_handle_osd); /* osd task init */
    sample_svp_check_exps_goto(ret != TD_SUCCESS, destroy_rgn, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "vgs begin job fail with %#x\n", ret);

    ot_vgs_task_attr vgs_task_osd = {}; /* config osd frame info */
    ret = memcpy_s(&vgs_task_osd.img_in, sizeof(ot_video_frame_info), frame_info, sizeof(ot_video_frame_info));
    sample_svp_check_exps_goto(ret != EOK, cancel_job, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "copy osd img_in task attribute failed");
    ret = memcpy_s(&vgs_task_osd.img_out, sizeof(ot_video_frame_info), frame_info, sizeof(ot_video_frame_info));
    sample_svp_check_exps_goto(ret != EOK, cancel_job, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "copy osd img_out task attribute failed");

    for (td_s32 i = 0; i < rect->num && i < SAMPLE_SVP_MAX_RGN_CNT; i++) { /* config osd other attr */
        ret = sample_common_svp_config_osd(i, osd, rect, width, height);
        sample_svp_check_exps_goto(ret != TD_SUCCESS, cancel_job, SAMPLE_SVP_ERR_LEVEL_ERROR, "ss_mpi_config_osd fail\
            with %#x\n", ret);
    }

    if (rect->num > 0) { /* if exist rect, add osd task */
        ret = ss_mpi_vgs_add_osd_task(vgs_handle_osd, &vgs_task_osd, osd, (rect->num < SAMPLE_SVP_MAX_RGN_CNT) ?
            rect->num : SAMPLE_SVP_MAX_RGN_CNT); /* rely on rgn */
        sample_svp_check_exps_goto(ret != TD_SUCCESS, cancel_job, SAMPLE_SVP_ERR_LEVEL_ERROR,
            "ss_mpi_vgs_add_osd_task fail with %#x\n", ret);
    } else { /* check no rect, cancel job insteadof end job */
        ss_mpi_vgs_cancel_job(vgs_handle_osd);
        sample_common_svp_destroy_all_rgn(SAMPLE_SVP_MAX_RGN_CNT); /* if fail or no rect, osd task cancel */
        return TD_SUCCESS; /* NO RECT SUCCESS */
    }
    ret = ss_mpi_vgs_end_job(vgs_handle_osd); /* if success, osd task end */

cancel_job:
    if (ret != TD_SUCCESS) {
        ss_mpi_vgs_cancel_job(vgs_handle_osd); /* if fail or no rect, osd task cancel */
    }
destroy_rgn:
    sample_common_svp_destroy_all_rgn(SAMPLE_SVP_MAX_RGN_CNT); /* rgn resource release */
    return ret;
}

td_s32 sample_common_svp_vgs_fill_rect(const ot_video_frame_info *frame_info,
    ot_sample_svp_rect_info *rect, td_u32 color)
{
    ot_vgs_handle vgs_handle = -1;
    td_s32 ret = TD_FAILURE;
    td_u16 i;
    ot_vgs_task_attr vgs_task;
    ot_cover vgs_add_cover;

    sample_svp_check_exps_return(frame_info == TD_NULL, ret, SAMPLE_SVP_ERR_LEVEL_ERROR, "frame_info can't be null\n");
    sample_svp_check_exps_return(rect == TD_NULL, ret, SAMPLE_SVP_ERR_LEVEL_ERROR, "rect can't be null\n");
    sample_svp_check_exps_return(rect->num > OT_SVP_RECT_NUM, ret,
        SAMPLE_SVP_ERR_LEVEL_ERROR, "rect->num can't lager than %u\n", OT_SVP_RECT_NUM);
    if (rect->num == 0) {
        return TD_SUCCESS;
    }

    ret = ss_mpi_vgs_begin_job(&vgs_handle);
    sample_svp_check_exps_return(ret != TD_SUCCESS, ret, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Vgs begin job fail,Error(%#x)\n", ret);

    ret = memcpy_s(&vgs_task.img_out, sizeof(ot_video_frame_info), frame_info, sizeof(ot_video_frame_info));
    sample_svp_check_exps_goto(ret != EOK, fail, SAMPLE_SVP_ERR_LEVEL_ERROR, "get img_out failed\n");
    ret = memcpy_s(&vgs_task.img_in, sizeof(ot_video_frame_info), frame_info, sizeof(ot_video_frame_info));
    sample_svp_check_exps_goto(ret != EOK, fail, SAMPLE_SVP_ERR_LEVEL_ERROR, "get img_in failed\n");

    vgs_add_cover.type = OT_COVER_QUAD;
    vgs_add_cover.color = color;
    for (i = 0; i < rect->num; i++) {
        vgs_add_cover.quad_attr.is_solid = TD_FALSE;
        vgs_add_cover.quad_attr.thick = OT_SAMPLE_IVE_DRAW_THICK;
        ret = memcpy_s(vgs_add_cover.quad_attr.point, sizeof(rect->rect[i].point),
            rect->rect[i].point, sizeof(rect->rect[i].point));
        sample_svp_check_exps_goto(ret != EOK, fail, SAMPLE_SVP_ERR_LEVEL_ERROR, "get point failed\n");
        ret = ss_mpi_vgs_add_cover_task(vgs_handle, &vgs_task, &vgs_add_cover, 1);
        sample_svp_check_exps_goto(ret != TD_SUCCESS, fail, SAMPLE_SVP_ERR_LEVEL_ERROR,
            "ss_mpi_vgs_add_cover_task fail, Error(%#x)\n", ret);
    }

    ret = ss_mpi_vgs_end_job(vgs_handle);
    sample_svp_check_exps_goto(ret != TD_SUCCESS, fail, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "ss_mpi_vgs_end_job fail, Error(%#x)\n", ret);

    return ret;
fail:
    ss_mpi_vgs_cancel_job(vgs_handle);
    return ret;
}

/* function : Start Vpss */
static td_s32 sample_common_svp_start_vpss(td_s32 vpss_grp_cnt, ot_size *pic_size, td_u32 vpss_chn_num)
{
    td_u32 i;
    sample_vpss_chn_attr vpss_chn_attr = {0};
    ot_vpss_grp_attr vpss_grp_attr;
    ot_vpss_grp vpss_grp;
    td_s32 ret;

    (td_void)memset_s(&vpss_grp_attr, sizeof(ot_vpss_grp_attr), 0, sizeof(ot_vpss_grp_attr));
    sample_comm_vpss_get_default_grp_attr(&vpss_grp_attr);
    vpss_grp_attr.max_width = pic_size[0].width;
    vpss_grp_attr.max_height = pic_size[0].height;
    /* VPSS only onle channel0 support compress seg mode */
    sample_comm_vpss_get_default_chn_attr(&vpss_chn_attr.chn_attr[0]);
    vpss_chn_attr.chn_attr[0].width = pic_size[0].width;
    vpss_chn_attr.chn_attr[0].height = pic_size[0].height;
    vpss_chn_attr.chn_attr[0].depth = 1;
    vpss_chn_attr.chn_attr[0].compress_mode = OT_COMPRESS_MODE_NONE;

    for (i = 1; i < vpss_chn_num; i++) {
        (td_void)memset_s(&vpss_chn_attr.chn_attr[i], sizeof(ot_vpss_chn_attr), 0, sizeof(ot_vpss_chn_attr));
        sample_comm_vpss_get_default_chn_attr(&vpss_chn_attr.chn_attr[i]);
        vpss_chn_attr.chn_attr[i].width = pic_size[i].width;
        vpss_chn_attr.chn_attr[i].height = pic_size[i].height;
        vpss_chn_attr.chn_attr[i].compress_mode = OT_COMPRESS_MODE_NONE;
        vpss_chn_attr.chn_attr[i].depth = 1;
    }

    vpss_chn_attr.chn_enable[0] = TD_TRUE;
    vpss_chn_attr.chn_enable[1] = TD_TRUE;
    vpss_chn_attr.chn_array_size = OT_VPSS_MAX_CHN_NUM;

    for (vpss_grp = 0; vpss_grp < vpss_grp_cnt; vpss_grp++) {
        ret = sample_common_vpss_start(vpss_grp, &vpss_grp_attr, &vpss_chn_attr);
        if (ret != TD_SUCCESS) {
            sample_svp_trace_err("failed with %#x!\n", ret);
            return TD_FAILURE;
        }
    }
    return TD_SUCCESS;
}

/* function : Start Vpss */
static td_s32 sample_common_svp_start_vpss_multi_frame(td_s32 vpss_grp_cnt, ot_size *pic_size, td_u32 vpss_chn_num)
{
    sample_vpss_chn_attr vpss_chn_attr = {0};
    ot_vpss_grp_attr vpss_grp_attr;
    ot_vpss_grp vpss_grp;
    td_s32 ret;

    (td_void)memset_s(&vpss_grp_attr, sizeof(ot_vpss_grp_attr), 0, sizeof(ot_vpss_grp_attr));
    sample_comm_vpss_get_default_grp_attr(&vpss_grp_attr);

    for (td_u32 i = 0; i < vpss_chn_num; i++) {
        sample_comm_vpss_get_default_chn_attr(&vpss_chn_attr.chn_attr[i]);
        vpss_chn_attr.chn_attr[i].width = pic_size[i].width;
        vpss_chn_attr.chn_attr[i].height = pic_size[i].height;
        vpss_chn_attr.chn_attr[i].depth = 1;
        vpss_chn_attr.chn_attr[i].pixel_format = OT_PIXEL_FORMAT_YUV_SEMIPLANAR_420;
        vpss_chn_attr.chn_attr[i].compress_mode = OT_COMPRESS_MODE_NONE;
        vpss_chn_attr.chn_enable[i] = TD_TRUE;
        vpss_grp_attr.max_width = vpss_grp_attr.max_width > pic_size[i].width ?
            vpss_grp_attr.max_width : pic_size[i].width;
        vpss_grp_attr.max_height = vpss_grp_attr.max_height > pic_size[i].height ?
            vpss_grp_attr.max_height : pic_size[i].height;
    }
    vpss_chn_attr.chn_array_size = OT_VPSS_MAX_CHN_NUM;
    vpss_grp_attr.pixel_format = OT_PIXEL_FORMAT_YUV_SEMIPLANAR_420;

    for (vpss_grp = 0; vpss_grp < vpss_grp_cnt; vpss_grp++) {
        ret = sample_common_vpss_start(vpss_grp, &vpss_grp_attr, &vpss_chn_attr);
        if (ret != TD_SUCCESS) {
            sample_svp_trace_err("failed with %#x!\n", ret);
            return TD_FAILURE;
        }
    }
    return TD_SUCCESS;
}

/* function : Stop Vpss */
static td_void sample_common_svp_stop_vpss(td_s32 vpss_grp_cnt)
{
    ot_vpss_grp vpss_grp = 0;
    td_bool chn_enable[OT_VPSS_MAX_CHN_NUM] = { TD_TRUE, TD_TRUE, TD_FALSE, TD_FALSE };
    td_s32 i;

    for (i = 0; (i < vpss_grp_cnt) && (i < OT_VPSS_MAX_CHN_NUM); i++) {
        sample_common_vpss_stop(vpss_grp, chn_enable, OT_VPSS_MAX_CHN_NUM);
        vpss_grp++;
    }
}

static td_s32 sample_common_svp_vi_bind_multi_vpss(td_s32 vpss_grp_cnt, td_s32 vi_chn_cnt,
    td_s32 vi_chn_interval)
{
    td_s32 ret;
    td_s32 loop;
    ot_vi_chn vi_chn;
    ot_vpss_grp vpss_grp = 0;

    for (loop = 0; loop < vi_chn_cnt  && vpss_grp < vpss_grp_cnt; loop++) {
        vi_chn = loop * vi_chn_interval;
        ret = sample_comm_vi_bind_vpss(0, vi_chn, vpss_grp, 0);
        if (ret != TD_SUCCESS) {
            sample_svp_trace_err("vi bind vpss failed!\n");
            return ret;
        }
        vpss_grp++;
    }

    return TD_SUCCESS;
}

static td_s32 sample_common_svp_vi_unbind_multi_vpss(td_s32 vpss_grp_cnt, td_s32 vi_chn_cnt,
    td_s32 vi_chn_interval)
{
    td_s32 ret;
    td_s32 loop;
    ot_vi_chn vi_chn;
    ot_vpss_grp vpss_grp = 0;

    for (loop = 0; loop < vi_chn_cnt && vpss_grp < vpss_grp_cnt; loop++) {
        vi_chn = loop * vi_chn_interval;
        ret = sample_comm_vi_un_bind_vpss(0, vi_chn, vpss_grp, 0);
        if (ret != TD_SUCCESS) {
            sample_svp_trace_err("vi bind vpss failed!\n");
            return ret;
        }

        vpss_grp++;
    }

    return TD_SUCCESS;
}

static td_s32 sample_common_svp_set_vi_cfg(sample_vi_cfg *vi_cfg, ot_pic_size *pic_type,
    td_u32 pic_type_len, ot_pic_size *ext_pic_size_type, sample_sns_type sns_type)
{
    sample_comm_vi_get_default_vi_cfg(sns_type, vi_cfg);
    sample_svp_check_exps_return(pic_type_len < OT_VPSS_CHN_NUM,
        TD_FAILURE, SAMPLE_SVP_ERR_LEVEL_ERROR, "pic_type_len is illegal!\n");
    pic_type[1] = *ext_pic_size_type;

    return TD_SUCCESS;
}

static td_s32 sample_common_svp_start_venc(const ot_sample_svp_switch *switch_ptr)
{
    td_s32 ret = TD_SUCCESS;
    ot_venc_chn h264_chn = 0;
    sample_comm_venc_chn_param chn_param;
    ot_size ven_size = {1920, 1080};

    chn_param.frame_rate = 30; /* 30 is a number */
    chn_param.stats_time = 2; /* 2 is a number */
    chn_param.gop = 60; /* 60 is a number */
    chn_param.venc_size = ven_size;
    chn_param.size = PIC_1080P;
    chn_param.profile = 0;
    chn_param.is_rcn_ref_share_buf = TD_TRUE;

    chn_param.type = OT_PT_H264;
    chn_param.rc_mode = SAMPLE_RC_CBR;

    if (switch_ptr->is_venc_open  == TD_TRUE) {
        ret = sample_comm_venc_get_gop_attr(OT_VENC_GOP_MODE_NORMAL_P, &chn_param.gop_attr);
        sample_svp_check_exps_goto(ret != TD_SUCCESS, end_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
            "Error(%#x),sample_comm_venc_get_gop_attr failed!\n", ret);

        ret = sample_comm_venc_start(h264_chn, &chn_param);
        sample_svp_check_exps_goto(ret != TD_SUCCESS, end_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
            "Error(%#x),sample_comm_venc_start failed!\n", ret);

        ret = sample_comm_venc_start_get_stream(&h264_chn, 1);
        sample_svp_check_exps_goto(ret != TD_SUCCESS, end_1, SAMPLE_SVP_ERR_LEVEL_ERROR,
            "Error(%#x),sample_comm_venc_start_get_stream failed!\n", ret);
    }
    return ret;

end_1:
    if (switch_ptr->is_venc_open == TD_TRUE) {
        sample_comm_venc_stop(h264_chn);
    }
end_0:
    return ret;
}

static td_s32 sample_common_svp_get_pic_type_by_sns_type(sample_sns_type sns_type, ot_pic_size size[], td_u32 num)
{
    sample_svp_check_exps_return(num > SAMPLE_SVP_NPU_VPSS_CHN_NUM, TD_FAILURE, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "num(%u) can't be larger than (%u)\n", num, SAMPLE_SVP_NPU_VPSS_CHN_NUM);
    switch (sns_type) {
        case SC4336P_MIPI_4M_30FPS_10BIT:
        case OS04D10_MIPI_4M_30FPS_10BIT:
        case GC4023_MIPI_4M_30FPS_10BIT:
        case SC431HAI_MIPI_4M_30FPS_10BIT:
        case SC431HAI_MIPI_4M_30FPS_10BIT_WDR2TO1:
            size[0] = PIC_2560X1440;
            break;
        case SC450AI_MIPI_4M_30FPS_10BIT:
        case SC450AI_MIPI_4M_30FPS_10BIT_WDR2TO1:
            size[0] = PIC_2688X1520;
            break;
        case SC500AI_MIPI_5M_30FPS_10BIT:
        case SC500AI_MIPI_5M_30FPS_10BIT_WDR2TO1:
            size[0] = PIC_2880X1620;
            break;
        case SC4336P_MIPI_3M_30FPS_10BIT:
            size[0] = PIC_2304X1296;
            break;
        case SC533HAI_MIPI_5M_30FPS_10BIT:
        case SC533HAI_MIPI_5M_30FPS_10BIT_WDR2TO1:
            size[0] = PIC_2880X1620;
            break;
        default:
            size[0] = PIC_1080P;
            break;
     }
    return TD_SUCCESS;
}

/*
 * function : Start Vi/Vpss/Venc/Vo
 */
td_s32 sample_common_svp_start_vi_vpss_venc(sample_vi_cfg *vi_cfg,
    ot_sample_svp_switch *switch_ptr, ot_pic_size *ext_pic_size_type)
{
    ot_size pic_size[OT_VPSS_CHN_NUM];
    ot_pic_size pic_type[OT_VPSS_CHN_NUM];

    const td_s32 vpss_grp_cnt = 1;
    td_s32 ret = TD_FAILURE;
    sample_sns_type sns_type = SENSOR0_TYPE;

    sample_svp_check_exps_return(vi_cfg == TD_NULL, ret, SAMPLE_SVP_ERR_LEVEL_ERROR, "vi_cfg can't be null\n");
    sample_svp_check_exps_return(switch_ptr == TD_NULL, ret, SAMPLE_SVP_ERR_LEVEL_ERROR, "switch_ptr can't be null\n");
    sample_svp_check_exps_return(ext_pic_size_type == TD_NULL, ret,
        SAMPLE_SVP_ERR_LEVEL_ERROR, "ext_pic_size_type can't be null\n");

    ret = sample_common_svp_get_pic_type_by_sns_type(sns_type, pic_type, OT_VPSS_CHN_NUM);
    sample_svp_check_exps_return(ret != TD_SUCCESS, ret, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "sample_common_svp_get_pic_type_by_sns_type failed!\n");
    ret = sample_common_svp_set_vi_cfg(vi_cfg, pic_type, OT_VPSS_CHN_NUM, ext_pic_size_type, sns_type);
    sample_svp_check_exps_return(ret != TD_SUCCESS, ret, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "sample_common_svp_set_vi_cfg failed,Error:%#x\n", ret);

    /* step  1: Init vb */
    ret = sample_common_svp_vb_init(pic_type, pic_size, OT_VPSS_CHN_NUM);
    sample_svp_check_exps_return(ret != TD_SUCCESS, ret, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error(%#x),sample_common_svp_vb_init failed!\n", ret);

    /* step 2: Start vi */
    ret = sample_comm_vi_start_vi(vi_cfg);
    sample_svp_check_exps_goto(ret != TD_SUCCESS, end_init_1, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error(%#x),sample_comm_vi_start_vi failed!\n", ret);

    /* step 3: Bind vpss to vi */
    ret = sample_common_svp_vi_bind_multi_vpss(vpss_grp_cnt, 1, 1);
    sample_svp_check_exps_goto(ret != TD_SUCCESS, end_init_2, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error(%#x),sample_common_vi_bind_multi_vpss failed!\n", ret);

    /* step 4: Start vpss */
    ret = sample_common_svp_start_vpss(vpss_grp_cnt, pic_size, OT_VPSS_CHN_NUM);
    sample_svp_check_exps_goto(ret != TD_SUCCESS, end_init_3, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error(%#x),sample_common_svp_start_vpss failed!\n", ret);

    /* step 5: Start Venc */
    ret = sample_common_svp_start_venc(switch_ptr);
    sample_svp_check_exps_goto(ret != TD_SUCCESS, end_init_4, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error(%#x),sample_common_svp_start_vencb failed!\n", ret);

    return TD_SUCCESS;
end_init_4:
    sample_common_svp_stop_vpss(vpss_grp_cnt);
end_init_3:
    ret = sample_common_svp_vi_unbind_multi_vpss(vpss_grp_cnt, 1, 1);
    sample_svp_check_exps_trace(ret != TD_SUCCESS, SAMPLE_SVP_ERR_LEVEL_ERROR, "svp_vi_unbind_multi_vpss failed!\n");
end_init_2:
    sample_comm_vi_stop_vi(vi_cfg);
end_init_1:  /*  system exit */
    sample_comm_sys_exit();
    (td_void)memset_s(vi_cfg, sizeof(sample_vi_cfg), 0, sizeof(sample_vi_cfg));
    return ret;
}

/*
 * function : Stop Vi/Vpss/Venc
 */
td_void sample_common_svp_stop_vi_vpss_venc(sample_vi_cfg *vi_cfg,
    ot_sample_svp_switch *switch_ptr)
{
    td_s32 ret;
    const td_s32 vpss_grp_cnt = 1;

    sample_svp_check_exps_return_void(vi_cfg == TD_NULL, SAMPLE_SVP_ERR_LEVEL_ERROR, "vi_cfg can't be null\n");
    sample_svp_check_exps_return_void(switch_ptr == TD_NULL, SAMPLE_SVP_ERR_LEVEL_ERROR, "switch_ptr can't be null\n");

    if (switch_ptr->is_venc_open == TD_TRUE) {
        sample_comm_venc_stop_get_stream(1);
        sample_comm_venc_stop(0);
    }

    ret = sample_common_svp_vi_unbind_multi_vpss(vpss_grp_cnt, 1, 1);
    sample_svp_check_exps_trace(ret != TD_SUCCESS, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "sample_common_svp_vi_unbind_multi_vpss failed\n");
    sample_common_svp_stop_vpss(vpss_grp_cnt);
    sample_comm_vi_stop_vi(vi_cfg);
    sample_comm_sys_exit();

    (td_void)memset_s(vi_cfg, sizeof(sample_vi_cfg), 0, sizeof(sample_vi_cfg));
}

static td_s32 sample_common_svp_create_vb(ot_sample_svp_media_cfg *media_cfg)
{
    td_s32 ret = OT_INVALID_VALUE;
    td_u32 i, j;
    td_u64 blk_size;
    ot_vb_pool_cfg vb_pool_cfg = { 0 };
    ot_pic_buf_attr pic_buf_attr = {0};
    td_u32 size = 640 * 480 / 2; /* 640, 480, 2: algn */

    sample_svp_check_exps_return(media_cfg == TD_NULL, ret, SAMPLE_SVP_ERR_LEVEL_ERROR, "media_cfg is NULL!\n");

    sample_svp_check_exps_return((media_cfg->chn_num == 0) || (media_cfg->chn_num > OT_SVP_MAX_VPSS_CHN_NUM), ret,
        SAMPLE_SVP_ERR_LEVEL_ERROR, "media_cfg->chn_num must be [1, %d],Error(%#x)!\n", OT_SVP_MAX_VPSS_CHN_NUM, ret);
    {
        vb_pool_cfg.blk_cnt = 3; /* 3: algn */
        vb_pool_cfg.blk_size = size;
        vb_pool_cfg.remap_mode = OT_VB_REMAP_MODE_NOCACHE;
        media_cfg->mask_vb_pool = ss_mpi_vb_create_pool(&vb_pool_cfg);
        if (media_cfg->mask_vb_pool == OT_VB_INVALID_POOL_ID) {
            sample_svp_check_exps_return(media_cfg->mask_vb_pool == OT_VB_INVALID_POOL_ID, 0,
                SAMPLE_SVP_ERR_LEVEL_ERROR, "Error(%#x),ss_mpi_vb_create_pool failed!\n", 0);
            return TD_SUCCESS;
        }
    }
    for (i = 1; i < media_cfg->chn_num; i++) {
        if (media_cfg->pic_type[i] != PIC_BUTT) {
            ret = sample_comm_sys_get_pic_size(media_cfg->pic_type[i], &media_cfg->pic_size[i]);
            sample_svp_check_exps_goto(ret != TD_SUCCESS, vb_fail_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
                "sample_comm_sys_get_pic_size failed,Error(%#x)!\n", ret);
        }
        pic_buf_attr.width = media_cfg->pic_size[i].width;
        pic_buf_attr.height = media_cfg->pic_size[i].height;
        pic_buf_attr.compress_mode = OT_COMPRESS_MODE_NONE;
        pic_buf_attr.align = OT_DEFAULT_ALIGN;
        pic_buf_attr.bit_width = OT_DATA_BIT_WIDTH_8;
        pic_buf_attr.pixel_format = OT_PIXEL_FORMAT_YVU_SEMIPLANAR_422;
        pic_buf_attr.video_format = OT_VIDEO_FORMAT_LINEAR;
        blk_size = ot_common_get_pic_buf_size(&pic_buf_attr);
        vb_pool_cfg.blk_size = blk_size;
        vb_pool_cfg.blk_cnt = SAMPLE_SVP_BLK_CNT;
        media_cfg->vb_pool[i] = ss_mpi_vb_create_pool(&vb_pool_cfg);

        (td_void)ss_mpi_vb_pool_share_all(media_cfg->vb_pool[i]);
        sample_svp_check_exps_goto(media_cfg->vb_pool[i] == OT_VB_INVALID_POOL_ID, vb_fail_0,
            SAMPLE_SVP_ERR_LEVEL_ERROR, "create %u-th vb pool failed!\n", i);
    }
    return TD_SUCCESS;
vb_fail_0:
    for (j = 0; j < i; j++) {
        (td_void)ss_mpi_vb_destroy_pool(media_cfg->vb_pool[j]);
    }
    (td_void)ss_mpi_vb_destroy_pool(media_cfg->mask_vb_pool);
    return TD_FAILURE;
}

static td_void sample_common_svp_destroy_vb(ot_sample_svp_media_cfg *media_cfg)
{
    td_u32 i;
    for (i = 1; i < media_cfg->chn_num; i++) {
        (td_void)ss_mpi_vb_destroy_pool(media_cfg->vb_pool[i]);
    }
    (td_void)ss_mpi_vb_destroy_pool(media_cfg->mask_vb_pool);
}
static td_s32 sample_common_svp_vpss_attach_vb(ot_sample_svp_media_cfg *media_cfg, td_s32 vpss_grp_cnt)
{
    ot_vpss_grp grp, grp_tmp;
    ot_vpss_chn chn, chn_tmp;
    td_s32 ret = TD_SUCCESS;

    for (grp = 0; grp < vpss_grp_cnt; grp++) {
        for (chn = 1; chn < (td_s32)media_cfg->chn_num; chn++) {
            (td_void)ss_mpi_vpss_set_chn_vb_src(grp, chn, OT_VB_SRC_USER);
            ret = ss_mpi_vpss_attach_chn_vb_pool(grp, chn, media_cfg->vb_pool[chn]);
            if (ret == TD_SUCCESS) {
                continue;
            }
            for (chn_tmp = 1; chn_tmp < chn; chn_tmp++) {
                (td_void)ss_mpi_vpss_detach_chn_vb_pool(grp, chn_tmp);
            }
            sample_svp_check_exps_goto(ret != TD_SUCCESS, end_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
                "Error(%#x), ss_mpi_vpss_attach_chn_vb_pool failed!\n", ret);
        }
    }
    return ret;
end_0:
    for (grp_tmp = 0; grp_tmp < grp; grp_tmp++) {
        for (chn_tmp = 1; chn_tmp < (td_s32)media_cfg->chn_num; chn_tmp++) {
            (td_void)ss_mpi_vpss_detach_chn_vb_pool(grp_tmp, chn_tmp);
        }
    }
    return ret;
}

static td_void sample_common_svp_vpss_detach_vb(ot_sample_svp_media_cfg *media_cfg, td_s32 vpss_grp_cnt)
{
    ot_vpss_grp grp;
    ot_vpss_chn chn;

    for (grp = 0; grp < vpss_grp_cnt; grp++) {
        for (chn = 1; chn < (td_s32)media_cfg->chn_num; chn++) {
            (td_void)ss_mpi_vpss_detach_chn_vb_pool(grp, chn);
        }
    }
}

td_s32 sample_common_svp_venc_send_stream(const ot_sample_svp_switch *vo_venc_switch, ot_venc_chn venc_chn,
    const ot_video_frame_info *frame)
{
    td_s32 ret = OT_INVALID_VALUE;

    sample_svp_check_exps_return((vo_venc_switch == TD_NULL || frame == TD_NULL), ret, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "vo_venc_switch is NULL!\n");
    if (vo_venc_switch->is_venc_open) {
        ret = ss_mpi_venc_send_frame(venc_chn, frame, OT_SVP_TIMEOUT);
        sample_svp_check_exps_return(ret != TD_SUCCESS, ret, SAMPLE_SVP_ERR_LEVEL_ERROR,
            "Error(%#x),ss_mpi_venc_send_frame failed!\n", ret);
    }
    return TD_SUCCESS;
}

static td_s32 sample_common_svp_npu_get_pic_size(ot_sample_svp_media_cfg *media_cfg, td_u64 *blk_size)
{
    td_s32 ret;
    ot_pic_buf_attr pic_buf_attr = {0};

    ret = sample_comm_sys_get_pic_size(media_cfg->pic_type[0], &media_cfg->pic_size[0]);
    sample_svp_check_exps_return(ret != TD_SUCCESS, ret, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "sample_comm_sys_get_pic_size failed,Error(%#x)!\n", ret);
    pic_buf_attr.width = media_cfg->pic_size[0].width;
    pic_buf_attr.height = media_cfg->pic_size[0].height;
    pic_buf_attr.compress_mode = OT_COMPRESS_MODE_NONE;
    pic_buf_attr.align = OT_DEFAULT_ALIGN;
    pic_buf_attr.bit_width = OT_DATA_BIT_WIDTH_8;
    pic_buf_attr.pixel_format = OT_PIXEL_FORMAT_YVU_SEMIPLANAR_422;
    pic_buf_attr.video_format = OT_VIDEO_FORMAT_LINEAR;
    *blk_size = ot_common_get_pic_buf_size(&pic_buf_attr);
    return TD_SUCCESS;
}

static td_s32 sample_common_svp_npu_reset_vb(ot_sample_svp_media_cfg *media_cfg, td_u64 *blk_size,
    ot_vb_cfg *vb_default_cfg)
{
    td_s32 ret;
    ret = sample_comm_svp_sys_exit();
    sample_svp_check_exps_return(ret != TD_SUCCESS, ret, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error(%#x):sample_comm_svp_sys_exit failed!\n", ret);
    ret = sample_common_svp_npu_get_pic_size(media_cfg, blk_size);
    sample_svp_check_exps_return(ret != TD_SUCCESS, ret, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error(%#x):sample_common_svp_npu_get_pic_size failed!\n", ret);
    vb_default_cfg->max_pool_cnt = SAMPLE_SVP_VB_POOL_NUM;
    vb_default_cfg->common_pool[0].blk_cnt = SAMPLE_SVP_BLK_CNT;
    vb_default_cfg->common_pool[0].blk_size = *blk_size;
    ret = ss_mpi_vb_set_cfg(vb_default_cfg);
    sample_svp_check_exps_return(ret != TD_SUCCESS, ret, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error(%#x):ss_mpi_vb_set_cfg failed!\n", ret);
    ret = ss_mpi_vb_init();
    sample_svp_check_exps_return(ret != TD_SUCCESS, ret, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error(%#x):ss_mpi_vb_init failed!\n", ret);
    ret = ss_mpi_sys_init();
    sample_svp_check_exps_return(ret != TD_SUCCESS, ret, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error(%#x):ss_mpi_sys_init failed!\n", ret);
    return TD_SUCCESS;
}

td_s32 sample_common_svp_npu_reset_vb_multi_frame(ot_sample_svp_media_cfg *media_cfg, td_u64 *blk_size,
    ot_vb_cfg *vb_default_cfg)
{
    td_s32 ret;
    ret = sample_comm_svp_sys_exit();
    sample_svp_check_exps_return(ret != TD_SUCCESS, ret, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error(%#x):sample_comm_svp_sys_exit failed!\n", ret);

    for (int i = 0; i < OT_VB_MAX_COMMON_POOLS; i++) {
        vb_default_cfg->common_pool[i].blk_cnt = 0;
        vb_default_cfg->common_pool[i].blk_size = 0;
    }

    ot_pic_buf_attr pic_buf_attr;
    ot_vb_calc_cfg calc_cfg;

    pic_buf_attr.bit_width = OT_DATA_BIT_WIDTH_8;
    pic_buf_attr.pixel_format = OT_PIXEL_FORMAT_YVU_SEMIPLANAR_422;
    pic_buf_attr.compress_mode = OT_COMPRESS_MODE_NONE;
    pic_buf_attr.video_format = OT_VIDEO_FORMAT_LINEAR;

    for (td_u32 i = 0; (i < media_cfg->chn_num) && (i < OT_VB_MAX_COMMON_POOLS); i++) {
        if (media_cfg->pic_type[i] != PIC_BUTT) {
            ret = sample_comm_sys_get_pic_size(media_cfg->pic_type[i], &media_cfg->pic_size[i]);
            sample_svp_check_exps_return(ret != TD_SUCCESS, ret, SAMPLE_SVP_ERR_LEVEL_ERROR,
                "Error(%#x):sample_comm_sys_get_pic_size failed!\n", ret);
        }
        pic_buf_attr.width = media_cfg->pic_size[i].width;
        pic_buf_attr.height = media_cfg->pic_size[i].height;
        pic_buf_attr.compress_mode = OT_COMPRESS_MODE_NONE;
        pic_buf_attr.align = OT_DEFAULT_ALIGN;

        ot_common_get_pic_buf_cfg(&pic_buf_attr, &calc_cfg);

        /* comm video buffer */
        vb_default_cfg->common_pool[i].blk_size = calc_cfg.vb_size;
        vb_default_cfg->common_pool[i].blk_cnt = SAMPLE_SVP_BLK_CNT;
    }

    ret = ss_mpi_vb_set_cfg(vb_default_cfg);
    sample_svp_check_exps_return(ret != TD_SUCCESS, ret, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error(%#x): ss_mpi_vb_set_cfg failed!\n", ret);
    ret = ss_mpi_vb_init();
    sample_svp_check_exps_return(ret != TD_SUCCESS, ret, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error(%#x): ss_mpi_vb_init failed!\n", ret);
    ret = ss_mpi_sys_init();
    sample_svp_check_exps_return(ret != TD_SUCCESS, ret, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error(%#x): ss_mpi_sys_init failed!\n", ret);
    return TD_SUCCESS;
}

static ot_pic_size sample_region_get_pic_size_by_sns_type(sample_sns_type sns_type)
{
    switch (sns_type) {
        case SC4336P_MIPI_4M_30FPS_10BIT:
        case OS04D10_MIPI_4M_30FPS_10BIT:
        case GC4023_MIPI_4M_30FPS_10BIT:
        case SC431HAI_MIPI_4M_30FPS_10BIT:
        case SC431HAI_MIPI_4M_30FPS_10BIT_WDR2TO1:
            return PIC_2560X1440;
        case SC450AI_MIPI_4M_30FPS_10BIT:
        case SC450AI_MIPI_4M_30FPS_10BIT_WDR2TO1:
            return PIC_2688X1520;
        case SC500AI_MIPI_5M_30FPS_10BIT:
        case SC500AI_MIPI_5M_30FPS_10BIT_WDR2TO1:
            return PIC_2880X1620;
        case SC4336P_MIPI_3M_30FPS_10BIT:
            return PIC_2304X1296;
        case SC533HAI_MIPI_5M_30FPS_10BIT:
        case SC533HAI_MIPI_5M_30FPS_10BIT_WDR2TO1:
            return PIC_2880X1620;
        default:
            return PIC_1080P;
    }
}

static td_s32 sample_common_svp_npu_start_venc(sample_sns_type sns_type, ot_size *pic_size)
{
    td_s32 ret = TD_SUCCESS;
    ot_venc_chn h265_chn = 0;
    sample_comm_venc_chn_param chn_param;

    chn_param.frame_rate = 25; /* 25 is a number */
    chn_param.stats_time = 2; /* 2 is a number */
    chn_param.gop = 50; /* 50 is a number */
    sample_comm_vi_get_size_by_sns_type(sns_type, &chn_param.venc_size);
    chn_param.size = sample_region_get_pic_size_by_sns_type(sns_type);
    chn_param.profile = 0;
    chn_param.is_rcn_ref_share_buf = TD_FALSE;
    chn_param.type = OT_PT_H265;
    chn_param.rc_mode = SAMPLE_RC_CBR;

    ot_venc_chn_config chn_config;
    ret = ss_mpi_venc_get_chn_config(h265_chn, &chn_config);
    sample_svp_check_exps_goto(ret != TD_SUCCESS, end_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error(%#x), ss_mpi_venc_set_chn_config failed!\n", ret);
    chn_config.svc_version = OT_VENC_SVC_V2;
    ret = ss_mpi_venc_set_chn_config(h265_chn, &chn_config);
    sample_svp_check_exps_goto(ret != TD_SUCCESS, end_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error(%#x), ss_mpi_venc_set_chn_config failed!\n", ret);

    ret = sample_comm_venc_get_gop_attr(OT_VENC_GOP_MODE_NORMAL_P, &chn_param.gop_attr);
    sample_svp_check_exps_goto(ret != TD_SUCCESS, end_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error(%#x), sample_comm_venc_get_gop_attr failed!\n", ret);

    ret = sample_comm_venc_start(h265_chn, &chn_param);
    sample_svp_check_exps_goto(ret != TD_SUCCESS, end_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error(%#x), sample_comm_venc_start failed!\n", ret);

    ret = sample_comm_venc_start_get_stream(&h265_chn, 1);
    sample_svp_check_exps_goto(ret != TD_SUCCESS, end_1, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error(%#x), sample_comm_venc_start_get_stream failed!\n", ret);

    return ret;

end_1:
    sample_comm_venc_stop(h265_chn);
end_0:
    return ret;
}

/*
 * function : Start Vi/Vpss
 */
td_s32 sample_common_svp_npu_start_vi_vpss(sample_vi_cfg *vi_cfg, ot_sample_svp_media_cfg *media_cfg)
{
    const td_s32 vpss_grp_cnt = 1;
    sample_sns_type sns_type = SENSOR0_TYPE;
    ot_vb_cfg vb_default_cfg;
    td_u64 blk_size;
    ot_vpss_svc_param vpss_svc_param;
    ot_vpss_grp vpss_grp = 0;

    td_s32 ret = sample_common_svp_get_pic_type_by_sns_type(sns_type, media_cfg->pic_type, OT_SVP_MAX_VPSS_CHN_NUM);
    sample_svp_check_exps_return(ret != TD_SUCCESS, ret, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "sample_common_svp_get_pic_type_by_sns_type failed!\n");
    sample_comm_vi_get_default_vi_cfg(sns_type, vi_cfg);

    if (media_cfg->chn_num > SAMPLE_SVP_SINGLE_FRAME_VPSS_CHN_NUM) { /* 2: one image + one show frame. */
        ret = sample_common_svp_npu_reset_vb_multi_frame(media_cfg, &blk_size, &vb_default_cfg);
    } else {
        ret = sample_common_svp_npu_reset_vb(media_cfg, &blk_size, &vb_default_cfg);
    }
    sample_svp_check_exps_return(ret != TD_SUCCESS, ret, SAMPLE_SVP_ERR_LEVEL_ERROR, "reset vb failed!\n");

    /* step  1: Init vb */
    ret = sample_common_svp_create_vb(media_cfg);
    sample_svp_check_exps_return(ret != TD_SUCCESS, ret, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error(%#x),sample_common_svp_create_vb failed!\n", ret);

    /* step 2: Start vi */
    ret = sample_comm_vi_start_vi(vi_cfg);
    sample_svp_check_exps_goto(ret != TD_SUCCESS, end_init_1, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error(%#x),sample_comm_vi_start_vi failed!\n", ret);

    /* step 3: Bind vpss to vi */
    ret = sample_common_svp_vi_bind_multi_vpss(vpss_grp_cnt, 1, 1);
    sample_svp_check_exps_goto(ret != TD_SUCCESS, end_init_2, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error(%#x),sample_common_vi_bind_multi_vpss failed!\n", ret);

    /* step 4: Start vpss */
    if (media_cfg->chn_num > SAMPLE_SVP_SINGLE_FRAME_VPSS_CHN_NUM) { /* 2: one image + one show frame. */
        ret = sample_common_svp_start_vpss_multi_frame(vpss_grp_cnt, media_cfg->pic_size, media_cfg->chn_num);
    } else {
        ret = sample_common_svp_start_vpss(vpss_grp_cnt, media_cfg->pic_size, media_cfg->chn_num);
    }
    sample_svp_check_exps_goto(ret != TD_SUCCESS, end_init_3, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error(%#x),sample_common_svp_start_vpss failed!\n", ret);

    /* step 5: attach vb */
    ret = sample_common_svp_vpss_attach_vb(media_cfg, vpss_grp_cnt);
    sample_svp_check_exps_goto(ret != TD_SUCCESS, end_init_4, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error(%#x),sample_common_svp_start_vpss failed!\n", ret);

    /* step 6: start VENC */
    ret = sample_common_svp_npu_start_venc(sns_type, media_cfg->pic_size);
    sample_svp_check_exps_goto(ret != TD_SUCCESS, end_init_5, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error(%#x), sample_common_svp_start_vencb failed!\n", ret);
    ret = ss_mpi_vpss_get_grp_svc_param(vpss_grp, &vpss_svc_param);
    sample_svp_check_exps_goto(ret != TD_SUCCESS, end_init_5, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error(%#x), ss_mpi_vpss_get_svc_param failed!\n", ret);
    vpss_svc_param.enable = 1;
    vpss_svc_param.svc_interval = 3; /* 3: algn */
    vpss_svc_param.svc_chn = 1;
    ret = ss_mpi_vpss_set_grp_svc_param(vpss_grp, &vpss_svc_param);
    sample_svp_check_exps_goto(ret != TD_SUCCESS, end_init_5, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error(%#x), ss_mpi_vpss_set_svc_param failed!\n", ret);

    return ret;

end_init_5:
    sample_common_svp_vpss_detach_vb(media_cfg, vpss_grp_cnt);
end_init_4:
    sample_common_svp_stop_vpss(vpss_grp_cnt);
end_init_3:
    sample_common_svp_vi_unbind_multi_vpss(vpss_grp_cnt, 1, 1);
end_init_2:
    sample_comm_vi_stop_vi(vi_cfg);
end_init_1:
    sample_common_svp_destroy_vb(media_cfg);
    return ret;
}

td_void sample_common_svp_destroy_vb_stop_vi_vpss(sample_vi_cfg *vi_cfg, ot_sample_svp_media_cfg *media_cfg)
{
    const td_s32 vpss_grp_cnt = 1;

    sample_svp_check_exps_return_void(media_cfg == TD_NULL, SAMPLE_SVP_ERR_LEVEL_ERROR, "media_cfg is NULL!\n");

    sample_svp_check_exps_return_void((media_cfg->chn_num == 0) || (media_cfg->chn_num > OT_SVP_MAX_VPSS_CHN_NUM),
        SAMPLE_SVP_ERR_LEVEL_ERROR, "media_cfg->chn_num must be [1, %u]!\n", OT_SVP_MAX_VPSS_CHN_NUM);

    sample_comm_venc_stop_get_stream(0);
    sample_comm_venc_stop(0);
    sample_common_svp_vpss_detach_vb(media_cfg, vpss_grp_cnt);
    sample_common_svp_stop_vpss(vpss_grp_cnt);
    sample_common_svp_vi_unbind_multi_vpss(vpss_grp_cnt, 1, 1);
    sample_comm_vi_stop_vi(vi_cfg);
    sample_common_svp_destroy_vb(media_cfg);
}
