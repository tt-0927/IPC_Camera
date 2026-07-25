/**
 * @FilePath     : security_subsys_selftest.c
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-06-13
 * @Description  : 海思安全子系统设备侧接口自测程序
 cmake -S hi_pipeline/cipher/test -B output/security_subsys_selftest_build -DCMAKE_BUILD_TYPE=Release
 */

#include "cipher_context.h"
#include "cipher_hash.h"
#include "cipher_km.h"
#include "cipher_pke.h"
#include "cipher_symc.h"
#include "cipher_trng.h"
#include "ot_mpi_cipher.h"
#include "ot_mpi_otp.h"
#include "ss_mpi_cipher.h"
#include "ss_mpi_km.h"
#include "ss_mpi_sys_mem.h"

#include <openssl/evp.h>

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SELFTEST_SM3_LEN 32
#define SELFTEST_SM2_LEN 32
#define SELFTEST_SM4_BLOCK_LEN 16
#define SELFTEST_TRNG_MAX_LEN 1024
#define SELFTEST_IRQ_DIAG_FIRST_GIC 101
#define SELFTEST_IRQ_DIAG_LAST_GIC 104
#define SELFTEST_IRQ_DIAG_COUNT (SELFTEST_IRQ_DIAG_LAST_GIC - SELFTEST_IRQ_DIAG_FIRST_GIC + 1)

typedef struct {
    td_phys_addr_t phys_addr;
    td_u8 *virt_addr;
    td_u32 size;
} SelftestMmzBuf_S;

typedef struct {
    td_bool valid[SELFTEST_IRQ_DIAG_COUNT];
    unsigned long long total[SELFTEST_IRQ_DIAG_COUNT];
} SelftestIrqSnapshot_S;

static const td_u8 g_aSm3HelloDigest[SELFTEST_SM3_LEN] = {
    0xdc, 0x74, 0xf0, 0x51, 0xad, 0x5b, 0xc1, 0x9b, 0xa7, 0x21, 0xbf, 0x00, 0x23, 0xe1, 0x0d, 0xe0,
    0x3b, 0xae, 0x29, 0xbb, 0xe0, 0x13, 0xc4, 0x39, 0x88, 0xba, 0xe5, 0x58, 0x28, 0xbc, 0xeb, 0xbc,
};

static const td_u8 g_aSm3EmptyDigest[SELFTEST_SM3_LEN] = {
    0x1a, 0xb2, 0x1d, 0x83, 0x55, 0xcf, 0xa1, 0x7f, 0x8e, 0x61, 0x19, 0x48, 0x31, 0xe8, 0x1a, 0x8f,
    0x22, 0xbe, 0xc8, 0xc7, 0x28, 0xfe, 0xfb, 0x74, 0x7e, 0xd0, 0x35, 0xeb, 0x50, 0x82, 0xaa, 0x2b,
};

static void selftest_print_hex(const char *name, const td_u8 *data, td_u32 len)
{
    td_u32 i = 0;

    printf("%s: ", name);
    for (i = 0; i < len; ++i) {
        printf("%02x", data[i]);
    }
    printf("\n");
}

static int selftest_check_ret(const char *name, td_s32 ret)
{
    if (ret != TD_SUCCESS) {
        printf("[FAIL] %s ret=0x%08X\n", name, (unsigned int)ret);
        return ret;
    }

    printf("[PASS] %s\n", name);
    return TD_SUCCESS;
}

static int selftest_check_true(const char *name, td_bool condition)
{
    if (condition != TD_TRUE) {
        printf("[FAIL] %s\n", name);
        return TD_FAILURE;
    }

    printf("[PASS] %s\n", name);
    return TD_SUCCESS;
}

static int selftest_mem_equal(const char *name, const td_u8 *left, const td_u8 *right, td_u32 len)
{
    if (memcmp(left, right, len) != 0) {
        printf("[FAIL] %s\n", name);
        selftest_print_hex("left ", left, len);
        selftest_print_hex("right", right, len);
        return TD_FAILURE;
    }

    printf("[PASS] %s\n", name);
    return TD_SUCCESS;
}

static void selftest_dump_text_file(const char *path, const char *tag)
{
    char line[256] = {0};
    FILE *fp = NULL;

    fp = fopen(path, "r");
    if (fp == NULL) {
        printf("[DIAG] %s open %s failed errno=%d\n", tag, path, errno);
        return;
    }

    printf("[DIAG] ===== %s: %s =====\n", tag, path);
    while (fgets(line, sizeof(line), fp) != NULL) {
        printf("%s", line);
    }
    printf("[DIAG] ===== end %s =====\n", tag);

    fclose(fp);
}

static int selftest_read_spacc_irq_snapshot(SelftestIrqSnapshot_S *snapshot)
{
    char line[256] = {0};
    FILE *fp = NULL;

    if (snapshot == NULL) {
        return TD_FAILURE;
    }

    memset(snapshot, 0, sizeof(SelftestIrqSnapshot_S));
    fp = fopen("/proc/interrupts", "r");
    if (fp == NULL) {
        printf("[DIAG] irq snapshot open /proc/interrupts failed errno=%d\n", errno);
        return TD_FAILURE;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        unsigned int linux_irq = 0;
        unsigned int gic_irq = 0;
        unsigned long long cpu0_count = 0;
        unsigned long long cpu1_count = 0;

        if (sscanf(line, " %u: %llu %llu GIC-0 %u", &linux_irq, &cpu0_count, &cpu1_count, &gic_irq) == 4 &&
            gic_irq >= SELFTEST_IRQ_DIAG_FIRST_GIC && gic_irq <= SELFTEST_IRQ_DIAG_LAST_GIC) {
            td_u32 index = gic_irq - SELFTEST_IRQ_DIAG_FIRST_GIC;
            snapshot->valid[index] = TD_TRUE;
            snapshot->total[index] = cpu0_count + cpu1_count;
        }
    }

    fclose(fp);
    return TD_SUCCESS;
}

static void selftest_dump_spacc_irq_delta(const SelftestIrqSnapshot_S *before, const SelftestIrqSnapshot_S *after)
{
    td_u32 i = 0;

    if (before == NULL || after == NULL) {
        return;
    }

    /*
     * test: 板端 /proc/interrupts 对 SPACC/PKE 中断没有名称，只能跟踪 GIC 101-104。
     * 这些编号来自当前设备诊断输出，用于判断 SYMC 提交后是否有硬件中断响应。
     */
    printf("[DIAG] SYMC IRQ delta:");
    for (i = 0; i < SELFTEST_IRQ_DIAG_COUNT; ++i) {
        unsigned int gic_irq = SELFTEST_IRQ_DIAG_FIRST_GIC + i;
        if (before->valid[i] == TD_TRUE && after->valid[i] == TD_TRUE) {
            printf(" GIC-%u=%llu->%llu(+%llu)", gic_irq, before->total[i], after->total[i],
                   after->total[i] >= before->total[i] ? after->total[i] - before->total[i] : 0);
        } else {
            printf(" GIC-%u=N/A", gic_irq);
        }
    }
    printf("\n");
}

static td_bool selftest_name_has_security_keyword(const char *name)
{
    if (name == NULL) {
        return TD_FALSE;
    }

    if (strstr(name, "cipher") != NULL || strstr(name, "crypto") != NULL || strstr(name, "klad") != NULL ||
        strstr(name, "keyslot") != NULL || strstr(name, "km") != NULL || strstr(name, "otp") != NULL ||
        strstr(name, "mmz") != NULL) {
        return TD_TRUE;
    }

    return TD_FALSE;
}

static void selftest_dump_dev_security_nodes(void)
{
    DIR *dir = NULL;
    struct dirent *entry = NULL;
    td_bool found = TD_FALSE;

    dir = opendir("/dev");
    if (dir == NULL) {
        printf("[DIAG] dev nodes open /dev failed errno=%d\n", errno);
        return;
    }

    /*
     * test: 只打印和安全子系统、MMZ 相关的设备节点，避免 /dev 全量输出干扰
     * SYMC 超时定位。节点缺失通常说明用户态库和内核模块/设备树没有完整匹配。
     */
    printf("[DIAG] ===== dev security nodes: /dev =====\n");
    while ((entry = readdir(dir)) != NULL) {
        if (selftest_name_has_security_keyword(entry->d_name) == TD_TRUE) {
            printf("[DIAG] /dev/%s\n", entry->d_name);
            found = TD_TRUE;
        }
    }
    if (found != TD_TRUE) {
        printf("[DIAG] no matched security/mmz device node\n");
    }
    printf("[DIAG] ===== end dev security nodes =====\n");

    closedir(dir);
}

