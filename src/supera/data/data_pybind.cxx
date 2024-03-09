#ifdef BUILD_PYTHON_BINDINGS

#include "data_pybind.h"

#include "pybind11/operators.h"
#include "pybind11/stl.h"

#include "supera/pybind_mkdoc.h"

#include "Event.h"
#include "ImageMeta3D.h"
#include "Particle.h"
#include "TriggerMeta.h"

void init_data(pybind11::module& m)
{
  using namespace pybind11::literals;

  // class from ImageMeta3D.h
  pybind11::class_<supera::ImageMeta3D, supera::BBox3D>(m, "ImageMeta3D", DOC(supera, ImageMeta3D))
      .def(pybind11::init<>(), DOC(supera, ImageMeta3D, ImageMeta3D))

      // overloaded operators
      .def(pybind11::self == pybind11::self)
      .def(pybind11::self != pybind11::self)

      // other methods
      .def("update", &supera::ImageMeta3D::update, DOC(supera, ImageMeta3D, update),
           "xnum"_a, "ynum"_a, "znum"_a)
      .def("set", &supera::ImageMeta3D::set, DOC(supera, ImageMeta3D, set),
           "xmin"_a, "ymin"_a, "zmin"_a, "xmax"_a, "ymax"_a, "zmax"_a, "xnum"_a, "ynum"_a, "znum"_a)
      .def("clear", &supera::ImageMeta3D::clear, DOC(supera, ImageMeta3D, clear))
      .def("valid", &supera::ImageMeta3D::valid, DOC(supera, ImageMeta3D, valid))
      .def("size", &supera::ImageMeta3D::size, DOC(supera, ImageMeta3D, size))
      .def("id", pybind11::overload_cast<const supera::Point3D&>(&supera::ImageMeta3D::id, pybind11::const_),
           DOC(supera, ImageMeta3D, id), "pt"_a)
      .def("id", pybind11::overload_cast<double, double, double>(&supera::ImageMeta3D::id, pybind11::const_),
           DOC(supera, ImageMeta3D, id), "x"_a, "y"_a, "z"_a)
      .def("index", &supera::ImageMeta3D::index, DOC(supera, ImageMeta3D, index),
           "i_x"_a, "i_y"_a, "i_z"_a)
      .def("shift", &supera::ImageMeta3D::shift, DOC(supera, ImageMeta3D, shift),
           "origin_id"_a, "shift_x"_a, "shift_y"_a, "shift_z"_a)
      .def("invalid_voxel_id", &supera::ImageMeta3D::invalid_voxel_id, DOC(supera, ImageMeta3D, invalid_voxel_id))
      .def("position", &supera::ImageMeta3D::position, DOC(supera, ImageMeta3D, position), "id"_a)
      .def("pos_x", &supera::ImageMeta3D::pos_x, DOC(supera, ImageMeta3D, pos_x), "id"_a)
      .def("pos_y", &supera::ImageMeta3D::pos_y, DOC(supera, ImageMeta3D, pos_y), "id"_a)
      .def("pos_z", &supera::ImageMeta3D::pos_z, DOC(supera, ImageMeta3D, pos_z), "id"_a)
      .def("num_voxel_x", &supera::ImageMeta3D::num_voxel_x, DOC(supera, ImageMeta3D, num_voxel_x))
      .def("num_voxel_y", &supera::ImageMeta3D::num_voxel_y, DOC(supera, ImageMeta3D, num_voxel_y))
      .def("num_voxel_z", &supera::ImageMeta3D::num_voxel_z, DOC(supera, ImageMeta3D, num_voxel_z))
      .def("size_voxel_x", &supera::ImageMeta3D::size_voxel_x, DOC(supera, ImageMeta3D, size_voxel_x))
      .def("size_voxel_y", &supera::ImageMeta3D::size_voxel_y, DOC(supera, ImageMeta3D, size_voxel_y))
      .def("size_voxel_z", &supera::ImageMeta3D::num_voxel_z, DOC(supera, ImageMeta3D, size_voxel_z))
      .def("dump", &supera::ImageMeta3D::dump, DOC(supera, ImageMeta3D, dump))
      .def("id_to_x_index", &supera::ImageMeta3D::id_to_x_index, DOC(supera, ImageMeta3D, id_to_x_index), "id"_a)
      .def("id_to_y_index", &supera::ImageMeta3D::id_to_y_index, DOC(supera, ImageMeta3D, id_to_y_index), "id"_a)
      .def("id_to_z_index", &supera::ImageMeta3D::id_to_z_index, DOC(supera, ImageMeta3D, id_to_z_index), "id"_a)
      .def("id_to_xyz_index", &supera::ImageMeta3D::id_to_xyz_index, DOC(supera, ImageMeta3D, id_to_xyz_index),
           "id"_a, "x"_a, "y"_a, "z"_a);

  // ----------------------------------------------------------------------

  // classes from Particle.h
  pybind11::class_<supera::Particle>(m, "Particle", DOC(supera, Particle))
      .def(pybind11::init<>(), DOC(supera, Particle, Particle))
      .def("p", &supera::Particle::p, DOC(supera, Particle, p))
      .def("p_final", &supera::Particle::p_final, DOC(supera, Particle, p_final))
      .def("dump", &supera::Particle::dump, DOC(supera, Particle, dump))
      .def_readwrite("id", &supera::Particle::id, DOC(supera, Particle, id))
      .def_readwrite("type", &supera::Particle::type, DOC(supera, Particle, type))
      .def_readwrite("shape", &supera::Particle::shape, DOC(supera, Particle, shape))
      .def_readwrite("trackid", &supera::Particle::trackid, DOC(supera, Particle, trackid))
      .def_readwrite("genid", &supera::Particle::genid, DOC(supera, Particle, trackid))
      .def_readwrite("pdg", &supera::Particle::pdg, DOC(supera, Particle, pdg))
      .def_readwrite("px", &supera::Particle::px, DOC(supera, Particle, px))
      .def_readwrite("py", &supera::Particle::py, DOC(supera, Particle, py))
      .def_readwrite("pz", &supera::Particle::pz, DOC(supera, Particle, pz))
      .def_readwrite("px_final", &supera::Particle::px_final, DOC(supera, Particle, px_final))
      .def_readwrite("py_final", &supera::Particle::py_final, DOC(supera, Particle, py_final))
      .def_readwrite("pz_final", &supera::Particle::pz_final, DOC(supera, Particle, pz_final))
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


  pybind11::class_<supera::ParticleInput>(m, "ParticleInput", DOC(supera, ParticleInput))
      .def(pybind11::init<>(), DOC(supera, ParticleInput, ParticleInput))
      .def_readwrite("part", &supera::ParticleInput::part, DOC(supera, ParticleInput, part))
      .def_readwrite("pcloud", &supera::ParticleInput::pcloud, DOC(supera, ParticleInput, pcloud))
      .def_readwrite("valid", &supera::ParticleInput::valid, DOC(supera, ParticleInput, valid));

  pybind11::class_<supera::ParticleLabel>(m, "ParticleLabel", DOC(supera, ParticleLabel))
      // constructors
      .def(pybind11::init<>(), DOC(supera, ParticleLabel, ParticleLabel))
      .def(pybind11::init<const supera::ParticleLabel&>(), DOC(supera, ParticleLabel, ParticleLabel, 2), "other"_a)
      // note: move constructor is not exposed here

      // note: don't explicitly expose assignment operator either

      // other methods
      .def("AddEDep", &supera::ParticleLabel::AddEDep, DOC(supera, ParticleLabel, AddEDep), "pt"_a)
      .def("SizeCheck", &supera::ParticleLabel::SizeCheck, DOC(supera, ParticleLabel, SizeCheck))
      .def("Size", &supera::ParticleLabel::Size, DOC(supera, ParticleLabel, Size))
      .def("Merge", &supera::ParticleLabel::Merge, DOC(supera, ParticleLabel, Merge), "child"_a, "verbose"_a)
      .def("dump", &supera::ParticleLabel::dump, DOC(supera, ParticleLabel, dump))

      // data members
      .def_readwrite("part", &supera::ParticleLabel::part, DOC(supera, ParticleLabel, part))
      .def_readwrite("valid", &supera::ParticleLabel::valid, DOC(supera, ParticleLabel, valid))
      .def_readwrite("valid", &supera::ParticleLabel::valid, DOC(supera, ParticleLabel, valid))
      .def_readwrite("energy", &supera::ParticleLabel::energy, DOC(supera, ParticleLabel, energy))
      .def_readwrite("dedx", &supera::ParticleLabel::dedx, DOC(supera, ParticleLabel, dedx))
      .def_readwrite("first_pt", &supera::ParticleLabel::first_pt, DOC(supera, ParticleLabel, first_pt))
      .def_readwrite("last_pt", &supera::ParticleLabel::last_pt, DOC(supera, ParticleLabel, last_pt));

  pybind11::class_<supera::EventOutput>(m, "EventOutput", DOC(supera, EventOutput))
      // this is slightly different than the C++ interface:
      // instead of a method called Particles() that retrieves the inner particles
      // with a const version (for reading only) and a non-const version (for modifying)
      // we map it to a Python 'property' that calls the 'const' version of Particles() when reading
      // and the non-const when setting (so you'll always have to get the particle list out first,
      // then reassign it after modifying the copy).
      // I'm not sure if there's a cleaner way to map it.
      .def_property("particles", pybind11::overload_cast<>(&supera::EventOutput::Particles, pybind11::const_),
                    pybind11::overload_cast<>(&supera::EventOutput::Particles),
                    "Particles contained in this Event.  If you need to modify them, "
                    "read them out first, modify the list, and after that, reassign back to the `particles` member")

      // note: assignment operators not explicitly bound

      // other methods
      .def("VoxelDeDxs", &supera::EventOutput::VoxelDeDxs, DOC(supera, EventOutput, VoxelDeDxs))
      .def("VoxelEnergies", &supera::EventOutput::VoxelEnergies, DOC(supera, EventOutput, VoxelEnergies))
      .def("VoxelLabels", &supera::EventOutput::VoxelLabels, DOC(supera, EventOutput, VoxelLabels), "semanticPriority"_a)
      .def("dump2cpp", &supera::EventOutput::dump2cpp, DOC(supera, EventOutput, dump2cpp), "instanceName"_a="evtOutput");
    
  // ----------------------------------------------------------------------

  // classes from Neutrino.h
  pybind11::class_<supera::Neutrino>(m, "Neutrino", DOC(supera, Neutrino))
      .def(pybind11::init<>(), DOC(supera, Neutrino, Neutrino))
      .def("p", &supera::Neutrino::p, DOC(supera, Neutrino, p))
      .def_readwrite("id", &supera::Neutrino::id, DOC(supera, Neutrino, id))
      .def_readwrite("genid", &supera::Neutrino::genid, DOC(supera, Neutrino, genid))
      .def_readwrite("track_id", &supera::Neutrino::track_id, DOC(supera, Neutrino, track_id))
      .def_readwrite("lepton_track_id", &supera::Neutrino::lepton_track_id, DOC(supera, Neutrino, lepton_track_id))
      .def_readwrite("pdg_code", &supera::Neutrino::pdg_code, DOC(supera, Neutrino, pdg_code))
      .def_readwrite("lepton_pdg_code", &supera::Neutrino::lepton_pdg_code, DOC(supera, Neutrino, lepton_pdg_code))
      .def_readwrite("px", &supera::Neutrino::px, DOC(supera, Neutrino, px))
      .def_readwrite("py", &supera::Neutrino::py, DOC(supera, Neutrino, py))
      .def_readwrite("pz", &supera::Neutrino::pz, DOC(supera, Neutrino, pz))
      .def_readwrite("lepton_p", &supera::Neutrino::lepton_p, DOC(supera, Neutrino, lepton_p))
      .def_readwrite("current_type", &supera::Neutrino::current_type, DOC(supera, Neutrino, current_type))
      .def_readwrite("interaction_mode", &supera::Neutrino::interaction_mode, DOC(supera, Neutrino, interaction_mode))
      .def_readwrite("interaction_type", &supera::Neutrino::interaction_type, DOC(supera, Neutrino, interaction_type))
      .def_readwrite("target", &supera::Neutrino::target, DOC(supera, Neutrino, target))
      .def_readwrite("nucleon", &supera::Neutrino::nucleon, DOC(supera, Neutrino, nucleon))
      .def_readwrite("quark", &supera::Neutrino::quark, DOC(supera, Neutrino, quark))
      .def_readwrite("hadronic_invariant_mass", &supera::Neutrino::hadronic_invariant_mass, DOC(supera, Neutrino, hadronic_invariant_mass))
      .def_readwrite("bjorken_x", &supera::Neutrino::bjorken_x, DOC(supera, Neutrino, bjorken_x))
      .def_readwrite("inelasticity", &supera::Neutrino::inelasticity, DOC(supera, Neutrino, inelasticity))
      .def_readwrite("momentum_transfer", &supera::Neutrino::momentum_transfer, DOC(supera, Neutrino, momentum_transfer))
      .def_readwrite("momentum_transfer_mag", &supera::Neutrino::momentum_transfer_mag, DOC(supera, Neutrino, momentum_transfer_mag))
      .def_readwrite("energy_transfer", &supera::Neutrino::energy_transfer, DOC(supera, Neutrino, energy_transfer))
      .def_readwrite("dist_travel", &supera::Neutrino::dist_travel, DOC(supera, Neutrino, dist_travel))
      .def_readwrite("energy_init", &supera::Neutrino::energy_init, DOC(supera, Neutrino, energy_init))
      .def_readwrite("energy_deposit", &supera::Neutrino::energy_deposit, DOC(supera, Neutrino, energy_deposit))
      .def_readwrite("creation_process", &supera::Neutrino::creation_process, DOC(supera, Neutrino, creation_process))
      .def_readwrite("theta", &supera::Neutrino::theta, DOC(supera, Neutrino, theta))
      .def_readwrite("num_voxels", &supera::Neutrino::num_voxels, DOC(supera, Neutrino, num_voxels))
}
#endif
