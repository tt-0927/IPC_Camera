/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
*/
#if defined(CONFIG_SYMC_HARDEN_INTERFACE_SUPPORT)
#include "aes_harden_impl.h"
#include "uapi_symc.h"
#include "uapi_km.h"
#include "uapi_otp.h"
#include "ot_mpi_sys_mem.h"
#include "ot_osal.h"
#include "securec.h"

#define unused(x) (void)(x)

#define TEE_DISABLE 0x42
#define TEE_STATUS_OFFSET 0x12
#define MMZ_BUFF_LENGTH (64 * 1024)

static crypto_buf_attr g_aes_cipher_mmz_buff = {0};

#define mbedtls_adapt_chk_return(cond, err_ret, ...) do {      \
    if (cond) {                                         \
        printf("%s:%d:", __func__, __LINE__);                                                  \
        printf(__VA_ARGS__);                       \
        return err_ret;                                 \
    }                                                   \
} while (0)

#define mbedtls_adapt_chk_goto(cond, label, ...) do {      \
    if (cond) {                                         \
        printf("%s:%d:", __func__, __LINE__);                                                  \
        printf(__VA_ARGS__);                       \
        goto label;                                 \
    }                                                   \
} while (0)

#define mbedtls_adapt_chk_goto_with_ret(cond, label, ret_err, ...) do {      \
    if (cond) {                                         \
        printf("%s:%d:", __func__, __LINE__);                                                  \
        printf(__VA_ARGS__);                       \
        ret = ret_err;                              \
        goto label;                                 \
    }                                                   \
} while (0)

#define mbedtls_null_ptr_chk(ptr)    \
    mbedtls_adapt_chk_return((ptr) == TD_NULL, TD_FAILURE, "param is NULL\n")

static crypto_symc_key_length inner_get_key_length(unsigned int key_len)
{
    switch (key_len) {
        case CRYPTO_128_KEY_LEN:
            return CRYPTO_SYMC_KEY_128BIT;
        case CRYPTO_192_KEY_LEN:
            return CRYPTO_SYMC_KEY_192BIT;
        case CRYPTO_256_KEY_LEN:
            return CRYPTO_SYMC_KEY_256BIT;
        default:
            return CRYPTO_SYMC_KEY_LENGTH_INVALID;
    }
}

static td_s32 priv_check_tee_is_open(td_bool *tee_open)
{
    td_s32 ret = TD_FAILURE;
    td_u8 value = 0;
    *tee_open = TD_TRUE;

    ret = uapi_otp_init();
    mbedtls_adapt_chk_return(ret != TD_SUCCESS, ret, "uapi_otp_init failed\r\n");

    ret = uapi_otp_read_byte(TEE_STATUS_OFFSET, &value);
    mbedtls_adapt_chk_goto(ret != TD_SUCCESS, otp_deinit, "uapi_otp_read_byte failed\r\n");

    if (value == TEE_DISABLE) {
        *tee_open = TD_FALSE;
    }
otp_deinit:
    uapi_otp_deinit();
    return ret;
}

int mbedtls_alt_aes_create_key_impl(td_handle *keyslot_handle)
{
    int ret;

    ret = uapi_km_create_key_impl(keyslot_handle);
    mbedtls_adapt_chk_return(ret != TD_SUCCESS, ret, "uapi_km_create_key_impl failed\n");
    return ret;
}

int mbedtls_alt_aes_set_key_impl(td_handle keyslot_handle, const unsigned char *key, unsigned int key_len)
{
    int ret;
    td_bool tee_open;

    ret = priv_check_tee_is_open(&tee_open);
    mbedtls_adapt_chk_return(ret != TD_SUCCESS, ret, "priv_check_tee_is_open failed\r\n");

    ret = uapi_km_set_key_impl(keyslot_handle, key, key_len, tee_open);
    mbedtls_adapt_chk_return(ret != TD_SUCCESS, ret, "uapi_km_create_key_impl failed\n");
    return ret;
}

int mbedtls_alt_aes_destroy_key_impl(const td_handle keyslot_handle)
{
    int ret;
    ret = uapi_km_destroy_key_impl(keyslot_handle);
    mbedtls_adapt_chk_return(ret != TD_SUCCESS, ret, "uapi_km_destroy_key_impl failed\n");
    return TD_SUCCESS;
}

