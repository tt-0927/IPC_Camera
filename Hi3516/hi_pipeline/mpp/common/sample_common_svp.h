/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#ifndef SAMPLE_COMMON_SVP_H
#define SAMPLE_COMMON_SVP_H

#include <stdio.h>
#include "ss_mpi_sys.h"
#include "ot_common.h"
#include "ot_common_svp.h"
#include "sample_comm.h"
#include "ot_common_region.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#define OT_SVP_RECT_NUM                         64
#define OT_SVP_COORDS_NUM                       64
#define OT_COORDS_POINT_NUM                     2
#define OT_POINT_NUM                            4
#define OT_SVP_MAX_VPSS_CHN_NUM                 2
#define OT_SVP_TIMEOUT                          2000
#define OT_SVP_NOTIFY_BOX_SCALE                 30

typedef enum {
    SAMPLE_SVP_ERR_LEVEL_DEBUG   = 0x0,    /* debug-level                                  */
    SAMPLE_SVP_ERR_LEVEL_INFO    = 0x1,    /* informational                                */
    SAMPLE_SVP_ERR_LEVEL_NOTICE  = 0x2,    /* normal but significant condition             */
    SAMPLE_SVP_ERR_LEVEL_WARNING = 0x3,    /* warning conditions                           */
    SAMPLE_SVP_ERR_LEVEL_ERROR   = 0x4,    /* error conditions                             */
    SAMPLE_SVP_ERR_LEVEL_CRIT    = 0x5,    /* critical conditions                          */
    SAMPLE_SVP_ERR_LEVEL_ALERT   = 0x6,    /* action must be taken immediately             */
    SAMPLE_SVP_ERR_LEVEL_FATAL   = 0x7,    /* just for compatibility with previous version */

    SAMPLE_SVP_ERR_LEVEL_BUTT
} sample_svp_err_level;

typedef struct {
    ot_point point[OT_POINT_NUM];
} ot_sample_svp_rect;

typedef struct {
    td_u16 num;
    ot_sample_svp_rect rect[OT_SVP_RECT_NUM];
    td_u32 ids[OT_SVP_RECT_NUM];
    td_float sensitivity; /* 灵敏度 */
} ot_sample_svp_rect_info;

typedef struct {
    td_float point[OT_COORDS_POINT_NUM];
} ot_sample_svp_coords;

typedef struct {
    td_u16 num;
    ot_sample_svp_coords coords[OT_SVP_RECT_NUM];
} ot_sample_svp_coords_info;

typedef struct {
    td_bool is_venc_open;
} ot_sample_svp_switch;

typedef struct {
    ot_sample_svp_switch svp_switch;
    ot_size pic_size[OT_SVP_MAX_VPSS_CHN_NUM];
    ot_pic_size pic_type[OT_SVP_MAX_VPSS_CHN_NUM];
    ot_vb_pool vb_pool[OT_SVP_MAX_VPSS_CHN_NUM];
    ot_vb_pool mask_vb_pool;
    td_u32 chn_num;
} ot_sample_svp_media_cfg;

