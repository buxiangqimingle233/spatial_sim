#include "spatial_chip.hpp"
#include "pybind11/pybind11.h"
#include "pybind11/stl.h"

namespace py = pybind11;

PYBIND11_MODULE(simulator, m) {
    m.doc() = R"pbdoc(
        Pybind11 simulator plugin
        -----------------------

        .. currentmodule:: simulator

        .. autosummary::
           :toctree: _generate

           step
           is_finished
           is_deadlock
           reset
           display_stats
    )pbdoc";

    py::class_<spatial::SpatialChip>(m, "SpatialChip")
        .def(py::init<const std::string &>())
        .def("run", &spatial::SpatialChip::run)
        .def("is_finished", &spatial::SpatialChip::task_finished)
        .def("is_deadlock", &spatial::SpatialChip::check_deadlock)
        .def("reset", &spatial::SpatialChip::reset)
        .def("compute_cycles", &spatial::SpatialChip::compute_cycles)
        .def("communicate_cycles", &spatial::SpatialChip::communicate_cycles)
        .def("router_conflict_factors", &spatial::SpatialChip::router_conflict_factors);
}