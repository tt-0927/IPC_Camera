/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#ifndef TEMPLATE_SNS_CMOS_H
#define TEMPLATE_SNS_CMOS_H

#include "ot_common_isp.h"
#include "ot_sns_ctrl.h"
#include "securec.h"
#include "ot_math.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/****************************************************************************
 * basic info config                                                        *
 ****************************************************************************/
/* sensor ID & Resolution */
#define TEMPLATE_SNS_ID                    4
#define SENSOR_TEMPLATE_SNS_WIDTH          2688
#define SENSOR_TEMPLATE_SNS_HEIGHT         1520

/* sensor slave mode */
#define INCK_ONCE_INCREASE_MAX            750000
#define TEMPLATE_SNS_SLAVE_INCK           24000000
#define TEMPLATE_SNS_SLAVE_INCK_PER_HS    400
#define TEMPLATE_SNS_SLAVE_INCK_PER_VS    800000

/* sensor Register Address */
#define TEMPLATE_SNS_EXPO_H_ADDR           0x3E00
#define TEMPLATE_SNS_EXPO_M_ADDR           0x3E01
#define TEMPLATE_SNS_EXPO_L_ADDR           0x3E02
#define TEMPLATE_SNS_AGAIN_H_ADDR          0x3E08
#define TEMPLATE_SNS_AGAIN_L_ADDR          0x3E09
#define TEMPLATE_SNS_DGAIN_H_ADDR          0x3E06
#define TEMPLATE_SNS_DGAIN_L_ADDR          0x3E07
#define TEMPLATE_SNS_SHORT_EXPO_H_ADDR     0x3E22
#define TEMPLATE_SNS_SHORT_EXPO_M_ADDR     0x3E04
#define TEMPLATE_SNS_SHORT_EXPO_L_ADDR     0x3E05
#define TEMPLATE_SNS_SHORT_AGAIN_H_ADDR    0x3E12
#define TEMPLATE_SNS_SHORT_AGAIN_L_ADDR    0x3E13
#define TEMPLATE_SNS_SHORT_DGAIN_H_ADDR    0x3E10
#define TEMPLATE_SNS_SHORT_DGAIN_L_ADDR    0x3E11
#define TEMPLATE_SNS_VMAX_H_ADDR           0x320E
#define TEMPLATE_SNS_VMAX_L_ADDR           0x320F
#define TEMPLATE_SNS_SHORT_EXP_MAX_H_ADDR  0x3E23
#define TEMPLATE_SNS_SHORT_EXP_MAX_L_ADDR  0x3E24

/* sensor Register mask & shift */
typedef enum {
    AGAIN_REG_LOW = 0,
    AGAIN_REG_HIGH,
    DGAIN_REG_LOW,
    DGAIN_REG_MID,
    DGAIN_REG_HIGH
} reg_mode;

#define AGAIN_REG_LOW_MASK    0xf
#define AGAIN_REG_HIGH_MASK   0xf0
#define DGAIN_REG_LOW_MASK    0x03
#define DGAIN_REG_MID_MASK    0x3FC
#define DGAIN_REG_HIGH_MASK   0x3C00

#define AGAIN_REG_LOW_LSHIFT    4
#define AGAIN_REG_HIGH_RSHIFT   4
#define DGAIN_REG_LOW_LSHIFT    6
#define DGAIN_REG_MID_RSHIFT    2
#define DGAIN_REG_HIGH_RSHIFT   10

/****************************************************************************
 * i2c bus config                                                           *
 ****************************************************************************/
/* sensor I2C bus config */
#define TEMPLATE_SNS_I2C_ADDR    0x6c
#define TEMPLATE_SNS_ADDR_BYTE   2
#define TEMPLATE_SNS_DATA_BYTE   1

/* common I2C bus config */
#define I2C_DEV_FILE_NUM    16
#define I2C_BUF_NUM         8

/****************************************************************************
 * sensor lines configs                                                     *
 ****************************************************************************/
/* increase line */
#define TEMPLATE_SNS_INCREASE_LINES 0  /* make real fps less than stand fps because NVR require */

