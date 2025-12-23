# build_flags.mk
#
# Compiler and Linker Configuration
#
# This file defines the specific flags used to build the kernel. It ensures:
# - Freestanding environment (no standard library dependencies).
# - Proper warnings and optimization levels.
# - Target architecture settings (x86_64).
#
# Included by the main Makefile.

CFLAGS += -Wall -Wextra -Wno-unused-parameter
LDFLAGS += -nostdlib

# Example: override CC for some developer environments
# CC ?= x86_64-elf-gcc
