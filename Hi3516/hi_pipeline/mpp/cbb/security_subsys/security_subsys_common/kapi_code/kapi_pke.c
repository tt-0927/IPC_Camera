/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
*/

#include "kapi_pke.h"
#include "crypto_drv_common.h"
#include "drv_pke.h"

crypto_mutex g_pke_mutex;

#define kapi_pke_mutex_lock() do {          \
    crypto_mutex_lock(&g_pke_mutex);            \
} while (0)

#define kapi_pke_mutex_unlock() do {        \
    crypto_mutex_unlock(&g_pke_mutex);          \
} while (0)

td_s32 kapi_cipher_pke_env_init(td_void)
{
    td_s32 ret = TD_SUCCESS;

    ret = drv_cipher_pke_init();
    if (ret != TD_SUCCESS) {
        crypto_log_err("drv_cipher_hash_init failed\n");
        return ret;
    }
    ret = crypto_mutex_init(&g_pke_mutex);
    if (ret != TD_SUCCESS) {
        crypto_log_err("crypto_mutex_init failed\n");
        goto error_hash_deinit;
    }
    return ret;

error_hash_deinit:
    drv_cipher_pke_deinit();
    return ret;
}

td_s32 kapi_cipher_pke_env_deinit(td_void)
{
    td_s32 ret = TD_SUCCESS;
    crypto_mutex_destroy(&g_pke_mutex);
    drv_cipher_pke_deinit();
    return ret;
}

td_s32 kapi_pke_ecc_gen_key(drv_pke_ecc_curve_type curve_type, const drv_pke_data *input_priv_key,
    const drv_pke_data *output_priv_key, const drv_pke_ecc_point *output_pub_key)
{
    td_s32 ret = TD_SUCCESS;

    kapi_pke_mutex_lock();
    ret = drv_cipher_pke_ecc_gen_key(curve_type, input_priv_key, output_priv_key, output_pub_key);
    crypto_chk_goto(ret != TD_SUCCESS, unlock_exit, "drv_cipher_pke_ecc_gen_key failed, ret is 0x%x\n", ret);

unlock_exit:
    kapi_pke_mutex_unlock();

    return ret;
}

td_s32 kapi_pke_ecdsa_sign(drv_pke_ecc_curve_type curve_type, const drv_pke_data *priv_key,
    const drv_pke_data *hash, const drv_pke_ecc_sig *sig)
{
    td_s32 ret = TD_SUCCESS;

#if defined(CONFIG_HI3516CV610)
    if (curve_type == DRV_PKE_ECC_TYPE_SM2) {
#if defined(CONFIG_PKE_ECC_SM2_SIGN_SUPPORT)
        kapi_pke_mutex_lock();
        ret = drv_cipher_pke_sm2_sign(priv_key, hash, sig);
        kapi_pke_mutex_unlock();
        crypto_chk_return(ret != TD_SUCCESS, ret, "drv_cipher_pke_sm2_sign failed, ret is 0x%x\n", ret);
        return ret;
#else
        crypto_log_err("sm2 sign unsupport!\n");
        return ERROR_UNSUPPORT;
#endif
    }
#endif

    kapi_pke_mutex_lock();
    ret = drv_cipher_pke_ecdsa_sign(curve_type, priv_key, hash, sig);
    kapi_pke_mutex_unlock();
    crypto_chk_return(ret != TD_SUCCESS, ret, "drv_cipher_pke_ecdsa_sign failed, ret is 0x%x\n", ret);

    return ret;
}

td_s32 kapi_pke_ecdsa_verify(drv_pke_ecc_curve_type curve_type, const drv_pke_ecc_point *pub_key,
    const drv_pke_data *hash, const drv_pke_ecc_sig *sig)
{
    td_s32 ret = TD_SUCCESS;

#if defined(CONFIG_HI3516CV610)
    if (curve_type == DRV_PKE_ECC_TYPE_SM2) {
#if defined(CONFIG_PKE_ECC_SM2_VERIFY_SUPPORT)
        kapi_pke_mutex_lock();
        ret = drv_cipher_pke_sm2_verify(pub_key, hash, sig);
        kapi_pke_mutex_unlock();
        crypto_chk_return(ret != TD_SUCCESS, ret, "drv_cipher_pke_sm2_verify failed, ret is 0x%x\n", ret);
        return ret;
#else
        crypto_log_err("sm2 verify unsupport!\n");
        return ERROR_UNSUPPORT;
#endif
    }
#endif

    kapi_pke_mutex_lock();
    ret = drv_cipher_pke_ecdsa_verify(curve_type, pub_key, hash, sig);
    kapi_pke_mutex_unlock();
    crypto_chk_return(ret != TD_SUCCESS, ret, "drv_cipher_pke_ecdsa_verify failed, ret is 0x%x\n", ret);

    return ret;
}

