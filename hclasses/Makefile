HOME=/nobackup/users/plieger/software/install

INCLUDEDIR=$(HOME)/include  

CCOMPILER=g++ -Wall -I $(INCLUDEDIR) 
#optimalization options:
#CCOMPILER=g++ -march=pentium4 -mtune=pentium4 -msse -msse2 -msse3 -mssse3 -mfpmath=sse -O2 -I $(INCLUDEDIR) 
CCOMPILER=g++ -march=core2 -mtune=core2 -msse -msse2 -msse3 -mssse3 -mfpmath=sse -O2 -I $(INCLUDEDIR) 
#CCOMPILER=g++ -O2 -I $(INCLUDEDIR)
#CCOMPILER=g++ -march=k8-sse3 -mtune=k8-sse3 -msse -msse2 -msse3 -mssse3 -mfpmath=sse -O2 -I $(INCLUDEDIR)  


OBJECTS = CDirReader.o  CTypes.o CDebugger.o 

EXECUTABLE= hclasses

default: $(EXECUTABLE)
$(EXECUTABLE): $(OBJECTS)
	ar rcs $(EXECUTABLE).a $(OBJECTS)  

%.o: %.cpp
	$(CCOMPILER) -c $< -o $@

clean:
	rm $(OBJECTS) $(EXECUTABLE).a


