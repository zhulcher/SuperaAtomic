
bbox_cfg = {
    "VoxelSize": "[0.4,   0.4,  0.4]",
    "BBoxSize":   "[ 1000,  1000,  1000 ]",   # cm
    "BBoxTop":    "[ 500,  500,  500 ]",   # cm
    "BBoxBottom": "[ -500, -500,  -500 ]"   # cm
}

label_cfg = {
    "LogLevel": "INFO",

    "EnergyDepositThreshold":  "0",
    "UseSimEnergyDeposit":       "True",  # currently unused but required parameter
    "UseSimEnergyDepositPoints": "False", # ditto

}

def prop_compare(exp, recv, indent=0):
    all_ok = True
    for prop_name in dir(exp):
#        print("   property name:", prop_name)
        prop_exp = getattr(exp, prop_name)
        prop_recv = getattr(recv, prop_name)
        if hasattr(getattr(exp, prop_name), "__call__"):
            continue

        if prop_exp == prop_recv:
            continue

        all_ok = False
        indent_txt = ("  " * indent)
        print(indent_txt, "   --> Mismatch in property '%s':" % prop_name)
        if prop_name in ("energy", "dedx"):
            for txt, label in {"Expected": exp, "Received": recv}.items():
                print(txt, getattr(label, prop_name).size(), "voxels summing to", getattr(label, prop_name).sum())
        elif prop_name == "part":
            prop_compare(getattr(exp, prop_name), getattr(recv, prop_name), indent+1)

        else:
            print(indent_txt, "     Expected:", prop_exp.dump() if hasattr(prop_exp, "dump") else prop_exp)
            print(indent_txt, "     Received:", prop_recv.dump() if hasattr(prop_recv, "dump") else prop_recv)

    print("all ok:", all_ok)

def test_events():
    import supera

    driver = supera.Driver()
    driver.ConfigureBBoxAlgorithm("BBoxInteraction", bbox_cfg)
    driver.ConfigureLabelAlgorithm("LArTPCMLReco3D", label_cfg)

    for evnum, (evname, testev) in enumerate(supera.test.TestEvents().items()):
        driver.GenerateImageMeta(testev.input)
        driver.GenerateLabel(testev.input)

        same = True
        if not supera.test.VerifyEventMeta(driver.Meta(), testev.output_meta):
            same = False
            print("Metadata for test event '%s' disagrees with expectation." % evname)
            print("Expected metadata:")
            print(testev.output_meta.dump())
            print("Metadata received:")
            print(driver.Meta().dump())

        if not supera.test.VerifyEventLabels(driver.Label(), testev.output_labels):
            same = False
            print("Labels for test event '%s' disagree with expectation." % evname)

            for i, (label_exp, label_recv) in enumerate(zip(testev.output_labels.particles, driver.Label().particles)):
                print(" label", i)
                if label_exp != label_recv:
                    print("label #%d disagrees:" % i)

                    prop_compare(label_exp, label_recv)

        if not same:
            raise RuntimeError("Event #%d ('%s') failed test!" % (evnum, evname))


if __name__ == "__main__":
    test_events()