#if defined(CONFIG_PKE_ECC_EDDSA_SIGN_SUPPORT)
td_s32 kapi_pke_eddsa_sign(drv_pke_ecc_curve_type curve_type, const drv_pke_data *priv_key,
    const drv_pke_msg *msg, const drv_pke_ecc_sig *sig)
{
    td_s32 ret = TD_SUCCESS;

    kapi_pke_mutex_lock();
    ret = drv_cipher_pke_eddsa_sign(curve_type, priv_key, msg, sig);
    crypto_chk_goto(ret != TD_SUCCESS, unlock_exit, "drv_cipher_pke_eddsa_sign failed, ret is 0x%x\n", ret);

unlock_exit:
    kapi_pke_mutex_unlock();
    return ret;
}
#endif

#if defined(CONFIG_PKE_ECC_EDDSA_VERIFY_SUPPORT)
td_s32 kapi_pke_eddsa_verify(drv_pke_ecc_curve_type curve_type, const drv_pke_ecc_point *pub_key,
    const drv_pke_msg *msg, const drv_pke_ecc_sig *sig)
{
    td_s32 ret = TD_SUCCESS;

    kapi_pke_mutex_lock();
    ret = drv_cipher_pke_eddsa_verify(curve_type, pub_key, msg, sig);
    crypto_chk_goto(ret != TD_SUCCESS, unlock_exit, "drv_cipher_pke_eddsa_verify failed, ret is 0x%x\n", ret);

unlock_exit:
    kapi_pke_mutex_unlock();
    return ret;
}
#endif

td_s32 kapi_pke_ecc_gen_ecdh_key(drv_pke_ecc_curve_type curve_type, const drv_pke_ecc_point *input_pub_key,
    const drv_pke_data *input_priv_key, const drv_pke_data *output_shared_key)
{
    td_s32 ret = TD_SUCCESS;

    kapi_pke_mutex_lock();
    ret = drv_cipher_pke_ecc_gen_ecdh_key(curve_type, input_pub_key, input_priv_key, output_shared_key);
    crypto_chk_goto(ret != TD_SUCCESS, unlock_exit, "drv_cipher_pke_ecc_gen_ecdh_key failed, ret is 0x%x\n", ret);

unlock_exit:
    kapi_pke_mutex_unlock();
    return ret;
}

td_s32 kapi_pke_check_dot_on_curve(drv_pke_ecc_curve_type curve_type, const drv_pke_ecc_point *pub_key,
    td_bool *is_on_curve)
{
    td_s32 ret = TD_SUCCESS;

    kapi_pke_mutex_lock();
    ret = drv_cipher_pke_check_dot_on_curve(curve_type, pub_key, is_on_curve);
    crypto_chk_goto(ret != TD_SUCCESS, unlock_exit, "drv_pke_check_don_curve failed, ret is 0x%x\n", ret);

unlock_exit:
    kapi_pke_mutex_unlock();
    return ret;
}

td_s32 kapi_pke_sm2_dsa_hash(const drv_pke_data *sm2_id, const drv_pke_ecc_point *pub_key,
    const drv_pke_msg *msg, drv_pke_data *hash)
{
    td_s32 ret = TD_SUCCESS;

    kapi_pke_mutex_lock();
    ret = drv_cipher_pke_sm2_dsa_hash(sm2_id, pub_key, msg, hash);
    crypto_chk_goto(ret != TD_SUCCESS, unlock_exit, "drv_cipher_pke_sm2_dsa_hash failed, ret is 0x%x\n", ret);

unlock_exit:
    kapi_pke_mutex_unlock();
    return ret;
}

td_s32 kapi_pke_sm2_public_encrypt(const drv_pke_ecc_point *pub_key, const drv_pke_data *plain_text,
    drv_pke_data *cipher_text)
{
    td_s32 ret = TD_SUCCESS;

    kapi_pke_mutex_lock();
    ret = drv_cipher_pke_sm2_public_encrypt(pub_key, plain_text, cipher_text);
    crypto_chk_goto(ret != TD_SUCCESS, unlock_exit, "drv_cipher_pke_sm2_public_encrypt failed, ret is 0x%x\n", ret);
unlock_exit:
    kapi_pke_mutex_unlock();
    return ret;
}

td_s32 kapi_pke_sm2_private_decrypt(const drv_pke_data *priv_key, const drv_pke_data *cipher_text,
    drv_pke_data *plain_text)
{
    td_s32 ret = TD_SUCCESS;

    kapi_pke_mutex_lock();
    ret = drv_cipher_pke_sm2_private_decrypt(priv_key, cipher_text, plain_text);
    crypto_chk_goto(ret != TD_SUCCESS, unlock_exit, "drv_cipher_pke_sm2_private_decrypt failed, ret is 0x%x\n", ret);
unlock_exit:
    kapi_pke_mutex_unlock();
    return ret;
}

