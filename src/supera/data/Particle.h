/**
 * \file Particle.h
 *
 * \ingroup base
 *
 * \brief Class def header for a class supera::Particle
 *
 * @author kazuhiro
 */

/** \addtogroup base
    @{*/
#ifndef __PARTICLE_H__
#define __PARTICLE_H__

#include <iostream>
#include <vector>
#include "supera/base/Point.h"
#include "supera/base/SuperaType.h"
#include "supera/base/Voxel.h"
namespace supera {

  /**
     \class Particle
     \brief Particle/Interaction-wise truth information data
  */
  class Particle{

  public:

    /// Default constructor
    Particle(supera::SemanticType_t shape=supera::kShapeUnknown)
      : id               (kINVALID_INSTANCEID)
      , shape            (shape)
      , trackid          (kINVALID_TRACKID)
      , pdg              (kINVALID_PDG)
      , px               (0.)
      , py               (0.)
      , pz               (0.)
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
      , parent_id(kINVALID_INSTANCEID)
      , children_id()
      , group_id(kINVALID_INSTANCEID)
      , interaction_id(kINVALID_INSTANCEID)
    {}

    /// Default destructor
    ~Particle(){}

    inline double p() const { return sqrt(pow(px,2)+pow(py,2)+pow(pz,2)); }
    std::string dump() const;

  public:

    InstanceID_t id; ///< "ID" of this particle in ParticleSet collection
    SemanticType_t shape;     ///< shows if it is (e+/e-/gamma) or other particle types
    TrackID_t      trackid;     ///< Geant4 track id
    PdgCode_t      pdg;         ///< PDG code
    double         px,py,pz;  ///< (x,y,z) component of particle's initial momentum
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

    TrackID_t   ancestor_trackid; ///< Geant4 track id of the ancestor particle
    PdgCode_t   ancestor_pdg;     ///< PDG code of the ancestor particle
    Vertex      ancestor_vtx;     ///< (x,y,z,t) of ancestor's vertex information
    std::string ancestor_process; ///< string identifier of the ancestor particle's creation process from Geant4

    std::string   parent_process; ///< string identifier of the parent particle's creation process from Geant4
    InstanceID_t  parent_id;      ///< "ID" of the parent particle in ParticleSet collection
    std::vector<supera::InstanceID_t> children_id; ///< "ID" of the children particles in ParticleSet collection
    InstanceID_t  group_id;       ///< "ID" to group multiple particles together (for clustering purpose)
    InstanceID_t  interaction_id; ///< "ID" to group multiple particles per interaction
  };


  class ParticleInput {
  public:

    ParticleInput() : valid(true), type(supera::kInvalidProcess) {}

    supera::Particle part;         ///< a particle information 
    std::vector<EDep> pcloud;      ///< 3D energy deposition information (raw info)
    bool valid;
    supera::ProcessType type;
  };

  typedef std::vector<ParticleInput> EventInput;

  class ParticleLabel {
  public:
    ParticleLabel(size_t num_planes=0);
    void AddEDep(const EDep& pt);
    void SizeCheck() const;
    size_t Size() const;
    void Merge(ParticleLabel& child,bool verbose=false);
    supera::SemanticType_t shape() const;

    supera::Particle part;         ///< a particle information 
    bool valid;                    ///< a state flag whether this particle should be ignored or not
    bool add_to_parent;            ///< a state flag whether this particle should be merged into its parent
    ProcessType type;              ///< type of this particle for ML reco chain
    std::vector<size_t> trackid_v; ///< track ID of descendent particles
    supera::VoxelSet energy;       ///< 3D voxels (energy deposition)
    supera::VoxelSet dedx;         ///< 3D voxels (dE/dX)
    EDep first_pt;                 ///< first energy deposition point (not voxel)
    EDep last_pt;                  ///< last energy deposition point (not voxel)
  };

  typedef std::vector<ParticleLabel> EventOutput;

}
#endif
/** @} */ // end of doxygen group