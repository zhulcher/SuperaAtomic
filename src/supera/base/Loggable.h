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

        inline const supera::Logger& GetLogger() const { return LOG; }
/*
        inline void verbose (const std::string& msg) 
        { if( LOG.verbose() ) LOG.strm(::supera::msg::kVERBOSE, __FUNCTION__); }
        inline void debug   (const std::string& msg) { LOG.strm();}
        inline void info    (const std::string& msg) { LOG.strm();}
        inline void warning (const std::string& msg) { LOG.strm();}
        inline void error   (const std::string& msg) { LOG.strm();}
*/

      protected:
        supera::Logger LOG;
    };

}

#define LOG_VERBOSE()  if( LOG.verbose () ) LOG.strm(::supera::msg::kVERBOSE,  __FUNCTION__, __LINE__          )
#define LOG_DEBUG()    if( LOG.debug   () ) LOG.strm(::supera::msg::kDEBUG,    __FUNCTION__, __LINE__          )
#define LOG_INFO()     if( LOG.info    () ) LOG.strm(::supera::msg::kINFO,     __FUNCTION__                    )
#define LOG_WARNING()  if( LOG.warning () ) LOG.strm(::supera::msg::kWARNING,  __FUNCTION__                    )
#define LOG_ERROR()    if( LOG.error   () ) LOG.strm(::supera::msg::kERROR,    __FUNCTION__, __LINE__          )
#define LOG_FATAL()                         LOG.strm(::supera::msg::kFATAL,    __FUNCTION__, __LINE__, __FILE__)

#endif //SUPERA_LOGGABLE_H
