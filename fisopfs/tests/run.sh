#!/bin/bash
set -euo pipefail

verbose=false

while [[ "$#" -gt 0 ]]; do
	case "$1" in
	-v | --verbose) verbose=true ;;
	*) echo "Unknown option: $1" ;;
	esac
	shift
done

log() {
	if $verbose; then
		echo "[verbose] $*"
	fi
}

cleanup() {
	if mountpoint -q "$MOUNT"; then
		log "Unmounting $MOUNT..."
		umount "$MOUNT" 2>/dev/null || true
	fi
	if [ -n "${FS_PID:-}" ]; then
		log "Killing FS process $FS_PID"
		kill "$FS_PID" 2>/dev/null || true
		wait "$FS_PID" 2>/dev/null || true
	fi
}
trap cleanup EXIT

TESTDIR=tests

RED="\033[31m"
GREEN="\033[32m"
RESET="\033[0m"

MOUNT="$TESTDIR/mount"
CASES="$TESTDIR/cases"
EXPECTED="$TESTDIR/expected"
OUTPUT="$TESTDIR/output"
DISK="$TESTDIR/testdisk.fisopfs"
FS_BINARY=./fisopfs

rm -rf "$MOUNT" "$OUTPUT"
mkdir -p "$MOUNT" "$OUTPUT"
rm -f "$DISK"

log "Mounting filesystem..."
$FS_BINARY --filedisk "$DISK" "$MOUNT" &
FS_PID=$!

for _ in {1..10}; do
	if mountpoint -q "$MOUNT"; then
		break
	fi
	sleep 0.5
done

if ! mountpoint -q "$MOUNT"; then
	log -e "${RED}Filesystem did not mount properly${RESET}"
	exit 1
fi

EXIT_CODE=0

for script in "$CASES"/test_*.sh; do
	base=$(basename "$script" .sh)
	out="$OUTPUT/${base}_out.txt"
	exp="$EXPECTED/${base}_expected.txt"

	log "Running $base"
	if ! bash "$script" >"$out"; then
		log -e "${RED}$base: script exited with non-zero status${RESET}"
	fi

	log "Comparing $out to $exp"
	if diff_out=$(diff -u "$exp" "$out"); then
		echo -e "${GREEN}$base PASSED${RESET}"
	else
		echo -e "${RED}$base FAILED${RESET}"
		EXIT_CODE=1
	fi
	log "$diff_out"

	log "Cleaning test directory..."
	find "$MOUNT" -mindepth 1 -exec rm -rf {} +
done

exit $EXIT_CODE
