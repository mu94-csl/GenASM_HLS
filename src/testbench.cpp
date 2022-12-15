

#include "common.h"

// ##############################
// only enable one timing method!
// #define __SDSCC__TIME__
#define __CPP__TIME__
// ##############################

#ifdef __SDSCC__TIME__
#undef __CPP__TIME__
#endif

#ifndef __SDSCC__ // defined by sds compiler
#undef __SDSCC__TIME__
#endif

#ifdef __SDSCC__TIME__
class sds_perf_counter
{
private:
    uint64_t tot, cnt, calls;

public:
    sds_perf_counter() : tot(0), cnt(0), calls(0)
    {
        // printf("\n SDS counter freq: %llu ticks/sec \n", sds_clock_frequency());
        ;
    };
    inline void reset() { tot = cnt = calls = 0; }
    inline void start()
    {
        cnt = sds_clock_counter();
        calls++;
    };
    inline void stop() { tot += (sds_clock_counter() - cnt); };
    inline uint64_t cpu_cycles() { return (tot); };
    inline uint64_t avg_cpu_cycles() { return (tot / calls); };
};

#endif

#if defined __SDSCC__TIME__ || defined __CPP__TIME__
uint64_t time_total_SW = 0;
uint64_t time_total_HW = 0;
#endif

void report_time()
{
#if defined __SDSCC__TIME__ || defined __CPP__TIME__
    printf("\n\n\nTotal cycles/time units -- HW: %llu -- SW: %llu \n", time_total_HW, time_total_SW);
    std::cout << "\nSpeedup of HW over SW:  " << (double)time_total_SW / (double)time_total_HW << "\n\n\n";
    time_total_HW = time_total_SW = 0; // reset
#endif
}

enum ExecMode
{
    SW_MODE,
    HW_MODE
};
ExecMode globalExecMode;

#if !defined __SDSCC__TIME__ && !defined __CPP__TIME__
// default does nothing
#define __TIME_STAMP_START__
#define __TIME_STAMP_END__
#endif

#ifdef __CPP__TIME__
struct timespec start_clk, end_clk;
#define __TIME_STAMP_START__                            \
    {                                                   \
        clock_gettime(CLOCK_MONOTONIC_RAW, &start_clk); \
    }
#define __TIME_STAMP_END__                                                                                                \
    {                                                                                                                     \
        clock_gettime(CLOCK_MONOTONIC_RAW, &end_clk);                                                                     \
        uint64_t delta_us = (end_clk.tv_sec - start_clk.tv_sec) * 1000000 + (end_clk.tv_nsec - start_clk.tv_nsec) / 1000; \
        if (globalExecMode == SW_MODE)                                                                                    \
            time_total_SW += delta_us;                                                                                    \
        else                                                                                                              \
            time_total_HW += delta_us;                                                                                    \
    }
// printf("Time spent: %llu ms\n", delta_us);
#endif

#ifdef __SDSCC__TIME__
sds_perf_counter hw_ctr;
#define __TIME_STAMP_START__ \
    {                        \
        hw_ctr.reset();      \
        hw_ctr.start();      \
    }
#define __TIME_STAMP_END__                        \
    {                                             \
        hw_ctr.stop();                            \
        uint64_t hw_cycles = hw_ctr.cpu_cycles(); \
        if (globalExecMode == SW_MODE)            \
            time_total_SW += hw_cycles;           \
        else                                      \
            time_total_HW += hw_cycles;           \
    }
// std::cout << "CPU cycles: " << hw_cycles << std::endl;
#endif

// assert(m <= MAX_PATTERN_SZ);
// assert(n <= MAX_TEXT_SZ);
// assert(k <= MAX_EDIT_THRESHOLD);

// ##############################
// ##############################
// ##############################
// ##############################

int scoreM = 1;
int scoreS = 4;
int scoreOpen = 6;
int scoreExtend = 1;

int W = MAX_TEXT_SZ; // window
int O = 24;          // overlap

uint64_t *tracebackMatrix_SW;

ap_uint_128 *shared_mem; // shared dram buffer between cpu/fpga

