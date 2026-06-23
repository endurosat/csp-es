/**
 * @file csp_diag.c
 * @brief Sink registration + emit dispatch.
 *
 * Single global sink pointer pair guarded by atomic load/store. No mutex,
 * no heap, no I/O. Emit is best-effort; if no sink is registered it is a
 * no-op of cost atomic-load + branch.
 */
#include <csp/csp_diag.h>

#include <stdatomic.h>

static _Atomic(csp_diag_sink_fn) g_sink_fn   = NULL;
static _Atomic(void *)           g_sink_data = NULL;

void csp_diag_set_sink(csp_diag_sink_fn fn, void *user_data) {
    /* Store data first so that, by the time fn becomes observable, data is too. */
    atomic_store_explicit(&g_sink_data, user_data, memory_order_relaxed);
    atomic_store_explicit(&g_sink_fn,   fn,        memory_order_release);
}

void csp_diag_emit(const csp_diag_event_t *evt) {
    csp_diag_sink_fn fn = atomic_load_explicit(&g_sink_fn, memory_order_acquire);
    if (fn == NULL || evt == NULL) {
        return;
    }
    void *ud = atomic_load_explicit(&g_sink_data, memory_order_relaxed);
    fn(evt, ud);
}
