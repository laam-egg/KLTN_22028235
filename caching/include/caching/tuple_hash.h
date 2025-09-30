/**
 * ChatGPT
 */

 #ifndef TUPLE_HASH_INCLUDED
#define TUPLE_HASH_INCLUDED

#include <tuple>
#include <functional>
#include <utility>

struct TupleHash {
    template <typename ...Args>
    std::size_t operator()(const std::tuple<Args...>& t) const {
        return hashTuple(t, std::index_sequence_for<Args...>{});
    }

private:
    template <typename Tuple, std::size_t... I>
    static std::size_t hashTuple(const Tuple& t, std::index_sequence<I...>) {
        std::size_t seed = 0;
        (..., (seed ^= std::hash<std::tuple_element_t<I, Tuple>>{}(std::get<I>(t)) 
               + 0x9e3779b97f4a7c15ULL + (seed<<6) + (seed>>2)));
        return seed;
    }
};

#endif // TUPLE_HASH_INCLUDED
