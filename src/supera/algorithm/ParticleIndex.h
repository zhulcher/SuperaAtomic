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
#include "supera/algorithm/AlgorithmBase.h"

namespace supera {
    /**
     \class ParticleIndex
     \brief Workhorse class encapsulating the logic for tying together a list of GEANT4 particles and their genealogy information
    */
    class ParticleIndex : public AlgorithmBase {

    public:

    /// Default constructor
        ParticleIndex(std::string name="ParticleIndex") : AlgorithmBase(name) {}

    /// Default destructor
        ~ParticleIndex(){}

        void InferParentage(const EventInput& larmcp_v);   ///< Fill in the ParticleIndex working structures with information about particle parents
        void SetParentInfo(EventInput& larmcp_v);
    //std::vector<supera::TrackID_t> ParentTrackIDs(const TrackID_t trackid) const;

        const std::vector< PdgCode_t >& PdgCode()          const { return _pdgcode_v;          }
        const std::vector< Index_t   >& ParentIndex()      const { return _parent_index_v;     }
        const std::vector< TrackID_t >& ParentTrackId()    const { return _parent_trackid_v;   }
        const std::vector< PdgCode_t >& ParentPdgCode()    const { return _parent_pdg_v;       }
        const std::vector< Index_t   >& TrackIdToIndex()   const { return _trackid2index;      }
        const std::vector< Index_t   >& AncestorIndex()    const { return _ancestor_index_v;   }
        const std::vector< TrackID_t >& AncestorTrackId()  const { return _ancestor_trackid_v; }
        const std::vector< TrackID_t >& ParentTrackIdArray(const TrackID_t) const;

    protected:

        void _configure(const YAML::Node& cfg) override;

    private:
        std::vector< TrackID_t > _trackid_v;          ///< Track ID, index = std::vector<supera::ParticleInput> index
        std::vector< PdgCode_t > _pdgcode_v;          ///< PDG code, index = std::vector<supera::ParticleInput> index
        std::vector< Index_t   > _parent_index_v;     ///< Parent index, index = std::vector<supera::ParticleInput> index
        std::vector< TrackID_t > _parent_trackid_v;   ///< Parent track ID, index = std::vector<supera::ParticleInput> index
        std::vector< PdgCode_t > _parent_pdg_v;       ///< Parent PDG, index = std::vector<supera::ParticleInput> index
        std::vector< Index_t   > _ancestor_index_v;   ///< Ancestor index, index = std::vector<supera::ParticleInput> index
        std::vector< TrackID_t > _ancestor_trackid_v; ///< Ancestor track ID, index = std::vector<supera::ParticleInput> index
        std::vector< PdgCode_t > _ancestor_pdg_v;     ///< Ancestor PDG, index = std::vector<supera::ParticleInput> index
        std::vector< Index_t   > _trackid2index;      ///< TrackID => std::vector<supera::ParticleInput> index converter
        std::vector<std::vector< TrackID_t > > _parent_history_v;   ///< TrackID => std::vector<supera::TrackID_t> 
        std::vector< TrackID_t > _empty_trackid_v;    ///< Empty list of TrackID 
    };
}

#endif
/** @} */ // end of doxygen group 
