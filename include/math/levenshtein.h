/* @title: Levenshtein */
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <types/types.h>

int64_t levenshtein(const char *s1, const char *s2, size_t len1, size_t len2);
fx32_32_t fuzzy_substr_conf(const char *pattern, const char *text,
                            bool ignore_case);
