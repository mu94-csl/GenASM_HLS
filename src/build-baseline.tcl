#=============================================================================
# 
#=============================================================================

# Project name
set hls_prj genasm_hls_prj_baseline

set CFLAGS  "-I./s2p/"

# Open/reset the project
open_project ${hls_prj} -reset

# Top function of the design 
set_top genasm_HW_baseline

# Add design and testbench files
add_files hw_baseline.cpp       -cflags ${CFLAGS}
add_files sw.cpp                -cflags ${CFLAGS}

add_files -tb tb.cpp            -cflags ${CFLAGS}


open_solution "solution1"
# Use Zcu104
set_part {xczu7ev-ffvc1156-2-e}    

# Target clock period in ns
create_clock -period 10

### You can insert your own directives here ###
############################################

# Simulate the C++ design
# csim_design -O

# Synthesize the design
csynth_design

# Co-simulate the design
# cosim_design

# Export (and impl)
# export_design -flow syn -rtl verilog -format ip_catalog

exit
