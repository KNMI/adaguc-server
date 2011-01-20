################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
O_SRCS += \
../CADAGUCNCMLWriter.o \
../CADAGUC_R_API.o \
../CADAGUC_V_API.o \
../CADAGUC_time.o \
../CDebugger.o \
../CDirReader.o \
../CGetCMDOption.o \
../CPGSQLDB.o \
../CTypes.o 

CPP_SRCS += \
../CADAGUCNCMLWriter.cpp \
../CADAGUC_R_API.cpp \
../CADAGUC_V_API.cpp \
../CADAGUC_time.cpp \
../CDebugger.cpp \
../CDirReader.cpp \
../CGetCMDOption.cpp \
../CPGSQLDB.cpp \
../CTypes.cpp 

OBJS += \
./CADAGUCNCMLWriter.o \
./CADAGUC_R_API.o \
./CADAGUC_V_API.o \
./CADAGUC_time.o \
./CDebugger.o \
./CDirReader.o \
./CGetCMDOption.o \
./CPGSQLDB.o \
./CTypes.o 

CPP_DEPS += \
./CADAGUCNCMLWriter.d \
./CADAGUC_R_API.d \
./CADAGUC_V_API.d \
./CADAGUC_time.d \
./CDebugger.d \
./CDirReader.d \
./CGetCMDOption.d \
./CPGSQLDB.d \
./CTypes.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I/nobackup/users/plieger/hdf5mapserver/build/include -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


