
/**
 * \file Configurable.h
 *
 * \ingroup base
 *
 * \brief Base class for a built-in logger
 *
 * @author K. Terao <kterao@slac.stanford.edu>
 */


#ifndef SUPERA_CONFIGURABLE_H
#define SUPERA_CONFIGURABLE_H

#include "yaml-cpp/yaml.h"

namespace supera
{
    /**
        \class Configurable
        Base class for providing configuration functionality
    */
    class Configurable
    {
      public:

        Configurable() {};

        virtual void ConfigureFromText(const std::string& yaml_text)
        { auto cfg = YAML::Load(yaml_text);     this->Configure(cfg); }

        virtual void ConfigureFromFile(const std::string& yaml_file)
        { auto cfg = YAML::LoadFile(yaml_file); this->Configure(cfg); }

        virtual void Configure(const YAML::Node& cfg) = 0;

    };

}
#endif //SUPERA_CONFIGURABLE_H
