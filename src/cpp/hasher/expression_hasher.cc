#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mbedtls/md5.h"
#include "expression_hasher.h"

static unsigned char MD5_BUFFER[16];
static char HASH[HANDLE_HASH_SIZE];
static char HASHABLE_STRING[MAX_HASHABLE_STRING_SIZE];

char *compute_hash(char *input) {
    mbedtls_md5_context context;
    mbedtls_md5_init(&context);
    mbedtls_md5_starts_ret(&context);
    mbedtls_md5_update_ret(&context, (const unsigned char*) input, strlen(input));
    mbedtls_md5_finish_ret(&context, MD5_BUFFER);
    mbedtls_md5_free(&context);
    for (unsigned int i = 0; i < 16; i++) {
        sprintf((char *) ((unsigned long) HASH + 2 * i), "%02x", MD5_BUFFER[i]);
    }
    HASH[32] = '\0';
    return strdup(HASH);
}

char *named_type_hash(char *name) {
    return compute_hash(name);
}

char *terminal_hash(char *type, char *name) {
    if (strlen(type) + strlen(name) >= MAX_HASHABLE_STRING_SIZE) {
        fprintf(stderr, "Invalid (too large) terminal name");
        exit(1);
    }
    sprintf(HASHABLE_STRING, "%s%c%s", type, JOINING_CHAR, name);
    return compute_hash(HASHABLE_STRING);
}

char *composite_hash(char **elements, unsigned int nelements) {

    unsigned int total_size = 0;
    unsigned int element_size[nelements];

    for (unsigned int i = 0; i < nelements; i++) {
        unsigned int size = strlen(elements[i]);
        if (size > MAX_LITERAL_OR_SYMBOL_SIZE) {
            fprintf(stderr, "Invalid (too large) composite elements");
            exit(1);
        }
        element_size[i] = size;
        total_size += size;
    }
    if (total_size >= MAX_HASHABLE_STRING_SIZE) {
        fprintf(stderr, "Invalid (too large) composite elements");
        exit(1);
    }

    unsigned long cursor = 0;
    for (unsigned int i = 0; i < nelements; i++) {
        if (i == (nelements - 1)) {
            strcpy((char *) (HASHABLE_STRING + cursor), elements[i]);
        } else {
            sprintf((char *) (HASHABLE_STRING + cursor), "%s%c", elements[i], JOINING_CHAR);
            cursor += 1;
        }
        cursor += element_size[i];
    }

    return compute_hash(HASHABLE_STRING);
}

char *expression_hash(char *type_hash, char **elements, unsigned int nelements) {
    char *composite[nelements + 1];
    composite[0] = type_hash;
    for (unsigned int i = 0; i < nelements; i++) {
        composite[i + 1] = elements[i];
    }
    return composite_hash(composite, nelements + 1);
}
