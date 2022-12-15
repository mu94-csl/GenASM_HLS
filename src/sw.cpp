
#include "common.h"

inline uint64_t get_tracebackMatrix_value(uint64_t *tracebackMatrix, int t, int k, int i)
{
    const int idx1 = (MAX_EDIT_THRESHOLD + 1) * 4;
    const int idx2 = 4;

    return tracebackMatrix[t * idx1 + k * idx2 + i];
}

inline void set_tracebackMatrix_value(uint64_t *tracebackMatrix, int t, int k, int i, uint64_t val)
{
    const int idx1 = (MAX_EDIT_THRESHOLD + 1) * 4;
    const int idx2 = 4;
    tracebackMatrix[t * idx1 + k * idx2 + i] = val;
}

void genasmTB(int n, int k, uint64_t *tracebackMatrix, int m, int minError, int *ed, uint64_t mask,
              char *lastChar, char *lastChar2, char *lastChar3, int *charCount, int *charCount2, int *charCount3, char *CIGARstr, char *CIGARstr2, char *MD, char *text,
              int *countM, int *countS, int *countD, int *countI, int *countOpen, int *countExtend,
              bool *isFirst, int scoreS, int scoreOpen, int scoreExtend)
{
    int curPattern = m - 1;
    int curText = 0;
    int curError = minError;

    while ((curPattern >= 0) && (curError >= 0))
    {
        // affine-insertion
        if (*lastChar == 'I' && ((get_tracebackMatrix_value(tracebackMatrix, curText, curError, 2) & mask) == 0))
        {
            curPattern -= 1;
            curError -= 1;
            mask = 1ULL << curPattern;
            if (*lastChar == 'I')
            {
                *charCount += 1;
                *countExtend += 1;
            }
            else
            {
                if (!*isFirst)
                {
                    sprintf(CIGARstr, "%s%d", CIGARstr, *charCount);
                    sprintf(CIGARstr, "%s%c", CIGARstr, *lastChar);
                }
                *charCount = 1;
                *lastChar = 'I';
                *countOpen += 1;
            }
            if (*lastChar2 == 'I')
            {
                *charCount2 += 1;
            }
            else
            {
                if (!*isFirst)
                {
                    sprintf(CIGARstr2, "%s%d", CIGARstr2, *charCount2);
                    sprintf(CIGARstr2, "%s%c", CIGARstr2, *lastChar2);
                }
                *charCount2 = 1;
                *lastChar2 = 'I';
            }
            *countI += 1;
            *ed += 1;
        }
        // affine-deletion
        else if (*lastChar == 'D' && ((get_tracebackMatrix_value(tracebackMatrix, curText, curError, 3) & mask) == 0))
        {
            curText += 1;
            curError -= 1;
            if (*lastChar == 'D')
            {
                *charCount += 1;
                *countExtend += 1;
            }
            else
            {
                if (!*isFirst)
                {
                    sprintf(CIGARstr, "%s%d", CIGARstr, *charCount);
                    sprintf(CIGARstr, "%s%c", CIGARstr, *lastChar);
                }
                *charCount = 1;
                *lastChar = 'D';
                *countOpen += 1;
            }
            if (*lastChar2 == 'D')
            {
                *charCount2 += 1;
            }
            else
            {
                if (!*isFirst)
                {
                    sprintf(CIGARstr2, "%s%d", CIGARstr2, *charCount2);
                    sprintf(CIGARstr2, "%s%c", CIGARstr2, *lastChar2);
                }
                *charCount2 = 1;
                *lastChar2 = 'D';
            }
            if (*lastChar3 == 'M')
            {
                sprintf(MD, "%s%d^%c", MD, *charCount3, text[curText - 1]);
                *lastChar3 = 'D';
                *charCount3 = 0;
            }
            else if (*lastChar3 == 'D')
            {
                sprintf(MD, "%s%c", MD, text[curText - 1]);
                *lastChar3 = 'D';
                *charCount3 = 0;
            }
            else
            {
                sprintf(MD, "%s^%c", MD, text[curText - 1]);
                *lastChar3 = 'D';
                *charCount3 = 0;
            }
            *countD += 1;
            *ed += 1;
        }
        // match
        else if ((get_tracebackMatrix_value(tracebackMatrix, curText, curError, 0) & mask) == 0)
        {
            curText += 1;
            curPattern -= 1;
            mask = 1ULL << curPattern;
            if (*lastChar == 'M')
            {
                *charCount += 1;
            }
            else
            {
                if (!*isFirst)
                {
                    sprintf(CIGARstr, "%s%d", CIGARstr, *charCount);
                    sprintf(CIGARstr, "%s%c", CIGARstr, *lastChar);
                }
                *charCount = 1;
                *lastChar = 'M';
            }
            if (*lastChar2 == 'M')
            {
                *charCount2 += 1;
            }
            else
            {
                if (!*isFirst)
                {
                    sprintf(CIGARstr2, "%s%d", CIGARstr2, *charCount2);
                    sprintf(CIGARstr2, "%s%c", CIGARstr2, *lastChar2);
                }
                *charCount2 = 1;
                *lastChar2 = 'M';
            }
            if (*lastChar3 == 'M')
            {
                *charCount3 += 1;
            }
            else
            {
                *charCount3 = 1;
                *lastChar3 = 'M';
            }
            *countM += 1;
        }
        // substitution
        else if ((get_tracebackMatrix_value(tracebackMatrix, curText, curError, 1) & mask) == 0)
        {
            curText += 1;
            curPattern -= 1;
            curError -= 1;
            mask = 1ULL << curPattern;
            if (*lastChar == 'S')
            {
                *charCount += 1;
            }
            else
            {
                if (!*isFirst)
                {
                    sprintf(CIGARstr, "%s%d", CIGARstr, *charCount);
                    sprintf(CIGARstr, "%s%c", CIGARstr, *lastChar);
                }
                *charCount = 1;
                *lastChar = 'S';
            }
            if (*lastChar2 == 'M')
            {
                *charCount2 += 1;
            }
            else
            {
                if (!*isFirst)
                {
                    sprintf(CIGARstr2, "%s%d", CIGARstr2, *charCount2);
                    sprintf(CIGARstr2, "%s%c", CIGARstr2, *lastChar2);
                }
                *charCount2 = 1;
                *lastChar2 = 'M';
            }
            if (*lastChar3 == 'M')
            {
                sprintf(MD, "%s%d%c", MD, *charCount3, text[curText - 1]);
                *lastChar3 = 'S';
                *charCount3 = 0;
            }
            else
            {
                sprintf(MD, "%s%c", MD, text[curText - 1]);
                *lastChar3 = 'S';
                *charCount3 = 0;
            }
            *countS += 1;
            *ed += 1;
        }
        // deletion
        else if ((get_tracebackMatrix_value(tracebackMatrix, curText, curError, 3) & mask) == 0)
        {
            curText += 1;
            curError -= 1;
            if (*lastChar == 'D')
            {
                *charCount += 1;
                *countExtend += 1;
            }
            else
            {
                if (!*isFirst)
                {
                    sprintf(CIGARstr, "%s%d", CIGARstr, *charCount);
                    sprintf(CIGARstr, "%s%c", CIGARstr, *lastChar);
                }
                *charCount = 1;
                *lastChar = 'D';
                *countOpen += 1;
            }
            if (*lastChar2 == 'D')
            {
                *charCount2 += 1;
            }
            else
            {
                if (!*isFirst)
                {
                    sprintf(CIGARstr2, "%s%d", CIGARstr2, *charCount2);
                    sprintf(CIGARstr2, "%s%c", CIGARstr2, *lastChar2);
                }
                *charCount2 = 1;
                *lastChar2 = 'D';
            }
            if (*lastChar3 == 'M')
            {
                sprintf(MD, "%s%d^%c", MD, *charCount3, text[curText - 1]);
                *lastChar3 = 'D';
                *charCount3 = 0;
            }
            else if (*lastChar3 == 'D')
            {
                sprintf(MD, "%s%c", MD, text[curText - 1]);
                *lastChar3 = 'D';
                *charCount3 = 0;
            }
            else
            {
                sprintf(MD, "%s^%c", MD, text[curText - 1]);
                *lastChar3 = 'D';
                *charCount3 = 0;
            }
            *countD += 1;
            *ed += 1;
        }
        // insertion
        else if ((get_tracebackMatrix_value(tracebackMatrix, curText, curError, 2) & mask) == 0)
        {
            curPattern -= 1;
            curError -= 1;
            mask = 1ULL << curPattern;
            if (*lastChar == 'I')
            {
                *charCount += 1;
                *countExtend += 1;
            }
            else
            {
                if (!*isFirst)
                {
                    sprintf(CIGARstr, "%s%d", CIGARstr, *charCount);
                    sprintf(CIGARstr, "%s%c", CIGARstr, *lastChar);
                }
                *charCount = 1;
                *lastChar = 'I';
                *countOpen += 1;
            }
            if (*lastChar2 == 'I')
            {
                *charCount2 += 1;
            }
            else
            {
                if (!*isFirst)
                {
                    sprintf(CIGARstr2, "%s%d", CIGARstr2, *charCount2);
                    sprintf(CIGARstr2, "%s%c", CIGARstr2, *lastChar2);
                }
                *charCount2 = 1;
                *lastChar2 = 'I';
            }
            *countI += 1;
            *ed += 1;
        }

        *isFirst = false;
    }
}

void genasmTB_DQ(int W, int O, int k, uint64_t *tracebackMatrix, int m, int minError, int *ed, uint64_t mask, int *textConsumed, int *patternConsumed,
                 char *lastChar, char *lastChar2, char *lastChar3, int *charCount, int *charCount2, int *charCount3, char *CIGARstr, char *CIGARstr2, char *MD, char *text,
                 int *countM, int *countS, int *countD, int *countI, int *countOpen, int *countExtend,
                 bool *isFirst, int scoreS, int scoreOpen, int scoreExtend)
{
    int curPattern = m - 1;
    int curText = 0;
    int curError = minError;

    *textConsumed = 0;
    *patternConsumed = 0;

    while ((*textConsumed < (W - O)) && (*patternConsumed < (W - O)) && (curPattern >= 0) && (curError >= 0))
    {
        // affine-insertion
        if (*lastChar == 'I' && ((get_tracebackMatrix_value(tracebackMatrix, curText, curError, 2) & mask) == 0))
        {
            curPattern -= 1;
            curError -= 1;
            mask = 1ULL << curPattern;
            if (*lastChar == 'I')
            {
                *charCount += 1;
                *countExtend += 1;
            }
            else
            {
                if (!*isFirst)
                {
                    sprintf(CIGARstr, "%s%d", CIGARstr, *charCount);
                    sprintf(CIGARstr, "%s%c", CIGARstr, *lastChar);
                }
                *charCount = 1;
                *lastChar = 'I';
                *countOpen += 1;
            }
            if (*lastChar2 == 'I')
            {
                *charCount2 += 1;
            }
            else
            {
                if (!*isFirst)
                {
                    sprintf(CIGARstr2, "%s%d", CIGARstr2, *charCount2);
                    sprintf(CIGARstr2, "%s%c", CIGARstr2, *lastChar2);
                }
                *charCount2 = 1;
                *lastChar2 = 'I';
            }
            *countI += 1;
            *ed += 1;
            *patternConsumed += 1;
        }
        // affine-deletion
        else if (*lastChar == 'D' && ((get_tracebackMatrix_value(tracebackMatrix, curText, curError, 3) & mask) == 0))
        {
            curText += 1;
            curError -= 1;
            if (*lastChar == 'D')
            {
                *charCount += 1;
                *countExtend += 1;
            }
            else
            {
                if (!*isFirst)
                {
                    sprintf(CIGARstr, "%s%d", CIGARstr, *charCount);
                    sprintf(CIGARstr, "%s%c", CIGARstr, *lastChar);
                }
                *charCount = 1;
                *lastChar = 'D';
                *countOpen += 1;
            }
            if (*lastChar2 == 'D')
            {
                *charCount2 += 1;
            }
            else
            {
                if (!*isFirst)
                {
                    sprintf(CIGARstr2, "%s%d", CIGARstr2, *charCount2);
                    sprintf(CIGARstr2, "%s%c", CIGARstr2, *lastChar2);
                }
                *charCount2 = 1;
                *lastChar2 = 'D';
            }
            if (*lastChar3 == 'M')
            {
                sprintf(MD, "%s%d^%c", MD, *charCount3, text[curText - 1]);
                *lastChar3 = 'D';
                *charCount3 = 0;
            }
            else if (*lastChar3 == 'D')
            {
                sprintf(MD, "%s%c", MD, text[curText - 1]);
                *lastChar3 = 'D';
                *charCount3 = 0;
            }
            else
            {
                sprintf(MD, "%s^%c", MD, text[curText - 1]);
                *lastChar3 = 'D';
                *charCount3 = 0;
            }

            *countD += 1;
            *ed += 1;
            *textConsumed += 1;
        }
        // match
        else if ((get_tracebackMatrix_value(tracebackMatrix, curText, curError, 0) & mask) == 0)
        {
            curText += 1;
            curPattern -= 1;
            mask = 1ULL << curPattern;
            if (*lastChar == 'M')
            {
                *charCount += 1;
            }
            else
            {
                if (!*isFirst)
                {
                    sprintf(CIGARstr, "%s%d", CIGARstr, *charCount);
                    sprintf(CIGARstr, "%s%c", CIGARstr, *lastChar);
                }
                *charCount = 1;
                *lastChar = 'M';
            }
            if (*lastChar2 == 'M')
            {
                *charCount2 += 1;
            }
            else
            {
                if (!*isFirst)
                {
                    sprintf(CIGARstr2, "%s%d", CIGARstr2, *charCount2);
                    sprintf(CIGARstr2, "%s%c", CIGARstr2, *lastChar2);
                }
                *charCount2 = 1;
                *lastChar2 = 'M';
            }
            if (*lastChar3 == 'M')
            {
                *charCount3 += 1;
            }
            else
            {
                *charCount3 = 1;
                *lastChar3 = 'M';
            }
            *countM += 1;
            *textConsumed += 1;
            *patternConsumed += 1;
        }
        // substitution
        else if ((get_tracebackMatrix_value(tracebackMatrix, curText, curError, 1) & mask) == 0)
        {
            curText += 1;
            curPattern -= 1;
            curError -= 1;
            mask = 1ULL << curPattern;
            if (*lastChar == 'S')
            {
                *charCount += 1;
            }
            else
            {
                if (!*isFirst)
                {
                    sprintf(CIGARstr, "%s%d", CIGARstr, *charCount);
                    sprintf(CIGARstr, "%s%c", CIGARstr, *lastChar);
                }
                *charCount = 1;
                *lastChar = 'S';
            }
            if (*lastChar2 == 'M')
            {
                *charCount2 += 1;
            }
            else
            {
                if (!*isFirst)
                {
                    sprintf(CIGARstr2, "%s%d", CIGARstr2, *charCount2);
                    sprintf(CIGARstr2, "%s%c", CIGARstr2, *lastChar2);
                }
                *charCount2 = 1;
                *lastChar2 = 'M';
            }
            if (*lastChar3 == 'M')
            {
                sprintf(MD, "%s%d%c", MD, *charCount3, text[curText - 1]);
                *lastChar3 = 'S';
                *charCount3 = 0;
            }
            else
            {
                sprintf(MD, "%s%c", MD, text[curText - 1]);
                *lastChar3 = 'S';
                *charCount3 = 0;
            }
            *countS += 1;
            *ed += 1;
            *textConsumed += 1;
            *patternConsumed += 1;
        }
        // deletion
        else if ((get_tracebackMatrix_value(tracebackMatrix, curText, curError, 3) & mask) == 0)
        {
            curText += 1;
            curError -= 1;
            if (*lastChar == 'D')
            {
                *charCount += 1;
                *countExtend += 1;
            }
            else
            {
                if (!*isFirst)
                {
                    sprintf(CIGARstr, "%s%d", CIGARstr, *charCount);
                    sprintf(CIGARstr, "%s%c", CIGARstr, *lastChar);
                }
                *charCount = 1;
                *lastChar = 'D';
                *countOpen += 1;
            }
            if (*lastChar2 == 'D')
            {
                *charCount2 += 1;
            }
            else
            {
                if (!*isFirst)
                {
                    sprintf(CIGARstr2, "%s%d", CIGARstr2, *charCount2);
                    sprintf(CIGARstr2, "%s%c", CIGARstr2, *lastChar2);
                }
                *charCount2 = 1;
                *lastChar2 = 'D';
            }
            if (*lastChar3 == 'M')
            {
                sprintf(MD, "%s%d^%c", MD, *charCount3, text[curText - 1]);
                *lastChar3 = 'D';
                *charCount3 = 0;
            }
            else if (*lastChar3 == 'D')
            {
                sprintf(MD, "%s%c", MD, text[curText - 1]);
                *lastChar3 = 'D';
                *charCount3 = 0;
            }
            else
            {
                sprintf(MD, "%s^%c", MD, text[curText - 1]);
                *lastChar3 = 'D';
                *charCount3 = 0;
            }
            *countD += 1;
            *ed += 1;
            *textConsumed += 1;
        }
        // insertion
        else if ((get_tracebackMatrix_value(tracebackMatrix, curText, curError, 2) & mask) == 0)
        {
            curPattern -= 1;
            curError -= 1;
            mask = 1ULL << curPattern;
            if (*lastChar == 'I')
            {
                *charCount += 1;
                *countExtend += 1;
            }
            else
            {
                if (!*isFirst)
                {
                    sprintf(CIGARstr, "%s%d", CIGARstr, *charCount);
                    sprintf(CIGARstr, "%s%c", CIGARstr, *lastChar);
                }
                *charCount = 1;
                *lastChar = 'I';
                *countOpen += 1;
            }
            if (*lastChar2 == 'I')
            {
                *charCount2 += 1;
            }
            else
            {
                if (!*isFirst)
                {
                    sprintf(CIGARstr2, "%s%d", CIGARstr2, *charCount2);
                    sprintf(CIGARstr2, "%s%c", CIGARstr2, *lastChar2);
                }
                *charCount2 = 1;
                *lastChar2 = 'I';
            }
            *countI += 1;
            *ed += 1;
            *patternConsumed += 1;
        }

        *isFirst = false;
    }
}

