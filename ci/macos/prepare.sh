#!/bin/bash
set -euo pipefail
brew install qt open-scene-graph xerces-c ninja
brew list --versions qt open-scene-graph xerces-c ninja > artifacts/dependencies.txt
mkdir -p "$RUNNER_TEMP/gmat-deps"
curl --fail --location --retry 3 --output "$RUNNER_TEMP/gmat-deps/cspice.tar.Z" \
  https://naif.jpl.nasa.gov/pub/naif/toolkit/C/MacIntel_OSX_AppleC_64bit/packages/cspice.tar.Z
shasum -a 256 "$RUNNER_TEMP/gmat-deps/cspice.tar.Z" | tee artifacts/cspice-download.sha256
tar -xf "$RUNNER_TEMP/gmat-deps/cspice.tar.Z" -C "$RUNNER_TEMP/gmat-deps"
file "$RUNNER_TEMP/gmat-deps/cspice/lib/cspice.a" | tee artifacts/cspice-library.txt
