from pathlib import Path
import os, re, subprocess, tempfile
# Run after building GmatQt, GmatGUI and GmatConsole on Linux.
repo = Path(__file__).resolve().parents[3]
bindir = repo / 'application/bin'
evidence = repo / 'doc/DevelopersDocs/Qt6ParityValidation'
with tempfile.TemporaryDirectory(prefix='gmat polyhedron shared ') as scratch:
    root = Path(scratch)
    startup = (bindir / 'gmat_startup_qt.txt').read_text()
    values = {'ROOT_PATH': str(repo / 'application') + '/', 'OUTPUT_PATH': str(root) + '/', 'PERSONALIZATION_FILE': str(root / 'MyGmat.ini')}
    for name, value in values.items():
        startup = re.sub(r'^' + name + r'\s*=.*$', name + ' = ' + value, startup, flags=re.M)
    (root / 'startup.txt').write_text(startup)
    (root / 'MyGmat.ini').write_text('[Main]\nShowWelcomeOnStart=false\n')
    script = root / 'shared dispatch.script'
    shape = root / 'cube.txt'
    shape.write_text('8\n1 -10 -10 -10\n2 10 -10 -10\n3 10 10 -10\n4 -10 10 -10\n5 -10 -10 10\n6 10 -10 10\n7 10 10 10\n8 -10 10 10\n12\n1 1 3 2\n2 1 4 3\n3 5 6 7\n4 5 7 8\n5 1 5 8\n6 1 8 4\n7 2 3 7\n8 2 7 6\n9 1 2 6\n10 1 6 5\n11 4 8 7\n12 4 7 3\n')
    source = """% Independent body-qualified polyhedron configuration, shared frontends.
Create Spacecraft Sat;
Sat.DisplayStateType = Cartesian;
Sat.X = 100;
Sat.Y = 50;
Sat.Z = 30;
Sat.VX = 0;
Sat.VY = 0;
Sat.VZ = 0;
Create ForceModel FM;
FM.PointMasses = {Sun};
FM.PolyhedralBodies = {Earth, Mars};
FM.PolyhedronGravityModel.Earth.CreateForceBody = Earth;
FM.PolyhedronGravityModel.Earth.ShapeFileName = 'SHAPE_PATH';
FM.PolyhedronGravityModel.Earth.BodyDensity = 2000;
FM.PolyhedronGravityModel.Mars.CreateForceBody = Mars;
FM.PolyhedronGravityModel.Mars.ShapeFileName = 'SHAPE_PATH';
FM.PolyhedronGravityModel.Mars.BodyDensity = 1000;
Create Propagator P;
P.FM = FM;
P.Accuracy = 1e-12;
P.InitialStepSize = 1;
P.MaxStep = 1;
Create ReportFile Values;
Values.WriteHeaders = false;
Values.Precision = 16;
Values.Filename = 'REPORT_PATH';
BeginMissionSequence;
Propagate P(Sat) {Sat.ElapsedSecs = 60};
Report Values Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z Sat.EarthMJ2000Eq.VX Sat.EarthMJ2000Eq.VY Sat.EarthMJ2000Eq.VZ;
""".replace('SHAPE_PATH', str(shape))
    reports = []
    for frontend in ['GmatConsole', 'GMAT', 'GmatQt']:
        report = root / (frontend + '.txt')
        script.write_text(source.replace('REPORT_PATH', str(report)))
        if frontend == 'GmatQt':
            args = ['xvfb-run', '-a', str(bindir / frontend), '--startup', str(root / 'startup.txt'), '--settings-dir', str(root / 'qt-settings'), '--run', '--screenshot', str(root / 'qt.png'), str(script)]
        elif frontend == 'GMAT':
            args = ['xvfb-run', '-a', str(bindir / frontend), '--startup_file', str(root / 'startup.txt'), '--no_splash', '--run', str(script), '--exit']
        else:
            args = [str(bindir / frontend), '--startup_file', str(root / 'startup.txt'), '--run', str(script), '--exit']
        env = dict(os.environ, QT_QPA_PLATFORM='xcb', LIBGL_ALWAYS_SOFTWARE='1')
        result = subprocess.run(args, cwd=bindir, env=env, capture_output=True, timeout=45)
        (evidence / ('polyhedron-shared-' + frontend.lower() + '.txt')).write_bytes(result.stdout + result.stderr)
        assert result.returncode == 0, (frontend, result.returncode, (result.stdout + result.stderr)[-4000:])
        content = report.read_bytes()
        assert content.strip() and len(content.splitlines()) == 1 and len(content.split()) == 6, (frontend, content)
        reports.append(content)
        (evidence / ('polyhedron-shared-' + frontend.lower() + '.report.txt')).write_bytes(content)
        print(frontend + ': completed, six expected state columns')
    assert reports[0] == reports[1] == reports[2], reports
    print('PASS: rebuilt Console, wx and Qt run the same Earth/Mars body-qualified polyhedron fixture with byte-exact six-column state reports. Temporary startup/output/preferences isolated; existing force and propagation algorithms unchanged.')
