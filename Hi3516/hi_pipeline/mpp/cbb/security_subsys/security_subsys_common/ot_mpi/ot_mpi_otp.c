/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include "ot_mpi_otp.h"
#include "ot_mpi_utils.h"

td_s32 ot_mpi_otp_init(td_void)
{
    return UNIFY_MPI_OTP_INIT();
}

td_s32 ot_mpi_otp_deinit(td_void)
{
    return UNIFY_MPI_OTP_DEINIT();
}

td_s32 ot_mpi_otp_read_word(td_u32 offset, td_u32 *data)
{
    return UNIFY_MPI_OTP_READ_WORD(offset, data);
}

td_s32 ot_mpi_otp_read_byte(td_u32 offset, td_u8 *data)
{
    return UNIFY_MPI_OTP_READ_BYTE(offset, data);
}

td_s32 ot_mpi_otp_write_byte(td_u32 offset, td_u8 data)
{
    return UNIFY_MPI_OTP_WRITE_BYTE(offset, data);
}