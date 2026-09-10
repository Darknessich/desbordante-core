#include "python_bindings/abcod/bind_abcod_verification.h"

#include <pybind11/pybind11.h>

#include <string>

#include <pybind11/stl.h>

#include "core/algorithms/od/abcod/abcod_verifier.h"
#include "core/algorithms/od/abcod/series.h"
#include "python_bindings/py_util/bind_primitive.h"

namespace {
namespace py = pybind11;

std::string DirectionName(algos::abcod::Direction direction) {
    return direction == algos::abcod::Direction::kDescending ? "descending" : "ascending";
}
}  

namespace python_bindings {

void BindAbcodVerification(py::module_& main_module) {
    using namespace algos::abcod;

    auto abcod_verification_module = main_module.def_submodule("abcod_verification");

    py::class_<Series>(abcod_verification_module, "Series")
            .def_readonly("begin", &Series::begin)
            .def_readonly("end", &Series::end)
            .def_property_readonly(
                    "direction",
                    [](Series const& series) { return DirectionName(series.direction); })
            .def_readonly("non_null_count", &Series::non_null_count)
            .def_readonly("lmb_size", &Series::lmb_size)
            .def_readonly("cost", &Series::cost)
            .def_readonly("rows", &Series::rows)
            .def_readonly("outliers", &Series::outliers)
            .def_property_readonly("gain", &Series::Gain)
            .def("__repr__", [](Series const& series) {
                return "Series(positions=[" + std::to_string(series.begin) + ", " +
                       std::to_string(series.end) + "), " + DirectionName(series.direction) +
                       ", lmb=" + std::to_string(series.lmb_size) + "/" +
                       std::to_string(series.non_null_count) +
                       ", cost=" + std::to_string(series.cost) + ")";
            });

    BindPrimitiveNoBase<AbcodVerifier>(abcod_verification_module, "AbcodVerifier")
            .def("holds", &AbcodVerifier::Holds, py::arg("error") = 0.0)
            .def("get_error", &AbcodVerifier::GetError)
            .def("get_outliers", &AbcodVerifier::GetOutliers)
            .def("get_lmb_size", &AbcodVerifier::GetLmbSize)
            .def("get_lmb_direction",
                 [](AbcodVerifier const& verifier) {
                     return DirectionName(verifier.GetLmbDirection());
                 })
            .def("get_segments", &AbcodVerifier::GetSegments)
            .def("get_series", &AbcodVerifier::GetSeries)
            .def("get_gain", &AbcodVerifier::GetGain);

    main_module.attr("abcod_verification") = abcod_verification_module;
}

}  
