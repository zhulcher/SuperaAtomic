#include "supera/base/base_pybind.h"
#include "supera/process/process_pybind.h"
#include "supera/test/test_pybind.h"
//#include "supera/base/SuperaEvent.h"
/*
PYBIND11_MAKE_OPAQUE(std::vector<supera::ParticleLabel>);
PYBIND11_MAKE_OPAQUE(std::vector<supera::ImageLabel>);
*/
PYBIND11_MODULE(pysupera, m) {
	init_base(m);
  init_process(m);

  // users won't normally interact with the testing stuff,
  // so we put it all in a submodule
  auto submodule = m.def_submodule("test", "SuperaAtomic testing machinery submodule");
  init_tests(submodule);
}