int aes_harden_buff_alloc(void)
{
    td_s32 ret;
    ret = ot_mpi_sys_mmz_alloc((td_phys_addr_t *)&g_aes_cipher_mmz_buff.phys_addr,
        (td_void **)&g_aes_cipher_mmz_buff.virt_addr, NULL, NULL, MMZ_BUFF_LENGTH);
    mbedtls_adapt_chk_return(ret != TD_SUCCESS, ret, "ot_mpi_sys_mmz_alloc cipher buff failed\n");
    return 0;
}

void aes_harden_buff_free(void)
{
    ot_mpi_sys_mmz_free(g_aes_cipher_mmz_buff.phys_addr, g_aes_cipher_mmz_buff.virt_addr);
}

static int mbedlts_adapt_aes_crypto(crypto_symc_ctrl_t *symc_ctrl, const td_handle keyslot_handle,
    const unsigned char *key, unsigned int key_len, const unsigned char *src, unsigned char *dst,
    unsigned int data_len, unsigned int is_decrypt)
{
    td_s32 ret = TD_SUCCESS;
    td_u32 processed = 0;
    td_u32 processing = 0;
    td_void *virt_addr = g_aes_cipher_mmz_buff.virt_addr;
    td_phys_addr_t phys_addr = g_aes_cipher_mmz_buff.phys_addr;
    crypto_buf_attr src_buff = {
        .buf_sec = CRYPTO_BUF_NONSECURE
    };
    crypto_buf_attr dst_buff = {
        .buf_sec = CRYPTO_BUF_NONSECURE
    };

    unused(key);
    unused(key_len);
    mbedtls_null_ptr_chk(symc_ctrl);

    src_buff.phys_addr = phys_addr;
    dst_buff.phys_addr = phys_addr;
    while (processed < data_len) {
        processing = processed + MMZ_BUFF_LENGTH > data_len ? data_len - processed : MMZ_BUFF_LENGTH;
        ret = memcpy_s(virt_addr, MMZ_BUFF_LENGTH, src + processed, processing);
        mbedtls_adapt_chk_goto(ret != EOK, exit, "ot_mpi_sys_mmz_alloc failed\n");

        ret = unify_uapi_cipher_symc_crypt_impl(symc_ctrl, keyslot_handle,
            &src_buff, &dst_buff, processing, (td_bool)is_decrypt);
        mbedtls_adapt_chk_goto(ret != TD_SUCCESS, exit, "ot_mpi_sys_mmz_alloc failed\n");

        ret = memcpy_s(dst + processed, data_len - processed, virt_addr, processing);
        mbedtls_adapt_chk_goto(ret != EOK, exit, "ot_mpi_sys_mmz_alloc failed\n");

        processed += processing;
    }
    ret = TD_SUCCESS;
exit:
    return ret;
}

int mbedtls_alt_aes_encrypt_impl(const td_handle keyslot_handle, const unsigned char *key, unsigned int key_len,
    const unsigned char src[16], unsigned char dst[16])
{
    crypto_symc_ctrl_t symc_ctrl = {
        .symc_alg = CRYPTO_SYMC_ALG_AES,
        .work_mode = CRYPTO_SYMC_WORK_MODE_ECB,
        .symc_bit_width = CRYPTO_SYMC_BIT_WIDTH_128BIT
    };

    mbedtls_null_ptr_chk(src);
    mbedtls_null_ptr_chk(dst);

    symc_ctrl.symc_key_length = inner_get_key_length(key_len);
    return mbedlts_adapt_aes_crypto(&symc_ctrl, keyslot_handle, key, key_len,
        src, dst, CRYPTO_AES_BLOCK_SIZE_IN_BYTES, 0);
}

