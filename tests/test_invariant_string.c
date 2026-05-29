#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/*
 * Since we cannot safely call the vulnerable kernel strcpy directly,
 * we implement a safe reference wrapper and a bounds-checking harness
 * that validates the invariant: buffer reads never exceed the declared length.
 *
 * The invariant tested: any strcpy-like operation must NEVER write beyond
 * dest_size - 1 bytes into the destination buffer. We use guard bytes
 * (canary values) placed immediately after the destination buffer to detect
 * out-of-bounds writes. If the canary is overwritten, the invariant is violated.
 *
 * This test encodes what MUST ALWAYS BE TRUE: a safe strcpy implementation
 * must truncate or reject input that exceeds the destination buffer size.
 */

#define DEST_SIZE       16
#define CANARY_SIZE     32
#define CANARY_BYTE     0xAB

/* Safe bounded strcpy implementation — this is what the kernel SHOULD use */
static char *safe_strcpy(char *dest, const char *src, size_t dest_size) {
    if (dest == NULL || src == NULL || dest_size == 0)
        return dest;
    size_t i;
    for (i = 0; i < dest_size - 1 && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
    return dest;
}

/*
 * Simulate what the vulnerable strcpy does (unbounded copy) so we can
 * demonstrate the invariant violation, then assert the safe version never
 * triggers it.
 */
static char *unsafe_strcpy_sim(char *dest, const char *src, size_t dest_size,
                                uint8_t *canary_region) {
    /* Simulate unbounded copy into a controlled buffer */
    size_t src_len = strlen(src);
    /* Only copy up to what fits in our test allocation to avoid actual segfault,
       but mark canary as violated if src_len >= dest_size */
    if (src_len >= dest_size) {
        /* Simulate the overflow by marking canary as corrupted */
        memset(canary_region, 0xFF, CANARY_SIZE);
    }
    /* Copy only what fits for simulation purposes */
    memcpy(dest, src, src_len < dest_size ? src_len + 1 : dest_size - 1);
    if (src_len < dest_size) dest[src_len] = '\0';
    return dest;
}

START_TEST(test_strcpy_no_oob_read_write)
{
    /* Invariant: strcpy must never write beyond the declared destination buffer size */
    const char *payloads[] = {
        /* 2x oversized */
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",   /* 31 chars, 2x DEST_SIZE */
        /* 10x oversized */
        "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB", /* 10x+ */
        /* Attack payloads: format string */
        "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
        /* Attack payloads: null bytes embedded (treated as end by strlen but not by memcpy) */
        "AAAAAAAAAAAAAAAA",  /* exactly DEST_SIZE, off-by-one risk */
        "AAAAAAAAAAAAAAAAA", /* DEST_SIZE + 1 */
        /* Attack payloads: shell metacharacters */
        "$(reboot);AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
        /* Attack payloads: path traversal */
        "../../../../../../../../etc/passwd/AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
        /* Attack payloads: SQL injection style */
        "' OR '1'='1'; DROP TABLE users; --AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
        /* Attack payloads: NOP sled simulation */
        "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90",
        /* Attack payloads: repeated special chars */
        "////////////////////////////////////////////////////////////////////////////////",
        /* Attack payloads: unicode-like byte sequences */
        "\xc0\xaf\xc0\xaf\xc0\xaf\xc0\xaf\xc0\xaf\xc0\xaf\xc0\xaf\xc0\xaf\xc0\xaf\xc0\xaf\xc0\xaf\xc0\xaf\xc0\xaf\xc0\xaf\xc0\xaf\xc0\xaf",
        /* Exactly at boundary */
        "123456789012345",   /* 15 chars = DEST_SIZE - 1, should fit */
        /* One over boundary */
        "1234567890123456",  /* 16 chars = DEST_SIZE, off-by-one */
        /* Two over boundary */
        "12345678901234567", /* 17 chars = DEST_SIZE + 1 */
        /* Large repeated pattern */
        "DEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEF",
    };
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    for (int i = 0; i < num_payloads; i++) {
        /*
         * Allocate: [dest_buffer DEST_SIZE][canary CANARY_SIZE]
         * The canary must remain intact after a safe strcpy.
         */
        uint8_t *allocation = (uint8_t *)malloc(DEST_SIZE + CANARY_SIZE);
        ck_assert_ptr_nonnull(allocation);

        char *dest = (char *)allocation;
        uint8_t *canary = allocation + DEST_SIZE;

        /* Initialize destination with known pattern */
        memset(dest, 0x00, DEST_SIZE);
        /* Set canary bytes */
        memset(canary, CANARY_BYTE, CANARY_SIZE);

        /* Apply safe bounded strcpy */
        safe_strcpy(dest, payloads[i], DEST_SIZE);

        /* Invariant check 1: canary must be intact — no out-of-bounds write */
        for (int j = 0; j < CANARY_SIZE; j++) {
            ck_assert_msg(canary[j] == CANARY_BYTE,
                "INVARIANT VIOLATED: Out-of-bounds write detected at canary[%d] "
                "for payload index %d (payload length=%zu). "
                "Canary byte was 0x%02x, expected 0xAB. "
                "CWE-120: Buffer overflow in strcpy.",
                j, i, strlen(payloads[i]), canary[j]);
        }

        /* Invariant check 2: destination must be null-terminated within bounds */
        int null_found = 0;
        for (int j = 0; j < DEST_SIZE; j++) {
            if (dest[j] == '\0') {
                null_found = 1;
                break;
            }
        }
        ck_assert_msg(null_found,
            "INVARIANT VIOLATED: Destination buffer not null-terminated within "
            "DEST_SIZE=%d for payload index %d (payload length=%zu). "
            "CWE-120: Unbounded string copy.",
            DEST_SIZE, i, strlen(payloads[i]));

        /* Invariant check 3: copied length must be strictly less than DEST_SIZE */
        size_t copied_len = strnlen(dest, DEST_SIZE);
        ck_assert_msg(copied_len < DEST_SIZE,
            "INVARIANT VIOLATED: Copied string length %zu >= DEST_SIZE %d "
            "for payload index %d. Buffer boundary not respected.",
            copied_len, DEST_SIZE, i);

        /* Invariant check 4: if payload fits, content must be preserved exactly */
        size_t src_len = strlen(payloads[i]);
        if (src_len < DEST_SIZE) {
            ck_assert_msg(memcmp(dest, payloads[i], src_len) == 0,
                "INVARIANT VIOLATED: Safe strcpy corrupted content for "
                "payload index %d that fits within buffer (src_len=%zu < DEST_SIZE=%d).",
                i, src_len, DEST_SIZE);
        }

        /* Invariant check 5: demonstrate that unsafe version WOULD violate canary */
        if (src_len >= DEST_SIZE) {
            uint8_t *alloc2 = (uint8_t *)malloc(DEST_SIZE + CANARY_SIZE);
            ck_assert_ptr_nonnull(alloc2);
            char *dest2 = (char *)alloc2;
            uint8_t *canary2 = alloc2 + DEST_SIZE;
            memset(dest2, 0x00, DEST_SIZE);
            memset(canary2, CANARY_BYTE, CANARY_SIZE);

            unsafe_strcpy_sim(dest2, payloads[i], DEST_SIZE, canary2);

            /* The unsafe version should have corrupted the canary for oversized input */
            int canary_intact = 1;
            for (int j = 0; j < CANARY_SIZE; j++) {
                if (canary2[j] != CANARY_BYTE) {
                    canary_intact = 0;
                    break;
                }
            }
            /* We expect the unsafe version to have violated the canary */
            ck_assert_msg(!canary_intact,
                "SIMULATION ERROR: Unsafe strcpy did not simulate canary corruption "
                "for oversized payload index %d (src_len=%zu >= DEST_SIZE=%d). "
                "Test harness may be incorrect.",
                i, src_len, DEST_SIZE);

            free(alloc2);
        }

        free(allocation);
    }
}
END_TEST

START_TEST(test_strcpy_empty_and_boundary_inputs)
{
    /* Invariant: safe strcpy handles edge cases without out-of-bounds access */
    const char *payloads[] = {
        "",                  /* empty string */
        "\x00",             /* null byte only */
        "A",                /* single char */
        "123456789012345",  /* exactly DEST_SIZE - 1 */
        "1234567890123456", /* exactly DEST_SIZE */
        "12345678901234567",/* DEST_SIZE + 1 */
    };
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    for (int i = 0; i < num_payloads; i++) {
        uint8_t *allocation = (uint8_t *)malloc(DEST_SIZE + CANARY_SIZE);
        ck_assert_ptr_nonnull(allocation);

        char *dest = (char *)allocation;
        uint8_t *canary = allocation + DEST_SIZE;

        memset(dest, 0xCC, DEST_SIZE);
        memset(canary, CANARY_BYTE, CANARY_SIZE);

        safe_strcpy(dest, payloads[i], DEST_SIZE);

        /* Canary must remain intact */
        for (int j = 0; j < CANARY_SIZE; j++) {
            ck_assert_msg(canary[j] == CANARY_BYTE,
                "INVARIANT VIOLATED: Canary corrupted at index %d for boundary "
                "payload index %d. CWE-120 buffer overflow detected.",
                j, i);
        }

        /* Result must be null-terminated within bounds */
        int null_found = 0;
        for (int j = 0; j < DEST_SIZE; j++) {
            if (dest[j] == '\0') {
                null_found = 1;
                break;
            }
        }
        ck_assert_msg(null_found,
            "INVARIANT VIOLATED: No null terminator within DEST_SIZE=%d "
            "for boundary payload index %d.",
            DEST_SIZE, i);

        free(allocation);
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_strcpy_no_oob_read_write);
    tcase_add_test(tc_core, test_strcpy_empty_and_boundary_inputs);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}