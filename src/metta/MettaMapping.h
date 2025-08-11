#pragma once

#include <string>

using namespace std;
namespace commons {

/**
 * Abstract interface supposed to be implemented by classes that are capable of
 * making an Atom out of a Handle.
 */
class MettaMapping {
   private:
    MettaMapping();

   public:
    ~MettaMapping() {}
    static string EXPRESSION_LINK_TYPE;
    static string SYMBOL_NODE_TYPE;
    static string AND_QUERY_OPERATOR;
    static string OR_QUERY_OPERATOR;
};

}  // namespace commons
