/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include <math.h>
#include "isp_config.h"
#include "isp_alg.h"
#include "isp_sensor.h"
#include "isp_ext_config.h"
#include "isp_math_utils.h"
#include "isp_proc.h"
#include "isp_param_check.h"
#include "isp_ext_reg_access.h"

#define FORCELONG_WEIGHT 63
#define FORCELONG_FRACBITS 4
#define WEIGHT_FRACBITS 6

#define OT_WDR_BITDEPTH            12
#define OT_ISP_WDR_SQRT_OUT_BITS   8
#define OT_WDR_EXPOSURE_BASE       64
#define OT_ISP_WDR_RATIO_MAX       2048
#define OT_FUSION_LONG_SLOPE_BIT   10
#define OT_WDR_THR_BIT             12
#define OT_FUSION_THR_SHIFT_BIT    8
#define OT_ISP_WDR_MERGE_FRAME_VS  0
#define OT_ISP_WDR_MERGE_FRAME_S   1
#define OT_ISP_WDR_MERGE_FRAME_M   2
#define OT_ISP_WDR_MERGE_FRAME_L   3
#define OT_WDR_SQRT_GAIN_BITDEP    16
#define WDR_SQRT_GAIN_BITDEP       16
#define WDR_EXPOSURE_BASE          0x40
#define WDR_MAX_VALUE              ((1 << 19) - 1)
#define WDR_R_RATIO_BITS           10
#define WDR_BCOM_ALPHA_DEFAULT     0
#define WDR_BDEC_ALPHA_DEFAULT     4
#define WDR_2M1_FRAME              2
#define WDR_3M1_FRAME              3
#define WDR_4M1_FRAME              4
#define WDR_FUSION_RATIO_THR       512
#define WDR_SENSOR_GAIN_SHIFT_BITS 16
#define WDR_SENSOR_GAIN_BASE       1024
#define WDR_SENSOR_GAIN_ROUND      512
#define WDR_RATIO_MAX              2048
#define WDR_RG_WGT                 3
#define WDR_BG_WGT                 3
#define FUSION_THR_BIT             12
#define WDR_THR_BIT                12
#define WDR_MD_LOW_THR             45
#define WDR_MD_HIG_THR             45
#define WDR_AWB_RGAIN_INDEX        0
#define WDR_AWB_GGAIN_INDEX        1
#define WDR_AWB_BGAIN_INDEX        3
#define NOISE_SET_ELE_NUM          7

#define WDR_IMAGE_SHIFT       15
#define WDR_LPF_COEF_NUM      9
#define WDR_NOISE_NUM_EVEN    65
#define WDR_NOISE_NUM_ODD     64
#define WDR_SIGMA_LUT_SEG_NUM 8
#define WDR_NOISESET_ELENUM       7


static const  td_u32 g_noise_again_set[NOISE_SET_ELE_NUM] = { 1, 2, 4, 8, 16, 32, 64 };
static const  td_u8 g_noise_floor_set[NOISE_SET_ELE_NUM] = { 1, 2, 3, 6, 11, 17, 21 };
static const  td_u16 g_fusion_thr[OT_ISP_WDR_MAX_FRAME_NUM] = { 15420, 12000, 12000, 12000 };

static const  td_u8 g_lpf_coef[WDR_LPF_COEF_NUM] = { 1, 2, 1, 2, 4, 2, 1, 2, 1 };

