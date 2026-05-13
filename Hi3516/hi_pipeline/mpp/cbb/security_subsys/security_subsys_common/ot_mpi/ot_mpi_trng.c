/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include "ot_mpi_cipher.h"
#include "ot_mpi_utils.h"

td_s32 ot_mpi_cipher_trng_get_random(td_u32 *randnum)
{
    return UNIFY_MPI_TRNG_RANDOM(randnum);
}

td_s32 ot_mpi_cipher_trng_get_multi_random(td_u32 size, td_u8 *randnum)
{
    return UNIFY_MPI_TRNG_MULTI_RANDOM(size, randnum);
}