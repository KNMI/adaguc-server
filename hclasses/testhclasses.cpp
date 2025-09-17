#include "CDirReader.h"
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

TEST(string, splitToStackReferences) {
  CT::string stringToSplit = "abc,def,,ghi";
  CT::StackList<CT::stringref> splittedRefs = stringToSplit.splitToStackReferences(",");
  CHECK(splittedRefs.size() == 4);
  CHECK_EQUAL(std::string(splittedRefs[0].c_str()), "abc");
  CHECK_EQUAL(std::string(splittedRefs[1].c_str()), "def");
  CHECK_EQUAL(std::string(splittedRefs[2].c_str()), "");
  CHECK_EQUAL(std::string(splittedRefs[3].c_str()), "ghi");
}

TEST(string, splitToStackReferencesMultiComma) {
  CT::string stringToSplit = "abc,def,,,ghi";
  CT::StackList<CT::stringref> splittedRefs = stringToSplit.splitToStackReferences(",");
  CHECK(splittedRefs.size() == 5);
  CHECK_EQUAL(std::string(splittedRefs[0].c_str()), "abc");
  CHECK_EQUAL(std::string(splittedRefs[1].c_str()), "def");
  CHECK_EQUAL(std::string(splittedRefs[2].c_str()), "");
  CHECK_EQUAL(std::string(splittedRefs[3].c_str()), "");
  CHECK_EQUAL(std::string(splittedRefs[4].c_str()), "ghi");
}

TEST(string, splitToStackReferencesLinesAndComma) {
  CT::string linesToSplit = "A1,B1,C1,D1,E1\nA2,B2,,D2,E2\n\nA,B,C,D,E\n,,,,\nG1\n,,,,TEST";
  CT::StackList<CT::stringref> lines = linesToSplit.splitToStackReferences("\n");
  LONGS_EQUAL(7, lines.size());

  CT::StackList<CT::stringref> splittedRefs0 = lines[0].splitToStackReferences(",");
  LONGS_EQUAL(5, splittedRefs0.size());
  CHECK_EQUAL("A1", std::string(splittedRefs0[0].c_str()));
  CHECK_EQUAL("B1", std::string(splittedRefs0[1].c_str()));
  CHECK_EQUAL("C1", std::string(splittedRefs0[2].c_str()));
  CHECK_EQUAL("D1", std::string(splittedRefs0[3].c_str()));
  CHECK_EQUAL("E1", std::string(splittedRefs0[4].c_str()));
  CT::StackList<CT::stringref> splittedRefs1 = lines[1].splitToStackReferences(",");
  LONGS_EQUAL(5, splittedRefs1.size());
  CHECK_EQUAL("A2", std::string(splittedRefs1[0].c_str()));
  CHECK_EQUAL("B2", std::string(splittedRefs1[1].c_str()));
  CHECK_EQUAL(std::string(""), std::string(splittedRefs1[2].c_str()));
  CHECK_EQUAL("D2", std::string(splittedRefs1[3].c_str()));
  CHECK_EQUAL("E2", std::string(splittedRefs1[4].c_str()));
  CT::StackList<CT::stringref> splittedRefs3 = lines[3].splitToStackReferences(",");
  LONGS_EQUAL(5, splittedRefs3.size());
  CT::StackList<CT::stringref> splittedRefs4 = lines[4].splitToStackReferences(",");
  LONGS_EQUAL(5, splittedRefs4.size());
  CT::StackList<CT::stringref> splittedRefs5 = lines[5].splitToStackReferences(",");
  LONGS_EQUAL(1, splittedRefs5.size());
  CT::StackList<CT::stringref> splittedRefs6 = lines[6].splitToStackReferences(",");
  LONGS_EQUAL(5, splittedRefs6.size());
  CHECK_EQUAL("TEST", std::string(splittedRefs6[4].c_str()));
}

TEST(string, splitToStackReferencesLinesAndCommaDoubleSplit) {
  CT::string linesToSplit = "A1,B1,C1,D1,E1\n\rA2,B2,,D2,E2\n\r\n\rA,B,C,D,E\n\r,,,,\n\rG1\n\r,,,,TEST";
  CT::StackList<CT::stringref> lines = linesToSplit.splitToStackReferences("\n\r");
  LONGS_EQUAL(7, lines.size());

  CT::StackList<CT::stringref> splittedRefs0 = lines[0].splitToStackReferences(",");
  LONGS_EQUAL(5, splittedRefs0.size());
  CHECK_EQUAL("A1", std::string(splittedRefs0[0].c_str()));
  CHECK_EQUAL("B1", std::string(splittedRefs0[1].c_str()));
  CHECK_EQUAL("C1", std::string(splittedRefs0[2].c_str()));
  CHECK_EQUAL("D1", std::string(splittedRefs0[3].c_str()));
  CHECK_EQUAL("E1", std::string(splittedRefs0[4].c_str()));
  CT::StackList<CT::stringref> splittedRefs1 = lines[1].splitToStackReferences(",");
  LONGS_EQUAL(5, splittedRefs1.size());
  CHECK_EQUAL("A2", std::string(splittedRefs1[0].c_str()));
  CHECK_EQUAL("B2", std::string(splittedRefs1[1].c_str()));
  CHECK_EQUAL(std::string(""), std::string(splittedRefs1[2].c_str()));
  CHECK_EQUAL("D2", std::string(splittedRefs1[3].c_str()));
  CHECK_EQUAL("E2", std::string(splittedRefs1[4].c_str()));
  CT::StackList<CT::stringref> splittedRefs3 = lines[3].splitToStackReferences(",");
  LONGS_EQUAL(5, splittedRefs3.size());
  CT::StackList<CT::stringref> splittedRefs4 = lines[4].splitToStackReferences(",");
  LONGS_EQUAL(5, splittedRefs4.size());
  CT::StackList<CT::stringref> splittedRefs5 = lines[5].splitToStackReferences(",");
  LONGS_EQUAL(1, splittedRefs5.size());
  CT::StackList<CT::stringref> splittedRefs6 = lines[6].splitToStackReferences(",");
  LONGS_EQUAL(5, splittedRefs6.size());
  CHECK_EQUAL("TEST", std::string(splittedRefs6[4].c_str()));
}
