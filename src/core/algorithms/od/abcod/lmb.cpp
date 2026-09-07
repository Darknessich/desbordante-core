#include "core/algorithms/od/abcod/lmb.h"

#include <algorithm>
#include <bit>
#include <cassert>

namespace algos::abcod {

RankedSequence::RankedSequence(OrderedSequence const& sequence, double delta, bool descending)
    : is_null_(sequence.is_null) {
    size_t const size = sequence.Size();
    std::vector<double> oriented(size);
    std::vector<double> distinct;
    distinct.reserve(sequence.non_null_count);
    for (size_t pos = 0; pos < size; ++pos) {
        if (is_null_[pos]) continue;
        oriented[pos] = descending ? -sequence.values[pos] : sequence.values[pos];
        distinct.push_back(oriented[pos]);
    }
    std::ranges::sort(distinct);
    auto const [last, end] = std::ranges::unique(distinct);
    distinct.erase(last, end);
    distinct_count_ = distinct.size();

    rank_.assign(size, 0);
    count_le_.assign(size, 0);
    count_le_delta_.assign(size, 0);
    for (size_t pos = 0; pos < size; ++pos) {
        if (is_null_[pos]) continue;
        double const value = oriented[pos];
        rank_[pos] = std::ranges::lower_bound(distinct, value) - distinct.begin();
        count_le_[pos] = rank_[pos] + 1;
        count_le_delta_[pos] = std::ranges::upper_bound(distinct, value + delta) - distinct.begin();
    }
}

LmbBuilder::LmbBuilder(RankedSequence const& ranks)
    : ranks_(&ranks),
      tree_(ranks.DistinctCount() + 1, 0),
      top_bit_(std::bit_floor(std::max<size_t>(ranks.DistinctCount(), 1))),
      k1_(ranks.Size(), 0),
      k2_(ranks.Size(), 0) {}

void LmbBuilder::Add(size_t rank, int delta) {
    for (size_t i = rank + 1; i < tree_.size(); i += i & (~i + 1)) {
        tree_[i] += delta;
    }
}

size_t LmbBuilder::PrefixCount(size_t count) const {
    size_t sum = 0;
    for (size_t i = count; i > 0; i -= i & (~i + 1)) {
        sum += tree_[i];
    }
    return sum;
}

size_t LmbBuilder::FindKth(size_t k) const {
    
    size_t pos = 0;
    for (size_t step = top_bit_; step > 0; step >>= 1) {
        size_t next = pos + step;
        if (next < tree_.size() && static_cast<size_t>(tree_[next]) < k) {
            pos = next;
            k -= tree_[next];
        }
    }
    return pos;
}

void LmbBuilder::Reset(size_t begin) {
    for (size_t rank : touched_) {
        for (size_t i = rank + 1; i < tree_.size(); i += i & (~i + 1)) {
            tree_[i] = 0;
        }
    }
    touched_.clear();
    begin_ = begin;
    end_ = begin;
    length_ = 0;
}

void LmbBuilder::Push() {
    size_t const pos = end_++;
    assert(pos < ranks_->Size());
    if (ranks_->IsNull(pos)) {
        k1_[pos] = 0;
        return;
    }
    size_t const k1 = PrefixCount(ranks_->CountLe(pos)) + 1;
    size_t const k2 = PrefixCount(ranks_->CountLeDelta(pos)) + 1;
    if (k2 <= length_) {
        Add(FindKth(k2), -1);
    } else {
        ++length_;
    }
    size_t const rank = ranks_->Rank(pos);
    Add(rank, +1);
    touched_.push_back(rank);
    k1_[pos] = k1;
    k2_[pos] = k2;
}

std::vector<bool> LmbBuilder::Restore() const {
    std::vector<bool> in_lmb(end_ - begin_, false);
    size_t level = length_;
    for (size_t pos = end_; pos > begin_ && level > 0; --pos) {
        size_t const i = pos - 1;
        if (k1_[i] != 0 && k1_[i] <= level && level <= k2_[i]) {
            in_lmb[i - begin_] = true;
            --level;
        }
    }
    assert(level == 0);
    return in_lmb;
}

size_t LmbBuilder::Cost() const {
    size_t level = length_;
    size_t run = 0;
    size_t cost = 0;
    for (size_t pos = end_; pos > begin_; --pos) {
        size_t const i = pos - 1;
        if (k1_[i] == 0) continue;
        if (level > 0 && k1_[i] <= level && level <= k2_[i]) {
            --level;
            run = 0;
        } else {
            cost = std::max(cost, ++run);
        }
    }
    return cost;
}

}  
