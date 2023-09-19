/**
 * \file Event.h
 *
 * \ingroup base
 *
 * \brief Classes for information about events.
 *
 * @author kazuhiro
 */

/** \addtogroup base
    @{*/

#ifndef __SUPERA_EVENT_H__
#define __SUPERA_EVENT_H__

#include "Particle.h"

namespace supera
{
  class EventInput : public std::vector<ParticleInput>
  {
    public:
      /// 3D energy depositions unassociated to any input particle
      std::vector <EDep> unassociated_edeps;
  };

  /// Class to store the labeled particles of an event & their associated hit voxels (if any).
  class EventOutput
  {
    private:
      /// Helper to keep track of which parts of the storage need to be rebuilt after changes
      enum class DIRTY_FLAG : unsigned short
      {
        kLabel, kEnergy, kDeDx
      };

    public:
      /// Get the list of particle labels, const version.  If you need to change them see the other version of \ref Particles()
      const std::vector <ParticleLabel>& Particles() const
      { return _particles; }

      /// Get the list of particle labels, non-const version.  This implementation is quite lazy but I doubt we'll need anything more sophisticated.
      std::vector <ParticleLabel>& Particles()
      {
        _dirty.fill(true);
        return _particles;
      }

      /// \brief Get the dE/dx for all voxels with energy deposition in them.
      /// N.b.: For voxels that have multiple contributing particles, the dE/dx computed is the energy-weighted mean dE/dx.
      /// \return \ref VoxelSet with energy-weighted mean dE/dx for each voxel with energy in it
      const supera::VoxelSet& VoxelDeDxs() const;

      /// \brief Get the deposited energies for all voxels with energy deposition in them.
      /// \return \ref VoxelSet with total deposited energy for each voxel with energy in it
      const supera::VoxelSet& VoxelEnergies() const;

      /// \brief Get the labels for the voxels.
      /// \param semanticPriority Ranking for which label "wins" when multiple particles of different semantic type contribute to the same voxel: highest priority first.
      /// \return \ref VoxelSet with each entry corresponding to the semantic label for each voxel with energy in it
      const supera::VoxelSet& VoxelLabels(const std::vector <supera::SemanticType_t>& semanticPriority) const;

      EventOutput& operator=(const std::vector <ParticleLabel>& other)
      {
        Particles() = other;
        return *this;
      }

      EventOutput& operator=(std::vector <ParticleLabel>&& other)
      {
        Particles() = std::move(other);
        return *this;
      }

      /// Is this EventOutput the same as \a rhs?
      bool operator==(const EventOutput& rhs) const;

      /// Dump this \ref EventOutput to a string of C++ code that can be used to reproduce it
      std::string dump2cpp(const std::string& instanceName = "evtOutput") const;

      //private:
      /// Helper method to simplify querying the 'dirty' fields
      bool IsDirty(DIRTY_FLAG field) const
      { return _dirty[static_cast<unsigned short>(field)]; }

      /// Helper method to implement the ranking decision between two semantic labels using the priority passed by the caller
      static supera::SemanticType_t _SemanticPriority(supera::SemanticType_t a, supera::SemanticType_t b,
                                                      const std::vector <supera::SemanticType_t>& semanticPriority);

      /// Helper function that generates a vector of vector of voxel id and value
      void FillClustersEnergy(std::vector <std::vector<supera::VoxelID_t>>& ids,
                              std::vector <std::vector<float>>& values,
                              bool fill_unassociated = true) const;

      void FillClustersdEdX(std::vector <std::vector<supera::VoxelID_t>>& ids,
                            std::vector <std::vector<float>>& values,
                            bool fill_unassociated = true) const;

      void FillTensorSemantic(std::vector <VoxelID_t>& ids,
                              std::vector<float>& values) const;

      void FillTensorEnergy(std::vector <VoxelID_t>& ids,
                            std::vector<float>& values) const;

      std::vector <ParticleLabel> _particles;

      mutable supera::VoxelSet _energies;       ///< the total energy deposits in each voxel over all the contained particles contributing to the voxel
      //mutable supera::VoxelSet _dEdXs;          ///< the dE/dxs for each voxel taken as an energy-weighted mean over all the contained particles contributing to the voxel
      mutable supera::VoxelSet _semanticLabels; ///< semantic labels for each energy deposit, determined by \ref SemanticPriority()
      mutable supera::VoxelSet _unassociated_voxels; ///< 3D voxels that is a subset of _energies and _semanticLabels but has no associated particle.
      mutable std::array<bool, sizeof(DIRTY_FLAG)> _dirty = {};  ///< flag to signal when the internal sum fields need to be recalculated
  };
}

#endif  // __SUPERA_EVENT_H__

/** @} */ // end of doxygen group