void init()
{
    assert(MAX_PATTERN_SZ <= 64); // cannot be greater than cpu word in current impl

    tracebackMatrix_SW = (uint64_t *)malloc(TBM_SZ * sizeof(uint64_t));
#if 1
    shared_mem = (ap_uint_128 *)sds_alloc_non_cacheable(TOTAL_SHARED_MEM_UINT128 * sizeof(ap_uint_128)); // performs better?
#else
    shared_mem = (ap_uint_128 *)sds_alloc(TOTAL_SHARED_MEM_UINT128 * sizeof(ap_uint_128));
#endif
}

void cleanup()
{
    free(tracebackMatrix_SW);
    sds_free(shared_mem);
}

// ###################################

bool ENABLE_PRINT = false;

void print_result(int ed, char *CIGARstr2, char *MD, char *pattern, int countM, int countS, int countOpen, int countExtend)
{
    if (ENABLE_PRINT)
    {
        int score = countM * scoreM - countS * scoreS - countOpen * (scoreOpen + scoreExtend) - countExtend * scoreExtend;

        printf("EditDistance:%d  AlignmentScore:%d  CIGAR:%s  MD:%s \n", ed, score, CIGARstr2, MD);
        printf("#Matches:%d  #Substitutions:%d  #Gap-open:%d  #Gap-extend:%d \n", countM, countS, countOpen, countExtend);

        construct_pairwise_alignment(pattern, CIGARstr2, MD);
    }
}

// ###################################
// for comparison between SW/HW outputs, used for design verification

bool ENABLE_COMPARE = false;
int ed_SW;
int ed_HW;
char CIGARstr2_SW[STRING_SIZE_LARGE];
char CIGARstr2_HW[STRING_SIZE_LARGE];
char MD_SW[STRING_SIZE_LARGE];
char MD_HW[STRING_SIZE_LARGE];

void compareOutputs()
{
    if (ed_SW != ed_HW)
    {
        printf(" ERROR edit distance mismatch %d %d \n", ed_SW, ed_HW);
        cleanup();
        exit(-1);
    }

    if (strcmp(CIGARstr2_SW, CIGARstr2_HW) != 0 || strcmp(MD_SW, MD_HW) != 0)
    {
        printf(" ERROR string mismatch %s %s %s %s \n", CIGARstr2_SW, CIGARstr2_HW, MD_SW, MD_HW);
        cleanup();
        exit(-1);
    }
}

// ###################################

///////////////////////////////////////
// Small Alignment / Single Call
///////////////////////////////////////

void genasm_aligner_SW(char *text, char *pattern, int k, int n, int m)
{
    if (ENABLE_PRINT)
        printf(">>> SW MODE <<<\n");
    globalExecMode = SW_MODE;

    int minError;
    uint64_t mask;

    int ed = 0;
    char CIGARstr[STRING_SIZE_SMALL];
    strcpy(CIGARstr, "");
    char CIGARstr2[STRING_SIZE_SMALL];
    strcpy(CIGARstr2, "");
    char MD[STRING_SIZE_SMALL];
    strcpy(MD, "");
    int charCount = 0;
    int charCount2 = 0;
    int charCount3 = 0;
    char lastChar = '0';
    char lastChar2 = '0';
    char lastChar3 = '0';

    bool isFirst = true;

    int countM = 0;
    int countS = 0;
    int countI = 0;
    int countD = 0;
    int countOpen = 0;
    int countExtend = 0;

    __TIME_STAMP_START__

    genasmDC_SW(text, pattern, k, n, m, &minError, &mask, tracebackMatrix_SW);

    if (minError == -1)
    {
        printf("No alignment found!\n");
        cleanup();
        exit(-1);
    }

    genasmTB(n, k, tracebackMatrix_SW, m, minError, &ed, mask, &lastChar, &lastChar2, &lastChar3, &charCount, &charCount2, &charCount3, CIGARstr, CIGARstr2, MD, text,
             &countM, &countS, &countD, &countI, &countOpen, &countExtend, &isFirst, scoreS, scoreOpen, scoreExtend);

    __TIME_STAMP_END__

    sprintf(CIGARstr, "%s%d", CIGARstr, charCount);
    sprintf(CIGARstr, "%s%c", CIGARstr, lastChar);

    sprintf(CIGARstr2, "%s%d", CIGARstr2, charCount2);
    sprintf(CIGARstr2, "%s%c", CIGARstr2, lastChar2);

    if (lastChar3 == 'M')
        sprintf(MD, "%s%d", MD, charCount3);

    print_result(ed, CIGARstr2, MD, pattern, countM, countS, countOpen, countExtend);

    if (ENABLE_COMPARE)
    {
        ed_SW = ed;
        strcpy(CIGARstr2_SW, CIGARstr2);
        strcpy(MD_SW, MD);
    }
}

