
def test_events():
    import sys

    import supera

    driver = supera.Driver()
    driver.ConfigureBBoxAlgorithm("BBoxInteraction")
    driver.ConfigureLabelAlgorithm("LArTPCMLReco3D", {"LogLevel", "ERROR"})

    for evnum, (evname, testev) in enumerate(supera.test.kTestEvents.items()):
        driver.GenerateImageMeta(testev.input)
        driver.GenerateLabel(testev.inputev)

        if not supera.test.VerifyEventMeta(driver.Meta(), testev.output_meta) \
                or supera.test.VerifyEventLabels(driver.Label(), testev.output_labels):
            print("Event #%d ('%s') failed test!" % (evnum, evname), file=sys.stderr)
            return 1;

    return 0;
