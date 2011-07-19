#HOME=/data/apps/rcc/MaartenP/install
#HOME=/nobackup/users/plieger/software/install
#HOME=/data/data1/mapserver/
#HOME2=/data/data1/software/install/
#HOME=/home/plieger/interpolws/software/install
#HOME=/nobackup/users/plieger/build/
#HOME=/home/gsie/gsiews/software/install/
#HOME=/home/plieger/software/install
#HOME=/data/apps/rcc/MaartenP/install
HOME=/home/plieger/software/install/
LIBDIR=$(HOME)/lib

INCLUDEDIR=$(HOME)/include -I /usr/include/libxml2/ 

SRCLIBS=$(HOME)

LIBDIR=$(HOME)/lib 

HELPERCLASSES="../hclasses"
HELPOBJECTS = $(HELPERCLASSES)/hclasses.a

CCDFDATAMODEL = "../CCDFDataModel"
CCDFDATAMODELOBJ = $(CCDFDATAMODEL)/CCDFDataModel.a

INCLUDEDIR=$(SRCLIBS)/include -I $(HELPERCLASSES) -I $(CCDFDATAMODEL) -I /usr/include/libxml2/ -I $(SRCLIBS)/include/freetype2 -I $($HOME2)/include/freetype2 -I /data/data1/software/install/include/freetype2 -I /data/data1/software/install/include/ -I/usr/include/freetype2/ -I /usr/include/postgresql/


CCOMPILER=g++ -Wall -I $(INCLUDEDIR)

#optimalization options:
#CCOMPILER=g++ -march=pentium4 -mtune=pentium4 -msse -msse2 -msse3 -mssse3 -mfpmath=sse -O2 -I $(INCLUDEDIR) 
#CCOMPILER=g++ -march=core2 -mtune=core2 -msse -msse2 -msse3 -mssse3 -mfpmath=sse -O2 -I $(INCLUDEDIR) 
#CCOMPILER=g++ -O2 -I $(INCLUDEDIR) 
#CCOMPILER=g++ -march=k8-sse3 -mtune=k8-sse3 -msse -msse2 -msse3 -mssse3 -mfpmath=sse -O2 -I $(INCLUDEDIR)  
USERLIBS= -lhdf5 -lhdf5_hl -lnetcdf -lxml2 -lgd -lproj -lpq -ludunits -lfreetype -lgd -lpng -lrt -lpthread -lcurl #-lgdal 

# all libs together
LIBS = -L$(LIBDIR) $(USERLIBS) -L$(HOME2)/lib

OBJECTS = adagucserverEC.o CDataReader.o CImageWarper.o CDrawImage.o CServerError.o CRequest.o  CXMLGen.o CServerParams.o CGDALDataWriter.o CImageDataWriter.o CXMLSerializerInterface.o CDataSource.o CStopWatch.o CImgWarpBilinear.o CImgWarpBoolean.o CImgWarpNearestNeighbour.o CPGSQLDB.o

EXECUTABLE= adagucserverEC

default: $(EXECUTABLE)
$(EXECUTABLE): $(OBJECTS)
	g++ $(OBJECTS) $(HELPOBJECTS) $(CCDFDATAMODELOBJ) -o  $(EXECUTABLE) $(LIBS)

%.o: %.cpp
	$(CCOMPILER) -c $< -o $@

clean:
	rm $(OBJECTS) $(EXECUTABLE)
run:
	make
	./$(EXECUTABLE)		