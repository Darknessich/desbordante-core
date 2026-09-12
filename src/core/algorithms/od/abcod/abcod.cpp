#include "core/algorithms/od/abcod/abcod.h"

#include <utility>

#include "core/algorithms/od/abcod/ordered_sequence.h"
#include "core/algorithms/od/abcod/series_discovery.h"
#include "core/config/exceptions.h"
#include "core/config/names_and_descriptions.h"
#include "core/config/option.h"
#include "core/config/option_using.h"
#include "core/config/tabular_data/input_table/option.h"
#include "core/util/logger.h"

namespace algos::abcod {

Abcod::Abcod() : Algorithm() {
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName()});
}

void Abcod::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    auto check_delta = [](double delta) {
        if (!(delta >= 0)) {
            throw config::ConfigurationError("Band width must be non-negative");
        }
    };
    auto check_coverage = [](double coverage) {
        if (!(coverage >= 0 && coverage <= 1)) {
            throw config::ConfigurationError("Coverage must be in [0, 1]");
        }
    };

    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(Option<double>{&delta_, kDelta, kDBandWidth, 0.0}.SetValueCheck(check_delta));
    RegisterOption(
            Option<Direction>{&direction_, kBandDirection, kDBandDirection, Direction::kAscending});
    RegisterOption(Option<size_t>{&epsilon_, kMaxOutlierRun, kDMaxOutlierRun, 0});
    RegisterOption(Option<bool>{&use_pieces_, kUsePieces, kDUsePieces, true});
    RegisterOption(Option<double>{&min_coverage_, kMinCoverage, kDMinCoverage, 0.9}.SetValueCheck(
            check_coverage));
    RegisterOption(Option<size_t>{&min_series_size_, kMinSeriesSize, kDMinSeriesSize, 2});
}

void Abcod::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable(
            {kDelta, kBandDirection, kMaxOutlierRun, kUsePieces, kMinCoverage, kMinSeriesSize});
}

void Abcod::LoadDataInternal() {
    rows_.clear();
    column_count_ = input_table_->GetNumberOfColumns();
    input_table_->Reset();
    while (input_table_->HasNextRow()) {
        rows_.push_back(input_table_->GetNextRow());
    }
}

void Abcod::ResetState() {
    band_ods_.clear();
}

void Abcod::ExecuteInternal() {
    std::vector<ParsedColumn> columns;
    columns.reserve(column_count_);
    for (size_t column = 0; column < column_count_; ++column) {
        columns.push_back(ParseColumn(rows_, column));
    }

    for (config::IndexType lhs = 0; lhs < column_count_; ++lhs) {
        std::vector<size_t> const order = OrderRows(rows_, {lhs});
        for (config::IndexType rhs = 0; rhs < column_count_; ++rhs) {
            if (rhs == lhs || !columns[rhs].numeric) continue;
            OrderedSequence const sequence = MakeOrderedSequence(order, columns[rhs]);
            if (sequence.non_null_count == 0) continue;

            SeriesDiscoveryResult discovery =
                    DiscoverSeries(sequence, delta_, epsilon_, direction_, use_pieces_);
            size_t covered = 0;
            for (Series const& series : discovery.series) {
                if (series.non_null_count >= min_series_size_) covered += series.lmb_size;
            }
            double const coverage = static_cast<double>(covered) / sequence.non_null_count;
            if (coverage < min_coverage_) continue;

            band_ods_.push_back({.lhs = lhs,
                                 .rhs = rhs,
                                 .series = std::move(discovery.series),
                                 .gain = discovery.gain,
                                 .coverage = coverage});
            LOG_DEBUG("Found {}", band_ods_.back().ToString());
        }
    }
}

}  
