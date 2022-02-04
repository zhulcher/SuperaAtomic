#include "SuperaData.h"
#include <iostream>

namespace supera {


  ParticleGroup::ParticleGroup(size_t num_planes)
  : valid(false)
  , add_to_parent(false)
  , type(supera::kInvalidProcess)
  {}
  //{ vs2d_v.resize(num_planes); }

  void ParticleGroup::AddEDep(const EDep& pt)
  { 
    if(pt.x == supera::kINVALID_DOUBLE) return; 
    if(pt.t < first_pt.t) first_pt = pt; 
    if(pt.t > last_pt.t) last_pt = pt; 
  }

  void ParticleGroup::SizeCheck() const
  {
    /*
    if(dedx.size() && vs.size() != dedx.size()) {
      std::cerr << "Size mismatch: " << vs.size() << " v.s. " << dedx.size() << std::endl;
      throw std::exception();
    }
    */
  }


  size_t ParticleGroup::Size() const
  { 
    /*
    size_t res=vs.size(); 
    for(auto const& vs2d : vs2d_v) res += vs2d.size();
      return res;
    */
    return 0;
  }

  void ParticleGroup::Merge(ParticleGroup& child,bool verbose) {
    /*
    for(auto const& vox : child.vs.as_vector())
      this->vs.emplace(vox.id(),vox.value(),true);

    for(auto const& vox : child.dedx.as_vector())
      this->dedx.emplace(vox.id(),vox.value(),true);
    */
    if(verbose) {
      /*
      std::cout<<"Parent track id " << this->part.track_id() 
      << " PDG " << this->part.pdg_code() << " " << this->part.creation_process() << std::endl
      << "  ... merging " << child.part.track_id()
      << " PDG " << child.part.pdg_code() << " " << child.part.creation_process() << std::endl;
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

    child.vs.clear_data();
    child.dedx.clear_data();
    */
    child.valid=false;
  }

  // semantic classification (larcv::ShapeType_t)
  /*
  larcv::ShapeType_t ParticleGroup::shape() const
  {

    // identify delta ray
    if(type == kInvalidProcess) return larcv::kShapeUnknown;
    if(type == kDelta) return larcv::kShapeDelta;
    if(type == kNeutron) //return larcv::kShapeUnknown;
    return larcv::kShapeLEScatter;
    if(part.pdg_code() == 11 || part.pdg_code() == 22 || part.pdg_code() == -11) {
      if(type == kComptonHE || type == kPhoton || type == kPrimary || type == kConversion || type==kOtherShowerHE)
        return larcv::kShapeShower;
      if(type == kDecay) {
        if(part.parent_pdg_code() == 13 || part.parent_pdg_code() == -13)
          return larcv::kShapeMichel;
        else
          return larcv::kShapeShower;
      }
      return larcv::kShapeLEScatter;
    }else
    return larcv::kShapeTrack;
  }
  */

}