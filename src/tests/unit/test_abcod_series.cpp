#include <cstddef>
#include <cstdint>
#include <optional>
#include <random>
#include <vector>

#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/od/abcod/abcod_verifier.h"
#include "core/algorithms/od/abcod/direction.h"
#include "core/algorithms/od/abcod/lmb.h"
#include "core/algorithms/od/abcod/ordered_sequence.h"
#include "core/algorithms/od/abcod/series_discovery.h"
#include "core/config/names.h"
#include "tests/common/all_csv_configs.h"

namespace tests {

namespace {

using namespace algos::abcod;
using Values = std::vector<std::optional<double>>;


Values const kExampleT = {1992, 2012, 1996, 1995,         1999, 2000, 1999, 2001, 2002,
                          1982, 1987, 1989, std::nullopt, 1991, 1990, 1991, 1992};

Values const kFigure6 = {2000, 1999, 1998, 1997, 1996, 1995, 1992,
                         1994, 1995, 1994, 1997, 1998, 1999};

OrderedSequence MakeSequence(Values const& values) {
    OrderedSequence sequence;
    for (size_t pos = 0; pos < values.size(); ++pos) {
        sequence.row_ids.push_back(pos);
        sequence.values.push_back(values[pos].value_or(0));
        sequence.is_null.push_back(!values[pos].has_value());
        if (values[pos]) ++sequence.non_null_count;
    }
    return sequence;
}

using BoundList = std::vector<std::pair<size_t, size_t>>;

BoundList SeriesBounds(std::vector<Series> const& series) {
    BoundList bounds;
    for (Series const& s : series) bounds.emplace_back(s.begin, s.end);
    return bounds;
}


std::optional<int64_t> BestSeriesGain(OrderedSequence const& sequence, size_t begin, size_t end,
                                      double delta, size_t epsilon, Direction direction) {
    std::optional<int64_t> best;
    for (bool descending : {false, true}) {
        if (direction == Direction::kAscending && descending) continue;
        if (direction == Direction::kDescending && !descending) continue;
        RankedSequence ranks(sequence, delta, descending);
        LmbBuilder builder(ranks);
        builder.Reset(begin);
        int64_t non_null = 0;
        for (size_t pos = begin; pos < end; ++pos) {
            builder.Push();
            if (!sequence.is_null[pos]) ++non_null;
        }
        if (builder.Cost() > epsilon) continue;
        int64_t gain = (2 * static_cast<int64_t>(builder.Length()) - non_null) * non_null;
        if (!best || gain > *best) best = gain;
    }
    return best;
}

int64_t BruteForceGain(Values const& values, double delta, size_t epsilon, Direction direction) {
    OrderedSequence sequence = MakeSequence(values);
    size_t const size = values.size();
    std::vector<std::vector<std::optional<int64_t>>> series_gain(
            size + 1, std::vector<std::optional<int64_t>>(size + 1));
    for (size_t begin = 0; begin < size; ++begin) {
        for (size_t end = begin + 1; end <= size; ++end) {
            series_gain[begin][end] =
                    BestSeriesGain(sequence, begin, end, delta, epsilon, direction);
        }
    }
    std::optional<int64_t> best;
    for (size_t mask = 0; mask < (size_t{1} << (size - 1)); ++mask) {
        int64_t total = 0;
        bool feasible = true;
        size_t begin = 0;
        for (size_t end = 1; end <= size && feasible; ++end) {
            if (end < size && !(mask >> (end - 1) & 1)) continue;
            std::optional<int64_t> const& gain = series_gain[begin][end];
            if (!gain) feasible = false;
            total += gain.value_or(0);
            begin = end;
        }
        if (feasible && (!best || total > *best)) best = total;
    }
    return best.value_or(0);
}

}  



TEST(AbcodSeries, PaperTable3Bidirectional) {
    SeriesDiscoveryResult result =
            DiscoverSeries(MakeSequence(kExampleT), 1, 1, Direction::kBidirectional, false);
    std::vector<int64_t> const expected = {0, 1, 4, 5, 10, 15, 24, 35, 48, 63, 64, 67, 72};
    for (size_t j = 0; j < expected.size(); ++j) {
        EXPECT_EQ(result.prefix_gains[j], expected[j]) << "G[" << j << "]";
    }
    EXPECT_EQ(result.gain, 112);
}



TEST(AbcodSeries, PaperExample48Ascending) {
    SeriesDiscoveryResult result =
            DiscoverSeries(MakeSequence(kExampleT), 1, 1, Direction::kAscending, false);
    EXPECT_EQ(result.gain, 112);
    EXPECT_EQ(result.prefix_gains[4], 8);
    EXPECT_EQ(SeriesBounds(result.series), BoundList({{0, 9}, {9, 17}}));
    EXPECT_EQ(result.series[0].lmb_size, 8);
    EXPECT_EQ(result.series[0].outliers, std::vector<size_t>({1}));
    EXPECT_EQ(result.series[1].non_null_count, 7);
    EXPECT_EQ(result.series[1].Gain(), 49);
}


TEST(AbcodSeries, PaperExample413Pieces) {
    EXPECT_EQ(ComputePieceBoundaries(MakeSequence(kExampleT), 1, Direction::kBidirectional),
              std::vector<size_t>({0, 1, 2, 4, 7, 9, 10, 17}));
}


TEST(AbcodSeries, PaperTable4Pieces) {
    SeriesDiscoveryResult result =
            DiscoverSeries(MakeSequence(kExampleT), 1, 1, Direction::kBidirectional, true);
    EXPECT_EQ(result.prefix_gains, std::vector<int64_t>({0, 1, 4, 10, 35, 63, 64, 112}));
    EXPECT_EQ(SeriesBounds(result.series), BoundList({{0, 9}, {9, 17}}));
}


TEST(AbcodSeries, PaperExample53) {
    OrderedSequence sequence = MakeSequence(kFigure6);
    EXPECT_EQ(ComputePieceBoundaries(sequence, 1, Direction::kBidirectional),
              std::vector<size_t>({0, 6, 7, 13}));
    SeriesDiscoveryResult exact = DiscoverSeries(sequence, 1, 1, Direction::kBidirectional, false);
    EXPECT_EQ(exact.gain, 89);
    EXPECT_EQ(SeriesBounds(exact.series), BoundList({{0, 10}, {10, 13}}));
    EXPECT_EQ(DiscoverSeries(sequence, 1, 1, Direction::kBidirectional, true).gain, 85);
}



TEST(AbcodSeries, PiecesCanBeSuboptimal) {
    OrderedSequence sequence = MakeSequence({0, 6, 4, 3, 3, 4});
    EXPECT_EQ(ComputePieceBoundaries(sequence, 2, Direction::kAscending),
              std::vector<size_t>({0, 1, 3, 6}));
    EXPECT_EQ(DiscoverSeries(sequence, 2, 0, Direction::kAscending, false).gain, 20);
    EXPECT_EQ(DiscoverSeries(sequence, 2, 0, Direction::kAscending, true).gain, 18);
}

TEST(AbcodSeries, ExactMatchesBruteForce) {
    std::mt19937 gen(4);
    std::uniform_int_distribution<int> length_dist(1, 9);
    std::uniform_int_distribution<int> value_dist(0, 6);
    std::bernoulli_distribution null_dist(0.1);
    for (int iteration = 0; iteration < 400; ++iteration) {
        Values values(length_dist(gen));
        for (auto& value : values) {
            if (!null_dist(gen)) value = value_dist(gen);
        }
        for (double delta : {0.0, 1.0, 2.0}) {
            for (size_t epsilon : {0, 1, 2}) {
                for (Direction direction :
                     {Direction::kAscending, Direction::kDescending, Direction::kBidirectional}) {
                    OrderedSequence sequence = MakeSequence(values);
                    int64_t const expected = BruteForceGain(values, delta, epsilon, direction);
                    SeriesDiscoveryResult exact =
                            DiscoverSeries(sequence, delta, epsilon, direction, false);
                    ASSERT_EQ(exact.gain, expected);
                    int64_t sum = 0;
                    for (Series const& series : exact.series) {
                        ASSERT_LE(series.cost, epsilon);
                        sum += series.Gain();
                    }
                    ASSERT_EQ(sum, expected);
                    SeriesDiscoveryResult pieces =
                            DiscoverSeries(sequence, delta, epsilon, direction, true);
                    ASSERT_LE(pieces.gain, expected);
                }
            }
        }
    }
}


TEST(AbcodSeries, VerifierPaperExample48) {
    using namespace config::names;
    for (bool use_pieces : {false, true}) {
        auto verifier =
                algos::CreateAndLoadAlgorithm<AbcodVerifier>({{kCsvConfig, kAbcodRepriseS1S3},
                                                              {kLhsIndices, config::IndicesType{0}},
                                                              {kRhsIndices, config::IndicesType{2}},
                                                              {kDelta, 1.0},
                                                              {kMaxOutlierRun, size_t{1}},
                                                              {kUsePieces, use_pieces}});
        verifier->Execute();
        EXPECT_EQ(verifier->GetGain(), 112);
        ASSERT_EQ(verifier->GetSeries().size(), 2);
        EXPECT_EQ(verifier->GetSeries()[0].rows.size(), 9);
        EXPECT_EQ(verifier->GetSeries()[1].outliers.size(), 0);
    }
}

}  
