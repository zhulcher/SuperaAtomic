/**
 * \file Parser.h
 *
 * \ingroup base
 *
 * \brief Very rudimentary logger class.
 *
 * @author J. Wolcott <jwolcott@fnal.gov>
 */

/** \addtogroup base
    @{*/
#ifndef SUPERA_LOGGER_H
#define SUPERA_LOGGER_H


#include <cstdio>
#include <iostream>
#include <map>
#include "SuperaType.h"

namespace supera {

  /**
     \class Logger
     \brief Utility class used to show formatted message on the screen.
     A logger class for supera. Simply shows a formatted colored message on a screen. \n
     A static getter method is provided to create a sharable logger instance. \n
  */
  class Logger{
    
  public:
    
    /// Default constructor
    Logger(const std::string& name="no_name")
      : _ostrm(&std::cout)
      , _name(name)
    {}
    
    /// Default destructor
    virtual ~Logger(){};
    
  private:
    
    /// ostream
    std::ostream *_ostrm;
    
    /// Level
    msg::Level_t _level;
      
    /// Name
    std::string _name;
    
    /// Set of loggers
    static std::map<std::string,supera::Logger> *_logger_m;

    /// Shared logger
    static supera::Logger* _shared_logger;

    /// Default logger level
    static msg::Level_t _level_default;
    
  public:

    /// Logger's name
    const std::string& name() const { return _name; }

    /// Verbosity level setter
    void set(const msg::Level_t level) { _level = level; }

    /// Verbosity level getter
    msg::Level_t level() const { return _level; }

    /// Comparison operator for static collection of loggers
    inline bool operator<(const Logger& rhs) const
    {
      if(_name < rhs.name()) return true;
      if(_name > rhs.name()) return false;
      return false;
    }
    
    /// Getter of a message instance 
    static Logger& get(const std::string name)
    {
      if(!_logger_m) _logger_m = new std::map<std::string,supera::Logger>();
      auto iter = _logger_m->find(name);
      if(iter == _logger_m->end()) {
        iter = _logger_m->emplace(name,Logger(name)).first;
        iter->second.set(msg::kINFO);
      }
      return iter->second;
    };

    static Logger& get_shared();

    /// Default logger level getter
    static msg::Level_t default_level() { return _level_default; }
    /// Default logger level setter (only affect future loggers)
    static void default_level(msg::Level_t l) { _level_default = l; }
    /// Force all loggers to change level
    static void force_level(msg::Level_t l)
    {
      default_level(l);
      for(auto& name_logger : *_logger_m) name_logger.second.set(l);
    }
  
    //
    // Verbosity level checker
    //
    inline bool verbose () const { return _level <= msg::kVERBOSE; }
    inline bool debug   () const { return _level <= msg::kDEBUG;   }
    inline bool info    () const { return _level <= msg::kINFO;    }
    inline bool warning () const { return _level <= msg::kWARNING; }
    inline bool error   () const { return _level <= msg::kERROR;   }
    /// Formatted message (simplest)
    std::ostream& send(const msg::Level_t) const;
    /// Formatted message (function name included)
    std::ostream& send(const msg::Level_t level,
           const std::string& function ) const;
    /// Formatted message (function name + line number)
    std::ostream& send(const msg::Level_t level,
           const std::string& function,
           const unsigned int line_num ) const;
    /// Formatted message (function name + line number + file name)
    std::ostream& send(const msg::Level_t level,
           const std::string& function,
           const unsigned int line_num,
           const std::string& file_name) const;
    
  };
}

#define LOG_SVERBOSE()  if(supera::logger::get_shared().verbose())  supera::logger::get_shared().send(::supera::msg::kVERBOSE,  __FUNCTION__                  )
#define LOG_SDEBUG()    if(supera::logger::get_shared().debug())    supera::logger::get_shared().send(::supera::msg::kDEBUG,    __FUNCTION__,__LINE__,__FILE__)
#define LOG_SINFO()     if(supera::logger::get_shared().info())     supera::logger::get_shared().send(::supera::msg::kINFO,     __FUNCTION__,__LINE__         )
#define LOG_SWARNING()  if(supera::logger::get_shared().warning())  supera::logger::get_shared().send(::supera::msg::kWARNING,  __FUNCTION__                  )
#define LOG_SERROR()    if(supera::logger::get_shared().error())    supera::logger::get_shared().send(::supera::msg::kERROR,    __FUNCTION__,__LINE__         )
#define LOG_SFATAL() supera::logger::get_shared().send(::supera::msg::kFATAL, __FUNCTION__,__LINE__,__FILE__)

#endif //SUPERA_LOGGER_H

/** @} */ // end of doxygen group
