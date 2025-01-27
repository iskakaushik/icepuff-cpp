#include <gtest/gtest.h>
#include <icypuff/icypuff.h>

TEST(HelloWorldTest, NotEmpty) {
    std::string result = icypuff::HelloWorld();
    EXPECT_FALSE(result.empty());
} 