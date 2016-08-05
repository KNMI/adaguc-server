INCLUDEDIR=-I../hclasses  -I/usr/include/libxml2/ -I/usr/include/hdf5/serial/ -I/usr/include/udunits2/ $(CPPFLAGS) $(LDFLAGS)

ifndef BUILDER_ADAGUCCOMPILERSETTINGS
  BUILDER_ADAGUCCOMPILERSETTINGS=-Wall -DMEMLEAKCHECK
endif

CCOMPILER=g++ $(BUILDER_ADAGUCCOMPILERSETTINGS) $(INCLUDEDIR)

OBJECTS = CCDFDataModel.o  CCDFNetCDFIO.o CTime.o CCDFHDF5IO.o CProj4ToCF.o CCDFVariable.o CCDFObject.o CCDFTypes.o CCDFCache.o CCache.o CCDFStore.o CCDFGeoJSONIO.o CGeoJSONData.o

default: CCDFDataModel
CCDFDataModel: $(OBJECTS)
	ar rcs  CCDFDataModel.a $(OBJECTS) 

anydump: CCDFDataModel
	$(CCOMPILER) -c anydump.cpp -o anydump.o
	g++ "anydump.o" $(OBJECTS)  -o  "anydump"  $(LDFLAGS) -lhdf5 -lhdf5_hl -lnetcdf -lxml2 -ludunits2 ../hclasses/hclasses.a
	
%.o: %.cpp
	$(CCOMPILER) -c $< -o $@

clean:
	rm -f $(OBJECTS)  CCDFDataModel.a