static int selftest_diag(void)
{
    /*
     * test: SYMC 返回 ERROR_SYMC_CALC_TIMEOUT 时，需要先确认驱动节点、proc 状态和
     * 中断计数。该函数只读系统状态，不修改板端配置。
     */
    selftest_dump_dev_security_nodes();
    selftest_dump_text_file("/proc/cipher", "cipher proc");
    selftest_dump_text_file("/proc/crypto", "linux crypto proc");
    selftest_dump_text_file("/proc/umap/media-mem", "media mem proc");
    selftest_dump_text_file("/proc/media-mem", "media mem legacy proc");
    selftest_dump_text_file("/proc/devices", "devices");
    selftest_dump_text_file("/proc/iomem", "iomem");
    selftest_dump_text_file("/proc/interrupts", "interrupts");
    selftest_dump_text_file("/proc/modules", "modules");

    return TD_SUCCESS;
}

static int selftest_mmz_alloc(SelftestMmzBuf_S *buf, td_u32 size)
{
    td_s32 ret = TD_SUCCESS;

    if (buf == NULL || size == 0) {
        return TD_FAILURE;
    }

    memset(buf, 0, sizeof(SelftestMmzBuf_S));
    ret = ss_mpi_sys_mmz_alloc(&buf->phys_addr, (td_void **)&buf->virt_addr, "cipher_selftest", TD_NULL, size);
    if (ret != TD_SUCCESS) {
        printf("[FAIL] ss_mpi_sys_mmz_alloc ret=0x%08X\n", (unsigned int)ret);
        return ret;
    }

    buf->size = size;
    memset(buf->virt_addr, 0, size);
    return TD_SUCCESS;
}

static void selftest_mmz_free(SelftestMmzBuf_S *buf)
{
    if (buf == NULL || buf->virt_addr == NULL) {
        return;
    }

    (td_void)ss_mpi_sys_mmz_free(buf->phys_addr, buf->virt_addr);
    memset(buf, 0, sizeof(SelftestMmzBuf_S));
}

static int selftest_sm3_once(const td_u8 *data, td_u32 len, td_u8 out[SELFTEST_SM3_LEN])
{
    td_s32 ret = TD_SUCCESS;
    td_handle hash_handle = 0;
    td_u32 out_len = 0;
    crypto_hash_attr hash_attr;
    crypto_buf_attr buf_attr;

    memset(&hash_attr, 0, sizeof(hash_attr));
    hash_attr.hash_type = CRYPTO_HASH_TYPE_SM3;
    hash_attr.is_keyslot = TD_FALSE;
    hash_attr.is_long_term = TD_FALSE;

    ret = ot_mpi_cipher_hash_create(&hash_handle, &hash_attr);
    if (ret != TD_SUCCESS) {
        printf("[FAIL] ot_mpi_cipher_hash_create ret=0x%08X\n", (unsigned int)ret);
        return ret;
    }

    if (len > 0) {
        memset(&buf_attr, 0, sizeof(buf_attr));
        buf_attr.virt_addr = (td_void *)data;
        buf_attr.buf_sec = CRYPTO_BUF_NONSECURE;

        ret = ot_mpi_cipher_hash_update(hash_handle, &buf_attr, len);
        if (ret != TD_SUCCESS) {
            printf("[FAIL] ot_mpi_cipher_hash_update ret=0x%08X\n", (unsigned int)ret);
            (td_void)ot_mpi_cipher_hash_destroy(hash_handle);
            return ret;
        }
    }

    /* note: hash_finish 成功后底层会销毁 HASH 句柄，失败时才需要调用 destroy。 */
    ret = ot_mpi_cipher_hash_finish(hash_handle, out, SELFTEST_SM3_LEN, &out_len);
    if (ret != TD_SUCCESS) {
        printf("[FAIL] ot_mpi_cipher_hash_finish ret=0x%08X\n", (unsigned int)ret);
        (td_void)ot_mpi_cipher_hash_destroy(hash_handle);
        return ret;
    }

    if (out_len != SELFTEST_SM3_LEN) {
        printf("[FAIL] SM3 output length=%u\n", out_len);
        return TD_FAILURE;
    }

    return TD_SUCCESS;
}

static int selftest_sm3_split(const td_u8 *part1, td_u32 len1, const td_u8 *part2, td_u32 len2, td_u8 out[SELFTEST_SM3_LEN])
{
    td_s32 ret = TD_SUCCESS;
    td_handle hash_handle = 0;
    td_u32 out_len = 0;
    crypto_hash_attr hash_attr;
    crypto_buf_attr buf_attr;

    memset(&hash_attr, 0, sizeof(hash_attr));
    hash_attr.hash_type = CRYPTO_HASH_TYPE_SM3;

    ret = ot_mpi_cipher_hash_create(&hash_handle, &hash_attr);
    if (ret != TD_SUCCESS) {
        printf("[FAIL] ot_mpi_cipher_hash_create(split) ret=0x%08X\n", (unsigned int)ret);
        return ret;
    }

    memset(&buf_attr, 0, sizeof(buf_attr));
    buf_attr.virt_addr = (td_void *)part1;
    buf_attr.buf_sec = CRYPTO_BUF_NONSECURE;
    ret = ot_mpi_cipher_hash_update(hash_handle, &buf_attr, len1);
    if (ret != TD_SUCCESS) {
        printf("[FAIL] ot_mpi_cipher_hash_update(part1) ret=0x%08X\n", (unsigned int)ret);
        (td_void)ot_mpi_cipher_hash_destroy(hash_handle);
        return ret;
    }

    buf_attr.virt_addr = (td_void *)part2;
    ret = ot_mpi_cipher_hash_update(hash_handle, &buf_attr, len2);
    if (ret != TD_SUCCESS) {
        printf("[FAIL] ot_mpi_cipher_hash_update(part2) ret=0x%08X\n", (unsigned int)ret);
        (td_void)ot_mpi_cipher_hash_destroy(hash_handle);
        return ret;
    }

    ret = ot_mpi_cipher_hash_finish(hash_handle, out, SELFTEST_SM3_LEN, &out_len);
    if (ret != TD_SUCCESS) {
        printf("[FAIL] ot_mpi_cipher_hash_finish(split) ret=0x%08X\n", (unsigned int)ret);
        (td_void)ot_mpi_cipher_hash_destroy(hash_handle);
        return ret;
    }

    return (out_len == SELFTEST_SM3_LEN) ? TD_SUCCESS : TD_FAILURE;
}

static int selftest_trng(void)
{
    td_s32 ret = TD_SUCCESS;
    td_u32 rand_num = 0;
    td_u8 rand_buf[SELFTEST_TRNG_MAX_LEN] = {0};

    ret = cipherTrng_getRandom(&rand_num);
    if (selftest_check_ret("TRNG get_random", ret) != TD_SUCCESS) {
        return ret;
    }
    printf("TRNG-4: %08x\n", rand_num);

    ret = cipherTrng_getMultiRandom(16, rand_buf);
    if (selftest_check_ret("TRNG get_multi_random 16", ret) != TD_SUCCESS) {
        return ret;
    }
    selftest_print_hex("TRNG-16", rand_buf, 16);

    ret = cipherTrng_getMultiRandom(SELFTEST_TRNG_MAX_LEN, rand_buf);
    if (selftest_check_ret("TRNG get_multi_random 1024", ret) != TD_SUCCESS) {
        return ret;
    }

    ret = ot_mpi_cipher_trng_get_multi_random(0, rand_buf);
    if (ret == TD_SUCCESS) {
        printf("[FAIL] TRNG invalid size 0 unexpectedly success\n");
        return TD_FAILURE;
    }
    printf("[PASS] TRNG invalid size 0 rejected ret=0x%08X\n", (unsigned int)ret);

    ret = ot_mpi_cipher_trng_get_multi_random(SELFTEST_TRNG_MAX_LEN + 1, rand_buf);
    if (ret == TD_SUCCESS) {
        printf("[FAIL] TRNG invalid size 1025 unexpectedly success\n");
        return TD_FAILURE;
    }
    printf("[PASS] TRNG invalid size 1025 rejected ret=0x%08X\n", (unsigned int)ret);

    return TD_SUCCESS;
}

