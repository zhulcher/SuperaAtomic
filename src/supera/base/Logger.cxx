#include <algorithm>

#include "supera/base/Logger.h"
#include "supera/base/meatloaf.h"

namespace supera {
    Logger::THRESHOLD Logger::parseStringThresh(std::string threshStr)
    {
        std::for_each(threshStr.begin(), threshStr.end(), [](char &ch){ch = ::toupper(ch); });

        if (threshStr == "VERBOSE")
            return THRESHOLD::VERBOSE;
        else if (threshStr == "DEBUG")
            return THRESHOLD::DEBUG;
        else if (threshStr == "INFO")
            return THRESHOLD::INFO;
        else if (threshStr == "WARNING")
            return THRESHOLD::WARNING;
        else if (threshStr == "ERROR")
            return THRESHOLD::ERROR;
        else if (threshStr == "FATAL")
            return THRESHOLD::FATAL;

        throw meatloaf("Unrecognized log threshold: " + threshStr);
    }
}
