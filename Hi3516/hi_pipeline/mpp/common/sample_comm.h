/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#ifndef SAMPLE_COMM_H
#define SAMPLE_COMM_H

#include <pthread.h>

#include "ot_common.h"
#include "ot_math.h"
#include "ot_buffer.h"
#include "ot_defines.h"
#include "securec.h"
#include "ot_mipi_rx.h"
#include "ot_common_sys.h"
#include "ot_common_vb.h"
#include "ot_common_isp.h"
#include "ot_common_vi.h"
#include "ot_common_venc.h"
#include "ot_common_vpss.h"
#include "ot_common_region.h"
#include "ot_common_adec.h"
#include "ot_common_aenc.h"
#include "ot_common_aio.h"
#include "ot_common_vgs.h"
#include "ot_common_aibnr.h"

#include "ss_mpi_sys.h"
#include "ss_mpi_sys_bind.h"
#include "ss_mpi_sys_mem.h"
#include "ss_mpi_vb.h"
#include "ss_mpi_vi.h"
#include "ss_mpi_isp.h"
#include "ss_mpi_venc.h"
#include "ss_mpi_vpss.h"
#include "ss_mpi_region.h"
#include "ss_mpi_audio.h"
#include "ss_mpi_vgs.h"
#include "ot_sns_ctrl.h"
#include "ss_mpi_aibnr.h"

