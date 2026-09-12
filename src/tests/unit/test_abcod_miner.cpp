#include <cstddef>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/od/abcod/abcod.h"
#include "core/algorithms/od/abcod/direction.h"
#include "core/config/names.h"
#include "tests/common/all_csv_configs.h"

namespace tests {

namespace {

using namespace algos::abcod;

std::vector<std::pair<config::IndexType, config::IndexType>> Mine(algos::StdParamsMap params) {
    auto miner = algos::CreateAndLoadAlgorithm<Abcod>(params);
    miner->Execute();
    std::vector<std::pair<config::IndexType, config::IndexType>> found;
    for (BandOd const& od : miner->GetBandOds()) found.emplace_back(od.lhs, od.rhs);
    return found;
}

algos::StdParamsMap MakeParams(bool use_pieces) {
    using namespace config::names;
    return {{kCsvConfig, kAbcodMiner},   {kDelta, 1.0},
            {kMaxOutlierRun, size_t{1}}, {kBandDirection, Direction::kBidirectional},
            {kMinCoverage, 0.9},         {kMinSeriesSize, size_t{10}},
            {kUsePieces, use_pieces}};
}

}  



TEST(AbcodMiner, FindsBandOdsBothWays) {
    using Found = std::vector<std::pair<config::IndexType, config::IndexType>>;
    for (bool use_pieces : {false, true}) {
        EXPECT_EQ(Mine(MakeParams(use_pieces)), Found({{0, 1}, {1, 0}}));
    }
}

TEST(AbcodMiner, CoverageOfFoundDependency) {
    auto miner = algos::CreateAndLoadAlgorithm<Abcod>(MakeParams(false));
    miner->Execute();
    ASSERT_FALSE(miner->GetBandOds().empty());
    BandOd const& od = miner->GetBandOds().front();
    EXPECT_DOUBLE_EQ(od.coverage, 29.0 / 30);
    ASSERT_EQ(od.series.size(), 1);
    EXPECT_EQ(od.series[0].outliers, std::vector<size_t>({11}));
}


TEST(AbcodMiner, ShortSeriesCoverSaw) {
    algos::StdParamsMap params = MakeParams(false);
    params[config::names::kMinSeriesSize] = size_t{2};
    std::vector<std::pair<config::IndexType, config::IndexType>> found = Mine(params);
    EXPECT_NE(std::ranges::find(found, std::pair<config::IndexType, config::IndexType>{0, 2}),
              found.end());
}

}  
