find_package(LibXml2 REQUIRED)

add_library(hclasses
CTString.h
    CXMLParser.h
    CDebugger.h
    CDirReader.h
    CStopWatch.h
    CHTTPTools.h
    CReadFile.h
    CReporter.h
    CReportMessage.h
    CReportWriter.h
    json_adaguc.h
    CKeyValuePair.h
    CTString.cpp
    CXMLParser.cpp
    CDebugger.cpp
    CDirReader.cpp
    CStopWatch.cpp
    CHTTPTools.cpp
    CReadFile.cpp
    CReporter.cpp
    CReportMessage.cpp
    CReportWriter.cpp
    json_adaguc.cpp
    testhclasses.cpp
)


add_subdirectory(CppUnitLite)

target_include_directories(hclasses PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${LIBXML2_INCLUDE_DIR}  CppUnitLite)

# Build unit test executable
add_executable(testhclasses testhclasses.cpp)
target_link_libraries(testhclasses PRIVATE hclasses CppUnitLite)


