/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#ifndef CRYPTO_PLATFORM_H
#define CRYPTO_PLATFORM_H

/* Register Base Addr. */
#define SEC_SUBSYS_BASE_ADDR    0x10100000

#define SPACC_REG_BASE_ADDR     (SEC_SUBSYS_BASE_ADDR + 0xF0000)
#define SPACC_REG_SIZE          (0x10000)

#define TRNG_REG_BASE_ADDR      (SEC_SUBSYS_BASE_ADDR + 0xEE000)
#define TRNG_REG_SIZE           (0x1000)

#define PKE_REG_BASE_ADDR       (SEC_SUBSYS_BASE_ADDR + 0xEC000)
#define PKE_REG_SIZE            (0x2000)

#define CA_MISC_REG_BASE_ADDR   (SEC_SUBSYS_BASE_ADDR + 0xE8000)
#define CA_MISC_REG_SIZE        (0x1000)

#define KM_REG_BASE_ADDR        (SEC_SUBSYS_BASE_ADDR + 0xEA000)
#define KM_REG_SIZE             (0x2000)

#define OTPC_BASE_ADDR          (SEC_SUBSYS_BASE_ADDR + 0xE0000)
#define OTPC_ADDR_SIZE          (0x2000)

#define CRYPTO_DRV_AAD_SIZE             (4 * 1024)
#define CRYPTO_HASH_DRV_BUFFER_SIZE     (128 * 1024)

/* Define the time out us */
#define PKE_TIME_OUT_US               10000000
#define PKE_MAX_TIMES                 3000

/* PKE related defines */
/* define PKE rom lib related register micro. */
#ifdef DEBUG_IN_RAM
#define PKE_ROM_LIB_START_ADDR              ((uintptr_t)g_instr_rom)  /* the instr start address */
#else
#define PKE_ROM_LIB_START_ADDR              (0xc00000)  /* the instr start address */
#endif
#define PKE_ROM_LIB_INVALID_ADDR            0xFFFFFFFF
#define ROM_LIB_INSTR_NUM                   646         /* define the instr number, that put in the ROM. */

/* whether to enable the DFA function */
#define CRYPTO_DRV_DFA_ENABLE

#define CRYPTO_STATIC static

#endif