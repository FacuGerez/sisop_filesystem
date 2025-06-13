#!/bin/bash
set -euo pipefail

MOUNT=tests/mount

touch "$MOUNT"/testfile
ls "$MOUNT"
unlink "$MOUNT"/testfile
ls "$MOUNT"