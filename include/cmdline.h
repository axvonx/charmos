/* @title: Command Line */
#pragma once
#include <linker/symbols.h>
#include <stdbool.h>

enum cmdline_status {
    CMDLINE_NOT_FOUND = 0,
    CMDLINE_DEFAULTED,
    CMDLINE_FOUND,
};

typedef void (*cmdline_callback)(const char *value);

struct cmdline_entry {
    const char *name;
    cmdline_callback callback;
    char **value;
    const char *default_val;
    enum cmdline_status status;
    bool required;
};

#define CMDLINE_ENTRY_DECLARE(n, ...)                                          \
    static struct cmdline_entry __cmdline_##n                                  \
        __attribute__((used, section(".kernel_cmdline_entries"))) = {          \
            .name = #n, .status = CMDLINE_NOT_FOUND, __VA_ARGS__}

LINKER_SECTION_DEFINE(cmdline_entries, struct cmdline_entry);

void cmdline_parse(const char *input);
