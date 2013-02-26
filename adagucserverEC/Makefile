HELPERCLASSES= ../hclasses
HELPOBJECTS = $(HELPERCLASSES)/hclasses.a

CCDFDATAMODEL = ../CCDFDataModel
CCDFDATAMODELOBJ = $(CCDFDATAMODEL)/CCDFDataModel.a

#INCLUDEDIR=$(SRCLIBS)/include -I $(HELPERCLASSES) -I $(CCDFDATAMODEL) -I /usr/include/libxml2/ -I/usr/include/udunits2/ -I/usr/include/gdal/ -I $(SRCLIBS)/include/freetype2 -I $($INSTALLDIR2)/include/freetype2 -I /data/data1/software/install/include/freetype2 -I /data/data1/software/install/include/ -I/usr/include/freetype2/ -I /usr/include/postgresql/ -I /usr/include/cairo/
INCLUDEDIR=-I $(HELPERCLASSES) -I $(CCDFDATAMODEL) -I/usr/include/libxml2/  -I/usr/include/udunits2/ -I/usr/include/freetype2/ -I/usr/include/postgresql/ -I/usr/include/cairo/ -I/usr/include/gdal/ $(CPPFLAGS)



CCOMPILER=g++ -Wall $(INCLUDEDIR) 

#optimalization options:
#CCOMPILER=g++ -march=pentium4 -mtune=pentium4 -msse -msse2 -msse3 -mssse3 -mfpmath=sse -O2 $(INCLUDEDIR) 
#CCOMPILER=g++ -march=core2 -mtune=core2 -msse -msse2 -msse3 -mssse3 -mfpmath=sse -O2 $(INCLUDEDIR) 
#CCOMPILER=g++ -O2 $(INCLUDEDIR)
#CCOMPILER=g++ -march=k8-sse3 -mtune=k8-sse3 -msse -msse2 -msse3 -mssse3 -mfpmath=sse -O2 $(INCLUDEDIR)  

USERLIBS= -lhdf5 -lhdf5_hl -lnetcdf -lxml2 -lgd -lproj -lpq -ludunits2 -lfreetype -lgd -lpng -lrt -lpthread -lcurl -lcairo -lgdal 

# all libs together
LIBS = $(USERLIBS) $(LDFLAGS) -L$(INSTALLDIR2)/lib  
#-L/usr/lib64

OBJECTS = CDataReader.o CImageWarper.o CGeoParams.o CCairoPlotter.o CDrawImage.o CServerError.o CRequest.o  CXMLGen.o CServerParams.o CGDALDataWriter.o CImageDataWriter.o CXMLSerializerInterface.o CDataSource.o CStopWatch.o CImgWarpBilinear.o CImgWarpBoolean.o CImgWarpNearestNeighbour.o CImgWarpNearestRGBA.o CPGSQLDB.o CDBFileScanner.o CDFObjectStore.o CDrawAA.o CFillTriangle.o CConvertASCAT.o CConvertADAGUCVector.o CConvertADAGUCPoint.o CImgRenderPoints.o CCache.o

EXECUTABLE= adagucserver

default: $(EXECUTABLE)
$(EXECUTABLE): $(OBJECTS) $(EXECUTABLE).o h5ncdump.o
	g++ $(EXECUTABLE).o $(OBJECTS) $(HELPOBJECTS) $(CCDFDATAMODELOBJ) -o  $(EXECUTABLE) $(LIBS)
	g++ "h5ncdump.o" $(HELPOBJECTS) $(CCDFDATAMODELOBJ) -o  "h5ncdump" $(LIBS)

%.o: %.cpp
	$(CCOMPILER) -c $< -o $@

clean:
	rm $(OBJECTS) $(EXECUTABLE) $(EXECUTABLE).o h5ncdump.o h5ncdump
run:
	make
	./$(EXECUTABLE)		
