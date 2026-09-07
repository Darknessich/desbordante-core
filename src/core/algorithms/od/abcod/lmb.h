#pragma once

#include <cstddef>
#include <vector>

#include "core/algorithms/od/abcod/ordered_sequence.h"

namespace algos::abcod {



class RankedSequence {
private:
    std::vector<bool> is_null_;
    
    std::vector<size_t> rank_;
    
    std::vector<size_t> count_le_;
    
    std::vector<size_t> count_le_delta_;
    size_t distinct_count_ = 0;

public:
    RankedSequence(OrderedSequence const& sequence, double delta, bool descending);

    [[nodiscard]] size_t Size() const noexcept {
        return is_null_.size();
    }

    [[nodiscard]] bool IsNull(size_t pos) const {
        return is_null_[pos];
    }

    [[nodiscard]] size_t Rank(size_t pos) const {
        return rank_[pos];
    }

    [[nodiscard]] size_t CountLe(size_t pos) const {
        return count_le_[pos];
    }

    [[nodiscard]] size_t CountLeDelta(size_t pos) const {
        return count_le_delta_[pos];
    }

    [[nodiscard]] size_t DistinctCount() const noexcept {
        return distinct_count_;
    }
};









class LmbBuilder {
private:
    RankedSequence const* ranks_;
    
    std::vector<int> tree_;
    size_t top_bit_ = 1;
    
    std::vector<size_t> touched_;
    size_t begin_ = 0;
    size_t end_ = 0;
    size_t length_ = 0;
    
    std::vector<size_t> k1_;
    std::vector<size_t> k2_;

    void Add(size_t rank, int delta);
    [[nodiscard]] size_t PrefixCount(size_t count) const;
    [[nodiscard]] size_t FindKth(size_t k) const;

public:
    explicit LmbBuilder(RankedSequence const& ranks);

    
    void Reset(size_t begin);
    
    void Push();

    [[nodiscard]] size_t Begin() const noexcept {
        return begin_;
    }

    [[nodiscard]] size_t End() const noexcept {
        return end_;
    }

    
    [[nodiscard]] size_t Length() const noexcept {
        return length_;
    }

    
    [[nodiscard]] std::vector<bool> Restore() const;
    
    
    [[nodiscard]] size_t Cost() const;
};

}  