static int selftest_sm3(void)
{
    td_s32 ret = TD_SUCCESS;
    const td_u8 hello[] = "Hello";
    td_u8 digest[SELFTEST_SM3_LEN] = {0};
    td_u8 digest_split[SELFTEST_SM3_LEN] = {0};
    td_u32 out_len = 0;
    CipherContextNeedParam_S need_param;
    CipherContext_S *ctx = NULL;

    ret = ot_mpi_cipher_hash_init();
    if (selftest_check_ret("HASH init", ret) != TD_SUCCESS) {
        return ret;
    }

    ret = selftest_sm3_once(NULL, 0, digest);
    if (ret == TD_SUCCESS) {
        ret = selftest_mem_equal("SM3 empty digest", digest, g_aSm3EmptyDigest, SELFTEST_SM3_LEN);
    }
    if (ret != TD_SUCCESS) {
        ot_mpi_cipher_hash_deinit();
        return ret;
    }

    ret = selftest_sm3_once(hello, 5, digest);
    if (ret == TD_SUCCESS) {
        ret = selftest_mem_equal("SM3 Hello digest", digest, g_aSm3HelloDigest, SELFTEST_SM3_LEN);
    }
    if (ret != TD_SUCCESS) {
        ot_mpi_cipher_hash_deinit();
        return ret;
    }

    ret = selftest_sm3_split((const td_u8 *)"Hel", 3, (const td_u8 *)"lo", 2, digest_split);
    if (ret == TD_SUCCESS) {
        ret = selftest_mem_equal("SM3 split equals once", digest_split, digest, SELFTEST_SM3_LEN);
    }
    ot_mpi_cipher_hash_deinit();
    if (ret != TD_SUCCESS) {
        return ret;
    }

    memset(&need_param, 0, sizeof(need_param));
    need_param.bEnableHash = TD_TRUE;
    ctx = cipherContext_alloc(need_param);
    if (ctx == NULL) {
        printf("[FAIL] cipherContext_alloc\n");
        return TD_FAILURE;
    }

    ret = ctx->cipherContext_init(ctx);
    if (ret == TD_SUCCESS) {
        memset(digest, 0, sizeof(digest));
        ret = ctx->cipherContext_sm3_compute(ctx, hello, 5, digest, SELFTEST_SM3_LEN, &out_len);
    }
    if (ret == TD_SUCCESS) {
        ret = selftest_mem_equal("cipherContext SM3 Hello", digest, g_aSm3HelloDigest, SELFTEST_SM3_LEN);
    }

    ctx->cipherContext_uninit(ctx);
    cipherContext_release(ctx);
    return ret;
}

static int selftest_pke_sm2(void)
{
    td_s32 ret = TD_SUCCESS;
    td_bool is_on_curve = TD_FALSE;
    const td_u8 sm2_id[] = "1234567812345678";
    const td_u8 msg_data[] = "Hello";
    td_u8 priv_buf[SELFTEST_SM2_LEN] = {0};
    td_u8 pub_x[SELFTEST_SM2_LEN] = {0};
    td_u8 pub_y[SELFTEST_SM2_LEN] = {0};
    td_u8 hash_buf[SELFTEST_SM3_LEN] = {0};
    td_u8 sig_r[SELFTEST_SM2_LEN] = {0};
    td_u8 sig_s[SELFTEST_SM2_LEN] = {0};
    drv_pke_data priv = {SELFTEST_SM2_LEN, priv_buf};
    drv_pke_ecc_point pub = {pub_x, pub_y, SELFTEST_SM2_LEN};
    drv_pke_data id = {sizeof(sm2_id) - 1, (td_u8 *)sm2_id};
    drv_pke_msg msg = {sizeof(msg_data) - 1, (td_u8 *)msg_data, DRV_PKE_BUF_NONSECURE};
    drv_pke_data hash = {SELFTEST_SM3_LEN, hash_buf};
    drv_pke_ecc_sig sig = {sig_r, sig_s, SELFTEST_SM2_LEN};
    CipherPkeNeedParam_S need_param;
    CipherPke_S *pke = NULL;

    memset(&need_param, 0, sizeof(need_param));
    pke = cipherPke_alloc(need_param);
    if (pke == NULL) {
        printf("[FAIL] cipherPke_alloc\n");
        return TD_FAILURE;
    }

    ret = pke->cipherPke_init(pke);
    if (ret == TD_SUCCESS) {
        ret = pke->cipherPke_ecc_gen_key(pke, DRV_PKE_ECC_TYPE_SM2, NULL, &priv, &pub);
    }
    if (ret == TD_SUCCESS) {
        ret = pke->cipherPke_check_dot_on_curve(pke, DRV_PKE_ECC_TYPE_SM2, &pub, &is_on_curve);
    }
    if (ret == TD_SUCCESS) {
        ret = selftest_check_true("SM2 public key on curve", is_on_curve);
    }
    if (ret == TD_SUCCESS) {
        ret = pke->cipherPke_sm2_dsa_hash(pke, &id, &pub, &msg, &hash);
    }
    if (ret == TD_SUCCESS) {
        selftest_print_hex("SM2 ZA HASH", hash_buf, SELFTEST_SM3_LEN);
        ret = pke->cipherPke_sm2_sign(pke, DRV_PKE_ECC_TYPE_SM2, &priv, &hash, &sig);
    }
    if (ret == TD_SUCCESS) {
        ret = pke->cipherPke_sm2_verify(pke, DRV_PKE_ECC_TYPE_SM2, &pub, &hash, &sig);
    }
    if (ret == TD_SUCCESS) {
        printf("[PASS] SM2 sign/verify\n");
    } else {
        printf("[FAIL] SM2 sign/verify ret=0x%08X\n", (unsigned int)ret);
    }

    pke->cipherPke_uninit(pke);
    cipherPke_release(pke);
    return ret;
}

static int selftest_ctx_sm2_sign_verify(void)
{
    td_s32 ret = TD_SUCCESS;
    const td_u8 sm2_id[] = "1234567812345678";
    const td_u8 msg_data[] = "Hello";
    td_u8 priv_buf[SELFTEST_SM2_LEN] = {0};
    td_u8 pub_x[SELFTEST_SM2_LEN] = {0};
    td_u8 pub_y[SELFTEST_SM2_LEN] = {0};
    td_u8 hash_buf[SELFTEST_SM3_LEN] = {0};
    td_u8 sig_r[SELFTEST_SM2_LEN] = {0};
    td_u8 sig_s[SELFTEST_SM2_LEN] = {0};
    CipherContextNeedParam_S need_param = {0};
    CipherContext_S *ctx = NULL;

    need_param.bEnablePke = TD_TRUE;
    ctx = cipherContext_alloc(need_param);
    if (ctx == NULL) {
        printf("[FAIL] cipherContext_alloc PKE\n");
        return TD_FAILURE;
    }

    ret = ctx->cipherContext_init(ctx);
    if (ret == TD_SUCCESS) {
        ret = ctx->cipherContext_sm2_keygen(ctx, priv_buf, sizeof(priv_buf), pub_x, sizeof(pub_x), pub_y, sizeof(pub_y));
    }
    if (ret == TD_SUCCESS) {
        ret = ctx->cipherContext_sm2_dsa_hash(ctx, sm2_id, sizeof(sm2_id) - 1, pub_x, sizeof(pub_x), pub_y, sizeof(pub_y),
                                              msg_data, sizeof(msg_data) - 1, hash_buf, sizeof(hash_buf));
    }
    if (ret == TD_SUCCESS) {
        ret = ctx->cipherContext_sm2_sign(ctx, priv_buf, sizeof(priv_buf), hash_buf, sizeof(hash_buf),
                                          sig_r, sizeof(sig_r), sig_s, sizeof(sig_s));
    }
    if (ret == TD_SUCCESS) {
        ret = ctx->cipherContext_sm2_verify(ctx, pub_x, sizeof(pub_x), pub_y, sizeof(pub_y),
                                            hash_buf, sizeof(hash_buf), sig_r, sizeof(sig_r), sig_s, sizeof(sig_s));
    }
    if (ret == TD_SUCCESS) {
        printf("[PASS] cipherContext SM2 sign/verify\n");
    } else {
        printf("[FAIL] cipherContext SM2 sign/verify ret=0x%08X\n", (unsigned int)ret);
    }

    (td_void)ctx->cipherContext_uninit(ctx);
    cipherContext_release(ctx);
    return ret;
}

