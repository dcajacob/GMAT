#!/bin/bash
set -euo pipefail
# Extend only this disposable runner checkout with a CI-only target. Product
# sources remain at the published commit; preserve the exact generated diff.
printf '\ninclude("${PROJECT_SOURCE_DIR}/ci/macos/target.cmake")\n' >> src/qtgui/CMakeLists.txt
git diff -- src/qtgui/CMakeLists.txt > artifacts/ci-target-integration.patch
python3 - <<'PY'
import re
from pathlib import Path
names=re.findall(r'OPTION\((PLUGIN_\w+)',Path('plugins/CMakeLists.txt').read_text(),re.I)
Path('artifacts/minimal-plugins.cmake').write_text(''.join(f'set({n} OFF CACHE BOOL "Bounded core-only smoke" FORCE)\n' for n in names))
PY
cmake -S . -B build/macos-smoke -G Ninja -C artifacts/minimal-plugins.cmake \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=x86_64 -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 \
  -DCMAKE_PREFIX_PATH="$(brew --prefix qt);$(brew --prefix open-scene-graph);$(brew --prefix xerces-c)" \
  -DCSPICE_DIR="$RUNNER_TEMP/gmat-deps/cspice" \
  -DGMAT_INCLUDE_GUI=OFF -DGMAT_INCLUDE_QT_GUI=ON -DGMAT_QT_BUILD_TESTS=OFF \
  -DGMAT_INCLUDE_API=OFF -DGMAT_INCLUDE_CSALT=OFF
