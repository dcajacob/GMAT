#!/usr/bin/env python3
"""Check GroundTrack component lookup, partial publications and separate propagation."""
from support import context, passed

if __name__ == '__main__':
    ctx = context('groundtrack-state')
    ctx.runtime()
    binary = ctx.build_gui('GroundTrackStateRegression.cpp',
        extra_link=['-Wl,--wrap=_ZN16GroundTrackCurve7AddDataEddd'])
    ctx.gui(binary, 'state')
    passed('GroundTrack state mapping and missing-point handling')
    script = (ctx.source/'GuiSmoke.script').read_text().replace('BeginMissionSequence;', '''Create Spacecraft SecondSC;
SecondSC.RAAN = 90;
Create GroundTrackPlot StateTrack;
StateTrack.Add = {BuildTestSat, SecondSC};
BeginMissionSequence;''')
    script += '\nPropagate BuildTestProp(SecondSC) {SecondSC.ElapsedSecs = 60};\n'
    fixture = ctx.work/'separate-spacecraft.script'
    fixture.write_text(script)
    output = ctx.gui(ctx.application/'bin/GMAT', 'separate-spacecraft',
        arguments=['--run',str(fixture),'--exit'])
    if 'Mission run completed.' not in output:
        raise RuntimeError('Separate-spacecraft mission did not complete')
    passed('GroundTrack mission with separately propagated spacecraft')
