#ifndef __PARTICLEINDEX_CXX__
#define __PARTICLEINDEX_CXX__

#include "ParticleIndex.h"

namespace supera{ 

  void ParticleIndex::Update(const EventInput& larmcp_v)
  {
    _trackid_v.resize(larmcp_v.size());
    _pdgcode_v.resize(larmcp_v.size());
    _parent_index_v.resize(larmcp_v.size());
    _parent_trackid_v.resize(larmcp_v.size());
    _parent_pdg_v.resize(larmcp_v.size());
    _ancestor_trackid_v.resize(larmcp_v.size());
    _ancestor_index_v.resize(larmcp_v.size());

    for(size_t i=0; i<larmcp_v.size(); ++i)
      _trackid_v[i] = _pdgcode_v[i] = _parent_index_v[i] = _parent_trackid_v[i] = _parent_pdg_v[i] = _ancestor_trackid_v[i] = _ancestor_index_v[i] = -1;

    _trackid2index.resize(std::max(_trackid2index.size(),larmcp_v.size()));
    for(auto& v : _trackid2index) v = -1;

    for(size_t index=0; index<larmcp_v.size(); ++index) {
      auto const& mcpart = larmcp_v[index].part;
      _trackid_v[index] = abs(mcpart.trackid);
      _pdgcode_v[index] = abs(mcpart.pdg);
      _parent_trackid_v[index] = mcpart.parent_trackid;
      if(mcpart.trackid >= ((int)(_trackid2index.size()))) _trackid2index.resize(mcpart.trackid+1,-1);
      _trackid2index[mcpart.trackid] = index;
    }
    // Parent/Ancestor info
    for(size_t index=0; index<larmcp_v.size(); ++index) {
      auto const& mcpart = larmcp_v[index].part;
      int mother_id      = mcpart.parent_trackid;
      int mother_index   = -1;
      if(mother_id == 0) mother_id = abs(mcpart.trackid);
      if(mother_id < ((int)(_trackid2index.size()))) {
        mother_index = _trackid2index[mother_id];
        if(mother_index >= 0) {
          _parent_pdg_v[index] = larmcp_v[mother_index].part.pdg;
          _parent_index_v[index] = mother_index;
        }
      }

      int subject_track_id = abs(mcpart.trackid);
      int parent_track_id  = abs(mcpart.parent_trackid);
      int ancestor_index = -1;
      int ancestor_track_id = -1;
      while(1) {
        if((size_t)(parent_track_id) >= _trackid2index.size())
          break;
        if(parent_track_id == 0 || parent_track_id == subject_track_id) {
          ancestor_index = _trackid2index[subject_track_id];
          ancestor_track_id = subject_track_id;
          break;
        }
        auto const& parent_index = _trackid2index[parent_track_id];
        if(parent_index < 0)
          break;
        auto const& parent = larmcp_v[parent_index];
        subject_track_id = abs(parent.part.trackid);
        parent_track_id = abs(parent.part.parent_trackid);
      }
      _ancestor_index_v[index] = ancestor_index;
      _ancestor_trackid_v[index] = ancestor_track_id;
    }
  }
}
#endif