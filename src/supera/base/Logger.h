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
        _isWriting = (THRESHOLD::threshold >= _thresh);      \
        (*this) << (_preamble.empty() ? "" : _preamble) << (!_preamble.empty() ? " " : "") << #threshold << ": "; \
        return *this;                         \
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
          : _thresh(thresh), _isWriting(false)
        {}

        THRESHOLD GetThreshold() const           { return _thresh; }
        void      SetThreshold(THRESHOLD thresh) { _thresh = thresh; }

        /// Write to the output, if we're currently doing that; otherwise, discard.
        /// (Set the state by getting the stream via one of the \ref VERBOSE(), \ref DEBUG(), etc. functions.)
        template <typename T>
        const Logger & operator<<(const T & obj) const
        {
          if (_isWriting)
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
        mutable bool _isWriting; ///<  current logging state: writing output or not?
    };
}
#endif //SUPERA_LOGGER_H

/** @} */ // end of doxygen group
