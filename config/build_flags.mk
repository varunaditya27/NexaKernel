# build_flags.mk
# Centralized compiler/linker flags for the NexaKernel build system.
# Add any special flags or toolchain overrides here.

CFLAGS += -Wall -Wextra -Wno-unused-parameter
LDFLAGS += -nostdlib

# Example: override CC for some developer environments
# CC ?= x86_64-elf-gcc
