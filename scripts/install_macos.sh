#!/usr/bin/env bash
# Explicit, user-invoked install only. No sudo/system-wide writes.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUNDLE="${1:-$ROOT/build-mac/McBoyPlugin_artefacts/Release/VST3/McBoy.vst3}"
DEST="$HOME/Library/Audio/Plug-Ins/VST3"
if [[ "$(uname -s)" != "Darwin" ]]; then echo 'This installer requires macOS' >&2; exit 1; fi
if [[ ! -d "$BUNDLE/Contents" ]]; then echo "Not a built VST3 bundle: $BUNDLE" >&2; exit 1; fi
mkdir -p "$DEST"
if [[ -e "$DEST/McBoy.vst3" ]]; then
  echo "Existing plug-in at $DEST/McBoy.vst3; move or back it up first." >&2
  exit 1
fi
ditto "$BUNDLE" "$DEST/McBoy.vst3"
echo "Installed $DEST/McBoy.vst3. Restart/rescan your VST3 host."
