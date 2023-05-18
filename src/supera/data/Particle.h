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
    std::string dump() const;

    /// Dump this Particle into C++ code that could rebuild it.
    std::string dump2cpp(const std::string &instanceName = "part") const;

  public:

    InstanceID_t id;            ///< "ID" of this particle in ParticleSet collection
    ProcessType_t  type;        ///< Creation process type
    SemanticType_t shape;       ///< Semantic type info
    TrackID_t      trackid;     ///< Geant4 track id
    PdgCode_t      pdg;         ///< PDG code
    double         px,py,pz;    ///< (x,y,z) component of particle's initial momentum
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

  class ParticleInput {
  public:

    ParticleInput() : valid(true) {}

    std::string dump2cpp(const std::string & instanceName = "partInput") const;

    supera::Particle part;    ///< a particle information
    std::vector<EDep> pcloud; ///< 3D energy deposition information per particle
    bool valid;
  };

  class EventInput : public std::vector<ParticleInput> {
  public:
    /// 3D energy depositions unassociated to any input particle
    std::vector<EDep> unassociated_edeps;
  };

  class ParticleLabel {
  public:
    ParticleLabel();
    ParticleLabel(const ParticleLabel& other) = default;
    ParticleLabel(ParticleLabel&& other) = default;

    ParticleLabel & operator=(const ParticleLabel& other) = default;

    bool operator==(const ParticleLabel & rhs) const;
    bool operator!=(const ParticleLabel & rhs) const { return !(*this == rhs); }

    void UpdateFirstPoint(const EDep& pt);
    void UpdateLastPoint(const EDep& pt);
    void SizeCheck() const;
    size_t Size() const;
    void Merge(ParticleLabel& child,bool verbose=false);
    //supera::SemanticType_t shape() const;

    std::string dump() const;
    std::string dump2cpp(const std::string & instanceName = "partLabel") const;

    supera::Particle part;            ///< a particle information
    bool valid;                       ///< a state flag whether this particle should be ignored or not
    std::vector<TrackID_t> merged_v;  ///< track ID of descendent particles that are merged
    std::vector<TrackID_t> parent_trackid_v; ///< track ID of parent particles in the history
    TrackID_t merge_id;               ///< a track ID of the particle to which this one is merged
    supera::VoxelSet energy;          ///< 3D voxels (energy deposition)
    supera::VoxelSet dedx;            ///< 3D voxels (dE/dX)
    EDep first_pt;                    ///< first energy deposition point (not voxel)
    EDep last_pt;                     ///< last energy deposition point (not voxel)
  };

  /// Class to store the labeled particles of an event & their associated hit voxels (if any).
  class EventOutput
  {
    private:
      /// Helper to keep track of which parts of the storage need to be rebuilt after changes
      enum class DIRTY_FLAG: unsigned short { kLabel, kEnergy, kDeDx };

    public:
      /// Get the list of particle labels, const version.  If you need to change them see the other version of \ref Particles()
      const std::vector<ParticleLabel> & Particles() const { return _particles; }
      /// Get the list of particle labels, non-const version.  This implementation is quite lazy but I doubt we'll need anything more sophisticated.
            std::vector<ParticleLabel> & Particles()       { _dirty.fill(true); return _particles; }

      /// \brief Get the dE/dx for all voxels with energy deposition in them.
      /// N.b.: For voxels that have multiple contributing particles, the dE/dx computed is the energy-weighted mean dE/dx.
      /// \return \ref VoxelSet with energy-weighted mean dE/dx for each voxel with energy in it
      const supera::VoxelSet & VoxelDeDxs() const;

      /// \brief Get the deposited energies for all voxels with energy deposition in them.
      /// \return \ref VoxelSet with total deposited energy for each voxel with energy in it
      const supera::VoxelSet & VoxelEnergies() const;

      /// \brief Get the labels for the voxels.
      /// \param semanticPriority Ranking for which label "wins" when multiple particles of different semantic type contribute to the same voxel: highest priority first.
      /// \return \ref VoxelSet with each entry corresponding to the semantic label for each voxel with energy in it
      const supera::VoxelSet & VoxelLabels(const std::vector<supera::SemanticType_t> & semanticPriority) const;

      EventOutput & operator=(const std::vector<ParticleLabel> & other) { Particles() = other; return *this; }
      EventOutput & operator=(std::vector<ParticleLabel> && other) { Particles() = std::move(other); return *this; }

      /// Is this EventOutput the same as \a rhs?
      bool operator==(const EventOutput & rhs) const;

      /// Dump this \ref EventOutput to a string of C++ code that can be used to reproduce it
      std::string dump2cpp(const std::string &instanceName = "evtOutput") const;

    //private:
      /// Helper method to simplify querying the 'dirty' fields
      bool IsDirty(DIRTY_FLAG field) const { return _dirty[static_cast<unsigned short>(field)]; }

      /// Helper method to implement the ranking decision between two semantic labels using the priority passed by the caller
      static supera::SemanticType_t _SemanticPriority(supera::SemanticType_t a, supera::SemanticType_t b,
                                                      const std::vector<supera::SemanticType_t> & semanticPriority);

      /// Helper function that generates a vector of vector of voxel id and value
      void FillClustersEnergy(std::vector<std::vector<supera::VoxelID_t> >& ids,
        std::vector<std::vector<float> >& values,
        bool fill_unassociated=true) const;

      void FillClustersdEdX(std::vector<std::vector<supera::VoxelID_t> >& ids,
        std::vector<std::vector<float> >& values,
        bool fill_unassociated=true) const;

      void FillTensorSemantic(std::vector<VoxelID_t>& ids,
        std::vector<float>& values) const;

      void FillTensorEnergy(std::vector<VoxelID_t>& ids,
        std::vector<float>& values) const;

      std::vector<ParticleLabel> _particles;
      
      mutable supera::VoxelSet _energies;       ///< the total energy deposits in each voxel over all the contained particles contributing to the voxel
      //mutable supera::VoxelSet _dEdXs;          ///< the dE/dxs for each voxel taken as an energy-weighted mean over all the contained particles contributing to the voxel
      mutable supera::VoxelSet _semanticLabels; ///< semantic labels for each energy deposit, determined by \ref SemanticPriority()
      mutable supera::VoxelSet _unassociated_voxels; ///< 3D voxels that is a subset of _energies and _semanticLabels but has no associated particle.
      mutable std::array<bool, sizeof(DIRTY_FLAG)> _dirty = {};  ///< flag to signal when the internal sum fields need to be recalculated
  };

}
#endif
/** @} */ // end of doxygen group
