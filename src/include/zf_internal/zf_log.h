/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: (c) Advanced Micro Devices, Inc. */
#ifndef __ZF_LOG_H__
#define __ZF_LOG_H__

#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>

/* Ensure platform/compiler helpers (ZF_VISIBLE, ZF_UNLIKELY, ZF_COLD,
 * ZF_LIBENTRY, etc) are available.
 *
 * In normal builds this header is indirectly included after `<zf/zf.h>`,
 * which already includes `<zf/zf_platform.h>`. However, for robustness we
 * provide minimal fallbacks here for any translation unit that includes this
 * header without going via `<zf/zf.h>`. */
#ifndef ZF_UNLIKELY
# define ZF_UNLIKELY(t)  __builtin_expect((t), 0)
#endif
#ifndef ZF_LIKELY
# define ZF_LIKELY(t)    __builtin_expect((t), 1)
#endif
#ifndef ZF_COLD
# define ZF_COLD __attribute__((cold))
#endif
#ifndef ZF_VISIBLE
# define ZF_VISIBLE __attribute__((visibility("default")))
#endif
#ifndef ZF_LIBENTRY
# ifdef __cplusplus
#  define ZF_LIBENTRY extern "C" __attribute__((visibility("default")))
# else
#  define ZF_LIBENTRY extern
# endif
#endif

static const uint64_t ZF_LL_ERR = 0x1;
static const uint64_t ZF_LL_WARN = 0x2;
static const uint64_t ZF_LL_INFO = 0x4;
static const uint64_t ZF_LL_TRACE = 0x8;
#define ZF_LL_NUM_LEVELS 4

enum zf_log_fmt_flags {
  ZF_LF_STACK_NAME = 0x1,
  ZF_LF_FRC  = 0x2,
  ZF_LF_TCP_TIME  = 0x4,
  ZF_LF_PROCESS  = 0x8,
};


enum zf_log_comp {
  ZF_LC_STACK,
  ZF_LC_TCP_RX,
  ZF_LC_TCP_TX,
  ZF_LC_TCP_CONN,
  ZF_LC_UDP_RX,
  ZF_LC_UDP_TX,
  ZF_LC_UDP_CONN,
  ZF_LC_MUXER,
  ZF_LC_POOL,
  ZF_LC_EVENT,
  ZF_LC_TIMER,
  ZF_LC_FILTER,
  ZF_LC_CPLANE,
  ZF_LC_RX,
  ZF_LC_SOCKET_SHIM,
  ZF_LC_EMU,
  ZF_LC_NUM_COMPONENTS,
};

/* Attribute default: log component-level for ZF_LL_ERR on all components */
#define ZF_LCL_BIT(level, comp) (level << (comp * ZF_LL_NUM_LEVELS))
#define ZF_LCL_ALL_ERR ( ZF_LCL_BIT(ZF_LL_ERR, ZF_LC_STACK) |       \
                         ZF_LCL_BIT(ZF_LL_ERR, ZF_LC_TCP_RX) |      \
                         ZF_LCL_BIT(ZF_LL_ERR, ZF_LC_TCP_TX) |      \
                         ZF_LCL_BIT(ZF_LL_ERR, ZF_LC_TCP_CONN) |    \
                         ZF_LCL_BIT(ZF_LL_ERR, ZF_LC_UDP_RX) |      \
                         ZF_LCL_BIT(ZF_LL_ERR, ZF_LC_UDP_TX) |      \
                         ZF_LCL_BIT(ZF_LL_ERR, ZF_LC_UDP_CONN) |    \
                         ZF_LCL_BIT(ZF_LL_ERR, ZF_LC_MUXER) |       \
                         ZF_LCL_BIT(ZF_LL_ERR, ZF_LC_POOL) |        \
                         ZF_LCL_BIT(ZF_LL_ERR, ZF_LC_EVENT) |    \
                         ZF_LCL_BIT(ZF_LL_ERR, ZF_LC_TIMER) |       \
                         ZF_LCL_BIT(ZF_LL_ERR, ZF_LC_FILTER) |      \
                         ZF_LCL_BIT(ZF_LL_ERR, ZF_LC_CPLANE) |      \
                         ZF_LCL_BIT(ZF_LL_ERR, ZF_LC_RX) |          \
                         ZF_LCL_BIT(ZF_LL_ERR, ZF_LC_SOCKET_SHIM) | \
                         ZF_LCL_BIT(ZF_LL_ERR, ZF_LC_EMU) )