#define sample_svp_printf(level_str, msg, ...) \
do { \
    fprintf(stderr, "[level]:%s,[func]:%s [line]:%d [info]:" msg, level_str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
} while (0)

#define sample_svp_printf_red(level_str, msg, ...) \
do { \
    fprintf(stderr, "\033[0;31m [level]:%s,[func]:%s [line]:%d [info]:" msg"\033[0;39m\n", level_str, __FUNCTION__, \
        __LINE__, ##__VA_ARGS__); \
} while (0)
/* system is unusable   */
#define sample_svp_trace_fatal(msg, ...)   sample_svp_printf_red("Fatal", msg, ##__VA_ARGS__)
/* action must be taken immediately */
#define sample_svp_trace_alert(msg, ...)   sample_svp_printf_red("Alert", msg, ##__VA_ARGS__)
/* critical conditions */
#define sample_svp_trace_critical(msg, ...)    sample_svp_printf_red("Critical", msg, ##__VA_ARGS__)
/* error conditions */
#define sample_svp_trace_err(msg, ...)     sample_svp_printf_red("Error", msg, ##__VA_ARGS__)
/* warning conditions */
#define sample_svp_trace_warning(msg, ...)    sample_svp_printf("Warning", msg, ##__VA_ARGS__)
/* normal but significant condition  */
#define sample_svp_trace_notic(msg, ...)  sample_svp_printf("Notice", msg, ##__VA_ARGS__)
/* informational */
#define sample_svp_trace_info(msg, ...)    sample_svp_printf("Info", msg, ##__VA_ARGS__)
/* debug-level messages  */
#define sample_svp_trace_debug(msg, ...)  sample_svp_printf("Debug", msg, ##__VA_ARGS__)

/* exps is true, goto */
#define sample_svp_check_exps_goto(exps, label, level, msg, ...)                  \
do {                                                                              \
    if ((exps)) {                                                                 \
        sample_svp_trace_err(msg, ## __VA_ARGS__);                                \
        goto label;                                                               \
    }                                                                             \
} while (0)
/* exps is true, return td_void */
#define sample_svp_check_exps_return_void(exps, level, msg, ...)                 \
do {                                                                             \
    if ((exps)) {                                                                \
        sample_svp_trace_err(msg, ##__VA_ARGS__);                                \
        return;                                                                  \
    }                                                                            \
} while (0)
/* exps is true, return ret */
#define sample_svp_check_exps_return(exps, ret, level, msg, ...)                 \
do {                                                                             \
    if ((exps)) {                                                                \
        sample_svp_trace_err(msg, ##__VA_ARGS__);                                \
        return (ret);                                                            \
    }                                                                            \
} while (0)                                                                      \
/* exps is true, trace */
#define sample_svp_check_exps_trace(exps, level, msg, ...)                       \
do {                                                                             \
    if ((exps)) {                                                                \
        sample_svp_trace_err(msg, ##__VA_ARGS__);                                \
    }                                                                            \
} while (0)

#define sample_svp_check_exps_continue(exps, level, msg, ...)                    \
do {                                                                             \
    if ((exps)) {                                                                \
        sample_svp_trace_err(msg, ##__VA_ARGS__);                                \
        continue;                                                                \
    }                                                                            \
} while (0)                                                                      \

/* exps is not success, trace */
#define sample_svp_check_failed_trace(exps, level, msg, ...)                     \
do {                                                                             \
    if ((exps) != TD_SUCCESS) {                                                  \
        sample_svp_trace_err(msg, ##__VA_ARGS__);                                \
    }                                                                            \
} while (0)

/* exps is not success, goto */
#define sample_svp_check_failed_goto(exps, label, level, msg, ...)                \
do {                                                                              \
    if ((exps) != TD_SUCCESS) {                                                   \
        sample_svp_trace_err(msg, ## __VA_ARGS__);                                \
        goto label;                                                               \
    }                                                                             \
} while (0)                                                                       \

/* exps is not sucecss, return */
#define sample_svp_check_failed_return(exps, ret, level, msg, ...)               \
do {                                                                             \
    if ((exps) != TD_SUCCESS) {                                                  \
        sample_svp_trace_err(msg, ##__VA_ARGS__);                                \
        return (ret);                                                            \
    }                                                                            \
} while (0)

/* exps is not equal to success, goto with level SAMPLE_SVP_ERR_LEVEL_ERROR */
#define sample_svp_check_failed_err_level_goto(exps, label, msg, ...)             \
do {                                                                              \
    if ((exps) != TD_SUCCESS) {                                                   \
        sample_svp_trace_err(msg, ## __VA_ARGS__);                                \
        goto label;                                                               \
    }                                                                             \
} while (0)

/* exps is not equal to success, return with level SAMPLE_SVP_ERR_LEVEL_ERROR */
#define sample_svp_check_failed_err_level_return(exps, ret, msg, ...)             \
do {                                                                              \
    if ((exps) != TD_SUCCESS) {                                                   \
        sample_svp_trace_err(msg, ## __VA_ARGS__);                                \
        return (ret);                                                             \
    }                                                                             \
} while (0)

/* exps is not equal success, trace with level SAMPLE_SVP_ERR_LEVEL_ERROR */
#define sample_svp_check_failed_err_level_trace(exps, msg, ...)                  \
do {                                                                             \
    if ((exps) != TD_SUCCESS) {                                                  \
        sample_svp_trace_err(msg, ##__VA_ARGS__);                                \
    }                                                                            \
} while (0)

#define sample_svp_pause()                                                      \
do {                                                                            \
    printf("---------------press Enter key to exit!---------------\n");         \
    (void)getchar();                                                            \
} while (0)

#define SAMPLE_SVP_VB_POOL_NUM     1
#define SAMPLE_SVP_ALIGN_16        16
#define SAMPLE_SVP_ALIGN_32        32
#define SAMPLE_SVP_D1_PAL_HEIGHT   576
#define SAMPLE_SVP_D1_PAL_WIDTH    704
#define sample_svp_convert_addr_to_ptr(type, addr) ((type *)(td_uintptr_t)(addr))
#define sample_svp_convert_ptr_to_addr(type, addr) ((type)(td_uintptr_t)(addr))

/* free mmz */
#define sample_svp_mmz_free(phys, virt)                                                 \
do {                                                                                    \
    if (((phys) != 0) && ((virt) != 0)) {                                               \
        ss_mpi_sys_mmz_free((td_phys_addr_t)(phys), (td_void*)(td_uintptr_t)(virt));    \
        (phys) = 0;                                                                     \
        (virt) = 0;                                                                     \
    }                                                                                   \
} while (0)

#define sample_svp_close_file(fp)   \
do {                                \
    if ((fp) != TD_NULL) {          \
        fclose((fp));               \
        (fp) = TD_NULL;             \
    }                               \
} while (0)

/* System init */
td_s32 sample_common_svp_check_sys_init(td_void);

/* System exit */
td_void sample_common_svp_check_sys_exit(td_void);

/* Align */
td_u32 sample_common_svp_align(td_u32 size, td_u16 align);

/* Create mem info */
td_s32 sample_common_svp_create_mem_info(ot_svp_mem_info *mem_info, td_u32 size, td_u32 addr_offset);

/* Destory mem info */
td_void sample_common_svp_destroy_mem_info(ot_svp_mem_info *mem_info, td_u32 addr_offset);

/* Malloc memory */
td_s32 sample_common_svp_malloc_mem(td_char *mmb, td_char *zone, td_phys_addr_t *phys_addr,
    td_void **virt_addr, td_u32 size);

/* Malloc memory with cached */
td_s32 sample_common_svp_malloc_cached(td_char *mmb, td_char *zone, td_phys_addr_t *phys_addr,
    td_void **virt_addr, td_u32 size);

/* Fulsh cached */
td_s32 sample_common_svp_flush_cache(td_phys_addr_t phys_addr, td_void *virt_addr, td_u32 size);

/* function : Call vgs to fill rect */
td_s32 sample_common_svp_vgs_fill_rect(const ot_video_frame_info *frame_info,
    ot_sample_svp_rect_info *rect, td_u32 color);

/* function : Draw id of rect */
td_s32 sample_common_svp_vgs_fill_tracker_id(const ot_video_frame_info *frame_info, ot_sample_svp_rect_info *rect,
    td_u32 width, td_u32 height, ot_vgs_osd *osd);

/* function : Start Vi/Vpss/Venc */
td_s32 sample_common_svp_start_vi_vpss_venc(sample_vi_cfg *vi_cfg,
    ot_sample_svp_switch *switch_ptr, ot_pic_size *ext_pic_size_type);

/* function : Stop Vi/Vpss/Venc */
td_void sample_common_svp_stop_vi_vpss_venc(sample_vi_cfg *vi_cfg,
    ot_sample_svp_switch *switch_ptr);

/*
 * Start Vpss
 */
td_s32 sample_common_svp_create_vb_start_vpss(ot_sample_svp_media_cfg *media_cfg);
/*
 * Stop Vpss
 */
td_void sample_common_svp_destroy_vb_stop_vpss(ot_sample_svp_media_cfg *media_cfg);
/*
 * Send stream to venc
 */
td_s32 sample_common_svp_venc_send_stream(const ot_sample_svp_switch *vo_venc_switch,
    ot_venc_chn venc_chn, const ot_video_frame_info *frame);

td_s32 sample_common_svp_npu_start_vi_vpss(sample_vi_cfg *vi_cfg, ot_sample_svp_media_cfg *media_cfg);

td_void sample_common_svp_destroy_vb_stop_vi_vpss(sample_vi_cfg *vi_cfg, ot_sample_svp_media_cfg *media_cfg);
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */
#endif /* SAMPLE_COMMON_SVP_H */
