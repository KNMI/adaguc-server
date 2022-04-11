add_library(CppUnitLite
   CppUnitLite/Failure.h
   CppUnitLite/SimpleString.h
   CppUnitLite/TestHarness.h
   CppUnitLite/TestRegistry.h
   CppUnitLite/TestResult.h
   CppUnitLite/Test.h
   src/Failure.cpp
   src/SimpleString.cpp
   src/TestRegistry.cpp
   src/TestResult.cpp
   src/Test.cpp
)

target_include_directories(CppUnitLite PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}  CppUnitLite CppUnitLite)