/* linear mode */
#define TEMPLATE_SNS_FULL_LINES_MAX_LINEAR 0x79E0 /* 0x4650 = 18000 = Min:2.5fps */
#define TEMPLATE_SNS_VMAX_VAL_LINEAR 1560         /* linear init sequence e.g. 0x380e, 0x06, 0x380f, 0x58 */
#define TEMPLATE_SNS_VMAX_LINEAR     (TEMPLATE_SNS_VMAX_VAL_LINEAR + TEMPLATE_SNS_INCREASE_LINES)
#define TEMPLATE_SNS_FPS_MAX_LINEAR 30
#define TEMPLATE_SNS_FPS_MIN_LINEAR 1.5
#define TEMPLATE_SNS_WIDTH_LINEAR 2688
#define TEMPLATE_SNS_HEIGHT_LINEAR 1520
#define TEMPLATE_SNS_MODE_LINEAR 0

/* WDR mode */
#define TEMPLATE_SNS_FULL_LINES_MAX_2TO1_LINE_WDR 0x2490 /* 0x2328 = 9000 = Min:10fps */
#define TEMPLATE_SNS_VMAX_VAL_2TO1_LINE_WDR 3120         /* WDR init sequence 0x380e, 0x09, 0x380f, 0x84 */
#define TEMPLATE_SNS_VMAX_2TO1_LINE_WDR   (TEMPLATE_SNS_VMAX_VAL_2TO1_LINE_WDR + TEMPLATE_SNS_INCREASE_LINES)
#define TEMPLATE_SNS_FPS_MAX_2TO1_LINE_WDR 30
#define TEMPLATE_SNS_FPS_MIN_2TO1_LINE_WDR 10.0
#define TEMPLATE_SNS_WIDTH_2TO1_LINE_WDR 2688
#define TEMPLATE_SNS_HEIGHT_2TO1_LINE_WDR 1520
#define TEMPLATE_SNS_MODE_2TO1_LINE_WDR 0
#define TEMPLATE_SNS_EXP_Y 0x5FF

/****************************************************************************
 * sensor ae configs                                                        *
 ****************************************************************************/
#define TEMPLATE_SNS_AGAIN_MIN    1024
#define TEMPLATE_SNS_AGAIN_MAX    61976

#define TEMPLATE_SNS_DGAIN_MIN    1024
#define TEMPLATE_SNS_DGAIN_MAX    16256

#define ISP_DGAIN_SHIFT           8
#define ISP_DGAIN_TARGET_MIN      1
#define ISP_DGAIN_TARGET_MAX      32
#define ISP_DGAIN_TARGET_WDR_MIN  1
#define ISP_DGAIN_TARGET_WDR_MAX  4

#define FL_OFFSET_LINEAR       8
#define FL_OFFSET_WDR          10
#define SFL_OFFSET_WDR         11
#define FL_OFFSET_WDR_LONG                21
#define FL_OFFSET_WDR_SHORT               19
#define TEMPLATE_SNS_SEXP_MAX_DEFAULT         0xB8

#define INT_TIME_ACCURACY_LINEAR  1
#define INT_TIME_ACCURACY_WDR     4
#define AGAIN_ACCURACY         0.0625
#define DGAIN_ACCURACY         0.125

/****************************************************************************
 * sensor awb calibrate configs                                             *
 ****************************************************************************/
/* awb static param for Fuji Lens New IR_Cut */
#define CALIBRATE_STATIC_TEMP                         5000
#define CALIBRATE_STATIC_WB_R_GAIN                    527
#define CALIBRATE_STATIC_WB_GR_GAIN                   256
#define CALIBRATE_STATIC_WB_GB_GAIN                   256
#define CALIBRATE_STATIC_WB_B_GAIN                    526

/* Calibration results for Auto WB Planck */
#define CALIBRATE_AWB_P1                              1
#define CALIBRATE_AWB_P2                              241
#define CALIBRATE_AWB_Q1                              (-15)
#define CALIBRATE_AWB_A1                              159211
#define CALIBRATE_AWB_B1                              128
#define CALIBRATE_AWB_C1                              (-104902)

/* Rgain and Bgain of the golden sample */
#define GOLDEN_RGAIN                                  0
#define GOLDEN_BGAIN                                  0

/****************************************************************************
 * sensor other configs                                                     *
 ****************************************************************************/
