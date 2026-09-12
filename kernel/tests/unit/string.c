#include "tests/test_internal.h"

TEST_GROUP_DECLARE(string);

TEST_DECLARE_UNIT(string, strncmp_boundaries) {
    TEST_ASSERT_EQ(strncmp("a", "a", 2), 0);
    TEST_ASSERT_EQ(strncmp("", "", 1), 0);
    TEST_ASSERT_EQ(strncmp("a", "b", 0), 0);
    TEST_ASSERT_EQ(strncmp("abcd", "abce", 3), 0);
    TEST_ASSERT(strncmp("abcd", "abce", 4) < 0);
    TEST_ASSERT(strncmp("abce", "abcd", 4) > 0);
    TEST_ASSERT(strncmp("a", "ab", 2) < 0);
    TEST_ASSERT(strncmp("ab", "a", 2) > 0);
    const char high[] = {(char) 0x80, 0};
    TEST_ASSERT(strncmp(high, "\x7f", 1) > 0);
    TEST_ASSERT(strncmp("\x7f", high, 1) < 0);
    const char a[] = {'a', 'b'};
    const char b[] = {'a', 'b'};
    TEST_ASSERT_EQ(strncmp(a, b, sizeof(a)), 0);
    return TEST_SUCCESS;
}

TEST_DECLARE_UNIT(string, memmem_binary) {
    const unsigned char h[] = {0x80, 0, 0xff, 0x80, 0, 0xff};
    const unsigned char n[] = {0, 0xff};
    TEST_ASSERT_PTR_EQ(memmem(h, sizeof(h), n, sizeof(n)), h + 1);
    TEST_ASSERT_PTR_EQ(memmem(h, sizeof(h), h, sizeof(h)), h);
    TEST_ASSERT_PTR_EQ(memmem(h, sizeof(h), n, 0), h);
    TEST_ASSERT_PTR_EQ(memmem(h, 0, n, 0), h);
    TEST_ASSERT_NULL(memmem(h, 0, n, sizeof(n)));
    TEST_ASSERT_NULL(memmem(h, 1, n, sizeof(n)));
    TEST_ASSERT_NULL(memmem(h, 2, n, sizeof(n)));
    TEST_ASSERT_NULL(memmem(h, sizeof(h), "x", 1));
    const char repeated[] = "aaaaab";
    TEST_ASSERT_PTR_EQ(memmem(repeated, 6, "aaab", 4), repeated + 2);
    return TEST_SUCCESS;
}

TEST_DECLARE_UNIT(string, bounded_substring_search) {
    const char h[] = {'a', 'B', 'c', 'D', 'e'};
    TEST_ASSERT_PTR_EQ(strnstr(h, "BcD", sizeof(h)), h + 1);
    TEST_ASSERT_PTR_EQ(strnstr(h, "De", sizeof(h)), h + 3);
    TEST_ASSERT_NULL(strnstr(h, "BcD", 3));
    TEST_ASSERT_NULL(strnstr(h, "bcd", sizeof(h)));
    TEST_ASSERT_PTR_EQ(strncasestr(h, "bCd", 4), h + 1);
    TEST_ASSERT_NULL(strncasestr(h, "bCd", 3));
    TEST_ASSERT_PTR_EQ(strncasestr(h, "dE", sizeof(h)), h + 3);
    TEST_ASSERT_PTR_EQ(strnstr(h, "", 0), h);
    TEST_ASSERT_PTR_EQ(strncasestr(h, "", 0), h);
    TEST_ASSERT_NULL(strnstr(h, "a", 0));
    TEST_ASSERT_NULL(strncasestr(h, "a", 0));
    TEST_ASSERT_NULL(strnstr(h, "abcdef", sizeof(h)));
    TEST_ASSERT_NULL(strncasestr(h, "abcdef", sizeof(h)));
    const char terminated[] = {'a', 0, 'B', 0};
    TEST_ASSERT_NULL(strnstr(terminated, "B", sizeof(terminated)));
    TEST_ASSERT_NULL(strncasestr(terminated, "b", sizeof(terminated)));
    TEST_ASSERT_NULL(strnstr("", "a", 1));
    TEST_ASSERT_NULL(strncasestr("", "a", 1));
    const char high[] = {(char) 0xff, 'A', 0};
    const char needle[] = {(char) 0xff, 'a', 0};
    TEST_ASSERT_PTR_EQ(strncasestr(high, needle, 2), high);
    const char repeated[] = "aAaAab";
    TEST_ASSERT_PTR_EQ(strncasestr(repeated, "AAab", 6), repeated + 2);
    return TEST_SUCCESS;
}

