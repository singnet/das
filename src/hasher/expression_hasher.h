#pragma once

#define JOINING_CHAR ((char) ' ')
#define MAX_LITERAL_OR_SYMBOL_SIZE ((size_t) 10000)
#define MAX_HASHABLE_STRING_SIZE ((size_t) 100000)
#define HANDLE_HASH_SIZE ((unsigned int) 33)

#define PREFIX_LENGTH ((unsigned int) 1)
#define NODE_HANDLE_PREFIX ((char) 'n')
#define LINK_HANDLE_PREFIX ((char) 'l')

// PREFIX + (HANDLE - '\0')
#define ATOM_HANDLE_LENGTH ((unsigned int) (PREFIX_LENGTH + (HANDLE_HASH_SIZE - 1)))

char* compute_hash(char* input);
char* named_type_hash(char* name);
char* terminal_hash(char* type, char* name);
char* expression_hash(char* type_hash, char** elements, unsigned int nelements);
char* composite_hash(char** elements, unsigned int nelements);
