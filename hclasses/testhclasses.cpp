#include "CDirReader.h"
#include "CTString.h"
#include "CppUnitLite/TestHarness.h"

// To test this file do in the ./bin folder of adaguc-server:
// cmake --build . --config Debug --target testhclasses -j 10 -- && ctest --verbose

static inline SimpleString StringFrom(const std::string &value) { return SimpleString(value.c_str()); }

int main() {
  TestResult tr;
  TestRegistry::runAllTests(tr);
  if (tr.failureCount != 0) {
    return 1;
  }
  return 0;
}

TEST(string, equal) {
  CT::string a = "teststring_a";
  CHECK(a.equals("teststring_a"));
}

TEST(string, isNumeric) {
  CHECK(!CT::string("").isNumeric());
  CHECK(!CT::string(".").isNumeric());
  CHECK(!CT::string("HELLO").isNumeric());
  CHECK(CT::string("14").isNumeric());
  CHECK(CT::string("14.1").isNumeric());
}

TEST(string, isInt) {
  CHECK(!CT::string("").isInt());
  CHECK(!CT::string(".").isInt());
  CHECK(!CT::string("HELLO").isInt());
  CHECK(CT::string("14").isInt());
  CHECK(!CT::string("14.1").isInt());
  CHECK(CT::string("-15").isInt());
}

TEST(string, isFloat) {
  CHECK(!CT::string("").isFloat());
  CHECK(!CT::string(".").isFloat());
  CHECK(!CT::string("HELLO").isFloat());
  CHECK(!CT::string("15").isFloat());
  CHECK(CT::string("14.1").isFloat());
  CHECK(CT::string("15.0").isFloat());
  CHECK(CT::string("NaN").isFloat());
  CHECK(!CT::string("2019-07-28").isFloat());
  CHECK(CT::string("-15.0").isFloat());
  CHECK(CT::string("  -15.0  ").isFloat());
  CHECK(CT::string("  -15.0  ").toFloat() == -15.0);
}

TEST(string, initNULL) {
  CT::string a((const char *)NULL);
  CHECK(a.empty());
}

TEST(string, concatenation) {
  CT::string a = "teststring_a";
  CHECK(a.equals("teststring_a"));
  CT::string c = a + "b";
  CHECK(a.equals("teststring_a"));
  CHECK(c.equals("teststring_ab"));
  a += "bc";
  CHECK(a.equals("teststring_abc"));
  a += CT::string("d") + "e";
  CHECK(a.equals("teststring_abcde"));
  CT::string b = "f";
  CHECK(a.equals("teststring_abcde"));
  CT::string d = a + b;
  CHECK(a.equals("teststring_abcde"));

  CT::string teststring = "teststring";
  teststring.concat("abc");
  CHECK(teststring.equals("teststringabc"));
}

TEST(string, concatenationlength) {
  CT::string teststring = "teststring";
  teststring.concatlength("abc", 0);
  CHECK(teststring.equals("teststring"));
  teststring.concatlength("abc", 1);
  CHECK(teststring.equals("teststringa"));
  teststring.concatlength("bc", 2);
  CHECK(teststring.equals("teststringabc"));
}

// TEST(CDirReader, test_makeCleanPath) {
//   CDirReader::test_makeCleanPath();
//   CDirReader::test_compareLists();
// }

TEST(string, split) {
  CT::string stringToSplit = "abc,def,,ghi";
  std::vector<CT::string> splittedRefs = stringToSplit.split(",");
  CHECK(splittedRefs.size() == 4);
  CHECK_EQUAL(std::string(splittedRefs[0].c_str()), "abc");
  CHECK_EQUAL(std::string(splittedRefs[1].c_str()), "def");
  CHECK_EQUAL(std::string(splittedRefs[2].c_str()), "");
  CHECK_EQUAL(std::string(splittedRefs[3].c_str()), "ghi");
}

TEST(string, splitMultiComma) {
  CT::string stringToSplit = "abc,def,,,ghi";
  std::vector<CT::string> splittedRefs = stringToSplit.split(",");
  CHECK(splittedRefs.size() == 5);
  CHECK_EQUAL(std::string(splittedRefs[0].c_str()), "abc");
  CHECK_EQUAL(std::string(splittedRefs[1].c_str()), "def");
  CHECK_EQUAL(std::string(splittedRefs[2].c_str()), "");
  CHECK_EQUAL(std::string(splittedRefs[3].c_str()), "");
  CHECK_EQUAL(std::string(splittedRefs[4].c_str()), "ghi");
}

