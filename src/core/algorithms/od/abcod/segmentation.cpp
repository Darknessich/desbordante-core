#include "core/algorithms/od/abcod/segmentation.h"

#include <algorithm>
#include <limits>

namespace algos::abcod {

Series MakeSeries(OrderedSequence const& sequence, RankedSequence const& ranks, size_t begin,
                  size_t end, Direction direction) {
    LmbBuilder builder(ranks);
    builder.Reset(begin);
    for (size_t pos = begin; pos < end; ++pos) {
        builder.Push();
    }
    std::vector<bool> const in_lmb = builder.Restore();

    Series series{.begin = begin, .end = end, .direction = direction};
    series.lmb_size = builder.Length();
    series.cost = builder.Cost();
    series.rows.reserve(end - begin);
    for (size_t pos = begin; pos < end; ++pos) {
        size_t const row = sequence.row_ids[pos];
        series.rows.push_back(row);
        if (sequence.is_null[pos]) continue;
        ++series.non_null_count;
        if (!in_lmb[pos - begin]) series.outliers.push_back(row);
    }
    return series;
}

std::vector<Series> ComputeBcod(OrderedSequence const& sequence, double delta,
                                Direction direction) {
    bool const check_asc = direction != Direction::kDescending;
    bool const check_desc = direction != Direction::kAscending;
    std::vector<Series> segments;

    size_t begin = 0;
    double max = -std::numeric_limits<double>::infinity();
    double min = std::numeric_limits<double>::infinity();
    bool asc_holds = check_asc;
    bool desc_holds = check_desc;

    auto close_segment = [&](size_t end) {
        Series segment{.begin = begin,
                       .end = end,
                       .direction = asc_holds ? Direction::kAscending : Direction::kDescending};
        for (size_t pos = begin; pos < end; ++pos) {
            segment.rows.push_back(sequence.row_ids[pos]);
            if (!sequence.is_null[pos]) ++segment.non_null_count;
        }
        segment.lmb_size = segment.non_null_count;
        segments.push_back(std::move(segment));
    };

    for (size_t pos = 0; pos < sequence.Size(); ++pos) {
        if (sequence.is_null[pos]) continue;
        double const y = sequence.values[pos];
        bool const asc_next = asc_holds && y >= max - delta;
        bool const desc_next = desc_holds && y <= min + delta;
        if (!asc_next && !desc_next) {
            close_segment(pos);
            begin = pos;
            max = min = y;
            asc_holds = check_asc;
            desc_holds = check_desc;
            continue;
        }
        asc_holds = asc_next;
        desc_holds = desc_next;
        max = std::max(max, y);
        min = std::min(min, y);
    }
    if (sequence.Size() > 0) close_segment(sequence.Size());
    return segments;
}

}  
