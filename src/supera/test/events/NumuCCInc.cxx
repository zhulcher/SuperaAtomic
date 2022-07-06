#include "supera/test/TestEvents.h"

namespace supera
{
  namespace test
  {
    TestEvent NumuCCIncEvt()
    {
      supera::test::TestEvent ret;

      supera::EventInput input;

      supera::ParticleInput particleInput;
      particleInput.type = kPrimary;

      supera::EDep edep;
      edep.x = 0.;
      edep.y = 0.;
      edep.z = 0.;
      edep.t = 0.;
      edep.e = 0.;
      edep.dedx = 0.;
      particleInput.pcloud.emplace_back(std::move(edep));

      particleInput.part.id = 0; ///< "ID" of this particle in ParticleSet collection
      particleInput.part.shape = kShapeTrack;     ///< shows if it is (e+/e-/gamma) or other particle types
      particleInput.part.trackid = 0;     ///< Geant4 track id
      particleInput.part.pdg = 0;         ///< PDG code
      std::tie(particleInput.part.px, particleInput.part.py, particleInput.part.pz) = std::make_tuple(0., 0.,
                                                                                                      0.);  ///< (x,y,z) component of particle's initial momentum
      particleInput.part.vtx = {0., 0., 0., 0.};         ///< (x,y,z,t) of particle's vertex information
      particleInput.part.end_pt = {0., 0., 0., 0.};      ///< (x,y,z,t) at which particle disappeared from G4WorldVolume
      particleInput.part.first_step = {0., 0., 0.,
                                       0.};  ///< (x,y,z,t) of the first energy deposition point in the detector
      particleInput.part.last_step = {0., 0., 0.,
                                      0.};   ///< (x,y,z,t) of the last energy deposition point in the detector
      particleInput.part.dist_travel = 0; ///< filled only if MCTrack origin: distance measured along the trajectory
      particleInput.part.energy_init = 0; ///< initial energy of the particle
      particleInput.part.energy_deposit = 0; ///< deposited energy of the particle in the detector
      particleInput.part.process = "primary";     ///< string identifier of the particle's creation process from Geant4

      particleInput.part.parent_trackid = kINVALID_TRACKID; ///< Geant4 track id of the parent particle
      particleInput.part.parent_pdg = kINVALID_PDG;     ///< PDG code of the parent particle
      particleInput.part.parent_vtx = {0., 0., 0., -1.};     ///< (x,y,z,t) of parent's vertex information

      // this particle is at the top so it's its own ancestor
      particleInput.part.ancestor_trackid = particleInput.part.trackid; ///< Geant4 track id of the ancestor particle (*primary* particle that sits at the top of the hierarchy containing this particle)
      particleInput.part.ancestor_pdg = particleInput.part.pdg;     ///< PDG code of the ancestor particle
      particleInput.part.ancestor_vtx = particleInput.part.vtx;     ///< (x,y,z,t) of ancestor's vertex information
      particleInput.part.ancestor_process = particleInput.part.process; ///< string identifier of the ancestor particle's creation process from Geant4

      particleInput.part.parent_process = ""; ///< string identifier of the parent particle's creation process from Geant4
      particleInput.part.parent_id = kINVALID_INSTANCEID;      ///< "ID" of the parent particle in ParticleSet collection
      particleInput.part.children_id = {}; ///< "ID" of the children particles in ParticleSet collection
      particleInput.part.group_id = 0;       ///< "ID" to group multiple particles together (for clustering purpose)
      particleInput.part.interaction_id = 0; ///< "ID" to group multiple particles per interaction

      input.emplace_back(std::move(particleInput));

      ret.input = std::move(input);

      supera::ImageMeta3D meta;
      ret.output_meta = std::move(meta);

      supera::EventOutput output;
      ret.output_labels = std::move(output);

      return ret;
    }
  }
}
