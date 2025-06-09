#!/bin/bash
set -euo pipefail

MOUNT=tests/mount

mkdir "$MOUNT"/testdir
ls "$MOUNT"
mkdir "$MOUNT"/testdir/nested_dir
ls "$MOUNT"/testdir