#define STANDARD_FPS                   30
#define FLICKER_FREQ                  (50 * 256)  /* light flicker freq: 50Hz, accuracy: 256 */

#define INIT_EXP_DEFAULT_LINEAR        76151
#define INIT_EXP_DEFAULT_WDR           52000

#define MAX_INT_TIME_TARGET            65535

#define AE_COMENSATION_DEFAULT         0x40
#define AE_COMENSATION_WDR_LONG_FRM    56
#define AE_COMENSATION_WDR_NORM_FRM    32

#define AE_ARR_RATIO_0_WDR             0x400
#define AE_ARR_RATIO_1_WDR             0x40
#define AE_ARR_RATIO_2_WDR             0x40

/* Black level */
#define BLACK_LEVEL_DEFAULT            0x400

/* DNG */
#define DNG_RAW_FORMAT_BIT_LINEAR         12 /* raw 12 bit */
#define DNG_RAW_FORMAT_WHITE_LEVEL_LINEAR 4095 /* 2^12 - 1 */
#define DNG_RAW_FORMAT_BIT_WDR            12 /* raw 12 bit */
#define DNG_RAW_FORMAT_WHITE_LEVEL_WDR    4095 /* 2^12 - 1 */

/* WDR Step */
#define TEMPLATE_SNS_STEP_OFFSET          1520
#define TEMPLATE_SNS_MARGIN               40

/****************************************************************************
 * assist function macros                                                   *
 ****************************************************************************/
#define high_8bits(x)   (((x) & 0xff00) >> 8)
#define low_8bits(x)    ((x) & 0x00ff)
#define lower_4bits(x)  (((x) & 0x000F) << 4)
#define higher_4bits(x) (((x) & 0xF000) >> 12)
#define higher_8bits(x) (((x) & 0x0FF0) >> 4)
#ifndef clip3
#define clip3(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))
#endif

#ifndef max
#define max(a, b) (((a) < (b)) ?  (b) : (a))
#endif

#ifndef min
#define min(a, b) (((a) > (b)) ?  (b) : (a))
#endif

/****************************************************************************
 * sensor data type defines                                                 *
 ****************************************************************************/
/* define your sensor modes */
typedef enum {
    TEMPLATE_SNS_4M_30FPS_12BIT_LINEAR_MODE = 0,
    TEMPLATE_SNS_4M_30FPS_12BIT_2TO1_LINE_WDR_MODE,
    TEMPLATE_SNS_MODE_MAX
} template_sns_res_mode;

/* define your register ids */
typedef enum {
    EXPO_L_IDX = 0,
    EXPO_M_IDX,
    EXPO_H_IDX,
    AGAIN_L_IDX,
    AGAIN_H_IDX,
    DGAIN_L_IDX,
    DGAIN_H_IDX,
    VMAX_L_IDX,
    VMAX_H_IDX,
    REG_MAX_IDX
}template_sns_linear_reg_index;

typedef enum {
    WDR_EXPO_L_IDX = 0,
    WDR_EXPO_M_IDX,
    WDR_EXPO_H_IDX,
    WDR_AGAIN_L_IDX,
    WDR_AGAIN_H_IDX,
    WDR_DGAIN_L_IDX,
    WDR_DGAIN_H_IDX,
    WDR_VMAX_L_IDX,
    WDR_VMAX_H_IDX,
    WDR_SHORT_EXPO_L_IDX,
    WDR_SHORT_EXPO_M_IDX,
    WDR_SHORT_AGAIN_L_IDX,
    WDR_SHORT_AGAIN_H_IDX,
    WDR_SHORT_DGAIN_L_IDX,
    WDR_SHORT_DGAIN_H_IDX,
    SHORT_EXP_MAX_L_ADDR_IDX,
    SHORT_EXP_MAX_H_ADDR_IDX,
	WDR_REG_MAX_IDX
}template_sns_wdr_reg_index;

typedef struct {
    td_u32 dec[OT_ISP_WDR_MAX_FRAME_NUM];
    td_u32 inc[OT_ISP_WDR_MAX_FRAME_NUM];
} time_step;

/****************************************************************************
 * sensor functions declare                                                 *
 ****************************************************************************/

ot_isp_sns_obj *template_sns_get_obj(td_void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
#endif /* TEMPLATE_SNS_CMOS_H */
