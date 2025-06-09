#!/bin/bash
set -euo pipefail

MOUNT=tests/mount

echo "hola" > "$MOUNT"/testfile
ls "$MOUNT"
