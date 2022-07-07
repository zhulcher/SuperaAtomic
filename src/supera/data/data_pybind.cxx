
#include "data_pybind.h"

#include "supera/pybind_mkdoc.h"

#include "Particle.h"

void init_data(pybind11::module& m)
{
  using namespace pybind11::literals;

  pybind11::class_<supera::Particle>(m, "Particle", DOC(supera, Particle))
      .def(pybind11::init<supera::SemanticType_t>(), DOC(supera, Particle, Particle), "shape"_a=supera::kShapeUnknown)
      .def("p", &supera::Particle::p, DOC(supera, Particle, p))
      .def("dump", &supera::Particle::dump, DOC(supera, Particle, dump))
      .def_readwrite("id", &supera::Particle::id, DOC(supera, Particle, id))
      .def_readwrite("shape", &supera::Particle::shape, DOC(supera, Particle, shape))
      .def_readwrite("trackid", &supera::Particle::trackid, DOC(supera, Particle, trackid))
      .def_readwrite("pdg", &supera::Particle::pdg, DOC(supera, Particle, pdg))
      .def_readwrite("px", &supera::Particle::px, DOC(supera, Particle, px))
      .def_readwrite("py", &supera::Particle::py, DOC(supera, Particle, py))
      .def_readwrite("pz", &supera::Particle::pz, DOC(supera, Particle, pz))
      .def_readwrite("vtx", &supera::Particle::vtx, DOC(supera, Particle, vtx))
      .def_readwrite("end_pt", &supera::Particle::end_pt, DOC(supera, Particle, end_pt))
      .def_readwrite("first_step", &supera::Particle::first_step, DOC(supera, Particle, first_step))
      .def_readwrite("last_step", &supera::Particle::last_step, DOC(supera, Particle, last_step))
      .def_readwrite("dist_travel", &supera::Particle::dist_travel, DOC(supera, Particle, dist_travel))
      .def_readwrite("energy_init", &supera::Particle::energy_init, DOC(supera, Particle, energy_init))
      .def_readwrite("energy_deposit", &supera::Particle::energy_deposit, DOC(supera, Particle, energy_deposit))
      .def_readwrite("process", &supera::Particle::process, DOC(supera, Particle, process))
      .def_readwrite("parent_trackid", &supera::Particle::parent_trackid, DOC(supera, Particle, parent_trackid))
      .def_readwrite("parent_pdg", &supera::Particle::parent_pdg, DOC(supera, Particle, parent_pdg))
      .def_readwrite("parent_vtx", &supera::Particle::parent_vtx, DOC(supera, Particle, parent_vtx))
      .def_readwrite("ancestor_trackid", &supera::Particle::ancestor_trackid, DOC(supera, Particle, ancestor_trackid))
      .def_readwrite("ancestor_pdg", &supera::Particle::ancestor_pdg, DOC(supera, Particle, ancestor_pdg))
      .def_readwrite("ancestor_vtx", &supera::Particle::ancestor_vtx, DOC(supera, Particle, ancestor_vtx))
      .def_readwrite("ancestor_process", &supera::Particle::ancestor_process, DOC(supera, Particle, ancestor_process))
      .def_readwrite("parent_process", &supera::Particle::parent_process, DOC(supera, Particle, parent_process))
      .def_readwrite("parent_id", &supera::Particle::parent_id, DOC(supera, Particle, parent_id))
      .def_readwrite("children_id", &supera::Particle::children_id, DOC(supera, Particle, children_id))
      .def_readwrite("group_id", &supera::Particle::group_id, DOC(supera, Particle, group_id))
      .def_readwrite("interaction_id", &supera::Particle::interaction_id, DOC(supera, Particle, interaction_id));


}
