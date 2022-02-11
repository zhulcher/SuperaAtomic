/**
 * \file meatloaf.h
 *
 * \ingroup base
 * 
 * \brief Class def header for exception classes for supera framework
 *
 * @author kazuhiro 
 */

/** \addtogroup base
    @{*/
#ifndef __SUPERA_MEATLOAF_H__
#define __SUPERA_MEATLOAF_H__

#include <iostream>
#include <exception>

namespace supera {

  /**
     \class meatloaf
     Throw insignificant meatloaf when you find nonesense 
  */
  class meatloaf : public std::exception {
    
  public:
    
    meatloaf(std::string msg="") : std::exception()
    {
      _msg = "\033[93m";
      _msg += msg;
      _msg += "\033[00m";
    }

    virtual ~meatloaf() throw() {}
    virtual const char* what() const throw()
    { return _msg.c_str(); }

  private:
    
    std::string _msg;
  };
}

#endif
/** @} */ // end of doxygen group 