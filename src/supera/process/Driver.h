#ifndef __SUPERA_DRIVER_H__
#define __SUPERA_DRIVER_H__

#include "supera/data/Particle.h"
#include "supera/data/ImageMeta3D.h"
#include "supera/algorithm/BBoxBase.h"
#include "supera/algorithm/LabelBase.h"
#include "supera/base/Configurable.h"
#include "supera/base/Loggable.h"


namespace supera {
	
	/**
    	\class Driver
    	The top-level (i.e. "main") function of Supera that takes input data, execute algorithms, and return output. \n
		Two algorithms need to be configured: one to define image meta data, and another to produce output image tensor information. \n
		The former must inherit from BBoxAlgorithm (see algorithm/BBoxBase.h). The latter from LabelAlgorithm (see algorithm/LabelBase.h). \n
		Calling a function to configure each of them will instantiate and configure the algorithm with provided parameter information (PSet). \n
	*/
	class Driver : public Loggable, public Configurable {
	public:

		Driver(const std::string& name="Driver")
		: Loggable(name)
		, _algo_bbox(nullptr), _algo_label(nullptr) 
		{}

		virtual void Configure(const YAML::Node& cfg) override;

	    std::string DumpConfig(const YAML::Node& cfg);

		////////////////////////////////////
		// Functions to configure algorithms
		////////////////////////////////////

    	//void ConfigureFromYAML(const std::string yaml_file);
    	/*
		/// Configure an algorithm for defining image boundaries.
		void ConfigureBBoxAlgorithm(const std::string& name,
			const std::map<std::string,std::string>& params);

		/// Configure an algorithm for creating labels
		void ConfigureLabelAlgorithm(const std::string& name,
			const std::map<std::string,std::string>& params);
    	*/
		/////////////////////////////////////////////////
		// Per-image (per-event) process control functions
		/////////////////////////////////////////////////

		/// 1st function to reset the state of an instance for processing a new event
		void Reset() {_label = EventOutput(); _meta = ImageMeta3D(); }

		/// 2nd function to generate meta and labels
		void Generate(const EventInput& data);

		/// Function to generate image boundaries to be sampled (called by Generate())
		void GenerateImageMeta(const EventInput& data);

		/// Function to execute algorithms and create output (called by Generate())
		void GenerateLabel(const EventInput& data);

		///////////////////////////////
		// Attribute accessor functions
		///////////////////////////////

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
