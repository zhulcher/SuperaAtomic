/**
 * \file Particle.h
 *
 * \ingroup base
 *
 * \brief Classes for information about true particles
 *
 * @author kazuhiro
 */

/** \addtogroup base
    @{*/
#ifndef __SUPERA_PARTICLE_H__
#define __SUPERA_PARTICLE_H__

#include <array>
#include <iostream>
#include <vector>

#include "supera/base/Point.h"
#include "supera/base/SuperaType.h"
#include "supera/base/Voxel.h"

namespace supera {

  /**
     \class Particle
     \brief Particle/Interaction-wise truth information data.  Corresponds to a GEANT4 track.
  */
  class Particle{

  public:

    /// Default constructor
    Particle()
      : id               (kINVALID_INSTANCEID)
      , type             (kInvalidProcess)
      , shape            (kShapeUnknown)
      , trackid          (kINVALID_TRACKID)
      , genid            (kINVALID_TRACKID)
      , pdg              (kINVALID_PDG)
      , px               (0.)
      , py               (0.)
      , pz               (0.)
      , end_px         (kINVALID_DOUBLE)
      , end_py         (kINVALID_DOUBLE)
      , end_pz         (kINVALID_DOUBLE)
      , dist_travel      (-1)
      , energy_init      (0.)
      , energy_deposit   (0.)
      , process          ("")
      , parent_trackid   (kINVALID_TRACKID)
      , parent_pdg       (kINVALID_PDG)
      , ancestor_trackid (kINVALID_TRACKID)
      , ancestor_pdg     (kINVALID_PDG)
      , ancestor_process ("")
      , parent_process   ("")
      , parent_id        (kINVALID_INSTANCEID)
      , ancestor_id      (kINVALID_INSTANCEID)
      , children_id      ()
      , group_id         (kINVALID_INSTANCEID)
      , interaction_id   (kINVALID_INSTANCEID)
    {}

    /// Default destructor
    ~Particle() = default;

    bool operator==(const Particle & rhs) const;
    bool operator!=(const Particle & rhs) const { return !(*this == rhs); }

    inline double p() const { return sqrt(pow(px,2)+pow(py,2)+pow(pz,2)); }
    inline double end_p() const { return sqrt(pow(end_px, 2) + pow(end_py, 2) + pow(end_pz, 2)); }
    std::string dump() const;

    /// Dump this Particle into C++ code that could rebuild it.
    std::string dump2cpp(const std::string &instanceName = "part") const;

  public:

    InstanceID_t id;            ///< "ID" of this particle in ParticleSet collection
    ProcessType_t  type;        ///< Creation process type
    SemanticType_t shape;       ///< Semantic type info
    TrackID_t      trackid;     ///< Geant4 track id
    TrackID_t      genid;       ///< Original generator ID, if different than Geant4 (e.g.: GENIE particle ID)
    PdgCode_t      pdg;         ///< PDG code
    double         px,py,pz;    ///< (x,y,z) component of particle's initial momentum
    double         end_px, end_py, end_pz;          ///< (x,y,z) component of particle's final momentum
    Vertex         vtx;         ///< (x,y,z,t) of particle's vertex information
    Vertex         end_pt;      ///< (x,y,z,t) at which particle disappeared from G4WorldVolume
    Vertex         first_step;  ///< (x,y,z,t) of the first energy deposition point in the detector
    Vertex         last_step;   ///< (x,y,z,t) of the last energy deposition point in the detector
    double         dist_travel; ///< filled only if MCTrack origin: distance measured along the trajectory
    double         energy_init; ///< initial energy of the particle
    double         energy_deposit; ///< deposited energy of the particle in the detector
    std::string    process;     ///< string identifier of the particle's creation process from Geant4

    TrackID_t  parent_trackid; ///< Geant4 track id of the parent particle
    PdgCode_t  parent_pdg;     ///< PDG code of the parent particle
    Vertex     parent_vtx;     ///< (x,y,z,t) of parent's vertex information

