#ifndef __PARTICLEINDEX_CXX__
#define __PARTICLEINDEX_CXX__

#include "ParticleIndex.h"
#include "supera/base/meatloaf.h"

namespace supera{ 

  void ParticleIndex::InferParentage(const EventInput& larmcp_v)
  {
    /*
      Assumptions
        - track and parent id are valid for all particles
        - for primary particles in G4, its parent and own track id should be identical
    */
    _trackid_v.resize(larmcp_v.size());
    _pdgcode_v.resize(larmcp_v.size());
    _parent_index_v.resize(larmcp_v.size());
    _parent_trackid_v.resize(larmcp_v.size());
    _parent_pdg_v.resize(larmcp_v.size());
    _ancestor_trackid_v.resize(larmcp_v.size());
    _ancestor_index_v.resize(larmcp_v.size());
    _ancestor_pdg_v.resize(larmcp_v.size());
    _trackid2index.resize(larmcp_v.size());
    _parent_history_v.resize(larmcp_v.size());

    for(size_t i=0; i<larmcp_v.size(); ++i) {
      _pdgcode_v[i] = _parent_pdg_v[i] = _ancestor_pdg_v[i] = supera::kINVALID_PDG;
      _parent_index_v[i] = _ancestor_index_v[i] = supera::kINVALID_INDEX;
      _trackid_v[i] = _parent_trackid_v[i] = _ancestor_trackid_v[i] = supera::kINVALID_TRACKID;
    }

    _trackid2index.resize(std::max(_trackid2index.size(),larmcp_v.size()));
    for(auto& v : _trackid2index) v = supera::kINVALID_INDEX;

    for(auto& history : _parent_history_v) history.clear();

    // fill in the ParticleIndex's working structures.

    // first: create the mapping between GEANT4 trackid <-> index in the particle array
    // (FYI: the larmcp objects correspond each to one GEANT4 particle
    //       + any associated energy deposits)
    for(size_t index=0; index<larmcp_v.size(); ++index) {
      auto const& mcpart = larmcp_v[index].part;  // pull off the GEANT4 track information component
      if(mcpart.trackid == supera::kINVALID_TRACKID) {
        LOG.FATAL() << "Track ID cannot be invalid\n";
        throw supera::meatloaf();
      }
      _trackid_v[index] = mcpart.trackid;
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

      // Sanity check: all particle should have its parent track id
      if(mcpart.parent_trackid == supera::kINVALID_TRACKID) {
        LOG.FATAL() << "Parent ID cannot be invalid\n";
        throw supera::meatloaf();
      }

      // Attempt to find the parent PDG code and the input index
      supera::TrackID_t mother_id  = mcpart.parent_trackid;
      supera::Index_t mother_index = supera::kINVALID_INDEX;
      if(mother_id < _trackid2index.size()) {
        mother_index = _trackid2index[mother_id];
        if(mother_index != supera::kINVALID_INDEX) {
          _parent_pdg_v[index] = larmcp_v[mother_index].part.pdg;
          _parent_index_v[index] = mother_index;
        }
      }

      // Attempt to identify the ancestor
      auto subject_track_id = mcpart.trackid;
      auto parent_track_id  = mcpart.parent_trackid;
      auto ancestor_index = supera::kINVALID_INDEX;
      auto ancestor_track_id = supera::kINVALID_TRACKID;
      while(parent_track_id<_trackid2index.size()) {
        if(parent_track_id == subject_track_id) {
          ancestor_index = _trackid2index[subject_track_id];
          ancestor_track_id = subject_track_id;
          break;
        }
        _parent_history_v[index].push_back(parent_track_id);
        auto const& parent_index = _trackid2index[parent_track_id];
        if(parent_index == supera::kINVALID_INDEX)
          break;
        auto const& parent = larmcp_v[parent_index];
        subject_track_id = parent.part.trackid;
        parent_track_id = parent.part.parent_trackid;
      }
      // Set amcestor info
      _ancestor_index_v[index] = ancestor_index;
      _ancestor_trackid_v[index] = ancestor_track_id;
      if(ancestor_index < larmcp_v.size()) 
        _ancestor_pdg_v[index] = larmcp_v[ancestor_index].part.pdg;
    }
  }

  const std::vector< TrackID_t >& 
  ParticleIndex::ParentTrackIdArray(const TrackID_t trackid) const
  {
    if(trackid >= _parent_history_v.size()) {
      LOG.ERROR() << "Track ID " << trackid << " is not valid. Returning an empty list.\n";
      return _empty_trackid_v;
    }
    return _parent_history_v[trackid];
  }


  void ParticleIndex::SetParentInfo(EventInput& larmcp_v)
  {
    this->InferParentage(larmcp_v);

    for(supera::Index_t idx=0; idx<larmcp_v.size(); ++idx) {
      auto& part = larmcp_v[idx].part;
      part.parent_pdg       = _parent_pdg_v       [idx];
      part.parent_trackid   = _parent_trackid_v   [idx];
      part.ancestor_pdg     = _ancestor_pdg_v     [idx];
      part.ancestor_trackid = _ancestor_trackid_v [idx];
    }
  }
  /*
  std::vector<supera::TrackID_t> ParticleIndex::ParentTrackIDs(const TrackID_t trackid) const
  {
      auto const &trackid2index = this->TrackIdToIndex();
      std::vector<supera::TrackID_t> result;

      if (trackid >= trackid2index.size() || trackid2index[trackid] == supera::kINVALID_INDEX)
          return result;

      auto parent_trackid = this->ParentTrackId()[trackid2index[trackid]];
      std::set<supera::TrackID_t> accessed;
      while (parent_trackid < trackid2index.size() && 
          trackid2index[parent_trackid] != supera::kINVALID_INDEX)
      {
          if (accessed.find(parent_trackid) != accessed.end())
          {
              LOG.FATAL() << "LOOP-LOGIC-ERROR for ParentTrackIDs for track id " << StringifyTrackID(trackid) << ": repeated ancestor!\n";
              LOG.FATAL() << "Ancestors found:\n";
              for (size_t parent_cand_idx = 0; parent_cand_idx < result.size(); ++parent_cand_idx)
              {
                  auto const &parent_cand_trackid = result[parent_cand_idx];
                  LOG.FATAL() << "Parent idx " << parent_cand_idx
                              << " Track ID " << StringifyTrackID(parent_cand_trackid)
                              << " PDG " << _mcpl.PdgCode()[trackid2index[parent_cand_trackid]]
                              << " Mother " << StringifyTrackID(this->ParentTrackId()[trackid2index[parent_cand_trackid]])
                              << "\n";
              }
              throw meatloaf();
          }

          result.push_back(parent_trackid);
          accessed.insert(parent_trackid);
          auto parent_parent_trackid = this->ParentTrackId()[trackid2index[parent_trackid]];
          if (parent_parent_trackid == parent_trackid) break;
          parent_trackid = parent_parent_trackid;
      }
      return result;
  }
  */



}
#endif
