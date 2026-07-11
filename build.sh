#!/bin/bash
# Brolly build + publish script
# Builds .pbw, builds settings page, pushes both.
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

# ── 2. Bump settings page version to match ────────────────────────────────────
SETTINGS_VER="v${VERSION}"
sed -i "s/const VERSION = '[^']*'/const VERSION = '${SETTINGS_VER}'/" "$SETTINGS_DIR/src/App.tsx"
sed -i "s/Brolly Settings Page — [^ ]*/Brolly Settings Page — ${SETTINGS_VER}/" "$SETTINGS_DIR/src/App.tsx"
echo "Settings version set to: ${SETTINGS_VER}"

# ── 3. Build .pbw ─────────────────────────────────────────────────────────────
echo "Building: ${PBW_NAME}"
$PEBBLE build
cp "build/Brolly_v2.0.0.pbw" "build/${PBW_NAME}"
echo "Built: build/${PBW_NAME}"

# ── 4. Build and publish settings page ────────────────────────────────────────
echo "Building settings page..."
cd "$SETTINGS_DIR"
npm run build --silent
cp -r dist/* "$PAGES_DIR/"
cd "$PAGES_DIR"
git add -A
git commit -m "${SETTINGS_VER}: settings page update" || echo "No settings changes"
git push origin main
echo "Settings page published."

# ── 5. Commit and push .pbw to Brolly repo ────────────────────────────────────
cd ~/Brolly_v2.0.0
git add -A
git commit -m "v${VERSION}: build ${PBW_NAME}"
git push origin master

echo ""
echo "Done! ${PBW_NAME} built and pushed."
