#!/usr/bin/env bash
set -e

BUILD=".pio/build/m5stack-sticks3"

if [ ! -f "$BUILD/firmware.bin" ]; then
  echo "No build found — running pio run first..."
  pio run -e m5stack-sticks3
fi

echo "Copying firmware binaries to docs/..."
cp "$BUILD/bootloader.bin"  docs/bootloader.bin
cp "$BUILD/partitions.bin"  docs/partition-table.bin
cp "$BUILD/firmware.bin"    docs/firmware.bin

echo "Serving docs/ at http://localhost:8000/setup.html"
cd docs && python3 -m http.server 8000
