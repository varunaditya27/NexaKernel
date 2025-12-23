#!/usr/bin/env bash
# ===========================================================================
# scripts/run_qemu.sh
# ===========================================================================
#
# QEMU runner for NexaKernel
#
# This script launches the kernel in QEMU for testing. It supports both
# direct kernel loading (faster) and ISO boot (tests GRUB integration).
#
# Usage:
#   ./scripts/run_qemu.sh          # Direct kernel boot
#   ./scripts/run_qemu.sh --iso    # Boot from ISO
#   ./scripts/run_qemu.sh --debug  # Start with GDB server
#
# ===========================================================================

set -e

# Configuration
KERNEL_ELF="build/kernel.elf"
ISO_FILE="nexakernel.iso"
QEMU="qemu-system-i386"

# Default QEMU options
QEMU_OPTS="-serial stdio -no-reboot -no-shutdown"

# Check if QEMU is available
if ! command -v "$QEMU" &> /dev/null; then
    # Try x86_64 variant
    QEMU="qemu-system-x86_64"
    if ! command -v "$QEMU" &> /dev/null; then
        echo "Error: QEMU not found. Please install qemu-system-i386."
        exit 1
    fi
fi

# Parse arguments
case "${1:-}" in
    --iso)
        if [ ! -f "$ISO_FILE" ]; then
            echo "Error: ISO file not found. Run 'make iso' first."
            exit 1
        fi
        echo "Booting NexaKernel from ISO..."
        $QEMU -cdrom "$ISO_FILE" $QEMU_OPTS
        ;;
    --debug)
        if [ ! -f "$KERNEL_ELF" ]; then
            echo "Error: Kernel not found. Run 'make' first."
            exit 1
        fi
        echo "Starting NexaKernel in debug mode..."
        echo "Connect GDB with: target remote localhost:1234"
        $QEMU -kernel "$KERNEL_ELF" $QEMU_OPTS -s -S
        ;;
    *)
        if [ ! -f "$KERNEL_ELF" ]; then
            echo "Error: Kernel not found. Run 'make' first."
            exit 1
        fi
        echo "Starting NexaKernel..."
        $QEMU -kernel "$KERNEL_ELF" $QEMU_OPTS
        ;;
esac
