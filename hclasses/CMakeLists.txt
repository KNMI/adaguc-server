find_package(LibXml2 REQUIRED)

add_library(hclasses
    CTypes.h
    CTString.h
    CTStringRef.h
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
    test.h
    CTypes.cpp
    CTString.cpp
    CTStringRef.cpp
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
    test.h
    test.cpp
)

target_include_directories(hclasses PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${LIBXML2_INCLUDE_DIR})

# Build unit test executable
add_executable(test test.cpp)
target_link_libraries(test PRIVATE hclasses)