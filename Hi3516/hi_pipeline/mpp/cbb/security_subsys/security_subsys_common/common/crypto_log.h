/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#ifndef CRYPTO_LOG_H
#define CRYPTO_LOG_H

#if !defined(CRYPTO_LOG_DEF)

#ifndef crypto_print
#error "crypto_print is not set!"
#endif

#ifndef CRYPTO_LOG_LEVEL
#define CRYPTO_LOG_LEVEL 0
#endif

#if defined(CONFIG_CRYPTO_LOG_INFO_DISABLE)
#define crypto_log_fmt(LOG_LEVEL_LABEL, fmt, ...)   crypto_print(fmt, ##__VA_ARGS__)
#else
#define crypto_log_fmt(LOG_LEVEL_LABEL, fmt, ...)   \
    crypto_print("[%s:%d]" LOG_LEVEL_LABEL ": " fmt, __func__, __LINE__, ##__VA_ARGS__)
#endif

#if defined(CRYPTO_LOG_LEVEL) && (CRYPTO_LOG_LEVEL == 0)
#define crypto_log_err(fmt, ...) crypto_log_fmt("ERROR", fmt, ##__VA_ARGS__)
#define crypto_log_warn(fmt, ...)
#define crypto_log_notice(fmt, ...)
#define crypto_log_dbg(fmt, ...)
#define crypto_log_trace(fmt, ...)
#elif defined(CRYPTO_LOG_LEVEL) && (CRYPTO_LOG_LEVEL == 1)
#define crypto_log_err(fmt, ...) crypto_log_fmt("ERROR", fmt, ##__VA_ARGS__)
#define crypto_log_warn(fmt, ...) crypto_log_fmt("WARN:", fmt, ##__VA_ARGS__)
#define crypto_log_notice(fmt, ...)
#define crypto_log_dbg(fmt, ...)
#define crypto_log_trace(fmt, ...)
#elif defined(CRYPTO_LOG_LEVEL) && (CRYPTO_LOG_LEVEL == 2)
#define crypto_log_err(fmt, ...) crypto_log_fmt("ERROR", fmt, ##__VA_ARGS__)
#define crypto_log_warn(fmt, ...) crypto_log_fmt("WARN:", fmt, ##__VA_ARGS__)
#define crypto_log_notice(fmt, ...) crypto_log_fmt("NOTICE", fmt, ##__VA_ARGS__)
#define crypto_log_dbg(fmt, ...)
#define crypto_log_trace(fmt, ...)
#elif defined(CRYPTO_LOG_LEVEL) && (CRYPTO_LOG_LEVEL == 3)
#define crypto_log_err(fmt, ...) crypto_log_fmt("ERROR", fmt, ##__VA_ARGS__)
#define crypto_log_warn(fmt, ...) crypto_log_fmt("WARN:", fmt, ##__VA_ARGS__)
#define crypto_log_notice(fmt, ...) crypto_log_fmt("NOTICE", fmt, ##__VA_ARGS__)
#define crypto_log_dbg(fmt, ...) crypto_log_fmt("DBG", fmt, ##__VA_ARGS__)
#define crypto_log_trace(fmt, ...)
#elif defined(CRYPTO_LOG_LEVEL) && (CRYPTO_LOG_LEVEL == 4)
#define crypto_log_err(fmt, ...) crypto_log_fmt("ERROR", fmt, ##__VA_ARGS__)
#define crypto_log_warn(fmt, ...) crypto_log_fmt("WARN:", fmt, ##__VA_ARGS__)
#define crypto_log_notice(fmt, ...) crypto_log_fmt("NOTICE", fmt, ##__VA_ARGS__)
#define crypto_log_dbg(fmt, ...) crypto_log_fmt("DBG", fmt, ##__VA_ARGS__)
#define crypto_log_trace(fmt, ...) crypto_log_fmt("TRACE", fmt, ##__VA_ARGS__)
#endif

#define crypto_param_trace(fmt, ...)    crypto_print("[%s] " fmt, __func__, ##__VA_ARGS__)
#define crypto_param_dump_trace(name, data, data_len)   do {    \
    crypto_print("[%s] ", __func__);                            \
    crypto_dump_data(name, data, data_len);                     \
} while (0)

#endif
#endif