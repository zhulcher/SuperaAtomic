#ifndef __SUPERA_DRIVER_H__
#define __SUPERA_DRIVER_H__

#include "supera/data/Particle.h"
#include "supera/data/ImageMeta3D.h"
#include "supera/algorithm/BBoxBase.h"
#include "supera/algorithm/LabelBase.h"
namespace supera {
	
	class Driver {
	public:

		Driver() : _algo_bbox(nullptr), _algo_label(nullptr) 
		{}

		void ConfigureBBoxAlgorithm(const std::string& name,
			const std::map<std::string,std::string>& params);

		void ConfigureLabelAlgorithm(const std::string& name,
			const std::map<std::string,std::string>& params);

		void Reset() {_label = EventOutput(); _meta = ImageMeta3D(); }

		const EventOutput& Label() const
		{ return _label; }

		const ImageMeta3D& Meta() const
		{ return _meta;  }

		void GenerateLabel(const EventInput& data);


	private:
		BBoxAlgorithm* _algo_bbox;
		LabelAlgorithm* _algo_label;
		ImageMeta3D _meta;
		EventOutput _label;
	};
}

#endif