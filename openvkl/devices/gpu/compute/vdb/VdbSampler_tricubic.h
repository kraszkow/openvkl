// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0f

#pragma once

#include "VdbSampler_stencilDispatch.h"
#include "VdbSampler_traverseAndSample.h"

// ---------------------------------------------------------------------------
// TRICUBIC interpolation.
//
// We implement the technique from
//
// "Tricubic Interpolation in Three Dimensions"
// F. Lekien and J. Marsden
// Journal of Numerical Methods and Engineering (2005)
//
// Note that we use central differences to obtain partial derivatives for our
// constraints.
// ---------------------------------------------------------------------------

// Compute the coefficient vector COE (64 elements) from the constraint
// vector CTR (64 elements).
// CTR = [
//   f(0, 0, 0), f(0, 0, 1), ..., f(1, 1, 1),
//   d/dx f, ...,
//   d/dy f, ...,
//   d/dz f, ...,
//   d2/dxdy f, ...,
//   d2/dxdz f, ...,
//   d2/dydz f, ...,
//   d3/dxdydz f, ...
// ]
#define __vkl_tricubic_coefficients(CTR)                                  \
  {                                                                       \
    (1.0f) * CTR[0], (1.0f) * CTR[24],                                    \
        (-3.0f) * CTR[0] + (3.0f) * CTR[1] + (-2.0f) * CTR[24] +          \
            (-1.0f) * CTR[25],                                            \
        (2.0f) * CTR[0] + (-2.0f) * CTR[1] + (1.0f) * CTR[24] +           \
            (1.0f) * CTR[25],                                             \
        (1.0f) * CTR[16], (1.0f) * CTR[48],                               \
        (-3.0f) * CTR[16] + (3.0f) * CTR[17] + (-2.0f) * CTR[48] +        \
            (-1.0f) * CTR[49],                                            \
        (2.0f) * CTR[16] + (-2.0f) * CTR[17] + (1.0f) * CTR[48] +         \
            (1.0f) * CTR[49],                                             \
        (-3.0f) * CTR[0] + (3.0f) * CTR[2] + (-2.0f) * CTR[16] +          \
            (-1.0f) * CTR[18],                                            \
        (-3.0f) * CTR[24] + (3.0f) * CTR[26] + (-2.0f) * CTR[48] +        \
            (-1.0f) * CTR[50],                                            \
        (9.0f) * CTR[0] + (-9.0f) * CTR[1] + (-9.0f) * CTR[2] +           \
            (9.0f) * CTR[3] + (6.0f) * CTR[16] + (-6.0f) * CTR[17] +      \
            (3.0f) * CTR[18] + (-3.0f) * CTR[19] + (6.0f) * CTR[24] +     \
            (3.0f) * CTR[25] + (-6.0f) * CTR[26] + (-3.0f) * CTR[27] +    \
            (4.0f) * CTR[48] + (2.0f) * CTR[49] + (2.0f) * CTR[50] +      \
            (1.0f) * CTR[51],                                             \
        (-6.0f) * CTR[0] + (6.0f) * CTR[1] + (6.0f) * CTR[2] +            \
            (-6.0f) * CTR[3] + (-4.0f) * CTR[16] + (4.0f) * CTR[17] +     \
            (-2.0f) * CTR[18] + (2.0f) * CTR[19] + (-3.0f) * CTR[24] +    \
            (-3.0f) * CTR[25] + (3.0f) * CTR[26] + (3.0f) * CTR[27] +     \
            (-2.0f) * CTR[48] + (-2.0f) * CTR[49] + (-1.0f) * CTR[50] +   \
            (-1.0f) * CTR[51],                                            \
        (2.0f) * CTR[0] + (-2.0f) * CTR[2] + (1.0f) * CTR[16] +           \
            (1.0f) * CTR[18],                                             \
        (2.0f) * CTR[24] + (-2.0f) * CTR[26] + (1.0f) * CTR[48] +         \
            (1.0f) * CTR[50],                                             \
        (-6.0f) * CTR[0] + (6.0f) * CTR[1] + (6.0f) * CTR[2] +            \
            (-6.0f) * CTR[3] + (-3.0f) * CTR[16] + (3.0f) * CTR[17] +     \
            (-3.0f) * CTR[18] + (3.0f) * CTR[19] + (-4.0f) * CTR[24] +    \
            (-2.0f) * CTR[25] + (4.0f) * CTR[26] + (2.0f) * CTR[27] +     \
            (-2.0f) * CTR[48] + (-1.0f) * CTR[49] + (-2.0f) * CTR[50] +   \
            (-1.0f) * CTR[51],                                            \
        (4.0f) * CTR[0] + (-4.0f) * CTR[1] + (-4.0f) * CTR[2] +           \
            (4.0f) * CTR[3] + (2.0f) * CTR[16] + (-2.0f) * CTR[17] +      \
            (2.0f) * CTR[18] + (-2.0f) * CTR[19] + (2.0f) * CTR[24] +     \
            (2.0f) * CTR[25] + (-2.0f) * CTR[26] + (-2.0f) * CTR[27] +    \
            (1.0f) * CTR[48] + (1.0f) * CTR[49] + (1.0f) * CTR[50] +      \
            (1.0f) * CTR[51],                                             \
        (1.0f) * CTR[8], (1.0f) * CTR[40],                                \
        (-3.0f) * CTR[8] + (3.0f) * CTR[9] + (-2.0f) * CTR[40] +          \
            (-1.0f) * CTR[41],                                            \
        (2.0f) * CTR[8] + (-2.0f) * CTR[9] + (1.0f) * CTR[40] +           \
            (1.0f) * CTR[41],                                             \
        (1.0f) * CTR[32], (1.0f) * CTR[56],                               \
        (-3.0f) * CTR[32] + (3.0f) * CTR[33] + (-2.0f) * CTR[56] +        \
            (-1.0f) * CTR[57],                                            \
        (2.0f) * CTR[32] + (-2.0f) * CTR[33] + (1.0f) * CTR[56] +         \
            (1.0f) * CTR[57],                                             \
        (-3.0f) * CTR[8] + (3.0f) * CTR[10] + (-2.0f) * CTR[32] +         \
            (-1.0f) * CTR[34],                                            \
        (-3.0f) * CTR[40] + (3.0f) * CTR[42] + (-2.0f) * CTR[56] +        \
            (-1.0f) * CTR[58],                                            \
        (9.0f) * CTR[8] + (-9.0f) * CTR[9] + (-9.0f) * CTR[10] +          \
            (9.0f) * CTR[11] + (6.0f) * CTR[32] + (-6.0f) * CTR[33] +     \
            (3.0f) * CTR[34] + (-3.0f) * CTR[35] + (6.0f) * CTR[40] +     \
            (3.0f) * CTR[41] + (-6.0f) * CTR[42] + (-3.0f) * CTR[43] +    \
            (4.0f) * CTR[56] + (2.0f) * CTR[57] + (2.0f) * CTR[58] +      \
            (1.0f) * CTR[59],                                             \
        (-6.0f) * CTR[8] + (6.0f) * CTR[9] + (6.0f) * CTR[10] +           \
            (-6.0f) * CTR[11] + (-4.0f) * CTR[32] + (4.0f) * CTR[33] +    \
            (-2.0f) * CTR[34] + (2.0f) * CTR[35] + (-3.0f) * CTR[40] +    \
            (-3.0f) * CTR[41] + (3.0f) * CTR[42] + (3.0f) * CTR[43] +     \
            (-2.0f) * CTR[56] + (-2.0f) * CTR[57] + (-1.0f) * CTR[58] +   \
            (-1.0f) * CTR[59],                                            \
        (2.0f) * CTR[8] + (-2.0f) * CTR[10] + (1.0f) * CTR[32] +          \
            (1.0f) * CTR[34],                                             \
        (2.0f) * CTR[40] + (-2.0f) * CTR[42] + (1.0f) * CTR[56] +         \
            (1.0f) * CTR[58],                                             \
        (-6.0f) * CTR[8] + (6.0f) * CTR[9] + (6.0f) * CTR[10] +           \
            (-6.0f) * CTR[11] + (-3.0f) * CTR[32] + (3.0f) * CTR[33] +    \
            (-3.0f) * CTR[34] + (3.0f) * CTR[35] + (-4.0f) * CTR[40] +    \
            (-2.0f) * CTR[41] + (4.0f) * CTR[42] + (2.0f) * CTR[43] +     \
            (-2.0f) * CTR[56] + (-1.0f) * CTR[57] + (-2.0f) * CTR[58] +   \
            (-1.0f) * CTR[59],                                            \
        (4.0f) * CTR[8] + (-4.0f) * CTR[9] + (-4.0f) * CTR[10] +          \
            (4.0f) * CTR[11] + (2.0f) * CTR[32] + (-2.0f) * CTR[33] +     \
            (2.0f) * CTR[34] + (-2.0f) * CTR[35] + (2.0f) * CTR[40] +     \
            (2.0f) * CTR[41] + (-2.0f) * CTR[42] + (-2.0f) * CTR[43] +    \
            (1.0f) * CTR[56] + (1.0f) * CTR[57] + (1.0f) * CTR[58] +      \
            (1.0f) * CTR[59],                                             \
        (-3.0f) * CTR[0] + (3.0f) * CTR[4] + (-2.0f) * CTR[8] +           \
            (-1.0f) * CTR[12],                                            \
        (-3.0f) * CTR[24] + (3.0f) * CTR[28] + (-2.0f) * CTR[40] +        \
            (-1.0f) * CTR[44],                                            \
        (9.0f) * CTR[0] + (-9.0f) * CTR[1] + (-9.0f) * CTR[4] +           \
            (9.0f) * CTR[5] + (6.0f) * CTR[8] + (-6.0f) * CTR[9] +        \
            (3.0f) * CTR[12] + (-3.0f) * CTR[13] + (6.0f) * CTR[24] +     \
            (3.0f) * CTR[25] + (-6.0f) * CTR[28] + (-3.0f) * CTR[29] +    \
            (4.0f) * CTR[40] + (2.0f) * CTR[41] + (2.0f) * CTR[44] +      \
            (1.0f) * CTR[45],                                             \
        (-6.0f) * CTR[0] + (6.0f) * CTR[1] + (6.0f) * CTR[4] +            \
            (-6.0f) * CTR[5] + (-4.0f) * CTR[8] + (4.0f) * CTR[9] +       \
            (-2.0f) * CTR[12] + (2.0f) * CTR[13] + (-3.0f) * CTR[24] +    \
            (-3.0f) * CTR[25] + (3.0f) * CTR[28] + (3.0f) * CTR[29] +     \
            (-2.0f) * CTR[40] + (-2.0f) * CTR[41] + (-1.0f) * CTR[44] +   \
            (-1.0f) * CTR[45],                                            \
        (-3.0f) * CTR[16] + (3.0f) * CTR[20] + (-2.0f) * CTR[32] +        \
            (-1.0f) * CTR[36],                                            \
        (-3.0f) * CTR[48] + (3.0f) * CTR[52] + (-2.0f) * CTR[56] +        \
            (-1.0f) * CTR[60],                                            \
        (9.0f) * CTR[16] + (-9.0f) * CTR[17] + (-9.0f) * CTR[20] +        \
            (9.0f) * CTR[21] + (6.0f) * CTR[32] + (-6.0f) * CTR[33] +     \
            (3.0f) * CTR[36] + (-3.0f) * CTR[37] + (6.0f) * CTR[48] +     \
            (3.0f) * CTR[49] + (-6.0f) * CTR[52] + (-3.0f) * CTR[53] +    \
            (4.0f) * CTR[56] + (2.0f) * CTR[57] + (2.0f) * CTR[60] +      \
            (1.0f) * CTR[61],                                             \
        (-6.0f) * CTR[16] + (6.0f) * CTR[17] + (6.0f) * CTR[20] +         \
            (-6.0f) * CTR[21] + (-4.0f) * CTR[32] + (4.0f) * CTR[33] +    \
            (-2.0f) * CTR[36] + (2.0f) * CTR[37] + (-3.0f) * CTR[48] +    \
            (-3.0f) * CTR[49] + (3.0f) * CTR[52] + (3.0f) * CTR[53] +     \
            (-2.0f) * CTR[56] + (-2.0f) * CTR[57] + (-1.0f) * CTR[60] +   \
            (-1.0f) * CTR[61],                                            \
        (9.0f) * CTR[0] + (-9.0f) * CTR[2] + (-9.0f) * CTR[4] +           \
            (9.0f) * CTR[6] + (6.0f) * CTR[8] + (-6.0f) * CTR[10] +       \
            (3.0f) * CTR[12] + (-3.0f) * CTR[14] + (6.0f) * CTR[16] +     \
            (3.0f) * CTR[18] + (-6.0f) * CTR[20] + (-3.0f) * CTR[22] +    \
            (4.0f) * CTR[32] + (2.0f) * CTR[34] + (2.0f) * CTR[36] +      \
            (1.0f) * CTR[38],                                             \
        (9.0f) * CTR[24] + (-9.0f) * CTR[26] + (-9.0f) * CTR[28] +        \
            (9.0f) * CTR[30] + (6.0f) * CTR[40] + (-6.0f) * CTR[42] +     \
            (3.0f) * CTR[44] + (-3.0f) * CTR[46] + (6.0f) * CTR[48] +     \
            (3.0f) * CTR[50] + (-6.0f) * CTR[52] + (-3.0f) * CTR[54] +    \
            (4.0f) * CTR[56] + (2.0f) * CTR[58] + (2.0f) * CTR[60] +      \
            (1.0f) * CTR[62],                                             \
        (-27.0f) * CTR[0] + (27.0f) * CTR[1] + (27.0f) * CTR[2] +         \
            (-27.0f) * CTR[3] + (27.0f) * CTR[4] + (-27.0f) * CTR[5] +    \
            (-27.0f) * CTR[6] + (27.0f) * CTR[7] + (-18.0f) * CTR[8] +    \
            (18.0f) * CTR[9] + (18.0f) * CTR[10] + (-18.0f) * CTR[11] +   \
            (-9.0f) * CTR[12] + (9.0f) * CTR[13] + (9.0f) * CTR[14] +     \
            (-9.0f) * CTR[15] + (-18.0f) * CTR[16] + (18.0f) * CTR[17] +  \
            (-9.0f) * CTR[18] + (9.0f) * CTR[19] + (18.0f) * CTR[20] +    \
            (-18.0f) * CTR[21] + (9.0f) * CTR[22] + (-9.0f) * CTR[23] +   \
            (-18.0f) * CTR[24] + (-9.0f) * CTR[25] + (18.0f) * CTR[26] +  \
            (9.0f) * CTR[27] + (18.0f) * CTR[28] + (9.0f) * CTR[29] +     \
            (-18.0f) * CTR[30] + (-9.0f) * CTR[31] + (-12.0f) * CTR[32] + \
            (12.0f) * CTR[33] + (-6.0f) * CTR[34] + (6.0f) * CTR[35] +    \
            (-6.0f) * CTR[36] + (6.0f) * CTR[37] + (-3.0f) * CTR[38] +    \
            (3.0f) * CTR[39] + (-12.0f) * CTR[40] + (-6.0f) * CTR[41] +   \
            (12.0f) * CTR[42] + (6.0f) * CTR[43] + (-6.0f) * CTR[44] +    \
            (-3.0f) * CTR[45] + (6.0f) * CTR[46] + (3.0f) * CTR[47] +     \
            (-12.0f) * CTR[48] + (-6.0f) * CTR[49] + (-6.0f) * CTR[50] +  \
            (-3.0f) * CTR[51] + (12.0f) * CTR[52] + (6.0f) * CTR[53] +    \
            (6.0f) * CTR[54] + (3.0f) * CTR[55] + (-8.0f) * CTR[56] +     \
            (-4.0f) * CTR[57] + (-4.0f) * CTR[58] + (-2.0f) * CTR[59] +   \
            (-4.0f) * CTR[60] + (-2.0f) * CTR[61] + (-2.0f) * CTR[62] +   \
            (-1.0f) * CTR[63],                                            \
        (18.0f) * CTR[0] + (-18.0f) * CTR[1] + (-18.0f) * CTR[2] +        \
            (18.0f) * CTR[3] + (-18.0f) * CTR[4] + (18.0f) * CTR[5] +     \
            (18.0f) * CTR[6] + (-18.0f) * CTR[7] + (12.0f) * CTR[8] +     \
            (-12.0f) * CTR[9] + (-12.0f) * CTR[10] + (12.0f) * CTR[11] +  \
            (6.0f) * CTR[12] + (-6.0f) * CTR[13] + (-6.0f) * CTR[14] +    \
            (6.0f) * CTR[15] + (12.0f) * CTR[16] + (-12.0f) * CTR[17] +   \
            (6.0f) * CTR[18] + (-6.0f) * CTR[19] + (-12.0f) * CTR[20] +   \
            (12.0f) * CTR[21] + (-6.0f) * CTR[22] + (6.0f) * CTR[23] +    \
            (9.0f) * CTR[24] + (9.0f) * CTR[25] + (-9.0f) * CTR[26] +     \
            (-9.0f) * CTR[27] + (-9.0f) * CTR[28] + (-9.0f) * CTR[29] +   \
            (9.0f) * CTR[30] + (9.0f) * CTR[31] + (8.0f) * CTR[32] +      \
            (-8.0f) * CTR[33] + (4.0f) * CTR[34] + (-4.0f) * CTR[35] +    \
            (4.0f) * CTR[36] + (-4.0f) * CTR[37] + (2.0f) * CTR[38] +     \
            (-2.0f) * CTR[39] + (6.0f) * CTR[40] + (6.0f) * CTR[41] +     \
            (-6.0f) * CTR[42] + (-6.0f) * CTR[43] + (3.0f) * CTR[44] +    \
            (3.0f) * CTR[45] + (-3.0f) * CTR[46] + (-3.0f) * CTR[47] +    \
            (6.0f) * CTR[48] + (6.0f) * CTR[49] + (3.0f) * CTR[50] +      \
            (3.0f) * CTR[51] + (-6.0f) * CTR[52] + (-6.0f) * CTR[53] +    \
            (-3.0f) * CTR[54] + (-3.0f) * CTR[55] + (4.0f) * CTR[56] +    \
            (4.0f) * CTR[57] + (2.0f) * CTR[58] + (2.0f) * CTR[59] +      \
            (2.0f) * CTR[60] + (2.0f) * CTR[61] + (1.0f) * CTR[62] +      \
            (1.0f) * CTR[63],                                             \
        (-6.0f) * CTR[0] + (6.0f) * CTR[2] + (6.0f) * CTR[4] +            \
            (-6.0f) * CTR[6] + (-4.0f) * CTR[8] + (4.0f) * CTR[10] +      \
            (-2.0f) * CTR[12] + (2.0f) * CTR[14] + (-3.0f) * CTR[16] +    \
            (-3.0f) * CTR[18] + (3.0f) * CTR[20] + (3.0f) * CTR[22] +     \
            (-2.0f) * CTR[32] + (-2.0f) * CTR[34] + (-1.0f) * CTR[36] +   \
            (-1.0f) * CTR[38],                                            \
        (-6.0f) * CTR[24] + (6.0f) * CTR[26] + (6.0f) * CTR[28] +         \
            (-6.0f) * CTR[30] + (-4.0f) * CTR[40] + (4.0f) * CTR[42] +    \
            (-2.0f) * CTR[44] + (2.0f) * CTR[46] + (-3.0f) * CTR[48] +    \
            (-3.0f) * CTR[50] + (3.0f) * CTR[52] + (3.0f) * CTR[54] +     \
            (-2.0f) * CTR[56] + (-2.0f) * CTR[58] + (-1.0f) * CTR[60] +   \
            (-1.0f) * CTR[62],                                            \
        (18.0f) * CTR[0] + (-18.0f) * CTR[1] + (-18.0f) * CTR[2] +        \
            (18.0f) * CTR[3] + (-18.0f) * CTR[4] + (18.0f) * CTR[5] +     \
            (18.0f) * CTR[6] + (-18.0f) * CTR[7] + (12.0f) * CTR[8] +     \
            (-12.0f) * CTR[9] + (-12.0f) * CTR[10] + (12.0f) * CTR[11] +  \
            (6.0f) * CTR[12] + (-6.0f) * CTR[13] + (-6.0f) * CTR[14] +    \
            (6.0f) * CTR[15] + (9.0f) * CTR[16] + (-9.0f) * CTR[17] +     \
            (9.0f) * CTR[18] + (-9.0f) * CTR[19] + (-9.0f) * CTR[20] +    \
            (9.0f) * CTR[21] + (-9.0f) * CTR[22] + (9.0f) * CTR[23] +     \
            (12.0f) * CTR[24] + (6.0f) * CTR[25] + (-12.0f) * CTR[26] +   \
            (-6.0f) * CTR[27] + (-12.0f) * CTR[28] + (-6.0f) * CTR[29] +  \
            (12.0f) * CTR[30] + (6.0f) * CTR[31] + (6.0f) * CTR[32] +     \
            (-6.0f) * CTR[33] + (6.0f) * CTR[34] + (-6.0f) * CTR[35] +    \
            (3.0f) * CTR[36] + (-3.0f) * CTR[37] + (3.0f) * CTR[38] +     \
            (-3.0f) * CTR[39] + (8.0f) * CTR[40] + (4.0f) * CTR[41] +     \
            (-8.0f) * CTR[42] + (-4.0f) * CTR[43] + (4.0f) * CTR[44] +    \
            (2.0f) * CTR[45] + (-4.0f) * CTR[46] + (-2.0f) * CTR[47] +    \
            (6.0f) * CTR[48] + (3.0f) * CTR[49] + (6.0f) * CTR[50] +      \
            (3.0f) * CTR[51] + (-6.0f) * CTR[52] + (-3.0f) * CTR[53] +    \
            (-6.0f) * CTR[54] + (-3.0f) * CTR[55] + (4.0f) * CTR[56] +    \
            (2.0f) * CTR[57] + (4.0f) * CTR[58] + (2.0f) * CTR[59] +      \
            (2.0f) * CTR[60] + (1.0f) * CTR[61] + (2.0f) * CTR[62] +      \
            (1.0f) * CTR[63],                                             \
        (-12.0f) * CTR[0] + (12.0f) * CTR[1] + (12.0f) * CTR[2] +         \
            (-12.0f) * CTR[3] + (12.0f) * CTR[4] + (-12.0f) * CTR[5] +    \
            (-12.0f) * CTR[6] + (12.0f) * CTR[7] + (-8.0f) * CTR[8] +     \
            (8.0f) * CTR[9] + (8.0f) * CTR[10] + (-8.0f) * CTR[11] +      \
            (-4.0f) * CTR[12] + (4.0f) * CTR[13] + (4.0f) * CTR[14] +     \
            (-4.0f) * CTR[15] + (-6.0f) * CTR[16] + (6.0f) * CTR[17] +    \
            (-6.0f) * CTR[18] + (6.0f) * CTR[19] + (6.0f) * CTR[20] +     \
            (-6.0f) * CTR[21] + (6.0f) * CTR[22] + (-6.0f) * CTR[23] +    \
            (-6.0f) * CTR[24] + (-6.0f) * CTR[25] + (6.0f) * CTR[26] +    \
            (6.0f) * CTR[27] + (6.0f) * CTR[28] + (6.0f) * CTR[29] +      \
            (-6.0f) * CTR[30] + (-6.0f) * CTR[31] + (-4.0f) * CTR[32] +   \
            (4.0f) * CTR[33] + (-4.0f) * CTR[34] + (4.0f) * CTR[35] +     \
            (-2.0f) * CTR[36] + (2.0f) * CTR[37] + (-2.0f) * CTR[38] +    \
            (2.0f) * CTR[39] + (-4.0f) * CTR[40] + (-4.0f) * CTR[41] +    \
            (4.0f) * CTR[42] + (4.0f) * CTR[43] + (-2.0f) * CTR[44] +     \
            (-2.0f) * CTR[45] + (2.0f) * CTR[46] + (2.0f) * CTR[47] +     \
            (-3.0f) * CTR[48] + (-3.0f) * CTR[49] + (-3.0f) * CTR[50] +   \
            (-3.0f) * CTR[51] + (3.0f) * CTR[52] + (3.0f) * CTR[53] +     \
            (3.0f) * CTR[54] + (3.0f) * CTR[55] + (-2.0f) * CTR[56] +     \
            (-2.0f) * CTR[57] + (-2.0f) * CTR[58] + (-2.0f) * CTR[59] +   \
            (-1.0f) * CTR[60] + (-1.0f) * CTR[61] + (-1.0f) * CTR[62] +   \
            (-1.0f) * CTR[63],                                            \
        (2.0f) * CTR[0] + (-2.0f) * CTR[4] + (1.0f) * CTR[8] +            \
            (1.0f) * CTR[12],                                             \
        (2.0f) * CTR[24] + (-2.0f) * CTR[28] + (1.0f) * CTR[40] +         \
            (1.0f) * CTR[44],                                             \
        (-6.0f) * CTR[0] + (6.0f) * CTR[1] + (6.0f) * CTR[4] +            \
            (-6.0f) * CTR[5] + (-3.0f) * CTR[8] + (3.0f) * CTR[9] +       \
            (-3.0f) * CTR[12] + (3.0f) * CTR[13] + (-4.0f) * CTR[24] +    \
            (-2.0f) * CTR[25] + (4.0f) * CTR[28] + (2.0f) * CTR[29] +     \
            (-2.0f) * CTR[40] + (-1.0f) * CTR[41] + (-2.0f) * CTR[44] +   \
            (-1.0f) * CTR[45],                                            \
        (4.0f) * CTR[0] + (-4.0f) * CTR[1] + (-4.0f) * CTR[4] +           \
            (4.0f) * CTR[5] + (2.0f) * CTR[8] + (-2.0f) * CTR[9] +        \
            (2.0f) * CTR[12] + (-2.0f) * CTR[13] + (2.0f) * CTR[24] +     \
            (2.0f) * CTR[25] + (-2.0f) * CTR[28] + (-2.0f) * CTR[29] +    \
            (1.0f) * CTR[40] + (1.0f) * CTR[41] + (1.0f) * CTR[44] +      \
            (1.0f) * CTR[45],                                             \
        (2.0f) * CTR[16] + (-2.0f) * CTR[20] + (1.0f) * CTR[32] +         \
            (1.0f) * CTR[36],                                             \
        (2.0f) * CTR[48] + (-2.0f) * CTR[52] + (1.0f) * CTR[56] +         \
            (1.0f) * CTR[60],                                             \
        (-6.0f) * CTR[16] + (6.0f) * CTR[17] + (6.0f) * CTR[20] +         \
            (-6.0f) * CTR[21] + (-3.0f) * CTR[32] + (3.0f) * CTR[33] +    \
            (-3.0f) * CTR[36] + (3.0f) * CTR[37] + (-4.0f) * CTR[48] +    \
            (-2.0f) * CTR[49] + (4.0f) * CTR[52] + (2.0f) * CTR[53] +     \
            (-2.0f) * CTR[56] + (-1.0f) * CTR[57] + (-2.0f) * CTR[60] +   \
            (-1.0f) * CTR[61],                                            \
        (4.0f) * CTR[16] + (-4.0f) * CTR[17] + (-4.0f) * CTR[20] +        \
            (4.0f) * CTR[21] + (2.0f) * CTR[32] + (-2.0f) * CTR[33] +     \
            (2.0f) * CTR[36] + (-2.0f) * CTR[37] + (2.0f) * CTR[48] +     \
            (2.0f) * CTR[49] + (-2.0f) * CTR[52] + (-2.0f) * CTR[53] +    \
            (1.0f) * CTR[56] + (1.0f) * CTR[57] + (1.0f) * CTR[60] +      \
            (1.0f) * CTR[61],                                             \
        (-6.0f) * CTR[0] + (6.0f) * CTR[2] + (6.0f) * CTR[4] +            \
            (-6.0f) * CTR[6] + (-3.0f) * CTR[8] + (3.0f) * CTR[10] +      \
            (-3.0f) * CTR[12] + (3.0f) * CTR[14] + (-4.0f) * CTR[16] +    \
            (-2.0f) * CTR[18] + (4.0f) * CTR[20] + (2.0f) * CTR[22] +     \
            (-2.0f) * CTR[32] + (-1.0f) * CTR[34] + (-2.0f) * CTR[36] +   \
            (-1.0f) * CTR[38],                                            \
        (-6.0f) * CTR[24] + (6.0f) * CTR[26] + (6.0f) * CTR[28] +         \
            (-6.0f) * CTR[30] + (-3.0f) * CTR[40] + (3.0f) * CTR[42] +    \
            (-3.0f) * CTR[44] + (3.0f) * CTR[46] + (-4.0f) * CTR[48] +    \
            (-2.0f) * CTR[50] + (4.0f) * CTR[52] + (2.0f) * CTR[54] +     \
            (-2.0f) * CTR[56] + (-1.0f) * CTR[58] + (-2.0f) * CTR[60] +   \
            (-1.0f) * CTR[62],                                            \
        (18.0f) * CTR[0] + (-18.0f) * CTR[1] + (-18.0f) * CTR[2] +        \
            (18.0f) * CTR[3] + (-18.0f) * CTR[4] + (18.0f) * CTR[5] +     \
            (18.0f) * CTR[6] + (-18.0f) * CTR[7] + (9.0f) * CTR[8] +      \
            (-9.0f) * CTR[9] + (-9.0f) * CTR[10] + (9.0f) * CTR[11] +     \
            (9.0f) * CTR[12] + (-9.0f) * CTR[13] + (-9.0f) * CTR[14] +    \
            (9.0f) * CTR[15] + (12.0f) * CTR[16] + (-12.0f) * CTR[17] +   \
            (6.0f) * CTR[18] + (-6.0f) * CTR[19] + (-12.0f) * CTR[20] +   \
            (12.0f) * CTR[21] + (-6.0f) * CTR[22] + (6.0f) * CTR[23] +    \
            (12.0f) * CTR[24] + (6.0f) * CTR[25] + (-12.0f) * CTR[26] +   \
            (-6.0f) * CTR[27] + (-12.0f) * CTR[28] + (-6.0f) * CTR[29] +  \
            (12.0f) * CTR[30] + (6.0f) * CTR[31] + (6.0f) * CTR[32] +     \
            (-6.0f) * CTR[33] + (3.0f) * CTR[34] + (-3.0f) * CTR[35] +    \
            (6.0f) * CTR[36] + (-6.0f) * CTR[37] + (3.0f) * CTR[38] +     \
            (-3.0f) * CTR[39] + (6.0f) * CTR[40] + (3.0f) * CTR[41] +     \
            (-6.0f) * CTR[42] + (-3.0f) * CTR[43] + (6.0f) * CTR[44] +    \
            (3.0f) * CTR[45] + (-6.0f) * CTR[46] + (-3.0f) * CTR[47] +    \
            (8.0f) * CTR[48] + (4.0f) * CTR[49] + (4.0f) * CTR[50] +      \
            (2.0f) * CTR[51] + (-8.0f) * CTR[52] + (-4.0f) * CTR[53] +    \
            (-4.0f) * CTR[54] + (-2.0f) * CTR[55] + (4.0f) * CTR[56] +    \
            (2.0f) * CTR[57] + (2.0f) * CTR[58] + (1.0f) * CTR[59] +      \
            (4.0f) * CTR[60] + (2.0f) * CTR[61] + (2.0f) * CTR[62] +      \
            (1.0f) * CTR[63],                                             \
        (-12.0f) * CTR[0] + (12.0f) * CTR[1] + (12.0f) * CTR[2] +         \
            (-12.0f) * CTR[3] + (12.0f) * CTR[4] + (-12.0f) * CTR[5] +    \
            (-12.0f) * CTR[6] + (12.0f) * CTR[7] + (-6.0f) * CTR[8] +     \
            (6.0f) * CTR[9] + (6.0f) * CTR[10] + (-6.0f) * CTR[11] +      \
            (-6.0f) * CTR[12] + (6.0f) * CTR[13] + (6.0f) * CTR[14] +     \
            (-6.0f) * CTR[15] + (-8.0f) * CTR[16] + (8.0f) * CTR[17] +    \
            (-4.0f) * CTR[18] + (4.0f) * CTR[19] + (8.0f) * CTR[20] +     \
            (-8.0f) * CTR[21] + (4.0f) * CTR[22] + (-4.0f) * CTR[23] +    \
            (-6.0f) * CTR[24] + (-6.0f) * CTR[25] + (6.0f) * CTR[26] +    \
            (6.0f) * CTR[27] + (6.0f) * CTR[28] + (6.0f) * CTR[29] +      \
            (-6.0f) * CTR[30] + (-6.0f) * CTR[31] + (-4.0f) * CTR[32] +   \
            (4.0f) * CTR[33] + (-2.0f) * CTR[34] + (2.0f) * CTR[35] +     \
            (-4.0f) * CTR[36] + (4.0f) * CTR[37] + (-2.0f) * CTR[38] +    \
            (2.0f) * CTR[39] + (-3.0f) * CTR[40] + (-3.0f) * CTR[41] +    \
            (3.0f) * CTR[42] + (3.0f) * CTR[43] + (-3.0f) * CTR[44] +     \
            (-3.0f) * CTR[45] + (3.0f) * CTR[46] + (3.0f) * CTR[47] +     \
            (-4.0f) * CTR[48] + (-4.0f) * CTR[49] + (-2.0f) * CTR[50] +   \
            (-2.0f) * CTR[51] + (4.0f) * CTR[52] + (4.0f) * CTR[53] +     \
            (2.0f) * CTR[54] + (2.0f) * CTR[55] + (-2.0f) * CTR[56] +     \
            (-2.0f) * CTR[57] + (-1.0f) * CTR[58] + (-1.0f) * CTR[59] +   \
            (-2.0f) * CTR[60] + (-2.0f) * CTR[61] + (-1.0f) * CTR[62] +   \
            (-1.0f) * CTR[63],                                            \
        (4.0f) * CTR[0] + (-4.0f) * CTR[2] + (-4.0f) * CTR[4] +           \
            (4.0f) * CTR[6] + (2.0f) * CTR[8] + (-2.0f) * CTR[10] +       \
            (2.0f) * CTR[12] + (-2.0f) * CTR[14] + (2.0f) * CTR[16] +     \
            (2.0f) * CTR[18] + (-2.0f) * CTR[20] + (-2.0f) * CTR[22] +    \
            (1.0f) * CTR[32] + (1.0f) * CTR[34] + (1.0f) * CTR[36] +      \
            (1.0f) * CTR[38],                                             \
        (4.0f) * CTR[24] + (-4.0f) * CTR[26] + (-4.0f) * CTR[28] +        \
            (4.0f) * CTR[30] + (2.0f) * CTR[40] + (-2.0f) * CTR[42] +     \
            (2.0f) * CTR[44] + (-2.0f) * CTR[46] + (2.0f) * CTR[48] +     \
            (2.0f) * CTR[50] + (-2.0f) * CTR[52] + (-2.0f) * CTR[54] +    \
            (1.0f) * CTR[56] + (1.0f) * CTR[58] + (1.0f) * CTR[60] +      \
            (1.0f) * CTR[62],                                             \
        (-12.0f) * CTR[0] + (12.0f) * CTR[1] + (12.0f) * CTR[2] +         \
            (-12.0f) * CTR[3] + (12.0f) * CTR[4] + (-12.0f) * CTR[5] +    \
            (-12.0f) * CTR[6] + (12.0f) * CTR[7] + (-6.0f) * CTR[8] +     \
            (6.0f) * CTR[9] + (6.0f) * CTR[10] + (-6.0f) * CTR[11] +      \
            (-6.0f) * CTR[12] + (6.0f) * CTR[13] + (6.0f) * CTR[14] +     \
            (-6.0f) * CTR[15] + (-6.0f) * CTR[16] + (6.0f) * CTR[17] +    \
            (-6.0f) * CTR[18] + (6.0f) * CTR[19] + (6.0f) * CTR[20] +     \
            (-6.0f) * CTR[21] + (6.0f) * CTR[22] + (-6.0f) * CTR[23] +    \
            (-8.0f) * CTR[24] + (-4.0f) * CTR[25] + (8.0f) * CTR[26] +    \
            (4.0f) * CTR[27] + (8.0f) * CTR[28] + (4.0f) * CTR[29] +      \
            (-8.0f) * CTR[30] + (-4.0f) * CTR[31] + (-3.0f) * CTR[32] +   \
            (3.0f) * CTR[33] + (-3.0f) * CTR[34] + (3.0f) * CTR[35] +     \
            (-3.0f) * CTR[36] + (3.0f) * CTR[37] + (-3.0f) * CTR[38] +    \
            (3.0f) * CTR[39] + (-4.0f) * CTR[40] + (-2.0f) * CTR[41] +    \
            (4.0f) * CTR[42] + (2.0f) * CTR[43] + (-4.0f) * CTR[44] +     \
            (-2.0f) * CTR[45] + (4.0f) * CTR[46] + (2.0f) * CTR[47] +     \
            (-4.0f) * CTR[48] + (-2.0f) * CTR[49] + (-4.0f) * CTR[50] +   \
            (-2.0f) * CTR[51] + (4.0f) * CTR[52] + (2.0f) * CTR[53] +     \
            (4.0f) * CTR[54] + (2.0f) * CTR[55] + (-2.0f) * CTR[56] +     \
            (-1.0f) * CTR[57] + (-2.0f) * CTR[58] + (-1.0f) * CTR[59] +   \
            (-2.0f) * CTR[60] + (-1.0f) * CTR[61] + (-2.0f) * CTR[62] +   \
            (-1.0f) * CTR[63],                                            \
        (8.0f) * CTR[0] + (-8.0f) * CTR[1] + (-8.0f) * CTR[2] +           \
            (8.0f) * CTR[3] + (-8.0f) * CTR[4] + (8.0f) * CTR[5] +        \
            (8.0f) * CTR[6] + (-8.0f) * CTR[7] + (4.0f) * CTR[8] +        \
            (-4.0f) * CTR[9] + (-4.0f) * CTR[10] + (4.0f) * CTR[11] +     \
            (4.0f) * CTR[12] + (-4.0f) * CTR[13] + (-4.0f) * CTR[14] +    \
            (4.0f) * CTR[15] + (4.0f) * CTR[16] + (-4.0f) * CTR[17] +     \
            (4.0f) * CTR[18] + (-4.0f) * CTR[19] + (-4.0f) * CTR[20] +    \
            (4.0f) * CTR[21] + (-4.0f) * CTR[22] + (4.0f) * CTR[23] +     \
            (4.0f) * CTR[24] + (4.0f) * CTR[25] + (-4.0f) * CTR[26] +     \
            (-4.0f) * CTR[27] + (-4.0f) * CTR[28] + (-4.0f) * CTR[29] +   \
            (4.0f) * CTR[30] + (4.0f) * CTR[31] + (2.0f) * CTR[32] +      \
            (-2.0f) * CTR[33] + (2.0f) * CTR[34] + (-2.0f) * CTR[35] +    \
            (2.0f) * CTR[36] + (-2.0f) * CTR[37] + (2.0f) * CTR[38] +     \
            (-2.0f) * CTR[39] + (2.0f) * CTR[40] + (2.0f) * CTR[41] +     \
            (-2.0f) * CTR[42] + (-2.0f) * CTR[43] + (2.0f) * CTR[44] +    \
            (2.0f) * CTR[45] + (-2.0f) * CTR[46] + (-2.0f) * CTR[47] +    \
            (2.0f) * CTR[48] + (2.0f) * CTR[49] + (2.0f) * CTR[50] +      \
            (2.0f) * CTR[51] + (-2.0f) * CTR[52] + (-2.0f) * CTR[53] +    \
            (-2.0f) * CTR[54] + (-2.0f) * CTR[55] + (1.0f) * CTR[56] +    \
            (1.0f) * CTR[57] + (1.0f) * CTR[58] + (1.0f) * CTR[59] +      \
            (1.0f) * CTR[60] + (1.0f) * CTR[61] + (1.0f) * CTR[62] +      \
            (1.0f) * CTR[63]                                              \
  }

