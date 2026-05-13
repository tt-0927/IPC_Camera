/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include "hal_pke_alg.h"
#include "drv_trng.h"

td_s32 drv_cipher_pke_get_multi_random(td_u8 *random, td_u32 size)
{
    return drv_cipher_trng_get_multi_random(size, random);
}