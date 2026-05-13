/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#ifndef OT_MPI_UTILS
#define OT_MPI_UTILS

#include "uapi_hash.h"
#include "uapi_kdf.h"
#include "uapi_km.h"
#include "uapi_otp.h"
#include "uapi_pke.h"
#include "uapi_symc.h"
#include "uapi_trng.h"
#include "crypto_osal_user_lib.h"
// hash
#define UNIFY_MPI_HASH_INIT     unify_uapi_cipher_hash_init
#define UNIFY_MPI_HASH_DEINIT   unify_uapi_cipher_hash_deinit
#define UNIFY_MPI_HASH_START    unify_uapi_cipher_hash_start
#define UNIFY_MPI_HASH_UPDATE   unify_uapi_cipher_hash_update
#define UNIFY_MPI_HASH_FINISH   unify_uapi_cipher_hash_finish
#define UNIFY_MPI_HASH_GET      unify_uapi_cipher_hash_get
#define UNIFY_MPI_HASH_SET      unify_uapi_cipher_hash_set
#define UNIFY_MPI_HASH_DESTROY  unify_uapi_cipher_hash_destroy
// pbkdf2
#define UNIFY_MPI_PBKDF2        unify_uapi_cipher_pbkdf2
// km
#define UNIFY_MPI_KM_INIT                uapi_km_init
#define UNIFY_MPI_KM_DEINIT              uapi_km_deinit
#define UNIFY_MPI_KEYSLOT_CREATE         uapi_keyslot_create
#define UNIFY_MPI_KEYSLOT_DESTROY        uapi_keyslot_destroy
#define UNIFY_MPI_KLAD_CREATE            uapi_klad_create
#define UNIFY_MPI_KLAD_DESTROY           uapi_klad_destroy
#define UNIFY_MPI_KLAD_ATTACH            uapi_klad_attach
#define UNIFY_MPI_KLAD_DETACH            uapi_klad_detach
#define UNIFY_MPI_SET_ATTR               uapi_klad_set_attr
#define UNIFY_MPI_GET_ATTR               uapi_klad_get_attr
#define UNIFY_MPI_SESSION_KEY            uapi_klad_set_session_key
#define UNIFY_MPI_CONTENT_KEY            uapi_klad_set_content_key
#define UNIFY_MPI_CLEAR_KEY              uapi_klad_set_clear_key
#define UNIFY_MPI_EFFECTIVE_KEY          uapi_klad_set_effective_key
// otp
#define UNIFY_MPI_OTP_INIT                uapi_otp_init
#define UNIFY_MPI_OTP_DEINIT              uapi_otp_deinit
#define UNIFY_MPI_OTP_READ_WORD           uapi_otp_read_word
#define UNIFY_MPI_OTP_READ_BYTE           uapi_otp_read_byte
#define UNIFY_MPI_OTP_WRITE_BYTE          uapi_otp_write_byte
// pke
#define UNIFY_MPI_PKE_INIT              unify_uapi_cipher_pke_init
#define UNIFY_MPI_PKE_DEINIT            unify_uapi_cipher_pke_deinit
#define UNIFY_MPI_ECC_GEN_KEY           unify_uapi_cipher_pke_ecc_gen_key
#define UNIFY_MPI_ECDH_GEN_KEY          unify_uapi_cipher_pke_ecc_gen_ecdh_key
#define UNIFY_MPI_ECDSA_SIGN            unify_uapi_cipher_pke_ecdsa_sign
#define UNIFY_MPI_ECDSA_VERIFY          unify_uapi_cipher_pke_ecdsa_verify
#define UNIFY_MPI_EDDSA_SIGN            unify_uapi_cipher_pke_eddsa_sign
#define UNIFY_MPI_EDDSA_VERIFY          unify_uapi_cipher_pke_eddsa_verify
#define UNIFY_MPI_CHCEK_DOT_ON_CURVE    unify_uapi_cipher_pke_check_dot_on_curve
#define UNIFY_MPI_SM2_DSA_HASH          unify_uapi_cipher_pke_sm2_dsa_hash
#define UNIFY_MPI_SM2_PUB_ENCRYPT       unify_uapi_cipher_pke_sm2_public_encrypt
#define UNIFY_MPI_SM2_PRIV_DECRYPT      unify_uapi_cipher_pke_sm2_private_decrypt
#define UNIFY_MPI_RSA_SIGN              unify_uapi_cipher_pke_rsa_sign
#define UNIFY_MPI_RSA_VERIFY            unify_uapi_cipher_pke_rsa_verify
#define UNIFY_MPI_RSA_PUB_ENCRYPT       unify_uapi_cipher_pke_rsa_public_encrypt
#define UNIFY_MPI_RSA_PRIV_DECRYPT      unify_uapi_cipher_pke_rsa_private_decrypt
// symc
#define UNIFY_MPI_SYMC_INIT              unify_uapi_cipher_symc_init
#define UNIFY_MPI_SYMC_DEINIT            unify_uapi_cipher_symc_deinit
#define UNIFY_MPI_SYMC_CREATE            unify_uapi_cipher_symc_create
#define UNIFY_MPI_SYMC_DESTROY           unify_uapi_cipher_symc_destroy
#define UNIFY_MPI_SYMC_SET_CONFIG        unify_uapi_cipher_symc_set_config
#define UNIFY_MPI_SYMC_ATTACH            unify_uapi_cipher_symc_attach
#define UNIFY_MPI_SYMC_ENCRYPT           unify_uapi_cipher_symc_encrypt
#define UNIFY_MPI_SYMC_DECRYPT           unify_uapi_cipher_symc_decrypt
#define UNIFY_MPI_SYMC_GET_TAG           unify_uapi_cipher_symc_get_tag
#define UNIFY_MPI_MAC_START              unify_uapi_cipher_mac_start
#define UNIFY_MPI_MAC_UPDATE             unify_uapi_cipher_mac_update
#define UNIFY_MPI_MAC_FINISH             unify_uapi_cipher_mac_finish
#define UNIFY_MPI_SYMC_MULTI_ENCRYPT     unify_uapi_cipher_symc_encrypt_multi
#define UNIFY_MPI_SYMC_MULTI_DECRYPT     unify_uapi_cipher_symc_decrypt_multi
// trng
#define UNIFY_MPI_TRNG_RANDOM            unify_uapi_cipher_trng_get_random
#define UNIFY_MPI_TRNG_MULTI_RANDOM      unify_uapi_cipher_trng_get_multi_random

#endif