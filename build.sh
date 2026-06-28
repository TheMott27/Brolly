#!/bin/bash
# Brolly build script
# NOTE: Pebble SDK requires patch version = 0, so we increment the MINOR.
# Versions: 2.0.0 -> 2.1.0 -> 2.2.0 -> 2.3.0 etc.
# .pbw files are named "Brolly V2.x.0.pbw"
# Each build is pushed to master AND its own backup branch "release/v2.x.0"

set -e

# ── 1. Increment minor version ────────────────────────────────────────────────
python3 - <<'PYEOF'
import json

with open('package.json') as f:
    d = json.load(f)

parts = d['version'].split('.')
parts[1] = str(int(parts[1]) + 1)
parts[2] = '0'  # patch must always be 0 for Pebble SDK
new_ver = '.'.join(parts)

d['version'] = new_ver
d['pebble']['versionLabel'] = new_ver
d['pebble']['longName'] = 'Brolly v' + new_ver

with open('package.json', 'w') as f:
    json.dump(d, f, indent=2)

print('Version bumped to: ' + new_ver)
PYEOF

# ── 2. Get new version string ─────────────────────────────────────────────────
VERSION=$(python3 -c "import json; print(json.load(open('package.json'))['version'])")
PBW_NAME="Brolly V${VERSION}.pbw"
echo "Building: ${PBW_NAME}"

# ── 3. Build ──────────────────────────────────────────────────────────────────
~/.local/share/uv/tools/pebble-tool/bin/pebble clean
~/.local/share/uv/tools/pebble-tool/bin/pebble build

# ── 4. Copy .pbw with correct name ───────────────────────────────────────────
cp "build/Brolly_v2.0.0.pbw" "build/${PBW_NAME}"
echo "Built: build/${PBW_NAME}"

# ── 5. Commit and push to master ─────────────────────────────────────────────
git add -f "build/${PBW_NAME}" package.json build.sh 2>/dev/null || true
git add -f settings/src/App.tsx settings/src/defaults.ts src/c/main.c src/pkjs/index.js 2>/dev/null || true
git add -A 2>/dev/null || true
git commit -m "v${VERSION}: build ${PBW_NAME}"
git push origin master

# ── 6. Create version branch and push ────────────────────────────────────────
BRANCH="release/v${VERSION}"
git checkout -b "${BRANCH}"
git push origin "${BRANCH}"
git checkout master
echo "Pushed branch: ${BRANCH}"

echo ""
echo "Done! ${PBW_NAME} pushed to master and branch ${BRANCH}"
