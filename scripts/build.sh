#!/usr/bin/env bash
# ===========================================================================
# scripts/build.sh
# ===========================================================================
#
# Build script for NexaKernel
#
# This script provides a convenient wrapper around make with additional
# functionality like checking for required tools and providing helpful
# error messages.
#
# Usage:
#   ./scripts/build.sh          # Build kernel
#   ./scripts/build.sh clean    # Clean build artifacts
#   ./scripts/build.sh rebuild  # Clean and rebuild
#   ./scripts/build.sh iso      # Build ISO image
#
# ===========================================================================

set -e

# Colors for output (if terminal supports it)
if [ -t 1 ]; then
    RED='\033[0;31m'
    GREEN='\033[0;32m'
    YELLOW='\033[1;33m'
    NC='\033[0m' # No Color
else
    RED=''
    GREEN=''
    YELLOW=''
    NC=''
fi

# Function to print colored messages
info() { echo -e "${GREEN}[INFO]${NC} $1"; }
warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
error() { echo -e "${RED}[ERROR]${NC} $1"; exit 1; }

# Check for required tools
check_tools() {
    info "Checking for required tools..."
    
    # Check for GCC
    if ! command -v gcc &> /dev/null; then
        error "GCC not found. Please install build-essential."
    fi
    
    # Check for 32-bit support (gcc-multilib)
    if ! gcc -m32 -E -x c /dev/null -o /dev/null 2>/dev/null; then
        error "32-bit GCC support not found. Please install gcc-multilib:"
        error "  sudo apt install gcc-multilib"
    fi
    
    # Check for NASM
    if ! command -v nasm &> /dev/null; then
        error "NASM not found. Please install NASM assembler:"
        error "  sudo apt install nasm"
    fi
    
    # Check for LD
    if ! command -v ld &> /dev/null; then
        error "LD not found. Please install binutils."
    fi
    
    info "Using compiler: gcc (native with -m32)"
    info "GCC version: $(gcc --version | head -n1)"
    info "NASM version: $(nasm -v)"
}

# Main
case "${1:-build}" in
    build)
        check_tools
        info "Building NexaKernel..."
        make all
        info "Build complete!"
        ;;
    clean)
        info "Cleaning build artifacts..."
        make clean
        info "Clean complete!"
        ;;
    rebuild)
        check_tools
        info "Rebuilding NexaKernel..."
        make clean
        make all
        info "Rebuild complete!"
        ;;
    iso)
        check_tools
        if ! command -v grub-mkrescue &> /dev/null; then
            warn "grub-mkrescue not found. ISO creation may fail."
        fi
        info "Building ISO image..."
        make iso
        info "ISO creation complete!"
        ;;
    *)
        echo "Usage: $0 [build|clean|rebuild|iso]"
        echo "  build   - Build kernel (default)"
        echo "  clean   - Remove build artifacts"
        echo "  rebuild - Clean and rebuild"
        echo "  iso     - Create bootable ISO"
        exit 1
        ;;
esac
