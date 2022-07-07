
#include "test_pybind.h"
#include "TestEvents.h"

void init_tests(pybind11::module& m)
{
  pybind11::class_<supera::test::TestEvent>(m, "TestEvent", "Collection of input & output information for testing machinery")
      .def_readonly("input", &supera::test::TestEvent::input, "Input information as vector of supera::ParticleInputs")
      .def_readonly("output_labels", &supera::test::TestEvent::output_labels, "Output particle labels as supera::EventOutput")
      .def_readonly("output_meta", &supera::test::TestEvent::output_meta, "Output metadata as supera::ImageMeta3D");

//  m.attr("kTestEvents")   = supera::test::kTestEvents;

  using namespace pybind11::literals;
  m.def("VerifyEventLabels", &supera::test::VerifyEventLabels, "Verify computed labels against expected labels",
        "computedLabels"_a, "expectedLabels"_a);
  m.def("VerifyEventMeta", &supera::test::VerifyEventMeta, "Verify computed metadata against expected metadata",
        "computedMeta"_a, "expectedMeta"_a);
}
