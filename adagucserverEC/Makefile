#INSTALLDIR=/data/apps/rcc/MaartenP/install
INSTALLDIR=/nobackup/users/plieger/software/install
#INSTALLDIR=/data/data1/mapserver/
#INSTALLDIR2=/data/data1/software/install/
#INSTALLDIR=/INSTALLDIR/plieger/interpolws/software/install
#INSTALLDIR=/nobackup/users/plieger/build/
#INSTALLDIR=/INSTALLDIR/gsie/gsiews/software/install/
#INSTALLDIR=/INSTALLDIR/plieger/software/install
#INSTALLDIR=/data/apps/rcc/MaartenP/install
#INSTALLDIR=/INSTALLDIR/plieger/software/install/
LIBDIR=$(INSTALLDIR)/lib $(INSTALLDIR2)/lib

INCLUDEDIR=$(INSTALLDIR)/include -I /usr/include/libxml2/ -I /usr/include/cairo/

SRCLIBS=$(INSTALLDIR)

LIBDIR=$(INSTALLDIR)/lib 

HELPERCLASSES="../hclasses"
HELPOBJECTS = $(HELPERCLASSES)/hclasses.a

CCDFDATAMODEL = "../CCDFDataModel"
CCDFDATAMODELOBJ = $(CCDFDATAMODEL)/CCDFDataModel.a

INCLUDEDIR=$(SRCLIBS)/include -I $(HELPERCLASSES) -I $(CCDFDATAMODEL) -I /usr/include/libxml2/ -I $(SRCLIBS)/include/freetype2 -I $($INSTALLDIR2)/include/freetype2 -I /data/data1/software/install/include/freetype2 -I /data/data1/software/install/include/ -I/usr/include/freetype2/ -I /usr/include/postgresql/ -I /usr/include/cairo/



#CCOMPILER=g++ -Wall -I $(INCLUDEDIR)

#optimalization options:
#CCOMPILER=g++ -march=pentium4 -mtune=pentium4 -msse -msse2 -msse3 -mssse3 -mfpmath=sse -O2 -I $(INCLUDEDIR) 
CCOMPILER=g++ -march=core2 -mtune=core2 -msse -msse2 -msse3 -mssse3 -mfpmath=sse -O2 -I $(INCLUDEDIR) 
#CCOMPILER=g++ -O2 -I $(INCLUDEDIR) 
#CCOMPILER=g++ -march=k8-sse3 -mtune=k8-sse3 -msse -msse2 -msse3 -mssse3 -mfpmath=sse -O2 -I $(INCLUDEDIR)  
USERLIBS= -lhdf5 -lhdf5_hl -lnetcdf -lxml2 -lgd -lproj -lpq -ludunits2 -lfreetype -lgd -lpng -lrt -lpthread -lcurl -lgdal -lcairo

# all libs together
LIBS = -L$(LIBDIR) $(USERLIBS) -L$(INSTALLDIR2)/lib

OBJECTS = CDataReader.o CImageWarper.o CDrawImage.o CServerError.o CRequest.o  CXMLGen.o CServerParams.o CGDALDataWriter.o CImageDataWriter.o CXMLSerializerInterface.o CDataSource.o CStopWatch.o CImgWarpBilinear.o CImgWarpBoolean.o CImgWarpNearestNeighbour.o CPGSQLDB.o CCairoPlotter.o CDBFileScanner.o CDFObjectStore.o

EXECUTABLE= adagucserver

default: $(EXECUTABLE)
$(EXECUTABLE): $(OBJECTS) $(EXECUTABLE).o h5ncdump.o
	g++ $(EXECUTABLE).o $(OBJECTS) $(HELPOBJECTS) $(CCDFDATAMODELOBJ) -o  $(EXECUTABLE) $(LIBS)
	g++ "h5ncdump.o" $(HELPOBJECTS) $(CCDFDATAMODELOBJ) -o  "h5ncdump" $(LIBS)

%.o: %.cpp
	$(CCOMPILER) -c $< -o $@

clean:
	rm $(OBJECTS) $(EXECUTABLE) $(EXECUTABLE).o h5ncdump.o
run:
	make
	./$(EXECUTABLE)		
