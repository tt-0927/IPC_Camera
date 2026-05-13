/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#ifndef DRV_KEYSLOT_H
#define DRV_KEYSLOT_H

#include "crypto_type.h"
#include "crypto_km_struct.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

td_s32 drv_keyslot_init(td_void);

td_s32 drv_keyslot_deinit(td_void);

td_s32 drv_keyslot_create(td_handle *keyslot_handle, crypto_keyslot_type keyslot_type);

td_s32 drv_keyslot_destroy(td_handle keyslot_handle);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif