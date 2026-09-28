"""Build and launch focused Linux regressions using a completed Ninja build."""
import argparse
import os
from pathlib import Path
import shlex
import signal
import subprocess

BASE = '[Main]\nShowWelcomeOnStart=false\n'


def run(command, *, cwd, log=None, env=None, timeout=60):
    process = subprocess.Popen(command, cwd=cwd, env=env, start_new_session=True,
                               stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    timed_out = False
    try:
        output, _ = process.communicate(timeout=timeout)
    except subprocess.TimeoutExpired:
        timed_out = True
        # Kill the test's process group, including its virtual display and GUI.
        # Killing only xvfb-run leaves a hung GMAT process behind.
        os.killpg(process.pid, signal.SIGTERM)
        try:
            output, _ = process.communicate(timeout=5)
        except subprocess.TimeoutExpired:
            os.killpg(process.pid, signal.SIGKILL)
            output, _ = process.communicate()
    if log:
        log.write_text(output)
    if timed_out or process.returncode or 'FAIL:' in output:
        reason = 'timeout' if timed_out else str(process.returncode)
        raise RuntimeError(f'Command failed ({reason}): {command[0]}\n{output[-6000:]}')
    return output



class Context:
    def __init__(self, build, suite):
        self.build = build.resolve()
        self.source = Path(__file__).resolve().parent
        self.cache = {}
        for line in (self.build/'CMakeCache.txt').read_text().splitlines():
            if '=' in line and not line.startswith(('#', '//')):
                key, value = line.split('=', 1)
                self.cache[key.split(':', 1)[0]] = value
        self.root = Path(self.cache['CMAKE_HOME_DIRECTORY'])
        self.application = Path(self.cache['GMAT_BUILDOUTPUT_DIRECTORY'])
        self.work = self.build/'linux-gui-tests'/suite
        self.work.mkdir(parents=True, exist_ok=True)
        self.commands = run(['ninja', '-t', 'commands', 'GmatGUI'], cwd=self.build).splitlines()
        compile_line = next(c for c in self.commands if ' -c ' in c and c.endswith('/app/GmatApp.cpp'))
        self.flags = shlex.split(compile_line)
        self.flags = self.flags[:self.flags.index('-MD')]
        self.environment = {**os.environ, 'G_DEBUG': 'fatal-criticals', 'LIBGL_ALWAYS_SOFTWARE': '1',
                            'GDK_BACKEND': 'x11', 'GDK_SCALE': '1'}

    def build_gui(self, source, *, extra_flags=(), extra_link=()):
        obj = self.work/(Path(source).stem+'.o')
        run(self.flags+list(extra_flags)+['-g', '-c', str(self.source/source), '-o', str(obj)], cwd=self.build)
        original = str(self.application/'bin'/('GMAT-'+self.cache['GMAT_RELEASE_NAME']))
        link_line = next(c for c in self.commands if ' -o '+original+' ' in c)
        link = shlex.split(link_line.split('&&')[1])
        link = [a for a in link if not a.startswith('-Wl,--dependency-file=')]
        binary = self.work/Path(source).stem
        link[link.index('-o')+1] = str(binary)
        run(link+['-Wl,--wrap=main', str(obj)]+list(extra_link), cwd=self.build)
        return binary

    def runtime(self):
        self.config = self.work/'personalization.ini'
        self.config.write_text(BASE)
        template = (self.root/'application/bin/gmat_startup_file_mac_linux.public.txt').read_text()
        lines, self.of_plugins, disabled = [], [], []
        for line in template.splitlines():
            key, separator, value = line.partition('=')
            key = key.strip()
            if separator and key == 'PLUGIN':
                plugin = self.application/'plugins'/value.strip().split('../plugins/')[-1]
                if not plugin.with_suffix('.so').exists():
                    disabled.append(plugin.name)
                    continue
                line = 'PLUGIN = '+str(plugin)
                if plugin.name in ('libOpenFramesInterface', 'libOVtoOFI'):
                    self.of_plugins.append(line)
                    continue
            elif separator and key == 'ROOT_PATH':
                line = key+' = '+str(self.root/'application')+'/'
            elif separator and key == 'OUTPUT_PATH':
                line = key+' = '+str(self.work)+'/'
            elif separator and key == 'PERSONALIZATION_FILE':
                line = key+' = '+str(self.config)
            lines.append(line)
        self.startup = self.work/'startup.txt'
        self.startup.write_text('\n'.join(lines)+'\nWRITE_PERSONALIZATION_FILE = ON\n')
        (self.work/'disabled-plugins.txt').write_text('\n'.join(disabled)+'\n')
        libs, formats = [], []
        for key in ('OPENFRAMES_DIR', 'OSG_DIR', 'OSGEARTH_DIR'):
            if self.cache.get(key):
                for leaf in ('lib', 'lib64'):
                    directory = Path(self.cache[key])/leaf
                    if directory.is_dir():
                        libs.append(str(directory))
                        formats.extend(str(p) for p in directory.glob('osgPlugins-*'))
        self.of_environment = {**self.environment,
            'LD_LIBRARY_PATH': ':'.join(libs+[os.environ.get('LD_LIBRARY_PATH', '')]),
            'OSG_LIBRARY_PATH': ':'.join(formats),
            'OSG_FILE_PATH': os.environ.get('OSG_FILE_PATH', '/usr/share/fonts/truetype/msttcorefonts')}
        # External OFI TimeDilator may set a tooltip before its native Create().
        # Preserve its warning in the log but allow tests of actual exit behavior.
        self.of_environment.pop('G_DEBUG', None)
        self.of_startup = self.work/'startup-openframes.txt'
        self.of_startup.write_text(self.startup.read_text()+'\n'+'\n'.join(self.of_plugins)+'\n')

    def gui(self, binary, name, *, mode='bounds', screen='1280x900x24',
            startup=None, environment=None, arguments=()):
        env = {**(environment or self.environment), 'GMAT_GUI_TEST_MODE': mode,
               'GMAT_GUI_TEST_IMAGES': str(self.work)}
        return run(['xvfb-run', '-a', '-s', '-screen 0 '+screen, str(binary),
                    '--startup_file', str(startup or self.startup), '--no_splash', *arguments],
                   cwd=self.work, env=env, log=self.work/(name+'.log'))


def context(suite):
    parser = argparse.ArgumentParser(description='Run '+suite+' regressions against a completed Ninja build.')
    parser.add_argument('build_dir', type=Path)
    return Context(parser.parse_args().build_dir, suite)


def passed(message):
    print('PASS: '+message, flush=True)
