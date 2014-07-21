INCLUDEDIR=-I../hclasses  -I/usr/include/libxml2/ -I/usr/include/udunits2/ $(CPPFLAGS) $(LDFLAGS)

CCOMPILER=g++ -Wall $(INCLUDEDIR) 

#optimalization options:
#CCOMPILER=g++ -march=pentium4 -mtune=pentium4 -msse -msse2 -msse3 -mssse3 -mfpmath=sse -O2 $(INCLUDEDIR) 
#CCOMPILER=g++ -march=core2 -mtune=core2 -msse -msse2 -msse3 -mssse3 -mfpmath=sse -O2 $(INCLUDEDIR) 
#CCOMPILER=g++ -O2 $(INCLUDEDIR)
#CCOMPILER=g++ -march=k8-sse3 -mtune=k8-sse3 -msse -msse2 -msse3 -mssse3 -mfpmath=sse -O2 $(INCLUDEDIR)  

OBJECTS = CCDFDataModel.o  CCDFNetCDFIO.o CTime.o CCDFHDF5IO.o CProj4ToCF.o CCDFVariable.o CCDFObject.o CCDFTypes.o CCDFCache.o CCache.o CCDFStore.o

EXECUTABLE= CCDFDataModel

default: $(EXECUTABLE)
$(EXECUTABLE): $(OBJECTS)
	ar rcs  $(EXECUTABLE).a $(OBJECTS) 

%.o: %.cpp
	$(CCOMPILER) -c $< -o $@

clean:
	rm $(OBJECTS)  $(EXECUTABLE).a


