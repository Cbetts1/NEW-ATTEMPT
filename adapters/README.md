# AI Aura OS — Adapter Index
# File: adapters/README.md
#
# Adapters are plugins of type PLUGIN_TYPE_ADAPTER.  They virtualize
# external interfaces (filesystems, network, devices) entirely inside the OS.
#
# Current adapters:
#   aura_fs.c    — Virtual in-memory filesystem adapter
#   aura_net.c   — Virtual network adapter (stub, ready for extension)
#
# Adding a new adapter:
#   1. Create adapters/<name>.c exporting `plugin_descriptor_t <name>_adapter`.
#   2. Add to KERNEL_C_SRCS in the Makefile.
#   3. Register in kernel/kernel.c.
