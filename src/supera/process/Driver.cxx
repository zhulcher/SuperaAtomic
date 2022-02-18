#ifndef __DRIVER_CXX__
#define __DRIVER_CXX__

#include "Driver.h"
#include "supera/algorithm/BBoxInteraction.h"
#include "supera/algorithm/LArTPCMLReco3D.h"

namespace supera {
	
    void Driver::ConfigureBBoxAlgorithm(const std::string& name,
        const std::map<std::string,std::string>& params)
    {
        if(name == "BBoxInteraction") {
            _algo_bbox = new BBoxInteraction();
            PSet cfg;
            cfg.data = params;
            _algo_bbox->Configure(cfg);
        }
        else{
            std::string msg = name + " is not known to Supera...";
            throw meatloaf(msg);
        }

    }

    void Driver::ConfigureLabelAlgorithm(const std::string& name,
        const std::map<std::string,std::string>& params)
    {
        if(name == "LArTPCMLReco3D") {
            _algo_label = new LArTPCMLReco3D();
            PSet cfg;
            cfg.data = params;
            _algo_label->Configure(cfg);
        }
        else{
            std::string msg = name + " is not known to Supera...";
            throw meatloaf(msg);
        }
    }

    void Driver::GenerateLabel(const EventInput& data)
    {

        if(!_algo_bbox) 
            throw meatloaf("BBoxAlgorithm is not configured yet!");

        if(!_algo_label) 
            throw meatloaf("LabelAlgorithm is not configured yet!");

        _meta  = _algo_bbox->Generate(data);

        _label = _algo_label->Generate(data, _meta);

    }


}
#endif