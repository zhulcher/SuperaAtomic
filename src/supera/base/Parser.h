
/**
 * \file Parser.h
 *
 * \ingroup base
 * 
 * \brief Functions to type-cast std::string to an appropriate type value, used to parse PSet for configuring algorithms.
 *
 * @author Kazu
 */

/** \addtogroup base
    @{*/
#ifndef __SUPERA_PARSER_H__
#define __SUPERA_PARSER_H__

#include <string>
#include <vector>
#include <algorithm>

namespace supera {
  namespace parser {
    template <class T>
    T FromString( const std::string& value );
    template<> std::string    FromString< std::string    > (const std::string& value );
    template<> float          FromString< float          > (const std::string& value );
    template<> double         FromString< double         > (const std::string& value );
    template<> short          FromString< short          > (const std::string& value );
    template<> int            FromString< int            > (const std::string& value );
    template<> long           FromString< long           > (const std::string& value );
    template<> unsigned short FromString< unsigned short > (const std::string& value );
    template<> unsigned int   FromString< unsigned int   > (const std::string& value );
    template<> unsigned long  FromString< unsigned long  > (const std::string& value );
    template<> bool           FromString< bool           > (const std::string& value );
    template<> std::vector< std::string    > FromString< std::vector< std::string    > > (const std::string& value );
    template<> std::vector< float          > FromString< std::vector< float          > > (const std::string& value );
    template<> std::vector< double         > FromString< std::vector< double         > > (const std::string& value );
    template<> std::vector< short          > FromString< std::vector< short          > > (const std::string& value );
    template<> std::vector< int            > FromString< std::vector< int            > > (const std::string& value );
    template<> std::vector< long           > FromString< std::vector< long           > > (const std::string& value );
    template<> std::vector< unsigned short > FromString< std::vector< unsigned short > > (const std::string& value );
    template<> std::vector< unsigned int   > FromString< std::vector< unsigned int   > > (const std::string& value );
    template<> std::vector< unsigned long  > FromString< std::vector< unsigned long  > > (const std::string& value );
    template<> std::vector< bool           > FromString< std::vector< bool           > > (const std::string& value );
 }
}

#endif
/** @} */ // end of doxygen group