/****************************************************************************
 *
 *      mathOptimizations.h: Math aproximations to speed up things
 *      This is part of the yafray package
 *      Copyright (C) 2009 Rodrigo Placencia Vazquez (DarkTide)
 *		Creation date: 2009-03-26
 *
 *		fPow() based on the polynomials approach form Jose Fonséca's blog entry:
 *		Fast SSE2 pow: tables or polynomials?
 *		http://jrfonseca.blogspot.com/2008/09/fast-sse2-pow-tables-or-polynomials.html
 *
 *		fSin() and fCos() based on Fast and Accurate sine/cosine
 *		thread on DevMaster.net forum, posted by Nick
 *		http://www.devmaster.net/forums/showthread.php?t=5784
 *
 ****************************************************************************
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2.1 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#ifndef Y_MATHOPTIMIZATIONS_H
#define Y_MATHOPTIMIZATIONS_H

#include <cmath>
#include <algorithm>
#include "math_utils.h"

// Reference defines, this should be defined by the standard cmath header

//#define M_E		    2.7182818284590452354	/* e */
//#define M_LOG2E	    1.4426950408889634074	/* log_2 e */
//#define M_LOG10E	    0.43429448190325182765	/* log_10 e */
//#define M_LN2		    0.69314718055994530942	/* log_e 2 */
//#define M_LN10		2.30258509299404568402	/* log_e 10 */
//#define M_PI		    3.14159265358979323846  /* pi */
//#define M_PI_2		1.57079632679489661923  /* pi/2 */
//#define M_PI_4		0.78539816339744830962	/* pi/4 */
//#define M_1_PI		0.31830988618379067154	/* 1/pi */
//#define M_2_PI		0.63661977236758134308	/* 2/pi */
//#define M_2_SQRTPI	1.12837916709551257390	/* 2/sqrt(pi) */
//#define M_SQRT2	    1.41421356237309504880	/* sqrt(2) */
//#define M_SQRT1_2	    0.70710678118654752440	/* 1/sqrt(2) */

__BEGIN_YAFRAY

#define M_2PI       6.28318530717958647692 //5286766559 // PI * 2
#define M_PI2       9.86960440108935861882 // PI ^ 2
#define M_1_2PI     0.15915494309189533577 // 1 / (2 * PI)
#define M_4_PI      1.27323954473516268615 // 4 / PI
#define M_4_PI2     0.40528473456935108578 // 4 / PI ^ 2
/* povman add for Autodesk plugins*/
#define M_1_PI_4    0.07957747154594766788 //4441881686257 /* 1/(pi/4) */
#define M_1_PI_8    0.03978873577297383394 //2220940843129 /* 1/(pi/8) */
#if !defined(M_PI_2)
#define M_PI_2		1.57079632679489661923  /* pi/2 */
#endif
#if !defined(M_PI)
#define M_PI		3.14159265358979323846  /* pi */
#endif
#if !defined(M_1_PI)
#define M_1_PI		0.31830988618379067154	/* 1/pi */
#endif
/* end */

#define degToRad(deg) (deg * 0.01745329251994329576922)  // deg * PI / 180
#define radToDeg(rad) (rad * 57.29577951308232087684636) // rad * 180 / PI

//FIXME: All the overloaded double definitions have been added to fix the "white dots" problems and they seem to work fine.
//However several of them should be refined in the future to include constants with the correct precision for the doubles calculations.

#define POLYEXP(x) (float)(x * (x * (x * (x * (x * 1.8775767e-3f + 8.9893397e-3f) + 5.5826318e-2f) + 2.4015361e-1f) + 6.9315308e-1f) + 9.9999994e-1f)
#define POLYLOG(x) (float)(x * (x * (x * (x * (x * -3.4436006e-2f + 3.1821337e-1f) + -1.2315303f) + 2.5988452) + -3.3241990f) + 3.1157899f)

#define f_HI 129.00000f
#define f_LOW -126.99999f

#define LOG_EXP 0x7F800000
#define LOG_MANT 0x7FFFFF

#define CONST_P 0.225f

union bitTwiddler
{
	int i;
	float f;
};

inline float fExp2(float x)
{
	bitTwiddler ipart, fpart;
	bitTwiddler expipart;

	x = std::min(x, f_HI);
	x = std::max(x, f_LOW);

	ipart.i = (int)(x - 0.5f);
	fpart.f = (x - (float)(ipart.i));
	expipart.i = ((ipart.i + 127) << 23);

	return (expipart.f * POLYEXP(fpart.f));
}

