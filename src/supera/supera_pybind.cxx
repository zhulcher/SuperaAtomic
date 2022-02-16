
#include "supera/base/base_pybind.h"
#include "supera/base/SuperaEvent.h"

PYBIND11_MAKE_OPAQUE(std::vector<supera::ParticleGroup>);
PYBIND11_MAKE_OPAQUE(std::vector<supera::Event>);

PYBIND11_MODULE(pysupera, m) {
	init_base(m);
}
