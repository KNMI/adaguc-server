INCLUDEDIR=-I../hclasses  -I/usr/include/libxml2/ -I/usr/include/hdf5/serial/ -I/usr/include/udunits2/ $(CPPFLAGS) $(LDFLAGS)

ifndef BUILDER_ADAGUCCOMPILERSETTINGS
  BUILDER_ADAGUCCOMPILERSETTINGS=-Wall -DMEMLEAKCHECK -std=c++11
endif

CCOMPILER=g++ $(BUILDER_ADAGUCCOMPILERSETTINGS) $(INCLUDEDIR)

OBJECTS = CCDFDataModel.o  CCDFNetCDFIO.o CTime.o CCDFHDF5IO.o CProj4ToCF.o CCDFVariable.o CCDFObject.o CCDFTypes.o CCDFCache.o CCache.o CCDFStore.o CCDFGeoJSONIO.o CCDFCSVReader.o

default: CCDFDataModel test
CCDFDataModel: $(OBJECTS)
	ar rcs  CCDFDataModel.a $(OBJECTS) 

anydump: CCDFDataModel
	$(CCOMPILER) -c anydump.cpp -o anydump.o
	g++ "anydump.o" $(OBJECTS)  -o  "anydump"  $(LDFLAGS) -lhdf5 -lhdf5_hl -lnetcdf -lxml2 -ludunits2 ../hclasses/hclasses.a -L/usr/lib/x86_64-linux-gnu/hdf5/serial/

test: CCDFDataModel
	$(CCOMPILER) -c test.cpp -o test.o
	g++ "test.o" $(OBJECTS)  -o  "test"  $(LDFLAGS) -lhdf5 -lhdf5_hl -lnetcdf -lxml2 -ludunits2 ../hclasses/hclasses.a -L/usr/lib/x86_64-linux-gnu/hdf5/serial/
	./test
	
%.o: %.cpp
	$(CCOMPILER) -c $< -o $@

clean:
	rm -f $(OBJECTS)  CCDFDataModel.a


