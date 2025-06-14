#!/bin/bash
set -euo pipefail

MOUNT=tests/mount

echo "hello world" > "$MOUNT"/testfile
cat < "$MOUNT"/testfile
