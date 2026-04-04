/* =============================================================================
 * AI Aura OS — Kernel String Utilities Implementation
 * =============================================================================*/

#include "kstring.h"

int strlen_k(const char *s) {
    int n = 0;
    if (!s) return 0;
    while (s[n]) n++;
    return n;
}

void strncpy_k(char *dst, const char *src, int n) {
    int i = 0;
    if (!dst) return;
    if (src) {
        while (i < n - 1 && src[i]) { dst[i] = src[i]; i++; }
    }
    dst[i] = '\0';
}

int strncmp_k(const char *a, const char *b, int n) {
    if (!a || !b) return (a == b) ? 0 : -1;
    while (n-- > 0) {
        if (*a != *b) return (int)(unsigned char)*a - (int)(unsigned char)*b;
        if (*a == '\0') return 0;
        a++; b++;
    }
    return 0;
}
