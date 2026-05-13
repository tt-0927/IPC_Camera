/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#ifndef HAL_KEYSLOT_H
#define HAL_KEYSLOT_H

#include "crypto_type.h"
#include "crypto_km_struct.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

td_s32 hal_keyslot_lock(td_u32 keyslot_num, crypto_keyslot_type keyslot_type);

td_s32 hal_keyslot_unlock(td_u32 keyslot_num, crypto_keyslot_type keyslot_type);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif