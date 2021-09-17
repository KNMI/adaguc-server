INCLUDEDIR=. -I/usr/include/libxml2/ $(CPPFLAGS) $(LDFLAGS)


ifndef BUILDER_ADAGUCCOMPILERSETTINGS
  BUILDER_ADAGUCCOMPILERSETTINGS=-g -Wall -DMEMLEAKCHECK -std=c++11
endif

CCOMPILER=g++ $(BUILDER_ADAGUCCOMPILERSETTINGS) -I $(INCLUDEDIR)

OBJECTS =  CTypes.o CTString.o CTStringRef.o CXMLParser.o CDebugger.o CDirReader.o CStopWatch.o CHTTPTools.o CReadFile.o CReporter.o CReportMessage.o CReportWriter.o json_adaguc.o testhclasses.o

EXECUTABLE= hclasses

ifndef ADAGUCCOMPONENTS
  ADAGUCCOMPONENTS=-DENABLE_CURL
endif


ifneq (,$(findstring DENABLE_CURL,$(ADAGUCCOMPONENTS)))
ENABLE_CURL=-DENABLE_CURL
endif

default: $(EXECUTABLE) 
$(EXECUTABLE): $(OBJECTS) 
	ar rcs $(EXECUTABLE).a $(OBJECTS)  
	
%.o: %.cpp
	$(CCOMPILER) -c $< -o $@  $(ENABLE_CURL)

clean:
	rm -f $(OBJECTS) $(EXECUTABLE).a test

	
test: $(OBJECTS) $(EXECUTABLE)
	g++ $(OBJECTS) -o  testhclasses  $(LDFLAGS)  -lxml2 
	./test

	
