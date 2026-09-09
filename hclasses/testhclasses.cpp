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

TEST(string, isNumeric) {
  CHECK(!CT::isNumeric(""));
  CHECK(!CT::isNumeric("."));
  CHECK(!CT::isNumeric("HELLO"));
  CHECK(CT::isNumeric("14"));
  CHECK(CT::isNumeric("14.1"));
}

TEST(string, isInt) {
  CHECK(!CT::isInt(""));
  CHECK(!CT::isInt("."));
  CHECK(!CT::isInt("HELLO"));
  CHECK(CT::isInt("14"));
  CHECK(!CT::isInt("14.1"));
  CHECK(CT::isInt("-15"));
}

TEST(string, isFloat) {
  CHECK(!CT::isFloat(""));
  CHECK(!CT::isFloat("."));
  CHECK(!CT::isFloat("HELLO"));
  CHECK(!CT::isFloat("15"));
  CHECK(CT::isFloat("14.1"));
  CHECK(CT::isFloat("15.0"));
  CHECK(CT::isFloat("NaN"));
  CHECK(!CT::isFloat("2019-07-28"));
  CHECK(CT::isFloat("-15.0"));
  CHECK(CT::isFloat("  -15.0  "));
  CHECK(atof(CT::trim("  -15.0  ").c_str()) == -15.0);
}

TEST(string, fromCharPointer) {
  std::string a = CT::fromCharPointer((const char *)NULL);
  CHECK(a.empty());
}

TEST(string, split) {
  std::vector<std::string> splittedRefs = CT::split("abc,def,,ghi", ",");
  CHECK(splittedRefs.size() == 4);
  CHECK_EQUAL(splittedRefs[0], "abc");
  CHECK_EQUAL(splittedRefs[1], "def");
  CHECK_EQUAL(splittedRefs[2], "");
  CHECK_EQUAL(splittedRefs[3], "ghi");
}

TEST(string, splitMultiComma) {
  std::vector<std::string> splittedRefs = CT::split("abc,def,,,ghi", ",");
  CHECK(splittedRefs.size() == 5);
  CHECK_EQUAL(splittedRefs[0], "abc");
  CHECK_EQUAL(splittedRefs[1], "def");
  CHECK_EQUAL(splittedRefs[2], "");
  CHECK_EQUAL(splittedRefs[3], "");
  CHECK_EQUAL(splittedRefs[4], "ghi");
}

TEST(string, splitLinesAndComma) {
  std::string linesToSplit = "A1,B1,C1,D1,E1\nA2,B2,,D2,E2\n\nA,B,C,D,E\n,,,,\nG1\n,,,,TEST";
  std::vector<std::string> lines = CT::split(linesToSplit, "\n");
  LONGS_EQUAL(7, lines.size());

  std::vector<std::string> splittedRefs0 = CT::split(lines[0], ",");

  LONGS_EQUAL(5, splittedRefs0.size());
  CHECK_EQUAL("A1", splittedRefs0[0]);
  CHECK_EQUAL("B1", splittedRefs0[1]);
  CHECK_EQUAL("C1", splittedRefs0[2]);
  CHECK_EQUAL("D1", splittedRefs0[3]);
  CHECK_EQUAL("E1", splittedRefs0[4]);
  std::vector<std::string> splittedRefs1 = CT::split(lines[1], ",");
  LONGS_EQUAL(5, splittedRefs1.size());
  CHECK_EQUAL("A2", splittedRefs1[0]);
  CHECK_EQUAL("B2", splittedRefs1[1]);
  CHECK_EQUAL(std::string(""), splittedRefs1[2]);
  CHECK_EQUAL("D2", splittedRefs1[3]);
  CHECK_EQUAL("E2", splittedRefs1[4]);
  std::vector<std::string> splittedRefs3 = CT::split(lines[3], ",");
  LONGS_EQUAL(5, splittedRefs3.size());
  std::vector<std::string> splittedRefs4 = CT::split(lines[4], ",");
  LONGS_EQUAL(4, splittedRefs4.size());
  std::vector<std::string> splittedRefs5 = CT::split(lines[5], ",");
  LONGS_EQUAL(1, splittedRefs5.size());
  std::vector<std::string> splittedRefs6 = CT::split(lines[6], ",");
  LONGS_EQUAL(5, splittedRefs6.size());
  CHECK_EQUAL("TEST", splittedRefs6[4]);
}

TEST(string, splitLinesAndCommaDoubleSplit) {
  std::string linesToSplit = "A1,B1,C1,D1,E1\n\rA2,B2,,D2,E2\n\r\n\rA,B,C,D,E\n\r,,,,\n\rG1\n\r,,,,TEST";
  std::vector<std::string> lines = CT::split(linesToSplit, "\n\r");
  LONGS_EQUAL(7, lines.size());

  std::vector<std::string> splittedRefs0 = CT::split(lines[0], ",");
  LONGS_EQUAL(5, splittedRefs0.size());
  CHECK_EQUAL("A1", splittedRefs0[0]);
  CHECK_EQUAL("B1", splittedRefs0[1]);
  CHECK_EQUAL("C1", splittedRefs0[2]);
  CHECK_EQUAL("D1", splittedRefs0[3]);
  CHECK_EQUAL("E1", splittedRefs0[4]);
  std::vector<std::string> splittedRefs1 = CT::split(lines[1], ",");
  LONGS_EQUAL(5, splittedRefs1.size());
  CHECK_EQUAL("A2", splittedRefs1[0]);
  CHECK_EQUAL("B2", splittedRefs1[1]);
  CHECK_EQUAL(std::string(""), splittedRefs1[2]);
  CHECK_EQUAL("D2", splittedRefs1[3]);
  CHECK_EQUAL("E2", splittedRefs1[4]);
  std::vector<std::string> splittedRefs3 = CT::split(lines[3], ",");
  LONGS_EQUAL(5, splittedRefs3.size());
  std::vector<std::string> splittedRefs4 = CT::split(lines[4], ",");
  LONGS_EQUAL(4, splittedRefs4.size());
  std::vector<std::string> splittedRefs5 = CT::split(lines[5], ",");
  LONGS_EQUAL(1, splittedRefs5.size());
  std::vector<std::string> splittedRefs6 = CT::split(lines[6], ",");
  LONGS_EQUAL(5, splittedRefs6.size());
  CHECK_EQUAL("TEST", splittedRefs6[4]);
}

