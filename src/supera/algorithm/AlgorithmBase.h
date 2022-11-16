#ifndef __ALGORITHM_BASE_H__
#define __ALGORITHM_BASE_H__

#include "supera/base/Loggable.h"
#include "supera/base/Configurable.h"

namespace supera {

  /**
		\class AlgorithmBase
		The base class definition for algorithms.
	*/
	class AlgorithmBase : public Loggable
  , public Configurable {

  public:
    AlgorithmBase(std::string name="no_name") : Loggable(name) {}

    virtual ~AlgorithmBase() {}

    void Configure(const YAML::Node& cfg) override
    {
      if(cfg["LogLevel"])
      {
        this->SetLogConfig(supera::msg::parseStringThresh(cfg["LogLevel"].as<std::string>()));
      }
      this->_configure(cfg);
    }

  protected:

    virtual void _configure(const YAML::Node& cfg) = 0;
    
  };

}

#endif