#include "core/algorithms/od/abcod/series_discovery.h"

#include <algorithm>
#include <deque>
#include <limits>
#include <numeric>
#include <utility>

#include "core/algorithms/od/abcod/lmb.h"
#include "core/algorithms/od/abcod/segmentation.h"

namespace algos::abcod {

namespace {

constexpr int64_t kNoGain = std::numeric_limits<int64_t>::min();


std::vector<size_t> LongestBandEnds(OrderedSequence const& sequence, double delta,
                                    bool descending) {
    size_t const size = sequence.Size();
    auto value = [&](size_t pos) {
        return descending ? -sequence.values[pos] : sequence.values[pos];
    };
    std::vector<size_t> ends(size);
    
    std::deque<size_t> max_queue;
    size_t end = 0;
    for (size_t begin = 0; begin < size; ++begin) {
        if (end < begin) end = begin;
        while (!max_queue.empty() && max_queue.front() < begin) max_queue.pop_front();
        while (end < size) {
            if (!sequence.is_null[end]) {
                double const y = value(end);
                if (!max_queue.empty() && y < value(max_queue.front()) - delta) break;
                while (!max_queue.empty() && value(max_queue.back()) <= y) max_queue.pop_back();
                max_queue.push_back(end);
            }
            ++end;
        }
        ends[begin] = end;
    }
    return ends;
}

bool IsBand(OrderedSequence const& sequence, size_t begin, size_t end, double delta,
            bool descending) {
    double max = -std::numeric_limits<double>::infinity();
    for (size_t pos = begin; pos < end; ++pos) {
        if (sequence.is_null[pos]) continue;
        double const y = descending ? -sequence.values[pos] : sequence.values[pos];
        if (y < max - delta) return false;
        max = std::max(max, y);
    }
    return true;
}

}  

std::vector<size_t> ComputePieceBoundaries(OrderedSequence const& sequence, double delta,
                                           Direction direction) {
    size_t const size = sequence.Size();
    if (size == 0) return {0};

    
    std::vector<std::pair<size_t, size_t>> bands;
    for (bool descending : {false, true}) {
        std::vector<size_t> const ends = LongestBandEnds(sequence, delta, descending);
        for (size_t begin = 0; begin < size; ++begin) {
            if (begin == 0 || ends[begin - 1] < ends[begin]) bands.emplace_back(begin, ends[begin]);
        }
    }
    
    std::ranges::sort(bands, [](auto const& a, auto const& b) {
        return a.first != b.first ? a.first < b.first : a.second > b.second;
    });
    std::vector<size_t> cuts{0, size};
    size_t max_end = 0;
    for (auto const& [begin, end] : bands) {
        if (end <= max_end) continue;
        max_end = end;
        cuts.push_back(begin);
        cuts.push_back(end);
    }
    std::ranges::sort(cuts);
    auto const [last, cuts_end] = std::ranges::unique(cuts);
    cuts.erase(last, cuts_end);

    if (direction == Direction::kBidirectional) return cuts;
    bool const descending = direction == Direction::kDescending;
    std::vector<size_t> boundaries;
    for (size_t i = 0; i + 1 < cuts.size(); ++i) {
        if (IsBand(sequence, cuts[i], cuts[i + 1], delta, descending)) {
            boundaries.push_back(cuts[i]);
        } else {
            for (size_t pos = cuts[i]; pos < cuts[i + 1]; ++pos) boundaries.push_back(pos);
        }
    }
    boundaries.push_back(size);
    return boundaries;
}

SeriesDiscoveryResult DiscoverSeries(OrderedSequence const& sequence, double delta, size_t epsilon,
                                     Direction direction, bool use_pieces) {
    SeriesDiscoveryResult result;
    size_t const size = sequence.Size();
    if (use_pieces) {
        result.boundaries = ComputePieceBoundaries(sequence, delta, direction);
    } else {
        result.boundaries.resize(size + 1);
        std::iota(result.boundaries.begin(), result.boundaries.end(), 0);
    }
    std::vector<size_t> const& bounds = result.boundaries;
    size_t const count = bounds.size() - 1;

    std::vector<Direction> directions;
    if (direction != Direction::kDescending) directions.push_back(Direction::kAscending);
    if (direction != Direction::kAscending) directions.push_back(Direction::kDescending);
    std::vector<RankedSequence> ranks;
    std::vector<LmbBuilder> builders;
    ranks.reserve(directions.size());
    for (Direction band_direction : directions) {
        ranks.emplace_back(sequence, delta, band_direction == Direction::kDescending);
    }
    for (RankedSequence const& ranked : ranks) builders.emplace_back(ranked);

    std::vector<int64_t>& gains = result.prefix_gains;
    gains.assign(count + 1, kNoGain);
    gains[0] = 0;
    std::vector<size_t> previous(count + 1, 0);
    std::vector<size_t> chosen(count + 1, 0);
    std::vector<size_t> order(directions.size());

    for (size_t start = 0; start < count; ++start) {
        if (gains[start] == kNoGain) continue;
        for (LmbBuilder& builder : builders) builder.Reset(bounds[start]);
        int64_t non_null = 0;
        for (size_t stop = start + 1; stop <= count; ++stop) {
            for (size_t pos = bounds[stop - 1]; pos < bounds[stop]; ++pos) {
                for (LmbBuilder& builder : builders) builder.Push();
                if (!sequence.is_null[pos]) ++non_null;
            }
            
            std::iota(order.begin(), order.end(), 0);
            std::ranges::stable_sort(order, [&builders](size_t a, size_t b) {
                return builders[a].Length() > builders[b].Length();
            });
            for (size_t d : order) {
                auto const length = static_cast<int64_t>(builders[d].Length());
                int64_t const candidate = gains[start] + (2 * length - non_null) * non_null;
                if (candidate <= gains[stop]) break;
                if (builders[d].Cost() > epsilon) continue;
                gains[stop] = candidate;
                previous[stop] = start;
                chosen[stop] = d;
                break;
            }
        }
    }

    result.gain = gains[count];
    for (size_t stop = count; stop > 0; stop = previous[stop]) {
        size_t const start = previous[stop];
        size_t const d = chosen[stop];
        result.series.push_back(
                MakeSeries(sequence, ranks[d], bounds[start], bounds[stop], directions[d]));
    }
    std::ranges::reverse(result.series);
    return result;
}

}  