static int selftest_ctx_sm2_encrypt_decrypt(void)
{
    td_s32 ret = TD_SUCCESS;
    td_u8 priv_buf[SELFTEST_SM2_LEN] = {0};
    td_u8 pub_x[SELFTEST_SM2_LEN] = {0};
    td_u8 pub_y[SELFTEST_SM2_LEN] = {0};
    const td_u8 plain[] = "Hello, SM2 decrypt hardware path.";
    td_u8 cipher[sizeof(plain) - 1 + 97] = {0};
    td_u8 decrypt[sizeof(plain)] = {0};
    td_u32 cipher_len = 0;
    td_u32 decrypt_len = 0;
    CipherContextNeedParam_S need_param = {0};
    CipherContext_S *ctx = NULL;

    need_param.bEnablePke = TD_TRUE;
    ctx = cipherContext_alloc(need_param);
    if (ctx == NULL) {
        printf("[FAIL] cipherContext_alloc PKE crypto\n");
        return TD_FAILURE;
    }

    ret = ctx->cipherContext_init(ctx);
    if (ret == TD_SUCCESS) {
        ret = ctx->cipherContext_sm2_keygen(ctx, priv_buf, sizeof(priv_buf), pub_x, sizeof(pub_x), pub_y, sizeof(pub_y));
    }
    if (ret == TD_SUCCESS) {
        ret = ctx->cipherContext_sm2_encrypt(ctx, pub_x, sizeof(pub_x), pub_y, sizeof(pub_y),
                                             plain, sizeof(plain) - 1, cipher, sizeof(cipher), &cipher_len);
    }
    if (ret == TD_SUCCESS) {
        ret = ctx->cipherContext_sm2_decrypt(ctx, priv_buf, sizeof(priv_buf),
                                             cipher, cipher_len, decrypt, sizeof(decrypt), &decrypt_len);
    }
    if (ret == TD_SUCCESS && decrypt_len == sizeof(plain) - 1 && memcmp(plain, decrypt, decrypt_len) == 0) {
        printf("[PASS] cipherContext SM2 encrypt/decrypt\n");
    } else {
        printf("[FAIL] cipherContext SM2 encrypt/decrypt ret=0x%08X\n", (unsigned int)ret);
        ret = TD_FAILURE;
    }

    (td_void)ctx->cipherContext_uninit(ctx);
    cipherContext_release(ctx);
    return ret;
}

static int selftest_openssl_sm4_cbc_raw(const td_u8 key[SELFTEST_SM4_BLOCK_LEN],
                                        const td_u8 iv[SELFTEST_SM4_BLOCK_LEN],
                                        const td_u8 *plain,
                                        td_u32 plain_len,
                                        td_u8 *cipher,
                                        td_u32 *cipher_len)
{
    int len = 0;
    int total = 0;
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();

    if (ctx == NULL) {
        return TD_FAILURE;
    }

    if (EVP_EncryptInit_ex(ctx, EVP_sm4_cbc(), NULL, key, iv) != 1 ||
        EVP_CIPHER_CTX_set_padding(ctx, 0) != 1 ||
        EVP_EncryptUpdate(ctx, cipher, &len, plain, (int)plain_len) != 1 ||
        EVP_EncryptFinal_ex(ctx, cipher + len, &total) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return TD_FAILURE;
    }

    *cipher_len = (td_u32)(len + total);
    EVP_CIPHER_CTX_free(ctx);
    return TD_SUCCESS;
}

static int selftest_openssl_aes_128_cbc_raw(const td_u8 key[SELFTEST_SM4_BLOCK_LEN],
                                            const td_u8 iv[SELFTEST_SM4_BLOCK_LEN],
                                            const td_u8 *plain,
                                            td_u32 plain_len,
                                            td_u8 *cipher,
                                            td_u32 *cipher_len)
{
    int len = 0;
    int total = 0;
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();

    if (ctx == NULL) {
        return TD_FAILURE;
    }

    if (EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv) != 1 ||
        EVP_CIPHER_CTX_set_padding(ctx, 0) != 1 ||
        EVP_EncryptUpdate(ctx, cipher, &len, plain, (int)plain_len) != 1 ||
        EVP_EncryptFinal_ex(ctx, cipher + len, &total) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return TD_FAILURE;
    }

    *cipher_len = (td_u32)(len + total);
    EVP_CIPHER_CTX_free(ctx);
    return TD_SUCCESS;
}

static int selftest_openssl_sm4_cbc_pkcs7_base64(const td_u8 key[SELFTEST_SM4_BLOCK_LEN],
                                                 const td_u8 iv[SELFTEST_SM4_BLOCK_LEN],
                                                 const td_u8 *plain,
                                                 td_u32 plain_len,
                                                 char *base64,
                                                 td_u32 base64_size)
{
    int len = 0;
    int total = 0;
    int enc_len = 0;
    td_u8 cipher[128] = {0};
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();

    if (ctx == NULL) {
        return TD_FAILURE;
    }

    if (EVP_EncryptInit_ex(ctx, EVP_sm4_cbc(), NULL, key, iv) != 1 ||
        EVP_EncryptUpdate(ctx, cipher, &len, plain, (int)plain_len) != 1 ||
        EVP_EncryptFinal_ex(ctx, cipher + len, &total) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return TD_FAILURE;
    }

    enc_len = EVP_EncodeBlock((unsigned char *)base64, cipher, len + total);
    EVP_CIPHER_CTX_free(ctx);
    if (enc_len <= 0 || (td_u32)enc_len >= base64_size) {
        return TD_FAILURE;
    }

    base64[enc_len] = '\0';
    return TD_SUCCESS;
}

