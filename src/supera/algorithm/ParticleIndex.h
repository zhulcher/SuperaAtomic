/**
 * \file ParticleIndex.h
 *
 * \ingroup algorithm
 * 
 * \brief Class def header for a class ParticleIndex
 *
 * @author kterao
 */

/** \addtogroup algorithm
    @{*/
#ifndef __PARTICLEINDEX_H__
#define __PARTICLEINDEX_H__

#include "supera/data/Particle.h"

namespace supera {
  
  /**
     \class ParticleIndex
  */
  class ParticleIndex{
    
  public:
    
    /// Default constructor
    ParticleIndex(){}
    
    /// Default destructor
    ~ParticleIndex(){}

    void Update(const EventInput& larmcp_v);

    const std::vector<int>& PdgCode()        const { return _pdgcode_v;        }
    const std::vector<int>& ParentIndex()    const { return _parent_index_v;   }
    const std::vector<int>& ParentTrackId()  const { return _parent_trackid_v; }
    const std::vector<int>& ParentPdgCode()  const { return _parent_pdg_v;     }
    const std::vector<int>& TrackIdToIndex() const { return _trackid2index;    }
    const std::vector<int>& AncestorIndex()  const { return _ancestor_index_v; }
    const std::vector<int>& AncestorTrackId()  const { return _ancestor_trackid_v; }

  private:
    std::vector<int> _trackid_v;          ///< Track ID, index = std::vector<simb::MCParticle> index
    std::vector<int> _pdgcode_v;          ///< PDG code, index = std::vector<simb::MCParticle> index
    std::vector<int> _parent_index_v;     ///< Parent index, index = std::vector<simb::MCParticle> index
    std::vector<int> _parent_trackid_v;   ///< Parent track ID, index = std::vector<simb::MCParticle> index
    std::vector<int> _parent_pdg_v;       ///< Parent PDG, index = std::vector<simb::MCParticle> index
    std::vector<int> _ancestor_index_v;   ///< Ancestor index, index = std::vector<simb::MCParticle> index
    std::vector<int> _ancestor_trackid_v; ///< Ancestor track ID, index = std::vector<simb::MCParticle> index
    std::vector<int> _trackid2index;      ///< TrackID => std::vector<simb::MCParticle> index converter
  };
}

#endif
/** @} */ // end of doxygen group 
