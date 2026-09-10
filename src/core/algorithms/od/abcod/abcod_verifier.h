#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "core/algorithms/algorithm.h"
#include "core/algorithms/od/abcod/direction.h"
#include "core/algorithms/od/abcod/ordered_sequence.h"
#include "core/algorithms/od/abcod/series.h"
#include "core/config/indices/type.h"
#include "core/config/tabular_data/input_table_type.h"

namespace algos::abcod {

class AbcodVerifier : public Algorithm {
public:
    using Error = double;

private:
    config::InputTable input_table_;
    std::vector<std::vector<std::string>> rows_;

    config::IndicesType lhs_indices_;
    config::IndicesType rhs_indices_;
    double delta_ = 0;
    Direction direction_ = Direction::kAscending;
    size_t epsilon_ = 0;
    bool use_pieces_ = false;

    OrderedSequence sequence_;

    Direction lmb_direction_ = Direction::kAscending;
    size_t lmb_size_ = 0;
    std::vector<size_t> outliers_;
    Error error_ = 0;

    std::vector<Series> segments_;
    
    std::vector<Series> series_;
    int64_t gain_ = 0;

    void RegisterOptions();
    void VerifyAbod();

protected:
    void LoadDataInternal() override;
    void MakeExecuteOptsAvailable() override;
    void ExecuteInternal() override;
    void ResetState() override;

    [[nodiscard]] OrderedSequence const& GetSequence() const noexcept {
        return sequence_;
    }

    [[nodiscard]] double GetDelta() const noexcept {
        return delta_;
    }

    [[nodiscard]] Direction GetDirection() const noexcept {
        return direction_;
    }

public:
    AbcodVerifier();


    [[nodiscard]] bool Holds(Error error = 0.0) const noexcept {
        return error_ <= error;
    }


    [[nodiscard]] Error GetError() const noexcept {
        return error_;
    }


    [[nodiscard]] std::vector<size_t> const& GetOutliers() const noexcept {
        return outliers_;
    }

    [[nodiscard]] size_t GetLmbSize() const noexcept {
        return lmb_size_;
    }


    [[nodiscard]] Direction GetLmbDirection() const noexcept {
        return lmb_direction_;
    }


    [[nodiscard]] std::vector<Series> const& GetSegments() const noexcept {
        return segments_;
    }

    
    [[nodiscard]] std::vector<Series> const& GetSeries() const noexcept {
        return series_;
    }

    
    [[nodiscard]] int64_t GetGain() const noexcept {
        return gain_;
    }
};

}
