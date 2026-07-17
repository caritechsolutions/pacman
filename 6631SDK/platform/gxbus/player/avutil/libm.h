/*
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * Replacements for frequently missing libm functions
 */

#ifndef AVUTIL_LIBM_H
#define AVUTIL_LIBM_H

#ifndef LINUX_OS
#include <math.h>
#include "config.h"
#include "attributes.h"

#define NAN -1
#define M_E  2.71828182845904523536
#define M_PI 3.14159265358979323846

#undef cbrtf
#define cbrtf(x) powf(x, 1.0/3.0)

#undef exp2
#define exp2(x) exp((x) * 0.693147180559945)

#undef exp2f
#define exp2f(x) ((float)exp2(x))

#undef llrint
#define llrint(x) ((long long)rint(x))

#undef llrintf
#define llrintf(x) ((long long)rint(x))

#undef log2
#define log2(x) (log(x) * 1.44269504088896340736)

#undef log2f
#define log2f(x) ((float)log2(x))

static av_always_inline av_const long int lrint(double x)
{
    return rint(x);
}

static av_always_inline av_const long int lrintf(float x)
{
    return (int)(rint(x));
}

static av_always_inline av_const double round(double x)
{
    return (x > 0) ? floor(x + 0.5) : ceil(x - 0.5);
}

static av_always_inline av_const float roundf(float x)
{
    return (x > 0) ? floor(x + 0.5) : ceil(x - 0.5);
}

static av_always_inline av_const double trunc(double x)
{
    return (x > 0) ? floor(x) : ceil(x);
}

static av_always_inline av_const float truncf(float x)
{
    return (x > 0) ? floor(x) : ceil(x);
}
#endif
#endif
