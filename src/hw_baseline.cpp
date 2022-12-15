
#include "common.h"

char getCharacter(ap_uint_128 array[], int i)
{
#pragma HLS INLINE

  int idx1 = i / 16; // 16 chars in 128 bits
  int idx2 = i % 16;
  char c = (char)array[idx1]((idx2 + 1) * 8 - 1, idx2 * 8);
  return c;
}

void insertChar2Str(uint8_t arrayStr[], int &index, char val)
{
#pragma HLS INLINE

  arrayStr[index++] = val;
}

void insertInt2Str(uint8_t arrayStr[], int &index, int val)
{
#pragma HLS INLINE

  const int zero_ascii = 48;

  int num = val;

  // assume max five-digits for current design, upto 99999, works for long reads too

  int tt = num / 10000;
  int t = (num % 10000) / 1000;
  int h = (num % 1000) / 100;
  int d = (num % 100) / 10;
  int u = (num % 10) / 1;

  uint8_t tt_ = tt | zero_ascii;
  uint8_t t_ = t | zero_ascii;
  uint8_t h_ = h | zero_ascii;
  uint8_t d_ = d | zero_ascii;
  uint8_t u_ = u | zero_ascii;

  if (tt > 0)
  {
    arrayStr[index + 0] = tt_;
    arrayStr[index + 1] = t_;
    arrayStr[index + 2] = h_;
    arrayStr[index + 3] = d_;
    arrayStr[index + 4] = u_;
    index += 5;
  }
  else if (t > 0)
  {
    arrayStr[index + 0] = t_;
    arrayStr[index + 1] = h_;
    arrayStr[index + 2] = d_;
    arrayStr[index + 3] = u_;
    index += 4;
  }
  else if (h > 0)
  {
    arrayStr[index + 0] = h_;
    arrayStr[index + 1] = d_;
    arrayStr[index + 2] = u_;
    index += 3;
  }
  else if (d > 0)
  {
    arrayStr[index + 0] = d_;
    arrayStr[index + 1] = u_;
    index += 2;
  }
  else if (u > 0)
  {
    arrayStr[index + 0] = u_;
    index += 1;
  }
}

#ifdef __SDSCC__
#pragma SDS data zero_copy(shared_mem [0:TOTAL_SHARED_MEM_UINT128])
#endif

