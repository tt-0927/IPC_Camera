/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#ifndef CRYPTO_DRV_COMMON_H
#define CRYPTO_DRV_COMMON_H

#include "crypto_type.h"
#include "crypto_osal_lib.h"
#include "crypto_log.h"
#include "crypto_common_macro.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

typedef struct {
    const td_char *name;
    td_u32 offset;
} reg_item_t;

typedef td_u32 (*reg_read_func)(td_u32 offset);

void crypto_reg_item_dump(const td_char *name, const reg_item_t *reg_item_table, td_u32 item_size,
    reg_read_func read_func);

typedef enum {
    TIMER_ID_0  = 0,
    TIMER_ID_1,
    TIMER_ID_2,
    TIMER_ID_3,
    TIMER_ID_4,
    TIMER_ID_5,
    TIMER_ID_6,
    TIMER_ID_7,
    TIMER_ID_8,
    TIMER_ID_9,
} crypto_timer_id;
#if defined(CONFIG_CRYPTO_PERF_STATISTICS)
td_void crypto_timer_start(td_u32 timer_id, const td_char *name);

td_u64 crypto_timer_end(td_u32 timer_id, const td_char *item_name);

td_void crypto_timer_print(td_u32 timer_id);

td_void crypto_timer_print_all(td_void);
#else
#define crypto_timer_start(...)
#define crypto_timer_end(...)
#define crypto_timer_print(...)
#define crypto_timer_print_all(...)
#endif

typedef struct {
    td_u32 index;
    td_u32 value;
} crypto_table_item;

td_u32 crypto_get_value_by_index(const crypto_table_item *table, td_u32 table_size,
    td_u32 index, td_u32 *value);
td_u32 crypto_get_index_by_value(const crypto_table_item *table, td_u32 table_size,
    td_u32 value, td_u32 *index);

unsigned int crypto_align(unsigned int original_length, unsigned int aligned_length);

void crypto_dump_data(const char *name, const td_u8 *data, td_u32 data_len);

void crypto_dump_phys_addr(const char *name, const td_u64 phys_addr, td_u32 data_len);

td_s32 crypto_virt_xor_phys_copy_to_phys(td_u64 dst_phys_addr, const td_u8 *a_virt_addr,
    td_u64 b_phys_addr, td_u32 length);

td_s32 crypto_copy_from_phys_addr(td_u8 *dst, td_u32 dst_len, td_u64 src_phys_addr, td_u32 src_len);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif