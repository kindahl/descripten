/*
 * descripten - ECMAScript to native compiler
 * Copyright (C) 2011-2013 Christian Kindahl
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser Public License for more details.
 *
 * You should have received a copy of the GNU Lesser Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <cassert>
#include <limits>
#include "common/exception.hh"
#include "common/lexical.hh"
#include "common/stringbuilder.hh"
#include "date.hh"
#include "native.hh"

#define IS_LEAP_YEAR(year) ((year) % 4 == 0 && ((year) % 100 || ((year) % 400 == 0)))

#define HOUR_FROM_TIME(t) (((t) / ms_per_hour) % hours_per_day)
#define MIN_FROM_TIME(t) (((t) / ms_per_minute) % minutes_per_hour)
#define SEC_FROM_TIME(t) (((t) / ms_per_second) % seconds_per_minute)
#define MS_FROM_TIME(t) ((t) % ms_per_second)

static int64_t hours_per_day = 24;
static int64_t minutes_per_hour = 60;
static int64_t seconds_per_minute = 60;
static int64_t ms_per_second = 1000;
static int64_t ms_per_minute = 60000;   // = ms_per_second * seconds_per_minute
static int64_t ms_per_hour = 3600000;   // = ms_per_minute * minutes_per_hour
static int64_t ms_per_day = 86400000;   // = ms_per_hour * hours_per_day

// Number of days that we inherit from a certain month.
static int64_t days_from_month[] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

// Number of days that we inherit from a certain month during a leap year.
static int64_t days_from_month_leap[] = { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 };

double es_date_parse(const String &str)
{
    // Parse date on any of the following forms:
    // YYYY, YYYY-MM or YYYY-MM-DD.
    int year = 0;   // Required.
    int month = 1;  // 1 is default for month if not specified.
    int day = 1;    // 1 is default for day if not specified.

    // Parse years.
    size_t rem = str.length();  // How much remains to be parsed.
    if (rem < 4)
        return std::numeric_limits<double>::quiet_NaN();

    const uni_char *ptr = str.data();
    if (!es_is_dec_number(ptr, 4))
        return std::numeric_limits<double>::quiet_NaN();

    year = es_as_dec_digit(ptr[0]) * 1000 +
           es_as_dec_digit(ptr[1]) *  100 +
           es_as_dec_digit(ptr[2]) *   10 +
           es_as_dec_digit(ptr[3]) *    1;
    ptr += 4;
    rem -= 4;

    // Check for months.
    if (*ptr == '-')
    {
        ptr++; rem--;
        if (rem < 2 || !es_is_dec_number(ptr, 2))
            return std::numeric_limits<double>::quiet_NaN();

        month = es_as_dec_digit(ptr[0]) * 10 +
                es_as_dec_digit(ptr[1]) *  1;
        ptr += 2;
        rem -= 2;

        // Check for days.
        if (*ptr == '-')
        {
            ptr++; rem--;
            if (rem < 2 || !es_is_dec_number(ptr, 2))
                return std::numeric_limits<double>::quiet_NaN();

            day = es_as_dec_digit(ptr[0]) * 10 +
                  es_as_dec_digit(ptr[1]) *  1;
            ptr += 2;
            rem -= 2;
        }
    }

    // Parse time on any of the following forms:
    // THH:mm, THH:mm:ss, THH:mm:ss.sss
    int hour = 0;   // 0 is default for month if not specified.
    int min = 0;    // 0 is default for month if not specified.
    int sec = 0;    // 0 is default for month if not specified.
    int ms = 0;     // 0 is default for month if not specified.

    if (*ptr == 'T')
    {
        ptr++; rem--;

        // Parse hours.
        if (rem < 2 || !es_is_dec_number(ptr, 2))
            return std::numeric_limits<double>::quiet_NaN();

        hour = es_as_dec_digit(ptr[0]) * 10 +
               es_as_dec_digit(ptr[1]) *  1;
        ptr += 2;
        rem -= 2;

        if (*ptr != ':')
            return std::numeric_limits<double>::quiet_NaN();
        ptr++; rem--;

        // Parse minutes.
        if (rem < 2 || !es_is_dec_number(ptr, 2))
            return std::numeric_limits<double>::quiet_NaN();

        min = es_as_dec_digit(ptr[0]) * 10 +
              es_as_dec_digit(ptr[1]) *  1;
        ptr += 2;
        rem -= 2;

        // Parse seconds.
        if (*ptr == ':')
        {
            ptr++; rem--;

            if (rem < 2 || !es_is_dec_number(ptr, 2))
                return std::numeric_limits<double>::quiet_NaN();

            sec = es_as_dec_digit(ptr[0]) * 10 +
                  es_as_dec_digit(ptr[1]) *  1;
            ptr += 2;
            rem -= 2;

            // Parse milliseconds.
            if (*ptr == '.')
            {
                ptr++; rem--;

                if (rem < 3 || !es_is_dec_number(ptr, 3))
                    return std::numeric_limits<double>::quiet_NaN();

                sec = es_as_dec_digit(ptr[0]) * 100 +
                      es_as_dec_digit(ptr[1]) *  10 +
                      es_as_dec_digit(ptr[2]) *   1;
                ptr += 3;
                rem -= 3;
            }
        }
    }

    // FIXME: Parse time zone.

    // Validate value ranges.
    if (!(year >= 0 && year < 9999 &&
          month >= 1 && month <= 12 &&
          day >= 1 && day <= 31 &&
          hour >= 0 && hour <= 24 &&
          min >= 0 && min <= 59 &&
          sec >= 0 && sec <= 59 &&
          ms >= 0 && ms <= 999))
    {
        return std::numeric_limits<double>::quiet_NaN();
    }

    // NOTE: Month number is [0-11] not [1-12].
    return es_make_date(es_make_day(year, month - 1, day), es_make_time(hour, min, sec, ms));
}

double es_make_time(double hour, double min, double sec, double ms)
{
    if (!std::isfinite(hour) || !std::isfinite(min) || !std::isfinite(sec) || !std::isfinite(ms))
        return std::numeric_limits<double>::quiet_NaN();

    return static_cast<int64_t>(hour) * ms_per_hour +
           static_cast<int64_t>(min) * ms_per_minute +
           static_cast<int64_t>(sec) * ms_per_second +
           static_cast<int64_t>(ms);
}

double es_make_day(double year, double month, double date)
{
    if (!std::isfinite(year) || !std::isfinite(month) || !std::isfinite(date))
        return std::numeric_limits<double>::quiet_NaN();

    int64_t y = static_cast<int64_t>(year);
    int64_t m = static_cast<int64_t>(month);
    int64_t d = static_cast<int64_t>(date);

    int64_t ym = y + m / 12;
    int64_t mn = m % 12;

    static const int64_t year_delta = 399999;
    static const int64_t base_day = 365 * (1970 + year_delta) +
                                   (1970 + year_delta) / 4 -
                                   (1970 + year_delta) / 100 +
                                   (1970 + year_delta) / 400;

    int64_t year1 = ym + year_delta;
    int64_t day_from_year = 365 * year1 +
                            year1 / 4 -
                            year1 / 100 +
                            year1 / 400 -
                            base_day;

    if (IS_LEAP_YEAR(ym))
        return day_from_year + days_from_month_leap[mn] + d - 1;
    else
        return day_from_year + days_from_month[mn] + d - 1;
}

double es_make_date(double day, double time)
{
    if (!std::isfinite(day) || !std::isfinite(time))
        return std::numeric_limits<double>::quiet_NaN();

    return day * ms_per_day + time;
}

double es_time_clip(double time)
{
    if (!std::isfinite(time) || fabs(time) > ES_DATE_MAX_TIME)
        return std::numeric_limits<double>::quiet_NaN();

    if (time == 0.0)
        return time;

    return time >= 0.0 ? floor(time) : ceil(time);
}

double es_local_tza()
{
#if defined(PLATFORM_LINUX) || defined(PLATFORM_DARWIN)
    time_t raw_time = time(NULL);

    struct tm *cur_time = localtime(&raw_time);
    if (!cur_time)
        return std::numeric_limits<double>::quiet_NaN();

    return (cur_time->tm_gmtoff * ms_per_second) -
           (cur_time->tm_isdst ? ms_per_hour : 0);
#else
#error "es_timezone_offset is not implemented for the current platform."
#endif
}

double es_daylight_saving_ta(double t)
{
    if (!std::isfinite(t))
        return std::numeric_limits<double>::quiet_NaN();

#if defined(PLATFORM_LINUX) || defined(PLATFORM_DARWIN)
    time_t raw_time = time(NULL);

    struct tm *cur_time = localtime(&raw_time);
    if (!cur_time)
        return std::numeric_limits<double>::quiet_NaN();

    return (cur_time->tm_isdst ? ms_per_hour : 0);
#else
#error "es_daylight_saving_ta is not implemented for the current platform."
#endif
}

double es_local_time(double t)
{
    return t + es_local_tza() + es_daylight_saving_ta(t);
}

double es_utc(double t)
{
    return t - es_local_tza() - es_daylight_saving_ta(t - es_local_tza());
}

int64_t es_days_in_year(int64_t year)
{
    return IS_LEAP_YEAR(year) ? 366 : 365;
}

int64_t es_day_from_year(int64_t year)
{
    return 365 * (year - 1970) + (year - 1969) / 4 - (year - 1901) / 100 + (year - 1601) / 400;
}

int64_t es_time_from_year(int64_t year)
{
    return es_day_from_year(year) * ms_per_day;
}

int64_t es_day(double time)
{
    assert(std::isfinite(time));
    return floor(time / ms_per_day);
}

int64_t es_day_within_year(double time)
{
    assert(std::isfinite(time));
    return es_day(time) - es_day_from_year(es_year_from_time(time));
}

int64_t es_ms_from_time(double time)
{
    assert(std::isfinite(time));
    return static_cast<int64_t>(time) % ms_per_second;
}

int64_t es_sec_from_time(double time)
{
    assert(std::isfinite(time));
    return static_cast<int64_t>(time / ms_per_second) % seconds_per_minute;
}

int64_t es_min_from_time(double time)
{
    assert(std::isfinite(time));
    return static_cast<int64_t>(time / ms_per_minute) % minutes_per_hour;
}

int64_t es_hour_from_time(double time)
{
    assert(std::isfinite(time));
    return static_cast<int64_t>(time / ms_per_hour) % hours_per_day;
}

int64_t es_date_from_time(double time)
{
    assert(std::isfinite(time));

    int64_t year = es_year_from_time(time);
    int64_t day = es_day_within_year(time);

    const int64_t *days_ptr = IS_LEAP_YEAR(year) ? days_from_month_leap : days_from_month;

    for (int i = 11; i >= 0; i--)
    {
        if (day > days_ptr[i])
            return day - days_ptr[i] + 1;
    }

    return 1;
}

int64_t es_month_from_time(double time)
{
    assert(std::isfinite(time));

    int64_t year = es_year_from_time(time);
    int64_t day = es_day_within_year(time);

    const int64_t *days_ptr = IS_LEAP_YEAR(year) ? days_from_month_leap : days_from_month;

    for (int i = 11; i >= 0; i--)
    {
        if (day > days_ptr[i])
            return i + 1;
    }

    return 1;
}

int64_t es_year_from_time(double time)
{
    assert(std::isfinite(time));
    int64_t year = static_cast<int64_t>(floor(time /(ms_per_day * 365.2425)) + 1970);
    int64_t time2 = es_time_from_year(year);

    if (time2 > time)
        return year - 1;

    if (time2 + es_days_in_year(year) * ms_per_day <= time)
        return year + 1;

    return year;
}

String es_date_time_iso_str(double time)
{
    assert(std::isfinite(time));

    // Format: YYYY-MM-DDTHH:mm:ss.sssZ
    return StringBuilder::sprintf("%.4d-%.2d-%.2dT%.2d:%.2d:%.2d.%.3dZ",
                                  es_year_from_time(time), es_month_from_time(time), es_date_from_time(time),
                                  es_hour_from_time(time), es_min_from_time(time), es_sec_from_time(time),
                                  es_ms_from_time(time));
}
