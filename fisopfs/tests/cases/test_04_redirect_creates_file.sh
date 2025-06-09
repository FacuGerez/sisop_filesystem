#!/bin/bash
set -euo pipefail

MOUNT=tests/mount

echo "hello_world" > "$MOUNT"/testfile
ls "$MOUNT"