#define VKL_STENCIL_TRICUBIC_RES 4
#define VKL_STENCIL_TRICUBIC_SIZE 64
#define VKL_STENCIL_TRICUBIC_OFFSETS                                          \
  {                                                                           \
    {-1, -1, -1}, {-1, -1, 0}, {-1, -1, 1}, {-1, -1, 2}, {-1, 0, -1},         \
        {-1, 0, 0}, {-1, 0, 1}, {-1, 0, 2}, {-1, 1, -1}, {-1, 1, 0},          \
        {-1, 1, 1}, {-1, 1, 2}, {-1, 2, -1}, {-1, 2, 0}, {-1, 2, 1},          \
        {-1, 2, 2}, {0, -1, -1}, {0, -1, 0}, {0, -1, 1}, {0, -1, 2},          \
        {0, 0, -1}, {0, 0, 0}, {0, 0, 1}, {0, 0, 2}, {0, 1, -1}, {0, 1, 0},   \
        {0, 1, 1}, {0, 1, 2}, {0, 2, -1}, {0, 2, 0}, {0, 2, 1}, {0, 2, 2},    \
        {1, -1, -1}, {1, -1, 0}, {1, -1, 1}, {1, -1, 2}, {1, 0, -1},          \
        {1, 0, 0}, {1, 0, 1}, {1, 0, 2}, {1, 1, -1}, {1, 1, 0}, {1, 1, 1},    \
        {1, 1, 2}, {1, 2, -1}, {1, 2, 0}, {1, 2, 1}, {1, 2, 2}, {2, -1, -1},  \
        {2, -1, 0}, {2, -1, 1}, {2, -1, 2}, {2, 0, -1}, {2, 0, 0}, {2, 0, 1}, \
        {2, 0, 2}, {2, 1, -1}, {2, 1, 0}, {2, 1, 1}, {2, 1, 2}, {2, 2, -1},   \
        {2, 2, 0}, {2, 2, 1}, {2, 2, 2},                                      \
  }

