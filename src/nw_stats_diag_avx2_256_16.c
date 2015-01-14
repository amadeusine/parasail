/**
 * @file
 *
 * @author jeff.daily@pnnl.gov
 *
 * Copyright (c) 2014 Battelle Memorial Institute.
 *
 * All rights reserved. No warranty, explicit or implicit, provided.
 */
#include "config.h"

#include <assert.h>
#include <stdlib.h>

#include <immintrin.h>

#include "parasail.h"
#include "parasail_internal.h"
#include "parasail_internal_avx.h"
#include "blosum/blosum_map.h"

#define NEG_INF_16 (INT16_MIN/(int16_t)(2))

/* avx2 does not have _mm256_cmplt_epi16, emulate it */
static inline __m256i _mm256_cmplt_epi16(__m256i a, __m256i b) {
    return _mm256_cmpgt_epi16(b,a);
}

#if HAVE_AVX2_MM256_INSERT_EPI16
#else
static inline __m256i _mm256_insert_epi16(__m256i a, int16_t b, int imm) {
    __m256i_16_t tmp;
    tmp.m = a;
    tmp.v[imm] = b;
    return tmp.m;
}
#endif

#if HAVE_AVX2_MM256_EXTRACT_EPI16
#else
static inline int16_t _mm256_extract_epi16(__m256i a, int imm) {
    __m256i_16_t tmp;
    tmp.m = a;
    return tmp.v[imm];
}
#endif

/* avx2 _mm256_srli_si256 does not shift across 128-bit lanes, emulate it */
static inline __m256i rshift16(__m256i a) {
    return _mm256_or_si256(
            _mm256_slli_si256(
                _mm256_permute2x128_si256(a, a, _MM_SHUFFLE(3,0,0,1)),
                14),
            _mm256_srli_si256(a, 2));
}

static inline __m256i lshift16(__m256i a) {
    return _mm256_alignr_epi8(a,
            _mm256_permute2x128_si256(a, a, _MM_SHUFFLE(0,0,3,0)),
            14);
}

/* shift given vector v, insert val, return shifted val */
static inline __m256i vshift16(const __m256i v, const int val)
{
    __m256i ret = rshift16(v);
    ret = _mm256_insert_epi16(ret, val, 15);
    return ret;
}

#ifdef PARASAIL_TABLE
static inline void arr_store_si256(
        int *array,
        __m256i vWscore,
        int i,
        int s1Len,
        int j,
        int s2Len)
{
    if (0 <= i+0 && i+0 < s1Len && 0 <= j-0 && j-0 < s2Len) {
        array[(i+0)*s2Len + (j-0)] = (int16_t)_mm256_extract_epi16(vWscore, 15);
    }
    if (0 <= i+1 && i+1 < s1Len && 0 <= j-1 && j-1 < s2Len) {
        array[(i+1)*s2Len + (j-1)] = (int16_t)_mm256_extract_epi16(vWscore, 14);
    }
    if (0 <= i+2 && i+2 < s1Len && 0 <= j-2 && j-2 < s2Len) {
        array[(i+2)*s2Len + (j-2)] = (int16_t)_mm256_extract_epi16(vWscore, 13);
    }
    if (0 <= i+3 && i+3 < s1Len && 0 <= j-3 && j-3 < s2Len) {
        array[(i+3)*s2Len + (j-3)] = (int16_t)_mm256_extract_epi16(vWscore, 12);
    }
    if (0 <= i+4 && i+4 < s1Len && 0 <= j-4 && j-4 < s2Len) {
        array[(i+4)*s2Len + (j-4)] = (int16_t)_mm256_extract_epi16(vWscore, 11);
    }
    if (0 <= i+5 && i+5 < s1Len && 0 <= j-5 && j-5 < s2Len) {
        array[(i+5)*s2Len + (j-5)] = (int16_t)_mm256_extract_epi16(vWscore, 10);
    }
    if (0 <= i+6 && i+6 < s1Len && 0 <= j-6 && j-6 < s2Len) {
        array[(i+6)*s2Len + (j-6)] = (int16_t)_mm256_extract_epi16(vWscore, 9);
    }
    if (0 <= i+7 && i+7 < s1Len && 0 <= j-7 && j-7 < s2Len) {
        array[(i+7)*s2Len + (j-7)] = (int16_t)_mm256_extract_epi16(vWscore, 8);
    }
    if (0 <= i+8 && i+8 < s1Len && 0 <= j-8 && j-8 < s2Len) {
        array[(i+8)*s2Len + (j-8)] = (int16_t)_mm256_extract_epi16(vWscore, 7);
    }
    if (0 <= i+9 && i+9 < s1Len && 0 <= j-9 && j-9 < s2Len) {
        array[(i+9)*s2Len + (j-9)] = (int16_t)_mm256_extract_epi16(vWscore, 6);
    }
    if (0 <= i+10 && i+10 < s1Len && 0 <= j-10 && j-10 < s2Len) {
        array[(i+10)*s2Len + (j-10)] = (int16_t)_mm256_extract_epi16(vWscore, 5);
    }
    if (0 <= i+11 && i+11 < s1Len && 0 <= j-11 && j-11 < s2Len) {
        array[(i+11)*s2Len + (j-11)] = (int16_t)_mm256_extract_epi16(vWscore, 4);
    }
    if (0 <= i+12 && i+12 < s1Len && 0 <= j-12 && j-12 < s2Len) {
        array[(i+12)*s2Len + (j-12)] = (int16_t)_mm256_extract_epi16(vWscore, 3);
    }
    if (0 <= i+13 && i+13 < s1Len && 0 <= j-13 && j-13 < s2Len) {
        array[(i+13)*s2Len + (j-13)] = (int16_t)_mm256_extract_epi16(vWscore, 2);
    }
    if (0 <= i+14 && i+14 < s1Len && 0 <= j-14 && j-14 < s2Len) {
        array[(i+14)*s2Len + (j-14)] = (int16_t)_mm256_extract_epi16(vWscore, 1);
    }
    if (0 <= i+15 && i+15 < s1Len && 0 <= j-15 && j-15 < s2Len) {
        array[(i+15)*s2Len + (j-15)] = (int16_t)_mm256_extract_epi16(vWscore, 0);
    }
}
#endif


