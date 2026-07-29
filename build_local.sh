#!/bin/bash
# Brolly local build script for Pebble Round
set -e

PEBBLE=~/.local/share/uv/tools/pebble-tool/bin/pebble

# ── 1. Build .pbw ─────────────────────────────────────────────────────────────
echo "Building Brolly for all platforms including Chalk..."
$PEBBLE build

VERSION=$(python3 -c "import json; print(json.load(open('package.json'))['version'])")
PBW_NAME="Brolly_v${VERSION}_Circular.pbw"
cp "build/Brolly_v2.0.0.pbw" "build/${PBW_NAME}"

echo "Built: build/${PBW_NAME}"
echo "Local build complete. No changes pushed to GitHub or Settings."
