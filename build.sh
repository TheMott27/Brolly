#!/bin/bash
# Brolly build + publish script
# Builds .pbw, builds settings page, pushes both.
# NOTE: Pebble SDK requires patch version = 0, so we increment the MINOR.
# Versions: 2.0.0 -> 2.1.0 -> 2.2.0 -> 2.3.0 etc.
# Settings page uses its own v2.1.X versioning — do NOT overwrite with watchface version.
set -e

PEBBLE=~/.local/share/uv/tools/pebble-tool/bin/pebble
SETTINGS_DIR=~/Brolly_v2.0.0/settings
PAGES_DIR=~/Test_Brolly_v2_Settings

# ── 1. Bump minor version in package.json ─────────────────────────────────────
python3 - <<'PYEOF'
import json
with open('package.json') as f:
    d = json.load(f)
parts = d['version'].split('.')
parts[1] = str(int(parts[1]) + 1)
parts[2] = '0'
new_ver = '.'.join(parts)
d['version'] = new_ver
d['pebble']['versionLabel'] = new_ver
d['pebble']['longName'] = 'Brolly v' + new_ver
with open('package.json', 'w') as f:
    json.dump(d, f, indent=2)
print('Version bumped to: ' + new_ver)
PYEOF

VERSION=$(python3 -c "import json; print(json.load(open('package.json'))['version'])")
PBW_NAME="Brolly V${VERSION}.pbw"

# ── 2. Build .pbw ─────────────────────────────────────────────────────────────
echo "Building: ${PBW_NAME}"
$PEBBLE build
cp "build/Brolly_v2.0.0.pbw" "build/${PBW_NAME}"
echo "Built: build/${PBW_NAME}"

# ── 3. Build and publish settings page ────────────────────────────────────────
# Settings page uses its own v2.1.X versioning (already set in App.tsx).
echo "Building settings page..."
cd "$SETTINGS_DIR"
npm run build --silent
cp -r dist/* "$PAGES_DIR/"
cd "$PAGES_DIR"
# Get the current settings version from App.tsx for the commit message
SETTINGS_VER=$(grep "const VERSION = " "$SETTINGS_DIR/src/App.tsx" | sed "s/.*'\(.*\)'.*/\1/")
git add -A
git commit -m "${SETTINGS_VER}: settings page update" || echo "No settings changes"
git push origin main
echo "Settings page published (${SETTINGS_VER})."

# ── 4. Commit and push .pbw to Brolly repo ────────────────────────────────────
cd ~/Brolly_v2.0.0
git add -A
git commit -m "v${VERSION}: build ${PBW_NAME}"
git push origin master

# ── 5. Create version branch and push ─────────────────────────────────────────
BRANCH="release/v${VERSION}"
git checkout -b "${BRANCH}"
git push origin "${BRANCH}"
git checkout master
echo "Pushed branch: ${BRANCH}"

echo ""
echo "Done! ${PBW_NAME} built and pushed."
