#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
HOT=false

for arg in "$@"; do
  case $arg in
    --hot) HOT=true ;;
  esac
done

if [ "$HOT" = true ]; then
  echo "=== Hot-reload mode ==="
  echo "  Vite dev server will run on http://localhost:3000"
  echo "  JUCE app will load from the dev server (HMR enabled)"
  echo ""

  echo "=== Building JUCE app (Debug + USE_DEV_SERVER) ==="
  cd "$SCRIPT_DIR/Builds/MacOSX"
  xcodebuild -project NewProject.xcodeproj \
    -configuration Debug \
    GCC_PREPROCESSOR_DEFINITIONS="\$(inherited) USE_DEV_SERVER=1" \
    build | grep -E '(BUILD|error:|warning:.*WebUI|Compil|Link)'

  echo ""
  echo "=== Starting Vite dev server ==="
  cd "$SCRIPT_DIR/webui"
  npm run dev &
  VITE_PID=$!

  # Wait for Vite to be ready
  echo "  Waiting for Vite on port 3000..."
  for i in $(seq 1 30); do
    if curl -s http://localhost:3000 > /dev/null 2>&1; then
      echo "  Vite is ready!"
      break
    fi
    sleep 0.5
  done

  echo ""
  echo "=== Launching app ==="
  cd "$SCRIPT_DIR/Builds/MacOSX"
  open build/Debug/NewProject.app

  echo ""
  echo "Press Ctrl+C to stop Vite and exit."
  trap "kill $VITE_PID 2>/dev/null; exit" INT TERM
  wait $VITE_PID
else
  echo "=== Building Vue frontend ==="
  cd "$SCRIPT_DIR/webui"
  npm run build

  echo ""
  echo "=== Building JUCE app (Debug) ==="
  cd "$SCRIPT_DIR/Builds/MacOSX"
  xcodebuild -project NewProject.xcodeproj -configuration Debug build 2>&1 | tail -5

  echo ""
  echo "=== Launching app ==="
  open build/Debug/NewProject.app
fi
