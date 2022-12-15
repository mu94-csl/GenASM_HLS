#### BUILD HW

# Main Variables
LIBRARY     := libgenasmHW.so
PLATFORM    := zcu104

PROJ_PATH   := ../
SRC_PATH    := ${PROJ_PATH}/src/

VERBOSE  := -verbose

CC       := faketime -f "-3y"	 sds++   -sds-pf ${PLATFORM}   -sds-sys-config a53_linux   -sds-proc a53_linux     ${VERBOSE}

IDIRS    := -I${SRC_PATH}

CFLAGS   := -Wall -O3 -c ${IDIRS}  -Wno-unused-label 		-dmclkid  3

# CFLAGS   +=  -D__BASELINE=1 		  

REQ_LIBS := hw_wrapper.o genasmHW.o


.PHONY: all
all: ${LIBRARY}

genasmHW.o: ${SRC_PATH}/hw_optimized.cpp
	${CC}  ${CFLAGS}  -sds-hw 	genasm_HW_optimized 	${SRC_PATH}/hw_optimized.cpp   -clkid 1  -sds-end -fPIC $< -o $@

hw_wrapper.o: ${SRC_PATH}/hw_wrapper.cpp
	${CC}  ${CFLAGS}  -fPIC $< -o $@

${LIBRARY}: ${REQ_LIBS}
	${CC}   -shared $^ -o $@

clean:
	${RM} ${LIBRARY}  ${REQ_LIBS} *.o *.so
	${RM} ${LIBRARY}.bit
	${RM} -r _sds sd_card .Xil

