
#include "common.h"

// this wrapper for convenience in building SDSoC HW as a library, since HW function needs a caller function

void genasm_HW(ap_uint_128 shared_mem[TOTAL_SHARED_MEM_UINT128])
{
#ifdef __BASELINE
    genasm_HW_baseline(shared_mem);
#else
    genasm_HW_optimized(shared_mem);
#endif
}
