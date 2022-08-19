#include "supera/test/TestEvents.h"

namespace supera
{
  namespace test
  {
    TestEvent PCMuonEvt()
    {
      // this file is the dump from the exporter
#include "PCMuon.ixx"

      supera::test::TestEvent ret;
      ret.input = std::move(evInput);
      ret.output_meta = std::move(meta);
      ret.output_labels = std::move(evtOutput);

      return ret;
    }
  }
}
