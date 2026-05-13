/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#include "drv_hash.h"
#include "drv_hash_inner.h"
#include "crypto_hash_struct.h"
#include "crypto_drv_common.h"

td_s32 inner_hash_drv_handle_chk(td_handle hash_handle)
{
    td_u32 chn_num = (td_u32)hash_handle;
    crypto_chk_return(chn_num >= CONFIG_HASH_HARD_CHN_CNT, HASH_COMPAT_ERRNO(ERROR_INVALID_HANDLE),
        "hash_handle[%u] is invalid\n", hash_handle);
    crypto_chk_return(((1 << chn_num) & CONFIG_HASH_HARD_CHN_MASK) == 0, HASH_COMPAT_ERRNO(ERROR_INVALID_HANDLE),
        "hash_handle[%u] is invalid\n", hash_handle);

    return TD_SUCCESS;
}

td_void inner_drv_hash_length_add(td_u32 length[2], td_u32 addition)
{
    td_u32 diff = 0;
    if (length[1] > length[1] + addition) {
        length[0]++;
        diff = 0xFFFFFFFF - length[1] + 1;
        length[1] = addition - diff;
        return;
    }
    length[1] += addition;
}