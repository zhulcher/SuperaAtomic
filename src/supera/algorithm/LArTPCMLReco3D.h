#ifndef __LARTPCMLRECO3D_H__
#define __LARTPCMLRECO3D_H__

#include "LabelBase.h"

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
	};
}

#endif