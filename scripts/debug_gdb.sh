#!/usr/bin/env bash
# scripts/debug_gdb.sh
#
# Start QEMU in debug mode and listen on a port for GDB to attach. This script
# is a scaffold and should be adjusted to your preferred gdb invocation.

qemu-system-x86_64 -kernel kernel.bin -S -gdb tcp::1234
