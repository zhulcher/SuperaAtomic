#ifndef SUPERA_TESTEVENTS_H
#define SUPERA_TESTEVENTS_H

#include <map>
#include <string>

#include "supera/data/ImageMeta3D.h"
#include "supera/data/Particle.h"

namespace supera
{
  namespace test
  {
    /// Collection of input & expected output information for testing machinery
    struct TestEvent
    {
      supera::EventInput  input;

      /// known expected metadata that should be derived from the input
      supera::ImageMeta3D output_meta;

      /// known expected particle labels (etc.) that will be derived from the input if the labeler is working correctly
      supera::EventOutput output_labels;
    };

    // --------------------------------------------

    /// Validate that the computed metadata match expected metadata
    bool VerifyEventMeta(const supera::ImageMeta3D& computedMeta, const supera::ImageMeta3D& expectedMeta);

    /// Validate that the computed event labels match the expected ones
    bool VerifyEventLabels(const supera::EventOutput& computedLabels, const supera::EventOutput& expectedLabels);

    // --------------------------------------------

    TestEvent NumuCCIncEvt();

    /// Stockpile of test events with known (expected) outputs
    std::map<std::string, TestEvent> TestEvents();
  }
}

#endif //SUPERA_TESTEVENTS_H
