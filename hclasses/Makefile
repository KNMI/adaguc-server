INCLUDEDIR=. -I/usr/include/libxml2/ $(CPPFLAGS) $(LDFLAGS)


ifndef ADAGUCCOMPILERSETTINGS
  ADAGUCCOMPILERSETTINGS=-Wall
endif

CCOMPILER=g++ $(ADAGUCCOMPILERSETTINGS) -I $(INCLUDEDIR)

OBJECTS =  CTypes.o CXMLParser.o CDebugger.o CDirReader.o

EXECUTABLE= hclasses

default: $(EXECUTABLE) 
$(EXECUTABLE): $(OBJECTS) 
	ar rcs $(EXECUTABLE).a $(OBJECTS)  

%.o: %.cpp
	$(CCOMPILER) -c $< -o $@  -DENABLE_CURL

clean:
	rm $(OBJECTS) $(EXECUTABLE).a


	
