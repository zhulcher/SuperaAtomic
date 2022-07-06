#ifndef __PARTICLEINDEX_CXX__
#define __PARTICLEINDEX_CXX__

#include "ParticleIndex.h"

namespace supera{ 

  void ParticleIndex::InferParentage(const EventInput& larmcp_v)
  {
    _trackid_v.resize(larmcp_v.size());
    _pdgcode_v.resize(larmcp_v.size());
    _parent_index_v.resize(larmcp_v.size());
    _parent_trackid_v.resize(larmcp_v.size());
    _parent_pdg_v.resize(larmcp_v.size());
    _ancestor_trackid_v.resize(larmcp_v.size());
    _ancestor_index_v.resize(larmcp_v.size());
    _ancestor_pdg_v.resize(larmcp_v.size());

    for(size_t i=0; i<larmcp_v.size(); ++i) {
      _pdgcode_v[i] = _parent_pdg_v[i] = _ancestor_pdg_v[i] = supera::kINVALID_PDG;
      _parent_index_v[i] = _ancestor_index_v[i] = supera::kINVALID_INDEX;
      _trackid_v[i] = _parent_trackid_v[i] = _ancestor_trackid_v[i] = supera::kINVALID_TRACKID;
    }

    _trackid2index.resize(std::max(_trackid2index.size(),larmcp_v.size()));
    for(auto& v : _trackid2index) v = supera::kINVALID_INDEX;

    // fill in the ParticleIndex's working structures.

    // first: create the mapping between GEANT4 trackid <-> index in the particle array
    // (FYI: the larmcp objects correspond each to one GEANT4 particle
    //       + any associated energy deposits)
    for(size_t index=0; index<larmcp_v.size(); ++index) {
      auto const& mcpart = larmcp_v[index].part;  // pull off the GEANT4 track information component
      _trackid_v[index] = abs(mcpart.trackid);
      _pdgcode_v[index] = abs(mcpart.pdg);
      _parent_trackid_v[index] = mcpart.parent_trackid;
      if(mcpart.trackid >= _trackid2index.size()) _trackid2index.resize(mcpart.trackid+1, supera::kINVALID_INDEX);
      _trackid2index[mcpart.trackid] = index;
    }
    // now fill in the mapping between the index in the particle array <-> Parent/Ancestor info.
    // (note that for our purposes here, 'ancestor' is the *primary* particle that sits
    //  at the top of the hierarchy containing this particle.  if this particle is itself primary,
    //  it's its own ancestor.)
    for(size_t index=0; index<larmcp_v.size(); ++index) {
      auto const& mcpart = larmcp_v[index].part;
      unsigned long mother_id      = mcpart.parent_trackid;
      unsigned long mother_index   = supera::kINVALID_INDEX;
      
      // If mother ID is zero, interpret it as a primary particle
      if(mother_id == 0) mother_id = abs(mcpart.trackid);

      // Otherwise search if a mother particle is available
      if(mother_id < _trackid2index.size()) {
        mother_index = _trackid2index[mother_id];
        if(mother_index != supera::kINVALID_INDEX) {
          _parent_pdg_v[index] = larmcp_v[mother_index].part.pdg;
          _parent_index_v[index] = mother_index;
        }
      }

      unsigned long subject_track_id = mcpart.trackid;
      unsigned long parent_track_id  = mcpart.parent_trackid;
      unsigned long ancestor_index = supera::kINVALID_INDEX;
      unsigned long ancestor_track_id = supera::kINVALID_TRACKID;
      while(true) {
        if((size_t)(parent_track_id) >= _trackid2index.size())
          break;
        if(parent_track_id == 0 || parent_track_id == subject_track_id) {
          ancestor_index = _trackid2index[subject_track_id];
          ancestor_track_id = subject_track_id;
          break;
        }
        auto const& parent_index = _trackid2index[parent_track_id];
        if(parent_index == supera::kINVALID_INDEX)
          break;
        auto const& parent = larmcp_v[parent_index];
        subject_track_id = abs(parent.part.trackid);
        parent_track_id = abs(parent.part.parent_trackid);
      }
      _ancestor_index_v[index] = ancestor_index;
      _ancestor_trackid_v[index] = ancestor_track_id;
      if(ancestor_index < larmcp_v.size()) 
        _ancestor_pdg_v[index] = larmcp_v[ancestor_index].part.pdg;
    }
  }

  void ParticleIndex::SetParentInfo(EventInput& larmcp_v)
  {
    this->InferParentage(larmcp_v);

    for(unsigned int idx=0; idx<larmcp_v.size(); ++idx) {
      auto& part = larmcp_v[idx].part;
      part.parent_pdg       = _parent_pdg_v       [idx];
      part.parent_trackid   = _parent_trackid_v   [idx];
      part.ancestor_pdg     = _ancestor_pdg_v     [idx];
      part.ancestor_trackid = _ancestor_trackid_v [idx];
    }
  }
}
#endif
