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
    static string ANDNOT_QUERY_OPERATOR;
    static string OR_QUERY_OPERATOR;
    static string CHAIN_QUERY_OPERATOR;

    static string metta_expr3(const string& expr1, const string& expr2, const string& expr3) {
        return "(" + expr1 + " " + expr2 + " " + expr3 + ")";
    }

    static string metta_or(const string& expr1, const string& expr2) {
        return metta_expr3("or", expr1, expr2);
    }

    static string metta_and(const string& expr1, const string& expr2) {
        return metta_expr3("and", expr1, expr2);
    }

    static string metta_chain(const string& source, const string& target, const string& query) {
        return "(chain 0 1 2 " + source + " " + target + " " + query + ")";
    }

    static string metta_var(const string& name) { return "$" + name; }
};

}  // namespace commons