#ifdef PARASAIL_TABLE
#define FNAME nw_stats_table_diag_avx2_256_16
#else
#define FNAME nw_stats_diag_avx2_256_16
#endif

parasail_result_t* FNAME(
        const char * const restrict _s1, const int s1Len,
        const char * const restrict _s2, const int s2Len,
        const int open, const int gap, const int matrix[24][24])
{
    const int N = 16; /* number of values in vector */
    const int PAD = N-1; /* N 16-bit values in vector, so N - 1 */
    const int PAD2 = PAD*2;
    int * const restrict s1      = parasail_memalign_int(32, s1Len+PAD);
    int * const restrict s2B     = parasail_memalign_int(32, s2Len+PAD2);
    int * const restrict _tbl_pr = parasail_memalign_int(32, s2Len+PAD2);
    int * const restrict _del_pr = parasail_memalign_int(32, s2Len+PAD2);
    int * const restrict _mch_pr = parasail_memalign_int(32, s2Len+PAD2);
    int * const restrict _len_pr = parasail_memalign_int(32, s2Len+PAD2);
    int * const restrict s2 = s2B+PAD; /* will allow later for negative indices */
    int * const restrict tbl_pr = _tbl_pr+PAD;
    int * const restrict del_pr = _del_pr+PAD;
    int * const restrict mch_pr = _mch_pr+PAD;
    int * const restrict len_pr = _len_pr+PAD;
#ifdef PARASAIL_TABLE
    parasail_result_t *result = parasail_result_new_table3(s1Len, s2Len);
#else
    parasail_result_t *result = parasail_result_new();
#endif
    int i = 0;
    int j = 0;
    int score = NEG_INF_16;
    int matches = NEG_INF_16;
    int length = NEG_INF_16;
    __m256i vNegInf = _mm256_set1_epi16(NEG_INF_16);
    __m256i vOpen = _mm256_set1_epi16(open);
    __m256i vGap  = _mm256_set1_epi16(gap);
    __m256i vZero = _mm256_set1_epi16(0);
    __m256i vOne = _mm256_set1_epi16(1);
    __m256i vN = _mm256_set1_epi16(N);
    __m256i vGapN = _mm256_set1_epi16(gap*N);
    __m256i vNegOne = _mm256_set1_epi16(-1);
    __m256i vI = _mm256_set_epi16(0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15);
    __m256i vJreset = _mm256_set_epi16(0,-1,-2,-3,-4,-5,-6,-7,-8,-9,-10,-11,-12,-13,-14,-15);
    __m256i vMaxScore = vNegInf;
    __m256i vMaxMatch = vNegInf;
    __m256i vMaxLength = vNegInf;
    __m256i vILimit = _mm256_set1_epi16(s1Len);
    __m256i vILimit1 = _mm256_sub_epi16(vILimit, vOne);
    __m256i vJLimit = _mm256_set1_epi16(s2Len);
    __m256i vJLimit1 = _mm256_sub_epi16(vJLimit, vOne);
    __m256i vIBoundary = _mm256_set_epi16(
            -open-0*gap,
            -open-1*gap,
            -open-2*gap,
            -open-3*gap,
            -open-4*gap,
            -open-5*gap,
            -open-6*gap,
            -open-7*gap,
            -open-8*gap,
            -open-9*gap,
            -open-10*gap,
            -open-11*gap,
            -open-12*gap,
            -open-13*gap,
            -open-14*gap,
            -open-15*gap);
    assert(s1Len > N);
    assert(s2Len > N);

    /* convert _s1 from char to int in range 0-23 */
    for (i=0; i<s1Len; ++i) {
        s1[i] = MAP_BLOSUM_[(unsigned char)_s1[i]];
    }
    /* pad back of s1 with dummy values */
    for (i=s1Len; i<s1Len+PAD; ++i) {
        s1[i] = 0; /* point to first matrix row because we don't care */
    }

    /* convert _s2 from char to int in range 0-23 */
    for (j=0; j<s2Len; ++j) {
        s2[j] = MAP_BLOSUM_[(unsigned char)_s2[j]];
    }
    /* pad front of s2 with dummy values */
    for (j=-PAD; j<0; ++j) {
        s2[j] = 0; /* point to first matrix row because we don't care */
    }
    /* pad back of s2 with dummy values */
    for (j=s2Len; j<s2Len+PAD; ++j) {
        s2[j] = 0; /* point to first matrix row because we don't care */
    }

    /* set initial values for stored row */
    for (j=0; j<s2Len; ++j) {
        tbl_pr[j] = -open - j*gap;
        del_pr[j] = NEG_INF_16;
        mch_pr[j] = 0;
        len_pr[j] = 0;
    }
    /* pad front of stored row values */
    for (j=-PAD; j<0; ++j) {
        tbl_pr[j] = NEG_INF_16;
        del_pr[j] = NEG_INF_16;
        mch_pr[j] = 0;
        len_pr[j] = 0;
    }
    /* pad back of stored row values */
    for (j=s2Len; j<s2Len+PAD; ++j) {
        tbl_pr[j] = NEG_INF_16;
        del_pr[j] = NEG_INF_16;
        mch_pr[j] = 0;
        len_pr[j] = 0;
    }
    tbl_pr[-1] = 0; /* upper left corner */

    /* iterate over query sequence */
    for (i=0; i<s1Len-N; i+=N) {
        __m256i vNscore = vNegInf;
        __m256i vNmatch = vZero;
        __m256i vNlength = vZero;
        __m256i vWscore = vNegInf;
        __m256i vWmatch = vZero;
        __m256i vWlength = vZero;
        __m256i vIns = vNegInf;
        __m256i vDel = vNegInf;
        __m256i vJ = vJreset;
        __m256i vs1 = _mm256_set_epi16(
                s1[i+0],
                s1[i+1],
                s1[i+2],
                s1[i+3],
                s1[i+4],
                s1[i+5],
                s1[i+6],
                s1[i+7],
                s1[i+8],
                s1[i+9],
                s1[i+10],
                s1[i+11],
                s1[i+12],
                s1[i+13],
                s1[i+14],
                s1[i+15]
                );
        __m256i vs2 = vNegInf;
        const int * const restrict matrow0 = matrix[s1[i+0]];
        const int * const restrict matrow1 = matrix[s1[i+1]];
        const int * const restrict matrow2 = matrix[s1[i+2]];
        const int * const restrict matrow3 = matrix[s1[i+3]];
        const int * const restrict matrow4 = matrix[s1[i+4]];
        const int * const restrict matrow5 = matrix[s1[i+5]];
        const int * const restrict matrow6 = matrix[s1[i+6]];
        const int * const restrict matrow7 = matrix[s1[i+7]];
        const int * const restrict matrow8 = matrix[s1[i+8]];
        const int * const restrict matrow9 = matrix[s1[i+9]];
        const int * const restrict matrow10 = matrix[s1[i+10]];
        const int * const restrict matrow11 = matrix[s1[i+11]];
        const int * const restrict matrow12 = matrix[s1[i+12]];
        const int * const restrict matrow13 = matrix[s1[i+13]];
        const int * const restrict matrow14 = matrix[s1[i+14]];
        const int * const restrict matrow15 = matrix[s1[i+15]];
        vNscore = vshift16(vNscore, tbl_pr[-1]);
        vNmatch = vshift16(vNmatch, 0);
        vNlength = vshift16(vNlength, 0);
        vWscore = vshift16(vWscore, -open - i*gap);
        vWmatch = vshift16(vWmatch, 0);
        vWlength = vshift16(vWlength, 0);
        tbl_pr[-1] = -open - (i+N)*gap;
        /* iterate over database sequence */
        for (j=0; j<N; ++j) {
            __m256i vMat;
            __m256i vNWscore = vNscore;
            __m256i vNWmatch = vNmatch;
            __m256i vNWlength = vNlength;
            vNscore = vshift16(vWscore, tbl_pr[j]);
            vNmatch = vshift16(vWmatch, mch_pr[j]);
            vNlength = vshift16(vWlength, len_pr[j]);
            vDel = vshift16(vDel, del_pr[j]);
            vDel = _mm256_max_epi16(
                    _mm256_sub_epi16(vNscore, vOpen),
                    _mm256_sub_epi16(vDel, vGap));
            vIns = _mm256_max_epi16(
                    _mm256_sub_epi16(vWscore, vOpen),
                    _mm256_sub_epi16(vIns, vGap));
            vs2 = vshift16(vs2, s2[j]);
            vMat = _mm256_set_epi16(
                    matrow0[s2[j-0]],
                    matrow1[s2[j-1]],
                    matrow2[s2[j-2]],
                    matrow3[s2[j-3]],
                    matrow4[s2[j-4]],
                    matrow5[s2[j-5]],
                    matrow6[s2[j-6]],
                    matrow7[s2[j-7]],
                    matrow8[s2[j-8]],
                    matrow9[s2[j-9]],
                    matrow10[s2[j-10]],
                    matrow11[s2[j-11]],
                    matrow12[s2[j-12]],
                    matrow13[s2[j-13]],
                    matrow14[s2[j-14]],
                    matrow15[s2[j-15]]
                    );
            vNWscore = _mm256_add_epi16(vNWscore, vMat);
            vWscore = _mm256_max_epi16(vNWscore, vIns);
            vWscore = _mm256_max_epi16(vWscore, vDel);
            /* conditional block */
            {
                __m256i case1not;
                __m256i case2not;
                __m256i case2;
                __m256i case3;
                __m256i vCmatch;
                __m256i vClength;
                case1not = _mm256_or_si256(
                        _mm256_cmplt_epi16(vNWscore,vDel),
                        _mm256_cmplt_epi16(vNWscore,vIns));
                case2not = _mm256_cmplt_epi16(vDel,vIns);
                case2 = _mm256_andnot_si256(case2not,case1not);
                case3 = _mm256_and_si256(case1not,case2not);
                vCmatch = _mm256_andnot_si256(case1not,
                        _mm256_add_epi16(vNWmatch, _mm256_and_si256(
                                _mm256_cmpeq_epi16(vs1,vs2),vOne)));
                vClength= _mm256_andnot_si256(case1not,
                        _mm256_add_epi16(vNWlength, vOne));
                vCmatch = _mm256_or_si256(vCmatch, _mm256_and_si256(case2, vNmatch));
                vClength= _mm256_or_si256(vClength,_mm256_and_si256(case2,
                            _mm256_add_epi16(vNlength, vOne)));
                vCmatch = _mm256_or_si256(vCmatch, _mm256_and_si256(case3, vWmatch));
                vClength= _mm256_or_si256(vClength,_mm256_and_si256(case3,
                            _mm256_add_epi16(vWlength, vOne)));
                vWmatch = vCmatch;
                vWlength = vClength;
            }

            /* as minor diagonal vector passes across the j=-1 boundary,
             * assign the appropriate boundary conditions */
            {
                __m256i cond = _mm256_cmpeq_epi16(vJ,vNegOne);
                vWscore = _mm256_blendv_epi8(vWscore, vIBoundary, cond);
                vWmatch = _mm256_andnot_si256(cond, vWmatch);
                vWlength = _mm256_andnot_si256(cond, vWlength);
                vDel = _mm256_blendv_epi8(vDel, vNegInf, cond);
                vIns = _mm256_blendv_epi8(vIns, vNegInf, cond);
            }
#ifdef PARASAIL_TABLE
            arr_store_si256(result->score_table, vWscore, i, s1Len, j, s2Len);
            arr_store_si256(result->matches_table, vWmatch, i, s1Len, j, s2Len);
            arr_store_si256(result->length_table, vWlength, i, s1Len, j, s2Len);
#endif
            tbl_pr[j-15] = (int16_t)_mm256_extract_epi16(vWscore,0);
            mch_pr[j-15] = (int16_t)_mm256_extract_epi16(vWmatch,0);
            len_pr[j-15] = (int16_t)_mm256_extract_epi16(vWlength,0);
            del_pr[j-15] = (int16_t)_mm256_extract_epi16(vDel,0);
            vJ = _mm256_add_epi16(vJ, vOne);
        }
        for (j=N; j<s2Len+PAD; ++j) {
            __m256i vMat;
            __m256i vNWscore = vNscore;
            __m256i vNWmatch = vNmatch;
            __m256i vNWlength = vNlength;
            vNscore = vshift16(vWscore, tbl_pr[j]);
            vNmatch = vshift16(vWmatch, mch_pr[j]);
            vNlength = vshift16(vWlength, len_pr[j]);
            vDel = vshift16(vDel, del_pr[j]);
            vDel = _mm256_max_epi16(
                    _mm256_sub_epi16(vNscore, vOpen),
                    _mm256_sub_epi16(vDel, vGap));
            vIns = _mm256_max_epi16(
                    _mm256_sub_epi16(vWscore, vOpen),
                    _mm256_sub_epi16(vIns, vGap));
            vs2 = vshift16(vs2, s2[j]);
            vMat = _mm256_set_epi16(
                    matrow0[s2[j-0]],
                    matrow1[s2[j-1]],
                    matrow2[s2[j-2]],
                    matrow3[s2[j-3]],
                    matrow4[s2[j-4]],
                    matrow5[s2[j-5]],
                    matrow6[s2[j-6]],
                    matrow7[s2[j-7]],
                    matrow8[s2[j-8]],
                    matrow9[s2[j-9]],
                    matrow10[s2[j-10]],
                    matrow11[s2[j-11]],
                    matrow12[s2[j-12]],
                    matrow13[s2[j-13]],
                    matrow14[s2[j-14]],
                    matrow15[s2[j-15]]
                    );
            vNWscore = _mm256_add_epi16(vNWscore, vMat);
            vWscore = _mm256_max_epi16(vNWscore, vIns);
            vWscore = _mm256_max_epi16(vWscore, vDel);
            /* conditional block */
            {
                __m256i case1not;
                __m256i case2not;
                __m256i case2;
                __m256i case3;
                __m256i vCmatch;
                __m256i vClength;
                case1not = _mm256_or_si256(
                        _mm256_cmplt_epi16(vNWscore,vDel),
                        _mm256_cmplt_epi16(vNWscore,vIns));
                case2not = _mm256_cmplt_epi16(vDel,vIns);
                case2 = _mm256_andnot_si256(case2not,case1not);
                case3 = _mm256_and_si256(case1not,case2not);
                vCmatch = _mm256_andnot_si256(case1not,
                        _mm256_add_epi16(vNWmatch, _mm256_and_si256(
                                _mm256_cmpeq_epi16(vs1,vs2),vOne)));
                vClength= _mm256_andnot_si256(case1not,
                        _mm256_add_epi16(vNWlength, vOne));
                vCmatch = _mm256_or_si256(vCmatch, _mm256_and_si256(case2, vNmatch));
                vClength= _mm256_or_si256(vClength,_mm256_and_si256(case2,
                            _mm256_add_epi16(vNlength, vOne)));
                vCmatch = _mm256_or_si256(vCmatch, _mm256_and_si256(case3, vWmatch));
                vClength= _mm256_or_si256(vClength,_mm256_and_si256(case3,
                            _mm256_add_epi16(vWlength, vOne)));
                vWmatch = vCmatch;
                vWlength = vClength;
            }
#ifdef PARASAIL_TABLE
            arr_store_si256(result->score_table, vWscore, i, s1Len, j, s2Len);
            arr_store_si256(result->matches_table, vWmatch, i, s1Len, j, s2Len);
            arr_store_si256(result->length_table, vWlength, i, s1Len, j, s2Len);
#endif
            tbl_pr[j-15] = (int16_t)_mm256_extract_epi16(vWscore,0);
            mch_pr[j-15] = (int16_t)_mm256_extract_epi16(vWmatch,0);
            len_pr[j-15] = (int16_t)_mm256_extract_epi16(vWlength,0);
            del_pr[j-15] = (int16_t)_mm256_extract_epi16(vDel,0);
            vJ = _mm256_add_epi16(vJ, vOne);
        }
        vI = _mm256_add_epi16(vI, vN);
        vIBoundary = _mm256_sub_epi16(vIBoundary, vGapN);
    }
    for (/*i=?*/; i<s1Len; i+=N) {
        __m256i vNscore = vNegInf;
        __m256i vNmatch = vZero;
        __m256i vNlength = vZero;
        __m256i vWscore = vNegInf;
        __m256i vWmatch = vZero;
        __m256i vWlength = vZero;
        __m256i vIns = vNegInf;
        __m256i vDel = vNegInf;
        __m256i vJ = vJreset;
        __m256i vs1 = _mm256_set_epi16(
                s1[i+0],
                s1[i+1],
                s1[i+2],
                s1[i+3],
                s1[i+4],
                s1[i+5],
                s1[i+6],
                s1[i+7],
                s1[i+8],
                s1[i+9],
                s1[i+10],
                s1[i+11],
                s1[i+12],
                s1[i+13],
                s1[i+14],
                s1[i+15]);
        __m256i vs2 = vNegInf;
        const int * const restrict matrow0 = matrix[s1[i+0]];
        const int * const restrict matrow1 = matrix[s1[i+1]];
        const int * const restrict matrow2 = matrix[s1[i+2]];
        const int * const restrict matrow3 = matrix[s1[i+3]];
        const int * const restrict matrow4 = matrix[s1[i+4]];
        const int * const restrict matrow5 = matrix[s1[i+5]];
        const int * const restrict matrow6 = matrix[s1[i+6]];
        const int * const restrict matrow7 = matrix[s1[i+7]];
        const int * const restrict matrow8 = matrix[s1[i+8]];
        const int * const restrict matrow9 = matrix[s1[i+9]];
        const int * const restrict matrow10 = matrix[s1[i+10]];
        const int * const restrict matrow11 = matrix[s1[i+11]];
        const int * const restrict matrow12 = matrix[s1[i+12]];
        const int * const restrict matrow13 = matrix[s1[i+13]];
        const int * const restrict matrow14 = matrix[s1[i+14]];
        const int * const restrict matrow15 = matrix[s1[i+15]];
        vNscore = vshift16(vNscore, tbl_pr[-1]);
        vNmatch = vshift16(vNmatch, 0);
        vNlength = vshift16(vNlength, 0);
        vWscore = vshift16(vWscore, -open - i*gap);
        vWmatch = vshift16(vWmatch, 0);
        vWlength = vshift16(vWlength, 0);
        tbl_pr[-1] = -open - (i+N)*gap;
        /* iterate over database sequence */
        for (j=0; j<N; ++j) {
            __m256i vMat;
            __m256i vNWscore = vNscore;
            __m256i vNWmatch = vNmatch;
            __m256i vNWlength = vNlength;
            vNscore = vshift16(vWscore, tbl_pr[j]);
            vNmatch = vshift16(vWmatch, mch_pr[j]);
            vNlength = vshift16(vWlength, len_pr[j]);
            vDel = vshift16(vDel, del_pr[j]);
            vDel = _mm256_max_epi16(
                    _mm256_sub_epi16(vNscore, vOpen),
                    _mm256_sub_epi16(vDel, vGap));
            vIns = _mm256_max_epi16(
                    _mm256_sub_epi16(vWscore, vOpen),
                    _mm256_sub_epi16(vIns, vGap));
            vs2 = vshift16(vs2, s2[j]);
            vMat = _mm256_set_epi16(
                    matrow0[s2[j-0]],
                    matrow1[s2[j-1]],
                    matrow2[s2[j-2]],
                    matrow3[s2[j-3]],
                    matrow4[s2[j-4]],
                    matrow5[s2[j-5]],
                    matrow6[s2[j-6]],
                    matrow7[s2[j-7]],
                    matrow8[s2[j-8]],
                    matrow9[s2[j-9]],
                    matrow10[s2[j-10]],
                    matrow11[s2[j-11]],
                    matrow12[s2[j-12]],
                    matrow13[s2[j-13]],
                    matrow14[s2[j-14]],
                    matrow15[s2[j-15]]
                    );
            vNWscore = _mm256_add_epi16(vNWscore, vMat);
            vWscore = _mm256_max_epi16(vNWscore, vIns);
            vWscore = _mm256_max_epi16(vWscore, vDel);
            /* conditional block */
            {
                __m256i case1not;
                __m256i case2not;
                __m256i case2;
                __m256i case3;
                __m256i vCmatch;
                __m256i vClength;
                case1not = _mm256_or_si256(
                        _mm256_cmplt_epi16(vNWscore,vDel),
                        _mm256_cmplt_epi16(vNWscore,vIns));
                case2not = _mm256_cmplt_epi16(vDel,vIns);
                case2 = _mm256_andnot_si256(case2not,case1not);
                case3 = _mm256_and_si256(case1not,case2not);
                vCmatch = _mm256_andnot_si256(case1not,
                        _mm256_add_epi16(vNWmatch, _mm256_and_si256(
                                _mm256_cmpeq_epi16(vs1,vs2),vOne)));
                vClength= _mm256_andnot_si256(case1not,
                        _mm256_add_epi16(vNWlength, vOne));
                vCmatch = _mm256_or_si256(vCmatch, _mm256_and_si256(case2, vNmatch));
                vClength= _mm256_or_si256(vClength,_mm256_and_si256(case2,
                            _mm256_add_epi16(vNlength, vOne)));
                vCmatch = _mm256_or_si256(vCmatch, _mm256_and_si256(case3, vWmatch));
                vClength= _mm256_or_si256(vClength,_mm256_and_si256(case3,
                            _mm256_add_epi16(vWlength, vOne)));
                vWmatch = vCmatch;
                vWlength = vClength;
            }

            /* as minor diagonal vector passes across the j=-1 boundary,
             * assign the appropriate boundary conditions */
            {
                __m256i cond = _mm256_cmpeq_epi16(vJ,vNegOne);
                vWscore = _mm256_blendv_epi8(vWscore, vIBoundary, cond);
                vWmatch = _mm256_andnot_si256(cond, vWmatch);
                vWlength = _mm256_andnot_si256(cond, vWlength);
                vDel = _mm256_blendv_epi8(vDel, vNegInf, cond);
                vIns = _mm256_blendv_epi8(vIns, vNegInf, cond);
            }
#ifdef PARASAIL_TABLE
            arr_store_si256(result->score_table, vWscore, i, s1Len, j, s2Len);
            arr_store_si256(result->matches_table, vWmatch, i, s1Len, j, s2Len);
            arr_store_si256(result->length_table, vWlength, i, s1Len, j, s2Len);
#endif
            tbl_pr[j-15] = (int16_t)_mm256_extract_epi16(vWscore,0);
            mch_pr[j-15] = (int16_t)_mm256_extract_epi16(vWmatch,0);
            len_pr[j-15] = (int16_t)_mm256_extract_epi16(vWlength,0);
            del_pr[j-15] = (int16_t)_mm256_extract_epi16(vDel,0);
            vJ = _mm256_add_epi16(vJ, vOne);
        }
        for (j=N; j<s2Len-1; ++j) {
            __m256i vMat;
            __m256i vNWscore = vNscore;
            __m256i vNWmatch = vNmatch;
            __m256i vNWlength = vNlength;
            vNscore = vshift16(vWscore, tbl_pr[j]);
            vNmatch = vshift16(vWmatch, mch_pr[j]);
            vNlength = vshift16(vWlength, len_pr[j]);
            vDel = vshift16(vDel, del_pr[j]);
            vDel = _mm256_max_epi16(
                    _mm256_sub_epi16(vNscore, vOpen),
                    _mm256_sub_epi16(vDel, vGap));
            vIns = _mm256_max_epi16(
                    _mm256_sub_epi16(vWscore, vOpen),
                    _mm256_sub_epi16(vIns, vGap));
            vs2 = vshift16(vs2, s2[j]);
            vMat = _mm256_set_epi16(
                    matrow0[s2[j-0]],
                    matrow1[s2[j-1]],
                    matrow2[s2[j-2]],
                    matrow3[s2[j-3]],
                    matrow4[s2[j-4]],
                    matrow5[s2[j-5]],
                    matrow6[s2[j-6]],
                    matrow7[s2[j-7]],
                    matrow8[s2[j-8]],
                    matrow9[s2[j-9]],
                    matrow10[s2[j-10]],
                    matrow11[s2[j-11]],
                    matrow12[s2[j-12]],
                    matrow13[s2[j-13]],
                    matrow14[s2[j-14]],
                    matrow15[s2[j-15]]
                    );
            vNWscore = _mm256_add_epi16(vNWscore, vMat);
            vWscore = _mm256_max_epi16(vNWscore, vIns);
            vWscore = _mm256_max_epi16(vWscore, vDel);
            /* conditional block */
            {
                __m256i case1not;
                __m256i case2not;
                __m256i case2;
                __m256i case3;
                __m256i vCmatch;
                __m256i vClength;
                case1not = _mm256_or_si256(
                        _mm256_cmplt_epi16(vNWscore,vDel),
                        _mm256_cmplt_epi16(vNWscore,vIns));
                case2not = _mm256_cmplt_epi16(vDel,vIns);
                case2 = _mm256_andnot_si256(case2not,case1not);
                case3 = _mm256_and_si256(case1not,case2not);
                vCmatch = _mm256_andnot_si256(case1not,
                        _mm256_add_epi16(vNWmatch, _mm256_and_si256(
                                _mm256_cmpeq_epi16(vs1,vs2),vOne)));
                vClength= _mm256_andnot_si256(case1not,
                        _mm256_add_epi16(vNWlength, vOne));
                vCmatch = _mm256_or_si256(vCmatch, _mm256_and_si256(case2, vNmatch));
                vClength= _mm256_or_si256(vClength,_mm256_and_si256(case2,
                            _mm256_add_epi16(vNlength, vOne)));
                vCmatch = _mm256_or_si256(vCmatch, _mm256_and_si256(case3, vWmatch));
                vClength= _mm256_or_si256(vClength,_mm256_and_si256(case3,
                            _mm256_add_epi16(vWlength, vOne)));
                vWmatch = vCmatch;
                vWlength = vClength;
            }
#ifdef PARASAIL_TABLE
            arr_store_si256(result->score_table, vWscore, i, s1Len, j, s2Len);
            arr_store_si256(result->matches_table, vWmatch, i, s1Len, j, s2Len);
            arr_store_si256(result->length_table, vWlength, i, s1Len, j, s2Len);
#endif
            tbl_pr[j-15] = (int16_t)_mm256_extract_epi16(vWscore,0);
            mch_pr[j-15] = (int16_t)_mm256_extract_epi16(vWmatch,0);
            len_pr[j-15] = (int16_t)_mm256_extract_epi16(vWlength,0);
            del_pr[j-15] = (int16_t)_mm256_extract_epi16(vDel,0);
            vJ = _mm256_add_epi16(vJ, vOne);
        }
        for (j=s2Len-1; j<s2Len+PAD; ++j) {
            __m256i vMat;
            __m256i vNWscore = vNscore;
            __m256i vNWmatch = vNmatch;
            __m256i vNWlength = vNlength;
            vNscore = vshift16(vWscore, tbl_pr[j]);
            vNmatch = vshift16(vWmatch, mch_pr[j]);
            vNlength = vshift16(vWlength, len_pr[j]);
            vDel = vshift16(vDel, del_pr[j]);
            vDel = _mm256_max_epi16(
                    _mm256_sub_epi16(vNscore, vOpen),
                    _mm256_sub_epi16(vDel, vGap));
            vIns = _mm256_max_epi16(
                    _mm256_sub_epi16(vWscore, vOpen),
                    _mm256_sub_epi16(vIns, vGap));
            vs2 = vshift16(vs2, s2[j]);
            vMat = _mm256_set_epi16(
                    matrow0[s2[j-0]],
                    matrow1[s2[j-1]],
                    matrow2[s2[j-2]],
                    matrow3[s2[j-3]],
                    matrow4[s2[j-4]],
                    matrow5[s2[j-5]],
                    matrow6[s2[j-6]],
                    matrow7[s2[j-7]],
                    matrow8[s2[j-8]],
                    matrow9[s2[j-9]],
                    matrow10[s2[j-10]],
                    matrow11[s2[j-11]],
                    matrow12[s2[j-12]],
                    matrow13[s2[j-13]],
                    matrow14[s2[j-14]],
                    matrow15[s2[j-15]]
                    );
            vNWscore = _mm256_add_epi16(vNWscore, vMat);
            vWscore = _mm256_max_epi16(vNWscore, vIns);
            vWscore = _mm256_max_epi16(vWscore, vDel);
            /* conditional block */
            {
                __m256i case1not;
                __m256i case2not;
                __m256i case2;
                __m256i case3;
                __m256i vCmatch;
                __m256i vClength;
                case1not = _mm256_or_si256(
                        _mm256_cmplt_epi16(vNWscore,vDel),
                        _mm256_cmplt_epi16(vNWscore,vIns));
                case2not = _mm256_cmplt_epi16(vDel,vIns);
                case2 = _mm256_andnot_si256(case2not,case1not);
                case3 = _mm256_and_si256(case1not,case2not);
                vCmatch = _mm256_andnot_si256(case1not,
                        _mm256_add_epi16(vNWmatch, _mm256_and_si256(
                                _mm256_cmpeq_epi16(vs1,vs2),vOne)));
                vClength= _mm256_andnot_si256(case1not,
                        _mm256_add_epi16(vNWlength, vOne));
                vCmatch = _mm256_or_si256(vCmatch, _mm256_and_si256(case2, vNmatch));
                vClength= _mm256_or_si256(vClength,_mm256_and_si256(case2,
                            _mm256_add_epi16(vNlength, vOne)));
                vCmatch = _mm256_or_si256(vCmatch, _mm256_and_si256(case3, vWmatch));
                vClength= _mm256_or_si256(vClength,_mm256_and_si256(case3,
                            _mm256_add_epi16(vWlength, vOne)));
                vWmatch = vCmatch;
                vWlength = vClength;
            }
#ifdef PARASAIL_TABLE
            arr_store_si256(result->score_table, vWscore, i, s1Len, j, s2Len);
            arr_store_si256(result->matches_table, vWmatch, i, s1Len, j, s2Len);
            arr_store_si256(result->length_table, vWlength, i, s1Len, j, s2Len);
#endif
            tbl_pr[j-15] = (int16_t)_mm256_extract_epi16(vWscore,0);
            mch_pr[j-15] = (int16_t)_mm256_extract_epi16(vWmatch,0);
            len_pr[j-15] = (int16_t)_mm256_extract_epi16(vWlength,0);
            del_pr[j-15] = (int16_t)_mm256_extract_epi16(vDel,0);
            /* as minor diagonal vector passes across table, extract
               last table value at the i,j bound */
            {
                __m256i cond_valid_I = _mm256_cmpeq_epi16(vI, vILimit1);
                __m256i cond_valid_J = _mm256_cmpeq_epi16(vJ, vJLimit1);
                __m256i cond_max = _mm256_cmpgt_epi16(vWscore, vMaxScore);
                __m256i cond_all = _mm256_and_si256(cond_max,
                        _mm256_and_si256(cond_valid_I, cond_valid_J));
                vMaxScore = _mm256_blendv_epi8(vMaxScore, vWscore, cond_all);
                vMaxMatch = _mm256_blendv_epi8(vMaxMatch, vWmatch, cond_all);
                vMaxLength = _mm256_blendv_epi8(vMaxLength, vWlength, cond_all);
            }
            vJ = _mm256_add_epi16(vJ, vOne);
        }
        vI = _mm256_add_epi16(vI, vN);
        vIBoundary = _mm256_sub_epi16(vIBoundary, vGapN);
    }

    /* max in vMaxScore */
    for (i=0; i<N; ++i) {
        int16_t value;
        value = (int16_t) _mm256_extract_epi16(vMaxScore, 15);
        if (value > score) {
            score = value;
            matches = (int16_t) _mm256_extract_epi16(vMaxMatch, 15);
            length= (int16_t) _mm256_extract_epi16(vMaxLength,15);
        }
        vMaxScore = lshift16(vMaxScore);
        vMaxMatch = lshift16(vMaxMatch);
        vMaxLength = lshift16(vMaxLength);
    }

    result->score = score;
    result->matches = matches;
    result->length = length;

    parasail_free(_len_pr);
    parasail_free(_mch_pr);
    parasail_free(_del_pr);
    parasail_free(_tbl_pr);
    parasail_free(s2B);
    parasail_free(s1);

    return result;
}