TEST(string, splitLinesAndComma) {
  CT::string linesToSplit = "A1,B1,C1,D1,E1\nA2,B2,,D2,E2\n\nA,B,C,D,E\n,,,,\nG1\n,,,,TEST";
  std::vector<CT::string> lines = linesToSplit.split("\n");
  LONGS_EQUAL(7, lines.size());

  std::vector<CT::string> splittedRefs0 = lines[0].split(",");

  LONGS_EQUAL(5, splittedRefs0.size());
  CHECK_EQUAL("A1", std::string(splittedRefs0[0].c_str()));
  CHECK_EQUAL("B1", std::string(splittedRefs0[1].c_str()));
  CHECK_EQUAL("C1", std::string(splittedRefs0[2].c_str()));
  CHECK_EQUAL("D1", std::string(splittedRefs0[3].c_str()));
  CHECK_EQUAL("E1", std::string(splittedRefs0[4].c_str()));
  std::vector<CT::string> splittedRefs1 = lines[1].split(",");
  LONGS_EQUAL(5, splittedRefs1.size());
  CHECK_EQUAL("A2", std::string(splittedRefs1[0].c_str()));
  CHECK_EQUAL("B2", std::string(splittedRefs1[1].c_str()));
  CHECK_EQUAL(std::string(""), std::string(splittedRefs1[2].c_str()));
  CHECK_EQUAL("D2", std::string(splittedRefs1[3].c_str()));
  CHECK_EQUAL("E2", std::string(splittedRefs1[4].c_str()));
  std::vector<CT::string> splittedRefs3 = lines[3].split(",");
  LONGS_EQUAL(5, splittedRefs3.size());
  std::vector<CT::string> splittedRefs4 = lines[4].split(",");
  LONGS_EQUAL(4, splittedRefs4.size());
  std::vector<CT::string> splittedRefs5 = lines[5].split(",");
  LONGS_EQUAL(1, splittedRefs5.size());
  std::vector<CT::string> splittedRefs6 = lines[6].split(",");
  LONGS_EQUAL(5, splittedRefs6.size());
  CHECK_EQUAL("TEST", std::string(splittedRefs6[4].c_str()));
}

TEST(string, splitLinesAndCommaDoubleSplit) {
  CT::string linesToSplit = "A1,B1,C1,D1,E1\n\rA2,B2,,D2,E2\n\r\n\rA,B,C,D,E\n\r,,,,\n\rG1\n\r,,,,TEST";
  std::vector<CT::string> lines = linesToSplit.split("\n\r");
  LONGS_EQUAL(7, lines.size());

  std::vector<CT::string> splittedRefs0 = lines[0].split(",");
  LONGS_EQUAL(5, splittedRefs0.size());
  CHECK_EQUAL("A1", std::string(splittedRefs0[0].c_str()));
  CHECK_EQUAL("B1", std::string(splittedRefs0[1].c_str()));
  CHECK_EQUAL("C1", std::string(splittedRefs0[2].c_str()));
  CHECK_EQUAL("D1", std::string(splittedRefs0[3].c_str()));
  CHECK_EQUAL("E1", std::string(splittedRefs0[4].c_str()));
  std::vector<CT::string> splittedRefs1 = lines[1].split(",");
  LONGS_EQUAL(5, splittedRefs1.size());
  CHECK_EQUAL("A2", std::string(splittedRefs1[0].c_str()));
  CHECK_EQUAL("B2", std::string(splittedRefs1[1].c_str()));
  CHECK_EQUAL(std::string(""), std::string(splittedRefs1[2].c_str()));
  CHECK_EQUAL("D2", std::string(splittedRefs1[3].c_str()));
  CHECK_EQUAL("E2", std::string(splittedRefs1[4].c_str()));
  std::vector<CT::string> splittedRefs3 = lines[3].split(",");
  LONGS_EQUAL(5, splittedRefs3.size());
  std::vector<CT::string> splittedRefs4 = lines[4].split(",");
  LONGS_EQUAL(4, splittedRefs4.size());
  std::vector<CT::string> splittedRefs5 = lines[5].split(",");
  LONGS_EQUAL(1, splittedRefs5.size());
  std::vector<CT::string> splittedRefs6 = lines[6].split(",");
  LONGS_EQUAL(5, splittedRefs6.size());
  CHECK_EQUAL("TEST", std::string(splittedRefs6[4].c_str()));
}

