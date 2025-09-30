/**
 * ChatGPT
 */

#ifndef CACHED_FUNCTION_INCLUDED
#define CACHED_FUNCTION_INCLUDED

template <typename Func, typename Cache>
class CachedFunction {
    Func func_;
    Cache cache_;

public:
    explicit CachedFunction(Func f, Cache cache)
        : func_(std::move(f)), cache_(std::move(cache)) {}

    template <typename... Args>
    auto operator()(Args&&... args) {
        using Key = std::tuple<std::decay_t<Args>...>;
        using Value = std::invoke_result_t<Func, Args...>;

        static_assert(is_cache<Cache, Key, Value>::value,
                      "Cache does not implement required interface!");

        Key key(std::forward<Args>(args)...);

        if (auto* v = cache_.get(key)) {
            return *v;
        }

        Value result = func_(std::forward<Args>(args)...);
        cache_.put(key, result);
        return result;
    }
};

template <typename Func, typename Cache>
auto make_cached_function(Func f, Cache cache) {
    return CachedFunction<Func, Cache>(std::move(f), std::move(cache));
}

#endif // CACHED_FUNCTION_INCLUDED