/* RSA. */
#if defined(CONFIG_PKE_RSA_GEN_KEY_SUPPORT)
td_s32 kapi_pke_rsa_gen_key(const drv_pke_data *input_e, drv_pke_rsa_priv_key *output_priv_key)
{
    td_s32 ret = TD_SUCCESS;
    kapi_pke_mutex_lock();
    ret = drv_cipher_pke_rsa_gen_key(input_e, output_priv_key);
    crypto_chk_goto(ret != TD_SUCCESS, unlock_exit, "drv_cipher_pke_rsa_gen_key failed, ret is 0x%x\n", ret);

unlock_exit:
    kapi_pke_mutex_unlock();
    return ret;
}
#endif

td_s32 kapi_pke_rsa_sign(const drv_pke_rsa_priv_key *priv_key, drv_pke_rsa_scheme scheme,
    drv_pke_hash_type hash_type, const drv_pke_data *input_hash,
    drv_pke_data *sign)
{
    td_s32 ret = TD_SUCCESS;

    kapi_pke_mutex_lock();
    ret = drv_cipher_pke_rsa_sign(priv_key, scheme, hash_type, input_hash, sign);
    crypto_chk_goto(ret != TD_SUCCESS, unlock_exit, "drv_cipher_pke_rsa_sign failed, ret is 0x%x\n", ret);

unlock_exit:
    kapi_pke_mutex_unlock();
    return ret;
}

td_s32 kapi_pke_rsa_verify(const drv_pke_rsa_pub_key *pub_key, drv_pke_rsa_scheme scheme,
    drv_pke_hash_type hash_type, drv_pke_data *input_hash, const drv_pke_data *sig)
{
    td_s32 ret = TD_SUCCESS;

    kapi_pke_mutex_lock();
    ret = drv_cipher_pke_rsa_verify(pub_key, scheme, hash_type, input_hash, sig);
    crypto_chk_goto(ret != TD_SUCCESS, unlock_exit, "drv_cipher_pke_rsa_verify failed, ret is 0x%x\n", ret);

unlock_exit:
    kapi_pke_mutex_unlock();
    return ret;
}

td_s32 kapi_pke_rsa_public_encrypt(drv_pke_rsa_scheme scheme, drv_pke_hash_type hash_type,
    const drv_pke_rsa_pub_key *pub_key, const drv_pke_data *input, const drv_pke_data *label,
    drv_pke_data *output)
{
    td_s32 ret = TD_SUCCESS;

    kapi_pke_mutex_lock();
    ret = drv_cipher_pke_rsa_public_encrypt(scheme, hash_type, pub_key, input, label, output);
    crypto_chk_goto(ret != TD_SUCCESS, unlock_exit, "drv_cipher_pke_rsa_public_encrypt failed, ret is 0x%x\n", ret);

unlock_exit:
    kapi_pke_mutex_unlock();
    return ret;
}

td_s32 kapi_pke_rsa_private_decrypt(drv_pke_rsa_scheme scheme, drv_pke_hash_type hash_type,
    const drv_pke_rsa_priv_key *priv_key, const drv_pke_data *input, const drv_pke_data *label,
    drv_pke_data *output)
{
    td_s32 ret = TD_SUCCESS;

    kapi_pke_mutex_lock();
    ret = drv_cipher_pke_rsa_private_decrypt(scheme, hash_type, priv_key, input, label, output);
    crypto_chk_goto(ret != TD_SUCCESS, unlock_exit, "drv_cipher_pke_rsa_private_decrypt failed, ret is 0x%x\n", ret);

unlock_exit:
    kapi_pke_mutex_unlock();
    return ret;
}

#if defined(CONFIG_PKE_DH_SUPPORT)
td_s32 kapi_pke_dh_gen_key(const drv_pke_data *g_data, const drv_pke_data *mod_n,
    const drv_pke_data *input_priv_key, const drv_pke_data *output_priv_key, const drv_pke_data *output_pub_key)
{
    td_s32 ret = TD_SUCCESS;

    kapi_pke_mutex_lock();
    ret = drv_cipher_pke_dh_gen_key(g_data, mod_n, input_priv_key, output_priv_key, output_pub_key);
    crypto_chk_print(ret != TD_SUCCESS, "drv_cipher_pke_dh_gen_key failed, ret is 0x%x\n", ret);

    kapi_pke_mutex_unlock();
    return ret;
}

td_s32 kapi_pke_dh_compute_key(const drv_pke_data *mod_n, const drv_pke_data  *input_priv_key,
    const drv_pke_data  *input_pub_key, const drv_pke_data  *output_shared_key)
{
    td_s32 ret = TD_SUCCESS;

    kapi_pke_mutex_lock();
    ret = drv_cipher_pke_dh_compute_key(mod_n, input_priv_key, input_pub_key, output_shared_key);
    crypto_chk_print(ret != TD_SUCCESS, "drv_cipher_pke_dh_compute_key failed, ret is 0x%x\n", ret);

    kapi_pke_mutex_unlock();
    return ret;
}
#endif