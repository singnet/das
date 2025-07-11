#include "expression_hasher.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mbedtls/md5.h"

char* compute_hash(char* input) {
    unsigned char md5_buffer[16];
    char* hash = new char[HANDLE_HASH_SIZE];

    mbedtls_md5_context context;
    mbedtls_md5_init(&context);
    mbedtls_md5_starts(&context);
    mbedtls_md5_update(&context, (const unsigned char*) input, strlen(input));
    mbedtls_md5_finish(&context, md5_buffer);
    mbedtls_md5_free(&context);
    for (unsigned int i = 0; i < 16; i++) {
        sprintf((char*) ((unsigned long) hash + 2 * i), "%02x", md5_buffer[i]);
    }
    hash[32] = '\0';
    return hash;
}

char* named_type_hash(char* name) { return compute_hash(name); }

char* terminal_hash(char* type, char* name) {
    char hashable_string[MAX_HASHABLE_STRING_SIZE];

    if (strlen(type) + strlen(name) >= MAX_HASHABLE_STRING_SIZE) {
        fprintf(stderr, "Invalid (too large) terminal name");
        exit(1);
    }
    sprintf(hashable_string, "%s%c%s", type, JOINING_CHAR, name);
    return compute_hash(hashable_string);
}

char* composite_hash(char** elements, unsigned int nelements) {
    char hashable_string[MAX_HASHABLE_STRING_SIZE];
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
            strcpy((char*) (hashable_string + cursor), elements[i]);
        } else {
            sprintf((char*) (hashable_string + cursor), "%s%c", elements[i], JOINING_CHAR);
            cursor += 1;
        }
        cursor += element_size[i];
    }

    return compute_hash(hashable_string);
}

char* expression_hash(char* type_hash, char** elements, unsigned int nelements) {
    char* composite[nelements + 1];
    composite[0] = type_hash;
    for (unsigned int i = 0; i < nelements; i++) {
        composite[i + 1] = elements[i];
    }
    return composite_hash(composite, nelements + 1);
}
