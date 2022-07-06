#include "Particle.h"
#include <iostream>

namespace supera {


  ParticleLabel::ParticleLabel(size_t num_planes)
  : valid(false)
  , add_to_parent(false)
  , type(supera::kInvalidProcess)
  {}
  //{ vs2d_v.resize(num_planes); }

  void ParticleLabel::AddEDep(const EDep& pt)
  { 
    if(pt.x == supera::kINVALID_DOUBLE) return; 
    if(pt.t < first_pt.t) first_pt = pt; 
    if(pt.t > last_pt.t) last_pt = pt;
  }

  void ParticleLabel::SizeCheck() const
  {

    if(dedx.size() && energy.size() != dedx.size()) {
      std::cerr << "Size mismatch: " << energy.size() << " v.s. " << dedx.size() << std::endl;
      throw std::exception();
    }

  }


  size_t ParticleLabel::Size() const
  { 
    SizeCheck();
    return energy.size();
  }

  void ParticleLabel::Merge(ParticleLabel& child,bool verbose) {

    for(auto const& vox : child.energy.as_vector())
      this->energy.emplace(vox.id(),vox.value(),true);

    for(auto const& vox : child.dedx.as_vector())
      this->dedx.emplace(vox.id(),vox.value(),true);
    
    if(verbose) {
      /*
      std::cout<<"Parent track id " << this->part.track_id() 
      << " PDG " << this->part.pdg << " " << this->part.creation_process() << std::endl
      << "  ... merging " << child.part.track_id()
      << " PDG " << child.part.pdg << " " << child.part.creation_process() << std::endl;
      */
    }

    this->AddEDep(child.last_pt);
    this->AddEDep(child.first_pt);
    //this->trackid_v.push_back(child.part.track_id());

    for(auto const& trackid : child.trackid_v)
      this->trackid_v.push_back(trackid);
    /*
    for(size_t plane_id=0; plane_id < vs2d_v.size(); ++plane_id) {
      auto& vs2d = vs2d_v[plane_id];
      auto& child_vs2d = child.vs2d_v[plane_id];
      for(auto const& vox : child_vs2d.as_vector())
        vs2d.emplace(vox.id(),vox.value(),true);
      child_vs2d.clear_data();
    }
    */
    child.energy.clear_data();
    child.dedx.clear_data();
    
    child.valid=false;
  }

  // semantic classification (supera::SemanticType_t)
  
  supera::SemanticType_t ParticleLabel::shape() const
  {

    // identify delta ray
    if(type == kInvalidProcess) return supera::kShapeUnknown;
    if(type == kDelta) return supera::kShapeDelta;
    if(type == kNeutron) //return supera::kShapeUnknown;
    return supera::kShapeLEScatter;
    if(part.pdg == 11 || part.pdg == 22 || part.pdg == -11) {
      if(type == kComptonHE || type == kPhoton || type == kPrimary || type == kConversion || type==kOtherShowerHE)
        return supera::kShapeShower;
      if(type == kDecay) {
        if(part.parent_pdg == 13 || part.parent_pdg == -13)
          return supera::kShapeMichel;
        else
          return supera::kShapeShower;
      }
      return supera::kShapeLEScatter;
    }else
    return supera::kShapeTrack;
  }

  // --------------------------------------------------------

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
        SemanticType_t semantic = part.shape();
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

} // namespace supera