void genasm_aligner_HW(char *text, char *pattern, int k, int n, int m)
{
    if (ENABLE_PRINT)
        printf(">>> HW MODE <<<\n");
    globalExecMode = HW_MODE;

    ap_uint_128 param_0, param_1, param_2, param_3, param_4;

    int ed = 0;
    char CIGARstr[STRING_SIZE_SMALL];
    strcpy(CIGARstr, "");
    char CIGARstr2[STRING_SIZE_SMALL];
    strcpy(CIGARstr2, "");
    char MD[STRING_SIZE_SMALL];
    strcpy(MD, "");
    int charCount = 0;
    int charCount2 = 0;
    int charCount3 = 0;
    char lastChar = '0';
    char lastChar2 = '0';
    char lastChar3 = '0';

    int minError = -1;

    bool isFirst = true;

    int countM = 0;
    int countS = 0;
    int countI = 0;
    int countD = 0;
    int countOpen = 0;
    int countExtend = 0;

    int DQ_mode = 0;

    __TIME_STAMP_START__

    param_0(31, 0) = k;
    param_0(63, 32) = n;
    param_0(95, 64) = m;
    param_0(127, 96) = O;

    param_1(39, 32) = isFirst;
    param_1(40, 40) = DQ_mode;

    int combined_lastChar = lastChar3 << 16 | lastChar2 << 8 | lastChar;
    param_2(31, 0) = charCount;
    param_2(63, 32) = charCount2;
    param_2(95, 64) = charCount3;
    param_2(127, 96) = combined_lastChar;

    shared_mem[PARAMS_OFFSET + 0] = param_0;
    shared_mem[PARAMS_OFFSET + 1] = param_1;
    shared_mem[PARAMS_OFFSET + 2] = param_2;

    memcpy(shared_mem + TEXT_OFFSET, text, MAX_TEXT_SZ);
    memcpy(shared_mem + PATTERN_OFFSET, pattern, MAX_PATTERN_SZ);

    ///////
    genasm_HW(shared_mem);
    //////

    param_1 = shared_mem[PARAMS_OFFSET + 1];
    param_2 = shared_mem[PARAMS_OFFSET + 2];
    param_3 = shared_mem[PARAMS_OFFSET + 3];
    param_4 = shared_mem[PARAMS_OFFSET + 4];

    ed += param_1(31, 0);
    isFirst = param_1(39, 32);
    minError = param_1(63, 63);
    int index_CIGARstr = param_1(79, 64);
    int index_CIGARstr2 = param_1(95, 80);
    int index_MD = param_1(111, 96);

    charCount = param_2(31, 0);
    charCount2 = param_2(63, 32);
    charCount3 = param_2(95, 64);
    lastChar = param_2(103, 96);
    lastChar2 = param_2(111, 104);
    lastChar3 = param_2(119, 112);

    countM += param_3(31, 0);
    countS += param_3(63, 32);
    countI += param_3(95, 64);
    countD += param_3(127, 96);
    countOpen += param_4(31, 0);
    countExtend += param_4(63, 32);

    // assert(index_CIGARstr < CIGAR_STR_SZ);
    // assert(index_CIGARstr2 < CIGAR_STR2_SZ);
    // assert(index_MD < MD_STR_SZ);

    if (minError == -1)
    {
        printf("No alignment found!\t");
        cleanup();
        exit(-1);
    }

    int amount; // align text width for 64-bit access, otherwise bus errors?
    void *ptr;

    amount = ((index_CIGARstr / 8) + 1) * 8;
    ptr = (void *)(shared_mem + CIGAR_STR_OFFSET);
    memcpy(CIGARstr, ptr, amount);
    CIGARstr[index_CIGARstr] = '\0';

    amount = ((index_CIGARstr2 / 8) + 1) * 8;
    ptr = (void *)(shared_mem + CIGAR_STR2_OFFSET);
    memcpy(CIGARstr2, ptr, amount);
    CIGARstr2[index_CIGARstr2] = '\0';

    amount = ((index_MD / 8) + 1) * 8;
    ptr = (void *)(shared_mem + MD_STR_OFFSET);
    memcpy(MD, ptr, amount);
    MD[index_MD] = '\0';

    __TIME_STAMP_END__

    sprintf(CIGARstr, "%s%d", CIGARstr, charCount);
    sprintf(CIGARstr, "%s%c", CIGARstr, lastChar);

    sprintf(CIGARstr2, "%s%d", CIGARstr2, charCount2);
    sprintf(CIGARstr2, "%s%c", CIGARstr2, lastChar2);

    if (lastChar3 == 'M')
        sprintf(MD, "%s%d", MD, charCount3);

    print_result(ed, CIGARstr2, MD, pattern, countM, countS, countOpen, countExtend);

    if (ENABLE_COMPARE)
    {
        ed_HW = ed;
        strcpy(CIGARstr2_HW, CIGARstr2);
        strcpy(MD_HW, MD);
    }
}