void genasmDC_SW(char *text, char *pattern, int k, int n, int m, int *minError, uint64_t *mask, uint64_t *tracebackMatrix)
{
    uint64_t max = ULLONG_MAX; // all ones

    // ##############################################################

    int len = 4; // A,C,G,T
    uint64_t patternBitmasks[len];

    // Initialize the pattern bitmasks
    for (int i = 0; i < len; i++)
        patternBitmasks[i] = max;

    // Update the pattern bitmasks
    for (int i = 0; i < m; i++)
    {
        if ((pattern[i] == 'A') || (pattern[i] == 'a'))
        {
            patternBitmasks[0] &= ~(1ULL << ((m - i - 1)));
        }
        else if ((pattern[i] == 'C') || (pattern[i] == 'c'))
        {
            patternBitmasks[1] &= ~(1ULL << ((m - i - 1)));
        }
        else if ((pattern[i] == 'G') || (pattern[i] == 'g'))
        {
            patternBitmasks[2] &= ~(1ULL << ((m - i - 1)));
        }
        else if ((pattern[i] == 'T') || (pattern[i] == 't'))
        {
            patternBitmasks[3] &= ~(1ULL << ((m - i - 1)));
        }
    }

    // ##############################################################

    // Initialize the bit arrays R
    int len1 = (k + 1);
    uint64_t R[MAX_EDIT_THRESHOLD + 1];
    uint64_t oldR[MAX_EDIT_THRESHOLD + 1];
    uint64_t substitution, insertion, match, deletion, curBitmask;

    for (int i = 0; i < len1; i++)
    {
        R[i] = max;
        // printBits(8, &R[i]);
    }

    // now traverse the text in opposite direction (i.e., forward), generate partial tracebacks at each checkpoint
    for (int i = n - 1; i >= 0; i--)
    {
        char c = text[i];

        // if ((c == 'A') || (c == 'a') || (c == 'C') || (c == 'c') || (c == 'G') || (c == 'g') || (c == 'T') || (c == 't'))
        {
            // copy the content of R to oldR
            for (int itR = 0; itR < len1; itR++)
            {
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

            set_tracebackMatrix_value(tracebackMatrix, i, 0, 0, R[0]);
            set_tracebackMatrix_value(tracebackMatrix, i, 0, 1, max);
            set_tracebackMatrix_value(tracebackMatrix, i, 0, 2, max);
            set_tracebackMatrix_value(tracebackMatrix, i, 0, 3, max);

            for (int d = 1; d <= k; d++)
            {
                int index = (d - 1);
                deletion = oldR[index];
                substitution = deletion << 1;
                insertion = R[index] << 1;
                index += 1;
                match = oldR[index] << 1;
                match |= curBitmask;
                R[index] = deletion & substitution & insertion & match;

                set_tracebackMatrix_value(tracebackMatrix, i, d, 0, match);
                set_tracebackMatrix_value(tracebackMatrix, i, d, 1, substitution);
                set_tracebackMatrix_value(tracebackMatrix, i, d, 2, insertion);
                set_tracebackMatrix_value(tracebackMatrix, i, d, 3, deletion);
            }
        }
    }

    uint64_t max1 = 1ULL << (m - 1);

    *minError = -1;
    *mask = max1;

    for (int t = 0; t <= k; t++)
    {
        if ((R[t] & *mask) == 0)
        {
            *minError = t;
            break;
        }
    }
}