TEST(string, splitOnString) {
  CT::string linesToSplit = "Line1<SEP>Line2<SEP>Line3<SEP>Line4<SEP>Line5<SEP>Line6<SEP>Line7";
  std::vector<CT::string> lines = linesToSplit.split("<SEP>");
  LONGS_EQUAL(7, lines.size());
  CHECK_EQUAL("Line1", std::string(lines[0].c_str()));
  CHECK_EQUAL("Line7", std::string(lines[6].c_str()));
}

TEST(string, splitOnKeyWhichRepeats) {
  CT::string linesToSplit = "aaaaaaaaaaaaaaaaaaaaaaaaa";
  // For now this behavior is to be expected
  std::vector<CT::string> lines = linesToSplit.split("aaa");
  LONGS_EQUAL(9, lines.size());
}

TEST(string, substring) {
  CT::string stringToSubstitute = "We think in generalities, but we live in details.";
  CT::string sub1 = stringToSubstitute.substring(3, 8);
  CHECK_EQUAL("think", std::string(sub1.c_str()));
  CT::string sub2 = stringToSubstitute.substring(0, 2);
  CHECK_EQUAL("We", std::string(sub2.c_str()));
  CT::string sub3 = stringToSubstitute.substring(12, 13);
  CHECK_EQUAL("g", std::string(sub3.c_str()));
  CT::string sub4 = stringToSubstitute.substring(41, -1);
  CHECK_EQUAL("details.", std::string(sub4.c_str()));
  CT::string sub5 = stringToSubstitute.substring(41, 101);
  CHECK_EQUAL("details.", std::string(sub5.c_str()));
  CT::string sub6 = stringToSubstitute.substring(12, 12);
  CHECK_EQUAL("", std::string(sub6.c_str()));
  CT::string sub7 = stringToSubstitute.substring(12, 11);
  CHECK_EQUAL("", std::string(sub7.c_str()));
  CT::string sub8 = stringToSubstitute.substring(12, -11);
  CHECK_EQUAL("generalities, but we live in details.", std::string(sub8.c_str()));
  CT::string sub9 = stringToSubstitute.substring(-1, 5);
  CHECK_EQUAL("", std::string(sub9.c_str()));
}

TEST(string, basename) {
  CHECK_EQUAL(CT::basename("/hoallo/test.nc"), "test.nc");
  CHECK_EQUAL(CT::basename("\\hoall\\test.nc"), "test.nc");
  CHECK_EQUAL(CT::basename("test.nc"), "test.nc");
  CHECK_EQUAL(CT::basename(""), "");
  CHECK_EQUAL(CT::basename("/a/b/c/d/e/f"), "f");
  CHECK_EQUAL(CT::basename("/a/b\\/c/d\\/e/f"), "f");
}

TEST(string, ctprintf) { CHECK_EQUAL("hi! 2 3.140000", CT::printf("%s %d %f", "hi!", 2, 3.14)); }

TEST(string, ctprintfconcat) {
  std::string test = "abc ";
  CT::printfconcat(test, "%s %d %f", "hi!", 2, 3.14);
  CHECK_EQUAL("abc hi! 2 3.140000", test);
  CT::printfconcat(test, " MORESTUFF");
  CHECK_EQUAL("abc hi! 2 3.140000 MORESTUFF", test);
}

TEST(string, replace) {
  std::string test = "abcdefgabcdefg";
  CHECK_EQUAL("ab!!!efgab!!!efg", CT::replace(test, "cd", "!!!"));
  CHECK_EQUAL("ab!efgab!efg", CT::replace(test, "cd", "!"));
  CHECK_EQUAL("abefgabefg", CT::replace(test, "cd", ""));
}

TEST(string, toLowerCase) {
  std::string test = "abcdefgabcdefg";
  CHECK_EQUAL("abcd", CT::toLowerCase("AbCd"));
}

TEST(string, eraseTableNames) {
  std::vector<std::string> tableNamesDone;
  tableNamesDone.push_back("test1");
  tableNamesDone.push_back("test2");
  tableNamesDone.push_back("test1");
  tableNamesDone.push_back("test3");
  std::erase(tableNamesDone, "test1");
  LONGS_EQUAL(2, tableNamesDone.size());
}
