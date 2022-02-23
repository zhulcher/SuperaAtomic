#ifndef __LABELALGORITHM_H__
#define __LABELALGORITHM_H__

#include "supera/base/PSet.h"
#include "supera/data/Particle.h"
#include "supera/data/ImageMeta3D.h"
namespace supera {

	/**
		\class LabelAlgorithm
		The base class definition for algorithms that are responsible for creating output tensor information.
	*/
	class LabelAlgorithm {
	public:
		LabelAlgorithm() {}
		virtual ~LabelAlgorithm() {}
		virtual void Configure(const PSet& p) = 0;
		virtual EventOutput Generate(const EventInput& data, const ImageMeta3D& meta) = 0;
	};
}

#endif