#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "core/config/indices/type.h"

namespace algos::abcod {



struct OrderedSequence {
    
    std::vector<size_t> row_ids;
    
    std::vector<double> values;
    std::vector<bool> is_null;
    size_t non_null_count = 0;

    [[nodiscard]] size_t Size() const noexcept {
        return row_ids.size();
    }
};


bool IsNullCell(std::string const& cell);


std::optional<double> ParseNumber(std::string const& cell);


bool IsNumericColumn(std::vector<std::vector<std::string>> const& rows, size_t column);




OrderedSequence BuildOrderedSequence(std::vector<std::vector<std::string>> const& rows,
                                     config::IndicesType const& lhs_indices,
                                     config::IndexType rhs_index);

}  
