#include "python_bindings/abcod/bind_abcod.h"

#include <pybind11/pybind11.h>

#include <pybind11/stl.h>

#include "core/algorithms/od/abcod/abcod.h"
#include "core/algorithms/od/abcod/band_od.h"
#include "python_bindings/py_util/bind_primitive.h"

namespace {
namespace py = pybind11;
}  

namespace python_bindings {

void BindAbcod(py::module_& main_module) {
    using namespace algos::abcod;

    auto abcod_module = main_module.def_submodule("abcod");

    py::class_<BandOd>(abcod_module, "BandOD")
            .def_readonly("lhs", &BandOd::lhs)
            .def_readonly("rhs", &BandOd::rhs)
            .def_readonly("series", &BandOd::series)
            .def_readonly("gain", &BandOd::gain)
            .def_readonly("coverage", &BandOd::coverage)
            .def("__str__", &BandOd::ToString)
            .def("__repr__", &BandOd::ToString);

    BindPrimitiveNoBase<Abcod>(abcod_module, "Abcod").def("get_band_ods", &Abcod::GetBandOds);

    main_module.attr("abcod") = abcod_module;
}

}  
