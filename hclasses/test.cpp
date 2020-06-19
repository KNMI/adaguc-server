#include "CDirReader.h"
#include "CDebugger.h"

DEF_ERRORMAIN ()

int main(){
  
  CDirReader::test_makeCleanPath();
  CDirReader::test_compareLists();

  try {
    CT::string a = "teststring_a";
    if (!a.equals("teststring_a")) throw __LINE__;
    CT::string c = a + "b";
    if (!a.equals("teststring_a")) throw __LINE__;
    if (!c.equals("teststring_ab")) throw __LINE__;
    a+="bc";
    if (!a.equals("teststring_abc")) throw __LINE__;
    a+=CT::string("d") + "e";
    if (!a.equals("teststring_abcde")) throw __LINE__;
    CT::string b = "f";
    if (!a.equals("teststring_abcde")) throw __LINE__;
    CT::string d = a + b;
    if (!a.equals("teststring_abcde")) throw __LINE__;
  }catch(int e) {
    CDBError("Error at line %d", e);

  }

  /* CT::string.isNumeric() tests */

  if (CT::string("").isNumeric() == true) {
    CDBError("[] should not be numeric");
    throw __LINE__;
  }

  if (CT::string(".").isNumeric() == true) {
    CDBError("[.] should not be numeric");
    throw __LINE__;
  }

  if (CT::string("HELLO").isNumeric() == true) {
    CDBError("[HELLO] should not be numeric");
    throw __LINE__;
  }

  if (CT::string("14").isNumeric() == false) {
    CDBError("[14] should be numeric");
    throw __LINE__;
  }

  if (CT::string("14.1").isNumeric() == false) {
    CDBError("[14.1] should be numeric");
    throw __LINE__;
  }

/* CT::string.isInt() tests */

  if (CT::string("").isInt() == true) {
    CDBError("[] should not be int");
    throw __LINE__;
  }

  if (CT::string(".").isInt() == true) {
    CDBError("[.] should not be int");
    throw __LINE__;
  }

  if (CT::string("HELLO").isInt() == true) {
    CDBError("[HELLO] should not be int");
    throw __LINE__;
  }

  if (CT::string("14").isInt() == false) {
    CDBError("[14] should be int");
    throw __LINE__;
  }

  if (CT::string("14.1").isInt() == true) {
    CDBError("[14.1] should not be int");
    throw __LINE__;
  }

  /* CT::string.isFloat() tests */
  if (CT::string("").isFloat() == true) {
    CDBError("[] should not be float");
    throw __LINE__;
  }

  if (CT::string(".").isFloat() == true) {
    CDBError("[.] should not be float");
    throw __LINE__;
  }

  if (CT::string("HELLO").isFloat() == true) {
    CDBError("[HELLO] should not be float");
    throw __LINE__;
  }

  if (CT::string("15").isFloat() == true) {
    CDBError("[15] should not be float");
    throw __LINE__;
  }

  if (CT::string("14.1").isFloat() == false) {
    CDBError("[14.1] should be a float");
    throw __LINE__;
  }

  if (CT::string("15.0").isFloat() == false) {
    CDBError("[15.0] should be a float");
    throw __LINE__;
  }

  if (CT::string("NaN").isFloat() == false) {
    CDBError("[NaN] should be a float");
    throw __LINE__;
  }
  
  
  
  return 0;
}
