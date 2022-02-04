
#ifndef __BASE_PYBIND_H__
#define __BASE_PYBIND_H__

#ifdef BUILD_PYTHON_BINDINGS
  #include <pybind11/pybind11.h>
  __attribute__ ((visibility ("default"))) void init_base(pybind11::module m);
#endif


#endif 