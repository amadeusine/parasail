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

#include <stdint.h>
#include <stdlib.h>

#include <emmintrin.h>
#include <smmintrin.h>

#include "parasail.h"
#include "parasail_internal.h"
#include "parasail_internal_sse.h"
#include "blosum/blosum_map.h"

#define NEG_INF_8 (INT8_MIN)
#define MAX(a,b) ((a)>(b)?(a):(b))

#ifdef PARASAIL_TABLE
static inline void arr_store_si128(
        int *array,
        __m128i vH,
        int32_t t,
        int32_t seglen,
        int32_t d,
        int32_t dlen)
{
    array[(0*seglen+t)*dlen + d] = (int8_t)_mm_extract_epi8(vH, 0);
    array[(1*seglen+t)*dlen + d] = (int8_t)_mm_extract_epi8(vH, 1);
    array[(2*seglen+t)*dlen + d] = (int8_t)_mm_extract_epi8(vH, 2);
    array[(3*seglen+t)*dlen + d] = (int8_t)_mm_extract_epi8(vH, 3);
    array[(4*seglen+t)*dlen + d] = (int8_t)_mm_extract_epi8(vH, 4);
    array[(5*seglen+t)*dlen + d] = (int8_t)_mm_extract_epi8(vH, 5);
    array[(6*seglen+t)*dlen + d] = (int8_t)_mm_extract_epi8(vH, 6);
    array[(7*seglen+t)*dlen + d] = (int8_t)_mm_extract_epi8(vH, 7);
    array[(8*seglen+t)*dlen + d] = (int8_t)_mm_extract_epi8(vH, 8);
    array[(9*seglen+t)*dlen + d] = (int8_t)_mm_extract_epi8(vH, 9);
    array[(10*seglen+t)*dlen + d] = (int8_t)_mm_extract_epi8(vH, 10);
    array[(11*seglen+t)*dlen + d] = (int8_t)_mm_extract_epi8(vH, 11);
    array[(12*seglen+t)*dlen + d] = (int8_t)_mm_extract_epi8(vH, 12);
    array[(13*seglen+t)*dlen + d] = (int8_t)_mm_extract_epi8(vH, 13);
    array[(14*seglen+t)*dlen + d] = (int8_t)_mm_extract_epi8(vH, 14);
    array[(15*seglen+t)*dlen + d] = (int8_t)_mm_extract_epi8(vH, 15);

}

static inline void arr_store_si128_bias(
        int *array,
        __m128i vH,
        int32_t t,
        int32_t seglen,
        int32_t d,
        int32_t dlen,
        int32_t bias)
{
    array[(0*seglen+t)*dlen + d] = bias + (int8_t)_mm_extract_epi8(vH, 0);
    array[(1*seglen+t)*dlen + d] = bias + (int8_t)_mm_extract_epi8(vH, 1);
    array[(2*seglen+t)*dlen + d] = bias + (int8_t)_mm_extract_epi8(vH, 2);
    array[(3*seglen+t)*dlen + d] = bias + (int8_t)_mm_extract_epi8(vH, 3);
    array[(4*seglen+t)*dlen + d] = bias + (int8_t)_mm_extract_epi8(vH, 4);
    array[(5*seglen+t)*dlen + d] = bias + (int8_t)_mm_extract_epi8(vH, 5);
    array[(6*seglen+t)*dlen + d] = bias + (int8_t)_mm_extract_epi8(vH, 6);
    array[(7*seglen+t)*dlen + d] = bias + (int8_t)_mm_extract_epi8(vH, 7);
    array[(8*seglen+t)*dlen + d] = bias + (int8_t)_mm_extract_epi8(vH, 8);
    array[(9*seglen+t)*dlen + d] = bias + (int8_t)_mm_extract_epi8(vH, 9);
    array[(10*seglen+t)*dlen + d] = bias + (int8_t)_mm_extract_epi8(vH, 10);
    array[(11*seglen+t)*dlen + d] = bias + (int8_t)_mm_extract_epi8(vH, 11);
    array[(12*seglen+t)*dlen + d] = bias + (int8_t)_mm_extract_epi8(vH, 12);
    array[(13*seglen+t)*dlen + d] = bias + (int8_t)_mm_extract_epi8(vH, 13);
    array[(14*seglen+t)*dlen + d] = bias + (int8_t)_mm_extract_epi8(vH, 14);
    array[(15*seglen+t)*dlen + d] = bias + (int8_t)_mm_extract_epi8(vH, 15);
}
#endif

