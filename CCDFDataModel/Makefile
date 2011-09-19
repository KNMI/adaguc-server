HOME=/nobackup/users/plieger/software/install
#HOME=/home/plieger/software/install
#HOME=/data/data1/software/install
HOME=/data/data1/mapserver
INCLUDEDIR=$(HOME)/include -I ../hclasses  -I /usr/include/libxml2/

CCOMPILER=g++ -Wall -I $(INCLUDEDIR) 
#optimalization options:
#CCOMPILER=g++ -march=pentium4 -mtune=pentium4 -msse -msse2 -msse3 -mssse3 -mfpmath=sse -O2 -I $(INCLUDEDIR) 
#CCOMPILER=g++ -march=core2 -mtune=core2 -msse -msse2 -msse3 -mssse3 -mfpmath=sse -O2 -I $(INCLUDEDIR) 
CCOMPILER=g++ -O2 -I $(INCLUDEDIR)
#CCOMPILER=g++ -march=k8-sse3 -mtune=k8-sse3 -msse -msse2 -msse3 -mssse3 -mfpmath=sse -O2 -I $(INCLUDEDIR)  


OBJECTS = CCDFDataModel.o  CCDFNetCDFIO.o CADAGUC_time.o CCDFHDF5IO.o 
#OBJECTS = CCDFDataModel.o  CCDFNetCDFIO.o 


EXECUTABLE= CCDFDataModel

default: $(EXECUTABLE)
$(EXECUTABLE): $(OBJECTS)
	ar rcs  $(EXECUTABLE).a $(OBJECTS) 

%.o: %.cpp
	$(CCOMPILER) -c $< -o $@

clean:
	rm $(OBJECTS)  $(EXECUTABLE).a


