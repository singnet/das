#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <utility>
#include <vector>

#include "Link.h"
#include "Node.h"
#include "RedisMongoDBAPITypes.h"
#include "gtest/gtest.h"

using namespace atomdb::atomdb_api_types;
using namespace atoms;
using namespace std;
using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;

TEST(RedisMongoDBCustomAttributes, Stv) {
    Node similarity("Symbol", "Similarity");
    Node human("Symbol", "\"human\"");
    Properties attributes;
    attributes["stv"] = make_pair(0.25, 0.5);
    attributes["name"] = string("human");
    attributes["count"] = 7L;
    attributes["weight"] = 0.75;
    attributes["active"] = true;
    Link link("Expression", {similarity.handle(), human.handle()}, true, attributes);

    // The truth value round-trips together with the other supported types.
    MongodbDocument document(&link, "", {}, false);
    EXPECT_EQ(document.extract_custom_attributes(document.get_object("custom_attributes")), attributes);

    attributes["unsigned"] = 1U;
    Link unsupported("Expression", {similarity.handle(), human.handle()}, true, attributes);
    EXPECT_THROW(MongodbDocument(&unsupported, "", {}, false), runtime_error);

    for (const vector<double>& values : {vector<double>{0.25}, vector<double>{0.25, 0.5, 0.75}}) {
        bsoncxx::builder::basic::array stv;
        for (double value : values) stv.append(value);
        auto stored = make_document(kvp("custom_attributes", make_document(kvp("stv", stv.extract()))));
        bsoncxx::v_noabi::stdx::optional<bsoncxx::v_noabi::document::value> optional(std::move(stored));
        MongodbDocument invalid(optional);
        EXPECT_THROW(invalid.extract_custom_attributes(invalid.get_object("custom_attributes")),
                     runtime_error);
    }
}