// ###################################

///////////////////////////////////////
// DQ
///////////////////////////////////////

void genasm_aligner_DQ_SW(char *text, char *pattern, int k)
{
    if (ENABLE_PRINT)
        printf(">>> SW MODE <<<\n");
    globalExecMode = SW_MODE;

    int m = (int)strlen(pattern);
    int n = (int)strlen(text);

    uint64_t max = ULLONG_MAX;

    int ed = 0;
    char CIGARstr[STRING_SIZE_LARGE];
    strcpy(CIGARstr, "");
    char CIGARstr2[STRING_SIZE_LARGE];
    strcpy(CIGARstr2, "");
    char MD[STRING_SIZE_LARGE];
    strcpy(MD, "");
    int charCount = 0;
    int charCount2 = 0;
    int charCount3 = 0;
    char lastChar = '0';
    char lastChar2 = '0';
    char lastChar3 = '0';

    bool isFirst = true;

    int countM = 0;
    int countS = 0;
    int countI = 0;
    int countD = 0;
    int countOpen = 0;
    int countExtend = 0;

    int textCur = 0;
    int patternCur = 0;
    int textEnd = 0;
    int patternEnd = 0;

    int newError = 0;
    if (W > k)
        newError = k;
    else
        newError = W;

    char subText[MAX_TEXT_SZ];
    char subPattern[MAX_PATTERN_SZ];

    while ((patternCur < m) && (textCur < n))
    {
        textEnd = textCur + W;
        if (textEnd > n)
            textEnd = n;

        patternEnd = patternCur + W;
        if (patternEnd > m)
            patternEnd = m;

        int n_sub = textEnd - textCur;
        int m_sub = patternEnd - patternCur;

        memcpy(subText, text + textCur, n_sub);
        subText[n_sub] = '\0';

        memcpy(subPattern, pattern + patternCur, m_sub);
        subPattern[m_sub] = '\0';

        int minError = -1;
        uint64_t mask;

        int textConsumed;
        int patternConsumed;

        if (ENABLE_PRINT)
            printf("=> Aligner Call  TextPos:%d PatternPos:%d \n", textCur, patternCur);

        __TIME_STAMP_START__

        genasmDC_SW(subText, subPattern, newError, n_sub, m_sub, &minError, &mask, tracebackMatrix_SW);

        if (minError == -1)
        {
            printf("No alignment found!\t");
            cleanup();
            exit(-1);
        }

        genasmTB_DQ(W, O, minError, tracebackMatrix_SW, m_sub, minError, &ed, mask, &textConsumed, &patternConsumed, &lastChar, &lastChar2, &lastChar3, &charCount, &charCount2, &charCount3, CIGARstr, CIGARstr2, MD, subText, &countM, &countS, &countD, &countI, &countOpen, &countExtend, &isFirst, scoreS, scoreOpen, scoreExtend);

        textCur += textConsumed;
        patternCur += patternConsumed;

        __TIME_STAMP_END__
    }

    sprintf(CIGARstr, "%s%d", CIGARstr, charCount);
    sprintf(CIGARstr, "%s%c", CIGARstr, lastChar);

    sprintf(CIGARstr2, "%s%d", CIGARstr2, charCount2);
    sprintf(CIGARstr2, "%s%c", CIGARstr2, lastChar2);

    if (lastChar3 == 'M')
        sprintf(MD, "%s%d", MD, charCount3);

    print_result(ed, CIGARstr2, MD, pattern, countM, countS, countOpen, countExtend);

    if (ENABLE_COMPARE)
    {
        ed_SW = ed;
        strcpy(CIGARstr2_SW, CIGARstr2);
        strcpy(MD_SW, MD);
    }
}