int mbedtls_alt_aes_decrypt_impl(const td_handle keyslot_handle, const unsigned char *key, unsigned int key_len,
    const unsigned char src[16], unsigned char dst[16])
{
    crypto_symc_ctrl_t symc_ctrl = {
        .symc_alg = CRYPTO_SYMC_ALG_AES,
        .work_mode = CRYPTO_SYMC_WORK_MODE_ECB,
        .symc_bit_width = CRYPTO_SYMC_BIT_WIDTH_128BIT
    };

    mbedtls_null_ptr_chk(src);
    mbedtls_null_ptr_chk(dst);

    symc_ctrl.symc_key_length = inner_get_key_length(key_len);
    return mbedlts_adapt_aes_crypto(&symc_ctrl, keyslot_handle, key, key_len,
        src, dst, CRYPTO_AES_BLOCK_SIZE_IN_BYTES, 1);
}

int mbedtls_alt_aes_cbc_encrypt_impl(const td_handle keyslot_handle, const unsigned char *key, unsigned int key_len,
    unsigned char iv[16], const unsigned char *src, unsigned char *dst, unsigned int data_len)
{
    int ret;
    crypto_symc_ctrl_t symc_ctrl = {
        .symc_alg = CRYPTO_SYMC_ALG_AES,
        .work_mode = CRYPTO_SYMC_WORK_MODE_CBC,
        .symc_bit_width = CRYPTO_SYMC_BIT_WIDTH_128BIT,
        .iv_length = CRYPTO_IV_LEN_IN_BYTES
    };

    mbedtls_null_ptr_chk(src);
    mbedtls_null_ptr_chk(dst);
    mbedtls_null_ptr_chk(iv);

    ret = memcpy_s(symc_ctrl.iv, sizeof(symc_ctrl.iv), iv, sizeof(symc_ctrl.iv));
    mbedtls_adapt_chk_return(ret != EOK, CRYPTO_FAILURE, "memcpy failed\n");

    symc_ctrl.symc_key_length = inner_get_key_length(key_len);
    ret = mbedlts_adapt_aes_crypto(&symc_ctrl, keyslot_handle, key, key_len, src, dst, data_len, 0);
    mbedtls_adapt_chk_return(ret != CRYPTO_SUCCESS, ret, "mbedlts_adapt_aes_crypto failed\n");

    ret = memcpy_s(iv, sizeof(symc_ctrl.iv), symc_ctrl.iv, sizeof(symc_ctrl.iv));
    mbedtls_adapt_chk_return(ret != EOK, CRYPTO_FAILURE, "memcpy failed\n");
    return 0;
}

int mbedtls_alt_aes_cbc_decrypt_impl(const td_handle keyslot_handle, const unsigned char *key,
    unsigned int key_len, unsigned char iv[16], const unsigned char *src, unsigned char *dst, unsigned int data_len)
{
    int ret;
    crypto_symc_ctrl_t symc_ctrl = {
        .symc_alg = CRYPTO_SYMC_ALG_AES,
        .work_mode = CRYPTO_SYMC_WORK_MODE_CBC,
        .symc_bit_width = CRYPTO_SYMC_BIT_WIDTH_128BIT,
        .iv_length = CRYPTO_IV_LEN_IN_BYTES
    };

    mbedtls_null_ptr_chk(src);
    mbedtls_null_ptr_chk(dst);
    mbedtls_null_ptr_chk(iv);

    ret = memcpy_s(symc_ctrl.iv, sizeof(symc_ctrl.iv), iv, sizeof(symc_ctrl.iv));
    mbedtls_adapt_chk_return(ret != EOK, CRYPTO_FAILURE, "memcpy failed\n");

    symc_ctrl.symc_key_length = inner_get_key_length(key_len);
    ret = mbedlts_adapt_aes_crypto(&symc_ctrl, keyslot_handle, key, key_len, src, dst, data_len, 1);
    mbedtls_adapt_chk_return(ret != CRYPTO_SUCCESS, ret, "mbedlts_adapt_aes_crypto failed\n");

    ret = memcpy_s(iv, sizeof(symc_ctrl.iv), symc_ctrl.iv, sizeof(symc_ctrl.iv));
    mbedtls_adapt_chk_return(ret != EOK, CRYPTO_FAILURE, "memcpy failed\n");
    return 0;
}