static int selftest_cbc_hw_raw(crypto_symc_alg symc_alg,
                               km_crypto_alg km_engine,
                               const char *case_name,
                               td_bool deinit_km_before_crypt,
                               const td_u8 key[SELFTEST_SM4_BLOCK_LEN],
                               const td_u8 iv[SELFTEST_SM4_BLOCK_LEN],
                               const td_u8 *input,
                               td_u32 input_len,
                               td_u8 *output,
                               td_bool encrypt)
{
    td_s32 ret = TD_SUCCESS;
    CipherKmNeedParam_S km_param;
    CipherSymcNeedParam_S symc_param;
    CipherKm_S *km = NULL;
    CipherSymc_S *symc = NULL;
    SelftestMmzBuf_S src_buf = {0};
    SelftestMmzBuf_S dst_buf = {0};
    crypto_buf_attr src_attr;
    crypto_buf_attr dst_attr;
    crypto_symc_attr symc_attr;
    crypto_symc_ctrl_t symc_ctrl;
    SelftestIrqSnapshot_S irq_before;
    SelftestIrqSnapshot_S irq_after;
    td_bool has_irq_before = TD_FALSE;

    if (input_len == 0 || input_len % SELFTEST_SM4_BLOCK_LEN != 0) {
        return TD_FAILURE;
    }

    memset(&km_param, 0, sizeof(km_param));
    km_param.enKeyslot_type = KM_KEYSLOT_TYPE_MCIPHER;
    km_param.enKlad_type = KM_KLAD_DEST_TYPE_MCIPHER;
    km_param.enEngine = km_engine;
    km_param.bDecrypt_support = TD_TRUE;
    km_param.bEncrypt_support = TD_TRUE;

    memset(&symc_param, 0, sizeof(symc_param));
    km = cipherKm_alloc(km_param);
    symc = cipherSymc_alloc(symc_param);
    if (km == NULL || symc == NULL) {
        ret = TD_FAILURE;
        goto cleanup;
    }

    ret = km->cipherKm_init(km);
    if (ret != TD_SUCCESS) {
        goto cleanup;
    }
    ret = km->cipherKm_set_clear_key(km, key, SELFTEST_SM4_BLOCK_LEN);
    if (ret != TD_SUCCESS) {
        printf("[FAIL] %s set clear key ret=0x%08X\n", case_name, (unsigned int)ret);
        goto cleanup;
    }

    if (deinit_km_before_crypt == TD_TRUE) {
        /*
         * test: PDF 的明文 KEY 传递流程在 set_clear_key 后会 deinit KM。该分支用于确认
         * MPI 层保持 KM init 状态是否会影响后续 SYMC 使用已写入 keyslot 的工作密钥。
         */
        ret = ot_mpi_km_deinit();
        if (ret != TD_SUCCESS) {
            printf("[FAIL] %s km_deinit before crypt ret=0x%08X\n", case_name, (unsigned int)ret);
            goto cleanup;
        }
        /*
         * test: KM deinit 会释放当前进程在 KAPI 中登记的 keyslot 上下文。后续为了验证
         * PDF 流程仍会尝试用这个硬件 keyslot 做一次 SYMC，但清理阶段不能再 destroy
         * 同一个 MPI handle，否则会产生 invalid keyslot_handle 的误导性日志。
         */
        km->bKmInited = TD_FALSE;
        km->bKeyslotCreated = TD_FALSE;
    }

    ret = symc->cipherSymc_init(symc);
    if (ret != TD_SUCCESS) {
        goto cleanup;
    }

    ret = selftest_mmz_alloc(&src_buf, input_len);
    if (ret != TD_SUCCESS) {
        goto cleanup;
    }
    ret = selftest_mmz_alloc(&dst_buf, input_len);
    if (ret != TD_SUCCESS) {
        goto cleanup;
    }
    memcpy(src_buf.virt_addr, input, input_len);
    ss_mpi_sys_flush_cache(src_buf.phys_addr, src_buf.virt_addr, input_len);

    memset(&src_attr, 0, sizeof(src_attr));
    src_attr.phys_addr = src_buf.phys_addr;
    src_attr.virt_addr = src_buf.virt_addr;
    src_attr.buf_sec = CRYPTO_BUF_NONSECURE;

    memset(&dst_attr, 0, sizeof(dst_attr));
    dst_attr.phys_addr = dst_buf.phys_addr;
    dst_attr.virt_addr = dst_buf.virt_addr;
    dst_attr.buf_sec = CRYPTO_BUF_NONSECURE;

    memset(&symc_attr, 0, sizeof(symc_attr));
    symc_attr.symc_type = CRYPTO_SYMC_TYPE_NORMAL;
    symc_attr.symc_alg = symc_alg;
    symc_attr.work_mode = CRYPTO_SYMC_WORK_MODE_CBC;
    symc_attr.is_long_term = TD_FALSE;

    memset(&symc_ctrl, 0, sizeof(symc_ctrl));
    symc_ctrl.symc_alg = symc_alg;
    symc_ctrl.work_mode = CRYPTO_SYMC_WORK_MODE_CBC;
    symc_ctrl.symc_key_length = CRYPTO_SYMC_KEY_128BIT;
    symc_ctrl.key_parity = CRYPTO_SYMC_KEY_ODD;
    symc_ctrl.symc_bit_width = CRYPTO_SYMC_BIT_WIDTH_128BIT;
    symc_ctrl.iv_change_flag = CRYPTO_SYMC_IV_DO_NOT_CHANGE;
    symc_ctrl.iv_length = SELFTEST_SM4_BLOCK_LEN;
    memcpy(symc_ctrl.iv, iv, SELFTEST_SM4_BLOCK_LEN);

    if (selftest_read_spacc_irq_snapshot(&irq_before) == TD_SUCCESS) {
        has_irq_before = TD_TRUE;
    }

    if (encrypt == TD_TRUE) {
        printf("[DIAG] %s encrypt alg=%u keyslot=0x%08X src_phys=0x%llX dst_phys=0x%llX len=%u\n", case_name,
               (unsigned int)symc_alg, (unsigned int)km->mpi_keyslot_handle,
               (unsigned long long)src_attr.phys_addr, (unsigned long long)dst_attr.phys_addr,
               (unsigned int)input_len);
        ret = symc->cipherSymc_encryption(symc, symc_attr, km->mpi_keyslot_handle, symc_ctrl, &src_attr, &dst_attr, input_len, NULL);
    } else {
        printf("[DIAG] %s decrypt alg=%u keyslot=0x%08X src_phys=0x%llX dst_phys=0x%llX len=%u\n", case_name,
               (unsigned int)symc_alg, (unsigned int)km->mpi_keyslot_handle,
               (unsigned long long)src_attr.phys_addr, (unsigned long long)dst_attr.phys_addr,
               (unsigned int)input_len);
        ret = symc->cipherSymc_decryption(symc, symc_attr, km->mpi_keyslot_handle, symc_ctrl, &src_attr, &dst_attr, input_len, NULL);
    }
    if (has_irq_before == TD_TRUE && selftest_read_spacc_irq_snapshot(&irq_after) == TD_SUCCESS) {
        selftest_dump_spacc_irq_delta(&irq_before, &irq_after);
    }
    if (ret != TD_SUCCESS) {
        printf("[FAIL] %s %s ret=0x%08X\n", case_name, encrypt == TD_TRUE ? "encrypt" : "decrypt", (unsigned int)ret);
        goto cleanup;
    }

    ss_mpi_sys_flush_cache(dst_buf.phys_addr, dst_buf.virt_addr, input_len);
    memcpy(output, dst_buf.virt_addr, input_len);

cleanup:
    selftest_mmz_free(&dst_buf);
    selftest_mmz_free(&src_buf);
    if (symc != NULL) {
        symc->cipherSymc_uninit(symc);
        cipherSymc_release(symc);
    }
    if (km != NULL) {
        km->cipherKm_uninit(km);
        cipherKm_release(km);
    }
    return ret;
}

static int selftest_set_clear_key_sample_flow(crypto_handle keyslot_handle,
                                              km_crypto_alg km_engine,
                                              const td_u8 *key,
                                              td_u32 key_len)
{
    td_s32 ret = TD_SUCCESS;
    crypto_handle klad_handle = 0;
    td_u8 tee_enable = 0;
    km_klad_attr klad_attr;
    km_klad_clear_key clear_key;

    if (key == NULL || key_len == 0) {
        return TD_FAILURE;
    }

    memset(&klad_attr, 0, sizeof(klad_attr));
    klad_attr.key_cfg.engine = km_engine;
    klad_attr.key_cfg.decrypt_support = TD_TRUE;
    klad_attr.key_cfg.encrypt_support = TD_TRUE;

    memset(&clear_key, 0, sizeof(clear_key));
    clear_key.key = (td_u8 *)key;
    clear_key.key_size = key_len;

    /*
     * test: 对齐官方 sample 的 OTP 0x12 安全域判断。当前平台提供的是
     * libot_mpi_otp.so，因此这里调用 ot_mpi_otp_*，其语义与 sample 中的 ss_mpi_otp_* 一致。
     */
    ret = ot_mpi_otp_init();
    if (ret == TD_SUCCESS) {
        ret = ot_mpi_otp_read_byte(0x12, &tee_enable);
        (td_void)ot_mpi_otp_deinit();
    }
    if (ret != TD_SUCCESS) {
        printf("[DIAG] sample-flow OTP 0x12 read failed ret=0x%08X, fallback to REE non-secure\n",
               (unsigned int)ret);
        tee_enable = 0;
    } else {
        printf("[DIAG] sample-flow OTP[0x12]=0x%02X\n", tee_enable);
    }
    if (tee_enable == 0x42) {
        klad_attr.key_sec_cfg.key_sec = KM_KLAD_SEC_ENABLE;
        klad_attr.key_sec_cfg.master_only_enable = TD_TRUE;
        klad_attr.key_sec_cfg.dest_buf_sec_support = TD_TRUE;
        klad_attr.key_sec_cfg.src_buf_sec_support = TD_TRUE;
        klad_attr.key_sec_cfg.src_buf_non_sec_support = TD_FALSE;
        klad_attr.key_sec_cfg.dest_buf_non_sec_support = TD_FALSE;
    } else {
        klad_attr.key_sec_cfg.key_sec = KM_KLAD_SEC_DISABLE;
        klad_attr.key_sec_cfg.master_only_enable = TD_FALSE;
        klad_attr.key_sec_cfg.dest_buf_sec_support = TD_FALSE;
        klad_attr.key_sec_cfg.dest_buf_non_sec_support = TD_TRUE;
        klad_attr.key_sec_cfg.src_buf_sec_support = TD_FALSE;
        klad_attr.key_sec_cfg.src_buf_non_sec_support = TD_TRUE;
    }

    ret = ss_mpi_klad_create(&klad_handle);
    if (ret != TD_SUCCESS) {
        printf("[FAIL] sample-flow ss_mpi_klad_create ret=0x%08X\n", (unsigned int)ret);
        return ret;
    }

    ret = ss_mpi_klad_set_attr(klad_handle, &klad_attr);
    if (ret != TD_SUCCESS) {
        printf("[FAIL] sample-flow ss_mpi_klad_set_attr ret=0x%08X\n", (unsigned int)ret);
        goto cleanup_destroy;
    }

    ret = ss_mpi_klad_attach(klad_handle, KM_KLAD_DEST_TYPE_MCIPHER, keyslot_handle);
    if (ret != TD_SUCCESS) {
        printf("[FAIL] sample-flow ss_mpi_klad_attach ret=0x%08X\n", (unsigned int)ret);
        goto cleanup_destroy;
    }

    ret = ss_mpi_klad_set_clear_key(klad_handle, &clear_key);
    if (ret != TD_SUCCESS) {
        printf("[FAIL] sample-flow ss_mpi_klad_set_clear_key ret=0x%08X\n", (unsigned int)ret);
    }

    (td_void)ss_mpi_klad_detach(klad_handle, KM_KLAD_DEST_TYPE_MCIPHER, keyslot_handle);

cleanup_destroy:
    (td_void)ss_mpi_klad_destroy(klad_handle);
    return ret;
}

