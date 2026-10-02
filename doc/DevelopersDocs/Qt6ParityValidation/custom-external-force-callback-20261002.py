"""Owned single-spacecraft Cartesian qualification callback; km and seconds."""

AX = 0.001
AY = -0.0002
AZ = 0.0003
_reported = False


def GetConstantAcceleration(state, epoch_a1_mjd, state_names, order):
    global _reported
    if len(state) != 6 or order != 1.0:
        raise ValueError("Owned fixture requires one six-state first-order Cartesian propagation")
    if not _reported:
        print("QT_EXTERNAL_CONSTANT_ORIGIN=" + __file__)
        _reported = True
    # ODEModel fills dx/dt from state velocity; this contributor supplies dv/dt.
    return [0.0, 0.0, 0.0, AX, AY, AZ]
