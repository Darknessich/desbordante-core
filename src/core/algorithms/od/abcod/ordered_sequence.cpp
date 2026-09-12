#include "core/algorithms/od/abcod/ordered_sequence.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <numeric>
#include <stdexcept>

namespace algos::abcod {

namespace {

std::string Trim(std::string const& cell) {
    auto is_space = [](unsigned char c) { return std::isspace(c); };
    auto begin = std::find_if_not(cell.begin(), cell.end(), is_space);
    auto end = std::find_if_not(cell.rbegin(), cell.rend(), is_space).base();
    return begin < end ? std::string(begin, end) : std::string{};
}


struct LhsColumn {
    size_t index;
    bool numeric;
    std::vector<double> numbers;
};

}

bool IsNullCell(std::string const& cell) {
    std::string trimmed = Trim(cell);
    std::transform(trimmed.begin(), trimmed.end(), trimmed.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return trimmed.empty() || trimmed == "null" || trimmed == "nan";
}

std::optional<double> ParseNumber(std::string const& cell) {
    std::string trimmed = Trim(cell);
    if (trimmed.empty()) return std::nullopt;
    size_t parsed = 0;
    double value = 0;
    try {
        value = std::stod(trimmed, &parsed);
    } catch (std::exception const&) {
        return std::nullopt;
    }
    if (parsed != trimmed.size() || !std::isfinite(value)) return std::nullopt;
    return value;
}

bool IsNumericColumn(std::vector<std::vector<std::string>> const& rows, size_t column) {
    return std::ranges::all_of(rows, [column](std::vector<std::string> const& row) {
        return column >= row.size() || IsNullCell(row[column]) ||
               ParseNumber(row[column]).has_value();
    });
}

ParsedColumn ParseColumn(std::vector<std::vector<std::string>> const& rows, size_t column) {
    ParsedColumn parsed;
    parsed.values.resize(rows.size());
    parsed.is_null.resize(rows.size());
    for (size_t row = 0; row < rows.size(); ++row) {
        std::string const& cell = rows[row][column];
        if (IsNullCell(cell)) {
            parsed.is_null[row] = true;
            continue;
        }
        std::optional<double> number = ParseNumber(cell);
        if (!number) {
            parsed.numeric = false;
            continue;
        }
        parsed.values[row] = *number;
    }
    return parsed;
}

std::vector<size_t> OrderRows(std::vector<std::vector<std::string>> const& rows,
                              config::IndicesType const& lhs_indices) {
    std::vector<LhsColumn> lhs;
    lhs.reserve(lhs_indices.size());
    for (config::IndexType index : lhs_indices) {
        ParsedColumn parsed = ParseColumn(rows, index);
        lhs.push_back({index, parsed.numeric, std::move(parsed.values)});
    }

    std::vector<size_t> order;
    order.reserve(rows.size());
    for (size_t row = 0; row < rows.size(); ++row) {
        bool has_null = std::ranges::any_of(lhs_indices, [&rows, row](config::IndexType index) {
            return IsNullCell(rows[row][index]);
        });
        if (!has_null) order.push_back(row);
    }

    std::ranges::stable_sort(order, [&lhs, &rows](size_t a, size_t b) {
        for (LhsColumn const& column : lhs) {
            if (column.numeric) {
                if (column.numbers[a] != column.numbers[b]) {
                    return column.numbers[a] < column.numbers[b];
                }
            } else {
                int cmp = rows[a][column.index].compare(rows[b][column.index]);
                if (cmp != 0) return cmp < 0;
            }
        }
        return false;
    });
    return order;
}

OrderedSequence MakeOrderedSequence(std::vector<size_t> const& order, ParsedColumn const& rhs) {
    OrderedSequence sequence;
    sequence.row_ids = order;
    sequence.values.resize(order.size());
    sequence.is_null.resize(order.size());
    for (size_t pos = 0; pos < order.size(); ++pos) {
        size_t const row = order[pos];
        if (rhs.is_null[row]) {
            sequence.is_null[pos] = true;
            continue;
        }
        sequence.values[pos] = rhs.values[row];
        ++sequence.non_null_count;
    }
    return sequence;
}

OrderedSequence BuildOrderedSequence(std::vector<std::vector<std::string>> const& rows,
                                     config::IndicesType const& lhs_indices,
                                     config::IndexType rhs_index) {
    ParsedColumn const rhs = ParseColumn(rows, rhs_index);
    if (!rhs.numeric) {
        throw std::invalid_argument("Right-hand side column " + std::to_string(rhs_index) +
                                    " must be numeric");
    }
    return MakeOrderedSequence(OrderRows(rows, lhs_indices), rhs);
}

}  