int mbedtls_alt_aes_cfb128_encrypt_impl(const td_handle keyslot_handle, const unsigned char *key, unsigned int key_len,
    unsigned char iv[16], const unsigned char *src, unsigned char *dst, unsigned int data_len)
{
    int ret;
    crypto_symc_ctrl_t symc_ctrl = {
        .symc_alg = CRYPTO_SYMC_ALG_AES,
        .work_mode = CRYPTO_SYMC_WORK_MODE_CFB,
        .symc_bit_width = CRYPTO_SYMC_BIT_WIDTH_128BIT,
        .iv_length = CRYPTO_IV_LEN_IN_BYTES
    };

    mbedtls_null_ptr_chk(src);
    mbedtls_null_ptr_chk(dst);
    mbedtls_null_ptr_chk(iv);

    ret = memcpy_s(symc_ctrl.iv, sizeof(symc_ctrl.iv), iv, sizeof(symc_ctrl.iv));
    mbedtls_adapt_chk_return(ret != EOK, CRYPTO_FAILURE, "memcpy failed\n");

    symc_ctrl.symc_key_length = inner_get_key_length(key_len);
    ret = mbedlts_adapt_aes_crypto(&symc_ctrl, keyslot_handle, key, key_len, src, dst, data_len, 0);
    mbedtls_adapt_chk_return(ret != CRYPTO_SUCCESS, ret, "mbedlts_adapt_aes_crypto failed\n");

    ret = memcpy_s(iv, sizeof(symc_ctrl.iv), symc_ctrl.iv, sizeof(symc_ctrl.iv));
    mbedtls_adapt_chk_return(ret != EOK, CRYPTO_FAILURE, "memcpy failed\n");
    return 0;
}

int mbedtls_alt_aes_cfb128_decrypt_impl(const td_handle keyslot_handle, const unsigned char *key, unsigned int key_len,
    unsigned char iv[16], const unsigned char *src, unsigned char *dst, unsigned int data_len)
{
    int ret;
    crypto_symc_ctrl_t symc_ctrl = {
        .symc_alg = CRYPTO_SYMC_ALG_AES,
        .work_mode = CRYPTO_SYMC_WORK_MODE_CFB,
        .symc_bit_width = CRYPTO_SYMC_BIT_WIDTH_128BIT,
        .iv_length = CRYPTO_IV_LEN_IN_BYTES
    };

    mbedtls_null_ptr_chk(src);
    mbedtls_null_ptr_chk(dst);
    mbedtls_null_ptr_chk(iv);

    ret = memcpy_s(symc_ctrl.iv, sizeof(symc_ctrl.iv), iv, sizeof(symc_ctrl.iv));
    mbedtls_adapt_chk_return(ret != EOK, CRYPTO_FAILURE, "memcpy failed\n");

    symc_ctrl.symc_key_length = inner_get_key_length(key_len);
    ret = mbedlts_adapt_aes_crypto(&symc_ctrl, keyslot_handle, key, key_len, src, dst, data_len, 1);
    mbedtls_adapt_chk_return(ret != CRYPTO_SUCCESS, ret, "mbedlts_adapt_aes_crypto failed\n");

    ret = memcpy_s(iv, sizeof(symc_ctrl.iv), symc_ctrl.iv, sizeof(symc_ctrl.iv));
    mbedtls_adapt_chk_return(ret != EOK, CRYPTO_FAILURE, "memcpy failed\n");
    return 0;
}

int mbedtls_alt_aes_cfb8_encrypt_impl(const td_handle keyslot_handle, const unsigned char *key, unsigned int key_len,
    unsigned char iv[16], const unsigned char *src, unsigned char *dst, unsigned int data_len)
{
    int ret;
    crypto_symc_ctrl_t symc_ctrl = {
        .symc_alg = CRYPTO_SYMC_ALG_AES,
        .work_mode = CRYPTO_SYMC_WORK_MODE_CFB,
        .symc_bit_width = CRYPTO_SYMC_BIT_WIDTH_8BIT,
        .iv_length = CRYPTO_IV_LEN_IN_BYTES
    };

    mbedtls_null_ptr_chk(src);
    mbedtls_null_ptr_chk(dst);
    mbedtls_null_ptr_chk(iv);

    ret = memcpy_s(symc_ctrl.iv, sizeof(symc_ctrl.iv), iv, sizeof(symc_ctrl.iv));
    mbedtls_adapt_chk_return(ret != EOK, CRYPTO_FAILURE, "memcpy failed\n");

    symc_ctrl.symc_key_length = inner_get_key_length(key_len);
    ret = mbedlts_adapt_aes_crypto(&symc_ctrl, keyslot_handle, key, key_len, src, dst, data_len, 0);
    mbedtls_adapt_chk_return(ret != CRYPTO_SUCCESS, ret, "mbedlts_adapt_aes_crypto failed\n");

    ret = memcpy_s(iv, sizeof(symc_ctrl.iv), symc_ctrl.iv, sizeof(symc_ctrl.iv));
    mbedtls_adapt_chk_return(ret != EOK, CRYPTO_FAILURE, "memcpy failed\n");
    return 0;
}

