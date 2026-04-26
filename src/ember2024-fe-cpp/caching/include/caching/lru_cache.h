/**
 * ChatGPT
 */

#ifndef LRU_CACHE_INCLUDED
#define LRU_CACHE_INCLUDED

#include <functional>

/**
 * ChatGPT-generated
 */
template <typename Key, typename Value, typename Hash = std::hash<Key>>
class LRUCache {
public:
    explicit LRUCache(size_t capacity) : capacity_(capacity) {}

    bool get(const Key& key, Value& value) {
        auto it = map_.find(key);
        if (it == map_.end()) return false;
        cache_.splice(cache_.begin(), cache_, it->second);
        value = it->second->second;
        return true;
    }

    void put(const Key& key, const Value& value) {
        auto it = map_.find(key);
        if (it != map_.end()) {
            it->second->second = value;
            cache_.splice(cache_.begin(), cache_, it->second);
        } else {
            if (cache_.size() == capacity_) {
                auto last = cache_.end(); --last;
                map_.erase(last->first);
                cache_.pop_back();
            }
            cache_.emplace_front(key, value);
            map_[key] = cache_.begin();
        }
    }

private:
    size_t capacity_;
    std::list<std::pair<Key, Value>> cache_;
    std::unordered_map<Key, typename std::list<std::pair<Key, Value>>::iterator, Hash> map_;
};

#endif // LRU_CACHE_INCLUDED
