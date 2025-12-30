# ===========================================================================
# build_flags.mk
# ===========================================================================
#
# Compiler and Linker Configuration for NexaKernel
#
# This file defines additional flags and configuration options that can be
# used to customize the build process. It is included by the main Makefile.
#
# Configuration Options:
#   DEBUG=1          - Enable debug build (more symbols, less optimization)
#   VERBOSE=1        - Show full compiler commands
#   SCHEDULER=rr     - Use round-robin scheduler (default)
#   SCHEDULER=prio   - Use priority-based scheduler
#
# ===========================================================================

# ---------------------------------------------------------------------------
# Debug Configuration
# ---------------------------------------------------------------------------
ifdef DEBUG
  CFLAGS += -O0 -DDEBUG -g3
else
  CFLAGS += -O2 -DNDEBUG
endif

# ---------------------------------------------------------------------------
# Verbose Mode
# ---------------------------------------------------------------------------
ifndef VERBOSE
  Q := @
else
  Q :=
endif

# ---------------------------------------------------------------------------
# Scheduler Selection
# ---------------------------------------------------------------------------
ifeq ($(SCHEDULER),prio)
  CFLAGS += -DUSE_PRIORITY_SCHEDULER
else
  CFLAGS += -DUSE_ROUND_ROBIN_SCHEDULER
endif

# ---------------------------------------------------------------------------
# Architecture-Specific Flags
# ---------------------------------------------------------------------------
ARCH_CFLAGS = -march=i686 -mtune=generic

# ---------------------------------------------------------------------------
# Warning Flags (can be extended)
# ---------------------------------------------------------------------------
WARN_FLAGS = -Wall \
             -Wextra \
             -Wno-unused-parameter \
             -Wno-unused-function \
             -Wcast-align \
             -Wformat=2 \
             -Wredundant-decls

# ---------------------------------------------------------------------------
# Security/Hardening Flags (disabled for kernel)
# ---------------------------------------------------------------------------
# Note: Many security features are incompatible with bare-metal kernels
SECURITY_FLAGS = -fno-stack-protector \
                 -fno-pie \
                 -fno-exceptions

# ---------------------------------------------------------------------------
# Cross-Compiler Support (optional)
# ---------------------------------------------------------------------------
# Uncomment to use a cross-compiler (recommended for non-Linux hosts)
# CROSS_COMPILE ?= i686-elf-
# CC := $(CROSS_COMPILE)gcc
# LD := $(CROSS_COMPILE)ld
# OBJCOPY := $(CROSS_COMPILE)objcopy

# ---------------------------------------------------------------------------
# Additional Include Paths
# ---------------------------------------------------------------------------
# CFLAGS += -Ipath/to/additional/includes

# ---------------------------------------------------------------------------
# Build Timestamp
# ---------------------------------------------------------------------------
BUILD_DATE := $(shell date +%Y-%m-%d)
BUILD_TIME := $(shell date +%H:%M:%S)
CFLAGS += -DBUILD_DATE=\"$(BUILD_DATE)\" -DBUILD_TIME=\"$(BUILD_TIME)\"

# ---------------------------------------------------------------------------
# Version Information
# ---------------------------------------------------------------------------
VERSION_MAJOR := 0
VERSION_MINOR := 1
VERSION_PATCH := 0
CFLAGS += -DVERSION_MAJOR=$(VERSION_MAJOR) \
          -DVERSION_MINOR=$(VERSION_MINOR) \
          -DVERSION_PATCH=$(VERSION_PATCH)
