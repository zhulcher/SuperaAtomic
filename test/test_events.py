
bbox_cfg = {
    "BBoxSize": "[ 740,  320,  530]",
    "VoxelSize": "[0.4,   0.4,  0.4]",
    "BBoxBottom": "[-370, -160,  400]",
}

label_cfg = {
    "LogLevel": "VERBOSE",

    "EnergyDepositThreshold":  "0",
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
