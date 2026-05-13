/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
*/

#ifndef KAPI_KM_IMPL_H
#define KAPI_KM_IMPL_H
#include "crypto_km_struct.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

td_s32 kapi_km_init_impl(td_void);

td_void kapi_km_deinit_impl(td_void);

td_s32 kapi_km_create_key_impl(td_handle *kapi_keyslot_handle);

td_s32 kapi_km_set_key_impl(td_handle kapi_keyslot_handle, const td_u8 *key, td_u32 key_len, td_bool tee_open);

td_s32 kapi_km_desroy_key_impl(td_handle kapi_keyslot_handle);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* KAPI_KM_IMPL_H */