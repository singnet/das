#pragma once

#include <stack>

#include "Assignment.h"
#include "Link.h"
#include "Node.h"
#include "Wildcard.h"

namespace atoms {

class LinkSchema : public Wildcard {
   private:
    class SchemaElement {
       public:
        string handle;
        string name;
        string type;
        vector<SchemaElement> targets;
        bool is_link;
        bool is_wildcard;
        SchemaElement() : is_link(false), is_wildcard(false) {}
        bool match(const string& handle, Assignment& assignment, HandleDecoder& decoder, Atom* atom_ptr);
    };
    bool _frozen;
    unsigned int _arity;
    vector<string> _schema;
    vector<string> _composite_type;
    string _composite_type_hash;
    string _atom_handle;
    string _metta_representation;
    vector<string> _tokens;
    vector<vector<string>> _build_tokens;
    vector<tuple<string, string, string>> _atom_stack;
    stack<SchemaElement> _schema_element_stack;
    SchemaElement _schema_element;

    void _init(unsigned int arity);
    bool _check_frozen() const;
    bool _check_not_frozen() const;
    void _stack_link_schema(const string& type, unsigned int link_arity, bool is_link);
    void _syntax_assert(bool flag, string token, unsigned int cursor);
    unsigned int _push_stack_top(
        stack<tuple<string, string, unsigned int, unsigned int>>& link_schema_stack,
        string cursor_token,
        unsigned int cursor);

   public:
    static string LINK_TEMPLATE;
    static string ATOM;
    static string NODE;
    static string LINK;
    static string UNTYPED_VARIABLE;

    // ---------------------------------------------------------------------------------------------
    // Constructors, destructors , basic operators and initializers

    /**
     * @brief Construct a LinkSchema.
     * @param type The type of the link.
     * @param LinkSchema arity
     * @param custom_attributes Custom attributes for the schema.
     */
    LinkSchema(const string& type, unsigned int arity, const Properties& custom_attributes = {});

    /**
     * @brief Copy constructor.
     * Deeply copies link type, targets and all custom attributes.
     *
     * @param other LinkSchema to be copied.
     */
    LinkSchema(const LinkSchema& other);

    /**
     * @brief Construct a LinkSchema using a vector of tokens.
     * @param tokens Vector of tokens.
     */
    LinkSchema(const vector<string>& tokens, const Properties& custom_attributes = {});

    /**
     * @brief Assignment operator.
     * Deeply copies link type, targets and all custom attributes.
     *
     * @param other LinkSchema to be copied.
     */
    virtual LinkSchema& operator=(const LinkSchema& other);

    /**
     * @brief Comparisson operator.
     *
     * @param other LinkSchema to be compared to.
     */
    virtual bool operator==(const LinkSchema& other);

    /**
     * @brief Comparisson operator.
     *
     * @param other LinkSchema to be compared to.
     */
    virtual bool operator!=(const LinkSchema& other);

    /**
     * @brief Validate the LinkSchema object.
     *
     * @throws std::runtime_error if the number of targets is less than MINIMUM_TARGETS_SIZE.
     */
    void validate() const;

    // ---------------------------------------------------------------------------------------------
    // Public Atom API

    virtual string to_string() const;

    /**
     * @brief Returns this Atom's handle.
     *
     * @return Handle for this atom.
     */
    virtual string handle() const;

    /**
     * @brief Returns this Atom's composite type hash.
     *
     * @return composite type hash for this atom.
     */
    virtual string composite_type_hash(HandleDecoder& decoder) const;

    /**
     * @brief Returns this Atom's composite type hash.
     *
     * @return composite type for this atom.
     */
    virtual vector<string> composite_type(HandleDecoder& decoder) const;

    /**
     * @brief Constructs and returns a MeTTa expression which represents this Atom.
     *
     * @return a MeTTa expression which represents this Atom.
     */
    virtual string metta_representation(HandleDecoder& decoder) const;

    /**
     * @brief Return this LinkSchema's arity
     * @return this LinkSchema's arity.
     */
    unsigned int arity() const;

    /**
     * @brief Returns this LinkSchema's targets.
     *
     * @return targets for this LinkSchema.
     */
    const vector<string>& targets() const;

    bool match(Link& link, Assignment& assignment, HandleDecoder& decoder);
    bool match(const string& handle, Assignment& assignment, HandleDecoder& decoder);

    // ---------------------------------------------------------------------------------------------
    // Public API to build LinkSchema objects

    void add_target(const string& schema_handle,
                    const string& composite_type_hash,
                    const string& metta_representation);

    void stack_atom(const string& handle);

    void stack_node(const string& type, const string& name);

    void stack_link(const string& type, unsigned int link_arity);

    void stack_untyped_variable(const string& name);

    void stack_link_schema(const string& type, unsigned int link_arity);

    void build();

    const vector<string>& tokens();

    vector<string> tokenize();

    void tokenize(vector<string> output);

    void untokenize(const vector<string>& tokens);
};
}  // namespace atoms