inline float fLog2(float x)
{
	bitTwiddler one, i, m, e;

	one.f = 1.0f;
	i.f = x;
	e.f = (float)(((i.i & LOG_EXP) >> 23) - 127);
	m.i = ((i.i & LOG_MANT) | one.i);

	return (POLYLOG(m.f) * (m.f - one.f) + e.f);
}

inline float asmSqrt(float n)
{
    float r = n;

// refine flags because x64 don't accept __asm, tested by 'paultron' on win7 x64 using MSVS 2013
#if defined(_MSC_VER) && !defined(_WIN64)
    __asm
    {
        fld r
        fsqrt
        fstp r
    }
// add explicit case for Clang compilers tested by 'jensverwiebe' & 'Sembei' on OSX x64
// also tested by 'povmaniac' on Ubuntu 12.04 amd64 using Clang 3.5
#elif defined __clang__
    asm(
        "flds %0;"
        "fsqrt;"
        "fstps %0"
        :"=m" (r)
        :"m" (r)
        );
#elif defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
    asm(
        "fld %0;"
        "fsqrt;"
        "fstp %0"
        :"=m" (r)
        :"m" (r)
        );
#else
    // this function is non defined, maybe is 'fSqrt' defined in this same file..?,
    // but if fSqrt is used, a recursive flow is created, making on error of 'runtime stackoverflow',
    // because if FAST_MATH is ON, fSqrt call this fuction 'inline float asmSqrt(float n)'
    // temporary solution: use standar sqrt function declared on math.h.
    // TODO: make some test with black & white dots scenes

    r = sqrt(n); // atm, is use for MSVC compilers under windows x64

#endif
    return r;
}

inline float fPow(float a, float b)
{
#ifdef FAST_MATH
	return fExp2(fLog2(a) * b);
#else
	return pow(a,b);
#endif
}

inline float fLog(float a)
{
#ifdef FAST_MATH
	return fLog2(a) * (float) M_LN2;
#else
	return log(a);
#endif
}

inline float fExp(float a)
{
#ifdef FAST_MATH
	return fExp2((float)M_LOG2E * a);
#else
	return exp(a);
#endif
}

inline float fSqrt(float a)
{
#ifdef FAST_MATH
	return asmSqrt(a);
#else
	return sqrt(a);
#endif
}

inline float fLdexp(float x, int a)
{
#ifdef FAST_MATH
	//return x * fPow(2.0, a);
	return ldexp(x, a);
#else
	return ldexp(x, a);
#endif
}
inline float fSin(float x)
{
#ifdef FAST_TRIG
	if(x > M_2PI || x < -M_2PI) x -= ((int)(x * (float)M_1_2PI)) * (float)M_2PI; //float modulo x % M_2PI
	if(x < -M_PI)
	{
		x += (float)M_2PI;
	}
	else if(x > M_PI)
	{
		x -= (float)M_2PI;
	}

	x = ((float)M_4_PI * x) - ((float)M_4_PI2 * x * std::fabs(x));
	float result = CONST_P * (x * std::fabs(x) - x) + x;
    //Make sure that the function is in the valid range [-1.0,1.0]
	result = inRange(1.0, -1.0, result);
	return result;
#else
	return sin(x);
#endif
}

inline float fCos(float x)
{
#ifdef FAST_TRIG
	return fSin(x + (float)M_PI_2);
#else
	return cos(x);
#endif
}
/*
inline float fAcos(float x)
{
	//checks if variable gets out of domain [-1.0,+1.0], so you get the range limit instead of NaN
	if(x <= -1.0) return((float)M_PI);
	else if(x >= 1.0) return(0.0);
	else return acos(x);
}
*/
static float acosArray[629];
inline float fAcos(float x)
{
	if (x<-1 || x>1)
		return acos(x);
	float x1 = x*100.0f + 100.0f;

	int c = (int)(x1);
	float r = x1 - c;

	float res = acosArray[c] * (1 - r) + acosArray[c + 1] * r;
	return res;
}

inline float fAsin(float x)
{
	//checks if variable gets out of domain [-1.0,+1.0], so you get the range limit instead of NaN
	if(x <= -1.0) return((float)-M_PI_2);
	else if(x >= 1.0) return((float)M_PI_2);
	else return asin(x);
}

__END_YAFRAY

#endif
