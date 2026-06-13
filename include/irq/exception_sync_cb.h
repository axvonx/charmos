/* @title: Processor Exception Synchronous Work */
#pragma once
#include <linker/symbols.h>
#include <stddef.h>
#include <stdint.h>

struct exception_sync_cb;
struct irq_context;

enum exception_sync_cb_result {
    EXCEPTION_SYNC_CB_OK = 0,
    EXCEPTION_SYNC_CB_ERR = -1, /* TODO: more */
};

typedef enum exception_sync_cb_result (*exception_sync_cb_fn)(
    struct exception_sync_cb *this, struct irq_context *irqc);

struct exception_sync_cb {
    const char *name;
    uint8_t vector;
    exception_sync_cb_fn fn;
    void *private;
} __linker_aligned;

LINKER_SECTION_DEFINE(struct exception_sync_cb, exception_sync_cbs);
#define EXCEPTION_SYNC_CB_REGISTER(n, v, func, priv)                           \
    LINKER_SECTION_OBJECT(struct exception_sync_cb, exception_sync_cbs)        \
    exception_sync_cb_##n = {                                                  \
        .name = #n, .vector = v, .fn = func, .private = priv};