static int selftest_cbc_hw_raw_sample_flow(crypto_symc_alg symc_alg,
                                           km_crypto_alg km_engine,
                                           const char *case_name,
                                           const td_u8 key[SELFTEST_SM4_BLOCK_LEN],
                                           const td_u8 iv[SELFTEST_SM4_BLOCK_LEN],
                                           const td_u8 *input,
                                           td_u32 input_len,
                                           td_u8 *output)
{
    td_s32 ret = TD_SUCCESS;
    td_handle symc_handle = 0;
    crypto_handle keyslot_handle = 0;
    SelftestMmzBuf_S src_buf = {0};
    SelftestMmzBuf_S dst_buf = {0};
    crypto_buf_attr src_attr;
    crypto_buf_attr dst_attr;
    crypto_symc_attr symc_attr;
    crypto_symc_ctrl_t symc_ctrl;
    td_bool symc_inited = TD_FALSE;
    td_bool km_inited = TD_FALSE;
    td_bool symc_created = TD_FALSE;
    td_bool keyslot_created = TD_FALSE;
    SelftestIrqSnapshot_S irq_before;
    SelftestIrqSnapshot_S irq_after;
    td_bool has_irq_before = TD_FALSE;

    if (input_len == 0 || input_len % SELFTEST_SM4_BLOCK_LEN != 0 || input == NULL || output == NULL) {
        return TD_FAILURE;
    }

    ret = selftest_mmz_alloc(&src_buf, input_len);
    if (ret != TD_SUCCESS) {
        goto cleanup;
    }
    ret = selftest_mmz_alloc(&dst_buf, input_len);
    if (ret != TD_SUCCESS) {
        goto cleanup;
    }
    memcpy(src_buf.virt_addr, input, input_len);
    ss_mpi_sys_flush_cache(src_buf.phys_addr, src_buf.virt_addr, input_len);

    memset(&src_attr, 0, sizeof(src_attr));
    src_attr.phys_addr = src_buf.phys_addr;
    src_attr.virt_addr = src_buf.virt_addr;
    src_attr.buf_sec = CRYPTO_BUF_NONSECURE;

    memset(&dst_attr, 0, sizeof(dst_attr));
    dst_attr.phys_addr = dst_buf.phys_addr;
    dst_attr.virt_addr = dst_buf.virt_addr;
    dst_attr.buf_sec = CRYPTO_BUF_NONSECURE;

    memset(&symc_attr, 0, sizeof(symc_attr));
    symc_attr.symc_type = CRYPTO_SYMC_TYPE_NORMAL;
    symc_attr.is_long_term = TD_FALSE;

    memset(&symc_ctrl, 0, sizeof(symc_ctrl));
    symc_ctrl.symc_alg = symc_alg;
    symc_ctrl.work_mode = CRYPTO_SYMC_WORK_MODE_CBC;
    symc_ctrl.symc_key_length = CRYPTO_SYMC_KEY_128BIT;
    symc_ctrl.symc_bit_width = CRYPTO_SYMC_BIT_WIDTH_128BIT;
    symc_ctrl.iv_length = SELFTEST_SM4_BLOCK_LEN;
    memcpy(symc_ctrl.iv, iv, SELFTEST_SM4_BLOCK_LEN);

    ret = ss_mpi_cipher_symc_init();
    if (ret != TD_SUCCESS) {
        printf("[FAIL] sample-flow ss_mpi_cipher_symc_init ret=0x%08X\n", (unsigned int)ret);
        goto cleanup;
    }
    symc_inited = TD_TRUE;

    ret = ss_mpi_km_init();
    if (ret != TD_SUCCESS) {
        printf("[FAIL] sample-flow ss_mpi_km_init ret=0x%08X\n", (unsigned int)ret);
        goto cleanup;
    }
    km_inited = TD_TRUE;

    ret = ss_mpi_cipher_symc_create(&symc_handle, &symc_attr);
    if (ret != TD_SUCCESS) {
        printf("[FAIL] sample-flow ss_mpi_cipher_symc_create ret=0x%08X\n", (unsigned int)ret);
        goto cleanup;
    }
    symc_created = TD_TRUE;

    ret = ss_mpi_keyslot_create(&keyslot_handle, KM_KEYSLOT_TYPE_MCIPHER);
    if (ret != TD_SUCCESS) {
        printf("[FAIL] sample-flow ss_mpi_keyslot_create ret=0x%08X\n", (unsigned int)ret);
        goto cleanup;
    }
    keyslot_created = TD_TRUE;

    ret = ss_mpi_cipher_symc_attach(symc_handle, (td_handle)keyslot_handle);
    if (ret != TD_SUCCESS) {
        printf("[FAIL] sample-flow ss_mpi_cipher_symc_attach ret=0x%08X\n", (unsigned int)ret);
        goto cleanup;
    }

    ret = selftest_set_clear_key_sample_flow(keyslot_handle, km_engine, key, SELFTEST_SM4_BLOCK_LEN);
    if (ret != TD_SUCCESS) {
        goto cleanup;
    }

    ret = ss_mpi_cipher_symc_set_config(symc_handle, &symc_ctrl);
    if (ret != TD_SUCCESS) {
        printf("[FAIL] sample-flow ss_mpi_cipher_symc_set_config ret=0x%08X\n", (unsigned int)ret);
        goto cleanup;
    }

    if (selftest_read_spacc_irq_snapshot(&irq_before) == TD_SUCCESS) {
        has_irq_before = TD_TRUE;
    }
    printf("[DIAG] %s sample-flow encrypt alg=%u keyslot=0x%08X symc=0x%08X src_phys=0x%llX dst_phys=0x%llX len=%u\n",
           case_name, (unsigned int)symc_alg, (unsigned int)keyslot_handle, (unsigned int)symc_handle,
           (unsigned long long)src_attr.phys_addr, (unsigned long long)dst_attr.phys_addr, (unsigned int)input_len);

    ret = ss_mpi_cipher_symc_encrypt(symc_handle, &src_attr, &dst_attr, input_len);
    if (has_irq_before == TD_TRUE && selftest_read_spacc_irq_snapshot(&irq_after) == TD_SUCCESS) {
        selftest_dump_spacc_irq_delta(&irq_before, &irq_after);
    }
    if (ret != TD_SUCCESS) {
        printf("[FAIL] %s sample-flow encrypt ret=0x%08X\n", case_name, (unsigned int)ret);
        goto cleanup;
    }

    ss_mpi_sys_flush_cache(dst_buf.phys_addr, dst_buf.virt_addr, input_len);
    memcpy(output, dst_buf.virt_addr, input_len);

cleanup:
    if (keyslot_created == TD_TRUE) {
        (td_void)ss_mpi_keyslot_destroy(keyslot_handle);
    }
    if (symc_created == TD_TRUE) {
        (td_void)ss_mpi_cipher_symc_destroy(symc_handle);
    }
    if (km_inited == TD_TRUE) {
        (td_void)ss_mpi_km_deinit();
    }
    if (symc_inited == TD_TRUE) {
        (td_void)ss_mpi_cipher_symc_deinit();
    }
    selftest_mmz_free(&dst_buf);
    selftest_mmz_free(&src_buf);
    return ret;
}

static int selftest_sm4_cbc_hw_raw(const td_u8 key[SELFTEST_SM4_BLOCK_LEN],
                                   const td_u8 iv[SELFTEST_SM4_BLOCK_LEN],
                                   const td_u8 *input,
                                   td_u32 input_len,
                                   td_u8 *output,
                                   td_bool encrypt)
{
    return selftest_cbc_hw_raw(CRYPTO_SYMC_ALG_SM4, KM_CRYPTO_ALG_SM4, "HW SM4-CBC", TD_FALSE, key, iv, input,
                               input_len, output, encrypt);
}

static int selftest_sm4_cbc_hw_raw_sample_flow(const td_u8 key[SELFTEST_SM4_BLOCK_LEN],
                                               const td_u8 iv[SELFTEST_SM4_BLOCK_LEN],
                                               const td_u8 *input,
                                               td_u32 input_len,
                                               td_u8 *output)
{
    return selftest_cbc_hw_raw_sample_flow(CRYPTO_SYMC_ALG_SM4, KM_CRYPTO_ALG_SM4, "HW SM4-CBC", key, iv, input,
                                           input_len, output);
}

static int selftest_aes_cbc_hw_raw(const td_u8 key[SELFTEST_SM4_BLOCK_LEN],
                                   const td_u8 iv[SELFTEST_SM4_BLOCK_LEN],
                                   const td_u8 *input,
                                   td_u32 input_len,
                                   td_u8 *output,
                                   td_bool encrypt)
{
    return selftest_cbc_hw_raw(CRYPTO_SYMC_ALG_AES, KM_CRYPTO_ALG_AES, "HW AES-CBC", TD_FALSE, key, iv, input,
                               input_len, output, encrypt);
}

