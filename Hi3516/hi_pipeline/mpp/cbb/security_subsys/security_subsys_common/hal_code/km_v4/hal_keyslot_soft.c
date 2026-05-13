/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#if defined(CONFIG_KM_KEYSLOT_SOFT_SUPPORT)
#include "hal_keyslot.h"

#define KEYSLOT_MCIPHER_KEYSLOT_CNT     2
#define KEYSLOT_HMAC_KEYSLOT_CNT        2

static td_s32 keyslot_mcipher_channel[KEYSLOT_MCIPHER_KEYSLOT_CNT] = {0};
static td_s32 keyslot_hmac_channel[KEYSLOT_HMAC_KEYSLOT_CNT] = {0};

td_s32 hal_keyslot_lock(td_u32 keyslot_num, crypto_keyslot_type keyslot_type)
{
    switch (keyslot_type) {
        case CRYPTO_KEYSLOT_TYPE_MCIPHER:
                if (keyslot_num >= KEYSLOT_MCIPHER_KEYSLOT_CNT || keyslot_mcipher_channel[keyslot_num] == 1) {
                    return TD_FAILURE;
                }
                keyslot_mcipher_channel[keyslot_num] = 1;
                break;
        case CRYPTO_KEYSLOT_TYPE_HMAC:
                if (keyslot_num >= KEYSLOT_HMAC_KEYSLOT_CNT || keyslot_hmac_channel[keyslot_num] == 1) {
                    return TD_FAILURE;
                }
                keyslot_hmac_channel[keyslot_num] = 1;
                break;
        default:
            return TD_FAILURE;
    }

    return TD_SUCCESS;
}

td_s32 hal_keyslot_unlock(td_u32 keyslot_num, crypto_keyslot_type keyslot_type)
{
    switch (keyslot_type) {
        case CRYPTO_KEYSLOT_TYPE_MCIPHER:
                if (keyslot_num >= KEYSLOT_MCIPHER_KEYSLOT_CNT) {
                    return TD_FAILURE;
                }
                keyslot_mcipher_channel[keyslot_num] = 0;
                break;
        case CRYPTO_KEYSLOT_TYPE_HMAC:
                if (keyslot_num >= KEYSLOT_HMAC_KEYSLOT_CNT) {
                    return TD_FAILURE;
                }
                keyslot_hmac_channel[keyslot_num] = 0;
                break;
        default:
            return TD_FAILURE;
    }

    return TD_SUCCESS;
}
#endif