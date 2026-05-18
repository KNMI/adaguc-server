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

/* ------------------------------------------------------------------ *
 *  Tests for the new free helpers in namespace CT::, operating on
 *  std::string. These cover the migration from CT::string to
 *  std::string in the project's hot path; existing CT::string tests
 *  above remain to verify backward compatibility.
 * ------------------------------------------------------------------ */

TEST(stringHelpers, fromCStr) {
  CHECK(CT::fromCStr(nullptr).empty());
  CHECK(CT::fromCStr("") == "");
  CHECK(CT::fromCStr("hello") == "hello");
}

TEST(stringHelpers, ctprintf) { CHECK_EQUAL("hi! 2 3.140000", CT::printf("%s %d %f", "hi!", 2, 3.14)); }

TEST(stringHelpers, ctprintfconcat) {
  std::string test = "abc ";
  CT::printfconcat(test, "%s %d %f", "hi!", 2, 3.14);
  CHECK_EQUAL("abc hi! 2 3.140000", test);
  CT::printfconcat(test, " MORESTUFF");
  CHECK_EQUAL("abc hi! 2 3.140000 MORESTUFF", test);
}

TEST(stringHelpers, replace) {
  std::string test = "abcdefgabcdefg";
  CHECK_EQUAL("ab!!!efgab!!!efg", CT::replace(test, "cd", "!!!"));
  CHECK_EQUAL("ab!efgab!efg", CT::replace(test, "cd", "!"));
  CHECK_EQUAL("abefgabefg", CT::replace(test, "cd", ""));
}

TEST(stringHelpers, replaceSelf) {
  std::string testA = "abcdefgabcdefg";
  CT::replaceSelf(testA, "cd", "!!!");
  CHECK_EQUAL("ab!!!efgab!!!efg", testA);
  std::string testB = "abcdefgabcdefg";
  CT::replaceSelf(testB, "cd", "!");
  CHECK_EQUAL("ab!efgab!efg", testB);
  std::string testC = "abcdefgabcdefg";
  CT::replaceSelf(testC, "cd", "");
  CHECK_EQUAL("abefgabefg", testC);
}

TEST(stringHelpers, toLowerCase) { CHECK_EQUAL("abcd", CT::toLowerCase("AbCd")); }
TEST(stringHelpers, toUpperCase) { CHECK_EQUAL("ABCD", CT::toUpperCase("AbCd")); }

TEST(stringHelpers, trim) {
  CHECK_EQUAL("hello", CT::trim("   hello   "));
  CHECK_EQUAL("hello world", CT::trim("\t hello world\n\r"));
  CHECK_EQUAL("", CT::trim("    "));
  CHECK_EQUAL("", CT::trim(""));
}

TEST(stringHelpers, indexOf) {
  std::string valueToCheck = "Hello planet earth, you are a great planet.";
  LONGS_EQUAL(-1, CT::indexOf(valueToCheck, "mars"));
  LONGS_EQUAL(6, CT::indexOf(valueToCheck, "planet"));
  LONGS_EQUAL(0, CT::indexOf(valueToCheck, "Hello"));
  LONGS_EQUAL(0, CT::indexOf(valueToCheck, ""));
  LONGS_EQUAL(0, CT::indexOf(valueToCheck, "Hello planet earth, you are a great planet."));
  LONGS_EQUAL(-1, CT::indexOf(valueToCheck, "Hello planet earth, you are a great planet. ---------------------------"));
  LONGS_EQUAL(-1, CT::indexOf(valueToCheck, "planet earth, you are a great planet. ---------------------------"));
}

TEST(stringHelpers, lastIndexOf) {
  std::string valueToCheck = "Hello planet earth, you are a great planet.";
  LONGS_EQUAL(-1, CT::lastIndexOf(valueToCheck, "mars"));
  LONGS_EQUAL(36, CT::lastIndexOf(valueToCheck, "planet"));
  LONGS_EQUAL(0, CT::lastIndexOf(valueToCheck, "Hello"));
  LONGS_EQUAL(0, CT::lastIndexOf(valueToCheck, ""));
  LONGS_EQUAL(0, CT::lastIndexOf(valueToCheck, "Hello planet earth, you are a great planet."));
  LONGS_EQUAL(-1, CT::lastIndexOf(valueToCheck, "Hello planet earth, you are a great planet. ---------------------------"));
  LONGS_EQUAL(-1, CT::lastIndexOf(valueToCheck, "planet earth, you are a great planet. ---------------------------"));
}

