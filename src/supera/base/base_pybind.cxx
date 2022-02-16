
#include "base_pybind.h"
#include "SuperaEvent.h"
#include "SuperaType.h"

void init_base(pybind11::module m)
{

  pybind11::class_<supera::ParticleGroup>(m, "ParticleGroup")
  .def(pybind11::init<size_t&>())
  .def(pybind11::init<size_t&>(), pybind11::arg("num_planes")=0)
  ;

  pybind11::class_<supera::Event>(m, "SuperaEvent")
  ;

  m.attr("kSUPERA_INVALID_SIZE")   = supera::kINVALID_SIZE;
  m.attr("kSUPERA_INVALID_DOUBLE") = supera::kINVALID_DOUBLE;

}
