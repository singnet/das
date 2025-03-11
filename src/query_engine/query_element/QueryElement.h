#ifndef _QUERY_ELEMENT_QUERYELEMENT_H
#define _QUERY_ELEMENT_QUERYELEMENT_H

#include <string>
#include <memory>
#include "Utils.h"
#include "QueryNode.h"

#define DEBUG

using namespace std;
using namespace query_node;
using namespace commons;

namespace query_element {

/**
 * Basic element in the class hierarchy which represents boolean logical expression involving
 * nodes, links and patterns.
 *
 * Boolean logical expressions are formed by logical operators (AND, OR, NOT) and
 * operands (Node, Link and LinkTemplate). Nested expression are allowed. AND and OR may operate
 * on any number (> 1) of arguments while NOT takes a single argument.
 *
 * Nodes are defined by type+name. Links are defined by type+targets. LinkTemplates are defined
 * like Links, where the Link type and any number of targets may be wildcards (actually, wildcards
 * are named variables which are unified as the query is executed). LinkTemplates can also be
 * nested, i.e., one of the targets of a LinkTemplate can be another LinkTemplate.
 *
 * There's no limit in the number of nesting levels of LinkTemplates or boolean expressions.
 *
 * A query can be understood as a tree whose nodes are QueryElements. Internal nodes are
 * logical operators and leaves are either Links or LinkTemplates (nested or not).
 *
 * The query engine we implement here uses the Nodes/Links values that satisfy the leaves in this
 * tree and flows them up through the internal nodes (logical operators) until they reach the root
 * of the tree. In this path, some links are dropped because they don't satisfy the properties
 * required by the operators or they don't satisfy a proper unification in the set of variables.
 *
 * Links that reach the root of the tree are considered actual query answers.
 *
 * Each QueryElement is an element in a distributed algorithm, with one or more threads processing
 * its inputs and generating outputs according to the logic of each element. A communication
 * framework is used to flow the links up through the tree using our DistributedAlgorithmNode
 * which is essentially a framework to implement the basic functionalities required by a
 * distributed algorithm. Since this framework allows communication either intra-process and
 * extra-process (in the same machine or in different ones), we can have QueryElements of the
 * same tree (i.e. of the same query) being processed in different machines or all of them in the
 * same machine (either in the same process or in different processes).
 */
class QueryElement {

public:

    string id;
    string subsequent_id;

    /**
     * Basic constructor which solely initialize variables.
     */
    QueryElement();

    /**
     * Destructor.
     */
    virtual ~QueryElement();

    // --------------------------------------------------------------------------------------------
    // API to be extended by concrete subclasses

    /**
     * Setup QueryNodes used by concrete implementations of QueryElements. This method is called
     * after all ids and other topological-related setup in the query tree is finished.
     */
    virtual void setup_buffers() = 0;

    /**
     * Synchronously request this QueryElement to shutdown any threads it may have spawned.
     */
    virtual void graceful_shutdown() = 0;

    /**
     * Indicates whether this QueryElement is a Terminal (i.e. Node, Link or Variable).
     */
    bool is_terminal;

protected:

    /**
     * Return true iff this QueryElement have finished its work in the flow of links up through
     * the query tree.
     *
     * When this method return true, it means that all the QueryElements below than in the chain
     * have already provided all the links they are supposed to and this QueryElement have already
     * processed all of them and delivered all the links that are supposed to pass through the flow
     * to the upper element in the tree. In other words, this QueryElement have no further work
     * to do.
     *
     * @return true iff this QueryElement have finished its work in the flow of links up througth
     * the query tree.
     */
    bool is_flow_finished();

    /**
     * Sets a flag to indicate that this QueryElement have finished its work in the query. See
     * comments in method is_flow_finished().
     */
    void set_flow_finished();

private:

    bool flow_finished;
    mutex flow_finished_mutex;
};

} // namespace query_element

#endif // _QUERY_ELEMENT_QUERYELEMENT_H