// In the macros below, x, y, z must be in [0, 1].

#define __vkl_tricubic_spl(SPL, x, y, z)                               \
  SPL[((x + 1) * VKL_STENCIL_TRICUBIC_RES * VKL_STENCIL_TRICUBIC_RES + \
       (y + 1) * VKL_STENCIL_TRICUBIC_RES + (z + 1))]

#define __vkl_tricubic_ddx(SPL, x, y, z)           \
  (0.5f * (-__vkl_tricubic_spl(SPL, x - 1, y, z) + \
           __vkl_tricubic_spl(SPL, x + 1, y, z)))

#define __vkl_tricubic_ddy(SPL, x, y, z)           \
  (0.5f * (-__vkl_tricubic_spl(SPL, x, y - 1, z) + \
           __vkl_tricubic_spl(SPL, x, y + 1, z)))

#define __vkl_tricubic_ddz(SPL, x, y, z)           \
  (0.5f * (-__vkl_tricubic_spl(SPL, x, y, z - 1) + \
           __vkl_tricubic_spl(SPL, x, y, z + 1)))

#define __vkl_tricubic_d2dxdy(SPL, x, y, z)            \
  (0.25f * (__vkl_tricubic_spl(SPL, x - 1, y - 1, z) - \
            __vkl_tricubic_spl(SPL, x - 1, y + 1, z) - \
            __vkl_tricubic_spl(SPL, x + 1, y - 1, z) + \
            __vkl_tricubic_spl(SPL, x + 1, y + 1, z)))

