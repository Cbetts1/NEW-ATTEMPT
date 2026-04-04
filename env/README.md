# AI Aura OS — Environment Bootstrap Notes
# File: env/README.md
#
# The `env/` directory holds runtime configuration constants and future
# environment descriptors for the OS's self-contained execution context.
#
# At build time, `aios.env` is a reference document; values are mirrored
# directly into the C headers and Makefile where needed.
#
# At runtime (inside the booted OS), the kernel reads its configuration
# from compiled-in constants — there is no filesystem-level config parser,
# keeping the environment fully self-contained.