void genasm_aligner_DQ_HW(char *text, char *pattern, int k)
{
    if (ENABLE_PRINT)
        printf(">>> HW MODE <<<\n");
    globalExecMode = HW_MODE;

    int m = (int)strlen(pattern);
    int n = (int)strlen(text);

    uint64_t max = ULLONG_MAX;

    int ed = 0;
    char CIGARstr[STRING_SIZE_LARGE];
    strcpy(CIGARstr, "");
    char CIGARstr2[STRING_SIZE_LARGE];
    strcpy(CIGARstr2, "");
    char MD[STRING_SIZE_LARGE];
    strcpy(MD, "");
    int curr_index_CIGARstr = 0;
    int curr_index_CIGARstr2 = 0;
    int curr_index_MD = 0;
    int charCount = 0;
    int charCount2 = 0;
    int charCount3 = 0;
    char lastChar = '0';
    char lastChar2 = '0';
    char lastChar3 = '0';

    bool isFirst = true;

    int countM = 0;
    int countS = 0;
    int countI = 0;
    int countD = 0;
    int countOpen = 0;
    int countExtend = 0;

    int textCur = 0;
    int patternCur = 0;
    int textEnd = 0;
    int patternEnd = 0;

    int newError = 0;
    if (W > k)
        newError = k;
    else
        newError = W;

    int DQ_mode = 1;

    ap_uint_128 param_0, param_1, param_2, param_3, param_4;

    //
    param_0(31, 0) = newError;
    param_0(127, 96) = O;
    param_1(39, 32) = isFirst;
    param_1(40, 40) = DQ_mode;
    shared_mem[PARAMS_OFFSET + 1] = param_1;

    int combined_lastChar = lastChar3 << 16 | lastChar2 << 8 | lastChar;
    param_2(31, 0) = charCount;
    param_2(63, 32) = charCount2;
    param_2(95, 64) = charCount3;
    param_2(127, 96) = combined_lastChar;
    shared_mem[PARAMS_OFFSET + 2] = param_2;

    while ((patternCur < m) && (textCur < n))
    {
        textEnd = textCur + W;
        if (textEnd > n)
            textEnd = n;

        patternEnd = patternCur + W;
        if (patternEnd > m)
            patternEnd = m;

        int n_sub = textEnd - textCur;
        int m_sub = patternEnd - patternCur;

        int minError = -1;
        uint64_t mask;

        if (ENABLE_PRINT)
            printf("=> Aligner Call  TextPos:%d PatternPos:%d \n", textCur, patternCur);

        __TIME_STAMP_START__

        param_0(63, 32) = n_sub;
        param_0(95, 64) = m_sub;
        shared_mem[PARAMS_OFFSET + 0] = param_0;

        memcpy(shared_mem + TEXT_OFFSET, text + textCur, MAX_TEXT_SZ); // text buffer size must have at least MAX_TEXT_SZ chars
        memcpy(shared_mem + PATTERN_OFFSET, pattern + patternCur, MAX_PATTERN_SZ); // ^ same for pattern buffer

        ///////
        genasm_HW(shared_mem);
        ///////

        param_1 = shared_mem[PARAMS_OFFSET + 1];
        param_3 = shared_mem[PARAMS_OFFSET + 3];
        param_4 = shared_mem[PARAMS_OFFSET + 4];

        ed += param_1(31, 0);
        minError = param_1(63, 63);
        int index_CIGARstr = param_1(79, 64);
        int index_CIGARstr2 = param_1(95, 80);
        int index_MD = param_1(111, 96);

        countM += param_3(31, 0);
        countS += param_3(63, 32);
        countI += param_3(95, 64);
        countD += param_3(127, 96);
        countOpen += param_4(31, 0);
        countExtend += param_4(63, 32);

        int textConsumed = 0;
        int patternConsumed = 0;
        textConsumed += param_4(95, 64);
        patternConsumed += param_4(127, 96);
        textCur += textConsumed;
        patternCur += patternConsumed;

        // assert(index_CIGARstr < CIGAR_STR_SZ);
        // assert(index_CIGARstr2 < CIGAR_STR2_SZ);
        // assert(index_MD < MD_STR_SZ);

        if (minError == -1)
        {
            printf("No alignment found!\t");
            cleanup();
            exit(-1);
        }

        int amount; // align text width for 64-bit access, otherwise bus errors?
        void *ptr;

        amount = ((index_CIGARstr / 8) + 1) * 8;
        ptr = (void *)(shared_mem + CIGAR_STR_OFFSET);
        memcpy(CIGARstr + curr_index_CIGARstr, ptr, amount);
        curr_index_CIGARstr += index_CIGARstr;
        CIGARstr[curr_index_CIGARstr] = '\0';

        amount = ((index_CIGARstr2 / 8) + 1) * 8;
        ptr = (void *)(shared_mem + CIGAR_STR2_OFFSET);
        memcpy(CIGARstr2 + curr_index_CIGARstr2, ptr, amount);
        curr_index_CIGARstr2 += index_CIGARstr2;
        CIGARstr2[curr_index_CIGARstr2] = '\0';

        amount = ((index_MD / 8) + 1) * 8;
        ptr = (void *)(shared_mem + MD_STR_OFFSET);
        memcpy(MD + curr_index_MD, ptr, amount);
        curr_index_MD += index_MD;
        MD[curr_index_MD] = '\0';

        __TIME_STAMP_END__
    }

    param_2 = shared_mem[PARAMS_OFFSET + 2];
    charCount = param_2(31, 0);
    charCount2 = param_2(63, 32);
    charCount3 = param_2(95, 64);
    lastChar = param_2(103, 96);
    lastChar2 = param_2(111, 104);
    lastChar3 = param_2(119, 112);

    sprintf(CIGARstr, "%s%d", CIGARstr, charCount);
    sprintf(CIGARstr, "%s%c", CIGARstr, lastChar);

    sprintf(CIGARstr2, "%s%d", CIGARstr2, charCount2);
    sprintf(CIGARstr2, "%s%c", CIGARstr2, lastChar2);

    if (lastChar3 == 'M')
        sprintf(MD, "%s%d", MD, charCount3);

    print_result(ed, CIGARstr2, MD, pattern, countM, countS, countOpen, countExtend);

    if (ENABLE_COMPARE)
    {
        ed_HW = ed;
        strcpy(CIGARstr2_HW, CIGARstr2);
        strcpy(MD_HW, MD);
    }
}

