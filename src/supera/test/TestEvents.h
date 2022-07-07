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
    struct TestEvent
    {
      supera::EventInput  input;
      supera::ImageMeta3D output_meta;
      supera::EventOutput output_labels;
    };

    // --------------------------------------------

    bool VerifyEventMeta(const supera::ImageMeta3D& computedMeta, const supera::ImageMeta3D& expectedMeta);

    bool VerifyEventLabels(const supera::EventOutput& computedLabels, const supera::EventOutput& expectedLabels);

    // --------------------------------------------

    TestEvent NumuCCIncEvt();

    /// Stockpile of test events with known (expected) outputs
    std::map<std::string, TestEvent> TestEvents();
  }
}

#endif //SUPERA_TESTEVENTS_H
