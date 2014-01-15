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

#pragma once
#include "common/string.hh"

#define ES_DATE_MAX_TIME            8640000000000000.0

/**
 * Parses a date time string.
 * @param [in] str String to parse.
 * @return On success the parsed date a ECMAScript time, on failure NaN is
 *         returned.
 * @see ECMA-262 15.9.1.15
 */
double es_date_parse(const String &str);

/**
 * Converts hours, minutes, seconds and milliseconds into ECMAScript time.
 * @param [in] hour Number of hours.
 * @param [in] min Number of minutes.
 * @param [in] sec Number of seconds.
 * @param [in] ms Number of milliseconds.
 * @return hours, minutes, seconds and milliseconds in ECMAScript time.
 * @see ECMA-262 15.9.1.11
 */
double es_make_time(double hour, double min, double sec, double ms);
double es_make_day(double year, double month, double date);
double es_make_date(double day, double time);

double es_time_clip(double time);

/**
 * Local time zone offset from UTC in milliseconds. The offset does not
 * compensate for daylight saving time.
 * @return Number of milliseconds from UTC to local time excluding daylight
 *         saving time.
 * @see ECMA-262 15.9.1.7
 */
double es_local_tza();

/**
 * Time offset caused  by daylight saving time.
 * @param [in] t Time to calculate daylight saving time for.
 * @return Time offset caused by daylight saving time in milliseconds.
 * @see ECMA-262 15.9.1.8
 */
double es_daylight_saving_ta(double t);

/**
 * Converts UTC time to local time.
 * @param [in] time UTC time.
 * return time converted to local time.
 * @see ECMA-262 15.9.1.9
 */
double es_local_time(double time);

/**
 * Converts local time to UTC time.
 * @param [in] time Local time.
 * @return time converted to UTC time.
 * @see ECMA-262 15.9.1.9
 */
double es_utc(double time);

/**
 * Calculates the number of days [365,366] in the year specified by year.
 * @param [in] year Year to determine number of year days for.
 * @return Number of days in year.
 */
int64_t es_days_in_year(int64_t year);

//int64_t es_day_from_year(int64_t year);
//int64_t es_time_from_year(int64_t year);
//int64_t es_day_within_year(double time);
//int64_t es_day(double time);

/**
 * Identifies the number of milliseconds [0-999] within the second.
 * @param [in] time Time to determine milliseconds for.
 * @return Milliseconds specified by time.
 */
int64_t es_ms_from_time(double time);

/**
 * Identifies the number of seconds [0-59] within the minute.
 * @param [in] time Time to determine seconds for.
 * @return Seconds specified by time.
 */
int64_t es_sec_from_time(double time);

/**
 * Identifies the number of minutes [0-59] within the hour.
 * @param [in] time Time to determine minutes for.
 * @return Minutes specified by time.
 */
int64_t es_min_from_time(double time);

/**
 * Identifies the number of hours [0-23] within the day.
 * @param [in] time Time to determine hours for.
 * @return Hours specified by time.
 */
int64_t es_hour_from_time(double time);

/**
 * Identifies the day of month [1-31].
 * @param [in] time Time to determine date for.
 * @return Date specified by time.
 */
int64_t es_date_from_time(double time);

/**
 * Identifies the month [0-11] within a year.
 * @param [in] time Time to find month for.
 * @return Month within the year specified by time.
 */
int64_t es_month_from_time(double time);

/**
 * Identifies the year.
 * @param [in] time Time to determine year for.
 * @return Year specified by time.
 */
int64_t es_year_from_time(double time);

/**
 * Converts ECMAScript time to ISO 8601 string format.
 * @param [in] time Time to convert.
 * @return Time in ISO 8601 format.
 * @see ECMA-262 15.9.1.15
 */
String es_date_time_iso_str(double time);
