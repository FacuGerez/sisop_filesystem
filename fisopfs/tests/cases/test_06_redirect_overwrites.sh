#!/bin/bash
set -euo pipefail

MOUNT=tests/mount

echo "hello wordl" > "$MOUNT"/testfile
echo "goodbye" > "$MOUNT"/testfile
cat "$MOUNT"/testfile