static const  td_u8  g_lut_mdt_low_thr[OT_ISP_WDR_RATIO_NUM][OT_ISP_AUTO_ISO_NUM] = {
    {45, 45, 45, 64, 96, 128, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
    {45, 45, 45, 64, 96, 128, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
    {24, 24, 24, 45, 64,  96, 128, 255, 255, 255, 255, 255, 255, 255, 255, 255},
    {24, 24, 24, 45, 64,  96, 128, 255, 255, 255, 255, 255, 255, 255, 255, 255},
    {45, 45, 45, 45, 64, 128, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
    {45, 45, 45, 45, 64, 128, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
    {64, 64, 64, 64, 128, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
    {128, 128, 128, 128, 128, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
    {128, 128, 128, 128, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},

};
static const  td_u8  g_lut_mdt_hig_thr[OT_ISP_WDR_RATIO_NUM][OT_ISP_AUTO_ISO_NUM] = {
    {64, 64, 64, 64, 96, 128, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
    {64, 64, 64, 64, 96, 128, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
    {45, 45, 45, 64, 96, 128, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
    {45, 45, 45, 64, 96, 128, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
    {64, 64, 64, 64, 96, 128, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
    {64, 64, 64, 64, 96, 128, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
    {128, 128, 128, 128, 128, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},

};

static const td_u32 g_nrm_lut[WDR_SIGMA_LUT_NUM] = {
    128,     256,     384,     512,     640,     768,     896,    1024,    1152,
    1280,    1408,    1536,    1664,    1792,    1920,    2048,    2176,    2304,
    2432,    2560,    2688,    2816,    2944,    3072,    3200,    3328,    3456,
    3584,    3712,    3840,    3968,    4096,    4224,    4352,    4480,    4608,
    4736,    4864,    4992,    5120,    5248,    5376,    5504,    5632,    5760,
    5888,    6016,    6144,    6272,    6400,    6528,    6656,    6784,    6912,
    7040,    7168,    7296,    7424,    7552,    7680,    7808,    7936,    8064,
    8192,    8320,    8448,    8576,    8704,    8832,    8960,    9088,    9216,
    9344,    9472,    9600,    9728,    9856,    9984,    10112,   10240,   10368,
    10496,   10624,   10752,   10880,   11008,   11136,   11264,   11392,   11520,
    11648,   11776,   11904,   12032,   12160,   12288,   12416,   12544,   12672,
    12800,   12928,   13056,   13184,   13312,   13440,   13568,   13696,   13824,
    13952,   14080,   14208,   14336,   14464,   14592,   14720,   14848,   14976,
    15104,   15232,   15360,   15488,   15616,   15744,   15872,   16000,   16128,
    16256,   16384,   16640,   16896,   17152,   17408,   17664,   17920,   18176,
    18432,   18688,   18944,   19200,   19456,   19712,   19968,   20224,   20480,
    20736,   20992,   21248,   21504,   21760,   22016,   22272,   22528,   22784,
    23040,   23296,   23552,   23808,   24064,   24320,   24576,   24832,   25088,
    25344,   25600,   25856,   26112,   26368,   26624,   26880,   27136,   27392,
    27648,   27904,   28160,   28416,   28672,   28928,   29184,   29440,   29696,
    29952,   30208,   30464,   30720,   30976,   31232,   31488,   31744,   32000,
    32256,   32512,   32768,   33280,   33792,   34304,   34816,   35328,   35840,
    36352,   36864,   37376,   37888,   38400,   38912,   39424,   39936,   40448,
    40960,   41472,   41984,   42496,   43008,   43520,   44032,   44544,   45056,
    45568,   46080,   46592,   47104,   47616,   48128,   48640,   49152,   49664,
    50176,   50688,   51200,   51712,   52224,   52736,   53248,   53760,   54272,
    54784,   55296,   55808,   56320,   56832,   57344,   57856,   58368,   58880,
    59392,   59904,   60416,   60928,   61440,   61952,   62464,   62976,   63488,
    64000,   64512,   65024,   65536,   66560,   67584,   68608,   69632,   70656,
    71680,   72704,   73728,   74752,   75776,   76800,   77824,   78848,   79872,
    80896,   81920,   82944,   83968,   84992,   86016,   87040,   88064,   89088,
    90112,   91136,   92160,   93184,   94208,   95232,   96256,   97280,   98304,
    99328,   100352,  101376,  102400,  103424,  104448,  105472,  106496,  107520,
    108544,  109568,  110592,  111616,  112640,  113664,  114688,  115712,  116736,
    117760,  118784,  119808,  120832,  121856,  122880,  123904,  124928,  125952,
    126976,  128000,  129024,  130048,  131072,  133120,  135168,  137216,  139264,
    141312,  143360,  145408,  147456,  149504,  151552,  153600,  155648,  157696,
    159744,  161792,  163840,  165888,  167936,  169984,  172032,  174080,  176128,
    178176,  180224,  182272,  184320,  186368,  188416,  190464,  192512,  194560,
    196608,  198656,  200704,  202752,  204800,  206848,  208896,  210944,  212992,
    215040,  217088,  219136,  221184,  223232,  225280,  227328,  229376,  231424,
    233472,  235520,  237568,  239616,  241664,  243712,  245760,  247808,  249856,
    251904,  253952,  256000,  258048,  260096,  262144,  266240,  270336,  274432,
    278528,  282624,  286720,  290816,  294912,  299008,  303104,  307200,  311296,
    315392,  319488,  323584,  327680,  331776,  335872,  339968,  344064,  348160,
    352256,  356352,  360448,  364544,  368640,  372736,  376832,  380928,  385024,
    389120,  393216,  397312,  401408,  405504,  409600,  413696,  417792,  421888,
    425984,  430080,  434176,  438272,  442368,  446464,  450560,  454656,  458752,
    462848,  466944,  471040,  475136,  479232,  483328,  487424,  491520,  495616,
    499712,  503808,  507904,  512000,  516096,  520192,  524288,  532480,  540672,
    548864,  557056,  565248,  573440,  581632,  589824,  598016,  606208,  614400,
    622592,  630784,  638976,  647168,  655360,  663552,  671744,  679936,  688128,
    696320,  704512,  712704,  720896,  729088,  737280,  745472,  753664,  761856,
    770048,  778240,  786432,  794624,  802816,  811008,  819200,  827392,  835584,
    843776,  851968,  860160,  868352,  876544,  884736,  892928,  901120,  909312,
    917504,  925696,  933888,  942080,  950272,  958464,  966656,  974848,  983040,
    991232,  999424,  1007616, 1015808, 1024000, 1032192, 1040384, 1048575, 1048575
};

static const td_u16  g_noise_profile_even[WDR_NOISE_NUM_EVEN] = {
    0x27a,  0x27a,  0x27a,  0x27a,  0x319,  0x4d9,  0x61e,  0x72a,  0x813,  0x8e5,  0x9a5,  0xa57,  0xafe,
    0xb9b,  0xc31,  0xcc0,  0xd48,  0xdcc,  0xe4a,  0xec5,  0xf3c,  0xfaf,  0x101f, 0x108b, 0x10f6,
    0x115d, 0x11c3, 0x1226, 0x1287, 0x12e6, 0x1343, 0x139f, 0x13f9, 0x1451, 0x14a8, 0x14fd,
    0x1551, 0x15a4, 0x15f6, 0x1646, 0x1696, 0x16e4, 0x1731, 0x177d, 0x17c9, 0x1813, 0x185c,
    0x18a5, 0x18ed, 0x1934, 0x197a, 0x19c0, 0x1a04, 0x1a48, 0x1a8c, 0x1ace, 0x1b10, 0x1b52,
    0x1b93, 0x1bd3, 0x1c13, 0x1c52, 0x1c90, 0x1cce, 0x1d0c
};

static const td_u16 g_noise_profile_odd[WDR_NOISE_NUM_ODD] = {
    0x27a,  0x27a,  0x27a,  0x27a,  0x412,  0x585,  0x6a9,  0x7a2,  0x87f,  0x947,  0xa00,  0xaac,  0xb4e,
    0xbe7,  0xc79,  0xd05,  0xd8b,  0xe0c,  0xe88,  0xf01,  0xf75,  0xfe7,  0x1055, 0x10c1, 0x112a,
    0x1190, 0x11f5, 0x1257, 0x12b7, 0x1315, 0x1371, 0x13cc, 0x1425, 0x147d, 0x14d3, 0x1527,
    0x157b, 0x15cd, 0x161e, 0x166e, 0x16bd, 0x170b, 0x1757, 0x17a3, 0x17ee, 0x1838, 0x1881,
    0x18c9, 0x1910, 0x1957, 0x199d, 0x19e2, 0x1a26, 0x1a6a, 0x1aad, 0x1af0, 0x1b31, 0x1b72,
    0x1bb3, 0x1bf3, 0x1c32, 0x1c71, 0x1caf, 0x1ced
};

static const td_s32 g_ratio_thd[OT_ISP_WDR_RATIO_NUM] = {128, 256, 512, 1024, 1536, 2048, 2560, 3072, 3584, 4096};

/* default gamma_bcom curve */
static const td_u32 gamma_bcom_non_unit_lut[BCOM_BDEC_NODE_NUMBER] = {
    0,       112,     219,     320,     416,     506,     591,     673,
    750,     824,     895,     961,     1026,    1087,    1146,    1202,
    1256,    1308,    1357,    1405,    1452,    1495,    1538,    1580,
    1619,    1657,    1694,    1731,    1765,    1799,    1831,    1862,
    1893,    1951,    2005,    2057,    2106,    2152,    2195,    2237,
    2276,    2313,    2349,    2382,    2414,    2446,    2475,    2503,
    2530,    2581,    2627,    2670,    2710,    2747,    2780,    2812,
    2842,    2870,    2896,    2920,    2943,    2965,    2985,    3004,
    3022,    3039,    3056,    3071,    3085,    3100,    3112,    3125,
    3137,    3148,    3159,    3170,    3180,    3190,    3199,    3208,
    3216,    3225,    3233,    3241,    3248,    3256,    3263,    3270,
    3277,    3283,    3290,    3296,    3302,    3309,    3314,    3320,
    3326,    3332,    3338,    3343,    3349,    3355,    3360,    3366,
    3371,    3376,    3382,    3387,    3392,    3398,    3403,    3408,
    3414,    3419,    3424,    3429,    3435,    3440,    3445,    3451,
    3456,    3462,    3467,    3472,    3478,    3483,    3488,    3494,
    3499,    3504,    3510,    3515,    3521,    3526,    3532,    3537,
    3542,    3548,    3553,    3559,    3564,    3569,    3574,    3580,
    3585,    3591,    3596,    3602,    3606,    3611,    3617,    3623,
    3628,    3633,    3638,    3643,    3648,    3654,    3658,    3664,
    3669,    3674,    3678,    3684,    3689,    3694,    3698,    3703,
    3708,    3713,    3718,    3723,    3728,    3732,    3737,    3742,
    3746,    3751,    3755,    3760,    3765,    3769,    3774,    3777,
    3782,    3786,    3791,    3796,    3799,    3804,    3808,    3812,
    3816,    3821,    3824,    3828,    3833,    3837,    3840,    3844,
    3848,    3852,    3855,    3859,    3863,    3867,    3871,    3875,
    3878,    3882,    3885,    3889,    3893,    3896,    3900,    3903,
    3906,    3909,    3913,    3916,    3920,    3923,    3927,    3930,
    3933,    3940,    3946,    3951,    3957,    3964,    3970,    3975,
    3981,    3987,    3992,    3998,    4003,    4009,    4014,    4019,
    4024,    4029,    4034,    4039,    4044,    4048,    4053,    4058,
    4062,    4067,    4071,    4075,    4080,    4084,    4088,    4092,
    4096
};

static const td_u8 gamma_bcom_non_unit_step_lut[BCOM_BDEC_OUTSEG_NUMBER] = {7, 7, 8, 9, 9, 9, 9, 10};
static const td_u16 gamma_bcom_non_unit_seg_lut[BCOM_BDEC_OUTSEG_NUMBER] = {16, 32, 48, 64, 96, 160, 224, 256};
static const td_u32 gamma_bcom_non_unit_pos_lut[BCOM_BDEC_OUTSEG_NUMBER] = {2048, 4096, 8192, 16384,
    32768, 65536, 98304, 131072};

/* default gamma_bdec curve */
static const td_u32 gamma_bdec_non_unit_lut[BCOM_BDEC_NODE_NUMBER] = {
    0,      36,     72,     109,    146,    184,    222,    262,
    302,    343,    384,    426,    469,    513,    558,    603,
    650,    697,    745,    794,    843,    894,    948,    1001,
    1055,   1110,   1166,   1224,   1284,   1345,   1406,   1469,
    1533,   1567,   1599,   1633,   1666,   1701,   1737,   1771,
    1808,   1842,   1880,   1915,   1954,   1992,   2029,   2069,
    2107,   2148,   2187,   2228,   2271,   2311,   2354,   2396,
    2440,   2485,   2529,   2575,   2619,   2667,   2713,   2761,
    2811,   2858,   2909,   2958,   3010,   3063,   3114,   3169,
    3223,   3278,   3335,   3390,   3448,   3508,   3566,   3627,
    3689,   3750,   3815,   3877,   3943,   4011,   4076,   4146,
    4217,   4286,   4359,   4434,   4506,   4584,   4663,   4738,
    4820,   4903,   4983,   5070,   5157,   5246,   5333,   5426,
    5521,   5612,   5711,   5811,   5914,   6017,   6119,   6228,
    6339,   6453,   6569,   6684,   6802,   6926,   7053,   7183,
    7315,   7451,   7590,   7732,   7878,   8027,   8181,   8338,
    8499,   8664,   8833,   9007,   9186,   9370,   9562,   9764,
    9964,   10169,  10393,  10612,  10841,  11082,  11322,  11583,
    11841,  12118,  12407,  12706,  13003,  13322,  13655,  14002,
    14362,  14736,  15148,  15555,  15994,  16447,  16937,  17426,
    17970,  18532,  19127,  19779,  20442,  21178,  21922,  22750,
    23633,  24568,  25552,  26614,  27759,  28961,  30241,  31552,
    32968,  34409,  35899,  37403,  38917,  40474,  41991,  43544,
    45085,  46607,  48101,  49666,  51193,  52674,  54219,  55705,
    57254,  58799,  60338,  61938,  63528,  65101,  66736,  68349,
    70024,  71765,  73575,  75359,  77212,  79031,  80920,  83002,
    85049,  86042,  87181,  88215,  89269,  90480,  91579,  92701,
    93990,  95162,  96358,  97580,  98828,  100103, 101570, 102906,
    104271, 104965, 105668, 106378, 107096, 107823, 108558, 109301,
    110054, 110815, 111585, 112365, 113154, 113952, 114761, 115579,
    116407, 117245, 118094, 118954, 119824, 120706, 121599, 122503,
    123419, 124348, 125288, 126240, 127206, 128184, 129176, 130181,
    131072
};

static const td_u8 gamma_bdec_non_unit_step_lut[BCOM_BDEC_OUTSEG_NUMBER] = {5, 4, 4, 4, 4, 3, 2, 2};
static const td_u16 gamma_bdec_non_unit_seg_lut[BCOM_BDEC_OUTSEG_NUMBER] = {32, 96, 160, 192, 208, 224, 240, 256};
static const td_u32 gamma_bdec_non_unit_pos_lut[BCOM_BDEC_OUTSEG_NUMBER] = {1024, 2048, 3072, 3584,
    3840, 3968, 4032, 4096};

typedef struct {
    td_bool     init;
    td_bool     coef_update_en;
    td_bool     wdr_en;
    td_bool     erosion_en;
    td_u8       actual_md_thr_low_gain;
    td_u8       actual_md_thr_hig_gain;
    td_u8       bit_depth_prc;
    td_u8       noise_model_coef;

    td_u16      short_thr_reg;
    td_u16      long_thr_reg;
    td_u32      pre_again;

    td_u8       floor_set_lut[WDR_NOISESET_ELENUM];
    td_u32      again_set_lut[WDR_NOISESET_ELENUM];
    td_u16      fusion_barrier0;        /* U14.0 */
    td_u16      fusion_barrier1;        /* U14.0 */
    td_u16      fusion_barrier2;        /* U14.0 */
    td_u16      fusion_barrier3;        /* U14.0 */

    td_u16      force_long_low_thr_reg; /* u14.0,[0,16383] */
    td_u16      force_long_hig_thr_reg; /* u14.0,[0,16383] */

    td_bool     sync_bcom_en[OT_ISP_STRIPING_MAX_NUM][CFG2VLD_DLY_LIMIT];
    td_bool     sync_bdec_en[OT_ISP_STRIPING_MAX_NUM][CFG2VLD_DLY_LIMIT];

    ot_isp_wdr_fs_attr mpi_cfg;
} isp_fs_wdr;

isp_fs_wdr *g_fswdr_ctx[OT_ISP_MAX_PIPE_NUM] = {TD_NULL};

#define fs_wdr_get_ctx(pipe, ctx)   ((ctx) = g_fswdr_ctx[pipe])
#define fs_wdr_set_ctx(pipe, ctx)   (g_fswdr_ctx[pipe] = (ctx))
#define fs_wdr_reset_ctx(pipe)         (g_fswdr_ctx[pipe] = TD_NULL)

static inline td_u16 fswdr_calc_wgt_slope(td_u16 short_thr, td_u16 long_thr)
{
    return (OT_ISP_WDR_DFTWGT_FL_DEFAULT << WEIGHT_FRACBITS) / div_0_to_1(short_thr - long_thr);
}

static td_s32 frame_wdr_ctx_init(ot_vi_pipe vi_pipe)
{
    isp_fs_wdr *fswdr_ctx = TD_NULL;

    fs_wdr_get_ctx(vi_pipe, fswdr_ctx);

    if (fswdr_ctx == TD_NULL) {
        fswdr_ctx = (isp_fs_wdr *)isp_malloc(sizeof(isp_fs_wdr));
        if (fswdr_ctx == TD_NULL) {
            isp_err_trace("Isp[%d] fs_wdr_ctx malloc memory failed!\n", vi_pipe);
            return OT_ERR_ISP_NOMEM;
        }
    }

    (td_void)memset_s(fswdr_ctx, sizeof(isp_fs_wdr), 0, sizeof(isp_fs_wdr));

    fs_wdr_set_ctx(vi_pipe, fswdr_ctx);

    return TD_SUCCESS;
}

static td_void frame_wdr_ctx_exit(ot_vi_pipe vi_pipe)
{
    isp_fs_wdr *fswdr_ctx;

    fs_wdr_get_ctx(vi_pipe, fswdr_ctx);
    isp_free(fswdr_ctx);
    fs_wdr_reset_ctx(vi_pipe);
}

static td_u32 wdr_sqrt(td_u32 in_val, td_u32 dst_bit_dep)
{
    td_u32 x;                                                       /* u10.0 */
    td_u32 y;                                                       /* u20.0 */
    td_s8 j;
    const td_u8 shift_value = 2;
    td_u8 jtemp;
    td_u32 val = in_val;

    x = (1 << dst_bit_dep) - 1;
    y = x * x;

    val = val << shift_value;

    for (j = (td_s8)dst_bit_dep; j >= 0; j--) {
        jtemp = (td_u8)j;
        if (val > y) {
            y = y + ((td_u64)1 << (jtemp + 1)) * x + ((td_u64)1 << (shift_value * jtemp));
            x = x + ((td_u64)1 << jtemp);                               /* u10.0 */
        } else {
            y = y - ((td_u64)1 << (jtemp + 1)) * x + ((td_u64)1 << (shift_value * jtemp));
            x = x - ((td_u64)1 << jtemp);                               /* u10.0 */
        }
    }

    if (val > y) {
        x = x + 1;
    } else if (val < y) {
        x = x - 1;
    }

    x = x >> 1;

    return x;
}

static td_void frame_wdr_ext_regs_initialize(ot_vi_pipe vi_pipe, isp_fs_wdr *fswdr)
{
    ot_ext_system_wdr_en_write(vi_pipe, fswdr->wdr_en);

    isp_fswdr_attr_write(vi_pipe, &fswdr->mpi_cfg);

    ot_ext_system_wdr_noise_model_coef_write(vi_pipe, fswdr->noise_model_coef);

    ot_ext_system_fusion_thr_write(vi_pipe, 0, fswdr->fusion_barrier0);
    ot_ext_system_fusion_thr_write(vi_pipe, 1, fswdr->fusion_barrier1);

    ot_ext_system_wdr_coef_update_en_write(vi_pipe, TD_TRUE);
}

static td_void frame_wdr_static_regs_initialize_inner(ot_vi_pipe vi_pipe,
    isp_fswdr_static_cfg *static_reg_cfg)
{
    td_u32 saturate_low, saturate_hig;
    td_s32 blc_value;
    td_s32 m_max_value_in = isp_bitmask(OT_WDR_BITDEPTH);
    td_u16 seg_idx_base[WDR_SIGMA_LUT_SEG_NUM] = { 0, 0, 64, 128, 192, 256, 320, 384 }; /* u10.0, max value = 512 */
    td_u16 seg_max_val[WDR_SIGMA_LUT_SEG_NUM] =  { 0, 2,  4,  8, 16,  32,  64, 128 }; /* u8.0 */
    td_u16 i;
    ot_isp_cmos_black_level *sns_black_level = TD_NULL;
    isp_sensor_get_blc(vi_pipe, &sns_black_level);
    isp_check_pointer_void_return(sns_black_level);

    blc_value = (td_s32)(sns_black_level->auto_attr.black_level[0][0]);
    static_reg_cfg->save_blc         = OT_ISP_WDR_SAVE_BLC_EN_DEFAULT;
    static_reg_cfg->gray_scale_mode  = OT_ISP_WDR_GRAYSCALE_DEFAULT;
    static_reg_cfg->mask_similar_thr = OT_ISP_WDR_MASK_SIMILAR_THR_DEFAULT;
    static_reg_cfg->mask_similar_cnt = OT_ISP_WDR_MASK_SIMILAR_CNT_DEFAULT;
    static_reg_cfg->dft_wgt_fl       = OT_ISP_WDR_DFTWGT_FL_DEFAULT;
    static_reg_cfg->bldr_lhf_idx     = OT_ISP_WDR_BLDRLHFIDX_DEFAULT;

    static_reg_cfg->saturate_thr     = OT_ISP_WDR_SATURATE_THR_DEFAULT;
    saturate_hig = ((td_u32)(m_max_value_in - blc_value));
    saturate_low = wdr_sqrt(saturate_hig, OT_ISP_WDR_SQRT_OUT_BITS);
    static_reg_cfg->saturate_thr        = (td_u16)(saturate_hig - saturate_low);

    static_reg_cfg->fusion_saturate_thd = static_reg_cfg->saturate_thr;

    static_reg_cfg->force_long_smooth_en = OT_ISP_WDR_FORCELONG_SMOOTH_EN;

    for (i = 0; i < WDR_SIGMA_LUT_SEG_NUM; i++) {
        static_reg_cfg->seg_idx_base[i] = seg_idx_base[i];
        static_reg_cfg->seg_max_val[i]  = (td_u8)seg_max_val[i];
    }

    static_reg_cfg->resh                 = TD_TRUE;
    static_reg_cfg->first_frame          = TD_TRUE;
}

static td_void frame_wdr_expo_value_get(ot_vi_pipe vi_pipe, td_u32 *expo_value_lut, td_u32 *ratio,
                                        td_u8 array_length, const isp_fs_wdr *fswdr)
{
    td_u8 j;
    td_u32 expo_value_max = isp_bitmask(11); /* max 11 bits */
    isp_usr_ctx         *isp_ctx = TD_NULL;
    ot_isp_cmos_default *sns_dft = TD_NULL;

    isp_sensor_get_default(vi_pipe, &sns_dft);
    isp_get_ctx(vi_pipe, isp_ctx);

    isp_check_pointer_void_return(sns_dft);

    for (j = 0; j < array_length; j++) {
        ratio[j] = clip3(sns_dft->wdr_switch_attr.exp_ratio[j], 0x40, 0x3FFF);
    }

    if (is_2to1_wdr_mode(isp_ctx->sns_wdr_mode)) {
        if (fswdr->mpi_cfg.wdr_merge_mode == OT_ISP_MERGE_WDR_MODE) {
            expo_value_lut[1] = 0x40;
            expo_value_lut[0] = MIN2(ratio[0], expo_value_max);
        } else {
            expo_value_lut[1] = 0x40;
            expo_value_lut[0] = MIN2((ratio[0] + 0x40), expo_value_max);
        }
    } else {
        expo_value_lut[1] = 0;
        expo_value_lut[0] = 0;
    }
}

static td_void frame_wdr_static_regs_initialize(ot_vi_pipe vi_pipe, td_u8 wdr_mode,
                                                isp_fswdr_static_cfg *static_reg, isp_fs_wdr *fswdr)
{
    td_u32 expo_value_lut[OT_ISP_EXP_RATIO_NUM] = { 0 };
    td_u32 ratio_lut[OT_ISP_EXP_RATIO_NUM] = { 0 };
    td_u16 r_ratio_max = isp_bitmask(WDR_R_RATIO_BITS);
    td_u16 i = 0;

    frame_wdr_expo_value_get(vi_pipe, expo_value_lut, ratio_lut, OT_ISP_EXP_RATIO_NUM,  fswdr);

    if (is_2to1_wdr_mode(wdr_mode)) {
        static_reg->expo_value_lut[0] = (td_u16)expo_value_lut[0];
        static_reg->expo_value_lut[1] = (td_u16)expo_value_lut[1];

        static_reg->expo_r_ratio_lut[0] = (td_u16)(MIN2(r_ratio_max, r_ratio_max * 0x40 / div_0_to_1(ratio_lut[0])));
        static_reg->expo_r_ratio_lut[1] = 0;

        static_reg->flick_exp_ratio  = (td_u16)ratio_lut[0];
        static_reg->blc_comp_lut[0] = expo_value_lut[0] - expo_value_lut[1];
    } else {
        static_reg->expo_value_lut[0]   = 0;
        static_reg->expo_value_lut[1]   = 0;

        static_reg->expo_r_ratio_lut[0] = 0;
        static_reg->expo_r_ratio_lut[1] = 0;

        static_reg->flick_exp_ratio  = 0;
        static_reg->blc_comp_lut[0] = 0;
    }

    for (i = 0; i < BCOM_BDEC_NODE_NUMBER; i++) {
        static_reg->gamma_bcom_non_unit_lut[i] = gamma_bcom_non_unit_lut[i];
        static_reg->gamma_bdec_non_unit_lut[i] = gamma_bdec_non_unit_lut[i];
        }
    for (i = 0; i < BCOM_BDEC_OUTSEG_NUMBER; i++) {
        static_reg->gamma_bcom_non_unit_pos_lut[i] = gamma_bcom_non_unit_pos_lut[i];
        static_reg->gamma_bcom_non_unit_seg_lut[i] = gamma_bcom_non_unit_seg_lut[i];
        static_reg->gamma_bcom_non_unit_step_lut[i] = gamma_bcom_non_unit_step_lut[i];
        static_reg->gamma_bdec_non_unit_pos_lut[i] = gamma_bdec_non_unit_pos_lut[i];
        static_reg->gamma_bdec_non_unit_seg_lut[i] = gamma_bdec_non_unit_seg_lut[i];
        static_reg->gamma_bdec_non_unit_step_lut[i] = gamma_bdec_non_unit_step_lut[i];
    }

    static_reg->max_ratio = WDR_MAX_VALUE / div_0_to_1(expo_value_lut[0]);

    frame_wdr_static_regs_initialize_inner(vi_pipe, static_reg);
}

static td_void frame_wdr_usr_regs_initialize(isp_fswdr_usr_cfg *usr_reg_cfg, isp_fs_wdr *fswdr)
{
    usr_reg_cfg->fusion_mode        = (fswdr->mpi_cfg.wdr_merge_mode == OT_ISP_MERGE_FUSION_MODE);
    usr_reg_cfg->short_expo_chk     = fswdr->mpi_cfg.wdr_combine.wdr_mdt.short_expo_chk;
    usr_reg_cfg->mdt_l_bld          = fswdr->mpi_cfg.wdr_combine.wdr_mdt.mdt_long_blend;
    usr_reg_cfg->mdt_still_thr      = fswdr->mpi_cfg.wdr_combine.wdr_mdt.mdt_still_threshold;
    usr_reg_cfg->mdt_full_thr       = fswdr->mpi_cfg.wdr_combine.wdr_mdt.mdt_full_threshold;
    usr_reg_cfg->pixel_avg_max_diff = OT_ISP_WDR_PIXEL_AVG_MAX_DIFF_DEFAULT;
    usr_reg_cfg->resh               = TD_TRUE;
    usr_reg_cfg->update_index       = 1;
}

static td_void frame_wdr_sync_regs_initialize(isp_fswdr_sync_cfg *sync_reg_cfg, isp_fs_wdr *fswdr)
{
    sync_reg_cfg->fusion_mode     = (fswdr->mpi_cfg.wdr_merge_mode == OT_ISP_MERGE_FUSION_MODE);
    sync_reg_cfg->wdr_mdt_en      = fswdr->mpi_cfg.wdr_combine.motion_comp;
    sync_reg_cfg->short_thr    = fswdr->short_thr_reg;
    sync_reg_cfg->long_thr     = fswdr->long_thr_reg;
}

static td_void frame_wdr_dyna_regs_initialize_first(td_u8 wdr_mode, isp_fswdr_dyna_cfg *dyna_reg_cfg)
{
    td_u8   frm_merge = 1;

    if (is_fs_wdr_mode(wdr_mode) || is_built_in_wdr_mode(wdr_mode)) {
        dyna_reg_cfg->bcom_en    = TD_TRUE;
        dyna_reg_cfg->bdec_en    = TD_TRUE;
    } else {
        dyna_reg_cfg->bcom_en    = TD_FALSE;
        dyna_reg_cfg->bdec_en    = TD_FALSE;
    }
    if (is_2to1_wdr_mode(wdr_mode)) {
        frm_merge = WDR_2M1_FRAME;
    } else if (is_3to1_wdr_mode(wdr_mode)) {
        frm_merge = WDR_3M1_FRAME;
    } else if (is_4to1_wdr_mode(wdr_mode)) {
        frm_merge = WDR_4M1_FRAME;
    }
    dyna_reg_cfg->frm_merge = frm_merge;
}

static td_void frame_wdr_dyna_regs_initialize_second(ot_vi_pipe vi_pipe,
    isp_fswdr_dyna_cfg *dyna_reg_cfg, isp_fs_wdr *fswdr)
{
    td_s32  i;
    ot_isp_cmos_default *sns_dft = TD_NULL;
    isp_sensor_get_default(vi_pipe, &sns_dft);
    isp_check_pointer_void_return(sns_dft);
    dyna_reg_cfg->wdr_mdt_en     = fswdr->mpi_cfg.wdr_combine.motion_comp;
    dyna_reg_cfg->short_thr      = fswdr->short_thr_reg;
    dyna_reg_cfg->long_thr       = fswdr->long_thr_reg;
    dyna_reg_cfg->erosion_en     = fswdr->erosion_en;

    dyna_reg_cfg->still_thr_lut[0]   = OT_ISP_WDR_STILL_THR0_DEFAULT;
    dyna_reg_cfg->still_thr_lut[1]   = OT_ISP_WDR_STILL_THR0_DEFAULT;

    dyna_reg_cfg->md_thr_hig_gain = 0x40;
    dyna_reg_cfg->md_thr_low_gain = 0x40;

    dyna_reg_cfg->fusion_thr_lut[0]  = fswdr->mpi_cfg.fusion_attr.fusion_threshold[0];
    dyna_reg_cfg->fusion_thr_lut[1]  = fswdr->mpi_cfg.fusion_attr.fusion_threshold[1];
    dyna_reg_cfg->fusion_blend_en    = fswdr->mpi_cfg.fusion_attr.fusion_blend_en;
    dyna_reg_cfg->fusion_blend_wgt   = fswdr->mpi_cfg.fusion_attr.fusion_blend_wgt;

    dyna_reg_cfg->fusion_lf_threshold =
        MIN2(isp_bitmask(FUSION_THR_BIT), fswdr->mpi_cfg.fusion_attr.fusion_threshold[1]);

    dyna_reg_cfg->force_long         = fswdr->mpi_cfg.wdr_combine.force_long;
    dyna_reg_cfg->force_long_low_thr = fswdr->mpi_cfg.wdr_combine.force_long_low_threshold;
    dyna_reg_cfg->force_long_hig_thr = fswdr->mpi_cfg.wdr_combine.force_long_hig_threshold;

    dyna_reg_cfg->short_check_thd   = fswdr->mpi_cfg.wdr_combine.wdr_mdt.short_check_threshold;

    if (sns_dft->wdr_switch_attr.exp_ratio[0] < WDR_FUSION_RATIO_THR) {
        dyna_reg_cfg->fusion_data_mode = 0x1;
        dyna_reg_cfg->pixel_fusion_data_select = 0x1;
    } else {
        dyna_reg_cfg->fusion_data_mode = 0x0;
        dyna_reg_cfg->pixel_fusion_data_select = 0x0;
    }

    dyna_reg_cfg->wdr_lpf_shift = 0x4;
    for (i = 0; i < 9; i++) { /* coef num 9 */
        dyna_reg_cfg->wdr_lpf_coef[i] = g_lpf_coef[i];
    }
}

static td_void frame_wdr_dyna_regs_initialize(ot_vi_pipe vi_pipe, td_u8 wdr_mode,
    isp_fswdr_dyna_cfg *dyna_reg_cfg, isp_fs_wdr *fswdr)
{
    td_u16 i, index;
    frame_wdr_dyna_regs_initialize_first(wdr_mode, dyna_reg_cfg);
    frame_wdr_dyna_regs_initialize_second(vi_pipe, dyna_reg_cfg, fswdr);

    for (i = 0; i < WDR_NOISE_LUT_NUM; i++) {
        if (i % 2 == 0) { /* index 2 */
            index = MIN2(i / 4, WDR_NOISE_NUM_EVEN - 1); /* 4 */
            dyna_reg_cfg->noise_profile[0][i] = g_noise_profile_even[index] << 3; /* 3 */
            dyna_reg_cfg->noise_profile[1][i] = g_noise_profile_even[index] << 3; /* 3 */
        } else {
            index = MIN2(i / 4, WDR_NOISE_NUM_ODD - 1); /* 4 */
            dyna_reg_cfg->noise_profile[0][i] = g_noise_profile_odd[index] << 3; /* 3 */
            dyna_reg_cfg->noise_profile[1][i] = g_noise_profile_odd[index] << 3; /* 3 */
        }
    }

    for (i = 0; i < WDR_SIGMA_LUT_NUM; i++)  {
        dyna_reg_cfg->sigma_nml_lut[i] = (td_u16)(MIN2((g_nrm_lut[i] >> 4), 65535)); /* shift 4bits, max: 65535 */
    }

    dyna_reg_cfg->wdr_revmode       = 0;
    dyna_reg_cfg->sigma_mode_select = 1;
    dyna_reg_cfg->avg_mode_select   = 1;

    dyna_reg_cfg->fusion_thr_r_lut[0]  = OT_ISP_WDR_FUSION_F0_THR_R;
    dyna_reg_cfg->fusion_thr_r_lut[1]  = OT_ISP_WDR_FUSION_F1_THR_R;

    dyna_reg_cfg->fusion_thr_g_lut[0]  = OT_ISP_WDR_FUSION_F0_THR_G;
    dyna_reg_cfg->fusion_thr_g_lut[1]  = OT_ISP_WDR_FUSION_F1_THR_G;

    dyna_reg_cfg->fusion_thr_b_lut[0]  = OT_ISP_WDR_FUSION_F0_THR_B;
    dyna_reg_cfg->fusion_thr_b_lut[1]  = OT_ISP_WDR_FUSION_F1_THR_B;

    dyna_reg_cfg->force_long         = OT_ISP_WDR_FORCELONG_EN_DEFAULT;
    dyna_reg_cfg->force_long_low_thr = OT_ISP_WDR_FORCELONG_LOW_THD_DEFAULT;
    dyna_reg_cfg->force_long_hig_thr = OT_ISP_WDR_FORCELONG_HIGH_THD_DEFAULT;

    dyna_reg_cfg->short_check_thd    = ISP_WDR_SHORTCHK_THD_DEFAULT;
    dyna_reg_cfg->fusion_data_mode   = OT_ISP_WDR_FUSION_DATA_MODE;

    dyna_reg_cfg->mdt_nf_low_thr  = OT_ISP_WDR_MDT_NOSF_LOW_THR_DEFAULT;
    dyna_reg_cfg->mdt_nf_high_thr = OT_ISP_WDR_MDT_NOSF_HIG_THR_DEFAULT;

    dyna_reg_cfg->gain_sum_high_thr = OT_ISP_WDR_GAIN_SUM_HIG_THR_DEFAULT;
    dyna_reg_cfg->gain_sum_low_thr  = OT_ISP_WDR_GAIN_SUM_LOW_THR_DEFAULT;

    dyna_reg_cfg->force_long_slope = OT_ISP_WDR_FORCELONG_SLOPE_DEFAULT;
    dyna_reg_cfg->wgt_slope        = OT_ISP_WDR_WGT_SLOPE_DEFAULT;

    dyna_reg_cfg->resh              = TD_TRUE;
}

static td_void frame_wdr_regs_initialize(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg, isp_fs_wdr *fswdr)
{
    td_u8 wdr_mode, i, block_num;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_get_ctx(vi_pipe, isp_ctx);

    wdr_mode   = isp_ctx->sns_wdr_mode;
    block_num  = isp_ctx->block_attr.block_num;

    for (i = 0; i < block_num; i++) {
        frame_wdr_static_regs_initialize(vi_pipe, wdr_mode, &reg_cfg->alg_reg_cfg[i].wdr_reg_cfg.static_reg_cfg, fswdr);
        frame_wdr_usr_regs_initialize(&reg_cfg->alg_reg_cfg[i].wdr_reg_cfg.usr_reg_cfg, fswdr);
        frame_wdr_dyna_regs_initialize(vi_pipe, wdr_mode, &reg_cfg->alg_reg_cfg[i].wdr_reg_cfg.dyna_reg_cfg, fswdr);
        frame_wdr_sync_regs_initialize(&reg_cfg->alg_reg_cfg[i].wdr_reg_cfg.sync_reg_cfg, fswdr);
        reg_cfg->alg_reg_cfg[i].wdr_reg_cfg.wdr_en = fswdr->wdr_en;
    }

    reg_cfg->cfg_key.bit1_fs_wdr_cfg = 1;
}


static td_void isp_fswdr_thr_reg_update(isp_fs_wdr *fs_wdr_ctx)
{
    const td_u8 wdr_thr_shift = 0;
    const td_u8 wdr_thr_offset = 0;
    td_u16 short_thr, long_thr, force_long_hig_thr, force_long_low_thr;

    short_thr = fs_wdr_ctx->mpi_cfg.wdr_combine.short_threshold;
    if (short_thr != 0) {
        fs_wdr_ctx->short_thr_reg = (short_thr << wdr_thr_shift) + wdr_thr_offset;
    } else {
        fs_wdr_ctx->short_thr_reg = short_thr;
    }
    long_thr = fs_wdr_ctx->mpi_cfg.wdr_combine.long_threshold;
    if (long_thr != 0) {
        fs_wdr_ctx->long_thr_reg = (long_thr << wdr_thr_shift) + wdr_thr_offset;
    } else {
        fs_wdr_ctx->long_thr_reg = long_thr;
    }

    force_long_hig_thr = fs_wdr_ctx->mpi_cfg.wdr_combine.force_long_hig_threshold;
    if (force_long_hig_thr != 0) {
        fs_wdr_ctx->force_long_hig_thr_reg = (force_long_hig_thr << wdr_thr_shift) + wdr_thr_offset;
    } else {
        fs_wdr_ctx->force_long_hig_thr_reg = force_long_hig_thr;
    }
    force_long_low_thr = fs_wdr_ctx->mpi_cfg.wdr_combine.force_long_low_threshold;
    if (force_long_low_thr != 0) {
        fs_wdr_ctx->force_long_low_thr_reg = (force_long_low_thr << wdr_thr_shift) + wdr_thr_offset;
    } else {
        fs_wdr_ctx->force_long_low_thr_reg = force_long_low_thr;
    }
}
static td_void frame_wdr_read_ext_regs(ot_vi_pipe vi_pipe, isp_fs_wdr *fs_wdr_ctx)
{
    fs_wdr_ctx->coef_update_en = ot_ext_system_wdr_coef_update_en_read(vi_pipe);
    if (fs_wdr_ctx->coef_update_en) {
        ot_ext_system_wdr_coef_update_en_write(vi_pipe, TD_FALSE);
        fs_wdr_ctx->wdr_en           = ot_ext_system_wdr_en_read(vi_pipe);
        fs_wdr_ctx->noise_model_coef = ot_ext_system_wdr_noise_model_coef_read(vi_pipe);

        isp_fswdr_attr_read(vi_pipe, &fs_wdr_ctx->mpi_cfg);

        isp_fswdr_thr_reg_update(fs_wdr_ctx);

        fs_wdr_ctx->fusion_barrier0 = ot_ext_system_fusion_thr_read(vi_pipe, OT_ISP_WDR_MERGE_FRAME_VS);
        fs_wdr_ctx->fusion_barrier1 = ot_ext_system_fusion_thr_read(vi_pipe, OT_ISP_WDR_MERGE_FRAME_S);
        fs_wdr_ctx->fusion_barrier2 = ot_ext_system_fusion_thr_read(vi_pipe, OT_ISP_WDR_MERGE_FRAME_M);
        fs_wdr_ctx->fusion_barrier3 = ot_ext_system_fusion_thr_read(vi_pipe, OT_ISP_WDR_MERGE_FRAME_L);
    }

    return;
}

static td_void isp_cal_wdr_image_size(isp_reg_cfg *reg_cfg, isp_usr_ctx *isp_ctx, td_u8 block_num,
                                      td_u32 with_imp, td_u32 height_imp)
{
    td_u32 w_div;
    td_u32 w_div_acc = 0;
    td_u32 i;
    isp_fswdr_dyna_cfg *dyna_reg_cfg = TD_NULL;
    isp_rect block_rect;
    isp_block_attr *block = TD_NULL;

    block = &isp_ctx->block_attr;

    for (i = 0; i < block->block_num; i++) {
        dyna_reg_cfg = &reg_cfg->alg_reg_cfg[i].wdr_reg_cfg.dyna_reg_cfg;
        isp_get_block_rect(&block_rect, &isp_ctx->block_attr, (td_u8)i);

        dyna_reg_cfg->wdr_split_num = block->block_num;

        if ((i == 0) || (i == (td_u32)(block->block_num - 1))) {
            w_div = block_rect.width - block->over_lap;
        } else {
            w_div = block_rect.width - 2 * block->over_lap; /* 2 is a multiple */
        }

        dyna_reg_cfg->wdr_blk_idx = (td_u8)i;

        dyna_reg_cfg->wdr_stat_startx = (td_u16)((i > 0) ? block->over_lap : 0);
        dyna_reg_cfg->wdr_stat_endx = (td_u16)(
            (i < (td_u32)(block->block_num - 1)) ? (block_rect.width - block->over_lap - 1) : (block_rect.width - 1));

        if (i < WDR_STAT_HOR_NUM % div_0_to_1(block_num)) {
            dyna_reg_cfg->wdr_ref_stat_hblk_num = WDR_STAT_HOR_NUM / div_0_to_1(block_num) + 1;
        } else {
            dyna_reg_cfg->wdr_ref_stat_hblk_num = WDR_STAT_HOR_NUM / div_0_to_1(block_num);
        }
        dyna_reg_cfg->wdr_reg_stat_vblk_num = WDR_STAT_VER_NUM;

        dyna_reg_cfg->wdr_stat_shift = (td_u8)(i * dyna_reg_cfg->wdr_ref_stat_hblk_num);

        /* calc */
        dyna_reg_cfg->wdr_cur_calc_hblk_num = WDR_STAT_HOR_NUM;
        dyna_reg_cfg->wdr_cur_calc_vblk_num = WDR_STAT_VER_NUM;

        dyna_reg_cfg->wdr_kx = (td_u16)((WDR_STAT_HOR_NUM << WDR_IMAGE_SHIFT) / div_0_to_1(with_imp));
        dyna_reg_cfg->wdr_ky = (td_u16)((WDR_STAT_VER_NUM << WDR_IMAGE_SHIFT) / div_0_to_1(height_imp));
        dyna_reg_cfg->wdr_smlmap_offset = (w_div_acc - dyna_reg_cfg->wdr_stat_startx) * dyna_reg_cfg->wdr_kx;

        w_div_acc += w_div;
    }
}

static td_void isp_set_wdr_image_size(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg)
{
    td_u8 block_num;
    td_u32 with_imp;
    td_u32 height_imp;

    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_get_ctx(vi_pipe, isp_ctx);

    /* ca_l_cul for */
    block_num = isp_ctx->block_attr.block_num;
    with_imp   = isp_ctx->block_attr.frame_rect.width;
    height_imp = isp_ctx->block_attr.frame_rect.height;

    isp_cal_wdr_image_size(reg_cfg, isp_ctx, block_num, with_imp, height_imp);
}

static td_void frame_wdr_def_initialize(isp_fs_wdr *fswdr)
{
    td_u8  i, j;
    fswdr->mpi_cfg.wdr_merge_mode = OT_ISP_MERGE_WDR_MODE;
    /* wdr_combine */
    fswdr->mpi_cfg.wdr_combine.motion_comp              = OT_EXT_SYSTEM_MDT_EN_DEFAULT;
    fswdr->mpi_cfg.wdr_combine.short_threshold          = OT_EXT_SYSTEM_WDR_SHORTTHR_WRITE_DEFAULT;
    fswdr->mpi_cfg.wdr_combine.long_threshold           = OT_EXT_SYSTEM_WDR_LONGTHR_WRITE_DEFAULT;
    fswdr->mpi_cfg.wdr_combine.force_long               = OT_EXT_SYSTEM_WDR_FORCELONG_EN;
    fswdr->mpi_cfg.wdr_combine.force_long_low_threshold = OT_EXT_SYSTEM_WDR_FORCELONG_LOW_THD;
    fswdr->mpi_cfg.wdr_combine.force_long_hig_threshold = OT_EXT_SYSTEM_WDR_FORCELONG_HIGH_THD;

    fswdr->mpi_cfg.wdr_combine.wdr_mdt.short_expo_chk        = OT_ISP_WDR_SHORT_EXPO_CHK_DEFAULT;
    fswdr->mpi_cfg.wdr_combine.wdr_mdt.short_check_threshold = OT_EXT_SYSTEM_WDR_SHORTCHECK_THD;
    fswdr->mpi_cfg.wdr_combine.wdr_mdt.mdt_still_threshold   = OT_EXT_SYSTEM_WDR_MDT_STILL_THR_DEFAULT;
    fswdr->mpi_cfg.wdr_combine.wdr_mdt.mdt_full_threshold    = OT_EXT_SYSTEM_WDR_MDT_FULL_THR_DEFAULT;
    fswdr->mpi_cfg.wdr_combine.wdr_mdt.mdt_long_blend        = OT_EXT_SYSTEM_WDR_MDT_LONG_BLEND_DEFAULT;
    fswdr->mpi_cfg.wdr_combine.wdr_mdt.op_type               = OT_OP_MODE_AUTO;
    fswdr->mpi_cfg.wdr_combine.wdr_mdt.manual_attr.md_thr_low_gain = 0x40;
    fswdr->mpi_cfg.wdr_combine.wdr_mdt.manual_attr.md_thr_hig_gain = 0x40;

    for (j = 0; j < OT_ISP_WDR_RATIO_NUM; j++) {
        for (i = 0; i < OT_ISP_AUTO_ISO_NUM; i++) {
            fswdr->mpi_cfg.wdr_combine.wdr_mdt.auto_attr.md_thr_low_gain[j][i] = g_lut_mdt_low_thr[j][i];
            fswdr->mpi_cfg.wdr_combine.wdr_mdt.auto_attr.md_thr_hig_gain[j][i] = g_lut_mdt_hig_thr[j][i];
        }
    }

    /* fusion_attr */
    for (i = 0; i < OT_ISP_WDR_MAX_FRAME_NUM; i++) {
        fswdr->mpi_cfg.fusion_attr.fusion_threshold[i] = g_fusion_thr[i];
    }
    fswdr->mpi_cfg.fusion_attr.fusion_blend_en = OT_ISP_WDR_FUSION_BLEND_EN_DEFAULT;
 //   fswdr->mpi_cfg.fusion_attr.pixel_fusion_en = OT_ISP_WDR_FUSION_PIXEL_EN_FUSION;
    fswdr->mpi_cfg.fusion_attr.fusion_blend_wgt = OT_ISP_WDR_FUSION_BLEND_WGT;
}

static td_s32 frame_wdr_initialize(ot_vi_pipe vi_pipe, isp_fs_wdr *fswdr)
{
    td_s32 ret, i, j;
    isp_usr_ctx *isp_ctx = TD_NULL;
    ot_isp_cmos_default *sns_dft = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);
    isp_sensor_get_default(vi_pipe, &sns_dft);
    isp_check_pointer_return(sns_dft);

    if (is_linear_mode(isp_ctx->sns_wdr_mode) || is_built_in_wdr_mode(isp_ctx->sns_wdr_mode)) {
        fswdr->wdr_en = TD_FALSE;
    } else {
        fswdr->wdr_en = TD_TRUE;
    }

    fswdr->bit_depth_prc   = WDR_BITDEPTH;
    fswdr->pre_again       = 0;

    fswdr->erosion_en       = TD_FALSE;
    fswdr->noise_model_coef = OT_EXT_SYSTEM_WDR_NOISE_MODEL_COEF_DEFAULT;

    for (i = 0; i < WDR_NOISESET_ELENUM; i++) {
        fswdr->floor_set_lut[i] = g_noise_floor_set[i];
        fswdr->again_set_lut[i] = g_noise_again_set[i];
    }

    if (sns_dft->key.bit1_wdr) {
        isp_check_pointer_return(sns_dft->wdr);

        ret = isp_fswdr_attr_check("cmos", sns_dft->wdr);
        if (ret != TD_SUCCESS) {
            return ret;
        }

        fswdr->fusion_barrier0 = sns_dft->wdr->fusion_attr.fusion_threshold[0];
        fswdr->fusion_barrier1 = sns_dft->wdr->fusion_attr.fusion_threshold[1];

        (td_void)memcpy_s(&fswdr->mpi_cfg, sizeof(ot_isp_wdr_fs_attr), sns_dft->wdr, sizeof(ot_isp_wdr_fs_attr));
    } else {
        frame_wdr_def_initialize(fswdr);

        fswdr->fusion_barrier0 = g_fusion_thr[0];
        fswdr->fusion_barrier1 = g_fusion_thr[1];
    }

    if (fswdr->mpi_cfg.wdr_combine.short_threshold != 0) {
        fswdr->short_thr_reg = (fswdr->mpi_cfg.wdr_combine.short_threshold << 2) + 3; /* index 2 3 */
    } else {
        fswdr->short_thr_reg = fswdr->mpi_cfg.wdr_combine.short_threshold;
    }

    if (fswdr->mpi_cfg.wdr_combine.long_threshold != 0) {
        fswdr->long_thr_reg = (fswdr->mpi_cfg.wdr_combine.long_threshold << 2) + 3; /* index 2 3 */
    } else {
        fswdr->long_thr_reg = fswdr->mpi_cfg.wdr_combine.long_threshold;
    }

    for (i = 0; i < OT_ISP_STRIPING_MAX_NUM; i++) {
        for (j = 0; j < CFG2VLD_DLY_LIMIT; j++) {
            fswdr->sync_bcom_en[i][j] = TD_TRUE;
            fswdr->sync_bdec_en[i][j] = TD_TRUE;
        }
    }
    return TD_SUCCESS;
}

static td_s32 isp_frame_wdr_param_init(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg, isp_fs_wdr *fswdr)
{
    td_s32 ret;
    fswdr->init = TD_FALSE;
    ret = frame_wdr_initialize(vi_pipe, fswdr);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    isp_set_wdr_image_size(vi_pipe, reg_cfg);
    frame_wdr_ext_regs_initialize(vi_pipe, fswdr);
    frame_wdr_regs_initialize(vi_pipe, reg_cfg, fswdr);

    fswdr->init = TD_TRUE;
    ot_ext_system_isp_fswdr_init_status_write(vi_pipe, fswdr->init);
    return TD_SUCCESS;
}

static td_s32 isp_frame_wdr_init(ot_vi_pipe vi_pipe, td_void *cfg)
{
    td_s32 ret;
    isp_fs_wdr *fswdr = TD_NULL;
    ot_ext_system_isp_fswdr_init_status_write(vi_pipe, TD_FALSE);
    ret = frame_wdr_ctx_init(vi_pipe);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    fs_wdr_get_ctx(vi_pipe, fswdr);
    isp_check_pointer_return(fswdr);

    return isp_frame_wdr_param_init(vi_pipe, (isp_reg_cfg *)cfg, fswdr);
}

static td_s32 isp_frame_wdr_switch_mode(ot_vi_pipe vi_pipe, td_void *cfg)
{
    td_u8  i;
    td_s32 ret;
    isp_reg_cfg *reg_cfg = (isp_reg_cfg *)cfg;

    for (i = 0; i < reg_cfg->cfg_num; i++) {
        reg_cfg->alg_reg_cfg[i].wdr_reg_cfg.wdr_en = TD_FALSE;
    }
    reg_cfg->cfg_key.bit1_fs_wdr_cfg = 1;

    ret = isp_frame_wdr_init(vi_pipe, reg_cfg);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    for (i = 0; i < reg_cfg->cfg_num; i++) {
        reg_cfg->alg_reg_cfg[i].wdr_reg_cfg.static_reg_cfg.first_frame = TD_TRUE;
    }
    return TD_SUCCESS;
}

static td_s32 isp_frame_res_switch_mode(ot_vi_pipe vi_pipe, td_void *cfg)
{
    td_u8 i;
    td_s32 ret;
    isp_reg_cfg *reg_cfg = (isp_reg_cfg *)cfg;
    isp_usr_ctx *isp_ctx = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);

    if (is_linear_mode(isp_ctx->sns_wdr_mode) || is_built_in_wdr_mode(isp_ctx->sns_wdr_mode)) {
        return TD_SUCCESS;
    }

    for (i = 0; i < reg_cfg->cfg_num; i++) {
        reg_cfg->alg_reg_cfg[i].wdr_reg_cfg.wdr_en = TD_FALSE;
    }
    reg_cfg->cfg_key.bit1_fs_wdr_cfg = 1;

    ret = isp_frame_wdr_init(vi_pipe, reg_cfg);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    for (i = 0; i < reg_cfg->cfg_num; i++) {
        reg_cfg->alg_reg_cfg[i].wdr_reg_cfg.static_reg_cfg.first_frame = TD_TRUE;
    }
    return TD_SUCCESS;
}

static __inline td_bool  check_wdr_open(const isp_fs_wdr *fs_wdr)
{
    return (fs_wdr->wdr_en == TD_TRUE);
}

static td_void check_wdr_mode(ot_vi_pipe vi_pipe, isp_fs_wdr *fs_wdr)
{
    td_u8  wdr_mode;
    isp_usr_ctx *isp_ctx = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);

    wdr_mode = isp_ctx->sns_wdr_mode;

    if (is_linear_mode(wdr_mode) || is_built_in_wdr_mode(wdr_mode)) {
        fs_wdr->wdr_en = TD_FALSE;
    }
}

static td_s32 get_exp_ratio_index(td_s32 exp_ratio, const td_s32 *exp_lut)
{
    td_s32 index;

    for (index = 0; index < OT_ISP_WDR_RATIO_NUM - 1; index++) {
        if (exp_ratio <= exp_lut[index]) {
            break;
        }
    }

    return index;
}

static td_s32 get_value_from_lut(td_u32 iso_in, td_u32 exp_ratio_in, td_u8 **lut)
{
    td_s32 output;
    td_s32 ratio_idx_low, ratio_idx_high;
    td_u8 iso_idx_low, iso_idx_high;
    td_s32 tmp1, tmp2;
    td_s32 iso, exp_ratio, iso1, iso2;

    iso = (td_s32)(clip3(iso_in, 100, 3276800)); /* 100-iso low thr,3276800-iso high thr */
    exp_ratio = (td_s32)(clip3(exp_ratio_in, 64, 4096)); /* 64-ratio low thr,4096 - ratio high thr */
    /* a(exp_idx_low, iso_idx_low)   b(exp_idx_low, iso_idx_high)

       c(exp_idx_high, iso_idx_low)  d(exp_idx_high, iso_idx_high)
       a and b -> tmp1, c and d -> tmp2
    */
    ratio_idx_high = get_exp_ratio_index(exp_ratio, g_ratio_thd);
    ratio_idx_low  = MAX2(ratio_idx_high - 1, 0);
    iso_idx_high = get_iso_index((td_u32)iso);
    iso_idx_low  = MAX2(iso_idx_high - 1, 0);
    iso1 = (td_s32)get_iso(iso_idx_low);
    iso2 = (td_s32)get_iso(iso_idx_high);

    tmp1 = linear_inter(iso, iso1, lut[ratio_idx_low][iso_idx_low],
                        iso2, lut[ratio_idx_low][iso_idx_high]);

    tmp2 = linear_inter(iso, iso1, lut[ratio_idx_high][iso_idx_low],
                        iso2, lut[ratio_idx_high][iso_idx_high]);

    output = linear_inter(exp_ratio, g_ratio_thd[ratio_idx_low], tmp1, g_ratio_thd[ratio_idx_high], tmp2);

    return output;
}

static td_u32 wdr_noise_floor_calc(td_u32 m_f_sensor_again)
{
    const td_u8  sensor_again_shift = 16;
    const td_u8  noise_gain_base = 64;
    td_u32 m_again_g;
    td_u32 m_noise_floor = 0;
    td_u32 i;
    td_u32 again_set_lut[NOISE_SET_ELE_NUM];
    td_u8  floor_set_lut[NOISE_SET_ELE_NUM];

    m_again_g = (td_u32)(m_f_sensor_again * noise_gain_base) >> sensor_again_shift;

    for (i = 0; i < NOISE_SET_ELE_NUM; i++) {
        again_set_lut[i] = g_noise_again_set[i] * noise_gain_base;
        floor_set_lut[i] = g_noise_floor_set[i];
    }

    /* noise floor interpolation */
    for (i = 0; i < (NOISE_SET_ELE_NUM - 1); i++) {
        if (m_again_g >= again_set_lut[i] && m_again_g <= again_set_lut[i + 1]) {
            m_noise_floor = floor_set_lut[i] +
                            ((floor_set_lut[i + 1] - floor_set_lut[i]) * (m_again_g - again_set_lut[i])) /
                            div_0_to_1(again_set_lut[i + 1] - again_set_lut[i]);
        }
    }

    return m_noise_floor;
}

static td_u32 wdr_rb_noise_floor_calc(td_u32 m_noise_floor, td_u32 m_noise_ratio_rg_bg_wgt)
{
    const td_u8  noise_floor_fix_value = 90;
    const td_u8  noise_floor_shift_factor1 = 6;
    const td_u8  noise_floor_shift_factor2 = 7;
    const td_u8  noise_floor_round_value1 = 32;
    const td_u8  noise_floor_round_value2 = 64;
    td_u32 f_rb_noise_floor;

    f_rb_noise_floor = ((((m_noise_floor * noise_floor_fix_value * m_noise_ratio_rg_bg_wgt + 1) >> 1) +
        noise_floor_round_value1) >> noise_floor_shift_factor1) +
        ((((m_noise_floor * m_noise_ratio_rg_bg_wgt + 1) >> 1) + noise_floor_round_value2) >>
        noise_floor_shift_factor2);

    return f_rb_noise_floor;
}

static td_void wdr_nr_param_cal(isp_fs_wdr *fs_wdr, isp_usr_ctx *isp_ctx, isp_fswdr_dyna_cfg *dyna_reg)
{
    td_u32 m_f_sensor_again = ((isp_ctx->linkage.again << WDR_SENSOR_GAIN_SHIFT_BITS) +
                               WDR_SENSOR_GAIN_ROUND) / WDR_SENSOR_GAIN_BASE;
    td_u32 m_f_sensor_dgain = ((isp_ctx->linkage.dgain << WDR_SENSOR_GAIN_SHIFT_BITS) + WDR_SENSOR_GAIN_ROUND) /
        WDR_SENSOR_GAIN_BASE + (1 << WDR_SENSOR_GAIN_SHIFT_BITS);
    td_u32 m_sqrt_again_g, m_sqrt_dgain_g;
    td_u32 m_noise_floor;
    td_u32 f_gnoisefloor, f_rnoisefloor, f_bnoisefloor;
    td_u16 nos_floor_g;
    const td_u8  sensor_sqrt_gain_shift = 8;
    const td_u8  sqrt_gain_max = 6;
    const td_u8  ratio_sqrt_bits = 11;
    const td_u8  mdt_nos_floor_bitdep = 7;
    const td_u8  wdr_sqrt_out_bits = 8;
    const td_u8  wdr_sqrt_round = 32;
    const td_u8  noise_floor_g_shift = 16;
    const td_u8  nos_floor_g_biedep = 9;
    const td_u8  t_nos_floor_bitdep = 12;
    td_u32 ratio = isp_ctx->linkage.exp_ratio;
    td_u32 tmp;

    ratio = clip3(ratio, WDR_EXPOSURE_BASE, WDR_RATIO_MAX);
    m_sqrt_again_g = (wdr_sqrt(m_f_sensor_again, WDR_SQRT_GAIN_BITDEP)) >> sensor_sqrt_gain_shift;
    m_sqrt_dgain_g = (wdr_sqrt(m_f_sensor_dgain, WDR_SQRT_GAIN_BITDEP)) >> sensor_sqrt_gain_shift;

    fs_wdr->pre_again = isp_ctx->linkage.again;

    m_noise_floor = wdr_noise_floor_calc(m_f_sensor_again);

    f_gnoisefloor = m_noise_floor;
    f_rnoisefloor = wdr_rb_noise_floor_calc(m_noise_floor, WDR_RG_WGT);
    f_bnoisefloor = wdr_rb_noise_floor_calc(m_noise_floor, WDR_BG_WGT);

    dyna_reg->t_nos_floor    = (td_u16)(MIN2(isp_bitmask(t_nos_floor_bitdep),
        ((f_gnoisefloor + f_rnoisefloor + f_bnoisefloor) * wdr_sqrt(m_f_sensor_dgain, WDR_SQRT_GAIN_BITDEP) +
        wdr_sqrt_round) >> wdr_sqrt_out_bits));
    tmp = 1 << (noise_floor_g_shift - 1);
    nos_floor_g = (td_u16)(MIN2(isp_bitmask(nos_floor_g_biedep),
        ((m_noise_floor * m_f_sensor_dgain + tmp)) >> noise_floor_g_shift));

    dyna_reg->mdt_nos_floor = (td_u8)(MIN2(isp_bitmask(mdt_nos_floor_bitdep),
        nos_floor_g * wdr_sqrt(wdr_sqrt(ratio, ratio_sqrt_bits), ratio_sqrt_bits)));

    dyna_reg->sqrt_again_g  = (td_u8)(MIN2(sqrt_gain_max, m_sqrt_again_g));
    dyna_reg->sqrt_dgain_g  = (td_u8)(MIN2(sqrt_gain_max, m_sqrt_dgain_g));
}

static td_void isp_wdr_fusion_thr_lut_calc(const isp_usr_ctx *isp_ctx, isp_fs_wdr *fs_wdr,
    isp_fswdr_dyna_cfg *dyna_reg)
{
    td_u8  i;
    td_u32 awb_r_gain = isp_ctx->linkage.white_balance_gain[WDR_AWB_RGAIN_INDEX] >> 0x8;
    td_u32 awb_g_gain = isp_ctx->linkage.white_balance_gain[WDR_AWB_GGAIN_INDEX] >> 0x8;
    td_u32 awb_b_gain = isp_ctx->linkage.white_balance_gain[WDR_AWB_BGAIN_INDEX] >> 0x8;
    td_u16 fusion_threshold;

    if (awb_r_gain != 0 && awb_b_gain != 0 && awb_g_gain != 0) {
        dyna_reg->fusion_thr_r_lut[0] = (td_u16)(MIN2(isp_bitmask(OT_WDR_THR_BIT), \
            (fs_wdr->fusion_barrier0 << OT_FUSION_THR_SHIFT_BIT) / div_0_to_1(awb_r_gain)));
        dyna_reg->fusion_thr_r_lut[1] = (td_u16)(MIN2(isp_bitmask(OT_WDR_THR_BIT), \
            (fs_wdr->fusion_barrier1 << OT_FUSION_THR_SHIFT_BIT) / div_0_to_1(awb_r_gain)));

        dyna_reg->fusion_thr_g_lut[0] = (td_u16)(MIN2(isp_bitmask(OT_WDR_THR_BIT), \
            (fs_wdr->fusion_barrier0 << OT_FUSION_THR_SHIFT_BIT) / div_0_to_1(awb_g_gain)));
        dyna_reg->fusion_thr_g_lut[1] = (td_u16)(MIN2(isp_bitmask(OT_WDR_THR_BIT), \
            (fs_wdr->fusion_barrier1 << OT_FUSION_THR_SHIFT_BIT) / div_0_to_1(awb_g_gain)));

        dyna_reg->fusion_thr_b_lut[0] = (td_u16)(MIN2(isp_bitmask(OT_WDR_THR_BIT), \
            (fs_wdr->fusion_barrier0 << OT_FUSION_THR_SHIFT_BIT) / div_0_to_1(awb_b_gain)));
        dyna_reg->fusion_thr_b_lut[1] = (td_u16)(MIN2(isp_bitmask(OT_WDR_THR_BIT), \
            (fs_wdr->fusion_barrier1 << OT_FUSION_THR_SHIFT_BIT) / div_0_to_1(awb_b_gain)));
    }

    for (i = 0; i < OT_ISP_WDR_MAX_FRAME_NUM; i++) {
        fusion_threshold = fs_wdr->mpi_cfg.fusion_attr.fusion_threshold[i];
        dyna_reg->fusion_thr_lut[i] = MIN2(isp_bitmask(FUSION_THR_BIT), fusion_threshold);
    }
    dyna_reg->fusion_blend_en = fs_wdr->mpi_cfg.fusion_attr.fusion_blend_en;
    dyna_reg->fusion_blend_wgt = fs_wdr->mpi_cfg.fusion_attr.fusion_blend_wgt;
    dyna_reg->fusion_lf_threshold = MIN2(isp_bitmask(FUSION_THR_BIT), fs_wdr->mpi_cfg.fusion_attr.fusion_threshold[1]);
}

static td_void isp_wdr_func_third(ot_vi_pipe vi_pipe, const isp_usr_ctx *isp_ctx, isp_fs_wdr *fs_wdr,
    isp_fswdr_dyna_cfg *dyna_reg)
{
    td_u32 still_exp_s_low, still_exp_s_hig;
    td_s32 blc_value;
    td_s32 m_max_value_in  = isp_bitmask(fs_wdr->bit_depth_prc);
    td_s32 i;
    ot_isp_cmos_default     *sns_dft         = TD_NULL;
    ot_isp_cmos_black_level *sns_black_level = TD_NULL;
    ot_isp_wdr_combine_attr *wdr_combine = &fs_wdr->mpi_cfg.wdr_combine;
    const td_u8 still_exp_shift = 6;

    isp_sensor_get_default(vi_pipe, &sns_dft);
    isp_sensor_get_blc(vi_pipe, &sns_black_level);

    isp_check_pointer_void_return(sns_dft);
    isp_check_pointer_void_return(sns_black_level);

    fs_wdr->actual_md_thr_low_gain = dyna_reg->md_thr_low_gain;
    fs_wdr->actual_md_thr_hig_gain = dyna_reg->md_thr_hig_gain;

    dyna_reg->erosion_en = fs_wdr->erosion_en;
    blc_value = sns_black_level->auto_attr.black_level[0][0];
    still_exp_s_hig = ((td_u32)(m_max_value_in - blc_value)) << still_exp_shift;
    still_exp_s_low = ((td_u32)wdr_sqrt((td_u32)(m_max_value_in - blc_value), WDR_SQRT_OUT_BITS));

    for (i = 0; i < MIN2(ISP_WDR_CHN_MAX - 1, dyna_reg->frm_merge - 1); i++) {
        dyna_reg->still_thr_lut[i] = (td_u16)(
            clip3(((td_s32)(still_exp_s_hig / div_0_to_1(isp_ctx->linkage.exp_ratio_lut[i])) - (td_s32)still_exp_s_low),
            0, isp_bitmask(WDR_THR_BIT)));
    }

    dyna_reg->force_long         = wdr_combine->force_long;
    dyna_reg->force_long_low_thr = MIN2(isp_bitmask(WDR_THR_BIT), fs_wdr->force_long_low_thr_reg);
    dyna_reg->force_long_hig_thr = MIN2(isp_bitmask(WDR_THR_BIT), fs_wdr->force_long_hig_thr_reg);
    dyna_reg->short_check_thd   = MIN2(isp_bitmask(WDR_THR_BIT), wdr_combine->wdr_mdt.short_check_threshold);

    dyna_reg->force_long_slope  = MIN2(isp_bitmask(OT_FUSION_LONG_SLOPE_BIT), \
        (FORCELONG_WEIGHT << FORCELONG_FRACBITS) / \
        div_0_to_1(dyna_reg->force_long_hig_thr - dyna_reg->force_long_low_thr)); /* u10.6 */
    dyna_reg->wgt_slope = fswdr_calc_wgt_slope(dyna_reg->short_thr, dyna_reg->long_thr);
    dyna_reg->resh = TD_TRUE;
}

static td_void isp_wdr_md_thr_calc(ot_vi_pipe vi_pipe, isp_usr_ctx *isp_ctx,
    isp_fs_wdr *fs_wdr, isp_fswdr_dyna_cfg *dyna_reg)
{
    td_u32 m_sqrt_again_g, m_sqrt_dgain_g, i;
    td_u8 *mdt_thr_low[OT_ISP_WDR_RATIO_NUM] = { TD_NULL };
    td_u8 *mdt_thr_high[OT_ISP_WDR_RATIO_NUM] = { TD_NULL };
    td_u8  mdt_nos_floor, sqrt_gain_sum;
    td_u16 nos_floor_g;
    td_u32 m_f_sensor_again, m_f_sensor_dgain, m_again_g;
    td_u32 m_noise_floor = 0;
    td_u32 ratio = isp_ctx->linkage.exp_ratio;
    const td_u8  sqrt_gain_max = 6;
    const td_u8  gain_sum_bits = 8;
    const td_u8  ratio_sqrt_bits = 11;
    const td_u8  mdt_nf_thr_bits = 11;
    const td_u8  mdt_nf_shift_bits = 4;
    const td_u8  mdt_nos_floor_bitdep = 7;
    const td_u8  sensor_gain_shift = 16;
    const td_u8  noise_floor_g_shift = 16;
    const td_u8  sensor_sqrt_gain_shift = 8;
    const td_u8  nos_floor_g_bitdep = 9;
    const td_u8  noise_gain_base = 64;
    const td_u8  sensor_again_shift = 16;
    const td_u16 sensor_gain_round = 512;
    const td_u16 sensor_gain_base  = 1024;

    ratio = MIN2(MAX2(ratio, OT_WDR_EXPOSURE_BASE), OT_ISP_WDR_RATIO_MAX);
    for (i = 0; i < OT_ISP_WDR_RATIO_NUM; i++) {
        mdt_thr_high[i] = fs_wdr->mpi_cfg.wdr_combine.wdr_mdt.auto_attr.md_thr_hig_gain[i];
        mdt_thr_low[i]  = fs_wdr->mpi_cfg.wdr_combine.wdr_mdt.auto_attr.md_thr_low_gain[i];
    }

    if (fs_wdr->mpi_cfg.wdr_combine.wdr_mdt.op_type == OT_OP_MODE_MANUAL) {
        dyna_reg->md_thr_low_gain = fs_wdr->mpi_cfg.wdr_combine.wdr_mdt.manual_attr.md_thr_low_gain;
        dyna_reg->md_thr_hig_gain = fs_wdr->mpi_cfg.wdr_combine.wdr_mdt.manual_attr.md_thr_hig_gain;
    } else {
        dyna_reg->md_thr_low_gain =  (td_u8)get_value_from_lut(isp_ctx->linkage.iso,
                                                               isp_ctx->linkage.exp_ratio_lut[0], mdt_thr_low);
        dyna_reg->md_thr_hig_gain =  (td_u8)get_value_from_lut(isp_ctx->linkage.iso,
                                                               isp_ctx->linkage.exp_ratio_lut[0], mdt_thr_high);
    }

    /* noise floor interpolation */
    m_f_sensor_again = ((isp_ctx->linkage.again << sensor_gain_shift) + sensor_gain_round) / sensor_gain_base;
    m_f_sensor_dgain = ((isp_ctx->linkage.dgain << sensor_gain_shift) + sensor_gain_round) / \
                       sensor_gain_base + (1 << sensor_gain_shift);
    m_again_g = (td_u32)(m_f_sensor_again * noise_gain_base) >> sensor_again_shift;
    m_sqrt_again_g = (td_u32)(wdr_sqrt(m_f_sensor_again, OT_WDR_SQRT_GAIN_BITDEP)) >> sensor_sqrt_gain_shift;
    m_sqrt_dgain_g = (td_u32)(wdr_sqrt(m_f_sensor_dgain, OT_WDR_SQRT_GAIN_BITDEP)) >> sensor_sqrt_gain_shift;
    for (i = 0; i < (NOISE_SET_ELE_NUM - 1); i++) {
        if (m_again_g >= fs_wdr->again_set_lut[i] && m_again_g <= fs_wdr->again_set_lut[i + 1]) {
            m_noise_floor = fs_wdr->floor_set_lut[i] + \
              ((fs_wdr->floor_set_lut[i + 1] - fs_wdr->floor_set_lut[i]) * \
              (m_again_g - fs_wdr->again_set_lut[i])) / \
              div_0_to_1(fs_wdr->again_set_lut[i + 1] - fs_wdr->again_set_lut[i]);
        }
    }

    nos_floor_g = (td_u16)(MIN2(isp_bitmask(nos_floor_g_bitdep), \
        (td_u32)(m_noise_floor * m_f_sensor_dgain + (1 << (noise_floor_g_shift - 1))) >> noise_floor_g_shift));
    mdt_nos_floor = (td_u8)(MIN2((td_u32)isp_bitmask(mdt_nos_floor_bitdep), \
        nos_floor_g * wdr_sqrt(wdr_sqrt(ratio, ratio_sqrt_bits), ratio_sqrt_bits)));
    m_sqrt_again_g  = MIN2(sqrt_gain_max, m_sqrt_again_g);
    m_sqrt_dgain_g  = MIN2(sqrt_gain_max, m_sqrt_dgain_g);
    sqrt_gain_sum      = (td_u8)(m_sqrt_again_g + m_sqrt_dgain_g);

    dyna_reg->mdt_nf_low_thr  = MIN2(isp_bitmask(mdt_nf_thr_bits), \
        ((mdt_nos_floor * dyna_reg->md_thr_low_gain) >> mdt_nf_shift_bits));
    dyna_reg->mdt_nf_high_thr = MIN2(isp_bitmask(mdt_nf_thr_bits), \
        ((mdt_nos_floor * dyna_reg->md_thr_hig_gain) >> mdt_nf_shift_bits));
    dyna_reg->gain_sum_low_thr  = MIN2(isp_bitmask(gain_sum_bits), \
        ((sqrt_gain_sum * dyna_reg->md_thr_low_gain) >> mdt_nf_shift_bits));
    dyna_reg->gain_sum_high_thr = MIN2(isp_bitmask(gain_sum_bits), \
        ((sqrt_gain_sum * dyna_reg->md_thr_hig_gain) >> mdt_nf_shift_bits));
}

static td_void isp_wdr_dyna_fw(ot_vi_pipe vi_pipe, isp_usr_ctx *isp_ctx, isp_fs_wdr *fs_wdr,
                               isp_fswdr_dyna_cfg *dyna_reg)
{
    td_u32 ratio = isp_ctx->linkage.exp_ratio;
    ratio = MIN2(MAX2(ratio, WDR_EXPOSURE_BASE), WDR_RATIO_MAX);
    dyna_reg->wdr_mdt_en     = fs_wdr->mpi_cfg.wdr_combine.motion_comp;
    dyna_reg->long_thr    = fs_wdr->long_thr_reg;
    dyna_reg->short_thr   = fs_wdr->short_thr_reg;

    if (ratio < WDR_FUSION_RATIO_THR) {
        dyna_reg->fusion_data_mode = 0x1;
        dyna_reg->pixel_fusion_data_select = 0x1;
    } else {
        dyna_reg->fusion_data_mode = 0x0;
        dyna_reg->pixel_fusion_data_select = 0x0;
    }

    isp_wdr_fusion_thr_lut_calc(isp_ctx, fs_wdr, dyna_reg);

    wdr_nr_param_cal(fs_wdr, isp_ctx, dyna_reg);

    isp_wdr_md_thr_calc(vi_pipe, isp_ctx, fs_wdr, dyna_reg);

    isp_wdr_func_third(vi_pipe, isp_ctx, fs_wdr, dyna_reg);
}

static td_void isp_wdr_sync_fw(isp_fs_wdr *fswdr, isp_fswdr_sync_cfg *sync_reg_cfg)
{
    sync_reg_cfg->fusion_mode  = (fswdr->mpi_cfg.wdr_merge_mode == OT_ISP_MERGE_FUSION_MODE) ? TD_TRUE : TD_FALSE;
    sync_reg_cfg->wdr_mdt_en   = fswdr->mpi_cfg.wdr_combine.motion_comp;
    sync_reg_cfg->short_thr = fswdr->short_thr_reg;
    sync_reg_cfg->long_thr = fswdr->long_thr_reg;
    sync_reg_cfg->wgt_slope = fswdr_calc_wgt_slope(sync_reg_cfg->short_thr, sync_reg_cfg->long_thr);
}

static td_void isp_wdr_usr_fw(isp_fs_wdr *fswdr, isp_fswdr_usr_cfg *usr_reg_cfg)
{
    usr_reg_cfg->fusion_mode     = (fswdr->mpi_cfg.wdr_merge_mode == OT_ISP_MERGE_FUSION_MODE) ? TD_TRUE : TD_FALSE;
    usr_reg_cfg->short_expo_chk  = fswdr->mpi_cfg.wdr_combine.wdr_mdt.short_expo_chk;
    usr_reg_cfg->mdt_full_thr    = fswdr->mpi_cfg.wdr_combine.wdr_mdt.mdt_full_threshold;
    usr_reg_cfg->mdt_l_bld       = fswdr->mpi_cfg.wdr_combine.wdr_mdt.mdt_long_blend;
    usr_reg_cfg->mdt_still_thr   = fswdr->mpi_cfg.wdr_combine.wdr_mdt.mdt_still_threshold;
    usr_reg_cfg->resh            = TD_TRUE;
    usr_reg_cfg->update_index += 1;
}

static td_void  wdr_set_long_frame_mode(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg, isp_fs_wdr *fswdr)
{
    td_s16 i, j;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_fswdr_dyna_cfg *dyna_reg_cfg = TD_NULL;
    isp_get_ctx(vi_pipe, isp_ctx);
    td_u8 sync_index;
    ot_isp_sns_regs_info *sns_regs_info = TD_NULL;
    isp_sensor_get_sns_reg(vi_pipe, &sns_regs_info);
    isp_check_pointer_void_return(sns_regs_info);

    if (is_offline_mode(isp_ctx->block_attr.running_mode) ||
        is_striping_mode(isp_ctx->block_attr.running_mode)) { /* offline mode */
        if (is_half_wdr_mode(isp_ctx->sns_wdr_mode)) {
            sync_index = MIN2(sns_regs_info->cfg2_valid_delay_max, CFG2VLD_DLY_LIMIT - 1);
        } else {
            sync_index = MIN2(sns_regs_info->cfg2_valid_delay_max + 1, CFG2VLD_DLY_LIMIT - 1);
        }
    } else { /* online mode */
        if (is_half_wdr_mode(isp_ctx->sns_wdr_mode)) {
            sync_index = (td_u8)(clip3((td_s8)sns_regs_info->cfg2_valid_delay_max - 1, 0, CFG2VLD_DLY_LIMIT - 1));
        } else {
            sync_index = MIN2(sns_regs_info->cfg2_valid_delay_max, CFG2VLD_DLY_LIMIT - 1);
        }
    }

    for (i = 0; i < reg_cfg->cfg_num && i < OT_ISP_STRIPING_MAX_NUM; i++) {
        for (j = CFG2VLD_DLY_LIMIT - 2; j >= 0; j--) { /* 2 */
            fswdr->sync_bcom_en[i][j + 1] = fswdr->sync_bcom_en[i][j];
            fswdr->sync_bdec_en[i][j + 1] = fswdr->sync_bdec_en[i][j];
        }
        if ((isp_ctx->linkage.fswdr_mode == OT_ISP_FSWDR_LONG_FRAME_MODE) ||
            (isp_ctx->linkage.fswdr_mode == OT_ISP_FSWDR_AUTO_LONG_FRAME_MODE)) {
            fswdr->sync_bcom_en[i][0] = TD_FALSE;
            fswdr->sync_bdec_en[i][0] = TD_FALSE;
        } else {
            fswdr->sync_bcom_en[i][0] = TD_TRUE;
            fswdr->sync_bdec_en[i][0] = TD_TRUE;
        }

        dyna_reg_cfg = &reg_cfg->alg_reg_cfg[i].wdr_reg_cfg.dyna_reg_cfg;
        dyna_reg_cfg->bcom_en = fswdr->sync_bcom_en[i][sync_index];
        dyna_reg_cfg->bdec_en = fswdr->sync_bdec_en[i][sync_index];
        if (is_pre_online_post_offline(isp_ctx->block_attr.running_mode) && sync_index < CFG2VLD_DLY_LIMIT - 1) {
            dyna_reg_cfg->bdec_en = fswdr->sync_bdec_en[i][sync_index + 1];
        }
    }
}

static td_void isp_fswdr_reg_update(ot_vi_pipe vi_pipe, isp_usr_ctx *isp_ctx, isp_reg_cfg *reg_cfg, isp_fs_wdr *fswdr)
{
    td_u8 i;

    wdr_set_long_frame_mode(vi_pipe, reg_cfg, fswdr);

    if (fswdr->coef_update_en) {
        for (i = 0; i < reg_cfg->cfg_num; i++) {
            isp_wdr_usr_fw(fswdr, &reg_cfg->alg_reg_cfg[i].wdr_reg_cfg.usr_reg_cfg);
        }
    }

    for (i = 0; i < reg_cfg->cfg_num; i++) {
        isp_wdr_sync_fw(fswdr, &reg_cfg->alg_reg_cfg[i].wdr_reg_cfg.sync_reg_cfg);
    }

    for (i = 0; i < reg_cfg->cfg_num; i++) {
        isp_wdr_dyna_fw(vi_pipe, isp_ctx, fswdr, &reg_cfg->alg_reg_cfg[i].wdr_reg_cfg.dyna_reg_cfg);
    }
}

static td_s32 isp_frame_wdr_run(ot_vi_pipe vi_pipe, const td_void *stat_info,
    td_void *cfg, td_s32 rsv)
{
    td_u8 i;
    td_s32 ret;
    ot_isp_alg_mod alg_mod = OT_ISP_ALG_FRAMEWDR;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_fs_wdr  *fswdr  = TD_NULL;
    isp_reg_cfg *reg_cfg = (isp_reg_cfg *)cfg;

    ot_unused(stat_info);
    ot_unused(rsv);

    isp_get_ctx(vi_pipe, isp_ctx);
    fs_wdr_get_ctx(vi_pipe, fswdr);
    isp_check_pointer_return(fswdr);

    if (isp_ctx->linkage.defect_pixel) {
        return TD_SUCCESS;
    }

    ot_ext_system_isp_fswdr_init_status_write(vi_pipe, fswdr->init);
    if ((is_fs_wdr_mode(isp_ctx->sns_wdr_mode)) && (fswdr->init != TD_TRUE)) {
        ioctl(isp_get_fd(vi_pipe), ISP_ALG_INIT_ERR_INFO_PRINT, &alg_mod);
        return TD_SUCCESS;
    }

    fswdr->wdr_en = ot_ext_system_wdr_en_read(vi_pipe);
    check_wdr_mode(vi_pipe, fswdr);

    for (i = 0; i < reg_cfg->cfg_num; i++) {
        reg_cfg->alg_reg_cfg[i].wdr_reg_cfg.wdr_en = fswdr->wdr_en;
    }

    reg_cfg->cfg_key.bit1_fs_wdr_cfg = 1;

    /* check hardware setting */
    ret = check_wdr_open(fswdr);
    if (!ret) {
        return TD_SUCCESS;
    }

    frame_wdr_read_ext_regs(vi_pipe, fswdr);

    isp_fswdr_reg_update(vi_pipe, isp_ctx, reg_cfg, fswdr);

    return TD_SUCCESS;
}

static td_s32 frame_wdr_proc_write(ot_vi_pipe vi_pipe, ot_isp_ctrl_proc_write *proc)
{
    ot_isp_ctrl_proc_write proc_tmp;
    isp_fs_wdr *fswdr = TD_NULL;

    fs_wdr_get_ctx(vi_pipe, fswdr);
    isp_check_pointer_return(fswdr);

    if ((proc->proc_buff == TD_NULL) || (proc->buff_len == 0)) {
        return TD_FAILURE;
    }

    proc_tmp.proc_buff = proc->proc_buff;
    proc_tmp.buff_len = proc->buff_len;

    isp_proc_print_title(&proc_tmp, &proc->write_len, "fswdr info");
    isp_proc_printf(&proc_tmp, proc->write_len,
                    "%10s" "%10s" "%12s" "%18s" "%18s\n",
                    "mdt_en", "long_thr", "short_thr", "md_thr_low_gain", "md_thr_high_gain");

    isp_proc_printf(&proc_tmp, proc->write_len,
                    "%10u" "%10u"  "%12u" "%18u" "%18u\n",
                    fswdr->mpi_cfg.wdr_combine.motion_comp, (td_u16)fswdr->mpi_cfg.wdr_combine.long_threshold,
                    (td_u16)fswdr->mpi_cfg.wdr_combine.short_threshold,
                    (td_u8)fswdr->actual_md_thr_low_gain, (td_u8)fswdr->actual_md_thr_hig_gain);

    proc->write_len += 1;

    return TD_SUCCESS;
}

static td_s32 isp_frame_wdr_ctrl(ot_vi_pipe vi_pipe, td_u32 cmd, td_void *value)
{
    isp_reg_cfg_attr *reg_cfg = TD_NULL;

    switch (cmd) {
        case OT_ISP_WDR_MODE_SET:
            isp_regcfg_get_ctx(vi_pipe, reg_cfg);
            isp_check_pointer_return(reg_cfg);
            isp_frame_wdr_switch_mode(vi_pipe, (td_void *)&reg_cfg->reg_cfg);
            break;

        case OT_ISP_CHANGE_IMAGE_MODE_SET:
            isp_regcfg_get_ctx(vi_pipe, reg_cfg);
            isp_check_pointer_return(reg_cfg);
            isp_frame_res_switch_mode(vi_pipe, (td_void *)&reg_cfg->reg_cfg);
            break;

        case OT_ISP_PROC_WRITE:
            frame_wdr_proc_write(vi_pipe, (ot_isp_ctrl_proc_write *)value);
            break;

        default:
            break;
    }

    return TD_SUCCESS;
}

static td_s32 isp_frame_wdr_exit(ot_vi_pipe vi_pipe)
{
    td_u8 i;
    isp_reg_cfg_attr *reg_cfg = TD_NULL;

    isp_regcfg_get_ctx(vi_pipe, reg_cfg);
    ot_ext_system_isp_fswdr_init_status_write(vi_pipe, TD_FALSE);
    for (i = 0; i < reg_cfg->reg_cfg.cfg_num; i++) {
        reg_cfg->reg_cfg.alg_reg_cfg[i].wdr_reg_cfg.dyna_reg_cfg.bcom_en = TD_FALSE;
        reg_cfg->reg_cfg.alg_reg_cfg[i].wdr_reg_cfg.dyna_reg_cfg.bdec_en = TD_FALSE;
    }

    reg_cfg->reg_cfg.cfg_key.bit1_fs_wdr_cfg = 1;

    frame_wdr_ctx_exit(vi_pipe);

    return TD_SUCCESS;
}

td_s32 isp_alg_register_frame_wdr(ot_vi_pipe vi_pipe)
{
    td_s32 ret;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_alg_node *algs = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);
    isp_alg_check_return(isp_ctx->alg_key.bit1_wdr);
    algs = isp_search_alg(isp_ctx->algs);
    isp_check_pointer_return(algs);
    ret = strncpy_s(algs->alg_name, sizeof(algs->alg_name), "fwdr", sizeof("fwdr"));
    isp_check_eok_return(ret, TD_FAILURE);
    algs->alg_type = OT_ISP_ALG_FRAMEWDR;
    algs->alg_func.pfn_alg_init = isp_frame_wdr_init;
    algs->alg_func.pfn_alg_run  = isp_frame_wdr_run;
    algs->alg_func.pfn_alg_ctrl = isp_frame_wdr_ctrl;
    algs->alg_func.pfn_alg_exit = isp_frame_wdr_exit;
    algs->used = TD_TRUE;

    return TD_SUCCESS;
}
