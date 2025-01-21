
/**
 * @file type_aliases.h
 * @brief This header file contains type aliases to improve code readability and maintainability
 *        within the atomdb namespace.
 *
 * The type aliases defined in this file are intended to simplify the usage of commonly used
 * STL containers and types, making the code more concise and easier to understand. The aliases
 * cover a range of types including optional values, sets, maps, and lists.
 *
 * @note 1. The alias MapOfAny for std::unordered_map<std::string, std::any> is commented out due
 *          to poor performance. It is kept in the file as a reminder of the performance implications.
 *       2. ListOfAny is an alias for std::vector<std::any>, representing a list of any type.
 *          Note that while std::vector<std::any> can be useful in some cases, its performance
 *          should be tested for each specific use case.
 */
#pragma once

#include <any>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;

namespace atomdb {

// Type aliases for readability
using uchar = unsigned char;
using StringList = vector<string>;
using StringUnorderedSet = unordered_set<string>;

/**
 * std::vector<std::any> performs well enough in some particular cases, but be cautious when using it.
 * Always test how it performs in your specific use case.
 */
using ListOfAny = vector<any>;

/**
 * NOTE:
 * The following type alias was commented out because std::unordered_map<T, std::any> performs
 * poorly, and it was kept here just as a reminder of the performance implications.
 *
 * using MapOfAny = unordered_map<string, any>;
 */

}  // namespace atomdb
