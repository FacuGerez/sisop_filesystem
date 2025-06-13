#!/bin/bash
set -euo pipefail

MOUNT=tests/mount

cat "$MOUNT"/moms_spaghetti 2> "$MOUNT"/errorfile
ls "$MOUNT"
cat "$MOUNT"/errorfile