#include <gtest/gtest.h>

#include "atomdb/AtomDBAPITypes.h"

using namespace std;
using namespace atomdb::atomdb_api_types;

TEST(CustomAttributesMapTest, InsertAndGetString) {
    CustomAttributesMap map;
    map["foo"] = string("bar");
    const string* value = map.get<string>("foo");
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, "bar");
}

TEST(CustomAttributesMapTest, InsertAndGetLong) {
    CustomAttributesMap map;
    map["num"] = 42L;
    const long* value = map.get<long>("num");
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, 42L);
}

TEST(CustomAttributesMapTest, InsertAndGetDouble) {
    CustomAttributesMap map;
    map["pi"] = 3.14;
    const double* value = map.get<double>("pi");
    ASSERT_NE(value, nullptr);
    EXPECT_DOUBLE_EQ(*value, 3.14);
}

TEST(CustomAttributesMapTest, InsertAndGetBool) {
    CustomAttributesMap map;
    map["flag"] = true;
    const bool* value = map.get<bool>("flag");
    ASSERT_NE(value, nullptr);
    EXPECT_TRUE(*value);
}

TEST(CustomAttributesMapTest, GetWrongTypeReturnsNullptr) {
    CustomAttributesMap map;
    map["foo"] = string("bar");
    const long* value = map.get<long>("foo");
    EXPECT_EQ(value, nullptr);
}

TEST(CustomAttributesMapTest, GetNonexistentKeyReturnsNullptr) {
    CustomAttributesMap map;
    const string* value = map.get<string>("missing");
    EXPECT_EQ(value, nullptr);
}

TEST(CustomAttributesMapTest, ToStringRepresentation) {
    CustomAttributesMap map;
    map["foo"] = string("bar");
    map["num"] = 42L;
    map["pi"] = 3.14;
    map["flag"] = false;
    string str = map.to_string();
    // Order is guaranteed: flag, foo, num, pi
    EXPECT_EQ(str, "{flag: false, foo: 'bar', num: 42, pi: 3.140000}");
}
