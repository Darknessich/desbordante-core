#include <cstddef>
#include <limits>
#include <optional>
#include <random>
#include <vector>

#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/od/abcod/abcod_verifier.h"
#include "core/algorithms/od/abcod/direction.h"
#include "core/algorithms/od/abcod/lmb.h"
#include "core/algorithms/od/abcod/ordered_sequence.h"
#include "core/algorithms/od/abcod/segmentation.h"
#include "core/config/names.h"
#include "tests/common/all_csv_configs.h"

namespace tests {

namespace {

using namespace algos::abcod;
using Values = std::vector<std::optional<double>>;


Values const kSeries1 = {1992, 2012, 1996, 1995, 1999, 2000, 1999, 2001, 2002};
Values const kSeries2 = {2000, 1998, 1997, 1996, 1994};

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

struct BandResult {
    size_t length;
    std::vector<bool> in_lmb;
    size_t cost;
};

BandResult ComputeBand(Values const& values, double delta, bool descending) {
    OrderedSequence sequence = MakeSequence(values);
    RankedSequence ranks(sequence, delta, descending);
    LmbBuilder builder(ranks);
    builder.Reset(0);
    for (size_t pos = 0; pos < values.size(); ++pos) builder.Push();
    return {builder.Length(), builder.Restore(), builder.Cost()};
}


bool IsMonotonicBand(std::vector<double> const& values, double delta) {
    double max = -std::numeric_limits<double>::infinity();
    for (double value : values) {
        if (value < max - delta) return false;
        max = std::max(max, value);
    }
    return true;
}

size_t BruteForceLmbLength(Values const& values, double delta, bool descending) {
    std::vector<double> present;
    for (auto const& value : values) {
        if (value) present.push_back(descending ? -*value : *value);
    }
    size_t best = 0;
    for (size_t mask = 0; mask < (size_t{1} << present.size()); ++mask) {
        std::vector<double> chosen;
        for (size_t i = 0; i < present.size(); ++i) {
            if (mask >> i & 1) chosen.push_back(present[i]);
        }
        if (chosen.size() > best && IsMonotonicBand(chosen, delta)) best = chosen.size();
    }
    return best;
}

algos::StdParamsMap MakeParams(CSVConfig const& csv_config, double delta, Direction direction,
                               config::IndexType rhs = 2) {
    using namespace config::names;
    return {{kCsvConfig, csv_config},
            {kLhsIndices, config::IndicesType{0}},
            {kRhsIndices, config::IndicesType{rhs}},
            {kDelta, delta},
            {kBandDirection, direction}};
}

}  


TEST(AbcodLmb, PaperExample312) {
    BandResult band = ComputeBand(kSeries1, 1, false);
    EXPECT_EQ(band.length, 8);
    EXPECT_EQ(band.in_lmb,
              std::vector<bool>({true, false, true, true, true, true, true, true, true}));
    EXPECT_EQ(band.cost, 1);  
}


TEST(AbcodLmb, PaperExample52Descending) {
    EXPECT_EQ(ComputeBand(kSeries2, 1, true).length, 5);
}



TEST(AbcodLmb, ZeroBandWidth) {
    BandResult band = ComputeBand({3, 1, 2, 2, 5, 4}, 0, false);
    EXPECT_EQ(band.length, 4);
    EXPECT_EQ(band.in_lmb, std::vector<bool>({false, true, true, true, false, true}));
    EXPECT_EQ(band.cost, 1);
}


TEST(AbcodLmb, MissingValues) {
    BandResult band = ComputeBand({1, std::nullopt, 9, std::nullopt, 8, 2, 3}, 0, false);
    EXPECT_EQ(band.length, 3);
    EXPECT_EQ(band.cost, 2);
}

TEST(AbcodLmb, MatchesBruteForce) {
    std::mt19937 gen(2026);
    std::uniform_int_distribution<int> length_dist(0, 10);
    std::uniform_int_distribution<int> value_dist(0, 6);
    std::bernoulli_distribution null_dist(0.1);
    for (int iteration = 0; iteration < 3000; ++iteration) {
        Values values(length_dist(gen));
        for (auto& value : values) {
            if (!null_dist(gen)) value = value_dist(gen);
        }
        for (double delta : {0.0, 0.5, 1.0, 2.0}) {
            for (bool descending : {false, true}) {
                BandResult band = ComputeBand(values, delta, descending);
                ASSERT_EQ(band.length, BruteForceLmbLength(values, delta, descending));
                std::vector<double> chosen;
                for (size_t pos = 0; pos < values.size(); ++pos) {
                    if (band.in_lmb[pos]) {
                        ASSERT_TRUE(values[pos].has_value());
                        chosen.push_back(descending ? -*values[pos] : *values[pos]);
                    }
                }
                ASSERT_EQ(chosen.size(), band.length);
                ASSERT_TRUE(IsMonotonicBand(chosen, delta));
            }
        }
    }
}


TEST(AbcodBcod, PaperExample42) {
    std::vector<Series> segments = ComputeBcod(MakeSequence(kSeries1), 1, Direction::kAscending);
    ASSERT_EQ(segments.size(), 2);
    EXPECT_EQ(segments[0].begin, 0);
    EXPECT_EQ(segments[0].end, 2);
    EXPECT_EQ(segments[1].begin, 2);
    EXPECT_EQ(segments[1].end, 9);
}


TEST(AbcodBcod, Bidirectional) {
    Values values = kSeries1;
    values.insert(values.end(), kSeries2.begin(), kSeries2.end());
    std::vector<Series> segments = ComputeBcod(MakeSequence(values), 1, Direction::kBidirectional);
    ASSERT_EQ(segments.size(), 3);
    EXPECT_EQ(segments[1].begin, 2);
    EXPECT_EQ(segments[1].end, 9);
    EXPECT_EQ(segments[1].direction, Direction::kAscending);
    EXPECT_EQ(segments[2].begin, 9);
    EXPECT_EQ(segments[2].end, 14);
    EXPECT_EQ(segments[2].direction, Direction::kDescending);
}


TEST(AbcodVerifier, PaperExample317) {
    auto verifier = algos::CreateAndLoadAlgorithm<AbcodVerifier>(
            MakeParams(kAbcodRepriseS1, 1, Direction::kAscending));
    verifier->Execute();
    EXPECT_DOUBLE_EQ(verifier->GetError(), 1.0 / 9);
    EXPECT_EQ(verifier->GetOutliers(), std::vector<size_t>({1}));
    EXPECT_EQ(verifier->GetLmbSize(), 8);
    EXPECT_TRUE(verifier->Holds(0.12));
    EXPECT_FALSE(verifier->Holds(0.1));
    ASSERT_EQ(verifier->GetSegments().size(), 2);
    EXPECT_EQ(verifier->GetSegments()[0].rows, std::vector<size_t>({0, 1}));
}


TEST(AbcodVerifier, MissingValueIsIgnored) {
    auto verifier = algos::CreateAndLoadAlgorithm<AbcodVerifier>(
            MakeParams(kAbcodRepriseS1S3, 1, Direction::kAscending));
    verifier->Execute();
    EXPECT_EQ(verifier->GetLmbSize() + verifier->GetOutliers().size(), 16);
}

TEST(AbcodVerifier, BidirectionalChoosesLongerBand) {
    auto verifier = algos::CreateAndLoadAlgorithm<AbcodVerifier>(
            MakeParams(kAbcodFig6, 1, Direction::kBidirectional, 1));
    verifier->Execute();
    EXPECT_EQ(verifier->GetLmbDirection(), Direction::kDescending);
}

TEST(AbcodVerifier, NonNumericRightHandSide) {
    algos::StdParamsMap params = MakeParams(kAbcodReprise, 0, Direction::kAscending);
    params[config::names::kRhsIndices] = config::IndicesType{1};
    auto verifier = algos::CreateAndLoadAlgorithm<AbcodVerifier>(params);
    EXPECT_THROW(verifier->Execute(), std::invalid_argument);
}

}  
