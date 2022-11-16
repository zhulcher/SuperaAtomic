#ifndef __LARTPCMLRECO3D_H__
#define __LARTPCMLRECO3D_H__

#include "LabelBase.h"
#include "ParticleIndex.h"

namespace supera {

	/**
		\class LArTPCMLReco3D
		An implementation of LabelAlgorithm for producing labels for lartpc_mlreco3d reconstruction chain
	*/
	class LArTPCMLReco3D : public LabelAlgorithm {
	public:
		LArTPCMLReco3D(std::string name="LArTPCMLReco3D");
		EventOutput Generate(const EventInput& data, const ImageMeta3D& meta) override;

	protected:
		
		void _configure(const YAML::Node& cfg) override;

	private:

        // ----- label making -----
        std::vector<supera::ParticleLabel>
        InitializeLabels(const EventInput &evtInput, const supera::ImageMeta3D &meta) const;

	    void BuildOutputLabels(std::vector<supera::ParticleLabel>& labels,
	        supera::EventOutput& result, 
	        const std::vector<TrackID_t>& output2trackid) const;

        // ----- internal label merging methods -----
        /// Merge deltas into their parents if they have fewer than threshold voxels
        void MergeDeltas(std::vector<supera::ParticleLabel>& labels) const;

        /// Combine particles from e+/e- pair conversion into their parent particles
        void MergeShowerConversion(std::vector<supera::ParticleLabel>& labels) const;

        /// Combine deltas/Michels/etc that derive from a 'EM shower' shape parent into their parent
        void MergeShowerFamilyTouching(const supera::ImageMeta3D& meta,
                                       std::vector<supera::ParticleLabel>& labels) const;

        /// Combine 'EM shower' type particles that are 'ionization' process with their parents (they are always touching)
        void MergeShowerIonizations(std::vector<supera::ParticleLabel>& labels) const;

        /// Combine instances of two shower groups that share a common ancestor and are touching
        void MergeShowerTouching(const supera::ImageMeta3D& meta, std::vector<supera::ParticleLabel>& labels) const;

        /// Combine 'LE scatter' type particles that are touching their parents with them
        void MergeShowerTouchingElectron(const supera::ImageMeta3D& meta,
                                         std::vector<supera::ParticleLabel>& labels) const;

	    void MergeShowerTouchingLEScatter(const supera::ImageMeta3D& meta,
    	                                  std::vector<supera::ParticleLabel>& labels) const;

        /// Identify and register a set of particles to be stored in the output
        void RegisterOutputParticles(const std::vector<TrackID_t> &trackid2index,
        	std::vector<supera::ParticleLabel> &inputLabels,
        	std::vector<TrackID_t> &output2trackid,
        	std::vector<Index_t> &trackid2output) const;

        /// Assign Group ID: this only 
    	void SetGroupID(std::vector<supera::ParticleLabel>& labels) const;

    	void SetInteractionID(std::vector<supera::ParticleLabel>& labels) const;

	    void SetAncestorAttributes(std::vector<supera::ParticleLabel>& labels) const;

        // -----  internal group-sanitizing methods -----
        /// The first step of the true trajectory is important, and sometimes winds up unset.
        /// If so, clean it up using the first voxel attached to the label group.
        static void FixFirstStepInfo(std::vector<supera::ParticleLabel> &inputLabels,
                                     const supera::ImageMeta3D &meta,
                                     const std::vector<TrackID_t> &output2trackid);

        // -----  utility methods -----
        /// filter out any voxels voxels that have energy below the given threshold
        void ApplyEnergyThreshold(std::vector<supera::ParticleLabel>& labels) const;

	    void MergeParticleLabel(std::vector<supera::ParticleLabel>& labels,
	    	TrackID_t dest_trackid,
	    	TrackID_t target_trackid) const;

	    void SetSemanticType(std::vector<supera::ParticleLabel>& labels) const;

	    void SetSemanticPriority(std::vector<size_t>& order);

        /// write the full ancestry of true particle with specified GEANT4 track id to the debug stream
        void DumpHierarchy(size_t trackid, const std::vector<supera::ParticleLabel>& inputLabels) const;

        /// Do the two given VoxelSets overlap at all?
        bool IsTouching(const ImageMeta3D& meta, const VoxelSet& vs1, const VoxelSet& vs2) const;

        /// Return the input index from the track id
        Index_t InputIndex(const TrackID_t& tid) const
        { return tid >= _mcpl.TrackIdToIndex().size() ? kINVALID_INDEX : _mcpl.TrackIdToIndex()[tid]; }

        /// Get a list of all the GEANT4 tracks that are in the ancestry chain of the given one,
        /// constrained to staying within the same EM shower.
        std::vector<supera::TrackID_t>
        ParentShowerTrackIDs(TrackID_t trackid,
                             const std::vector<supera::ParticleLabel>& labels,
                             bool include_lescatter=false) const;

        /// Get a list of all the GEANT4 tracks that are in the ancestry chain of the given one.
        /// Most recent ancestor at index 0.
        //std::vector<supera::TrackID_t> ParentTrackIDs(size_t trackid) const;


        size_t _debug;
		std::vector<size_t> _semantic_priority;
        size_t _touch_threshold;
        size_t _delta_size;
        size_t _lescatter_size;
        size_t _compton_size;
        double _edep_threshold;
        bool _store_lescatter;
        BBox3D _world_bounds;
        ParticleIndex _mcpl;
	};
}

#endif