int mbedtls_alt_aes_cfb8_decrypt_impl(const td_handle keyslot_handle, const unsigned char *key, unsigned int key_len,
    unsigned char iv[16], const unsigned char *src, unsigned char *dst, unsigned int data_len)
{
    int ret;
    crypto_symc_ctrl_t symc_ctrl = {
        .symc_alg = CRYPTO_SYMC_ALG_AES,
        .work_mode = CRYPTO_SYMC_WORK_MODE_CFB,
        .symc_bit_width = CRYPTO_SYMC_BIT_WIDTH_8BIT,
        .iv_length = CRYPTO_IV_LEN_IN_BYTES
    };

    mbedtls_null_ptr_chk(src);
    mbedtls_null_ptr_chk(dst);
    mbedtls_null_ptr_chk(iv);

    ret = memcpy_s(symc_ctrl.iv, sizeof(symc_ctrl.iv), iv, sizeof(symc_ctrl.iv));
    mbedtls_adapt_chk_return(ret != EOK, CRYPTO_FAILURE, "memcpy failed\n");

    symc_ctrl.symc_key_length = inner_get_key_length(key_len);
    ret = mbedlts_adapt_aes_crypto(&symc_ctrl, keyslot_handle, key, key_len, src, dst, data_len, 1);
    mbedtls_adapt_chk_return(ret != CRYPTO_SUCCESS, ret, "mbedlts_adapt_aes_crypto failed\n");

    ret = memcpy_s(iv, sizeof(symc_ctrl.iv), symc_ctrl.iv, sizeof(symc_ctrl.iv));
    mbedtls_adapt_chk_return(ret != EOK, CRYPTO_FAILURE, "memcpy failed\n");
    return 0;
}

int mbedtls_alt_aes_ofb_crypt_impl(const td_handle keyslot_handle, const unsigned char *key, unsigned int key_len,
    unsigned char *iv_off, unsigned char iv[16], const unsigned char *src, unsigned char *dst, unsigned int data_len)
{
    int ret;
    unsigned int i;
    unsigned int processed_len = 0;
    unsigned int processing_len = 0;
    unsigned int left = data_len;
    crypto_symc_ctrl_t symc_ctrl = {
        .symc_alg = CRYPTO_SYMC_ALG_AES,
        .work_mode = CRYPTO_SYMC_WORK_MODE_OFB,
        .symc_bit_width = CRYPTO_SYMC_BIT_WIDTH_128BIT,
        .iv_length = CRYPTO_IV_LEN_IN_BYTES
    };

    mbedtls_null_ptr_chk(src);
    mbedtls_null_ptr_chk(dst);
    mbedtls_null_ptr_chk(iv);
    mbedtls_null_ptr_chk(iv_off);

    symc_ctrl.symc_key_length = inner_get_key_length(key_len);

    if (*iv_off != 0) {
        for (i = *iv_off; i < CRYPTO_AES_BLOCK_SIZE_IN_BYTES && processed_len < data_len; i++) {
            dst[processed_len] = src[processed_len] ^ iv[i];
            processed_len++;
        }
    }
    left = data_len - processed_len;
    processing_len = left - left % CRYPTO_AES_BLOCK_SIZE_IN_BYTES;
    if (processing_len != 0) {
        ret = memcpy_s(symc_ctrl.iv, sizeof(symc_ctrl.iv), iv, sizeof(symc_ctrl.iv));
        mbedtls_adapt_chk_return(ret != EOK, CRYPTO_FAILURE, "memcpy failed\n");
        ret = mbedlts_adapt_aes_crypto(&symc_ctrl, keyslot_handle, key, key_len, src, dst, processing_len, 0);
        mbedtls_adapt_chk_return(ret != CRYPTO_SUCCESS, ret, "mbedlts_adapt_aes_crypto failed\n");
        left -= processing_len;
        processed_len += processing_len;
        ret = memcpy_s(iv, sizeof(symc_ctrl.iv), symc_ctrl.iv, sizeof(symc_ctrl.iv));
        mbedtls_adapt_chk_return(ret != EOK, CRYPTO_FAILURE, "memcpy failed\n");
    }
    *iv_off = (unsigned char)left;
    ret = mbedtls_alt_aes_encrypt_impl(keyslot_handle, key, key_len, iv, iv);
    mbedtls_adapt_chk_return(ret != CRYPTO_SUCCESS, ret, "mbedlts_adapt_aes_crypto failed\n");
    for (i = 0; i < *iv_off; i++) {
        dst[processed_len] = src[processed_len] ^ iv[i];
        processed_len++;
    }
    return 0;
}

