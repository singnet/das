/**
 * @file expression_hasher.h
 * @brief Header file for the ExpressionHasher class, providing utilities for generating MD5 hashes
 *        for various types of expressions.
 *
 * This file contains the definition of the ExpressionHasher class, which offers static methods to
 * compute MD5 hashes for different types of expressions, including named types, terminal expressions,
 * composite expressions, and general expressions. The class leverages the mbedtls library for MD5
 * hashing operations.
 *
 * The ExpressionHasher class is designed to handle strings and lists of strings as input for hashing.
 * It ensures that the generated hashes are consistent and unique for different expressions, making it
 * useful for scenarios where expression uniqueness and integrity are critical.
 *
 * @note The class uses a maximum hashable string size of 100,000 characters and a joining character
 *       of a single space (' ') for concatenating elements before hashing.
 *
 * @remark The class throws exceptions in cases where hashing operations fail or input constraints
 *         are violated, ensuring robust error handling.
 *
 * Dependencies:
 * - mbedtls/md5.h: For MD5 hashing functions.
 */
#pragma once

#include <stdexcept>
#include <string>

#include "type_aliases.h"

using namespace std;

namespace atomdb {

#define JOINING_CHAR ((char) ' ')
#define MAX_HASHABLE_STRING_SIZE ((size_t) 100000)
#define MD5_BUFFER_SIZE ((size_t) 16)

/**
 * @class ExpressionHasher
 * @brief A utility class for generating various types of hashes for expressions.
 *
 * The ExpressionHasher class provides static methods to compute MD5 hashes for different types of
 * expressions, including named types, terminal expressions, composite expressions, and general
 * expressions. It uses the mbedtls library for MD5 hashing.
 *
 * @note This class is designed to handle strings and lists of strings as input for hashing.
 *
 * @remark The class ensures that the generated hashes are consistent and unique for different
 *         expressions, making it useful for scenarios where expression uniqueness and integrity
 *         are critical.
 */
class ExpressionHasher {
   public:
    /**
     * @brief Computes the MD5 hash of the given input string.
     *
     * @param input The input string to be hashed.
     * @return A string representing the MD5 hash of the input.
     */
    static const string compute_hash(const string& input);

    /**
     * @brief Generates a hash for a named type.
     *
     * @param name The name of the type.
     * @return A string representing the hash of the named type.
     */
    static const string named_type_hash(const string& name);

    /**
     * @brief Generates a hash for a terminal expression.
     *
     * @param type The type of the terminal expression.
     * @param name The name of the terminal expression.
     * @return A string representing the hash of the terminal expression.
     * @throws invalid_argument if the terminal name is too large.
     */
    static const string terminal_hash(const string& type, const string& name);

    /**
     * @brief Generates a hash for a composite expression.
     *
     * @param elements A vector of strings representing the elements of the composite expression.
     * @return A string representing the hash of the composite expression.
     */
    static const string composite_hash(const StringList& elements);

    /**
     * @brief Generates a composite hash from a base hash.
     *
     * This function takes a base hash string and generates a composite hash
     * by applying additional hashing logic.
     *
     * @param hash_base A string representing the base hash.
     * @return A string representing the composite hash generated from the base hash.
     */
    static const string composite_hash(const string& hash_base);

    /**
     * @brief Generates a hash for an expression.
     *
     * @param type_hash The hash of the type of the expression.
     * @param elements A vector of strings representing the elements of the expression.
     * @return A string representing the hash of the expression.
     */
    static const string expression_hash(const string& type_hash, const StringList& elements);
};

}  // namespace atomdb
