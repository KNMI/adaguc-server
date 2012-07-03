#!/bin/bash

function quit {
  echo "Make sure include directories are indicated with CPPFLAGS and library directories with LDFLAGS"
  echo "  For example:"
  echo "  export CPPFLAGS=-I/home/user/software/install/include -I/home/user/othersoftware/install/include"
  echo "  export LDFLAGS=-L/home/user/software/install/lib/ -L/home/user/othersoftware/install/lib/" 
  echo ""
  exit ;
}

cd hclasses
make

if [ -f hclasses.a ]
  then
  echo "[OK] hclasses have succesfully been compiled."
  else
    echo "[FAILED] hclasses compilation failed"
    quit;
  fi

cd ../CCDFDataModel
make


if [ -f CCDFDataModel.a ]
  then
  echo "[OK] CCDFDataModel has been succesfully compiled."
  else
    echo "[FAILED] CCDFDataModel compilation failed"
    quit;
  fi
  
cd ../adagucserverEC
make


if [ -f adagucserver ]
  then
  echo "[OK] ADAGUC has been succesfully compiled."
   else
     echo "[FAILED] ADAGUC compilation failed"
     quit;
fi


test -f ../bin && mkdir ../bin/
cp adagucserver ../bin/
cp h5ncdump ../bin/
echo "[OK] Everything is installed in the ./bin directory"