// ###################################

enum TestingMode
{
    SW_TESTS,
    HW_TESTS,
    SW_HW_TESTS
};

void run_tests_small_reads(TestingMode testmode)
{
    printf("\n\n====================\n\n");
    printf("\n\nTesting very small reads\n\n");

    char text[MAX_TEXT_SZ + 1];
    char pattern[MAX_PATTERN_SZ + 1];

    int text_len, pattern_len, k; // fixed here
    text_len = MAX_TEXT_SZ;
    pattern_len = MAX_PATTERN_SZ;
    k = MAX_EDIT_THRESHOLD;

    std::ifstream file("data/genome1.txt");
    if (file.is_open())
    {
        int i = 0;
        std::string text_rd, pattern_rd, null_str;
        while (std::getline(file, text_rd))
        {
            std::getline(file, pattern_rd);
            std::getline(file, null_str);

            printf("\n\n\n\n\n");
            printf("== TEST %d ==\n", i + 1);
            i++;

            sprintf(text, "%s", text_rd.c_str());
            sprintf(pattern, "%s", pattern_rd.c_str());

            printf("Text    %s \t Length %d \n", text, text_len);
            printf("Pattern %s \t Length %d \n", pattern, pattern_len);
            printf("K       %d \n", k);
            printf("\n");

            if (testmode == SW_TESTS)
            {
                genasm_aligner_SW(text, pattern, k, text_len, pattern_len);
            }
            else if (testmode == HW_TESTS)
            {
                genasm_aligner_HW(text, pattern, k, text_len, pattern_len);
            }
            else if (testmode == SW_HW_TESTS)
            {
                genasm_aligner_SW(text, pattern, k, text_len, pattern_len);
                printf("\n");
                genasm_aligner_HW(text, pattern, k, text_len, pattern_len);

                if (ENABLE_COMPARE)
                    compareOutputs();
            }
        }
        file.close();
    }
    else
    {
        printf("Cannot load data input file.\n");
        cleanup();
        exit(-1);
    }
}