TEST(stringHelpers, endsWith) {
  std::string valueToCheck = "Hello planet earth, you are a great planet.";
  LONGS_EQUAL(false, CT::endsWith(valueToCheck, "mars"));
  LONGS_EQUAL(true, CT::endsWith(valueToCheck, "planet."));
  LONGS_EQUAL(false, CT::endsWith(valueToCheck, "Hello"));
  LONGS_EQUAL(true, CT::endsWith(valueToCheck, ""));
}

TEST(stringHelpers, startsWith) {
  std::string valueToCheck = "Hello planet earth, you are a great planet.";
  LONGS_EQUAL(false, CT::startsWith(valueToCheck, "mars"));
  LONGS_EQUAL(false, CT::startsWith(valueToCheck, "planet."));
  LONGS_EQUAL(true, CT::startsWith(valueToCheck, "Hello"));
  LONGS_EQUAL(true, CT::startsWith(valueToCheck, ""));
}

TEST(stringHelpers, basename) {
  CHECK_EQUAL("test.nc", CT::basename("/hoallo/test.nc"));
  CHECK_EQUAL("test.nc", CT::basename("\\hoall\\test.nc"));
  CHECK_EQUAL("test.nc", CT::basename("test.nc"));
  CHECK_EQUAL("", CT::basename(""));
  CHECK_EQUAL("f", CT::basename("/a/b/c/d/e/f"));
}

TEST(stringHelpers, equalsIgnoreCase) {
  CHECK(CT::equalsIgnoreCase("Hello", "hello"));
  CHECK(CT::equalsIgnoreCase("ABC", "abc"));
  CHECK(!CT::equalsIgnoreCase("hello", "world"));
  CHECK(!CT::equalsIgnoreCase("hello", "hello!"));
}

TEST(stringHelpers, split) {
  auto parts = CT::split("abc,def,,ghi", ",");
  LONGS_EQUAL(4, parts.size());
  CHECK_EQUAL("abc", parts[0]);
  CHECK_EQUAL("def", parts[1]);
  CHECK_EQUAL("", parts[2]);
  CHECK_EQUAL("ghi", parts[3]);
}

TEST(stringHelpers, isNumeric) {
  CHECK(!CT::isNumeric(""));
  CHECK(!CT::isNumeric("HELLO"));
  CHECK(CT::isNumeric("14"));
  CHECK(CT::isNumeric("14.1"));
  CHECK(CT::isNumeric("NaN"));
}

TEST(stringHelpers, isInt) {
  CHECK(!CT::isInt(""));
  CHECK(CT::isInt("14"));
  CHECK(CT::isInt("-15"));
  CHECK(!CT::isInt("14.1"));
}

TEST(stringHelpers, isFloat) {
  CHECK(!CT::isFloat(""));
  CHECK(CT::isFloat("14.1"));
  CHECK(CT::isFloat("NaN"));
  CHECK(CT::isFloat("  -15.0  "));
  CHECK(!CT::isFloat("2019-07-28"));
}

TEST(stringHelpers, toFloat) {
  CHECK(CT::toFloat("  -15.0  ") == -15.0f);
  CHECK(CT::toInt("42") == 42);
  CHECK(CT::toLong("100000") == 100000L);
}

TEST(stringHelpers, encodeXml) {
  std::string valueToCheck = "maybe<you>are&right&amp;";
  CHECK_EQUAL("maybe&lt;you&gt;are&amp;right&amp;", CT::encodeXml(valueToCheck));
}

TEST(stringHelpers, getHex) {
  CHECK_EQUAL("FF", CT::getHex(255));
  CHECK_EQUAL("00", CT::getHex(0));
  CHECK_EQUAL("10", CT::getHex(16));
}

TEST(stringHelpers, substring) {
  std::string s = "We think in generalities, but we live in details.";
  CHECK_EQUAL("think", CT::substring(s, 3, 8));
  CHECK_EQUAL("We", CT::substring(s, 0, 2));
  CHECK_EQUAL("details.", CT::substring(s, 41, -1));
  CHECK_EQUAL("", CT::substring(s, 12, 11));
  CHECK_EQUAL("", CT::substring(s, 12, 12));
  CHECK_EQUAL("", CT::substring(s, -1, 5));
}

TEST(stringHelpers, join) {
  std::vector<std::string> v = {"a", "b", "c"};
  CHECK_EQUAL("a,b,c", CT::join(v, ","));
  CHECK_EQUAL("a-b-c", CT::join(v, "-"));
  CHECK_EQUAL("", CT::join({}, ","));
}
