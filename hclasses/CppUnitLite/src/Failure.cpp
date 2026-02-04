#include "Failure.h"

#include <cstdio>
#include <cstring>


Failure::Failure (const SimpleString& theTestName,
                  const SimpleString& theFileName,
                  long theLineNumber,
                  const SimpleString& theCondition)
    : message(theCondition),
      testName(theTestName),
      fileName(theFileName),
      lineNumber(theLineNumber)
{}


Failure::Failure (const SimpleString& theTestName,
                  const SimpleString& theFileName,
                  long theLineNumber,
                  const SimpleString& expected,
                  const SimpleString& actual)
    : testName(theTestName),
      fileName(theFileName),
      lineNumber(theLineNumber) {
  const char *part1 = "expected ";
  const char *part3 = " but was: ";

  size_t len = strlen(part1)
               + expected.size ()
               + strlen(part3)
               + actual.size ()
               + 1;

  char *stage = new char [len];

  snprintf(stage, len, "%s%s%s%s",
           part1,
           expected.asCharString(),
           part3,
           actual.asCharString());

  message = SimpleString(stage);

  delete[] stage;
}
