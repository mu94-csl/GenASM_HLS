.PHONY: all

# Provide Correct SDx Path
SDx_BUILD_PATH := $(dir $(shell which sdx))/..
ARM_INC := $(SDx_BUILD_PATH)/target/aarch64-linux/include
VIV_PATH := $(dir $(shell which vivado))/..
SDX_INC := $(VIV_PATH)/include
SDSLIB_DIR := $(SDx_BUILD_PATH)/target/aarch64-linux/lib/

ARM_CXX = aarch64-linux-gnu-g++

LIB_DIR := ../hw_build_baseline/ 
HW_DIR := ../hw/

IDIRS = -I$(ARM_INC) -I$(SDX_INC) -I$(HW_DIR)  

LDIRS = -L$(LIB_DIR)  -L$(SDSLIB_DIR)
LLIBS = -lpthread -lgenasmHW -lsds_lib

# CFLAGS = -std=c++0x -Wno-Wwrite-strings								
CFLAGS_ARM = -std=c++11   -Wno-write-strings 			-I../src/s2p/		-D__SDSCC__=1	-D__APIMODE=1  
 
all: host.elf  

host.elf : ../src/testbench.cpp	 ../src/sw.cpp 	../src/s2p/*.cc
	@echo -e "\n## Compiling SW \n"
	$(ARM_CXX) $(CFLAGS_ARM) $(IDIRS) $(LDIRS) $(LLIBS)  $^ -o $@   -O3
	@echo

clean :
	rm -rf *.elf