TEST_DECLARE_UNIT(string, end_pointer_copies) {
    char dst[8];
    memset(dst, '!', sizeof(dst));
    TEST_ASSERT_PTR_EQ(stpcpy(dst, "abc"), dst + 3);
    TEST_ASSERT_EQ(memcmp(dst, "abc\0!", 5), 0);
    TEST_ASSERT_PTR_EQ(stpcpy(dst, ""), dst);
    TEST_ASSERT_EQ(dst[0], 0);

    memset(dst, '!', sizeof(dst));
    TEST_ASSERT_PTR_EQ(stpncpy(dst, "ab", 5), dst + 2);
    TEST_ASSERT_EQ(memcmp(dst, "ab\0\0\0!", 6), 0);
    TEST_ASSERT_PTR_EQ(stpncpy(dst, "", 3), dst);
    TEST_ASSERT_EQ(memcmp(dst, "\0\0\0", 3), 0);

    memset(dst, '!', sizeof(dst));
    const char src[] = {'a', 'b', 'c'};
    TEST_ASSERT_PTR_EQ(stpncpy(dst, src, sizeof(src)), dst + 3);
    TEST_ASSERT_EQ(memcmp(dst, "abc!", 4), 0);
    TEST_ASSERT_PTR_EQ(stpncpy(dst, "abcdef", 2), dst + 2);
    TEST_ASSERT_EQ(dst[2], 'c');
    TEST_ASSERT_PTR_EQ(stpncpy(dst, "ignored", 0), dst);
    TEST_ASSERT_EQ(memcmp(dst, "abc!", 4), 0);

    const unsigned char binary[] = {0x80, 0, 0xff};
    TEST_ASSERT_PTR_EQ(mempcpy(dst, binary, sizeof(binary)), dst + 3);
    TEST_ASSERT_EQ(memcmp(dst, binary, sizeof(binary)), 0);
    TEST_ASSERT_EQ(dst[3], '!');
    TEST_ASSERT_PTR_EQ(mempcpy(dst, binary, 0), dst);
    TEST_ASSERT_EQ(memcmp(dst, binary, sizeof(binary)), 0);
    return TEST_SUCCESS;
}

TEST_DECLARE_UNIT(string, ascii_classification) {
    for (int c = -1; c <= 256; c++) {
        TEST_ASSERT_EQ(!!isblank(c), c == ' ' || c == '\t');
        TEST_ASSERT_EQ(!!iscntrl(c), (c >= 0 && c <= 31) || c == 127);
        TEST_ASSERT_EQ(!!isgraph(c), c >= 33 && c <= 126);
        TEST_ASSERT_EQ(!!ispunct(c),
                       (c >= 33 && c <= 47) || (c >= 58 && c <= 64) ||
                           (c >= 91 && c <= 96) || (c >= 123 && c <= 126));
        TEST_ASSERT_EQ(!!isxdigit(c), (c >= '0' && c <= '9') ||
                                          (c >= 'A' && c <= 'F') ||
                                          (c >= 'a' && c <= 'f'));
    }
    return TEST_SUCCESS;
}

TEST_DECLARE_UNIT(string, strcasestr_patterns) {
    const char h[] = "aAbAaBaAbAaX";
    TEST_ASSERT_PTR_EQ(strcasestr(h, ""), h);
    TEST_ASSERT_PTR_EQ(strcasestr(h, "AABAAX"), h + 6);
    TEST_ASSERT_PTR_EQ(strcasestr(h, "aab"), h);
    TEST_ASSERT_NULL(strcasestr(h, "aabac"));
    TEST_ASSERT_NULL(strcasestr("short", "longer_needle"));
    TEST_ASSERT_NULL(strcasestr("", "a"));
    const char high[] = {(char) 0xff, 'A', 0};
    const char needle[] = {(char) 0xff, 'a', 0};
    TEST_ASSERT_PTR_EQ(strcasestr(high, needle), high);
    return TEST_SUCCESS;
}

TEST_DECLARE_UNIT(string, strstr_patterns) {
    /* Giving an empty needle returns start of haystack */
    const char *h1 = "abcdef";
    TEST_ASSERT_PTR_EQ(strstr(h1, ""), h1);

    TEST_ASSERT_NULL(strstr("short", "longer_needle"));

    TEST_ASSERT_NONNULL(strstr("hello", "hello"));

    /* KMP prefix backtracking */
    const char *h2 = "aabaabaabaax";
    const char *n2 = "aabaax";
    char *match2 = strstr(h2, n2);
    TEST_ASSERT_NONNULL(match2);
    TEST_ASSERT_PTR_EQ(match2, h2 + 6);

    /* trailing */
    const char *h3 = "aaaaab";
    const char *n3 = "aaab";
    char *match3 = strstr(h3, n3);
    TEST_ASSERT_NONNULL(match3);
    TEST_ASSERT_PTR_EQ(match3, h3 + 2);

    TEST_ASSERT_NULL(strstr("ababababa", "ababc"));

    return TEST_SUCCESS;
}
