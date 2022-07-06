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

#include <iostream>

// the methods for each THRESHOLD are duplicates.
// use a macro to eliminate some of the copy-paste
#define _SUPERA_INTERNAL_LOGGER_METHOD(threshold) \
    const Logger & threshold() const \
    { \
        if (_thresh >= THRESHOLD::threshold) \
            (*this) << (_preamble.empty() ? "" : _preamble) << (!_preamble.empty() ? " " : "") << #threshold << ": "; \
        return *this; \
    }

namespace supera {
    /// Rudimentary logger facility.  Currently only supports stdout...
    /// (would need some re-engineering to directly write to file
    ///  so that different instances didn't clobber one another)
    class Logger
    {
      public:
        enum class THRESHOLD { VERBOSE, DEBUG, INFO, WARNING, ERROR, FATAL };
        static THRESHOLD parseStringThresh(std::string threshStr);

        Logger(THRESHOLD thresh = THRESHOLD::INFO)
          : _thresh(thresh)
        {}

        THRESHOLD GetThreshold() const           { return _thresh; }
        void      SetThreshold(THRESHOLD thresh) { _thresh = thresh; }

        /// Force a write to the output.
        template <typename T>
        const Logger & operator<<(const T & obj) const
        {
            std::cout << obj;
            return *this;
        }

        _SUPERA_INTERNAL_LOGGER_METHOD(VERBOSE)
        _SUPERA_INTERNAL_LOGGER_METHOD(DEBUG)
        _SUPERA_INTERNAL_LOGGER_METHOD(INFO)
        _SUPERA_INTERNAL_LOGGER_METHOD(WARNING)
        _SUPERA_INTERNAL_LOGGER_METHOD(ERROR)
        _SUPERA_INTERNAL_LOGGER_METHOD(FATAL)

      private:
        std::string _preamble;  ///< write this in front of every message

        THRESHOLD _thresh;  ///<  current log threshold

    };

}
#endif //SUPERA_LOGGER_H

/** @} */ // end of doxygen group
