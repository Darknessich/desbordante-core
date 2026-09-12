#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "core/algorithms/algorithm.h"
#include "core/algorithms/od/abcod/band_od.h"
#include "core/algorithms/od/abcod/direction.h"
#include "core/config/tabular_data/input_table_type.h"

namespace algos::abcod {





class Abcod : public Algorithm {
private:
    config::InputTable input_table_;
    std::vector<std::vector<std::string>> rows_;
    size_t column_count_ = 0;

    double delta_ = 0;
    Direction direction_ = Direction::kAscending;
    size_t epsilon_ = 0;
    bool use_pieces_ = true;
    double min_coverage_ = 0.9;
    size_t min_series_size_ = 2;

    std::vector<BandOd> band_ods_;

    void RegisterOptions();

protected:
    void LoadDataInternal() override;
    void MakeExecuteOptsAvailable() override;
    void ExecuteInternal() override;
    void ResetState() override;

public:
    Abcod();

    [[nodiscard]] std::vector<BandOd> const& GetBandOds() const noexcept {
        return band_ods_;
    }
};

}  
