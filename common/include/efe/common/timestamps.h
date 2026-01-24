#ifndef EFE_TIMESTAMPS_UTILITIES_INCLUDED
#define EFE_TIMESTAMPS_UTILITIES_INCLUDED

#include <cstdint>
#include <array>

using time_array_t = std::array<int32_t, 6>;
uint64_t to_timestamp_utc(time_array_t const& t);

#endif // EFE_TIMESTAMPS_UTILITIES_INCLUDED
