#!/bin/sh
# Build the scene editor into build-editor/ (Release) with res/ next to it.
# Usage: scripts/build_editor.sh   then run: build-editor/editor
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
GENERATOR=""
command -v ninja >/dev/null 2>&1 && GENERATOR="-G Ninja"
cmake -S "$ROOT" -B "$ROOT/build-editor" $GENERATOR -DCMAKE_BUILD_TYPE=Release
cmake --build "$ROOT/build-editor" --target editor
echo "Editor ready: $ROOT/build-editor/editor"
