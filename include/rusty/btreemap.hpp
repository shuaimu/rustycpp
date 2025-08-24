#ifndef RUSTY_BTREEMAP_HPP
#define RUSTY_BTREEMAP_HPP

#include <algorithm>
#include <functional>
#include <utility>
#include "option.hpp"
#include "vec.hpp"

// @safe
namespace rusty {

// BTreeMap implementation using a sorted vector (flat map)
// This provides O(log n) lookups and O(n) insertions/deletions
// Keys are kept sorted at all times
template <typename K, typename V, typename Compare = std::less<K>>
class BTreeMap {
private:
    using Entry = std::pair<K, V>;
    Vec<Entry> entries_;
    Compare comp_;
    
    // Binary search for key
    size_t lower_bound_index(const K& key) const {
        size_t left = 0;
        size_t right = entries_.len();
        
        while (left < right) {
            size_t mid = left + (right - left) / 2;
            if (comp_(entries_[mid].first, key)) {
                left = mid + 1;
            } else {
                right = mid;
            }
        }
        
        return left;
    }
    
    // Find exact match
    size_t find_index(const K& key) const {
        size_t idx = lower_bound_index(key);
        if (idx < entries_.len() && !comp_(key, entries_[idx].first) && 
            !comp_(entries_[idx].first, key)) {
            return idx;
        }
        return static_cast<size_t>(-1);
    }
    
public:
    // Constructors
    BTreeMap() = default;
    
    // @lifetime: owned
    static BTreeMap new_() {
        return BTreeMap();
    }
    
    // Move constructor
    BTreeMap(BTreeMap&& other) noexcept 
        : entries_(std::move(other.entries_)), comp_(std::move(other.comp_)) {}
    
    // Move assignment
    BTreeMap& operator=(BTreeMap&& other) noexcept {
        if (this != &other) {
            entries_ = std::move(other.entries_);
            comp_ = std::move(other.comp_);
        }
        return *this;
    }
    
    // Delete copy operations
    BTreeMap(const BTreeMap&) = delete;
    BTreeMap& operator=(const BTreeMap&) = delete;
    
    // Clone for explicit copying
    // @lifetime: owned
    BTreeMap clone() const {
        BTreeMap result;
        result.entries_ = entries_.clone();
        result.comp_ = comp_;
        return result;
    }
    
    // Size and capacity
    size_t len() const { return entries_.len(); }
    bool is_empty() const { return entries_.is_empty(); }
    
    // Clear all entries
    void clear() {
        entries_.clear();
    }
    
    // Insert or update
    void insert(K key, V value) {
        size_t idx = lower_bound_index(key);
        
        if (idx < entries_.len() && !comp_(key, entries_[idx].first) && 
            !comp_(entries_[idx].first, key)) {
            // Key exists, update value
            entries_[idx].second = std::move(value);
        } else {
            // Insert new entry at correct position
            // Vec doesn't have insert, so we need to do it manually
            Entry new_entry{std::move(key), std::move(value)};
            entries_.push(Entry()); // Add dummy at end
            // Shift elements right
            for (size_t i = entries_.len() - 1; i > idx; i--) {
                entries_[i] = std::move(entries_[i - 1]);
            }
            entries_[idx] = std::move(new_entry);
        }
    }
    
    // Get value by key
    // @lifetime: (&'a) -> &'a
    Option<V*> get(const K& key) {
        size_t idx = find_index(key);
        if (idx != static_cast<size_t>(-1)) {
            return Some(&entries_[idx].second);
        }
        return None;
    }
    
    // @lifetime: (&'a) -> &'a
    Option<const V*> get(const K& key) const {
        size_t idx = find_index(key);
        if (idx != static_cast<size_t>(-1)) {
            return Option<const V*>(Some(const_cast<const V*>(&entries_[idx].second)));
        }
        return Option<const V*>(None);
    }
    
    // Get mutable reference to value
    // @lifetime: (&'a mut) -> &'a mut
    Option<V*> get_mut(const K& key) {
        size_t idx = find_index(key);
        if (idx != static_cast<size_t>(-1)) {
            return Some(&entries_[idx].second);
        }
        return None;
    }
    
    // Get key-value pair
    // @lifetime: (&'a) -> &'a
    Option<std::pair<const K*, const V*>> get_key_value(const K& key) const {
        size_t idx = find_index(key);
        if (idx != static_cast<size_t>(-1)) {
            return Option<std::pair<const K*, const V*>>(
                Some(std::make_pair(&entries_[idx].first, &entries_[idx].second))
            );
        }
        return Option<std::pair<const K*, const V*>>(None);
    }
    
