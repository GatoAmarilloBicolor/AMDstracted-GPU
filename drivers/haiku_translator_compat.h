/*
 * Haiku Translator Compatibility Layer
 * 
 * This header provides compatibility with Haiku's translator add-ons.
 * It solves symbol resolution issues when linking with Haiku system libraries.
 */

#ifndef HAIKU_TRANSLATOR_COMPAT_H
#define HAIKU_TRANSLATOR_COMPAT_H

#include <kernel/OS.h>
#include <stdio.h>

/* Forward declarations for Haiku translator classes */
#ifdef __cplusplus
extern "C" {
#endif

/* Haiku Translator class stubs */
typedef struct {
    char reserved[256];  /* Reserved space for actual translator object */
} HaikuTranslator;

typedef struct {
    void (*get_description)(void);
    void (*identify)(void);
    void (*translate)(void);
} TranslatorVTable;

/* Compatibility wrapper for missing symbols */
#define HAIKU_TRANSLATOR_STUB(name) \
    void *get_##name(void) { \
        fprintf(stderr, "[COMPAT] Stub: get_" #name "\n"); \
        return NULL; \
    }

#ifdef __cplusplus
}
#endif

#endif /* HAIKU_TRANSLATOR_COMPAT_H */
