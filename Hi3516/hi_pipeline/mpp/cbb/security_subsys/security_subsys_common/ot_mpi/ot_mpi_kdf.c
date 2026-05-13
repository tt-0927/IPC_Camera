/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include "ot_mpi_cipher.h"
#include "ot_mpi_utils.h"

td_s32 ot_mpi_cipher_pbkdf2(const crypto_kdf_pbkdf2_param *param, td_u8 *out, const td_u32 out_len)
{
    return UNIFY_MPI_PBKDF2(param, out, out_len);
}