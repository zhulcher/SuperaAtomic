
/*
 *
 * \ingroup base
 * 
 * \brief Class def header for a class supera::PSet
 *
 * @author kazuhiro
 */

/** \addtogroup base
    @{*/

#ifndef __PSet_H__
#define __PSet_H__

#include <map>
#include "supera/base/meatloaf.h"
#include "Parser.h"

namespace supera {

    class PSet {
    public:

        PSet() {}
        ~PSet() {}

        /// Template getter
        template <class T>
        T get(const std::string& key) const{
            auto iter = data.find(key);
            if( iter == data.end() ) {
                std::string msg;
                msg = "Key does not exist: \"" + key + "\"\n";
                throw meatloaf(msg);
            }
            return parser::FromString<T>((*iter).second);
        }

        /// Template getter w/ default value
        template <class T>
        T get(const std::string& key, const T default_value) const{
            auto iter = data.find(key);
            if( iter == data.end() )
                return default_value;
            return parser::FromString<T>((*iter).second);
        }

    public:
        std::map<std::string,std::string> data;

    };

}
#endif