    // Remove entry by key
    // @lifetime: owned
    Option<V> remove(const K& key) {
        size_t idx = find_index(key);
        if (idx != static_cast<size_t>(-1)) {
            V value = std::move(entries_[idx].second);
            // Shift elements left to remove at idx
            for (size_t i = idx; i < entries_.len() - 1; i++) {
                entries_[i] = std::move(entries_[i + 1]);
            }
            entries_.pop(); // Remove last element
            return Some(std::move(value));
        }
        return None;
    }
    
    // Remove entry and return both key and value
    // @lifetime: owned
    Option<std::pair<K, V>> remove_entry(const K& key) {
        size_t idx = find_index(key);
        if (idx != static_cast<size_t>(-1)) {
            std::pair<K, V> entry = std::move(entries_[idx]);
            // Shift elements left to remove at idx
            for (size_t i = idx; i < entries_.len() - 1; i++) {
                entries_[i] = std::move(entries_[i + 1]);
            }
            entries_.pop(); // Remove last element
            return Some(std::move(entry));
        }
        return None;
    }
    
    // Check if key exists
    bool contains_key(const K& key) const {
        return find_index(key) != static_cast<size_t>(-1);
    }
    
    // Get or insert with default
    // @lifetime: (&'a mut) -> &'a mut
    V& entry(K key) {
        size_t idx = lower_bound_index(key);
        
        if (idx < entries_.len() && !comp_(key, entries_[idx].first) && 
            !comp_(entries_[idx].first, key)) {
            return entries_[idx].second;
        } else {
            Entry new_entry{std::move(key), V()};
            entries_.push(Entry()); // Add dummy at end
            // Shift elements right
            for (size_t i = entries_.len() - 1; i > idx; i--) {
                entries_[i] = std::move(entries_[i - 1]);
            }
            entries_[idx] = std::move(new_entry);
            return entries_[idx].second;
        }
    }
    
    // Get or insert with value
    // @lifetime: (&'a mut) -> &'a mut
    V& or_insert(K key, V default_value) {
        size_t idx = lower_bound_index(key);
        
        if (idx < entries_.len() && !comp_(key, entries_[idx].first) && 
            !comp_(entries_[idx].first, key)) {
            return entries_[idx].second;
        } else {
            Entry new_entry{std::move(key), std::move(default_value)};
            entries_.push(Entry()); // Add dummy at end
            // Shift elements right
            for (size_t i = entries_.len() - 1; i > idx; i--) {
                entries_[i] = std::move(entries_[i - 1]);
            }
            entries_[idx] = std::move(new_entry);
            return entries_[idx].second;
        }
    }
    
    // Get first (minimum) key-value pair
    // @lifetime: (&'a) -> &'a
    Option<std::pair<const K*, const V*>> first_key_value() const {
        if (!entries_.is_empty()) {
            return Option<std::pair<const K*, const V*>>(
                Some(std::make_pair(&entries_[0].first, &entries_[0].second))
            );
        }
        return Option<std::pair<const K*, const V*>>(None);
    }
    
    // Get last (maximum) key-value pair
    // @lifetime: (&'a) -> &'a
    Option<std::pair<const K*, const V*>> last_key_value() const {
        if (!entries_.is_empty()) {
            size_t last_idx = entries_.len() - 1;
            return Option<std::pair<const K*, const V*>>(
                Some(std::make_pair(&entries_[last_idx].first, &entries_[last_idx].second))
            );
        }
        return Option<std::pair<const K*, const V*>>(None);
    }
    
    // Pop first (minimum) entry
    // @lifetime: owned
    Option<std::pair<K, V>> pop_first() {
        if (!entries_.is_empty()) {
            std::pair<K, V> entry = std::move(entries_[0]);
            // Shift all elements left
            for (size_t i = 0; i < entries_.len() - 1; i++) {
                entries_[i] = std::move(entries_[i + 1]);
            }
            entries_.pop();
            return Some(std::move(entry));
        }
        return None;
    }
    
    // Pop last (maximum) entry
    // @lifetime: owned
    Option<std::pair<K, V>> pop_last() {
        if (!entries_.is_empty()) {
            size_t last_idx = entries_.len() - 1;
            std::pair<K, V> entry = std::move(entries_[last_idx]);
            entries_.pop();
            return Some(std::move(entry));
        }
        return None;
    }
    
    // Range operations
    
    // Get all entries with keys in range [min, max]
    // @lifetime: owned
    Vec<std::pair<K, V>> range(const K& min, const K& max) const {
        Vec<std::pair<K, V>> result;
        
        size_t start = lower_bound_index(min);
        for (size_t i = start; i < entries_.len(); i++) {
            if (comp_(max, entries_[i].first)) {
                break;
            }
            result.push(entries_[i]);
        }
        
        return result;
    }
    
