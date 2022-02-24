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
		void Configure(const PSet& p);
		EventOutput Generate(const EventInput& data, const ImageMeta3D& meta);

	private:
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