int mbedtls_alt_aes_ctr_crypt_impl(const td_handle keyslot_handle, const unsigned char *key, unsigned int key_len,
    unsigned char *nc_off, unsigned char nonce_counter[16], unsigned char stream_block[16],
    const unsigned char *src, unsigned char *dst, unsigned int data_len)
{
    int ret;
    unsigned int i;
    unsigned int processed_len = 0;
    unsigned int processing_len = 0;
    unsigned int left = data_len;
    crypto_symc_ctrl_t symc_ctrl = {
        .symc_alg = CRYPTO_SYMC_ALG_AES,
        .work_mode = CRYPTO_SYMC_WORK_MODE_CTR,
        .symc_bit_width = CRYPTO_SYMC_BIT_WIDTH_128BIT,
        .iv_length = CRYPTO_IV_LEN_IN_BYTES
    };

    mbedtls_null_ptr_chk(src);
    mbedtls_null_ptr_chk(dst);
    mbedtls_null_ptr_chk(nc_off);
    mbedtls_null_ptr_chk(nonce_counter);
    mbedtls_null_ptr_chk(stream_block);

    symc_ctrl.symc_key_length = inner_get_key_length(key_len);

    if (*nc_off != 0) {
        for (i = *nc_off; i < CRYPTO_AES_BLOCK_SIZE_IN_BYTES && processed_len < data_len; i++) {
            dst[processed_len] = src[processed_len] ^ stream_block[i];
            processed_len++;
        }
    }
    left = data_len - processed_len;
    processing_len = left - left % CRYPTO_AES_BLOCK_SIZE_IN_BYTES;
    if (processing_len != 0) {
        ret = memcpy_s(symc_ctrl.iv, sizeof(symc_ctrl.iv), nonce_counter, sizeof(symc_ctrl.iv));
        mbedtls_adapt_chk_return(ret != EOK, CRYPTO_FAILURE, "memcpy failed\n");
        ret = mbedlts_adapt_aes_crypto(&symc_ctrl, keyslot_handle, key, key_len, src, dst, processing_len, 0);
        mbedtls_adapt_chk_return(ret != CRYPTO_SUCCESS, ret, "mbedlts_adapt_aes_crypto failed\n");
        left -= processing_len;
        processed_len += processing_len;
        ret = memcpy_s(nonce_counter, sizeof(symc_ctrl.iv), symc_ctrl.iv, sizeof(symc_ctrl.iv));
        mbedtls_adapt_chk_return(ret != EOK, CRYPTO_FAILURE, "memcpy failed\n");
    }
    *nc_off = (unsigned char)left;
    ret = mbedtls_alt_aes_encrypt_impl(keyslot_handle, key, key_len, nonce_counter, stream_block);
    mbedtls_adapt_chk_return(ret != CRYPTO_SUCCESS, ret, "mbedlts_adapt_aes_crypto failed\n");
    for (i = 0; i < *nc_off; i++) {
        dst[processed_len] = src[processed_len] ^ stream_block[i];
        processed_len++;
    }
    return 0;
}
#endif