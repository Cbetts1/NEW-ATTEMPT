#ifndef KSTRING_H
#define KSTRING_H

/* =============================================================================
 * AI Aura OS — Kernel String Utilities
 * Shared string helpers used across all subsystems.
 * =============================================================================*/

#include <stddef.h>

int  strlen_k (const char *s);
void strncpy_k(char *dst, const char *src, int n);
int  strncmp_k(const char *a, const char *b, int n);

#endif /* KSTRING_H */
