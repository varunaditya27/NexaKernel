#!/usr/bin/env bash
# scripts/build.sh
#
# Build wrapper for NexaKernel used in simple development environments. This
# script should call `make` and perform any extra preflight checks.

set -e
make
