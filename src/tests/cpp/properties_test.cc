#include "Properties.h"

#include <gtest/gtest.h>

#include "atomdb/AtomDBAPITypes.h"

using namespace std;
using namespace atomdb::atomdb_api_types;
using namespace commons;

TEST(PropertiesTest, InsertAndGetString) {
    Properties map;
    map["foo"] = string("bar");
    const string* value = map.get_ptr<string>("foo");
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, "bar");
}

TEST(PropertiesTest, InsertAndGetLong) {
    Properties map;
    map["num"] = 42L;
    const long* value = map.get_ptr<long>("num");
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, 42L);
}

TEST(PropertiesTest, InsertAndGetDouble) {
    Properties map;
    map["pi"] = 3.14;
    const double* value = map.get_ptr<double>("pi");
    ASSERT_NE(value, nullptr);
    EXPECT_DOUBLE_EQ(*value, 3.14);
}

TEST(PropertiesTest, InsertAndGetBool) {
    Properties map;
    map["flag"] = true;
    const bool* value = map.get_ptr<bool>("flag");
    ASSERT_NE(value, nullptr);
    EXPECT_TRUE(*value);
}

TEST(PropertiesTest, GetWrongTypeReturnsNullptr) {
    Properties map;
    map["foo"] = string("bar");
    const long* value = map.get_ptr<long>("foo");
    EXPECT_EQ(value, nullptr);
}

TEST(PropertiesTest, GetNonexistentKeyReturnsNullptr) {
    Properties map;
    const string* value = map.get_ptr<string>("missing");
    EXPECT_EQ(value, nullptr);
}

TEST(PropertiesTest, ToStringRepresentation) {
    Properties map;
    map["foo"] = string("bar");
    map["num"] = 42L;
    map["uint"] = 1;
    map["pi"] = 3.14;
    map["flag"] = false;
    string str = map.to_string();
    // Order is guaranteed: flag, foo, num, pi
    EXPECT_EQ(str, "{flag: false, foo: 'bar', num: 42, pi: 3.140000, uint: 1}");
}

TEST(PropertiesTest, Tokenization) {
    Properties map;
    map["foo"] = string("bar");
    map["num"] = 42L;
    map["uint"] = 1;
    map["pi"] = 3.14;
    map["flag"] = false;
    vector<string> v = map.tokenize();
    // Order is guaranteed: flag, foo, num, pi
    EXPECT_EQ(v.size(), 12);
    unsigned int cursor = 0;
    EXPECT_EQ(v[cursor++], "flag");
    EXPECT_EQ(v[cursor++], "bool");
    EXPECT_EQ(v[cursor++], "false");
    EXPECT_EQ(v[cursor++], "foo");
    EXPECT_EQ(v[cursor++], "string");
    EXPECT_EQ(v[cursor++], "bar");
    EXPECT_EQ(v[cursor++], "num");
    EXPECT_EQ(v[cursor++], "long");
    EXPECT_EQ(v[cursor++], "42");
    EXPECT_EQ(v[cursor++], "pi");
    EXPECT_EQ(v[cursor++], "double");
    EXPECT_EQ(v[cursor++], "3.140000");
    EXPECT_EQ(v[cursor++], "uint");
    EXPECT_EQ(v[cursor++], "unsigned_int");
    EXPECT_EQ(v[cursor++], "1");
    Properties map2;
    map2.untokenize(v);
    EXPECT_EQ(map2.get<string>("foo"), string("bar"));
    EXPECT_EQ(map2.get<long>("num"), 42L);
    EXPECT_EQ(map2.get<double>("pi"), 3.14);
    EXPECT_EQ(map2.get<bool>("flag"), false);
    EXPECT_EQ(map2.get<unsigned int>("uint"), 1);
}
