#include "expression_hasher.h"

#include <gtest/gtest.h>

TEST(ExpressionHasherTest, composite_hash_empty) {
    char* hash = composite_hash(nullptr, 0);
    ASSERT_NE(hash, nullptr);
    EXPECT_STREQ("d41d8cd98f00b204e9800998ecf8427e", hash);
    delete[] hash;
}
