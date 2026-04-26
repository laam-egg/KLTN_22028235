#include <limits>

// Credit: https://howardhinnant.github.io/date_algorithms.html#days_from_civil

// Returns number of days since civil 1970-01-01.  Negative values indicate
//    days prior to 1970-01-01.
// Preconditions:  y-m-d represents a date in the civil (Gregorian) calendar
//                 m is in [1, 12]
//                 d is in [1, last_day_of_month(y, m)]
//                 y is "approximately" in
//                   [numeric_limits<Int>::min()/366, numeric_limits<Int>::max()/366]
//                 Exact range of validity is:
//                 [civil_from_days(numeric_limits<Int>::min()),
//                  civil_from_days(numeric_limits<Int>::max()-719468)]
template <class Int>
constexpr
Int
days_from_civil(Int y, unsigned m, unsigned d) noexcept
{
    static_assert(std::numeric_limits<unsigned>::digits >= 18,
             "This algorithm has not been ported to a 16 bit unsigned integer");
    static_assert(std::numeric_limits<Int>::digits >= 20,
             "This algorithm has not been ported to a 16 bit signed integer");
    y -= m <= 2;
    const Int era = (y >= 0 ? y : y-399) / 400;
    const unsigned yoe = static_cast<unsigned>(y - era * 400);      // [0, 399]
    const unsigned doy = (153*(m > 2 ? m-3 : m+9) + 2)/5 + d-1;  // [0, 365]
    const unsigned doe = yoe * 365 + yoe/4 - yoe/100 + doy;         // [0, 146096]
    return era * 146097 + static_cast<Int>(doe) - 719468;
}

#include "efe/common/timestamps.h"
#include <chrono>

uint64_t to_timestamp_utc(time_array_t const& t) {
    int32_t year  = t[0];
    int32_t month = t[1];
    int32_t day   = t[2];
    int32_t hour  = t[3];
    int32_t min   = t[4];
    int32_t sec   = t[5];

    int64_t days = days_from_civil(year, month, day);
    int64_t ts = days * 86400LL + hour * 3600LL + min * 60LL + sec;

    return static_cast<uint64_t>(ts);
}
