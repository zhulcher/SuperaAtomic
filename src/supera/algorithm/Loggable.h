/**
 * \file Loggable.h
 *
 * \ingroup base
 *
 * \brief Base class for 'algorithms' that want a built-in logger
 *
 * @author J. Wolcott <jwolcott@fnal.gov>
 */


#ifndef SUPERA_LOGGABLE_H
#define SUPERA_LOGGABLE_H

#include "supera/base/Logger.h"

namespace supera
{
    /**
		\class Loggable
		Base class for any algorithm that wants to inherit Logger functionality
	*/
	class Loggable
    {
      public:
        void SetLogConfig(Logger::THRESHOLD thresh)
        {
            LOG.SetThreshold(thresh);
        }

      protected:
        supera::Logger LOG;
    };

}

#endif //SUPERA_LOGGABLE_H