#define __vkl_tricubic_d2dxdz(SPL, x, y, z)            \
  (0.25f * (__vkl_tricubic_spl(SPL, x - 1, y, z - 1) - \
            __vkl_tricubic_spl(SPL, x - 1, y, z + 1) - \
            __vkl_tricubic_spl(SPL, x + 1, y, z - 1) + \
            __vkl_tricubic_spl(SPL, x + 1, y, z + 1)))

#define __vkl_tricubic_d2dydz(SPL, x, y, z)            \
  (0.25f * (__vkl_tricubic_spl(SPL, x, y - 1, z - 1) - \
            __vkl_tricubic_spl(SPL, x, y - 1, z + 1) - \
            __vkl_tricubic_spl(SPL, x, y + 1, z - 1) + \
            __vkl_tricubic_spl(SPL, x, y + 1, z + 1)))

#define __vkl_tricubic_d3dxdydz(SPL, x, y, z)                \
  (0.125f * (-__vkl_tricubic_spl(SPL, x - 1, y - 1, z - 1) + \
             __vkl_tricubic_spl(SPL, x - 1, y - 1, z + 1) +  \
             __vkl_tricubic_spl(SPL, x - 1, y + 1, z - 1) -  \
             __vkl_tricubic_spl(SPL, x - 1, y + 1, z + 1) +  \
             __vkl_tricubic_spl(SPL, x + 1, y - 1, z - 1) -  \
             __vkl_tricubic_spl(SPL, x + 1, y - 1, z + 1) -  \
             __vkl_tricubic_spl(SPL, x + 1, y + 1, z - 1) +  \
             __vkl_tricubic_spl(SPL, x + 1, y + 1, z + 1)))