#ifdef PARASAIL_TABLE
#define FNAME sw_stats_table_scan_sse41_128_8
#else
#define FNAME sw_stats_scan_sse41_128_8
#endif

parasail_result_t* FNAME(
        const char * const restrict s1, const int s1Len,
        const char * const restrict s2, const int s2Len,
        const int open, const int gap, const int matrix[24][24])
{
    int32_t i = 0;
    int32_t j = 0;
    int32_t k = 0;
    const int32_t n = 24; /* number of amino acids in table */
    const int32_t segWidth = 16; /* number of values in vector unit */
    int32_t segNum = 0;
    const int32_t segLen = (s1Len + segWidth - 1) / segWidth;
    __m128i* const restrict pvP  = parasail_memalign_m128i(16, n * segLen);
    __m128i* const restrict pvPm = parasail_memalign_m128i(16, n * segLen);
    __m128i* const restrict pvPs = parasail_memalign_m128i(16, n * segLen);
    __m128i* const restrict pvE  = parasail_memalign_m128i(16, segLen);
    __m128i* const restrict pvHt = parasail_memalign_m128i(16, segLen);
    __m128i* const restrict pvFt = parasail_memalign_m128i(16, segLen);
    __m128i* const restrict pvMt = parasail_memalign_m128i(16, segLen);
    __m128i* const restrict pvSt = parasail_memalign_m128i(16, segLen);
    __m128i* const restrict pvLt = parasail_memalign_m128i(16, segLen);
    __m128i* const restrict pvEx = parasail_memalign_m128i(16, segLen);
    __m128i* const restrict pvH  = parasail_memalign_m128i(16, segLen);
    __m128i* const restrict pvM  = parasail_memalign_m128i(16, segLen);
    __m128i* const restrict pvS  = parasail_memalign_m128i(16, segLen);
    __m128i* const restrict pvL  = parasail_memalign_m128i(16, segLen);
    __m128i vGapO = _mm_set1_epi8(open);
    __m128i vGapE = _mm_set1_epi8(gap);
    __m128i vZero = _mm_setzero_si128();
    __m128i vOne = _mm_set1_epi8(1);
    int8_t bias = 127;
    int ibias = 127;
    __m128i vNegBias = _mm_set1_epi8(-bias);
    __m128i vSaturationCheck = _mm_setzero_si128();
    __m128i vNegLimit = _mm_set1_epi8(INT8_MIN);
    __m128i vPosLimit = _mm_set1_epi8(INT8_MAX);
    int8_t score = NEG_INF_8;
    int matches = 0;
    int similar = 0;
    int length = 0;
    __m128i vMaxH = vZero;
    __m128i vMaxM = vZero;
    __m128i vMaxS = vZero;
    __m128i vMaxL = vZero;
#ifdef PARASAIL_TABLE
    parasail_result_t *result = parasail_result_new_table3(segLen*segWidth, s2Len);
#else
    parasail_result_t *result = parasail_result_new();
#endif

    /* Generate query profile and match profile.
     * Rearrange query sequence & calculate the weight of match/mismatch.
     * Don't alias. */
    {
        int32_t index = 0;
        for (k=0; k<n; ++k) {
            for (i=0; i<segLen; ++i) {
                __m128i_8_t p;
                __m128i_8_t m;
                __m128i_8_t s;
                j = i;
                for (segNum=0; segNum<segWidth; ++segNum) {
                    p.v[segNum] = j >= s1Len ? 0 : matrix[k][MAP_BLOSUM_[(unsigned char)s1[j]]];
                    m.v[segNum] = j >= s1Len ? 0 : (k == MAP_BLOSUM_[(unsigned char)s1[j]]);
                    s.v[segNum] = p.v[segNum] > 0;
                    j += segLen;
                }
                _mm_store_si128(&pvP[index], p.m);
                _mm_store_si128(&pvPm[index], m.m);
                _mm_store_si128(&pvPs[index], s.m);
                ++index;
            }
        }
    }

    /* initialize H E M L */
    {
        int32_t index = 0;
        for (i=0; i<segLen; ++i) {
            __m128i_8_t h;
            __m128i_8_t e;
            __m128i_8_t m;
            __m128i_8_t l;
            for (segNum=0; segNum<segWidth; ++segNum) {
                h.v[segNum] = 0;
                e.v[segNum] = NEG_INF_8;
                m.v[segNum] = -bias;
                l.v[segNum] = -bias;
            }
            _mm_store_si128(&pvH[index], h.m);
            _mm_store_si128(&pvE[index], e.m);
            _mm_store_si128(&pvM[index], m.m);
            _mm_store_si128(&pvS[index], m.m);
            _mm_store_si128(&pvL[index], l.m);
            ++index;
        }
    }

    /* outer loop over database sequence */
    for (j=0; j<s2Len; ++j) {
        __m128i vE;
        __m128i vHt;
        __m128i vFt;
        __m128i vH;
        __m128i *pvW;
        __m128i vW;
        __m128i *pvC;
        __m128i *pvD;
        __m128i vC;
        __m128i vD;
        __m128i vM;
        __m128i vMp;
        __m128i vMt;
        __m128i vS;
        __m128i vSp;
        __m128i vSt;
        __m128i vL;
        __m128i vLp;
        __m128i vLt;
        __m128i vEx;

        /* calculate E */
        for (i=0; i<segLen; ++i) {
            vH = _mm_load_si128(pvH+i);
            vE = _mm_load_si128(pvE+i);
            vE = _mm_max_epi8(
                    _mm_subs_epi8(vE, vGapE),
                    _mm_subs_epi8(vH, vGapO));
            _mm_store_si128(pvE+i, vE);
        }

        /* calculate Ht */
        vH = _mm_slli_si128(_mm_load_si128(pvH+(segLen-1)), 1);
        vMp= _mm_slli_si128(_mm_load_si128(pvM+(segLen-1)), 1);
        vMp= _mm_insert_epi8(vMp, -bias, 0);
        vSp= _mm_slli_si128(_mm_load_si128(pvS+(segLen-1)), 1);
        vSp= _mm_insert_epi8(vSp, -bias, 0);
        vLp= _mm_slli_si128(_mm_load_si128(pvL+(segLen-1)), 1);
        vLp= _mm_insert_epi8(vLp, -bias, 0);
        vLp= _mm_adds_epi8(vLp, vOne);
        pvW = pvP + MAP_BLOSUM_[(unsigned char)s2[j]]*segLen;
        pvC = pvPm+ MAP_BLOSUM_[(unsigned char)s2[j]]*segLen;
        pvD = pvPs+ MAP_BLOSUM_[(unsigned char)s2[j]]*segLen;
        for (i=0; i<segLen; ++i) {
            __m128i cond_max;
            /* load values we need */
            vE = _mm_load_si128(pvE+i);
            vW = _mm_load_si128(pvW+i);
            /* compute */
            vH = _mm_adds_epi8(vH, vW);
            vHt = _mm_max_epi8(vH, vE);
            vHt = _mm_max_epi8(vHt, vZero);
            /* statistics */
            vC = _mm_load_si128(pvC+i);
            vD = _mm_load_si128(pvD+i);
            vMp = _mm_adds_epi8(vMp, vC);
            vSp = _mm_adds_epi8(vSp, vD);
            vEx = _mm_cmpgt_epi8(vE, vH);
            vM = _mm_load_si128(pvM+i);
            vS = _mm_load_si128(pvS+i);
            vL = _mm_load_si128(pvL+i);
            vL = _mm_adds_epi8(vL, vOne);
            vMt = _mm_blendv_epi8(vMp, vM, vEx);
            vSt = _mm_blendv_epi8(vSp, vS, vEx);
            vLt = _mm_blendv_epi8(vLp, vL, vEx);
            cond_max = _mm_cmpeq_epi8(vHt, vZero);
            vEx = _mm_andnot_si128(cond_max, vEx);
            vMt = _mm_blendv_epi8(vMt, vNegBias, cond_max);
            vSt = _mm_blendv_epi8(vSt, vNegBias, cond_max);
            vLt = _mm_blendv_epi8(vLt, vNegBias, cond_max);
            /* store results */
            _mm_store_si128(pvHt+i, vHt);
            _mm_store_si128(pvEx+i, vEx);
            _mm_store_si128(pvMt+i, vMt);
            _mm_store_si128(pvSt+i, vSt);
            _mm_store_si128(pvLt+i, vLt);
            /* prep for next iteration */
            vH = _mm_load_si128(pvH+i);
            vMp = vM;
            vSp = vS;
            vLp = vL;
        }

        /* calculate Ft */
        vHt = _mm_slli_si128(_mm_load_si128(pvHt+(segLen-1)), 1);
        vFt = _mm_set1_epi8(NEG_INF_8);
        for (i=0; i<segLen; ++i) {
            vFt = _mm_subs_epi8(vFt, vGapE);
            vFt = _mm_max_epi8(vFt, vHt);
            vHt = _mm_load_si128(pvHt+i);
        }
        {
            __m128i_8_t tmp;
            tmp.m = vFt;
            tmp.v[1] = MAX(tmp.v[0]-segLen*gap, tmp.v[1]);
            tmp.v[2] = MAX(tmp.v[1]-segLen*gap, tmp.v[2]);
            tmp.v[3] = MAX(tmp.v[2]-segLen*gap, tmp.v[3]);
            tmp.v[4] = MAX(tmp.v[3]-segLen*gap, tmp.v[4]);
            tmp.v[5] = MAX(tmp.v[4]-segLen*gap, tmp.v[5]);
            tmp.v[6] = MAX(tmp.v[5]-segLen*gap, tmp.v[6]);
            tmp.v[7] = MAX(tmp.v[6]-segLen*gap, tmp.v[7]);
            tmp.v[8]  = MAX(tmp.v[7] -segLen*gap, tmp.v[8]);
            tmp.v[9]  = MAX(tmp.v[8] -segLen*gap, tmp.v[9]);
            tmp.v[10] = MAX(tmp.v[9] -segLen*gap, tmp.v[10]);
            tmp.v[11] = MAX(tmp.v[10]-segLen*gap, tmp.v[11]);
            tmp.v[12] = MAX(tmp.v[11]-segLen*gap, tmp.v[12]);
            tmp.v[13] = MAX(tmp.v[12]-segLen*gap, tmp.v[13]);
            tmp.v[14] = MAX(tmp.v[13]-segLen*gap, tmp.v[14]);
            tmp.v[15] = MAX(tmp.v[14]-segLen*gap, tmp.v[15]);
            vFt = tmp.m;
        }
        vHt = _mm_slli_si128(_mm_load_si128(pvHt+(segLen-1)), 1);
        vFt = _mm_slli_si128(vFt, 1);
        vFt = _mm_insert_epi8(vFt, NEG_INF_8, 0);
        for (i=0; i<segLen; ++i) {
            vFt = _mm_subs_epi8(vFt, vGapE);
            vFt = _mm_max_epi8(vFt, vHt);
            vHt = _mm_load_si128(pvHt+i);
            _mm_store_si128(pvFt+i, vFt);
        }

        /* calculate H,M,L */
        vMp = vNegBias;
        vSp = vNegBias;
        vLp = _mm_adds_epi8(vNegBias, vOne);
        vC = _mm_cmpeq_epi8(vZero, vZero); /* check if prefix sum is needed */
        vC = _mm_srli_si128(vC, 1); /* zero out last value */
        for (i=0; i<segLen; ++i) {
            /* load values we need */
            vHt = _mm_load_si128(pvHt+i);
            vFt = _mm_load_si128(pvFt+i);
            /* compute */
            vFt = _mm_subs_epi8(vFt, vGapO);
            vH = _mm_max_epi8(vHt, vFt);
            /* statistics */
            vEx = _mm_load_si128(pvEx+i);
            vMt = _mm_load_si128(pvMt+i);
            vSt = _mm_load_si128(pvSt+i);
            vLt = _mm_load_si128(pvLt+i);
            vEx = _mm_or_si128(
                    _mm_and_si128(vEx, _mm_cmpeq_epi8(vHt, vFt)),
                    _mm_cmplt_epi8(vHt, vFt));
            vM = _mm_blendv_epi8(vMt, vMp, vEx);
            vS = _mm_blendv_epi8(vSt, vSp, vEx);
            vL = _mm_blendv_epi8(vLt, vLp, vEx);
            vMp = vM;
            vSp = vS;
            vLp = _mm_adds_epi8(vL, vOne);
            vC = _mm_and_si128(vC, vEx);
            /* store results */
            _mm_store_si128(pvH+i, vH);
            _mm_store_si128(pvEx+i, vEx);
            /* check for saturation */
            {
                vSaturationCheck = _mm_or_si128(vSaturationCheck,
                        _mm_or_si128(
                            _mm_cmpeq_epi8(vH, vNegLimit),
                            _mm_cmpeq_epi8(vH, vPosLimit)));
            }
#ifdef PARASAIL_TABLE
            arr_store_si128(result->score_table, vH, i, segLen, j, s2Len);
#endif
        }
        {
            vLp = _mm_subs_epi8(vLp, vOne);
            {
                __m128i_8_t uMp, uSp, uLp, uC;
                uC.m = vC;
                uMp.m = vMp;
                uMp.v[ 1] = uC.v[ 1] ? uMp.v[ 0] : uMp.v[ 1];
                uMp.v[ 2] = uC.v[ 2] ? uMp.v[ 1] : uMp.v[ 2];
                uMp.v[ 3] = uC.v[ 3] ? uMp.v[ 2] : uMp.v[ 3];
                uMp.v[ 4] = uC.v[ 4] ? uMp.v[ 3] : uMp.v[ 4];
                uMp.v[ 5] = uC.v[ 5] ? uMp.v[ 4] : uMp.v[ 5];
                uMp.v[ 6] = uC.v[ 6] ? uMp.v[ 5] : uMp.v[ 6];
                uMp.v[ 7] = uC.v[ 7] ? uMp.v[ 6] : uMp.v[ 7];
                uMp.v[ 8] = uC.v[ 8] ? uMp.v[ 7] : uMp.v[ 8];
                uMp.v[ 9] = uC.v[ 9] ? uMp.v[ 8] : uMp.v[ 9];
                uMp.v[10] = uC.v[10] ? uMp.v[ 9] : uMp.v[10];
                uMp.v[11] = uC.v[11] ? uMp.v[10] : uMp.v[11];
                uMp.v[12] = uC.v[12] ? uMp.v[11] : uMp.v[12];
                uMp.v[13] = uC.v[13] ? uMp.v[12] : uMp.v[13];
                uMp.v[14] = uC.v[14] ? uMp.v[13] : uMp.v[14];
                uMp.v[15] = uC.v[15] ? uMp.v[14] : uMp.v[15];
                vMp = uMp.m;
                uSp.m = vSp;
                uSp.v[ 1] = uC.v[ 1] ? uSp.v[ 0] : uSp.v[ 1];
                uSp.v[ 2] = uC.v[ 2] ? uSp.v[ 1] : uSp.v[ 2];
                uSp.v[ 3] = uC.v[ 3] ? uSp.v[ 2] : uSp.v[ 3];
                uSp.v[ 4] = uC.v[ 4] ? uSp.v[ 3] : uSp.v[ 4];
                uSp.v[ 5] = uC.v[ 5] ? uSp.v[ 4] : uSp.v[ 5];
                uSp.v[ 6] = uC.v[ 6] ? uSp.v[ 5] : uSp.v[ 6];
                uSp.v[ 7] = uC.v[ 7] ? uSp.v[ 6] : uSp.v[ 7];
                uSp.v[ 8] = uC.v[ 8] ? uSp.v[ 7] : uSp.v[ 8];
                uSp.v[ 9] = uC.v[ 9] ? uSp.v[ 8] : uSp.v[ 9];
                uSp.v[10] = uC.v[10] ? uSp.v[ 9] : uSp.v[10];
                uSp.v[11] = uC.v[11] ? uSp.v[10] : uSp.v[11];
                uSp.v[12] = uC.v[12] ? uSp.v[11] : uSp.v[12];
                uSp.v[13] = uC.v[13] ? uSp.v[12] : uSp.v[13];
                uSp.v[14] = uC.v[14] ? uSp.v[13] : uSp.v[14];
                uSp.v[15] = uC.v[15] ? uSp.v[14] : uSp.v[15];
                vSp = uSp.m;
                uLp.m = vLp;
                uLp.v[ 1] = uC.v[ 1] ? ibias + uLp.v[ 1] + uLp.v[ 0] : uLp.v[ 1];
                uLp.v[ 2] = uC.v[ 2] ? ibias + uLp.v[ 2] + uLp.v[ 1] : uLp.v[ 2];
                uLp.v[ 3] = uC.v[ 3] ? ibias + uLp.v[ 3] + uLp.v[ 2] : uLp.v[ 3];
                uLp.v[ 4] = uC.v[ 4] ? ibias + uLp.v[ 4] + uLp.v[ 3] : uLp.v[ 4];
                uLp.v[ 5] = uC.v[ 5] ? ibias + uLp.v[ 5] + uLp.v[ 4] : uLp.v[ 5];
                uLp.v[ 6] = uC.v[ 6] ? ibias + uLp.v[ 6] + uLp.v[ 5] : uLp.v[ 6];
                uLp.v[ 7] = uC.v[ 7] ? ibias + uLp.v[ 7] + uLp.v[ 6] : uLp.v[ 7];
                uLp.v[ 8] = uC.v[ 8] ? ibias + uLp.v[ 8] + uLp.v[ 7] : uLp.v[ 8];
                uLp.v[ 9] = uC.v[ 9] ? ibias + uLp.v[ 9] + uLp.v[ 8] : uLp.v[ 9];
                uLp.v[10] = uC.v[10] ? ibias + uLp.v[10] + uLp.v[ 9] : uLp.v[10];
                uLp.v[11] = uC.v[11] ? ibias + uLp.v[11] + uLp.v[10] : uLp.v[11];
                uLp.v[12] = uC.v[12] ? ibias + uLp.v[12] + uLp.v[11] : uLp.v[12];
                uLp.v[13] = uC.v[13] ? ibias + uLp.v[13] + uLp.v[12] : uLp.v[13];
                uLp.v[14] = uC.v[14] ? ibias + uLp.v[14] + uLp.v[13] : uLp.v[14];
                uLp.v[15] = uC.v[15] ? ibias + uLp.v[15] + uLp.v[14] : uLp.v[15];
                vLp = uLp.m;
            }
            vLp = _mm_adds_epi8(vLp, vOne);
        }
        /* final pass for M,L */
        vMp = _mm_slli_si128(vMp, 1);
        vMp = _mm_insert_epi8(vMp, -bias, 0);
        vSp = _mm_slli_si128(vSp, 1);
        vSp = _mm_insert_epi8(vSp, -bias, 0);
        vLp = _mm_slli_si128(vLp, 1);
        vLp = _mm_insert_epi8(vLp, -bias, 0);
        for (i=0; i<segLen; ++i) {
            /* load values we need */
            vH = _mm_load_si128(pvH+i);
            /* statistics */
            vEx = _mm_load_si128(pvEx+i);
            vMt = _mm_load_si128(pvMt+i);
            vSt = _mm_load_si128(pvSt+i);
            vLt = _mm_load_si128(pvLt+i);
            vM = _mm_blendv_epi8(vMt, vMp, vEx);
            vS = _mm_blendv_epi8(vSt, vSp, vEx);
            vL = _mm_blendv_epi8(vLt, vLp, vEx);
            vMp = vM;
            vSp = vS;
            vLp = _mm_adds_epi8(vL, vOne);
            /* store results */
            _mm_store_si128(pvM+i, vM);
            _mm_store_si128(pvS+i, vS);
            _mm_store_si128(pvL+i, vL);
            /* check for saturation */
            {
                vSaturationCheck = _mm_or_si128(vSaturationCheck,
                        _mm_or_si128(
                            _mm_cmpeq_epi8(vM, vNegLimit),
                            _mm_cmpeq_epi8(vM, vPosLimit)));
                vSaturationCheck = _mm_or_si128(vSaturationCheck,
                        _mm_or_si128(
                            _mm_cmpeq_epi8(vS, vNegLimit),
                            _mm_cmpeq_epi8(vS, vPosLimit)));
                vSaturationCheck = _mm_or_si128(vSaturationCheck,
                        _mm_or_si128(
                            _mm_cmpeq_epi8(vL, vNegLimit),
                            _mm_cmpeq_epi8(vL, vPosLimit)));
            }
#ifdef PARASAIL_TABLE
            arr_store_si128_bias(result->matches_table, vM, i, segLen, j, s2Len, bias);
            arr_store_si128_bias(result->similar_table, vS, i, segLen, j, s2Len, bias);
            arr_store_si128_bias(result->length_table, vL, i, segLen, j, s2Len, bias);
#endif
            /* update max vector seen so far */
            {
                __m128i cond_max = _mm_cmpgt_epi8(vH, vMaxH);
                vMaxH = _mm_blendv_epi8(vMaxH, vH, cond_max);
                vMaxM = _mm_blendv_epi8(vMaxM, vM, cond_max);
                vMaxS = _mm_blendv_epi8(vMaxS, vS, cond_max);
                vMaxL = _mm_blendv_epi8(vMaxL, vL, cond_max);
            }
        }
    }

    /* max in vec */
    for (j=0; j<segWidth; ++j) {
        int8_t value = (int8_t) _mm_extract_epi8(vMaxH, 15);
        if (value > score) {
            score = value;
            matches = ibias + (int)((int8_t)_mm_extract_epi8(vMaxM, 15));
            similar = ibias + (int)((int8_t)_mm_extract_epi8(vMaxS, 15));
            length = ibias + (int)((int8_t)_mm_extract_epi8(vMaxL, 15));
        }
        vMaxH = _mm_slli_si128(vMaxH, 1);
        vMaxM = _mm_slli_si128(vMaxM, 1);
        vMaxS = _mm_slli_si128(vMaxS, 1);
        vMaxL = _mm_slli_si128(vMaxL, 1);
    }

    if (_mm_movemask_epi8(vSaturationCheck)) {
        result->saturated = 1;
        score = INT8_MAX;
        matches = 0;
        similar = 0;
        length = 0;
    }

    result->score = score;
    result->matches = matches;
    result->similar = similar;
    result->length = length;

    parasail_free(pvL);
    parasail_free(pvS);
    parasail_free(pvM);
    parasail_free(pvH);
    parasail_free(pvEx);
    parasail_free(pvLt);
    parasail_free(pvSt);
    parasail_free(pvMt);
    parasail_free(pvFt);
    parasail_free(pvHt);
    parasail_free(pvE);
    parasail_free(pvPs);
    parasail_free(pvPm);
    parasail_free(pvP);

    return result;
}
