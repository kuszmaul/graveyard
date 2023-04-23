#include "print_numbers.h"

#include "gtest/gtest.h"

TEST(PrintNumbers, Precision) {
  EXPECT_EQ(PrintWithPrecision(1996.8, 3), "1997");
  EXPECT_EQ(PrintWithPrecision(1996.8, 4), "1997");
  EXPECT_EQ(PrintWithPrecision(1996.18, 5), "1996.2");
  EXPECT_EQ(PrintWithPrecision(1996, 3), "1996");
}

#if 0
TEST(PrintNumbers, One) {
  EXPECT_EQ(NumberWithBinarySuffix(2093826048, 3), "1.95Gi");
  EXPECT_EQ(NumberWithBinarySuffix(1020022784, 3), "973Mi");

#if 0
  EXPECT_EQ(NumberWithBinarySuffix(1, 1), "1");
  EXPECT_EQ(NumberWithBinarySuffix(11, 1), "10");
  EXPECT_EQ(NumberWithBinarySuffix(14, 1), "10");
  EXPECT_EQ(NumberWithBinarySuffix(15, 1), "20");
  EXPECT_EQ(NumberWithBinarySuffix(15, 2), "15");
  EXPECT_EQ(NumberWithBinarySuffix(15, 2), "15");
  EXPECT_EQ(NumberWithBinarySuffix(101, 3), "101");
  EXPECT_EQ(NumberWithBinarySuffix(101, 2), "100");
  EXPECT_EQ(NumberWithBinarySuffix(105, 2), "110");
  EXPECT_EQ(NumberWithBinarySuffix(105, 3), "105");
  EXPECT_EQ(NumberWithBinarySuffix(999, 1), "1000");
  EXPECT_EQ(NumberWithBinarySuffix(1000, 1), "1Ki");
  EXPECT_EQ(NumberWithBinarySuffix(2 * 1024 -1 , 1), "2Ki");
  EXPECT_EQ(NumberWithBinarySuffix(2 * 1024, 1), "2Ki");
  // For precision 3 for numbers between 1000Gi and 1024Gi, give 4
  // digits.  This decision may be wrong...
  EXPECT_EQ(NumberWithBinarySuffix(1000 * 1024 * 1024 - 1, 3), "1000Mi");
  EXPECT_EQ(NumberWithBinarySuffix(1000 * 1024 * 1024, 3), "1000Mi");
  EXPECT_EQ(NumberWithBinarySuffix(1024 * 1024 * 1024 - 1, 3), "1024Mi");
#endif
}
#endif