void genasm_HW_baseline(ap_uint_128 shared_mem[TOTAL_SHARED_MEM_UINT128])
{

#ifndef __SDSCC__ // hls equivalent of sds zero_copy
#pragma HLS INTERFACE m_axi port = shared_mem
#endif

#ifndef __SYNTHESIS__
  // printf("BASELINE HW\n");
#endif

  // ##############################################################
  // load params

  ap_uint_128 PARAMS[PARAM_WORDS_USED];

#define PARAM_WORDS_INPUT 3
LOOP_READ_PARAMS:
  for (int i = 0; i < PARAM_WORDS_INPUT; i++)
  {
    PARAMS[i] = shared_mem[PARAMS_OFFSET + i];
  }

  ap_uint_128 param_0 = PARAMS[0];
  int k = param_0(31, 0);
  int n = param_0(63, 32);
  int m = param_0(95, 64);
  int O = param_0(127, 96);
  int W = MAX_TEXT_SZ;

  // ##############################################################
  // local buffers

  ap_uint_128 text[MAX_TEXT_SZ / 16]; // 16 chars in 128 bits
  ap_uint_128 pattern[MAX_PATTERN_SZ / 16];

  uint64_t tracebackMatrix[MAX_TEXT_SZ][MAX_EDIT_THRESHOLD + 1][4];

  uint64_t max = ULLONG_MAX; // all ones

  // ##############################################################
  // copy input

LOOP_IN_COPY_TEXT:
  // for (int i = 0; i < n; i++)
  for (int i = 0; i < 4; i++)
    text[i] = shared_mem[TEXT_OFFSET + i];

LOOP_IN_COPY_PATTERN:
  // for (int i = 0; i < m; i++)
  for (int i = 0; i < 4; i++)
    pattern[i] = shared_mem[PATTERN_OFFSET + i];

  // ##############################################################
  // init pattern masks

  int len = 4; // A,C,G,T
  uint64_t patternBitmasks[len];

// Initialize the pattern bitmasks
LOOP_PBM_INIT:
  for (int i = 0; i < len; i++)
    patternBitmasks[i] = max;

// Update the pattern bitmasks
LOOP_PBM_UPDATE:
  for (int i = 0; i < m; i++)
  {
#pragma HLS LOOP_TRIPCOUNT max = 64

    char p = getCharacter(pattern, i);

    if ((p == 'A') || (p == 'a'))
    {
      patternBitmasks[0] &= ~(1ULL << ((m - i - 1)));
    }
    else if ((p == 'C') || (p == 'c'))
    {
      patternBitmasks[1] &= ~(1ULL << ((m - i - 1)));
    }
    else if ((p == 'G') || (p == 'g'))
    {
      patternBitmasks[2] &= ~(1ULL << ((m - i - 1)));
    }
    else if ((p == 'T') || (p == 't'))
    {
      patternBitmasks[3] &= ~(1ULL << ((m - i - 1)));
    }
  }

  // ##############################################################
  // main Bitap DC algo

  // Initialize the bit arrays R
  int len1 = (k + 1);
  uint64_t R[MAX_EDIT_THRESHOLD + 1];
  uint64_t oldR[MAX_EDIT_THRESHOLD + 1];
  uint64_t substitution, insertion, match, deletion, curBitmask;

LOOP_R_INIT:
  for (int i = 0; i < len1; i++)
  {
#pragma HLS LOOP_TRIPCOUNT max = 65
    R[i] = max;
  }

// now traverse the text in opposite direction (i.e., forward), generate partial tracebacks at each checkpoint
LOOP_BITAP_TEXT:
  for (int i = n - 1; i >= 0; i--)
  {
#pragma HLS LOOP_TRIPCOUNT max = 64

    char c = getCharacter(text, i);

    // if ((c == 'A') || (c == 'a') || (c == 'C') || (c == 'c') || (c == 'G') || (c == 'g') || (c == 'T') || (c == 't'))
    {
    // copy the content of R to oldR
    LOOP_OLD_COPY:
      for (int itR = 0; itR < len1; itR++)
      {
#pragma HLS LOOP_TRIPCOUNT max = 65
        oldR[itR] = R[itR];
      }

      if ((c == 'A') || (c == 'a'))
      {
        curBitmask = patternBitmasks[0];
      }
      else if ((c == 'C') || (c == 'c'))
      {
        curBitmask = patternBitmasks[1];
      }
      else if ((c == 'G') || (c == 'g'))
      {
        curBitmask = patternBitmasks[2];
      }
      else if ((c == 'T') || (c == 't'))
      {
        curBitmask = patternBitmasks[3];
      }

      // update R[0] by first shifting oldR[0] and then ORing with pattern bitmask
      R[0] = oldR[0] << 1;
      R[0] |= curBitmask;

      tracebackMatrix[i][0][0] = R[0];
      tracebackMatrix[i][0][1] = max;
      tracebackMatrix[i][0][2] = max;
      tracebackMatrix[i][0][3] = max;

    LOOP_EDIT_DIST:
      for (int d = 1; d <= k; d++)
      {
#pragma HLS LOOP_TRIPCOUNT max = 64
        int index = (d - 1);
        deletion = oldR[index];
        substitution = deletion << 1;
        insertion = R[index] << 1;
        index += 1;
        match = oldR[index] << 1;
        match |= curBitmask;
        R[index] = deletion & substitution & insertion & match;

        tracebackMatrix[i][d][0] = match;
        tracebackMatrix[i][d][1] = substitution;
        tracebackMatrix[i][d][2] = insertion;
        tracebackMatrix[i][d][3] = deletion;
      }
    }
  }

  // ##############################################################

  uint64_t max1 = 1ULL << (m - 1);
  int minErrorTemp = -1;
LOOP_MIN_ERROR:
  for (int t = 0; t <= k; t++)
  {
#pragma HLS LOOP_TRIPCOUNT max = 65
    if ((R[t] & max1) == 0)
    {
      minErrorTemp = t;
      break;
    }
  }

  // ##############################################################
  // traceback

  if (minErrorTemp == -1)
  {
    ap_uint_128 result;
    result(63, 63) = minErrorTemp;
    shared_mem[PARAMS_OFFSET + 1] = result;
    return;
  }

  ap_uint_128 param_1 = PARAMS[1];
  ap_uint_128 param_2 = PARAMS[2];
  ap_uint_128 param_3 = PARAMS[3];
  ap_uint_128 param_4 = PARAMS[4];

  int ed = 0; // = param_1(31, 0);
  bool isFirst = param_1(39, 32);
  int TB_DQ_mode = param_1(40, 40);

  int charCount = param_2(31, 0);
  int charCount2 = param_2(63, 32);
  int charCount3 = param_2(95, 64);
  char lastChar = param_2(103, 96);
  char lastChar2 = param_2(111, 104);
  char lastChar3 = param_2(119, 112);

  int countM = 0;
  int countS = 0;
  int countI = 0;
  int countD = 0;
  int countOpen = 0;
  int countExtend = 0;
  int textConsumed = 0;
  int patternConsumed = 0;

  int index_CIGARstr = 0;
  int index_CIGARstr2 = 0;
  int index_MD = 0;

  uint8_t array_CIGARstr[CIGAR_STR_SZ];
  uint8_t array_CIGARstr2[CIGAR_STR2_SZ];
  uint8_t array_MD[MD_STR_SZ];

  //////////////////////

  int curPattern = m - 1;
  int curText = 0;
  int curError = minErrorTemp;
  uint64_t mask = max1;

// simple vs dq mode conditions
#define COND1 ((curPattern >= 0) && (curError >= 0))
#define COND2 ((textConsumed < (W - O)) && (patternConsumed < (W - O)) && (curPattern >= 0) && (curError >= 0))

LOOP_TRACEBACK:
  while (TB_DQ_mode == 1 ? COND2 : COND1)
  {

#pragma HLS LOOP_TRIPCOUNT max = 128 // m+k upper-bound

    // affine-insertion
    if (lastChar == 'I' && ((tracebackMatrix[curText][curError][2] & mask) == 0))
    {
      curPattern -= 1;
      curError -= 1;
      mask = 1ULL << curPattern;
      if (lastChar == 'I')
      {
        charCount += 1;
        countExtend += 1;
      }
      else
      {
        if (!isFirst)
        {
          // sprintf(CIGARstr, "%s%d", CIGARstr, charCount);
          // sprintf(CIGARstr, "%s%c", CIGARstr, lastChar);
          insertInt2Str(array_CIGARstr, index_CIGARstr, charCount);
          insertChar2Str(array_CIGARstr, index_CIGARstr, lastChar);
        }
        charCount = 1;
        lastChar = 'I';
        countOpen += 1;
      }
      if (lastChar2 == 'I')
      {
        charCount2 += 1;
      }
      else
      {
        if (!isFirst)
        {
          // sprintf(CIGARstr2, "%s%d", CIGARstr2, charCount2);
          // sprintf(CIGARstr2, "%s%c", CIGARstr2, lastChar2);
          insertInt2Str(array_CIGARstr2, index_CIGARstr2, charCount2);
          insertChar2Str(array_CIGARstr2, index_CIGARstr2, lastChar2);
        }
        charCount2 = 1;
        lastChar2 = 'I';
      }
      countI += 1;
      ed += 1;
      patternConsumed += 1;
    }
    // affine-deletion
    else if (lastChar == 'D' && ((tracebackMatrix[curText][curError][3] & mask) == 0))
    {
      curText += 1;
      curError -= 1;
      if (lastChar == 'D')
      {
        charCount += 1;
        countExtend += 1;
      }
      else
      {
        if (!isFirst)
        {
          // sprintf(CIGARstr, "%s%d", CIGARstr, charCount);
          // sprintf(CIGARstr, "%s%c", CIGARstr, lastChar);
          insertInt2Str(array_CIGARstr, index_CIGARstr, charCount);
          insertChar2Str(array_CIGARstr, index_CIGARstr, lastChar);
        }
        charCount = 1;
        lastChar = 'D';
        countOpen += 1;
      }
      if (lastChar2 == 'D')
      {
        charCount2 += 1;
      }
      else
      {
        if (!isFirst)
        {
          // sprintf(CIGARstr2, "%s%d", CIGARstr2, charCount2);
          // sprintf(CIGARstr2, "%s%c", CIGARstr2, lastChar2);
          insertInt2Str(array_CIGARstr2, index_CIGARstr2, charCount2);
          insertChar2Str(array_CIGARstr2, index_CIGARstr2, lastChar2);
        }
        charCount2 = 1;
        lastChar2 = 'D';
      }
      if (lastChar3 == 'M')
      {
        // sprintf(MD, "%s%d^%c", MD, charCount3, getCharacter(text, curText - 1));
        insertInt2Str(array_MD, index_MD, charCount3);
        insertChar2Str(array_MD, index_MD, '^');
        insertChar2Str(array_MD, index_MD, getCharacter(text, curText - 1));

        lastChar3 = 'D';
        charCount3 = 0;
      }
      else if (lastChar3 == 'D')
      {
        // sprintf(MD, "%s%c", MD, getCharacter(text, curText - 1));
        insertChar2Str(array_MD, index_MD, getCharacter(text, curText - 1));
        lastChar3 = 'D';
        charCount3 = 0;
      }
      else
      {
        // sprintf(MD, "%s^%c", MD, getCharacter(text, curText - 1));
        insertChar2Str(array_MD, index_MD, '^');
        insertChar2Str(array_MD, index_MD, getCharacter(text, curText - 1));
        lastChar3 = 'D';
        charCount3 = 0;
      }
      countD += 1;
      ed += 1;
      textConsumed += 1;
    }
    // match
    else if ((tracebackMatrix[curText][curError][0] & mask) == 0)
    {
      curText += 1;
      curPattern -= 1;
      mask = 1ULL << curPattern;
      if (lastChar == 'M')
      {
        charCount += 1;
      }
      else
      {
        if (!isFirst)
        {
          // sprintf(CIGARstr, "%s%d", CIGARstr, charCount);
          // sprintf(CIGARstr, "%s%c", CIGARstr, lastChar);
          insertInt2Str(array_CIGARstr, index_CIGARstr, charCount);
          insertChar2Str(array_CIGARstr, index_CIGARstr, lastChar);
        }
        charCount = 1;
        lastChar = 'M';
      }
      if (lastChar2 == 'M')
      {
        charCount2 += 1;
      }
      else
      {
        if (!isFirst)
        {
          // sprintf(CIGARstr2, "%s%d", CIGARstr2, charCount2);
          // sprintf(CIGARstr2, "%s%c", CIGARstr2, lastChar2);
          insertInt2Str(array_CIGARstr2, index_CIGARstr2, charCount2);
          insertChar2Str(array_CIGARstr2, index_CIGARstr2, lastChar2);
        }
        charCount2 = 1;
        lastChar2 = 'M';
      }
      if (lastChar3 == 'M')
      {
        charCount3 += 1;
      }
      else
      {
        charCount3 = 1;
        lastChar3 = 'M';
      }
      countM += 1;
      textConsumed += 1;
      patternConsumed += 1;
    }
    // substitution
    else if ((tracebackMatrix[curText][curError][1] & mask) == 0)
    {
      curText += 1;
      curPattern -= 1;
      curError -= 1;
      mask = 1ULL << curPattern;
      if (lastChar == 'S')
      {
        charCount += 1;
      }
      else
      {
        if (!isFirst)
        {
          // sprintf(CIGARstr, "%s%d", CIGARstr, charCount);
          // sprintf(CIGARstr, "%s%c", CIGARstr, lastChar);
          insertInt2Str(array_CIGARstr, index_CIGARstr, charCount);
          insertChar2Str(array_CIGARstr, index_CIGARstr, lastChar);
        }
        charCount = 1;
        lastChar = 'S';
      }
      if (lastChar2 == 'M')
      {
        charCount2 += 1;
      }
      else
      {
        if (!isFirst)
        {
          // sprintf(CIGARstr2, "%s%d", CIGARstr2, charCount2);
          // sprintf(CIGARstr2, "%s%c", CIGARstr2, lastChar2);
          insertInt2Str(array_CIGARstr2, index_CIGARstr2, charCount2);
          insertChar2Str(array_CIGARstr2, index_CIGARstr2, lastChar2);
        }
        charCount2 = 1;
        lastChar2 = 'M';
      }
      if (lastChar3 == 'M')
      {
        // sprintf(MD, "%s%d%c", MD, charCount3, getCharacter(text, curText - 1));
        insertInt2Str(array_MD, index_MD, charCount3);
        insertChar2Str(array_MD, index_MD, getCharacter(text, curText - 1));
        lastChar3 = 'S';
        charCount3 = 0;
      }
      else
      {
        // sprintf(MD, "%s%c", MD, getCharacter(text, curText - 1));
        insertChar2Str(array_MD, index_MD, getCharacter(text, curText - 1));
        lastChar3 = 'S';
        charCount3 = 0;
      }
      countS += 1;
      ed += 1;
      textConsumed += 1;
      patternConsumed += 1;
    }
    // deletion
    else if ((tracebackMatrix[curText][curError][3] & mask) == 0)
    {
      curText += 1;
      curError -= 1;
      if (lastChar == 'D')
      {
        charCount += 1;
        countExtend += 1;
      }
      else
      {
        if (!isFirst)
        {
          // sprintf(CIGARstr, "%s%d", CIGARstr, charCount);
          // sprintf(CIGARstr, "%s%c", CIGARstr, lastChar);
          insertInt2Str(array_CIGARstr, index_CIGARstr, charCount);
          insertChar2Str(array_CIGARstr, index_CIGARstr, lastChar);
        }
        charCount = 1;
        lastChar = 'D';
        countOpen += 1;
      }
      if (lastChar2 == 'D')
      {
        charCount2 += 1;
      }
      else
      {
        if (!isFirst)
        {
          // sprintf(CIGARstr2, "%s%d", CIGARstr2, charCount2);
          // sprintf(CIGARstr2, "%s%c", CIGARstr2, lastChar2);
          insertInt2Str(array_CIGARstr2, index_CIGARstr2, charCount2);
          insertChar2Str(array_CIGARstr2, index_CIGARstr2, lastChar2);
        }
        charCount2 = 1;
        lastChar2 = 'D';
      }
      if (lastChar3 == 'M')
      {
        // sprintf(MD, "%s%d^%c", MD, charCount3, getCharacter(text, curText - 1));
        insertInt2Str(array_MD, index_MD, charCount3);
        insertChar2Str(array_MD, index_MD, '^');
        insertChar2Str(array_MD, index_MD, getCharacter(text, curText - 1));
        lastChar3 = 'D';
        charCount3 = 0;
      }
      else if (lastChar3 == 'D')
      {
        // sprintf(MD, "%s%c", MD, getCharacter(text, curText - 1));
        insertChar2Str(array_MD, index_MD, getCharacter(text, curText - 1));
        lastChar3 = 'D';
        charCount3 = 0;
      }
      else
      {
        // sprintf(MD, "%s^%c", MD, getCharacter(text, curText - 1));
        insertChar2Str(array_MD, index_MD, '^');
        insertChar2Str(array_MD, index_MD, getCharacter(text, curText - 1));
        lastChar3 = 'D';
        charCount3 = 0;
      }
      countD += 1;
      ed += 1;
      textConsumed += 1;
    }
    // insertion
    else if ((tracebackMatrix[curText][curError][2] & mask) == 0)
    {
      curPattern -= 1;
      curError -= 1;
      mask = 1ULL << curPattern;
      if (lastChar == 'I')
      {
        charCount += 1;
        countExtend += 1;
      }
      else
      {
        if (!isFirst)
        {
          // sprintf(CIGARstr, "%s%d", CIGARstr, charCount);
          // sprintf(CIGARstr, "%s%c", CIGARstr, lastChar);
          insertInt2Str(array_CIGARstr, index_CIGARstr, charCount);
          insertChar2Str(array_CIGARstr, index_CIGARstr, lastChar);
        }
        charCount = 1;
        lastChar = 'I';
        countOpen += 1;
      }
      if (lastChar2 == 'I')
      {
        charCount2 += 1;
      }
      else
      {
        if (!isFirst)
        {
          // sprintf(CIGARstr2, "%s%d", CIGARstr2, charCount2);
          // sprintf(CIGARstr2, "%s%c", CIGARstr2, lastChar2);
          insertInt2Str(array_CIGARstr2, index_CIGARstr2, charCount2);
          insertChar2Str(array_CIGARstr2, index_CIGARstr2, lastChar2);
        }
        charCount2 = 1;
        lastChar2 = 'I';
      }
      countI += 1;
      ed += 1;
      patternConsumed += 1;
    }

    isFirst = false;
  }

  // ##############################################################
  // write output of params and traceback here

  param_1(31, 0) = ed;
  param_1(39, 32) = isFirst;
  param_1(63, 63) = minErrorTemp;
  param_1(79, 64) = index_CIGARstr;
  param_1(95, 80) = index_CIGARstr2;
  param_1(111, 96) = index_MD;

  int combined_lastChar = lastChar3 << 16 | lastChar2 << 8 | lastChar;
  param_2(31, 0) = charCount;
  param_2(63, 32) = charCount2;
  param_2(95, 64) = charCount3;
  param_2(127, 96) = combined_lastChar;

  param_3(31, 0) = countM;
  param_3(63, 32) = countS;
  param_3(95, 64) = countI;
  param_3(127, 96) = countD;
  param_4(31, 0) = countOpen;
  param_4(63, 32) = countExtend;
  param_4(95, 64) = textConsumed;
  param_4(127, 96) = patternConsumed;

  // burst
  // shared_mem[PARAMS_OFFSET + 0] = param_0;
  shared_mem[PARAMS_OFFSET + 1] = param_1;
  shared_mem[PARAMS_OFFSET + 2] = param_2;
  shared_mem[PARAMS_OFFSET + 3] = param_3;
  shared_mem[PARAMS_OFFSET + 4] = param_4;

  // ################

  ap_uint_128 output_strings[3][CIGAR_STR_SZ / 16]; // 16 chars in 128 bits

LOOP_OUTSTR_CONCAT:
  for (size_t i = 0; i < CIGAR_STR_SZ / 16; i++)
  {
    ap_uint_128 str_concat1, str_concat2, str_concat3;

  LOOP_CONCAT:
    for (size_t idx = 0; idx < 16; idx++)
    {
      str_concat1(8 * (idx + 1) - 1, 8 * idx) = array_CIGARstr[i * 16 + idx];
      str_concat2(8 * (idx + 1) - 1, 8 * idx) = array_CIGARstr2[i * 16 + idx];
      str_concat3(8 * (idx + 1) - 1, 8 * idx) = array_MD[i * 16 + idx];
    }
    output_strings[0][i] = str_concat1;
    output_strings[1][i] = str_concat2;
    output_strings[2][i] = str_concat3;
  }

LOOP_OUTSTR_WRITE_TYPE:
  for (size_t j = 0; j < 3; j++)
  {
  LOOP_OUTSTR_WRITE_WORD:
    for (size_t i = 0; i < CIGAR_STR_SZ / 16; i++)
    {
      shared_mem[CIGAR_STR_OFFSET + j * (CIGAR_STR_SZ / 16) + i] = output_strings[j][i]; // one base offset for all strings
    }
  }
}