_Static_assert(ZF_LC_NUM_COMPONENTS * ZF_LL_NUM_LEVELS <= 64,
               "64-bit mask is not large enough for all components and levels");

#define ZF_LOG_FILE_NAME_SIZE 256

extern uint64_t zf_log_level;
extern int zf_log_format;
extern char zf_log_file_name[ZF_LOG_FILE_NAME_SIZE];

/* Debug-only “log API calls” switch (set via ZF_ATTR / zf_attr_set_int()).
 * When disabled, call tracing should be essentially free (one predicted-false
 * branch). */
extern int zf_log_calls;

struct zf_stack;

#ifdef __cplusplus
static constexpr zf_stack* NO_STACK = NULL;

class zf_logger
{
  private:
    const uint64_t log_comp_level;
  public:
    zf_logger(int comp, uint64_t level) :
        log_comp_level(ZF_LCL_BIT(level, comp))
        {}

    /* Fast-path check used by call-tracing macros to avoid evaluating
     * expensive arguments when logging is disabled. */
    ZF_VISIBLE bool enabled() const { return (log_comp_level & zf_log_level) != 0; }

    template <typename T> ZF_VISIBLE void operator()(T obj, const char* fmt,
                                                     ...) const;
    ZF_VISIBLE void operator()(const char* fmt, va_list v) const;
};

#define ZF_LOGGER_ENABLED(logger_) ((logger_).enabled())
#else
/* This header is primarily consumed by C++ (zf_logger uses templates).
 * Provide minimal C compatibility so that including it from C does not break
 * preprocessing / parsing (e.g. for tooling). */
#define NO_STACK NULL

typedef struct zf_logger {
  uint64_t log_comp_level;
} zf_logger;

static inline int zf_logger_enabled(const zf_logger* l)
{
  return (l->log_comp_level & zf_log_level) != 0;
}

#define ZF_LOGGER_ENABLED(logger_) zf_logger_enabled(&(logger_))
#endif

/* Emit message to log unconditionally. */
ZF_LIBENTRY ZF_COLD void zf_log(struct zf_stack*, const char* fmt, ...)
  __attribute__((format(printf,2,3)));

ZF_COLD void zf_dump(const char* fmt, ...);
ZF_COLD int zf_log_replace_stderr(const char* file);
ZF_COLD int zf_log_redirect(const char* file);
ZF_COLD void zf_log_stderr(void);

#ifndef NDEBUG
void zf_backtrace();
#else
#define zf_backtrace() do{}while(0)
#endif


/* API/shim call tracing helpers.
 *
 * These are intended for debug builds only (compiled out under NDEBUG), and
 * they avoid evaluating arguments when the relevant log level is disabled.
 *
 * Typical usage:
 *   ZF_LOG_CALL(zf_log_stack_trace, st, "arg=%d ptr=%p", x, p);
 *
 * This will emit:
 *   zf_stack_alloc(arg=..., ptr=...)
 */
#ifndef NDEBUG
#define ZF_LOG_CALL(logger, obj, fmt, ...)                                  \
  do {                                                                      \
    if( ZF_UNLIKELY(zf_log_calls) && ZF_UNLIKELY(ZF_LOGGER_ENABLED(logger)) ) \
      (logger)((obj), "CALL %s(" fmt ")\n", __func__, ##__VA_ARGS__);       \
  } while( 0 )
#else
#define ZF_LOG_CALL(logger, obj, fmt, ...) do{}while(0)
#endif

#endif
