

HELPERCLASSES= ../hclasses
HELPOBJECTS = $(HELPERCLASSES)/hclasses.a

CCDFDATAMODEL = ../CCDFDataModel
CCDFDATAMODELOBJ = $(CCDFDATAMODEL)/CCDFDataModel.a

INCLUDEDIR= $(CPPFLAGS) -I $(HELPERCLASSES) -I $(CCDFDATAMODEL) -I/usr/include/libxml2/  -I/usr/include/udunits2/ -I/usr/include/hdf5/serial/ -I/usr/include/freetype2/ -I/usr/include/freetype2/freetype/ -I/usr/include/postgresql/ -I/usr/include/cairo/ -I/usr/include/gdal/

ifndef BUILDER_ADAGUCCOMPILERSETTINGS
  BUILDER_ADAGUCCOMPILERSETTINGS=-Wall -DMEMLEAKCHECK -g -std=c++11 -pipe
endif

ifdef ADAGUCCOMPONENTS
BUILDER_ADAGUCCOMPONENTS=$(ADAGUCCOMPONENTS)
endif

ifndef BUILDER_ADAGUCCOMPONENTS
  BUILDER_ADAGUCCOMPONENTS=-DENABLE_CURL -DADAGUC_USE_GDAL -DADAGUC_USE_SQLITE -DADAGUC_USE_POSTGRESQL
endif



ifneq (,$(findstring DENABLE_CURL,$(BUILDER_ADAGUCCOMPONENTS)))
LIB_CURL=-lcurl
endif
ifneq (,$(findstring DADAGUC_USE_GDAL,$(BUILDER_ADAGUCCOMPONENTS)))
LIB_GDAL=-lgdal
endif
ifneq (,$(findstring DADAGUC_USE_SQLITE,$(BUILDER_ADAGUCCOMPONENTS)))
LIB_SQLITE=-lsqlite3
endif
ifneq (,$(findstring DADAGUC_USE_POSTGRESQL,$(BUILDER_ADAGUCCOMPONENTS)))
LIB_PQ=-lpq
endif
ifneq (,$(findstring DADAGUC_USE_KDCMONGODB,$(BUILDER_ADAGUCCOMPONENTS)))
LIB_MONGODB=-l:libboost_system.so.1.55.0 -l:libboost_thread.so.1.55.0 -l:libboost_filesystem.so.1.55.0 -l:libboost_program_options.so.1.55.0 -lmongoclient
endif
ifneq (,$(findstring DADAGUC_USE_WEBP,$(BUILDER_ADAGUCCOMPONENTS)))
LIB_WEBP=-lwebp
endif


LIB_CAIRO=-lcairo


CCOMPILER=g++ $(BUILDER_ADAGUCCOMPILERSETTINGS) $(INCLUDEDIR)


#CCOMPILER=g++ -Wall $(INCLUDEDIR) 

#optimalization options:
#CCOMPILER=g++ -march=pentium4 -mtune=pentium4 -msse -msse2 -msse3 -mssse3 -mfpmath=sse -O2 $(INCLUDEDIR) 
#CCOMPILER=g++ -march=core2 -mtune=core2 -msse -msse2 -msse3 -mssse3 -mfpmath=sse -O2 $(INCLUDEDIR) 
#CCOMPILER=g++ -O2 $(INCLUDEDIR)
#CCOMPILER=g++ -march=k8-sse3 -mtune=k8-sse3 -msse -msse2 -msse3 -mssse3 -mfpmath=sse -O2 $(INCLUDEDIR)  

USERLIBS= -lhdf5 -lhdf5_hl -lnetcdf -lxml2 -lgd -lproj  -ludunits2 -lfreetype -lgd -lpng -lpthread -lrt $(LIB_CAIRO) $(LIB_CURL) $(LIB_GDAL) $(LIB_PQ) $(LIB_SQLITE) $(LIB_MONGODB) $(LIB_WEBP) 




# all libs together
LIBS = $(USERLIBS) $(LDFLAGS) -L$(INSTALLDIR2)/lib -L/usr/lib/x86_64-linux-gnu/hdf5/serial/
#-L/usr/lib64

OBJECTS = CReadPNG.o CCDFPNGIO.o CDataReader.o COGCDims.o CImageWarper.o CGeoParams.o CCairoPlotter.o CDrawImage.o CServerError.o CRequest.o  CXMLGen.o CServerParams.o CGDALDataWriter.o CImageDataWriter.o CXMLSerializerInterface.o CDataSource.o CImgWarpBilinear.o CImgWarpBoolean.o CImgWarpNearestNeighbour.o CGenericDataWarper.o CImgWarpNearestRGBA.o CPGSQLDB.o CDBFileScanner.o CDFObjectStore.o CFillTriangle.o CConvertASCAT.o CConvertUGRIDMesh.o CConvertADAGUCVector.o CConvertEProfile.o CConvertADAGUCPoint.o CImgRenderPoints.o CConvertCurvilinear.o CConvertHexagon.o CInspire.o CGetFileInfo.o CStyleConfiguration.o CMakeJSONTimeSeries.o COpenDAPHandler.o CDataPostProcessor.o CDBFactory.o CDBAdapterPostgreSQL.o CAutoResource.o CDBAdapterSQLLite.o CDBAdapterMongoDB.o COctTreeColorQuantizer.o CCreateLegend.o CCreateHistogram.o CNetCDFDataWriter.o CAutoConfigure.o CMakeEProfile.o CImgRenderStippling.o CConvertGeoJSON.o  CGeoJSONData.o json.o CCreateScaleBar.o CConvertTROPOMI.o CImgRenderPolylines.o CAreaMapper.o CSLD.o

EXECUTABLE= adagucserver

default: $(EXECUTABLE)
$(EXECUTABLE): $(OBJECTS) $(EXECUTABLE).o h5ncdump.o aggregate_time.o aggregate_dim.o geojsondump.o 
	g++ $(EXECUTABLE).o $(OBJECTS) $(HELPOBJECTS) $(CCDFDATAMODELOBJ) -o  $(EXECUTABLE) $(LIBS)
	g++ "h5ncdump.o" $(CCDFDATAMODELOBJ) $(HELPOBJECTS) -o  "h5ncdump" $(LIBS)
	g++ "aggregate_time.o" $(CCDFDATAMODELOBJ)  $(HELPOBJECTS) -o  "aggregate_time" $(LIBS)
	g++ "aggregate_dim.o"  $(CCDFDATAMODELOBJ) $(HELPOBJECTS) -o  "aggregate_dim" $(LIBS)
	g++ "geojsondump.o"  $(OBJECTS) $(CCDFDATAMODELOBJ) $(HELPOBJECTS)  -o  "geojsondump" $(LIBS)
	
	
geojsondump: geojsondump.o 
	g++ "geojsondump.o" $(CCDFDATAMODELOBJ) $(HELPOBJECTS) -o  "geojsondump" $(LIBS)
	
	
%.o: %.cpp
	$(CCOMPILER) -c $< -o $@ $(BUILDER_ADAGUCCOMPONENTS)

clean:
	rm $(OBJECTS) $(EXECUTABLE) $(EXECUTABLE).o h5ncdump.o h5ncdump aggregate_time.o aggregate_time aggregate_dim.o aggregate_dim geojsondump.o  geojsondump
run:
	make
	./$(EXECUTABLE)


