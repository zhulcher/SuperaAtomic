#ifndef __SUPERAEVENT_H__
#define __SUPERAEVENT_H__

#include "ImageMeta3D.h"
#include "ParticleGroup.h"
#include "Voxel.h"

namespace supera {

	class Event {
	public:
		Event() {};
		/// Defines boundaries for 3D image
		ImageMeta3D meta; 
		/// A set of particles
		std::vector<supera::ParticleGroup> part_v;
	};
	
}

#endif