#define __vkl_tricubic_constraints_array(SPL)                               \
  {                                                                         \
    __vkl_tricubic_spl(SPL, 0, 0, 0), __vkl_tricubic_spl(SPL, 0, 0, 1),     \
        __vkl_tricubic_spl(SPL, 0, 1, 0), __vkl_tricubic_spl(SPL, 0, 1, 1), \
        __vkl_tricubic_spl(SPL, 1, 0, 0), __vkl_tricubic_spl(SPL, 1, 0, 1), \
        __vkl_tricubic_spl(SPL, 1, 1, 0), __vkl_tricubic_spl(SPL, 1, 1, 1), \
                                                                            \
        __vkl_tricubic_ddx(SPL, 0, 0, 0), __vkl_tricubic_ddx(SPL, 0, 0, 1), \
        __vkl_tricubic_ddx(SPL, 0, 1, 0), __vkl_tricubic_ddx(SPL, 0, 1, 1), \
        __vkl_tricubic_ddx(SPL, 1, 0, 0), __vkl_tricubic_ddx(SPL, 1, 0, 1), \
        __vkl_tricubic_ddx(SPL, 1, 1, 0), __vkl_tricubic_ddx(SPL, 1, 1, 1), \
                                                                            \
        __vkl_tricubic_ddy(SPL, 0, 0, 0), __vkl_tricubic_ddy(SPL, 0, 0, 1), \
        __vkl_tricubic_ddy(SPL, 0, 1, 0), __vkl_tricubic_ddy(SPL, 0, 1, 1), \
        __vkl_tricubic_ddy(SPL, 1, 0, 0), __vkl_tricubic_ddy(SPL, 1, 0, 1), \
        __vkl_tricubic_ddy(SPL, 1, 1, 0), __vkl_tricubic_ddy(SPL, 1, 1, 1), \
                                                                            \
        __vkl_tricubic_ddz(SPL, 0, 0, 0), __vkl_tricubic_ddz(SPL, 0, 0, 1), \
        __vkl_tricubic_ddz(SPL, 0, 1, 0), __vkl_tricubic_ddz(SPL, 0, 1, 1), \
        __vkl_tricubic_ddz(SPL, 1, 0, 0), __vkl_tricubic_ddz(SPL, 1, 0, 1), \
        __vkl_tricubic_ddz(SPL, 1, 1, 0), __vkl_tricubic_ddz(SPL, 1, 1, 1), \
                                                                            \
        __vkl_tricubic_d2dxdy(SPL, 0, 0, 0),                                \
        __vkl_tricubic_d2dxdy(SPL, 0, 0, 1),                                \
        __vkl_tricubic_d2dxdy(SPL, 0, 1, 0),                                \
        __vkl_tricubic_d2dxdy(SPL, 0, 1, 1),                                \
        __vkl_tricubic_d2dxdy(SPL, 1, 0, 0),                                \
        __vkl_tricubic_d2dxdy(SPL, 1, 0, 1),                                \
        __vkl_tricubic_d2dxdy(SPL, 1, 1, 0),                                \
        __vkl_tricubic_d2dxdy(SPL, 1, 1, 1),                                \
                                                                            \
        __vkl_tricubic_d2dxdz(SPL, 0, 0, 0),                                \
        __vkl_tricubic_d2dxdz(SPL, 0, 0, 1),                                \
        __vkl_tricubic_d2dxdz(SPL, 0, 1, 0),                                \
        __vkl_tricubic_d2dxdz(SPL, 0, 1, 1),                                \
        __vkl_tricubic_d2dxdz(SPL, 1, 0, 0),                                \
        __vkl_tricubic_d2dxdz(SPL, 1, 0, 1),                                \
        __vkl_tricubic_d2dxdz(SPL, 1, 1, 0),                                \
        __vkl_tricubic_d2dxdz(SPL, 1, 1, 1),                                \
                                                                            \
        __vkl_tricubic_d2dydz(SPL, 0, 0, 0),                                \
        __vkl_tricubic_d2dydz(SPL, 0, 0, 1),                                \
        __vkl_tricubic_d2dydz(SPL, 0, 1, 0),                                \
        __vkl_tricubic_d2dydz(SPL, 0, 1, 1),                                \
        __vkl_tricubic_d2dydz(SPL, 1, 0, 0),                                \
        __vkl_tricubic_d2dydz(SPL, 1, 0, 1),                                \
        __vkl_tricubic_d2dydz(SPL, 1, 1, 0),                                \
        __vkl_tricubic_d2dydz(SPL, 1, 1, 1),                                \
                                                                            \
        __vkl_tricubic_d3dxdydz(SPL, 0, 0, 0),                              \
        __vkl_tricubic_d3dxdydz(SPL, 0, 0, 1),                              \
        __vkl_tricubic_d3dxdydz(SPL, 0, 1, 0),                              \
        __vkl_tricubic_d3dxdydz(SPL, 0, 1, 1),                              \
        __vkl_tricubic_d3dxdydz(SPL, 1, 0, 0),                              \
        __vkl_tricubic_d3dxdydz(SPL, 1, 0, 1),                              \
        __vkl_tricubic_d3dxdydz(SPL, 1, 1, 0),                              \
        __vkl_tricubic_d3dxdydz(SPL, 1, 1, 1)                               \
  }

// Expanding the integer powers is much faster than using pow!
#define __vkl_tricubic_powers(X)         \
  {                                      \
    1.f, (X), (X) * (X), (X) * (X) * (X) \
  }

// Evaluate a tricubic polynomial given an offset in [0, 1] and coefficients.

// Uniform path.
inline float VdbSampler_tricubicPolynomial(const float *x,
                                           const float *y,
                                           const float *z,
                                           const float *coefficients)
{
  {
    float result = 0.f;
    for (uint32_t i = 0; i < 4; i++)
      for (uint32_t j = 0; j < 4; j++)
        for (uint32_t k = 0; k < 4; k++)
          result += coefficients[16 * i + 4 * j + k] * x[i] * y[j] * z[k];

    return result;
  }
}

inline vec3f VdbSampler_tricubicPolynomialGradient(const float *x,
                                                   const float *y,
                                                   const float *z,
                                                   const float *coefficients)
{
  vec3f result = make_vec3f(0.f);
  for (unsigned int i = 0; i < 4; ++i)
    for (unsigned int j = 0; j < 4; ++j)
      for (unsigned int k = 0; k < 4; ++k) {
        const float c = coefficients[16 * i + 4 * j + k];
        if (i > 0)
          result.x += c * i * x[i - 1] * y[j] * z[k];
        if (j > 0)
          result.y += c * j * x[i] * y[j - 1] * z[k];
        if (k > 0)
          result.z += c * k * x[i] * y[j] * z[k - 1];
      }
  return result;
}

// Note: These wrappers are necessary for two reasons; They help clean up the
// code below, and also help avoid nested foreach errors!
inline void VdbSampler_traverseVoxelValuesTricubic(
    const VdbSamplerShared *sampler,
    const vec3i &ic,
    uint64 *voxel,
    vec3ui *domainOffset)
{
  assert(!sampler->grid->dense);

  const float time = 0.f;
  __vkl_stencil_dispatch_uniform(TRICUBIC, ic, time, {
    uint64 voxelV;
    vec3ui domainOffsetV;
    VdbSampler_traverse(sampler, icDisp, voxelV, domainOffsetV);

    voxel[tgtIdx]        = voxelV;
    domainOffset[tgtIdx] = domainOffsetV;
  });
}

inline void VdbSampler_computeVoxelValuesTricubic(
    const VdbSamplerShared *sampler,
    const vec3i &ic,
    const uint64 *voxel,
    const vec3ui *domainOffset,
    const float time,
    const uint32 attributeIndex,
    float *sample)
{
  assert(!sampler->grid->dense);

  __vkl_stencil_dispatch_uniform(TRICUBIC, ic, time, {
    const uint64 voxelV        = voxel[tgtIdx];
    const vec3ui domainOffsetV = domainOffset[tgtIdx];
    sample[tgtIdx]             = VdbSampler_sample(
        sampler, voxelV, domainOffsetV, timeDisp, attributeIndex);
  });
}

inline void VdbSampler_computeVoxelValuesTricubic(
    const VdbSamplerShared *sampler,
    const vec3i &ic,
    const float time,
    const uint32 attributeIndex,
    float *sample)
{
  assert(!sampler->grid->dense);

  __vkl_stencil_dispatch_uniform(TRICUBIC, ic, time, {
    sample[tgtIdx] =
        VdbSampler_traverseAndSample(sampler, icDisp, timeDisp, attributeIndex);
  });
}

// Single attribute uniform.
inline float VdbSampler_interpolateTricubic(const VdbSamplerShared *sampler,
                                            const vec3f &indexCoordinates,
                                            const float time,
                                            const uint32 attributeIndex)
{
  assert(!sampler->grid->dense);

  const vec3i ic = make_vec3i(floor(indexCoordinates.x),
                              floor(indexCoordinates.y),
                              floor(indexCoordinates.z));

  float sample[VKL_STENCIL_TRICUBIC_SIZE];
  VdbSampler_computeVoxelValuesTricubic(
      sampler, ic, time, attributeIndex, sample);

  const float constraints[]  = __vkl_tricubic_constraints_array(sample);
  const float coefficients[] = __vkl_tricubic_coefficients(constraints);
  const vec3f delta          = indexCoordinates - make_vec3f(ic);
  const float x[]            = __vkl_tricubic_powers(delta.x);
  const float y[]            = __vkl_tricubic_powers(delta.y);
  const float z[]            = __vkl_tricubic_powers(delta.z);
  return VdbSampler_tricubicPolynomial(x, y, z, coefficients);
}

// Multi attribute uniform.
inline void VdbSampler_interpolateTricubic(const VdbSamplerShared *sampler,
                                           const vec3f &indexCoordinates,
                                           const float time,
                                           const uint32 M,
                                           const uint32 *attributeIndices,
                                           float *samples)
{
  assert(!sampler->grid->dense);

  const vec3i ic = make_vec3i(floor(indexCoordinates.x),
                              floor(indexCoordinates.y),
                              floor(indexCoordinates.z));

  uint64 voxel[VKL_STENCIL_TRICUBIC_SIZE];
  vec3ui domainOffset[VKL_STENCIL_TRICUBIC_SIZE];
  VdbSampler_traverseVoxelValuesTricubic(sampler, ic, voxel, domainOffset);

  const vec3f delta = indexCoordinates - make_vec3f(ic);
  const float x[]   = __vkl_tricubic_powers(delta.x);
  const float y[]   = __vkl_tricubic_powers(delta.y);
  const float z[]   = __vkl_tricubic_powers(delta.z);

  for (unsigned int a = 0; a < M; a++) {
    float sample[VKL_STENCIL_TRICUBIC_SIZE];
    VdbSampler_computeVoxelValuesTricubic(
        sampler, ic, voxel, domainOffset, time, attributeIndices[a], sample);

    const float constraints[]  = __vkl_tricubic_constraints_array(sample);
    const float coefficients[] = __vkl_tricubic_coefficients(constraints);
    samples[a] = VdbSampler_tricubicPolynomial(x, y, z, coefficients);
  }
}

// Gradient uniform
inline vec3f VdbSampler_computeGradientTricubic(const VdbSamplerShared *sampler,
                                                const vec3f &indexCoordinates,
                                                const float &time,
                                                const uint32 attributeIndex)
{
  assert(!sampler->grid->dense);

  const vec3i ic = make_vec3i(floor(indexCoordinates.x),
                              floor(indexCoordinates.y),
                              floor(indexCoordinates.z));

  float s[VKL_STENCIL_TRICUBIC_SIZE];
  VdbSampler_computeVoxelValuesTricubic(sampler, ic, time, attributeIndex, s);

  const float constraints[]  = __vkl_tricubic_constraints_array(s);
  const float coefficients[] = __vkl_tricubic_coefficients(constraints);
  const vec3f delta          = indexCoordinates - make_vec3f(ic);
  const float x[]            = __vkl_tricubic_powers(delta.x);
  const float y[]            = __vkl_tricubic_powers(delta.y);
  const float z[]            = __vkl_tricubic_powers(delta.z);
  return VdbSampler_tricubicPolynomialGradient(x, y, z, coefficients);
}
