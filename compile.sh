#!/bin/bash

#export ADAGUCCOMPILERSETTINGS="-Wall"
#export ADAGUCCOMPILERSETTINGS="-msse -msse2 -msse3 -mssse3 -mfpmath=sse -O2"

DEFAULTCOMPILERSETTINGS="-msse -msse2 -msse3 -mssse3 -mfpmath=sse -O2"

if [ -z ${ADAGUCCOMPILERSETTINGS+x} ]; then 
  echo "ADAGUCCOMPILERSETTINGS is unset";
  export ADAGUCCOMPILERSETTINGS=$DEFAULTCOMPILERSETTINGS
else 
  echo "ADAGUCCOMPILERSETTINGS is set to '$ADAGUCCOMPILERSETTINGS'"; 
fi
  


function quit {
  echo "Make sure include directories are indicated with CPPFLAGS and library directories with LDFLAGS"
  echo "  For example:"
  echo "  export CPPFLAGS=-I/home/user/software/install/include -I/home/user/othersoftware/install/include"
  echo "  export LDFLAGS=-L/home/user/software/install/lib/ -L/home/user/othersoftware/install/lib/" 
  echo ""
  exit ;
}

cd hclasses
rm *.o
rm *.a
make -j4

if [ -f hclasses.a ]
  then
  echo "[OK] hclasses have succesfully been compiled."
  else
    echo "[FAILED] hclasses compilation failed"
    quit;
  fi

cd ../CCDFDataModel
rm *.o
rm *.a
make -j4


if [ -f CCDFDataModel.a ]
  then
  echo "[OK] CCDFDataModel has been succesfully compiled."
  else
    echo "[FAILED] CCDFDataModel compilation failed"
    quit;
  fi
  
cd ../adagucserverEC
rm *.o
rm adagucserver
rm h5ncdump
make -j4


if [ -f adagucserver ]
  then
  echo "[OK] ADAGUC has been succesfully compiled."
   else
     echo "[FAILED] ADAGUC compilation failed"
     quit;
fi


test -d ../bin || mkdir ../bin/
cp adagucserver ../bin/
cp h5ncdump ../bin/
cp aggregate_time ../bin/
echo "[OK] Everything is installed in the ./bin directory"