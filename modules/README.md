# AI Aura OS — Module Index
# File: modules/README.md
#
# This directory contains compiled-in kernel modules.
# Each module exports a `plugin_descriptor_t` symbol referenced by kernel.c.
#
# Current modules:
#   aura_core.c   — Core system health module (built-in)
#
# Adding a new module:
#   1. Create modules/<name>.c exporting `plugin_descriptor_t <name>_plugin`.
#   2. Add the source to KERNEL_C_SRCS in the Makefile.
#   3. Declare the extern and call plugin_register() in kernel/kernel.c.
