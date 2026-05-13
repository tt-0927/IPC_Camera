/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#ifndef ISP_GAMMALUT_H
#define ISP_GAMMALUT_H

#include "ot_type.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

td_void gen_lut(td_u32 *lut, td_u32 lut_len, td_u8 output_bits, td_float gamma_scale);
td_void gen_lut_gamma(td_u32 *lut, td_u32 lut_len);
td_void gen_lut_degamma(td_u32 *lut, td_u32 lut_len);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif