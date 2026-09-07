#include "core/algorithms/od/abcod/abcod_verifier.h"

#include <algorithm>
#include <stdexcept>

#include "core/algorithms/od/abcod/lmb.h"
#include "core/algorithms/od/abcod/segmentation.h"
#include "core/config/exceptions.h"
#include "core/config/indices/option.h"
#include "core/config/names_and_descriptions.h"
#include "core/config/option.h"
#include "core/config/option_using.h"
#include "core/config/tabular_data/input_table/option.h"
#include "core/util/logger.h"

namespace algos::abcod {

AbcodVerifier::AbcodVerifier() : Algorithm() {
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName()});
}

void AbcodVerifier::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    auto get_cols_num = [this]() {
        return input_table_ ? static_cast<config::IndexType>(input_table_->GetNumberOfColumns())
                            : 0;
    };
    auto check_single_rhs = [](config::IndicesType const& indices) {
        if (indices.size() != 1) {
            throw config::ConfigurationError("abcOD supports exactly one right-hand side column");
        }
    };
    auto check_delta = [](double delta) {
        if (!(delta >= 0)) {
            throw config::ConfigurationError("Band width must be non-negative");
        }
    };

    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(config::kLhsIndicesOpt(&lhs_indices_, get_cols_num));
    RegisterOption(config::kRhsIndicesOpt(&rhs_indices_, get_cols_num, check_single_rhs));
    RegisterOption(Option<double>{&delta_, kDelta, kDBandWidth, 0.0}.SetValueCheck(check_delta));
    RegisterOption(
            Option<Direction>{&direction_, kBandDirection, kDBandDirection, Direction::kAscending});
}

void AbcodVerifier::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable({config::kLhsIndicesOpt.GetName(), config::kRhsIndicesOpt.GetName(),
                          kDelta, kBandDirection});
}

void AbcodVerifier::LoadDataInternal() {
    rows_.clear();
    input_table_->Reset();
    while (input_table_->HasNextRow()) {
        rows_.push_back(input_table_->GetNextRow());
    }
}

void AbcodVerifier::ResetState() {
    sequence_ = {};
    lmb_direction_ = Direction::kAscending;
    lmb_size_ = 0;
    outliers_.clear();
    error_ = 0;
    segments_.clear();
}

void AbcodVerifier::VerifyAbod() {
    Series best;
    bool first = true;
    for (Direction direction : {Direction::kAscending, Direction::kDescending}) {
        if (direction_ != Direction::kBidirectional && direction != direction_) continue;
        RankedSequence ranks(sequence_, delta_, direction == Direction::kDescending);
        Series series = MakeSeries(sequence_, ranks, 0, sequence_.Size(), direction);
        if (first || series.lmb_size > best.lmb_size) {
            best = std::move(series);
            first = false;
        }
    }
    lmb_direction_ = best.direction;
    lmb_size_ = best.lmb_size;
    outliers_ = std::move(best.outliers);
    std::ranges::sort(outliers_);
    error_ = sequence_.non_null_count == 0
                     ? 0
                     : static_cast<Error>(outliers_.size()) / sequence_.non_null_count;
}

void AbcodVerifier::ExecuteInternal() {
    sequence_ = BuildOrderedSequence(rows_, lhs_indices_, rhs_indices_.front());
    VerifyAbod();
    segments_ = ComputeBcod(sequence_, delta_, direction_);
    LOG_DEBUG("abOD error {}, LMB size {}, bcOD segments {}", error_, lmb_size_, segments_.size());
}

}
