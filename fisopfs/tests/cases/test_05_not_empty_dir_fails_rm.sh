#!/bin/bash

MOUNT=tests/mount

mkdir "$MOUNT"/testdir
touch "$MOUNT"/testdir/testfile
rmdir "$MOUNT"/testdir  2>&1
ls "$MOUNT"
ls "$MOUNT"/testdir