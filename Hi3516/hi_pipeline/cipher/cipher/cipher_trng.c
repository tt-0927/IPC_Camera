/**
 * @FilePath     : cipher_trng.c
 * @Author       : zhouzirui
 * @Date         : 2025-04-03 09:54:56
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-08-27 20:02:05
 * @Description  : 密码随机数封装
 */

#include "cipher_trng.h"

int cipherTrng_getRandom(td_u32 *pRandnum)
{
    if (NULL == pRandnum)
    {
        return TD_FAILURE;
    }

    CHECK_API_RETURN(ot_mpi_cipher_trng_get_random(pRandnum));

    return TD_SUCCESS;
}

int cipherTrng_getMultiRandom(td_u32 u32Size,td_u8 *pRandnum)
{
    if (NULL == pRandnum || u32Size == 0 || u32Size > 1024)
    {
        return TD_FAILURE;
    }

    CHECK_API_RETURN(ot_mpi_cipher_trng_get_multi_random(u32Size, pRandnum));

    return TD_SUCCESS;
}
