#==========================================================================
# Makefile
#==========================================================================

VHLS_PATH := $(dir $(shell which vivado_hls))/..
VHLS_INC ?= ${VHLS_PATH}/include

CFLAGS = -O3 -I${VHLS_INC} -DHLS_NO_XIL_FPO_LIB 		-Wno-write-strings  -I./s2p/				

default:
	@echo Running make without an option

csim_baseline: testbench.cpp hw_wrapper.cpp hw_baseline.cpp sw.cpp	s2p/*.cc
	@echo "Compiling & simulating ..."
	g++ ${CFLAGS} $^ -o testbench.exe -lrt			-D__BASELINE=1			
	./testbench.exe 0

csim_optimized: testbench.cpp hw_wrapper.cpp hw_optimized.cpp sw.cpp	s2p/*.cc
	@echo "Compiling & simulating ..."
	g++ ${CFLAGS} $^ -o testbench.exe -lrt		  
	./testbench.exe 0

vivado_baseline:
	@echo "Run Vivado HLS"
	faketime -f "-3y" 	vivado_hls -f build-baseline.tcl

vivado_optimized:
	@echo "Run Vivado HLS"
	faketime -f "-3y" 	vivado_hls -f build-optimized.tcl

clean:
	rm -rf genasm_hls_prj* *.dat *.prj *.log *.exe

