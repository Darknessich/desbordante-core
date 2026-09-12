#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "core/algorithms/od/abcod/series.h"
#include "core/config/indices/type.h"

namespace algos::abcod {


struct BandOd {
    config::IndexType lhs = 0;
    config::IndexType rhs = 0;
    std::vector<Series> series;
    
    int64_t gain = 0;
    
    
    double coverage = 0;

    [[nodiscard]] std::string ToString() const {
        return "[" + std::to_string(lhs) + "] -> " + std::to_string(rhs) + " (" +
               std::to_string(series.size()) + " series, coverage " + std::to_string(coverage) +
               ")";
    }
};

}  
