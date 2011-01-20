################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
O_SRCS += \
../src/CCDFDataModel.o \
../src/CCDFNetCDFIO.o \
../src/CDataReader.o \
../src/CDataSource.o \
../src/CDrawImage.o \
../src/CGDALDataWriter.o \
../src/CImageDataWriter.o \
../src/CImageWarper.o \
../src/CRequest.o \
../src/CServerError.o \
../src/CServerParams.o \
../src/CStopWatch.o \
../src/CXMLGen.o \
../src/CXMLSerializerInterface.o \
../src/adagucserverEC.o 

CPP_SRCS += \
../src/CCDFDataModel.cpp \
../src/CCDFNetCDFIO.cpp \
../src/CDataReader.cpp \
../src/CDataSource.cpp \
../src/CDrawImage.cpp \
../src/CGDALDataWriter.cpp \
../src/CImageDataWriter.cpp \
../src/CImageWarper.cpp \
../src/CRequest.cpp \
../src/CServerError.cpp \
../src/CServerParams.cpp \
../src/CStopWatch.cpp \
../src/CXMLGen.cpp \
../src/CXMLSerializerInterface.cpp \
../src/adagucserverEC.cpp 

OBJS += \
./src/CCDFDataModel.o \
./src/CCDFNetCDFIO.o \
./src/CDataReader.o \
./src/CDataSource.o \
./src/CDrawImage.o \
./src/CGDALDataWriter.o \
./src/CImageDataWriter.o \
./src/CImageWarper.o \
./src/CRequest.o \
./src/CServerError.o \
./src/CServerParams.o \
./src/CStopWatch.o \
./src/CXMLGen.o \
./src/CXMLSerializerInterface.o \
./src/adagucserverEC.o 

CPP_DEPS += \
./src/CCDFDataModel.d \
./src/CCDFNetCDFIO.d \
./src/CDataReader.d \
./src/CDataSource.d \
./src/CDrawImage.d \
./src/CGDALDataWriter.d \
./src/CImageDataWriter.d \
./src/CImageWarper.d \
./src/CRequest.d \
./src/CServerError.d \
./src/CServerParams.d \
./src/CStopWatch.d \
./src/CXMLGen.d \
./src/CXMLSerializerInterface.d \
./src/adagucserverEC.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I"/nobackup/users/plieger/eclipseworkspace/cpp/hclasses" -I/nobackup/users/plieger/hdf5mapserver/build/include -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


