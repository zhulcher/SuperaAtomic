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
#include "Point.h"
#include "SuperaTypes.h"
namespace larcv {

  /**
     \class Particle
     \brief Particle/Interaction-wise truth information data
  */
  class Particle{

  public:

    /// Default constructor
    Particle(larcv::ShapeType_t shape=larcv::kShapeUnknown)
      : id         (kINVALID_INDEX)
      , shape      (shape)
      , trackid          (kINVALID_UINT)
      , pdg              (0)
      , px               (0.)
      , py               (0.)
      , pz               (0.)
      , dist_travel      (-1)
      , energy_init      (0.)
      , energy_deposit   (0.)
      , process          ("")
      , parent_trackid   (kINVALID_UINT)
      , parent_pdg       (0)
      , ancestor_trackid (kINVALID_UINT)
      , ancestor_pdg     (0)
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
    SemanticType_t semantic;       ///< shows if it is (e+/e-/gamma) or other particle types
    unsigned int trackid;     ///< Geant4 track id
    int          pdg;         ///< PDG code
    double       px,_py,_pz;  ///< (x,y,z) component of particle's initial momentum
    Vertex       vtx;         ///< (x,y,z,t) of particle's vertex information
    Vertex       end_pt;      ///< (x,y,z,t) at which particle disappeared from G4WorldVolume
    Vertex       first_step;  ///< (x,y,z,t) of the first energy deposition point in the detector
    Vertex       last_step;   ///< (x,y,z,t) of the last energy deposition point in the detector
    double       dist_travel; ///< filled only if MCTrack origin: distance measured along the trajectory
    double       energy_init; ///< initial energy of the particle
    double       energy_deposit; ///< deposited energy of the particle in the detector
    std::string  process;     ///< string identifier of the particle's creation process from Geant4

    unsigned int parent_trackid; ///< Geant4 track id of the parent particle
    int          parent_pdg;     ///< PDG code of the parent particle
    Vertex       parent_vtx;     ///< (x,y,z,t) of parent's vertex information

    unsigned int ancestor_trackid; ///< Geant4 track id of the ancestor particle
    int          ancestor_pdg;     ///< PDG code of the ancestor particle
    Vertex       ancestor_vtx;     ///< (x,y,z,t) of ancestor's vertex information
    std::string  ancestor_process; ///< string identifier of the ancestor particle's creation process from Geant4

    std::string  parent_process; ///< string identifier of the parent particle's creation process from Geant4
    InstanceID_t parent_id;      ///< "ID" of the parent particle in ParticleSet collection
    std::vector<larcv::InstanceID_t> _children_id; ///< "ID" of the children particles in ParticleSet collection
    InstanceID_t group_id;       ///< "ID" to group multiple particles together (for clustering purpose)
    InstanceID_t interaction_id; ///< "ID" to group multiple particles per interaction
  };

}
#endif
/** @} */ // end of doxygen group