#!/usr/bin/env bash
# scripts/run_qemu.sh
#
# QEMU runner for NexaKernel. Adjust paths and file names if you change the
# kernel binary or disk layout. This file uses `qemu-system-x86_64` to boot
# the kernel for testing in a VM.

qemu-system-x86_64 -kernel kernel.bin -nographic
