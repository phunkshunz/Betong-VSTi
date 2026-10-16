#!/bin/bash
# Installerar maskinerna som ligger i den här mappen.
# Kör från Terminal:  bash installera.command
set -e
cd "$(dirname "$0")"

VST3="$HOME/Library/Audio/Plug-Ins/VST3"
AU="$HOME/Library/Audio/Plug-Ins/Components"
mkdir -p "$VST3" "$AU"

shopt -s nullglob
vst3=( *.vst3 )
components=( *.component )
apps=( *.app )

if [ ${#vst3[@]} -eq 0 ] && [ ${#components[@]} -eq 0 ]; then
  echo "Hittar inga plugin i mappen. Packade du upp zip-filen först?"
  exit 1
fi

for p in "${vst3[@]}"; do
  rm -rf "$VST3/$p"
  cp -R "$p" "$VST3/"
  echo "  $p  ->  $VST3"
done

for c in "${components[@]}"; do
  rm -rf "$AU/$c"
  cp -R "$c" "$AU/"
  echo "  $c  ->  $AU"
done

# Pluginen är osignerade, så macOS spärrar dem. Detta tar bort spärren.
for p in "${vst3[@]}"; do xattr -dr com.apple.quarantine "$VST3/$p" 2>/dev/null || true; done
for c in "${components[@]}"; do xattr -dr com.apple.quarantine "$AU/$c" 2>/dev/null || true; done
for a in "${apps[@]}"; do xattr -dr com.apple.quarantine "$a" 2>/dev/null || true; done

echo ""
echo "Klart! Maskinerna ligger nu i:"
echo "  $VST3"
echo "  $AU"
if [ ${#apps[@]} -gt 0 ]; then
  echo "Den fristående appen kan du köra direkt härifrån mappen."
fi
echo ""
echo "Starta om Bitwig, eller tryck Rescan under Settings > Locations > Plug-ins."
echo "Sök på BETONG eller SMIDET. Båda ligger under tillverkaren Phunkshunz."