static int selftest_aes_cbc_pdf_flow_hw_raw(const td_u8 key[SELFTEST_SM4_BLOCK_LEN],
                                            const td_u8 iv[SELFTEST_SM4_BLOCK_LEN],
                                            const td_u8 *input,
                                            td_u32 input_len,
                                            td_u8 *output,
                                            td_bool encrypt)
{
    return selftest_cbc_hw_raw(CRYPTO_SYMC_ALG_AES, KM_CRYPTO_ALG_AES, "HW AES-CBC-PDF-FLOW", TD_TRUE, key, iv,
                               input, input_len, output, encrypt);
}

static td_u32 selftest_pkcs7_pad(const td_u8 *plain, td_u32 plain_len, td_u8 *out, td_u32 out_size)
{
    td_u32 pad_len = SELFTEST_SM4_BLOCK_LEN - (plain_len % SELFTEST_SM4_BLOCK_LEN);
    td_u32 total_len = plain_len + pad_len;

    if (total_len > out_size) {
        return 0;
    }

    memcpy(out, plain, plain_len);
    memset(out + plain_len, (int)pad_len, pad_len);
    return total_len;
}

static int selftest_pkcs7_unpad(td_u8 *buf, td_u32 len, td_u32 *plain_len)
{
    td_u32 i = 0;
    td_u8 pad = 0;

    if (buf == NULL || plain_len == NULL || len == 0 || len % SELFTEST_SM4_BLOCK_LEN != 0) {
        return TD_FAILURE;
    }

    pad = buf[len - 1];
    if (pad == 0 || pad > SELFTEST_SM4_BLOCK_LEN || pad > len) {
        return TD_FAILURE;
    }

    for (i = 0; i < pad; ++i) {
        if (buf[len - 1 - i] != pad) {
            return TD_FAILURE;
        }
    }

    *plain_len = len - pad;
    return TD_SUCCESS;
}

static int selftest_sm4_cbc(void)
{
    td_s32 ret = TD_SUCCESS;
    const td_u8 key[SELFTEST_SM4_BLOCK_LEN] = {
        0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
        0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10,
    };
    const td_u8 iv[SELFTEST_SM4_BLOCK_LEN] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    };
    const td_u8 raw_plain[SELFTEST_SM4_BLOCK_LEN] = "0123456789abcdef";
    const td_u8 provider_plain[] = "Hello, Majesty! This is GM Crypto test.";
    td_u8 openssl_raw[64] = {0};
    td_u8 hw_raw[64] = {0};
    td_u8 hw_decrypt[128] = {0};
    td_u8 padded_plain[128] = {0};
    td_u8 hw_provider_cipher[128] = {0};
    td_u32 openssl_raw_len = 0;
    td_u32 padded_len = 0;
    td_u32 unpadded_len = 0;
    char openssl_base64[256] = {0};
    char hw_base64[256] = {0};
    int hw_base64_len = 0;

    ret = selftest_openssl_sm4_cbc_raw(key, iv, raw_plain, sizeof(raw_plain), openssl_raw, &openssl_raw_len);
    if (ret != TD_SUCCESS || openssl_raw_len != sizeof(raw_plain)) {
        printf("[FAIL] OpenSSL raw SM4-CBC baseline\n");
        return TD_FAILURE;
    }

    ret = selftest_sm4_cbc_hw_raw(key, iv, raw_plain, sizeof(raw_plain), hw_raw, TD_TRUE);
    if (ret != TD_SUCCESS) {
        printf("[FAIL] HW raw SM4-CBC encrypt ret=0x%08X\n", (unsigned int)ret);
        return ret;
    }
    ret = selftest_mem_equal("SM4-CBC raw ciphertext", hw_raw, openssl_raw, openssl_raw_len);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    ret = selftest_sm4_cbc_hw_raw(key, iv, hw_raw, sizeof(raw_plain), hw_decrypt, TD_FALSE);
    if (ret == TD_SUCCESS) {
        ret = selftest_mem_equal("SM4-CBC raw decrypt", hw_decrypt, raw_plain, sizeof(raw_plain));
    }
    if (ret != TD_SUCCESS) {
        return ret;
    }

    padded_len = selftest_pkcs7_pad(provider_plain, sizeof(provider_plain) - 1, padded_plain, sizeof(padded_plain));
    if (padded_len == 0) {
        return TD_FAILURE;
    }

    ret = selftest_openssl_sm4_cbc_pkcs7_base64(key, iv, provider_plain, sizeof(provider_plain) - 1, openssl_base64,
                                                sizeof(openssl_base64));
    if (ret != TD_SUCCESS) {
        printf("[FAIL] OpenSSL Provider semantic baseline\n");
        return ret;
    }

    ret = selftest_sm4_cbc_hw_raw(key, iv, padded_plain, padded_len, hw_provider_cipher, TD_TRUE);
    if (ret != TD_SUCCESS) {
        printf("[FAIL] HW Provider semantic encrypt ret=0x%08X\n", (unsigned int)ret);
        return ret;
    }

    hw_base64_len = EVP_EncodeBlock((unsigned char *)hw_base64, hw_provider_cipher, padded_len);
    if (hw_base64_len <= 0) {
        return TD_FAILURE;
    }
    hw_base64[hw_base64_len] = '\0';
    if (strcmp(hw_base64, openssl_base64) != 0) {
        printf("[FAIL] SM4-CBC Provider Base64 semantic\nopenssl=%s\nhardware=%s\n", openssl_base64, hw_base64);
        return TD_FAILURE;
    }
    printf("[PASS] SM4-CBC Provider Base64 semantic\n");

    memset(hw_decrypt, 0, sizeof(hw_decrypt));
    ret = selftest_sm4_cbc_hw_raw(key, iv, hw_provider_cipher, padded_len, hw_decrypt, TD_FALSE);
    if (ret != TD_SUCCESS) {
        return ret;
    }
    ret = selftest_pkcs7_unpad(hw_decrypt, padded_len, &unpadded_len);
    if (ret == TD_SUCCESS && unpadded_len == sizeof(provider_plain) - 1 &&
        memcmp(hw_decrypt, provider_plain, unpadded_len) == 0) {
        printf("[PASS] SM4-CBC Provider decrypt semantic\n");
        return TD_SUCCESS;
    }

    printf("[FAIL] SM4-CBC Provider decrypt semantic\n");
    return TD_FAILURE;
}

static int selftest_sm4_cbc_sample_flow(void)
{
    td_s32 ret = TD_SUCCESS;
    const td_u8 key[SELFTEST_SM4_BLOCK_LEN] = {
        0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
        0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10,
    };
    const td_u8 iv[SELFTEST_SM4_BLOCK_LEN] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    };
    const td_u8 raw_plain[SELFTEST_SM4_BLOCK_LEN * 2] = "0123456789abcdef0123456789abcdef";
    td_u8 openssl_raw[64] = {0};
    td_u8 hw_raw[64] = {0};
    td_u32 openssl_raw_len = 0;

    /*
     * test: 该用例严格对齐官方 sample_sm4.c 的 ClearKey 生命周期，用来隔离
     * 当前 wrapper “先写 key、后 create/attach SYMC”的生命周期差异。
     */
    ret = selftest_openssl_sm4_cbc_raw(key, iv, raw_plain, sizeof(raw_plain), openssl_raw, &openssl_raw_len);
    if (ret != TD_SUCCESS || openssl_raw_len != sizeof(raw_plain)) {
        printf("[FAIL] OpenSSL raw SM4-CBC baseline(sample-flow)\n");
        return TD_FAILURE;
    }

    ret = selftest_sm4_cbc_hw_raw_sample_flow(key, iv, raw_plain, sizeof(raw_plain), hw_raw);
    if (ret != TD_SUCCESS) {
        printf("[FAIL] HW raw SM4-CBC sample-flow encrypt ret=0x%08X\n", (unsigned int)ret);
        return ret;
    }

    return selftest_mem_equal("SM4-CBC sample-flow raw ciphertext", hw_raw, openssl_raw, openssl_raw_len);
}

