
#ifndef __BBOX_BASE_H__
#define __BBOX_BASE_H__

#include "supera/algorithm/AlgorithmBase.h"
#include "supera/data/ImageMeta3D.h"
#include "supera/data/Particle.h"

namespace supera {

	/**
		\class BBoxAlgorithm
		The base class definition for algorithms that are responsible for defining image meta data.
	*/
	class BBoxAlgorithm : public AlgorithmBase {

    public:

        BBoxAlgorithm(std::string name="no_name") : AlgorithmBase(name) {}

        virtual ~BBoxAlgorithm() {}

        virtual ImageMeta3D Generate(const EventInput& data) const = 0;

  };

}

#endif