TEST(string, splitOnString) {
  std::string linesToSplit = "Line1<SEP>Line2<SEP>Line3<SEP>Line4<SEP>Line5<SEP>Line6<SEP>Line7";
  std::vector<std::string> lines = CT::split(linesToSplit, "<SEP>");
  LONGS_EQUAL(7, lines.size());
  CHECK_EQUAL("Line1", lines[0]);
  CHECK_EQUAL("Line7", lines[6]);
}

TEST(string, splitOnKeyWhichRepeats) {
  std::string linesToSplit = "aaaaaaaaaaaaaaaaaaaaaaaaa";
  // For now this behavior is to be expected
  std::vector<std::string> lines = CT::split(linesToSplit, "aaa");
  LONGS_EQUAL(9, lines.size());
}

TEST(string, substring) {
  std::string stringToSubstitute = "We think in generalities, but we live in details.";
  CHECK_EQUAL("think", CT::substring(stringToSubstitute, 3, 8));
  CHECK_EQUAL("We", CT::substring(stringToSubstitute, 0, 2));
  CHECK_EQUAL("g", CT::substring(stringToSubstitute, 12, 13));
  CHECK_EQUAL("details.", CT::substring(stringToSubstitute, 41, -1));
  CHECK_EQUAL("details.", CT::substring(stringToSubstitute, 41, 101));
  CHECK_EQUAL("", CT::substring(stringToSubstitute, 12, 12));
  CHECK_EQUAL("", CT::substring(stringToSubstitute, 12, 11));
  CHECK_EQUAL("generalities, but we live in details.", CT::substring(stringToSubstitute, 12, -11));
  CHECK_EQUAL("", CT::substring(stringToSubstitute, -1, 5));
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

TEST(string, replaceSelf) {
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

TEST(string, indexOf) {
  std::string valueToCheck = "Hello planet earth, you are a great planet.";
  LONGS_EQUAL(-1, CT::indexOf(valueToCheck, "mars"));
  LONGS_EQUAL(6, CT::indexOf(valueToCheck, "planet"));
  LONGS_EQUAL(0, CT::indexOf(valueToCheck, "Hello"));
  LONGS_EQUAL(0, CT::indexOf(valueToCheck, ""));
  LONGS_EQUAL(0, CT::indexOf(valueToCheck, "Hello planet earth, you are a great planet."));
  LONGS_EQUAL(-1, CT::indexOf(valueToCheck, "Hello planet earth, you are a great planet. ---------------------------"));
  LONGS_EQUAL(-1, CT::indexOf(valueToCheck, "planet earth, you are a great planet. ---------------------------"));
}

TEST(string, lastIndexOf) {
  std::string valueToCheck = "Hello planet earth, you are a great planet.";
  LONGS_EQUAL(-1, CT::lastIndexOf(valueToCheck, "mars"));
  LONGS_EQUAL(36, CT::lastIndexOf(valueToCheck, "planet"));
  LONGS_EQUAL(0, CT::lastIndexOf(valueToCheck, "Hello"));
  LONGS_EQUAL(0, CT::lastIndexOf(valueToCheck, ""));
  LONGS_EQUAL(0, CT::lastIndexOf(valueToCheck, "Hello planet earth, you are a great planet."));
  LONGS_EQUAL(-1, CT::lastIndexOf(valueToCheck, "Hello planet earth, you are a great planet. ---------------------------"));
  LONGS_EQUAL(-1, CT::lastIndexOf(valueToCheck, "planet earth, you are a great planet. ---------------------------"));
}

TEST(string, endsWith) {
  std::string valueToCheck = "Hello planet earth, you are a great planet.";
  LONGS_EQUAL(false, CT::endsWith(valueToCheck, "mars"));
  LONGS_EQUAL(true, CT::endsWith(valueToCheck, "planet."));
  LONGS_EQUAL(false, CT::endsWith(valueToCheck, "Hello"));
  LONGS_EQUAL(true, CT::endsWith(valueToCheck, ""));
}

TEST(string, startsWith) {
  std::string valueToCheck = "Hello planet earth, you are a great planet.";
  LONGS_EQUAL(false, CT::startsWith(valueToCheck, "mars"));
  LONGS_EQUAL(false, CT::startsWith(valueToCheck, "planet."));
  LONGS_EQUAL(true, CT::startsWith(valueToCheck, "Hello"));
  LONGS_EQUAL(true, CT::startsWith(valueToCheck, ""));

  // Note, specifying a stdstring with a starting and ending double qoute as part of the string did not work previously.
  std::string n = "\" +proj=sterea +lat_0=52.15616055555555 +lon_0=5.38763888888889 +k=0.9999079 +x_0=155000 +y_0=463000 +ellps=bessel +units=m +no_defs +<>\"";
  LONGS_EQUAL(true, CT::startsWith(n, "\""));
}

TEST(string, encodeXml) {
  std::string valueToCheck = "maybe<you>are&right&amp;";
  CHECK_EQUAL("maybe&lt;you>are&amp;right&amp;", CT::encodeXml(valueToCheck));
}