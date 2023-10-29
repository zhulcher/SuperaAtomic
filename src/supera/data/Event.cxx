#include "Event.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <sstream>
#include "supera/base/meatloaf.h"

namespace supera {

  // --------------------------------------------------------
  /*
  const supera::VoxelSet &EventOutput::VoxelDeDxs() const
  {
    // recompute only if particle list has changed under us
    if (IsDirty(DIRTY_FLAG::kDeDx))
    {
      // we want an energy-weighted mean here.
      // first add all the dEdXs up, weighted by their energies...
      supera::VoxelSet dEdXs;
      for (const supera::ParticleLabel & part : Particles())
      {
        for (const supera::Voxel & dedx : part.dedx.as_vector())
          dEdXs.emplace(dedx.id(), dedx.value() * VoxelEnergies().find(dedx.id()).value(), true);
      }

      // now renormalize them...
      for (const supera::Voxel & dedx : dEdXs.as_vector())
      {
        auto energy = VoxelEnergies().find(dedx.id()).value();
        if (energy > 0)
          dEdXs.emplace(dedx.id(), dedx.value() / energy, false);
      }

      _dEdXs = std::move(dEdXs);
    } // if (IsDirty(...))

    return _dEdXs;
  }
  
  // --------------------------------------------------------

  const supera::VoxelSet &EventOutput::VoxelEnergies() const
  {
    // recompute only if particle list has changed under us
    if (IsDirty(DIRTY_FLAG::kEnergy))
    {
      supera::VoxelSet energies;
      for (const supera::ParticleLabel & part : Particles())
        energies.emplace(part.energy, true);
      _energies = std::move(energies);
    }

    return _energies;
  }

  // --------------------------------------------------------
  
  const supera::VoxelSet &
  EventOutput::VoxelLabels(const std::vector<supera::SemanticType_t> &semanticPriority) const
  {
    // recompute only if particle list has changed under us
    if (IsDirty(DIRTY_FLAG::kLabel))
    {
      supera::VoxelSet semantics;
      for (const supera::ParticleLabel & part : Particles())
      {
        auto const &vs = part.energy;
        SemanticType_t semantic = part.part.shape;
        for (auto const &vox : vs.as_vector())
        {
          auto const &prev = semantics.find(vox.id());
          if (prev.id() == supera::kINVALID_VOXELID)
            semantics.emplace(vox.id(), semantic, false);
          else
          {
            // todo: what if the new voxel has 10x the energy??
            SemanticType_t prioritized_semantic = EventOutput::_SemanticPriority(static_cast<SemanticType_t>(prev.value()), semantic, semanticPriority);
            if (prioritized_semantic != static_cast<SemanticType_t>(prev.value()))
              semantics.emplace(vox.id(), semantic, false);
          }
        } // for (vox)
      } // for (part)

      _semanticLabels = std::move(semantics);
    } // if (IsDirty(...))

    return _semanticLabels;
  }
*/
  // --------------------------------------------------------

  supera::SemanticType_t EventOutput::_SemanticPriority(supera::SemanticType_t a, supera::SemanticType_t b,
                                                        const std::vector<supera::SemanticType_t> & semanticPriority)
  {
    if (a == b)
      return a;
    for (auto const &semantic : semanticPriority)
    {
      if (a == semantic)
        return a;
      if (b == semantic)
        return b;
    }
    return a;
  }

  // --------------------------------------------------------

  bool EventOutput::operator==(const EventOutput &rhs) const
  {
    // the event outputs are the same if their ParticleLabels are the same.

    // definitely different if different lengths
    if (_particles.size() != rhs._particles.size())
      return false;

    // the ParticleLabels might not be in the same order,
    // but if the *contents* are the same, they're still equivalent.
    // we'll sort them by GEANT trackID using a map from track id -> vector index
    // (since maps are inherently sorted).
    // then we can walk through the track indices, check those first,
    // and only if they are equal do the actual comparison between ParticleList objects.
    std::map<supera::TrackID_t, std::size_t> lhs_tracks, rhs_tracks;
    for (std::size_t idx = 0; idx < _particles.size(); idx++)
    {
      lhs_tracks[_particles[idx].part.trackid] = idx;
      rhs_tracks[rhs._particles[idx].part.trackid] = idx;
    }

    auto it_lhs = lhs_tracks.begin();
    auto it_rhs = rhs_tracks.begin();
    for (; it_lhs != lhs_tracks.end() && it_rhs != rhs_tracks.end(); it_lhs++, it_rhs++ )
    {
      // since the map is sorted, any mismatch in track IDs already means they're not equal
      if (it_lhs->first != it_rhs->first)
        return false;

      // if we have to, check the objects themselves
      if (_particles[it_lhs->second] != rhs._particles[it_rhs->second])
        return false;
    }

    return true;

  }

