#ifndef SUPERA_PROCESS_PYBIND_H
#define SUPERA_PROCESS_PYBIND_H

#ifdef BUILD_PYTHON_BINDINGS
  #include <pybind11/pybind11.h>
  __attribute__ ((visibility ("default"))) void init_process(pybind11::module& m);
#endif

#endif //SUPERA_PROCESS_PYBIND_H