    // Split off everything after key (exclusive)
    // @lifetime: owned
    BTreeMap split_off(const K& key) {
        BTreeMap result;
        
        size_t idx = lower_bound_index(key);
        if (idx < entries_.len()) {
            // Move entries after key to new map
            for (size_t i = idx; i < entries_.len(); i++) {
                result.entries_.push(std::move(entries_[i]));
            }
            // Remove from original
            size_t to_remove = entries_.len() - idx;
            for (size_t i = 0; i < to_remove; i++) {
                entries_.pop();
            }
        }
        
        return result;
    }
    
    // Append another map (all keys in other must be > all keys in self)
    void append(BTreeMap&& other) {
        if (other.is_empty()) return;
        
        if (!is_empty() && !comp_(entries_[entries_.len() - 1].first, other.entries_[0].first)) {
            throw std::invalid_argument("append: other map keys must be greater than self keys");
        }
        
        for (size_t i = 0; i < other.entries_.len(); i++) {
            entries_.push(std::move(other.entries_[i]));
        }
        other.entries_.clear();
    }
    
    // Iterator support - simple index-based iteration
    class iterator {
    private:
        Vec<Entry>* vec_;
        size_t index_;
    public:
        iterator(Vec<Entry>* v, size_t i) : vec_(v), index_(i) {}
        Entry& operator*() { return (*vec_)[index_]; }
        Entry* operator->() { return &(*vec_)[index_]; }
        iterator& operator++() { ++index_; return *this; }
        bool operator!=(const iterator& other) const { return index_ != other.index_; }
        bool operator==(const iterator& other) const { return index_ == other.index_; }
    };
    
    class const_iterator {
    private:
        const Vec<Entry>* vec_;
        size_t index_;
    public:
        const_iterator(const Vec<Entry>* v, size_t i) : vec_(v), index_(i) {}
        const Entry& operator*() const { return (*vec_)[index_]; }
        const Entry* operator->() const { return &(*vec_)[index_]; }
        const_iterator& operator++() { ++index_; return *this; }
        bool operator!=(const const_iterator& other) const { return index_ != other.index_; }
        bool operator==(const const_iterator& other) const { return index_ == other.index_; }
    };
    
    // @lifetime: (&'a) -> &'a
    iterator begin() { return iterator(&entries_, 0); }
    // @lifetime: (&'a) -> &'a
    iterator end() { return iterator(&entries_, entries_.len()); }
    // @lifetime: (&'a) -> &'a
    const_iterator begin() const { return const_iterator(&entries_, 0); }
    // @lifetime: (&'a) -> &'a
    const_iterator end() const { return const_iterator(&entries_, entries_.len()); }
    
    // Collect all keys
    // @lifetime: owned
    Vec<K> keys() const {
        Vec<K> result = Vec<K>::with_capacity(entries_.len());
        for (size_t i = 0; i < entries_.len(); i++) {
            result.push(entries_[i].first);
        }
        return result;
    }
    
    // Collect all values
    // @lifetime: owned
    Vec<V> values() const {
        Vec<V> result = Vec<V>::with_capacity(entries_.len());
        for (size_t i = 0; i < entries_.len(); i++) {
            result.push(entries_[i].second);
        }
        return result;
    }
    
    // Extend from another map
    void extend(BTreeMap&& other) {
        for (size_t i = 0; i < other.entries_.len(); i++) {
            insert(std::move(other.entries_[i].first), std::move(other.entries_[i].second));
        }
        other.entries_.clear();
    }
    
    // Retain only entries matching predicate
    template<typename Pred>
    void retain(Pred pred) {
        Vec<Entry> new_entries;
        for (size_t i = 0; i < entries_.len(); i++) {
            if (pred(entries_[i].first, entries_[i].second)) {
                new_entries.push(std::move(entries_[i]));
            }
        }
        entries_ = std::move(new_entries);
    }
    
    // Equality comparison
    bool operator==(const BTreeMap& other) const {
        return entries_ == other.entries_;
    }
    
    bool operator!=(const BTreeMap& other) const {
        return entries_ != other.entries_;
    }
};

// Factory functions
template<typename K, typename V>
// @lifetime: owned
BTreeMap<K, V> btreemap() {
    return BTreeMap<K, V>::new_();
}

// Create from vector of pairs
template<typename K, typename V>
// @lifetime: owned
BTreeMap<K, V> btreemap_from_vec(Vec<std::pair<K, V>> vec) {
    BTreeMap<K, V> map;
    for (size_t i = 0; i < vec.len(); i++) {
        map.insert(std::move(vec[i].first), std::move(vec[i].second));
    }
    return map;
}

} // namespace rusty

#endif // RUSTY_BTREEMAP_HPP