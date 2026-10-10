#!/bin/bash
# Installerar BETONG. Kör från Terminal:  bash installera.command
set -e
cd "$(dirname "$0")"

VST3="$HOME/Library/Audio/Plug-Ins/VST3"
AU="$HOME/Library/Audio/Plug-Ins/Components"
mkdir -p "$VST3" "$AU"

rm -rf "$VST3/BETONG.vst3" "$AU/BETONG.component"
cp -R BETONG.vst3 "$VST3/"
cp -R BETONG.component "$AU/"

# Pluginet är osignerat, så macOS spärrar det. Detta tar bort spärren.
xattr -dr com.apple.quarantine "$VST3/BETONG.vst3" "$AU/BETONG.component" BETONG.app 2>/dev/null || true

echo ""
echo "Klart! BETONG ligger nu i:"
echo "  $VST3"
echo "Starta om Bitwig, eller tryck Rescan under Settings > Locations > Plug-ins."