  // --------------------------------------------------------

  std::string EventOutput::dump2cpp(const std::string &instanceName) const
  {
    std::stringstream ss;

    ss << "supera::EventOutput " << instanceName << ";\n";

    ss << instanceName << ".Particles().reserve(" << _particles.size() << ");\n";
    for (std::size_t idx = 0; idx < _particles.size(); idx++)
    {
      const supera::ParticleLabel & part = _particles[idx];
      std::string partInstance = instanceName + "_part" + std::to_string(idx) + "_label";
      ss << part.dump2cpp(partInstance);
      ss << instanceName << ".Particles().push_back(std::move(" << partInstance << "));\n";
    }

    return ss.str();
  }

  /// Helper function that generates a vector of vector of voxel id and value
  void EventOutput::FillClustersEnergy(std::vector<std::vector<supera::VoxelID_t> >& ids,
    std::vector<std::vector<float> >& values,
    bool fill_unassociated) const
  {
    ids.resize(_particles.size());
    values.resize(_particles.size());

    for(size_t i=0; i<_particles.size(); ++i) {
      auto const& target = _particles[i].energy;
      auto& id_v = ids[i];
      auto& value_v = values[i];
      id_v.clear();
      value_v.clear();
      id_v.reserve(target.size());
      value_v.reserve(target.size());
      for(auto const& vox : target.as_vector()) {
        id_v.push_back(vox.id());
        value_v.push_back(vox.value());
      }
    }

    if(fill_unassociated) {
      auto const& unass = _unassociated_voxels.as_vector();
      std::vector<supera::VoxelID_t> id_v;
      std::vector<float> value_v;
      id_v.resize(unass.size());
      value_v.resize(unass.size());
      for(size_t i=0; i<unass.size(); ++i){
        id_v[i] = unass[i].id();
        value_v[i] = unass[i].value();
      }
      ids.push_back(id_v);
      values.push_back(value_v);
    }
  }

  void EventOutput::FillClustersdEdX(std::vector<std::vector<supera::VoxelID_t> >& ids,
    std::vector<std::vector<float> >& values,
    bool fill_unassociated) const
  {
    ids.resize(_particles.size());
    values.resize(_particles.size());

    for(size_t i=0; i<_particles.size(); ++i) {
      auto const& target = _particles[i].dedx;
      auto& id_v = ids[i];
      auto& value_v = values[i];
      id_v.clear();
      value_v.clear();
      id_v.reserve(target.size());
      value_v.reserve(target.size());
      for(auto const& vox : target.as_vector()) {
        id_v.push_back(vox.id());
        value_v.push_back(vox.value());
      }
    }
    if(fill_unassociated) {
      auto const& unass = _unassociated_voxels.as_vector();
      std::vector<supera::VoxelID_t> id_v;
      std::vector<float> value_v;
      id_v.resize(unass.size());
      value_v.resize(unass.size(),0.);
      for(size_t i=0; i<unass.size(); ++i)
        id_v[i] = unass[i].id();
      ids.push_back(id_v);
      values.push_back(value_v);
    }
  }

  void EventOutput::FillTensorSemantic(std::vector<VoxelID_t>& ids,
    std::vector<float>& values) const
  {
    ids.clear();
    values.clear();
    ids.reserve(_semanticLabels.size());
    values.reserve(_semanticLabels.size());
    for(auto const& vox : _semanticLabels.as_vector())
    {
      ids.push_back(vox.id());
      values.push_back(vox.value());
    }
  }

  void EventOutput::FillTensorEnergy(std::vector<VoxelID_t>& ids,
    std::vector<float>& values) const
  {
    ids.clear();
    values.clear();
    ids.reserve(_energies.size());
    values.reserve(_energies.size());
    for(auto const& vox : _energies.as_vector())
    {
      ids.push_back(vox.id());
      values.push_back(vox.value());
    }
  }

} // namespace supera
