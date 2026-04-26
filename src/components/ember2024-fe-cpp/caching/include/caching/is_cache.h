/**
 * ChatGPT
 */

#ifndef IS_CACHE_INCLUDED
#define IS_CACHE_INCLUDED

#include <type_traits>
#include <utility>

// Primary template: defaults to false
template <typename, typename Key, typename Value, typename = void>
struct is_cache : std::false_type {};

// Specialization: true if contains/put/get exist with correct signatures
template <typename C, typename Key, typename Value>
struct is_cache<C, Key, Value, std::void_t<
    decltype(std::declval<C&>().contains(std::declval<const Key&>())),
    decltype(std::declval<C&>().put(std::declval<const Key&>(), std::declval<const Value&>())),
    decltype(std::declval<C&>().get(std::declval<const Key&>()))
>> : std::true_type {};

#endif // IS_CACHE_INCLUDED
