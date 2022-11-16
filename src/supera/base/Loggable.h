/**
 * \file Loggable.h
 *
 * \ingroup base
 *
 * \brief Base class for a built-in logger
 *
 * @author J. Wolcott <jwolcott@fnal.gov>
 */


#ifndef SUPERA_LOGGABLE_H
#define SUPERA_LOGGABLE_H

#include "Logger.h"
#include "yaml-cpp/yaml.h"

namespace supera
{
    /**
		\class Loggable
		Base class for any algorithm that wants to inherit Logger functionality
	*/
	class Loggable
    {
      public:

        Loggable(std::string name="no_name") : LOG(name) {};

        virtual ~Loggable() {}

        void SetLogConfig(msg::Level_t thresh)
        { LOG.set(thresh); }

      protected:
        supera::Logger LOG;
    };

}

#define LOG_VERBOSE()  if( LOG.verbose () ) LOG.send(::supera::msg::kVERBOSE,  __FUNCTION__, __LINE__          )
#define LOG_DEBUG()    if( LOG.debug   () ) LOG.send(::supera::msg::kDEBUG,    __FUNCTION__, __LINE__          )
#define LOG_INFO()     if( LOG.info    () ) LOG.send(::supera::msg::kINFO,     __FUNCTION__                    )
#define LOG_WARNING()  if( LOG.warning () ) LOG.send(::supera::msg::kWARNING,  __FUNCTION__                    )
#define LOG_ERROR()    if( LOG.error   () ) LOG.send(::supera::msg::kERROR,    __FUNCTION__, __LINE__          )
#define LOG_FATAL()                         LOG.send(::supera::msg::kFATAL,    __FUNCTION__, __LINE__, __FILE__)

#endif //SUPERA_LOGGABLE_H