static int selftest_ctx_sm4_cbc(void)
{
    td_s32 ret = TD_SUCCESS;
    CipherContextNeedParam_S need_param = {0};
    CipherContext_S *ctx = NULL;
    const td_u8 key[SELFTEST_SM4_BLOCK_LEN] = {
        0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
        0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10,
    };
    const td_u8 iv[SELFTEST_SM4_BLOCK_LEN] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    };
    const td_u8 plain[SELFTEST_SM4_BLOCK_LEN] = "0123456789abcdef";
    td_u8 openssl_raw[64] = {0};
    td_u8 ctx_cipher[64] = {0};
    td_u8 ctx_plain[64] = {0};
    td_u32 openssl_raw_len = 0;
    td_u32 ctx_cipher_len = 0;
    td_u32 ctx_plain_len = 0;

    ret = selftest_openssl_sm4_cbc_raw(key, iv, plain, sizeof(plain), openssl_raw, &openssl_raw_len);
    if (ret != TD_SUCCESS || openssl_raw_len != sizeof(plain)) {
        printf("[FAIL] OpenSSL raw SM4-CBC baseline(ctx)\n");
        return TD_FAILURE;
    }

    need_param.bEnableSymc = TD_TRUE;
    ctx = cipherContext_alloc(need_param);
    if (ctx == NULL) {
        return TD_FAILURE;
    }

    ret = ctx->cipherContext_init(ctx);
    if (ret != TD_SUCCESS) {
        goto cleanup;
    }

    /*
     * test: 该用例验证业务侧唯一入口 cipher_context 的 SM4-CBC raw block 语义，
     * Provider 层的 PKCS#7 与 Base64 只能建立在该门面稳定可用的基础上。
     */
    ret = ctx->cipherContext_sm4_cbc_crypt(ctx, key, sizeof(key), iv, sizeof(iv), plain, sizeof(plain),
                                           ctx_cipher, sizeof(ctx_cipher), &ctx_cipher_len, TD_TRUE);
    if (ret != TD_SUCCESS) {
        printf("[FAIL] cipherContext SM4-CBC encrypt ret=0x%08X\n", (unsigned int)ret);
        goto cleanup;
    }
    ret = selftest_mem_equal("cipherContext SM4-CBC ciphertext", ctx_cipher, openssl_raw, openssl_raw_len);
    if (ret != TD_SUCCESS) {
        goto cleanup;
    }

    ret = ctx->cipherContext_sm4_cbc_crypt(ctx, key, sizeof(key), iv, sizeof(iv), ctx_cipher, ctx_cipher_len,
                                           ctx_plain, sizeof(ctx_plain), &ctx_plain_len, TD_FALSE);
    if (ret == TD_SUCCESS && ctx_plain_len == sizeof(plain)) {
        ret = selftest_mem_equal("cipherContext SM4-CBC decrypt", ctx_plain, plain, sizeof(plain));
    }

cleanup:
    (td_void)ctx->cipherContext_uninit(ctx);
    cipherContext_release(ctx);
    return ret;
}

static int selftest_aes_cbc(void)
{
    td_s32 ret = TD_SUCCESS;
    const td_u8 key[SELFTEST_SM4_BLOCK_LEN] = {
        0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
        0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c,
    };
    const td_u8 iv[SELFTEST_SM4_BLOCK_LEN] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    };
    const td_u8 plain[SELFTEST_SM4_BLOCK_LEN] = "0123456789abcdef";
    td_u8 openssl_raw[64] = {0};
    td_u8 hw_raw[64] = {0};
    td_u8 hw_decrypt[64] = {0};
    td_u32 openssl_raw_len = 0;

    /*
     * test: 该用例用于诊断 SYMC/KM/MMZ 通用路径。AES-CBC 通过而 SM4-CBC 失败时，
     * 优先检查芯片/SDK 对 SM4 算法的使能、通道能力和安全策略，而不是内存或 keyslot 基础流程。
     */
    ret = selftest_openssl_aes_128_cbc_raw(key, iv, plain, sizeof(plain), openssl_raw, &openssl_raw_len);
    if (ret != TD_SUCCESS || openssl_raw_len != sizeof(plain)) {
        printf("[FAIL] OpenSSL raw AES-CBC baseline\n");
        return TD_FAILURE;
    }

    ret = selftest_aes_cbc_hw_raw(key, iv, plain, sizeof(plain), hw_raw, TD_TRUE);
    if (ret != TD_SUCCESS) {
        printf("[FAIL] HW raw AES-CBC encrypt ret=0x%08X\n", (unsigned int)ret);
        return ret;
    }
    ret = selftest_mem_equal("AES-CBC raw ciphertext", hw_raw, openssl_raw, openssl_raw_len);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    ret = selftest_aes_cbc_hw_raw(key, iv, hw_raw, sizeof(plain), hw_decrypt, TD_FALSE);
    if (ret == TD_SUCCESS) {
        ret = selftest_mem_equal("AES-CBC raw decrypt", hw_decrypt, plain, sizeof(plain));
    }

    return ret;
}

static int selftest_aes_cbc_pdf_flow(void)
{
    td_s32 ret = TD_SUCCESS;
    const td_u8 key[SELFTEST_SM4_BLOCK_LEN] = {
        0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
        0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c,
    };
    const td_u8 iv[SELFTEST_SM4_BLOCK_LEN] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    };
    const td_u8 plain[SELFTEST_SM4_BLOCK_LEN] = "0123456789abcdef";
    td_u8 openssl_raw[64] = {0};
    td_u8 hw_raw[64] = {0};
    td_u32 openssl_raw_len = 0;

    ret = selftest_openssl_aes_128_cbc_raw(key, iv, plain, sizeof(plain), openssl_raw, &openssl_raw_len);
    if (ret != TD_SUCCESS || openssl_raw_len != sizeof(plain)) {
        printf("[FAIL] OpenSSL raw AES-CBC baseline\n");
        return TD_FAILURE;
    }

    ret = selftest_aes_cbc_pdf_flow_hw_raw(key, iv, plain, sizeof(plain), hw_raw, TD_TRUE);
    if (ret != TD_SUCCESS) {
        printf("[FAIL] HW raw AES-CBC PDF flow encrypt ret=0x%08X\n", (unsigned int)ret);
        return ret;
    }

    return selftest_mem_equal("AES-CBC PDF flow raw ciphertext", hw_raw, openssl_raw, openssl_raw_len);
}

static int selftest_run_all(void)
{
    td_s32 ret = TD_SUCCESS;

    ret = selftest_trng();
    if (ret != TD_SUCCESS) {
        return ret;
    }
    ret = selftest_sm3();
    if (ret != TD_SUCCESS) {
        return ret;
    }
    ret = selftest_pke_sm2();
    if (ret != TD_SUCCESS) {
        return ret;
    }
    ret = selftest_ctx_sm2_sign_verify();
    if (ret != TD_SUCCESS) {
        return ret;
    }
    ret = selftest_ctx_sm2_encrypt_decrypt();
    if (ret != TD_SUCCESS) {
        return ret;
    }
    ret = selftest_sm4_cbc();
    if (ret != TD_SUCCESS) {
        return ret;
    }
    return selftest_ctx_sm4_cbc();
}

static void selftest_print_usage(const char *prog)
{
    printf("Usage: %s [--all|--trng|--sm3|--pke-sm2|--ctx-sm2-sign-verify|--ctx-sm2-encrypt-decrypt|--sm4-cbc|--ctx-sm4-cbc|--sm4-cbc-sample-flow|--aes-cbc|--aes-cbc-pdf-flow|--diag]\n", prog);
}

int main(int argc, char *argv[])
{
    td_s32 ret = TD_SUCCESS;

    if (argc != 2) {
        selftest_print_usage(argv[0]);
        return TD_FAILURE;
    }

    if (strcmp(argv[1], "--all") == 0) {
        ret = selftest_run_all();
    } else if (strcmp(argv[1], "--trng") == 0) {
        ret = selftest_trng();
    } else if (strcmp(argv[1], "--sm3") == 0) {
        ret = selftest_sm3();
    } else if (strcmp(argv[1], "--pke-sm2") == 0) {
        ret = selftest_pke_sm2();
    } else if (strcmp(argv[1], "--ctx-sm2-sign-verify") == 0) {
        ret = selftest_ctx_sm2_sign_verify();
    } else if (strcmp(argv[1], "--ctx-sm2-encrypt-decrypt") == 0) {
        ret = selftest_ctx_sm2_encrypt_decrypt();
    } else if (strcmp(argv[1], "--sm4-cbc") == 0) {
        ret = selftest_sm4_cbc();
    } else if (strcmp(argv[1], "--ctx-sm4-cbc") == 0) {
        ret = selftest_ctx_sm4_cbc();
    } else if (strcmp(argv[1], "--sm4-cbc-sample-flow") == 0) {
        ret = selftest_sm4_cbc_sample_flow();
    } else if (strcmp(argv[1], "--aes-cbc") == 0) {
        ret = selftest_aes_cbc();
    } else if (strcmp(argv[1], "--aes-cbc-pdf-flow") == 0) {
        ret = selftest_aes_cbc_pdf_flow();
    } else if (strcmp(argv[1], "--diag") == 0) {
        ret = selftest_diag();
    } else {
        selftest_print_usage(argv[0]);
        return TD_FAILURE;
    }

    if (ret != TD_SUCCESS) {
        printf("[FAIL] security_subsys_selftest ret=0x%08X\n", (unsigned int)ret);
        return ret;
    }

    printf("[PASS] security_subsys_selftest %s\n", argv[1]);
    return TD_SUCCESS;
}
