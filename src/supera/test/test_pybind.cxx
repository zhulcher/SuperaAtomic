
#include "test_pybind.h"

// needed to handle the std::map<> in the return value of TestEvents()
#include "pybind11/stl.h"

#include "supera/pybind_mkdoc.h"

#include "TestEvents.h"

void init_tests(pybind11::module& m)
{
  pybind11::class_<supera::test::TestEvent>(m, "TestEvent", DOC(supera, test, TestEvent))
      .def_readonly("input", &supera::test::TestEvent::input, DOC(supera, test, TestEvent, input))
      .def_readonly("output_labels", &supera::test::TestEvent::output_labels, DOC(supera, test, TestEvent, output_labels))
      .def_readonly("output_meta", &supera::test::TestEvent::output_meta,  DOC(supera, test, TestEvent, output_meta));

  m.def("TestEvents", &supera::test::TestEvents, DOC(supera, test, TestEvents));

  using namespace pybind11::literals;
  m.def("VerifyEventLabels", &supera::test::VerifyEventLabels, DOC(supera, test, VerifyEventLabels),
        "computedLabels"_a, "expectedLabels"_a);
  m.def("VerifyEventMeta", &supera::test::VerifyEventMeta, DOC(supera, test, VerifyEventMeta),
        "computedMeta"_a, "expectedMeta"_a);
}
