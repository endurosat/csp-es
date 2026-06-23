/**
 * @file csp_diag.h
 * @brief Structured diagnostic event emit API (host-side sink registration).
 *
 * Allows csp_es drivers and consumers to emit structured events to a
 * single host-process sink. Sink is registered once at startup; emit is
 * a NULL-check + indirect call when sink is set, an atomic-load + NULL
 * check (~10 ns) when not set.
 */
#ifndef CSP_DIAG_H
#define CSP_DIAG_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CSP_DIAG_U32,
    CSP_DIAG_I32,
    CSP_DIAG_U64,
    CSP_DIAG_STR,
    CSP_DIAG_HEX,
    CSP_DIAG_BOOL,
} csp_diag_field_type_t;

typedef struct {
    const char *key;
    csp_diag_field_type_t type;
    union {
        uint32_t u32;
        int32_t  i32;
        uint64_t u64;
        const char *str;
        struct { const void *data; size_t len; } hex;
        int bool_val;
    } v;
} csp_diag_field_t;

typedef struct {
    const char *cat;   /* "drv" | "csp" | "srv" | "fw" */
    const char *evt;
    const char *src;
    const csp_diag_field_t *fields;
    size_t fields_count;
} csp_diag_event_t;

typedef void (*csp_diag_sink_fn)(const csp_diag_event_t *evt, void *user_data);

/** Register the process-wide sink. Call once at startup, NULL to clear. */
void csp_diag_set_sink(csp_diag_sink_fn fn, void *user_data);

/** Emit one event. No-op if no sink registered. Never blocks. */
void csp_diag_emit(const csp_diag_event_t *evt);

/* --- ergonomic macros ------------------------------------------------ */

#define CSP_DIAG_U32(k, val)  { .key=(k), .type=CSP_DIAG_U32,  .v={.u32=(val)} }
#define CSP_DIAG_I32(k, val)  { .key=(k), .type=CSP_DIAG_I32,  .v={.i32=(val)} }
#define CSP_DIAG_U64(k, val)  { .key=(k), .type=CSP_DIAG_U64,  .v={.u64=(val)} }
#define CSP_DIAG_STR(k, val)  { .key=(k), .type=CSP_DIAG_STR,  .v={.str=(val)} }
#define CSP_DIAG_HEX(k, p, n) { .key=(k), .type=CSP_DIAG_HEX,  .v={.hex={.data=(p), .len=(n)}} }
#define CSP_DIAG_BOOL(k, val) { .key=(k), .type=CSP_DIAG_BOOL, .v={.bool_val=((val) ? 1 : 0)} }

#define CSP_DIAG_EMIT(_cat, _evt, _src, ...)                                  \
    do {                                                                       \
        const csp_diag_field_t _f[] = { __VA_ARGS__ };                         \
        const csp_diag_event_t _e = {                                          \
            .cat=(_cat), .evt=(_evt), .src=(_src),                             \
            .fields=_f,                                                        \
            .fields_count = sizeof(_f)/sizeof(_f[0]),                          \
        };                                                                     \
        csp_diag_emit(&_e);                                                    \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif /* CSP_DIAG_H */
