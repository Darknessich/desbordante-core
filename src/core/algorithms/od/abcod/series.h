#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "core/algorithms/od/abcod/direction.h"

namespace algos::abcod {



struct Series {
    
    size_t begin = 0;
    size_t end = 0;
    
    Direction direction = Direction::kAscending;
    
    size_t non_null_count = 0;
    size_t lmb_size = 0;
    
    size_t cost = 0;
    
    std::vector<size_t> rows;
    
    std::vector<size_t> outliers;

    
    [[nodiscard]] int64_t Gain() const noexcept {
        auto const nn = static_cast<int64_t>(non_null_count);
        return (2 * static_cast<int64_t>(lmb_size) - nn) * nn;
    }
};

}  
