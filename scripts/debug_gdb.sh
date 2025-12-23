#!/usr/bin/env bash
# ===========================================================================
# scripts/debug_gdb.sh
# ===========================================================================
#
# GDB debugging helper for NexaKernel
#
# This script provides two modes:
#   1. Start QEMU with GDB server enabled
#   2. Connect GDB to a running QEMU instance
#
# Usage:
#   ./scripts/debug_gdb.sh qemu    # Start QEMU with GDB server
#   ./scripts/debug_gdb.sh gdb     # Connect GDB to QEMU
#   ./scripts/debug_gdb.sh         # Start QEMU (default)
#
# ===========================================================================

set -e

KERNEL_ELF="build/kernel.elf"
GDB_PORT=1234

# Function to start QEMU with GDB server
start_qemu() {
    echo "Starting QEMU with GDB server on port $GDB_PORT..."
    echo ""
    echo "To connect GDB, run in another terminal:"
    echo "  gdb -ex 'target remote localhost:$GDB_PORT' $KERNEL_ELF"
    echo ""
    echo "Or run: ./scripts/debug_gdb.sh gdb"
    echo ""
    echo "Useful GDB commands:"
    echo "  c          - continue execution"
    echo "  si         - step one instruction"
    echo "  break *0x100000 - breakpoint at kernel start"
    echo "  info registers  - show CPU registers"
    echo ""
    
    qemu-system-i386 \
        -kernel "$KERNEL_ELF" \
        -serial stdio \
        -no-reboot \
        -no-shutdown \
        -s -S
}

# Function to connect GDB to QEMU
connect_gdb() {
    if [ ! -f "$KERNEL_ELF" ]; then
        echo "Error: Kernel ELF not found at $KERNEL_ELF"
        exit 1
    fi
    
    # Check for GDB
    if ! command -v gdb &> /dev/null; then
        echo "Error: GDB not found. Please install gdb:"
        echo "  sudo apt install gdb"
        exit 1
    fi
    
    echo "Connecting to QEMU on localhost:$GDB_PORT..."
    gdb "$KERNEL_ELF" \
        -ex "target remote localhost:$GDB_PORT" \
        -ex "set disassembly-flavor intel" \
        -ex "set architecture i386"
}

# Main
case "${1:-qemu}" in
    qemu)
        start_qemu
        ;;
    gdb)
        connect_gdb
        ;;
    *)
        echo "Usage: $0 [qemu|gdb]"
        echo "  qemu - Start QEMU with GDB server (default)"
        echo "  gdb  - Connect GDB to running QEMU"
        exit 1
        ;;
esac
