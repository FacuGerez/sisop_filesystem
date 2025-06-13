#!/bin/bash
set -euo pipefail

MOUNT=tests/mount

touch "$MOUNT"/testfile1
touch "$MOUNT"/testfile2
ls "$MOUNT"
rm "$MOUNT"/testfile*
ls "$MOUNT"