HELPERCLASSES= ../hclasses
HELPOBJECTS = $(HELPERCLASSES)/hclasses.a

CCDFDATAMODEL = ../CCDFDataModel
CCDFDATAMODELOBJ = $(CCDFDATAMODEL)/CCDFDataModel.a

INCLUDEDIR= $(CPPFLAGS) -I $(HELPERCLASSES) -I $(CCDFDATAMODEL) -I/usr/include/libxml2/  -I/usr/include/udunits2/ -I/usr/include/freetype2/ -I/usr/include/freetype2/freetype/ -I/usr/include/postgresql/ -I/usr/include/cairo/ -I/usr/include/gdal/

ifndef ADAGUCCOMPILERSETTINGS
  ADAGUCCOMPILERSETTINGS=-Wall
endif

CCOMPILER=g++ $(ADAGUCCOMPILERSETTINGS) $(INCLUDEDIR)


#CCOMPILER=g++ -Wall $(INCLUDEDIR) 

#optimalization options:
#CCOMPILER=g++ -march=pentium4 -mtune=pentium4 -msse -msse2 -msse3 -mssse3 -mfpmath=sse -O2 $(INCLUDEDIR) 
#CCOMPILER=g++ -march=core2 -mtune=core2 -msse -msse2 -msse3 -mssse3 -mfpmath=sse -O2 $(INCLUDEDIR) 
#CCOMPILER=g++ -O2 $(INCLUDEDIR)
#CCOMPILER=g++ -march=k8-sse3 -mtune=k8-sse3 -msse -msse2 -msse3 -mssse3 -mfpmath=sse -O2 $(INCLUDEDIR)  

# USERLIBS= -lhdf5 -lhdf5_hl -lnetcdf -lxml2 -lgd -lproj -lpq -ludunits2 -lfreetype -lgd -lpng -lpthread  -lrt

USERLIBS= -lhdf5 -lhdf5_hl -lnetcdf -lxml2 -lgd -lproj -lpq -ludunits2 -lfreetype -lgd -lpng -lpthread  -lrt -lcurl -lgdal -lcairo -lsqlite3

# all libs together
LIBS = $(USERLIBS) $(LDFLAGS) -L$(INSTALLDIR2)/lib  
#-L/usr/lib64

OBJECTS = CDataReader.o COGCDims.o CImageWarper.o CGeoParams.o CCairoPlotter.o CDrawImage.o CServerError.o CRequest.o  CXMLGen.o CServerParams.o CGDALDataWriter.o CImageDataWriter.o CXMLSerializerInterface.o CDataSource.o CStopWatch.o CImgWarpBilinear.o CImgWarpBoolean.o CImgWarpNearestNeighbour.o CGenericDataWarper.o CImgWarpNearestRGBA.o CPGSQLDB.o CDBFileScanner.o CDFObjectStore.o CFillTriangle.o CConvertASCAT.o CConvertUGRIDMesh.o CConvertADAGUCVector.o CConvertADAGUCPoint.o CImgRenderPoints.o CConvertCurvilinear.o CInspire.o CGetFileInfo.o CStyleConfiguration.o CMakeJSONTimeSeries.o COpenDAPHandler.o CDataPostProcessor.o CDBFactory.o CDBAdapterPostgreSQL.o CDBAdapterSQLLite.o

EXECUTABLE= adagucserver

default: $(EXECUTABLE)
$(EXECUTABLE): $(OBJECTS) $(EXECUTABLE).o h5ncdump.o aggregate_time.o
	g++ $(EXECUTABLE).o $(OBJECTS) $(HELPOBJECTS) $(CCDFDATAMODELOBJ) -o  $(EXECUTABLE) $(LIBS) 
	g++ "h5ncdump.o" $(HELPOBJECTS) $(CCDFDATAMODELOBJ) -o  "h5ncdump" $(LIBS)
	g++ "aggregate_time.o" $(HELPOBJECTS) $(CCDFDATAMODELOBJ) -o  "aggregate_time" $(LIBS)

%.o: %.cpp
	$(CCOMPILER) -c $< -o $@  -DENABLE_CURL -DADAGUC_USE_CAIRO -DADAGUC_USE_GDAL -DADAGUC_USE_SQLITE

clean:
	rm $(OBJECTS) $(EXECUTABLE) $(EXECUTABLE).o h5ncdump.o h5ncdump aggregate_time.o aggregate_time
run:
	make
	./$(EXECUTABLE)		


