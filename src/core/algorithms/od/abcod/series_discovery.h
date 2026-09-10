#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "core/algorithms/od/abcod/direction.h"
#include "core/algorithms/od/abcod/ordered_sequence.h"
#include "core/algorithms/od/abcod/series.h"

namespace algos::abcod {

struct SeriesDiscoveryResult {
    
    std::vector<Series> series;
    
    int64_t gain = 0;
    
    std::vector<size_t> boundaries;
    
    std::vector<int64_t> prefix_gains;
};







std::vector<size_t> ComputePieceBoundaries(OrderedSequence const& sequence, double delta,
                                           Direction direction);








SeriesDiscoveryResult DiscoverSeries(OrderedSequence const& sequence, double delta, size_t epsilon,
                                     Direction direction, bool use_pieces);

}  
