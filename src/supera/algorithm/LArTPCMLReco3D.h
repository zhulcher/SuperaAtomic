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
		LArTPCMLReco3D();
		void Configure(const PSet& p) override;
		EventOutput Generate(const EventInput& data, const ImageMeta3D& meta) override;

	private:
        // ----- internal label initialization -----
        std::vector<supera::ParticleLabel> InitializeLabels(const EventInput& evtInput) const;

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
        void MergeShowerTouchingLEScatter(const supera::ImageMeta3D& meta,
                                          std::vector<supera::ParticleLabel>& labels) const;


        // -----  utility methods -----
        /// filter out any voxels voxels that have energy below the given threshold
        void ApplyEnergyThreshold(std::vector<supera::ParticleLabel>& labels) const;

        /// Do the two given VoxelSets overlap at all?
        bool IsTouching(const ImageMeta3D& meta, const VoxelSet& vs1, const VoxelSet& vs2) const;

        /// Get a list of all the GEANT4 tracks that are in the ancestry chain of the given one,
        /// constrained to staying within the same EM shower.
        std::vector<unsigned int>
        ParentShowerTrackIDs(size_t trackid,
                             const std::vector<supera::ParticleLabel>& labels,
                             bool include_lescatter=false) const;

        /// Get a list of all the GEANT4 tracks that are in the ancestry chain of the given one.
        /// Most recent ancestor at index 0.
        std::vector<unsigned int> ParentTrackIDs(size_t trackid) const;


        size_t _debug;
		std::vector<size_t> _semantic_priority;
        size_t _touch_threshold;
        size_t _delta_size;
        size_t _eioni_size;
        size_t _compton_size;
        double _edep_threshold;
        bool _use_true_pos;
        bool _use_sed;
        bool _use_sed_points;
        bool _store_dedx;
        bool _use_ture_pos;
        bool _check_particle_validity;
        BBox3D _world_bounds;

        ParticleIndex _mcpl;
	};
}

#endif