#ifdef __cplusplus
extern "C" {
#endif /* end of #ifdef __cplusplus */

#ifdef SENSOR_SC500AI
#define SENSOR0_TYPE SC500AI_MIPI_5M_30FPS_10BIT
#define SC500AI_MIPI_5M_30FPS_10BIT_SELECT 1
#elif defined(SENSOR_SC533HAI)
#define SENSOR0_TYPE SC533HAI_MIPI_5M_30FPS_10BIT
#define SC533HAI_MIPI_5M_30FPS_10BIT_SELECT 1
#else
#define SENSOR0_TYPE SC500AI_MIPI_5M_30FPS_10BIT
#define SC500AI_MIPI_5M_30FPS_10BIT_SELECT 1
#endif

/* macro define */
#define FILE_NAME_LEN 128
#define FILE_PATH_LEN 128

#define SAMPLE_PIXEL_FORMAT OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420

#define COLOR_RGB_RED      0xFF0000
#define COLOR_RGB_GREEN    0x00FF00
#define COLOR_RGB_BLUE     0x0000FF
#define COLOR_RGB_BLACK    0x000000
#define COLOR_RGB_YELLOW   0xFFFF00
#define COLOR_RGB_CYN      0x00ffff
#define COLOR_RGB_WHITE    0xffffff

#define SAMPLE_RGN_HANDLE_NUM_MAX 16
#define SAMPLE_RGN_HANDLE_NUM_MIN 1

#define SAMPLE_AUDIO_EXTERN_AI_DEV 0
#define SAMPLE_AUDIO_EXTERN_AO_DEV 0
#define SAMPLE_AUDIO_INNER_AI_DEV 0
#define SAMPLE_AUDIO_INNER_AO_DEV 0

#define SAMPLE_AUDIO_POINT_NUM_PER_FRAME 480
#define SAMPLE_AUDIO_AI_USER_FRAME_DEPTH 5

#define WDR_MAX_PIPE_NUM 4

#define CHN_NUM_PRE_DEV            4
#define SECOND_CHN_OFFSET_2MUX     2

#define D1_WIDTH            720
#define D1_HEIGHT_PAL       576
#define D1_HEIGHT_NTSC      480

#define _960H_WIDTH         960
#define _960H_HEIGHT_PAL    576
#define _960H_HEIGHT_NTSC   480

#define HD_WIDTH            1280
#define HD_HEIGHT           720

#define FHD_WIDTH           1920
#define FHD_HEIGHT          1080

#define _4K_WIDTH           3840
#define _4K_HEIGHT          2160

#define WIDTH_2688          2688
#define WIDTH_2592          2592
#define HEIGHT_1520         1520

#define WIDTH_2560          2560
#define HEIGHT_1440         1440

#define WIDTH_2880          2880
#define HEIGHT_1620         1620

#define WIDTH_2304          2304
#define HEIGHT_1296         1296

#define SAMPLE_VIE_MAX_ROUTE_NUM 4
#define SAMPLE_VIE_POOL_NUM 3

#define ACODEC_FILE "/dev/acodec"

#define ES8388_FILE "/dev/es8388"
#define ES8388_CHIP_ID 0

#define SAMPLE_FRAME_BUF_RATIO_MAX 100
#define SAMPLE_FRAME_BUF_RATIO_MIN 70
#define SAMPLE_FRAME_BUF_RATIO_DEFAULT 75

#define minor_chn(vi_chn) ((vi_chn) + 1)

#define sample_pause() \
    do { \
        printf("---------------press enter key to exit!---------------\n"); \
        getchar(); \
    } while (0)

#define sample_print(fmt...) \
    do { \
        printf("[%s]-%d: ", __FUNCTION__, __LINE__); \
        printf(fmt); \
    } while (0)

#define sample_print_pause(fmt...) \
    do { \
        printf(fmt); \
        getchar(); \
    } while (0)

#define check_null_ptr_return(ptr) \
    do { \
        if ((ptr) == TD_NULL) { \
            printf("func:%s,line:%d, NULL pointer\n", __FUNCTION__, __LINE__); \
            return TD_FAILURE; \
        } \
    } while (0)

#define check_chn_return(express, chn, name) \
    do { \
        td_s32 ret_ = (express); \
        if (ret_ != TD_SUCCESS) { \
            printf("\033[0;31m%s chn %d failed at %s: LINE: %d with %#x!\033[0;39m\n", \
                   (name), (chn), __FUNCTION__, __LINE__, ret_); \
            fflush(stdout); \
            return ret_; \
        } \
    } while (0)

#define check_return(express, name) \
    do { \
        td_s32 ret_ = (express); \
        if (ret_ != TD_SUCCESS) { \
            printf("\033[0;31m%s failed at %s: LINE: %d with %#x!\033[0;39m\n", \
                   (name), __FUNCTION__, __LINE__, ret_); \
            return ret_; \
        } \
    } while (0)

#define sample_check_eok_return(ret, err_code) \
    do { \
        if ((ret) != EOK) { \
            printf("%s:%d:strncpy_s failed.\n", __FUNCTION__, __LINE__); \
            return (err_code); \
        } \
    } while (0)

#define rgn_check_handle_num_return(handle_num) \
    do { \
        if (((handle_num) < SAMPLE_RGN_HANDLE_NUM_MIN) || ((handle_num) > SAMPLE_RGN_HANDLE_NUM_MAX)) { \
            sample_print("handle_num(%d) should be in [%d, %d].\n", \
                (handle_num), SAMPLE_RGN_HANDLE_NUM_MIN, SAMPLE_RGN_HANDLE_NUM_MAX); \
            return TD_FAILURE; \
        } \
    } while (0)

#define check_digit(x) ((x) >= '0' && (x) <= '9')

/* structure define */
typedef enum {
    PIC_CIF,
    PIC_360P,    /* 640 * 360 */
    PIC_D1_PAL,  /* 720 * 576 */
    PIC_D1_NTSC, /* 720 * 480 */
    PIC_960H,      /* 960 * 576 */
    PIC_720P,    /* 1280 * 720 */
    PIC_1080P,   /* 1920 * 1080 */
    PIC_480P,
    PIC_576P,
    PIC_800X600,
    PIC_1024X768,
    PIC_1280X1024,
    PIC_1366X768,
    PIC_1440X900,
    PIC_1280X800,
    PIC_1600X1200,
    PIC_1680X1050,
    PIC_1920X1200,
    PIC_640X480,
    PIC_1920X2160,
    PIC_2304X1296,
    PIC_2560X1440,
    PIC_2560X1600,
    PIC_2592X1520,
    PIC_2688X1520,
    PIC_2592X1944,
    PIC_3840X2160,
    PIC_4096X2160,
    PIC_3000X3000,
    PIC_4000X3000,
    PIC_6080X2800,
    PIC_7680X4320,
    PIC_3840X8640,
    PIC_2880X1620,
    PIC_BUTT
} ot_pic_size;

typedef enum {
    SC4336P_MIPI_4M_30FPS_10BIT,
    OS04D10_MIPI_4M_30FPS_10BIT,
    GC4023_MIPI_4M_30FPS_10BIT,
    SC431HAI_MIPI_4M_30FPS_10BIT,
    SC431HAI_MIPI_4M_30FPS_10BIT_WDR2TO1,
    SC450AI_MIPI_4M_30FPS_10BIT,
    SC450AI_MIPI_4M_30FPS_10BIT_WDR2TO1,
    SC500AI_MIPI_5M_30FPS_10BIT,
    SC500AI_MIPI_5M_30FPS_10BIT_WDR2TO1,
    SC4336P_MIPI_3M_30FPS_10BIT,
    SC4336P_MIPI_2M_30FPS_10BIT,
    SC533HAI_MIPI_5M_30FPS_10BIT,
    SC533HAI_MIPI_5M_30FPS_10BIT_WDR2TO1,
    SNS_TYPE_BUTT,
} sample_sns_type;

typedef enum {
    DMEB_QFN,
    DMEB_BGA,
    BOARD_TYPE_BUTT,
} sample_board_type;

typedef struct {
    ot_size          vb_size[SAMPLE_VIE_POOL_NUM];
    ot_pixel_format  pixel_format[SAMPLE_VIE_POOL_NUM];
    ot_compress_mode compress_mode[SAMPLE_VIE_POOL_NUM];
    ot_video_format  video_format[SAMPLE_VIE_POOL_NUM];
    td_s32           blk_num[SAMPLE_VIE_POOL_NUM];
} sample_vb_param;

typedef struct {
    sample_sns_type sns_type;
    td_u32          sns_clk_src;
    td_u32          sns_rst_src;
    td_u32          bus_id;
    td_bool         sns_clk_rst_en;
} sample_sns_info;

typedef struct {
    td_s32             mipi_dev;
    lane_divide_mode_t divide_mode;
    combo_dev_attr_t   combo_dev_attr;
    ext_data_type_t    ext_data_type_attr;
} sample_mipi_info;

typedef struct {
    ot_vi_dev      vi_dev;
    ot_vi_dev_attr dev_attr;
    ot_vi_bas_attr bas_attr;
} sample_vi_dev_info;

typedef struct {
    ot_isp_pub_attr isp_pub_attr;
} sample_isp_info;

typedef struct {
    td_u32                    grp_num;
    ot_vi_grp                 fusion_grp[OT_VI_MAX_WDR_FUSION_GRP_NUM];
    ot_vi_wdr_fusion_grp_attr fusion_grp_attr[OT_VI_MAX_WDR_FUSION_GRP_NUM];
    td_u32                    distribute_grp_num;
    ot_vi_grp                 distribute_grp[OT_VI_MAX_DISTRIBUTE_GRP_NUM];
    ot_vi_distribute_grp_attr distribute_grp_attr[OT_VI_MAX_DISTRIBUTE_GRP_NUM];
} sample_vi_grp_info;

typedef struct {
    ot_vi_chn      vi_chn;
    ot_vi_chn_attr chn_attr;
    ot_fmu_mode    fmu_mode;
} sample_vi_chn_info;

typedef struct {
    ot_vi_pipe_attr    pipe_attr;

    td_bool            pipe_need_start;
    td_bool            isp_need_run;
    sample_isp_info    isp_info;
    td_bool            isp_be_end_trigger;
    td_bool            isp_quick_start;

    td_u32             chn_num;
    sample_vi_chn_info chn_info[OT_VI_MAX_PHYS_CHN_NUM];
    ot_3dnr_attr       nr_attr;
    td_bool            vc_change_en;
    td_u32             vc_number;
    td_bool            is_master_pipe;
    td_u32             bnr_bnf_num;
    ot_vi_pipe_buf_wrap_attr wrap_attr;
    td_u32             pixel_rate;
    ot_low_delay_info low_delay_info;
    ot_frame_interrupt_attr frame_interrupt_attr;
    td_bool            set_early_end_mode;
    ot_vb_pool         attach_pool;
} sample_vi_pipe_info;

typedef struct {
    sample_sns_info     sns_info;
    sample_mipi_info    mipi_info;
    sample_vi_dev_info  dev_info;
    ot_vi_bind_pipe     bind_pipe;
    sample_vi_grp_info  grp_info;
    sample_vi_pipe_info pipe_info[OT_VI_MAX_PHYS_PIPE_NUM];
} sample_vi_cfg;

typedef struct {
    ot_vb_blk           vb_blk;
    td_u32              blk_size;
    ot_video_frame_info frame_info;
} sample_vi_user_frame_info;

typedef struct {
    ot_size          size;
    ot_pixel_format  pixel_format;
    ot_video_format  video_format;
    ot_compress_mode compress_mode;
    ot_dynamic_range dynamic_range;
} sample_vi_get_frame_vb_cfg;

typedef struct {
    td_u32           threshold;
    td_u32           frame_num;
    ot_isp_fpn_type  fpn_type;
    ot_pixel_format  pixel_format;
    ot_compress_mode compress_mode;
} sample_vi_fpn_calibration_cfg;

typedef struct {
    ot_op_mode                op_mode;
    td_bool                   aibnr_mode;
    ot_isp_fpn_type           fpn_type;
    td_u32                    strength;
    ot_pixel_format           pixel_format;
    ot_compress_mode          compress_mode;
    sample_vi_user_frame_info user_frame_info;
} sample_vi_fpn_correction_cfg;

typedef struct {
    td_u32      offset;
} sample_scene_fpn_offset_cfg;

typedef struct {
    td_u32           pipe_num;
    ot_vi_vpss_mode_type mode_type;
    ot_vi_aiisp_mode aiisp_mode;
    ot_size          in_size;
    ot_vb_cfg        vb_cfg;
    td_u32           supplement_cfg;
    ot_3dnr_pos_type  nr_pos;
    ot_vi_pipe       vi_pipe;
    ot_vi_chn        vi_chn;
    sample_vi_cfg    vi_cfg;
    ot_vpss_grp_attr grp_attr;
    td_bool          chn_en[OT_VPSS_MAX_PHYS_CHN_NUM];
    ot_vpss_chn_attr chn_attr[OT_VPSS_MAX_PHYS_CHN_NUM];
    ot_vpss_grp      vpss_grp[SAMPLE_VIE_MAX_ROUTE_NUM];
    ot_fmu_mode      vi_fmu[SAMPLE_VIE_MAX_ROUTE_NUM];
    ot_fmu_mode      vpss_fmu[SAMPLE_VIE_MAX_ROUTE_NUM];
    td_bool          is_direct[SAMPLE_VIE_MAX_ROUTE_NUM];
} sample_comm_cfg;

typedef struct {
    td_s32               route_num;
    ot_vi_vpss_mode_type mode_type;
    ot_fmu_mode          vi_fmu[SAMPLE_VIE_MAX_ROUTE_NUM];
    ot_fmu_mode          vpss_fmu[SAMPLE_VIE_MAX_ROUTE_NUM];
    td_bool              vpss_wrap_en;
    td_u32               vpss_wrap_size;
} sampe_sys_cfg;

typedef struct {
    ot_vpss_grp                 vpss_grp;
    ot_vpss_grp_attr            grp_attr;
    ot_3dnr_attr                nr_attr;
    td_bool                     chn_en[OT_VPSS_MAX_PHYS_CHN_NUM];
    ot_vpss_chn_attr            chn_attr[OT_VPSS_MAX_PHYS_CHN_NUM];
    ot_vpss_chn_buf_wrap_attr   wrap_attr[OT_VPSS_MAX_PHYS_CHN_NUM];
} sample_vpss_cfg;

typedef enum {
    VI_USER_PIC_FRAME = 0,
    VI_USER_PIC_BGCOLOR,
} sample_vi_user_pic_type;

typedef enum {
    SAMPLE_RC_ABR = 0,
    SAMPLE_RC_CBR,
    SAMPLE_RC_VBR,
    SAMPLE_RC_AVBR,
    SAMPLE_RC_CVBR,
    SAMPLE_RC_QVBR,
    SAMPLE_RC_QPMAP,
    SAMPLE_RC_FIXQP
} sample_rc;

typedef struct {
    volatile td_bool thread_start;
    ot_venc_chn venc_chn[OT_VENC_MAX_CHN_NUM];
    td_s32 cnt;
} sample_venc_getstream_para;

typedef struct {
    td_bool thread_start;
    ot_venc_chn venc_chn[OT_VENC_MAX_CHN_NUM];
    td_u32  cnt;
    ot_vpss_grp vpss_grp;
    ot_vpss_chn      vpss_chn[OT_VENC_MAX_CHN_NUM];
} sample_venc_rateauto_para;

typedef struct {
    td_bool  thread_start;
    ot_vpss_grp vpss_grp;
    ot_venc_chn venc_chn[OT_VENC_MAX_CHN_NUM];
    ot_vpss_chn vpss_chn[OT_VPSS_MAX_PHYS_CHN_NUM];
    ot_size size[OT_VENC_MAX_CHN_NUM];
    td_u32 cnt;
} sample_venc_qpmap_sendframe_para;

typedef struct {
    td_bool thread_start;
    ot_vpss_grp vpss_grp;
    ot_venc_chn venc_chn[OT_VENC_MAX_CHN_NUM];
    ot_vpss_chn vpss_chn[OT_VPSS_MAX_PHYS_CHN_NUM];
    ot_size size[OT_VENC_MAX_CHN_NUM];
    ot_venc_jpeg_roi_attr roi_attr[OT_VENC_MAX_CHN_NUM];
    td_u32 cnt;
} sample_venc_roimap_frame_para;

#define REGION_OP_CHN               (0x01L << 0)
#define REGION_OP_DEV               (0x01L << 1)
#define REGION_DESTROY                  (0x01L << 2)
typedef td_u32 region_op_flag;

typedef enum {
    THREAD_CTRL_START,
    THREAD_CTRL_PAUSE,
    THREAD_CTRL_STOP,
} thread_contrl;

typedef struct {
    ot_video_format video_format;
    ot_pixel_format pixel_format;
    td_u32 width;
    td_u32 height;
    td_u32 align;
    ot_compress_mode compress_mode;
} sample_vb_base_info;

typedef struct {
    td_u32 vb_size;
    td_u32 head_stride;
    td_u32 head_size;
    td_u32 head_y_size;
    td_u32 main_stride;
    td_u32 main_size;
    td_u32 main_y_size;
    td_u32 ext_stride;
    td_u32 ext_y_size;
} sample_vb_cal_config;

typedef struct {
    td_u32 frame_rate;
    td_u32 stats_time;
    td_u32 gop;
    ot_size venc_size;
    ot_pic_size size;
    td_u32 profile;
    td_bool is_rcn_ref_share_buf;
    ot_venc_gop_attr gop_attr;
    ot_payload_type type;
    sample_rc rc_mode;
} sample_comm_venc_chn_param;

typedef struct {
    ot_vpss_chn *vpss_chn;
    ot_venc_chn *venc_chn;
    td_s32 cnt;
} sample_venc_roimap_chn_info;

typedef enum {
    SAMPLE_AUDIO_VQE_TYPE_NONE = 0,
    SAMPLE_AUDIO_VQE_TYPE_RECORD,
    SAMPLE_AUDIO_VQE_TYPE_TALK,
    SAMPLE_AUDIO_VQE_TYPE_TALKV2,
    SAMPLE_AUDIO_VQE_TYPE_MAX,
} sample_audio_vqe_type;

typedef struct {
    ot_audio_sample_rate out_sample_rate;
    td_bool resample_en;
    td_void *ai_vqe_attr;
    sample_audio_vqe_type ai_vqe_type;
} sample_comm_ai_vqe_param;

typedef struct {
    ot_wdr_mode wdr_mode;                         /* RW; WDR mode. */
    td_u32     pipe_num;                         /* RW; Range [1,OT_VI_MAX_PHYS_PIPE_NUM] */
    ot_vi_pipe pipe_id[OT_VI_MAX_WDR_FRAME_NUM]; /* RW; Array of pipe ID */
} sample_run_be_bind_pipe;

typedef struct {
    ot_vpss_chn_attr chn_attr[OT_VPSS_MAX_PHYS_CHN_NUM];
    td_bool chn_enable[OT_VPSS_MAX_PHYS_CHN_NUM];
    ot_vpss_chn_buf_wrap_attr wrap_attr[OT_VPSS_MAX_PHYS_CHN_NUM];
    td_u32 chn_array_size;
} sample_vpss_chn_attr;

typedef struct {
    ot_point point1;
    ot_point point2;
    td_u32 thick;
    td_u32 color;
    td_bool is_display;
} sample_osd_line;

typedef struct {
    ot_pixel_format pixel_format;
    td_u8 *data;
    ot_size size;
    sample_osd_line *line;
    td_u32 stride; /* get canvas interface need */
    td_bool is_set_bmp; /* use set_bmp or get_canvas interface */
} sample_drawline_param;

typedef struct {
    td_s32 start_x;
    td_s32 start_y;
    td_s32 end_x;
    td_s32 end_y;
} sample_line;

/* function announce */
#ifndef __LITEOS__
td_void sample_sys_signal(void (*func)(int));
#endif
td_void *sample_sys_io_mmap(td_u64 phy_addr, td_u32 size);
td_s32 sample_sys_munmap(td_void *vir_addr, td_u32 size);
td_s32 sample_sys_set_reg(td_u64 addr, td_u32 value);
td_s32 sample_sys_get_reg(td_u64 addr, td_u32 *value);

td_void sample_comm_sys_get_default_vb_cfg(sample_vb_param *input_cfg, ot_vb_cfg *vb_cfg);
td_void sample_comm_sys_get_default_cfg(td_u32 pipe_num, sample_comm_cfg *comm_cfg);
td_s32 sample_comm_sys_get_pic_size(ot_pic_size pic_size, ot_size *size);
ot_pic_size sample_comm_sys_get_pic_enum(const ot_size *size);
td_s32 sample_comm_sys_mem_config(td_void);
td_s32 sample_comm_sys_init(sample_comm_cfg *comm_cfg);
td_void sample_comm_sys_exit(td_void);
td_s32 sample_comm_sys_vb_init(const ot_vb_cfg *vb_cfg);
td_s32 sample_comm_sys_init_with_vb_supplement(const ot_vb_cfg *vb_conf, td_u32 supplement_config);

td_s32 sample_comm_vi_start_mipi_rx(const sample_sns_info *sns_info, const sample_mipi_info *mipi_info);
td_void sample_comm_vi_stop_mipi_rx(const sample_sns_info *sns_info, const sample_mipi_info *mipi_info);
td_s32 sample_comm_vi_set_mipi_hs_mode(lane_divide_mode_t hs_mode);
td_s32 sample_comm_vi_mipi_ctrl_cmd(td_u32 devno, td_u32 cmd);
td_s32 sample_comm_vi_bind_vpss(ot_vi_pipe vi_pipe, ot_vi_chn vi_chn, ot_vpss_grp vpss_grp, ot_vpss_chn vpss_chn);
td_s32 sample_comm_vi_un_bind_vpss(ot_vi_pipe vi_pipe, ot_vi_chn vi_chn, ot_vpss_grp vpss_grp, ot_vpss_chn vpss_chn);
td_s32 sample_comm_vi_bind_venc(ot_vi_pipe vi_pipe, ot_vi_chn vi_chn, ot_venc_chn venc_chn);
td_s32 sample_comm_vi_un_bind_venc(ot_vi_pipe vi_pipe, ot_vi_chn vi_chn, ot_venc_chn venc_chn);

td_s32 sample_comm_vi_switch_isp_mode(const sample_vi_cfg *vi_cfg);
td_s32 sample_comm_vi_switch_isp_resolution(const sample_vi_cfg *vi_cfg, const ot_size *size);
td_void sample_comm_vi_mode_switch_stop_vi(const sample_vi_cfg *vi_cfg);
td_s32 sample_comm_vi_mode_switch_start_vi(const sample_vi_cfg *vi_cfg, td_bool chg_resolution, const ot_size *size);

td_s32 sample_comm_vpss_bind_venc(ot_vpss_grp vpss_grp, ot_vpss_chn vpss_chn, ot_venc_chn venc_chn);
td_s32 sample_comm_vpss_un_bind_venc(ot_vpss_grp vpss_grp, ot_vpss_chn vpss_chn, ot_venc_chn venc_chn);

td_s32 sample_comm_isp_sensor_regiter_callback(ot_isp_dev isp_dev, sample_sns_type sns_type);
td_s32 sample_comm_isp_sensor_unregiter_callback(ot_isp_dev isp_dev);
td_s32 sample_comm_isp_bind_sns(ot_isp_dev isp_dev, sample_sns_type sns_type, td_s8 sns_dev);
td_s32 sample_comm_isp_thermo_lib_callback(ot_isp_dev isp_dev);
td_s32 sample_comm_isp_thermo_lib_uncallback(ot_isp_dev isp_dev);
td_s32 sample_comm_isp_ae_lib_callback(ot_isp_dev isp_dev);
td_s32 sample_comm_isp_ae_lib_uncallback(ot_isp_dev isp_dev);
td_s32 sample_comm_isp_awb_lib_callback(ot_isp_dev isp_dev);
td_s32 sample_comm_isp_awb_lib_uncallback(ot_isp_dev isp_dev);
td_void sample_common_isp_switch_set_exposure(const sample_vi_cfg *vi_cfg);
td_s32 sample_comm_isp_run(ot_isp_dev isp_dev);
td_void sample_comm_isp_stop(ot_isp_dev isp_dev);
td_void sample_comm_all_isp_stop(td_void);
/**
 * @brief   : 设置镜像、倒置翻转
 * @param    {sample_sns_type} sns_type sensor类型
 * @param    {ot_vi_pipe} vi_pipe VI 物理PIPE号
 * @param    {ot_isp_sns_mirrorflip_type} sns_mirror_flip sensor mirror-flip 枚举
 * @return   {td_s32} 0：成功 -1：失败
 */
td_s32 sample_comm_isp_set_mirror_flip(sample_sns_type sns_type,
                                       ot_vi_pipe vi_pipe,
                                       ot_isp_sns_mirrorflip_type sns_mirror_flip);
td_s32 sample_comm_isp_get_pub_attr_by_sns(sample_sns_type sns_type, ot_isp_pub_attr *pub_attr);
ot_isp_sns_type sample_comm_get_sns_bus_type(sample_sns_type sns_type);
ot_isp_sns_obj *sample_comm_isp_get_sns_obj(sample_sns_type sns_type);
td_void sample_comm_vi_get_size_by_sns_type(sample_sns_type sns_type, ot_size *size);
td_void sample_comm_vi_get_full_lines_std_by_sns_type(sample_sns_type sns_type, td_u32 *full_lines_std);
td_void sample_comm_vi_get_default_vi_cfg(sample_sns_type sns_type, sample_vi_cfg *vi_cfg);
td_void sample_comm_vi_get_vi_cfg_by_fmu_mode(sample_sns_type sns_type, ot_fmu_mode fum_mode, sample_vi_cfg *vi_cfg);
td_void sample_comm_vi_init_vi_cfg(sample_sns_type sns_type, ot_size *size, sample_vi_cfg *vi_cfg);
td_s32 sample_comm_vi_set_vi_vpss_mode(ot_vi_vpss_mode_type mode_type, ot_vi_aiisp_mode aiisp_mode);
td_s32 sample_comm_vi_get_isp_run_state(td_bool *isp_states, td_u32 size);
td_s32 sample_comm_vi_start_sensor(const sample_sns_info *sns_info, const sample_mipi_info *mipi_info);
td_void sample_comm_vi_stop_mipi_rx(const sample_sns_info *sns_info, const sample_mipi_info *mipi_info);
td_s32 sample_comm_vi_start_isp(const sample_vi_cfg *vi_cfg);
td_void sample_comm_vi_stop_isp(const sample_vi_cfg *vi_cfg);
td_s32 sample_comm_vi_start_vi(const sample_vi_cfg *vi_cfg);
td_void sample_comm_vi_stop_vi(const sample_vi_cfg *vi_cfg);
td_void sample_comm_vi_stop_four_vi(const sample_vi_cfg *vi_cfg, td_s32 route_num);
td_s32 sample_comm_vi_fpn_calibrate(ot_vi_pipe vi_pipe, sample_vi_fpn_calibration_cfg *calibration_cfg);
td_s32 sample_comm_vi_enable_fpn_correction(ot_vi_pipe vi_pipe, sample_vi_fpn_correction_cfg *correction_cfg);
td_s32 sample_comm_vi_disable_fpn_correction(ot_vi_pipe vi_pipe, sample_vi_fpn_correction_cfg *correction_cfg);

td_s32 sample_comm_vi_fpn_calibrate_for_thermo(ot_vi_pipe vi_pipe, sample_vi_fpn_calibration_cfg *calibration_cfg);
td_s32 sample_comm_vi_enable_fpn_correction_for_thermo(ot_vi_pipe vi_pipe,
    sample_vi_fpn_correction_cfg *correction_cfg);
td_s32 sample_comm_vi_disable_fpn_correction_for_thermo(ot_vi_pipe vi_pipe,
    sample_vi_fpn_correction_cfg *correction_cfg);

td_s32 sample_comm_vi_enable_fpn_correction_for_scene(ot_vi_pipe vi_pipe, sample_vi_fpn_correction_cfg *correction_cfg,
    td_u32 iso, sample_scene_fpn_offset_cfg *scene_fpn_offset_cfg, const td_char *dir_name);

td_s32 sample_comm_vi_start_virt_pipe(const sample_vi_cfg *vi_cfg);
td_void sample_comm_vi_stop_virt_pipe(const sample_vi_cfg *vi_cfg);
td_s32 sample_common_vi_load_user_pic(ot_vi_pipe vi_pipe, sample_vi_user_pic_type user_pic_type,
    sample_vi_user_frame_info *user_frame_info);
td_void sample_common_vi_unload_user_pic(sample_vi_user_frame_info *user_frame_info);

td_void sample_comm_vpss_get_default_grp_attr(ot_vpss_grp_attr *grp_attr);
td_void sample_comm_vpss_get_default_3dnr_attr(ot_3dnr_attr *nr_attr);
td_void sample_comm_vpss_get_default_chn_attr(ot_vpss_chn_attr *chn_attr);
td_s32 sample_comm_vpss_get_wrap_cfg(ot_vpss_chn_attr chn_attr[OT_VPSS_MAX_PHYS_CHN_NUM],
    ot_vi_vpss_mode_type mode, td_u32 full_lines_std, ot_vpss_chn_buf_wrap_attr *wrap_attr);
td_void sample_comm_vpss_get_default_vpss_cfg(sample_vpss_cfg *vpss_cfg, td_bool vpss_wrap_en);
td_s32 sample_common_vpss_start(ot_vpss_grp grp, const ot_vpss_grp_attr *grp_attr,
    const sample_vpss_chn_attr *vpss_chn_attr);
td_s32 sample_common_vpss_stop(ot_vpss_grp grp, const td_bool *chn_enable, td_u32 chn_array_size);

td_s32 sample_comm_venc_mem_config(td_void);
td_s32 sample_comm_venc_create(ot_venc_chn venc_chn, sample_comm_venc_chn_param *chn_param);
td_s32 sample_comm_venc_start(ot_venc_chn venc_chn, sample_comm_venc_chn_param *chn_param);
td_s32 sample_comm_venc_stop(ot_venc_chn venc_chn);
td_s32 sample_comm_venc_snap_start(ot_venc_chn venc_chn, ot_size *size, td_bool support_dcf);
td_s32 sample_comm_venc_photo_start(ot_venc_chn venc_chn, ot_size *size, td_bool support_dcf);
td_s32 sample_comm_venc_snap_process(ot_venc_chn venc_chn, td_u32 snap_cnt, td_bool save_jpg, td_bool save_thm);
td_s32 sample_comm_venc_save_jpeg(ot_venc_chn venc_chn, td_u32 snap_cnt);
td_s32 sample_comm_venc_snap_stop(ot_venc_chn venc_chn);
td_s32 sample_comm_venc_start_get_stream(ot_venc_chn *venc_chn, td_s32 cnt);
td_s32 sample_comm_venc_stop_get_stream(td_s32 chn_num);
td_s32 sample_comm_venc_start_get_stream_svc_t(td_s32 cnt);
td_s32 sample_comm_venc_get_gop_attr(ot_venc_gop_mode gop_mode, ot_venc_gop_attr *gop_attr);
td_s32 sample_comm_venc_qpmap_send_frame(ot_vpss_grp vpss_grp, ot_vpss_chn vpss_chn[],
                                         ot_venc_chn venc_chn[], td_s32 cnt, ot_size size[]);
td_s32 sample_comm_venc_stop_send_qpmap_frame(td_void);
td_s32 sample_comm_venc_rateauto_start(ot_venc_chn venc_chn[], td_s32 cnt, ot_vpss_grp vpss_grp,
    ot_vpss_chn vpss_chn[]);
td_s32 sample_comm_venc_stop_rateauto(ot_venc_chn chn[], td_s32 cnt);

td_s32 sample_comm_venc_get_file_postfix(ot_payload_type payload, td_char *file_postfix, td_u8 len);
td_s32 sample_comm_venc_send_roimap_frame(ot_vpss_grp vpss_grp, sample_venc_roimap_chn_info roimap_chn_info,
    ot_size size[], ot_venc_jpeg_roi_attr roi_attr[]);
td_s32 sample_comm_venc_stop_send_roimap_frame(td_void);
td_s32 sample_comm_venc_save_stream(FILE *fd, ot_venc_stream *stream);
td_s32 sample_comm_venc_mosaic_map_send_frame(ot_vpss_grp vpss_grp, ot_vpss_chn vpss_chn[], ot_venc_chn venc_chn[],
    td_s32 cnt, ot_size size[]);
td_s32 sample_comm_venc_stop_send_frame(td_void);
td_s32 sample_comm_venc_mini_buf_en(sample_comm_venc_chn_param *chn_param, td_s32 venc_chn_num);

td_s32 sample_comm_region_create(td_s32 handle_num, ot_rgn_type type);
td_s32 sample_comm_region_destroy(td_s32 handle_num, ot_rgn_type type);
td_s32 sample_comm_region_attach(td_s32 handle_num, ot_rgn_type type, ot_mpp_chn *mpp_chn,
    region_op_flag op_flag);
td_s32 sample_comm_check_min(td_s32 min_handle);
td_s32 sample_comm_region_detach(td_s32 handle_num, ot_rgn_type type, ot_mpp_chn *mpp_chn,
    region_op_flag op_flag);
td_s32 sample_comm_region_set_bit_map(ot_rgn_handle handle, const td_char *bmp_path);
td_s32 sample_comm_region_get_up_canvas(ot_rgn_handle handle, const td_char *bmp_path);
td_s32 sample_comm_region_get_min_handle(ot_rgn_type type);

td_s32 sample_comm_audio_init(td_void);
td_void sample_comm_audio_exit(td_void);
td_s32 sample_comm_audio_create_thread_ai_ao(ot_audio_dev ai_dev, ot_ai_chn ai_chn,
    ot_audio_dev ao_dev, ot_ao_chn ao_chn);
td_s32 sample_comm_audio_create_thread_ai_aenc(ot_audio_dev ai_dev, ot_ai_chn ai_chn, ot_aenc_chn ae_chn);
td_s32 sample_comm_audio_create_thread_aenc_adec(ot_aenc_chn ae_chn, ot_adec_chn ad_chn, FILE *aenc_fd);
td_s32 sample_comm_audio_create_thread_file_adec(ot_adec_chn ad_chn, FILE *adec_fd);
td_s32 sample_comm_audio_create_thread_ao_vol_ctrl(ot_audio_dev ao_dev);
td_s32 sample_comm_audio_destroy_thread_ai(ot_audio_dev ai_dev, ot_ai_chn ai_chn);
td_s32 sample_comm_audio_destroy_thread_aenc_adec(ot_aenc_chn ae_chn);
td_s32 sample_comm_audio_destroy_thread_file_adec(ot_adec_chn ad_chn);
td_s32 sample_comm_audio_destroy_thread_ao_vol_ctrl(ot_audio_dev ao_dev);
td_s32 sample_comm_audio_destroy_all_thread(td_void);
td_s32 sample_comm_audio_ao_bind_adec(ot_audio_dev ao_dev, ot_ao_chn ao_chn, ot_adec_chn ad_chn);
td_s32 sample_comm_audio_ao_unbind_adec(ot_audio_dev ao_dev, ot_ao_chn ao_chn, ot_adec_chn ad_chn);
td_s32 sample_comm_audio_ao_bind_ai(ot_audio_dev ai_dev, ot_ai_chn ai_chn, ot_audio_dev ao_dev, ot_ao_chn ao_chn);
td_s32 sample_comm_audio_ao_unbind_ai(ot_audio_dev ai_dev, ot_ai_chn ai_chn, ot_audio_dev ao_dev, ot_ao_chn ao_chn);
td_s32 sample_comm_audio_aenc_bind_ai(ot_audio_dev ai_dev, ot_ai_chn ai_chn, ot_aenc_chn ae_chn);
td_s32 sample_comm_audio_aenc_unbind_ai(ot_audio_dev ai_dev, ot_ai_chn ai_chn, ot_aenc_chn ae_chn);
td_s32 sample_comm_audio_start_ai(ot_audio_dev ai_dev_id, td_u32 ai_chn_cnt, ot_aio_attr *aio_attr,
    const sample_comm_ai_vqe_param *ai_vqe_param, ot_audio_dev ao_dev_id);
td_s32 sample_comm_audio_stop_ai(ot_audio_dev ai_dev_id, td_u32 ai_chn_cnt, td_bool resample_en, td_bool vqe_en);
td_s32 sample_comm_audio_start_ao(ot_audio_dev ao_dev_id, td_u32 ao_chn_cnt,
                                  ot_aio_attr *aio_attr, ot_audio_sample_rate in_sample_rate, td_bool resample_en);
td_s32 sample_comm_audio_stop_ao(ot_audio_dev ao_dev_id, td_u32 ao_chn_cnt, td_bool resample_en);
td_s32 sample_comm_audio_start_aenc(td_u32 aenc_chn_cnt, const ot_aio_attr *aio_attr, ot_payload_type type);
td_s32 sample_comm_audio_stop_aenc(td_u32 aenc_chn_cnt);
td_s32 sample_comm_audio_start_adec(td_u32 adec_chn_cnt, const ot_aio_attr *aio_attr, ot_payload_type type);
td_s32 sample_comm_audio_stop_adec(ot_adec_chn ad_chn);
td_s32 sample_comm_audio_cfg_acodec(const ot_aio_attr *aio_attr);

td_void sample_comm_vi_get_default_sns_info(sample_sns_type sns_type, sample_sns_info *sns_info);
td_void sample_comm_vi_get_default_pipe_info(sample_sns_type sns_type, ot_vi_bind_pipe *bind_pipe,
                                             sample_vi_pipe_info pipe_info[]);

td_void sample_comm_vi_get_default_mipi_info(sample_sns_type sns_type, sample_mipi_info *mipi_info);
td_void sample_comm_vi_get_default_dev_info(sample_sns_type sns_type, sample_vi_dev_info *dev_info);
td_void sample_comm_vi_get_mipi_info_by_dev_id(sample_sns_type sns_type, ot_vi_dev vi_dev,
                                               sample_mipi_info *mipi_info);

td_s32 sample_comm_vi_send_run_be_frame(sample_run_be_bind_pipe *bind_pipe);

td_u32 sample_comm_vi_get_raw_stride(ot_pixel_format pixel_format, td_u32 width, td_u32 byte_align, td_u32 align);
td_s32 sample_comm_vi_read_raw_frame(td_char *frame_file,
                                     sample_vi_user_frame_info user_frame_info[], td_u32 frame_cnt);

td_void sample_comm_set_osd_drawline(td_bool osd_drawline);
td_s32 sample_comm_osd_drawline(sample_drawline_param *drawline_param);
td_s32 sample_comm_region_drawline_set_bit_map(ot_rgn_handle handle, sample_osd_line *line, td_u32 line_num);
td_s32 sample_comm_region_drawline_get_canvas(ot_rgn_handle handle, sample_osd_line *line, td_u32 line_num);

#ifdef __cplusplus
}
#endif /* end of #ifdef __cplusplus */

#endif /* end of #ifndef SAMPLE_COMMON_H */
