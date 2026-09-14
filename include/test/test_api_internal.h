/* @title: Test API internal functions */
#pragma once

/* 1D piecewise-log */
#define TEST_INTENSITY_LOG(min, def, max, unit_str)                            \
    .flags = TEST_FLAG_HONORS_INTENSITY, .intensity_desc = {                   \
                                             .curve = SCALE_PIECEWISE_LOG,     \
                                             .min_val = (min),                 \
                                             .def_val = (def),                 \
                                             .max_val = (max),                 \
                                             .unit = (unit_str),               \
    }

/* Linear scaling */
#define TEST_INTENSITY_LINEAR(min, def, max, unit_str)                         \
    .flags = TEST_FLAG_HONORS_INTENSITY, .intensity_desc = {                   \
                                             .curve = SCALE_PIECEWISE_LINEAR,  \
                                             .min_val = (min),                 \
                                             .def_val = (def),                 \
                                             .max_val = (max),                 \
                                             .unit = (unit_str),               \
    }

/* Core-scaled thread counts (threads = base * core_count) */
#define TEST_INTENSITY_CORES(min_per_core, def_per_core, max_per_core,         \
                             unit_str)                                         \
    .flags = TEST_FLAG_HONORS_INTENSITY, .intensity_desc = {                   \
                                             .curve = SCALE_CORE_MULTIPLIER,   \
                                             .min_val = (min_per_core),        \
                                             .def_val = (def_per_core),        \
                                             .max_val = (max_per_core),        \
                                             .unit = (unit_str),               \
    }

/* set scale with min and max */
#define TEST_INTENSITY_CUSTOM_PRINT(scale, min, def, max, unit_str, print_fn)  \
    .flags = TEST_FLAG_HONORS_INTENSITY, .intensity_desc = {                   \
                                             .curve = scale,                   \
                                             .min_val = (min),                 \
                                             .def_val = (def),                 \
                                             .max_val = (max),                 \
                                             .unit = (unit_str),               \
                                             .custom_print = (print_fn),       \
    }

/* Entirely custom */
#define TEST_INTENSITY_CUSTOM(scale_fn, print_fn)                              \
    .flags = TEST_FLAG_HONORS_INTENSITY, .intensity_desc = {                   \
                                             .curve = SCALE_CUSTOM,            \
                                             .custom_scale = (scale_fn),       \
                                             .custom_print = (print_fn),       \
    }

#define TEST_DECLARE_SMOKE(grp, id, ...)                                       \
    TEST_DECLARE(grp, id, .tier = TEST_TIER_SMOKE, ##__VA_ARGS__)
#define TEST_DECLARE_UNIT(grp, id, ...)                                        \
    TEST_DECLARE(grp, id, .tier = TEST_TIER_UNIT, ##__VA_ARGS__)
#define TEST_DECLARE_INTEGRATION(grp, id, ...)                                 \
    TEST_DECLARE(grp, id, .tier = TEST_TIER_INTEGRATION, ##__VA_ARGS__)

#define TEST_DECLARE_SMOKE(grp, id, ...)                                       \
    TEST_DECLARE(grp, id, .tier = TEST_TIER_SMOKE, ##__VA_ARGS__)
#define TEST_DECLARE_UNIT(grp, id, ...)                                        \
    TEST_DECLARE(grp, id, .tier = TEST_TIER_UNIT, ##__VA_ARGS__)
#define TEST_DECLARE_INTEGRATION(grp, id, ...)                                 \
    TEST_DECLARE(grp, id, .tier = TEST_TIER_INTEGRATION, ##__VA_ARGS__)

/* Goofy macros needed for the DECLARE macro,
 * TODO: do something about this HACK: */
#define __test_group_TEST_GROUP_NONE test_group_orphan_parent
#define __test_group_none test_group_orphan_parent
#define __test_group_orphan test_group_orphan_parent
#define __test_group_test_group_orphan_parent test_group_orphan_parent
