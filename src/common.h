#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <math.h>
#include <inttypes.h>
#include <stdbool.h>
#include <ctype.h>
#include <assert.h>
#include <fstream>
#include <iostream>
#include <iomanip>

using namespace std;

#include <ap_int.h>
#include "s2p/sam2pairwise.hh"

#ifdef __SDSCC__
#include "sds_lib.h"
#else
#define sds_alloc(x) (malloc(x))
#define sds_alloc_non_cacheable(x) (malloc(x))
#define sds_free(x) (free(x))
#endif

// 8-bit
#define MAX_EDIT_THRESHOLD 64
#define MAX_TEXT_SZ 64
#define MAX_PATTERN_SZ 64

// 64-bit words here
#define TBM_DIMSZ_1 (MAX_EDIT_THRESHOLD + 1) * 4
#define TBM_DIMSZ_2 4
#define TBM_SZ (MAX_TEXT_SZ * TBM_DIMSZ_1)

// 128-bit
#define PARAM_WORDS 8
#define PARAM_WORDS_USED 5 // rest unused

// 8-bit, ASCII char strings
#define STR_SZ_MULT 3 // large enough perhaps for given HW params...
#define CIGAR_STR_SZ (MAX_PATTERN_SZ * STR_SZ_MULT)
#define CIGAR_STR2_SZ (MAX_PATTERN_SZ * STR_SZ_MULT)
#define MD_STR_SZ (MAX_PATTERN_SZ * STR_SZ_MULT)
#define TOTAL_OUT_STR_SZ (CIGAR_STR_SZ + CIGAR_STR2_SZ + MD_STR_SZ)

// ###########
// 128-bit words
#define PARAMS_OFFSET 0
#define TEXT_OFFSET (PARAMS_OFFSET + PARAM_WORDS)
#define PATTERN_OFFSET (TEXT_OFFSET + MAX_TEXT_SZ / 16)
#define CIGAR_STR_OFFSET (PATTERN_OFFSET + MAX_PATTERN_SZ / 16)
#define CIGAR_STR2_OFFSET (CIGAR_STR_OFFSET + CIGAR_STR_SZ / 16)
#define MD_STR_OFFSET (CIGAR_STR2_OFFSET + CIGAR_STR2_SZ / 16)

#define TOTAL_SHARED_MEM_UINT128 (MD_STR_OFFSET + MD_STR_SZ / 16)
/*
Shared Memory Layout (b/w host and HW accel)
Each word is 128-bit.
Word 0 - k m n O, each of 32 bits
Word 1 - ed, isFirst, DQ_mode, minError, indexStrs
Word 2 - charCounts, lastChars
Word 3 - countM/S/I/D
Word 4 - countOpen/Extend, text/patternConsumed
Word 5 6 7 - reserved, not used
Next multiple words: input text and pattern strings, then output CIGAR/MD strings
*/

#define STRING_SIZE_SMALL 250
#define STRING_SIZE_LARGE 100000

typedef ap_uint<64> ap_uint_64;
typedef ap_uint<128> ap_uint_128;

void genasm_HW(ap_uint_128 shared_mem[TOTAL_SHARED_MEM_UINT128]);

#ifdef __BASELINE
void genasm_HW_baseline(ap_uint_128 shared_mem[TOTAL_SHARED_MEM_UINT128]);
#else
void genasm_HW_optimized(ap_uint_128 shared_mem[TOTAL_SHARED_MEM_UINT128]);
#endif

void genasmDC_SW(char *text, char *pattern, int k, int n, int m, int *minError, uint64_t *mask, uint64_t *tracebackMatrix);

void genasmTB_wrapper(char *text, char *pattern, int n, int m, int k, int scoreM, int scoreS, int scoreOpen, int scoreExtend, int minError, uint64_t mask, uint64_t *tracebackMatrix);

void genasmTB(int n, int k, uint64_t *tracebackMatrix, int m, int minError, int *ed, uint64_t mask,
              char *lastChar, char *lastChar2, char *lastChar3, int *charCount, int *charCount2, int *charCount3,
              char *CIGARstr, char *CIGARstr2, char *MD, char *text,
              int *countM, int *countS, int *countD, int *countI, int *countOpen, int *countExtend,
              bool *isFirst, int scoreS, int scoreOpen, int scoreExtend);

void genasmTB_DQ(int W, int O, int k, uint64_t *tracebackMatrix, int m, int minError, int *ed, uint64_t mask,
                 int *textConsumed, int *patternConsumed,
                 char *lastChar, char *lastChar2, char *lastChar3, int *charCount, int *charCount2, int *charCount3,
                 char *CIGARstr, char *CIGARstr2, char *MD, char *text,
                 int *countM, int *countS, int *countD, int *countI, int *countOpen, int *countExtend,
                 bool *isFirst, int scoreS, int scoreOpen, int scoreExtend);

#endif
