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

		////////////////////////////
		// Process control functions
		////////////////////////////

		/// Call this function first for a new event
		void Reset() {_label = EventOutput(); _meta = ImageMeta3D(); }

		/// Call this function second to configure an algorithm for defining image boundaries
		void ConfigureBBoxAlgorithm(const std::string& name,
			const std::map<std::string,std::string>& params);

		/// Call this function third to configure an algorithm for creating labels
		void ConfigureLabelAlgorithm(const std::string& name,
			const std::map<std::string,std::string>& params);

		/// Call this function fourth to execute algorithms and create output
		void GenerateLabel(const EventInput& data);

		/// Getter for created labels
		const EventOutput& Label() const
		{ return _label; }

		/// Getter for created image boundaries
		const ImageMeta3D& Meta() const
		{ return _meta;  }

	private:
		BBoxAlgorithm* _algo_bbox;
		LabelAlgorithm* _algo_label;
		ImageMeta3D _meta;
		EventOutput _label;
	};
}

#endif