void run_tests_long_reads()
{
    printf("\n\n====================\n\n");
    printf("\n\nTesting longer reads\n\n");

    char text[STRING_SIZE_LARGE];
    char pattern[STRING_SIZE_LARGE];
    int WINDOW_K = MAX_EDIT_THRESHOLD;

    std::ifstream file("data/genome2.txt");
    if (file.is_open())
    {
        int i = 0;
        std::string text_rd, pattern_rd, null_str;
        while (std::getline(file, text_rd))
        {
            std::getline(file, pattern_rd);
            std::getline(file, null_str);

            printf("\n\n\n\n\n");
            printf("== TEST %d ==\n", i + 1);
            i++;

            sprintf(text, "%s", text_rd.c_str());
            sprintf(pattern, "%s", pattern_rd.c_str());

            printf("Text    %s \t Length %d \n", text, (int)strlen(text));
            printf("Pattern %s \t Length %d \n", pattern, (int)strlen(pattern));
            printf("K       %d \n", WINDOW_K);
            printf("\n");

            genasm_aligner_DQ_SW(text, pattern, WINDOW_K);
            printf("\n");
            genasm_aligner_DQ_HW(text, pattern, WINDOW_K);

            if (ENABLE_COMPARE)
                compareOutputs();
        }
        file.close();
    }
    else
    {
        printf("Cannot load data input file.\n");
        cleanup();
        exit(-1);
    }
}

int main(int argc, char *argv[])
{
    printf("\nGENASM TESTBENCH / DEMO\n");

    if (argc < 2)
    {
        printf("Insufficient program arguments.\n");
        exit(-1);
    }

    int mode = atoi(argv[1]);

    ENABLE_PRINT = true;
    ENABLE_COMPARE = false;

    init();

    // ############# Demo 1
    if (mode == 0)
    {
        run_tests_small_reads(SW_HW_TESTS); // run SW/HW alternately for verification/comparison of outputs
        report_time();
    }

    // ############# Demo 2
    else if (mode == 1)
    {
        run_tests_small_reads(SW_TESTS); // run all at once in SW only
        run_tests_small_reads(HW_TESTS); // run all at once in HW only
        report_time();
    }

    // ############# Demo 3
    else if (mode == 2)
    {
        run_tests_long_reads(); // DQ, SW & HW
        report_time();
    }

    cleanup();
}
