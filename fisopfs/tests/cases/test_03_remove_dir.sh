#!/bin/bash
set -euo pipefail

MOUNT=tests/mount

mkdir "$MOUNT"/testdir
ls "$MOUNT"
rmdir "$MOUNT"/testdir
ls "$MOUNT"
