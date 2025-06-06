#!/bin/bash
set -euo pipefail

TESTDIR=tests

RED="\033[31m"
GREEN="\033[32m"
RESET="\033[0m"

MOUNT="$TESTDIR"/mount
CASES="$TESTDIR"/cases
EXPECTED="$TESTDIR"/expected
OUTPUT="$TESTDIR"/output
DISK="$TESTDIR"/testdisk.fisopfs
FS_BINARY=./fisopfs

mkdir -p "$MOUNT"
mkdir -p "$OUTPUT"
rm -f "$DISK"

echo "Mounting filesystem..."
$FS_BINARY --filedisk "$DISK" "$MOUNT" &
sleep 1

EXIT_CODE=0

for script in "$CASES"/test_*.sh; do
  base=$(basename "$script" .sh)
  out=$OUTPUT/${base}_out.txt
  exp=$EXPECTED/${base}_expected.txt

  echo "Running $base"
  bash "$script" >"$out"

  echo "Comparing $out to $exp"
  if diff -u "$exp" "$out"; then
    echo -e "$GREEN$base PASSED$RESET"
  else
    echo -e "$RED$base FAILED$RESET"
    EXIT_CODE=1
  fi

  echo "Cleaning test directory..."
  find "$MOUNT" -mindepth 1 -exec rm -rf {} +
done

echo "Unmounting filesystem..."
umount "$MOUNT"

exit $EXIT_CODE
