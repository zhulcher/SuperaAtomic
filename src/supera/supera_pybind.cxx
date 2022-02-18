
#include "supera/base/base_pybind.h"
//#include "supera/base/SuperaEvent.h"
/*
PYBIND11_MAKE_OPAQUE(std::vector<supera::ParticleLabel>);
PYBIND11_MAKE_OPAQUE(std::vector<supera::ImageLabel>);
*/
PYBIND11_MODULE(pysupera, m) {
	init_base(m);
}
