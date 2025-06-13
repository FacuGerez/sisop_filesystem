#!/bin/bash
set -euo pipefail

MOUNT=tests/mount

echo "first" > "$MOUNT"/testfile
echo "second" >> "$MOUNT"/testfile
cat "$MOUNT"/testfile