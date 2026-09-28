#!/usr/bin/env python3
"""Opt-in numerical regressions for four maintained public documentation examples."""
import argparse
from datetime import datetime
import json
import math
from pathlib import Path
import re
import shutil

from support import Context, passed, run

ARCH = Path('doc/SystemDocs/ArchitecturalSpecification/script')
FORCE = Path('doc/help/src/files/scripts/ForceModelsTutorial.script')
SENSOR = Path('doc/SystemDocs/ComponentDesigns/SensorAccess/source/scripts/R2020a_BasicSensors.script')


def numeric_rows(path):
    rows = []
    for line in path.read_text().splitlines():
        try:
            row = [float(v) for v in line.split()]
        except ValueError:
            continue
        if row:
            assert all(math.isfinite(v) for v in row), path
            rows.append(row)
    return rows


def contacts(path):
    dates = re.compile(r'\d{2} [A-Z][a-z]{2} \d{4} \d{2}:\d{2}:\d{2}\.\d+')
    return [[datetime.strptime(v, '%d %b %Y %H:%M:%S.%f') for v in dates.findall(line)]
            for line in path.read_text().splitlines() if len(dates.findall(line)) == 2]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('build_dir', type=Path)
    p.add_argument('--label', default='public-example-numerics')
    p.add_argument('--mode', choices=('all', 'satsep', 'global', 'force', 'sensor'), default='all')
    args = p.parse_args()
    ctx = Context(args.build_dir, args.label)
    # Unique evidence labels prevent stale reports from making a failed run pass.
    result = ctx.work/'results.json'
    if any(ctx.work.iterdir()):
        p.error('Choose a fresh --label; existing results are retained')
    ctx.runtime()
    binary = ctx.build_gui('ExampleSweep.cpp')
    startup = ctx.work/'startup-examples.txt'
    startup.write_text(ctx.startup.read_text()+'\n'+'\n'.join(
        line for line in ctx.of_plugins if 'libOVtoOFI' not in line)+'\n')
    evidence = {}

    def execute(name, script):
        path = ctx.work/(name+'.script')
        path.write_text(script)
        env = {**ctx.of_environment, 'GMAT_EXAMPLE_SCRIPT': str(path),
               'GMAT_EXAMPLE_LIMIT': '300', 'GMAT_GUI_TEST_IMAGES': str(ctx.work),
               'LP_NUM_THREADS': '2', 'OSG_NUM_PROCESSORS': '2'}
        log = run(['xvfb-run', '-a', '-s', '-screen 0 1280x900x24', str(binary),
                   '--startup_file', str(startup), '--no_splash'],
                  cwd=ctx.work, env=env, log=ctx.work/(name+'.log'), timeout=350)
        assert 'AUDIT_RUN_END result=1 ' in log and 'AUDIT_CLOSE_RETURN' in log, name
        assert 'AUDIT_VIEWPORT match=0' not in log and 'AUDIT_ACTIVE match=0' not in log, name

    if args.mode in ('all', 'satsep'):
        for name in ('SatSep.gmf', 'dot.gmf'):
            shutil.copy2(ctx.root/ARCH/name, ctx.work/name)
        cases = [(1, 0, 0), (0, 2, 0), (0, 0, -3), (3, 4, 12), (-3, -4, -12), (0, 0, 0)]
        script = '''Create Spacecraft Sat1 Sat2;
Sat1.DisplayStateType = Cartesian;
Sat1.X = 7000;
Sat1.Y = 0;
Sat1.Z = 0;
Sat2.DisplayStateType = Cartesian;
Create Variable dx dy dz dr;
Create ReportFile values;
values.Filename = 'satsep.txt';
values.Precision = 16;
values.WriteHeaders = Off;
BeginMissionSequence;
Global dx dy dz;
'''
        for x, y, z in cases:
            script += f'''Sat2.X = {7000+x};
Sat2.Y = {y};
Sat2.Z = {z};
[dr] = SatSep(Sat1, Sat2);
Report values dx dy dz dr;
'''
        execute('satsep', script)
        rows = numeric_rows(ctx.work/'satsep.txt')
        expected = [[*v, math.sqrt(sum(x*x for x in v))] for v in cases]
        assert len(rows) == len(expected) and all(len(r) == 4 for r in rows), rows
        error = max(abs(a-b) for r, e in zip(rows, expected) for a, b in zip(r, e))
        assert error < 1e-10, (rows, expected)
        evidence['satsep'] = {'cases': rows, 'max_error_km': error}
        passed('axis, mixed, negative and coincident satellite separations')

    if args.mode in ('all', 'global'):
        shutil.copy2(ctx.root/ARCH/'RaiseApogee.gmf', ctx.work/'RaiseApogee.gmf')
        script = '''Create ImpulsiveBurn globalBurn;
Create Spacecraft globalSat;
globalSat.DisplayStateType = Cartesian;
globalSat.X = 7000;
globalSat.Y = 0;
globalSat.Z = 0;
globalSat.VX = 0;
globalSat.VY = 7.7;
globalSat.VZ = 0;
Create Variable index;
Create ReportFile values;
values.Filename = 'global.txt';
values.Precision = 16;
values.WriteHeaders = Off;
BeginMissionSequence;
Global globalBurn globalSat;
Report values globalSat.VX globalSat.VY globalSat.VZ globalSat.Earth.SMA globalSat.Earth.ECC;
For index = 1 : 4
   RaiseApogee(index);
   Report values globalSat.VX globalSat.VY globalSat.VZ globalSat.Earth.SMA globalSat.Earth.ECC;
EndFor;
'''
        execute('global', script)
        rows = numeric_rows(ctx.work/'global.txt')
        assert len(rows) == 5 and all(len(r) == 5 for r in rows), rows
        speeds = [math.sqrt(sum(v*v for v in r[:3])) for r in rows]
        errors = [abs(speeds[i]-speeds[i-1]-i/10) for i in range(1, 5)]
        apogees = [r[3]*(1+r[4]) for r in rows]
        assert max(errors) < 1e-12, speeds
        assert all(a < b for a, b in zip(apogees, apogees[1:])), apogees
        assert max(abs(r[3]*(1-r[4])-7000) for r in rows) < 1e-8, rows
        evidence['global'] = {'speeds_km_s': speeds, 'apogee_radii_km': apogees,
                              'max_burn_error_km_s': max(errors)}
        passed('four global-object burns change velocity and raise apogee')

    if args.mode in ('all', 'force'):
        script = (ctx.root/FORCE).read_text()
        execute('force', script)
        original = ctx.work/'Ex_ForceModels.report'
        rows = numeric_rows(original)
        assert len(rows) == 7 and all(len(r) == 7 for r in rows), rows
        assert all(abs(b[0]-a[0]-.1) < 1e-9 for a, b in zip(rows, rows[1:])), rows
        reference = script.replace("'Ex_ForceModels.report'", "'ForceReference.report'")
        reference = reference.replace('.MaxStep = 30;', '.MaxStep = 15;')
        reference = reference.replace('.InitialStepSize = 30;', '.InitialStepSize = 15;')
        execute('force-half-step', reference)
        smaller = numeric_rows(ctx.work/'ForceReference.report')
        assert len(smaller) == len(rows) and all(len(r) == 7 for r in smaller), smaller
        position_error = max(abs(a-b) for r, s in zip(rows, smaller) for a, b in zip(r[1:4], s[1:4]))
        velocity_error = max(abs(a-b) for r, s in zip(rows, smaller) for a, b in zip(r[4:], s[4:]))
        assert position_error < 1e-5 and velocity_error < 1e-8, (position_error, velocity_error)
        evidence['force'] = {'report_rows': len(rows), 'max_half_step_position_error_km': position_error,
                             'max_half_step_velocity_error_km_s': velocity_error}
        passed('configured output path, six propagation stages and step-halving agreement')

    if args.mode in ('all', 'sensor'):
        script = (ctx.root/SENSOR).read_text()
        # A separate station with identical geometry and no FOV provides the
        # circular 10-degree elevation reference on the very same trajectory.
        station = re.search(r'Create GroundStation TheCape;.*?MinimumElevationAngle = 10;', script, re.S)[0]
        locator = re.search(r'Create ContactLocator ContactsAtTheCape;.*?LightTimeDirection = Transmit;', script, re.S)[0]
        reference = station.replace('TheCape', 'CapeReference')+'\n'+locator.replace(
            'ContactsAtTheCape', 'ReferenceContacts').replace('TheCape', 'CapeReference').replace(
            "'CapeContacts.txt'", "'ReferenceContacts.txt'")+'\n'
        execute('sensor-reference', script.replace('BeginMissionSequence', reference+'BeginMissionSequence'))
        actual = contacts(ctx.work/'CapeContacts.txt')
        expected = contacts(ctx.work/'ReferenceContacts.txt')
        assert len(actual) == len(expected) == 30, (actual, expected)
        errors = [abs((a-b).total_seconds()) for r, s in zip(actual, expected) for a, b in zip(r, s)]
        # An inscribed 1-degree polygon lies at most 0.000374 degrees above
        # the circular horizon. Check all event endpoints, not just the count.
        elevation_error = math.degrees(math.atan(math.tan(math.radians(10))/math.cos(math.radians(.5))))-10
        assert elevation_error < .0004
        assert max(errors) < 1, max(errors)
        evidence['sensor'] = {'contacts': len(actual), 'max_endpoint_difference_seconds': max(errors),
                              'max_polygon_elevation_error_degrees': elevation_error}
        passed('30-day sensor contacts agree with circular horizon within one second')
    result.write_text(json.dumps(evidence, indent=2)+'\n')
    print(result)


if __name__ == '__main__':
    main()
