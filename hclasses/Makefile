INCLUDEDIR=. -I/usr/include/libxml2/ $(CPPFLAGS) $(LDFLAGS)


ifndef BUILDER_ADAGUCCOMPILERSETTINGS
  BUILDER_ADAGUCCOMPILERSETTINGS=-Wall -DMEMLEAKCHECK
endif

CCOMPILER=g++ -g $(BUILDER_ADAGUCCOMPILERSETTINGS) -I $(INCLUDEDIR)

OBJECTS =  CTypes.o CTString.o CXMLParser.o CDebugger.o CDirReader.o CStopWatch.o CHTTPTools.o CReadFile.o CReporter.o CReportWriter.o json_adaguc.o test.o

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
	g++ $(OBJECTS) -o  test  $(LDFLAGS)  -lxml2 
	./test

	
