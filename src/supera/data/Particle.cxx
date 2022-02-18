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

    size_t res=energy.size();
    /*
    for(auto const& vs2d : vs2d_v) res += vs2d.size();
      return res;
    */    
    return 0;
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
  

}