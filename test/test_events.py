
bbox_cfg = {
    "BBoxSize": "[100, 100, 100]",
    "VoxelSize": "[1, 1, 1]",
    "BBoxBottom": "[0, 0, 0]",
    "WorldBoundBottom": "[0, 0, 0]",
    "WorldBoundTop": "[100, 100, 100]",
}

label_cfg = {
    "LogLevel": "VERBOSE",

    "UseSimEnergyDeposit":       "True",  # currently unused but required parameter
    "UseSimEnergyDepositPoints": "False", # ditto

}

def test_events():
    import supera

    driver = supera.Driver()
    driver.ConfigureBBoxAlgorithm("BBoxInteraction", bbox_cfg)
    driver.ConfigureLabelAlgorithm("LArTPCMLReco3D", label_cfg)

    for evnum, (evname, testev) in enumerate(supera.test.TestEvents().items()):
        driver.GenerateImageMeta(testev.input)
        driver.GenerateLabel(testev.input)

        if not supera.test.VerifyEventMeta(driver.Meta(), testev.output_meta) \
                or supera.test.VerifyEventLabels(driver.Label(), testev.output_labels):
            raise RuntimeError("Event #%d ('%s') failed test!" % (evnum, evname))
