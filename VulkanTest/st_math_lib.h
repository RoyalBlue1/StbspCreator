#pragma once

#include <immintrin.h>

inline __m128 magnitude_ps(__m128 vec) {
	__m128 sqr = _mm_mul_ps(vec,vec);
	__m128 magnitude = _mm_add_ps(sqr,_mm_shuffle_ps(sqr,sqr,_MM_SHUFFLE(2,3,0,1)));
	return _mm_sqrt_ps(_mm_add_ps(magnitude,_mm_shuffle_ps(magnitude,magnitude,_MM_SHUFFLE(0,1,2,3))));
}

inline __m128 dotProduct_ps(__m128 a, __m128 b) {
	__m128 c = _mm_mul_ps(a,b);
	c = _mm_add_ps(c,_mm_shuffle_ps(c,c,_MM_SHUFFLE(2,3,0,1)));
	return _mm_add_ps(c,_mm_shuffle_ps(c,c,_MM_SHUFFLE(0,1,2,3)));
}

inline __m128 normalize_ps(__m128 a) {
	return _mm_div_ps(a,magnitude_ps(a));
}

inline __m128 crossProduct_ps(__m128 u, __m128 v) {
	return _mm_sub_ps(
		_mm_mul_ps(_mm_shuffle_ps(u, u, _MM_SHUFFLE(3, 0, 2, 1)), _mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 1, 0, 2))),
		_mm_mul_ps(_mm_shuffle_ps(u, u, _MM_SHUFFLE(3, 1, 0, 2)), _mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 0, 2, 1))));
}

inline __m128 faceNormal_ps(__m128 v0, __m128 v1, __m128 v2) {
	__m128 u = _mm_sub_ps(v1,v0);
	__m128 v = _mm_sub_ps(v2,v0);
	return normalize_ps(crossProduct_ps(u,v));
}

inline __m128 faceCenter_ps(__m128 v0, __m128 v1, __m128 v2) {
	return _mm_div_ps(_mm_add_ps(_mm_add_ps(v0,v1),v2),_mm_set1_ps(3.f));
}

