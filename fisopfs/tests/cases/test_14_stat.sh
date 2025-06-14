#!/bin/bash

MOUNT=tests/mount

touch "$MOUNT"/testfile
stat "$MOUNT"/testfile > "$MOUNT"/statfile
cat "$MOUNT"/statfile | sed 's/^[[:space:]]*//' | cut -d ':' -f1