#ifndef SUPERA_TEST_PYBIND_H
#define SUPERA_TEST_PYBIND_H

#ifdef BUILD_PYTHON_BINDINGS
  #include <pybind11/pybind11.h>
  __attribute__ ((visibility ("default"))) void init_tests(pybind11::module m);
#endif

#endif //SUPERA_TEST_PYBIND_H
