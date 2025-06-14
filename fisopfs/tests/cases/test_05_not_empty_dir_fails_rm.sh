#!/bin/bash

MOUNT=tests/mount

mkdir "$MOUNT"/testdir
touch "$MOUNT"/testdir/testfile
rmdir "$MOUNT"/testdir
ls "$MOUNT"
ls "$MOUNT"/testdir