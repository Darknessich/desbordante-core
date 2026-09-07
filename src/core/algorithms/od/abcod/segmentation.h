#pragma once

#include <vector>

#include "core/algorithms/od/abcod/direction.h"
#include "core/algorithms/od/abcod/lmb.h"
#include "core/algorithms/od/abcod/ordered_sequence.h"
#include "core/algorithms/od/abcod/series.h"

namespace algos::abcod {



Series MakeSeries(OrderedSequence const& sequence, RankedSequence const& ranks, size_t begin,
                  size_t end, Direction direction);





std::vector<Series> ComputeBcod(OrderedSequence const& sequence, double delta, Direction direction);

}  
