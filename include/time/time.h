/* @title: Time */
#pragma once
#include <stdint.h>
#include <types/types.h>

#define NS_PER_US 1000LL
#define NS_PER_MS 1000000LL
#define NS_PER_S 1000000000LL
#define NS_PER_MIN 60000000000LL
#define NS_PER_HOUR 3600000000000LL
#define NS_PER_DAY 86400000000000LL

#define US_PER_MS 1000LL
#define US_PER_S 1000000LL
#define US_PER_MIN 60000000LL
#define US_PER_HOUR 3600000000LL
#define US_PER_DAY 86400000000LL

#define MS_PER_S 1000LL
#define MS_PER_MIN 60000LL
#define MS_PER_HOUR 3600000LL
#define MS_PER_DAY 86400000LL

#define S_PER_MIN 60LL
#define S_PER_HOUR 3600LL
#define S_PER_DAY 86400LL

#define MIN_PER_HOUR 60LL
#define MIN_PER_DAY 1440LL

#define HOURS_PER_DAY 24LL

#define NS_TO_US(ns) ((time_us_t) ((ns) / NS_PER_US))
#define NS_TO_MS(ns) ((time_ms_t) ((ns) / NS_PER_MS))
#define NS_TO_SECONDS(ns) ((time_s_t) ((ns) / NS_PER_S))
#define NS_TO_MINUTES(ns) ((uint64_t) ((ns) / NS_PER_MIN))
#define NS_TO_HOURS(ns) ((uint64_t) ((ns) / NS_PER_HOUR))
#define NS_TO_DAYS(ns) ((uint64_t) ((ns) / NS_PER_DAY))

#define US_TO_NS(us) ((time_ns_t) ((us) * NS_PER_US))
#define US_TO_MS(us) ((time_ms_t) ((us) / US_PER_MS))
#define US_TO_SECONDS(us) ((time_s_t) ((us) / US_PER_S))
#define US_TO_MINUTES(us) ((uint64_t) ((us) / US_PER_MIN))
#define US_TO_HOURS(us) ((uint64_t) ((us) / US_PER_HOUR))
#define US_TO_DAYS(us) ((uint64_t) ((us) / US_PER_DAY))

#define MS_TO_NS(ms) ((time_ns_t) ((ms) * NS_PER_MS))
#define MS_TO_US(ms) ((time_us_t) ((ms) * US_PER_MS))
#define MS_TO_SECONDS(ms) ((time_s_t) ((ms) / MS_PER_S))
#define MS_TO_MINUTES(ms) ((uint64_t) ((ms) / MS_PER_MIN))
#define MS_TO_HOURS(ms) ((uint64_t) ((ms) / MS_PER_HOUR))
#define MS_TO_DAYS(ms) ((uint64_t) ((ms) / MS_PER_DAY))

#define SECONDS_TO_NS(s) ((time_ns_t) ((s) * NS_PER_S))
#define SECONDS_TO_US(s) ((time_us_t) ((s) * US_PER_S))
#define SECONDS_TO_MS(s) ((time_ms_t) ((s) * MS_PER_S))
#define SECONDS_TO_MINUTES(s) ((uint64_t) ((s) / S_PER_MIN))
#define SECONDS_TO_HOURS(s) ((uint64_t) ((s) / S_PER_HOUR))
#define SECONDS_TO_DAYS(s) ((uint64_t) ((s) / S_PER_DAY))

#define MINUTES_TO_NS(m) ((time_ns_t) ((m) * NS_PER_MIN))
#define MINUTES_TO_US(m) ((time_us_t) ((m) * US_PER_MIN))
#define MINUTES_TO_MS(m) ((time_ms_t) ((m) * MS_PER_MIN))
#define MINUTES_TO_SECONDS(m) ((time_s_t) ((m) * S_PER_MIN))
#define MINUTES_TO_HOURS(m) ((uint64_t) ((m) / MIN_PER_HOUR))
#define MINUTES_TO_DAYS(m) ((uint64_t) ((m) / MIN_PER_DAY))

#define HOURS_TO_NS(h) ((time_ns_t) ((h) * NS_PER_HOUR))
#define HOURS_TO_US(h) ((time_us_t) ((h) * US_PER_HOUR))
#define HOURS_TO_MS(h) ((time_ms_t) ((h) * MS_PER_HOUR))
#define HOURS_TO_SECONDS(h) ((time_s_t) ((h) * S_PER_HOUR))
#define HOURS_TO_MINUTES(h) ((uint64_t) ((h) * MIN_PER_HOUR))
#define HOURS_TO_DAYS(h) ((uint64_t) ((h) / HOURS_PER_DAY))

#define DAYS_TO_NS(d) ((time_ns_t) ((d) * NS_PER_DAY))
#define DAYS_TO_US(d) ((time_us_t) ((d) * US_PER_DAY))
#define DAYS_TO_MS(d) ((time_ms_t) ((d) * MS_PER_DAY))
#define DAYS_TO_SECONDS(d) ((time_s_t) ((d) * S_PER_DAY))
#define DAYS_TO_MINUTES(d) ((uint64_t) ((d) * MIN_PER_DAY))
#define DAYS_TO_HOURS(d) ((uint64_t) ((d) * HOURS_PER_DAY))

void time_print_unix(time_s_t timestamp);
void time_print_current();
uint32_t time_get_unix();
uint8_t time_get_second();
uint8_t time_get_minute();
uint8_t time_get_hour();
uint8_t time_get_day();
uint8_t time_get_month();
uint8_t time_get_year();
uint8_t time_get_century();
time_ms_t time_get_ms(void);

/* Non-blocking variant when spinning on the timekeeper is disallowed */
bool time_try_get_ms(time_ms_t *out);
time_ns_t time_get_ns();
time_us_t time_get_us(void);
freq_hz_t tsc_calibrate(void);