    TrackID_t   ancestor_trackid; ///< Geant4 track id of the ancestor particle (*primary* particle that sits at the top of the hierarchy containing this particle)
    PdgCode_t   ancestor_pdg;     ///< PDG code of the ancestor particle
    Vertex      ancestor_vtx;     ///< (x,y,z,t) of ancestor's vertex information
    std::string ancestor_process; ///< string identifier of the ancestor particle's creation process from Geant4

    std::string   parent_process; ///< string identifier of the parent particle's creation process from Geant4
    InstanceID_t  parent_id;      ///< "ID" of the parent particle in ParticleSet collection
    InstanceID_t  ancestor_id;    ///< "ID" of the ancestor particle in ParticleSet collection
    std::vector<supera::InstanceID_t> children_id; ///< "ID" of the children particles in ParticleSet collection
    InstanceID_t  group_id;       ///< "ID" to group multiple particles together (for clustering purpose)
    InstanceID_t  interaction_id; ///< "ID" to group multiple particles per interaction
  };

  /// ProcessType => SemanticType conversion
  /*
  SemanticType_t Process2Semantic(const PdgCode_t& pdg,
    const ProcessType_t& proc)
  {
    SemanticType_t res(kShapeUnknown);
    switch(proc) {
      case kPhoton:
      case kCompton:
      case kConversion:
      case kOtherShower:
        res = SemanticType_t::kShapeShower;
        break;
      case kDelta:
        res = SemanticType_t::kShapeDelta;
        break;
      case k
      case kDecay:
        if(std::abs(pdg)==11){
          res = SemanticType_t::kShapeMichel;
          break;
        }
      case kTrack:
      case kNeutron:
        res = SemanticType_t::kShapeTrack;
        break;
    }
    return res;
  }
  */

  /**
     \class ParticleInput
     \brief True particle information from upstream: particle quantities (kinematics, etc.) & associated energy deposits
  */
  class ParticleInput {
  public:

    ParticleInput() : valid(true) {}

    std::string dump2cpp(const std::string & instanceName = "partInput") const;

    supera::Particle part;    ///< a particle information
    std::vector<EDep> pcloud; ///< 3D energy deposition information per particle
    bool valid;
  };


  /**
     \class ParticleLabel
     \brief Output of the Supera algorithm for a true "top-level" particle (after regrouping)
  */
  class ParticleLabel
  {
    public:
      ParticleLabel();

      ParticleLabel(const ParticleLabel& other) = default;

      ParticleLabel(ParticleLabel&& other) = default;

      ParticleLabel& operator=(const ParticleLabel& other) = default;

      bool operator==(const ParticleLabel& rhs) const;

      bool operator!=(const ParticleLabel& rhs) const
      { return !(*this == rhs); }

      void UpdateFirstPoint(const EDep& pt);

      void UpdateLastPoint(const EDep& pt);

      void SizeCheck() const;

      size_t Size() const;

      void Merge(ParticleLabel& child, bool verbose = false);
      //supera::SemanticType_t shape() const;

      std::string dump() const;

      std::string dump2cpp(const std::string& instanceName = "partLabel") const;

      supera::Particle part;            ///< a particle information
      bool valid;                       ///< a state flag whether this particle should be ignored or not
      std::vector <TrackID_t> merged_v;  ///< track ID of descendent particles that are merged
      std::vector <TrackID_t> parent_trackid_v; ///< track ID of parent particles in the history
      TrackID_t merge_id;               ///< a track ID of the particle to which this one is merged
      supera::VoxelSet energy;          ///< 3D voxels (energy deposition)
      supera::VoxelSet dedx;            ///< 3D voxels (dE/dX)
      EDep first_pt;                    ///< first energy deposition point (not voxel)
      EDep last_pt;                     ///< last energy deposition point (not voxel)
  };

}
#endif
/** @} */ // end of doxygen group
