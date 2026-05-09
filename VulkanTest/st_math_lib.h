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

inline __m128 abs_ps(__m128 a) {
	return _mm_and_ps(a,_mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF)));
}

inline __m128 nr_rcp_ps(__m128 x)
{
	__m128 a = _mm_rcp_ps(x);
	__m128 b = _mm_sub_ps(_mm_set1_ps(1.f),_mm_mul_ps(a,x));
	return _mm_add_ps(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(b, b), b), a), a);
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

inline __m128 clamp_ps(__m128 x, __m128 min, __m128 max)
{
	return _mm_max_ps(_mm_min_ps(x,max),min);
}

inline bool boundingBoxCollides(__m128 point, __m128i box) {
	__m128 boxOrigin = _mm_cvtepi32_ps(_mm_srai_epi32(_mm_unpacklo_epi16(box, box), 0x10u));
	__m128 boxExtends = _mm_cvtepi32_ps(_mm_srai_epi32(_mm_unpackhi_epi16(box, box), 0x10u));

	__m128 cos = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(boxOrigin,boxOrigin,_MM_SHUFFLE(3,3,3,3)),
		_mm_set_ps(0,0,-0.000031,-0.000031)),_mm_set_ps(0,1,0,0));
	__m128 sin = _mm_mul_ps(_mm_shuffle_ps(boxExtends, boxExtends, _MM_SHUFFLE(3,3,3,3)), _mm_set_ps(0,0,-0.000031,0.000031));

	__m128 originOffset = _mm_sub_ps(point,boxOrigin);

	__m128 rotatedOriginOffset = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(originOffset, originOffset, _MM_SHUFFLE(2,3,0,1)), sin), _mm_mul_ps(originOffset, cos));
	
	uint32_t mask = _mm_movemask_ps(_mm_cmpgt_ps(abs_ps(rotatedOriginOffset),boxExtends));
	if(mask&0x7)
		return false;

	return true;
}

inline float minDistanceSqrBetweenLineSegments(__m128 p1, __m128 q1,__m128 p2,__m128 q2)
{
	__m128 distance1 = _mm_sub_ps(q1,p1);
	__m128 distance2 = _mm_sub_ps(q2,p2);
	__m128 distanceDiff = _mm_sub_ps(p1,p2);
	float length1 = _mm_cvtss_f32(dotProduct_ps(distance1,distance1));
	float length2 = _mm_cvtss_f32(dotProduct_ps(distance2,distance2));
	float f = _mm_cvtss_f32(dotProduct_ps(distance2,distanceDiff));
	float c = _mm_cvtss_f32(dotProduct_ps(distance1,distanceDiff));

	float fracClosest1= 0.f;
	float fracClosest2 = 0.f;


	bool noDistance1 = length1<=1e-6f;
	bool noDistance2 = length2<=1e-6f;
	if (noDistance1 & noDistance2)
	{
		return _mm_cvtss_f32(dotProduct_ps(distanceDiff,distanceDiff));
	}
	if (noDistance1)
	{
		fracClosest2 = std::clamp(f/length2,0.f,1.f);
	}else if (noDistance2)
	{
		fracClosest1 = std::clamp(-c/length1,0.f,1.f);
	}else
	{
		float b = _mm_cvtss_f32(dotProduct_ps(distance1,distance2));
		float denom = length1*length2 - b*b;
		if (denom!=0.f)
		{
			fracClosest1 = std::clamp(b*f - c*length2,0.f,1.f);
		}
		fracClosest2 = (b*fracClosest1+f)/length2;
		if (fracClosest2 < 0.f)
		{
			fracClosest2 = 0.f;
			fracClosest1 = std::clamp(-c/length1,0.f,1.f);
		}else if (fracClosest2>1.f)
		{
			fracClosest2 = 1.f;
			fracClosest1 = std::clamp((b-c)/length1,0.f,1.f);
		}
	}
	__m128 closestPoint1 = _mm_add_ps(p1,_mm_mul_ps(distance1,_mm_set1_ps(fracClosest1)));
	__m128 closestPoint2 = _mm_add_ps(p2,_mm_mul_ps(distance2,_mm_set1_ps(fracClosest2)));
	return _mm_cvtss_f32(dotProduct_ps(closestPoint1,closestPoint2));

}


inline float minDistanceSqrTriangleVsLineSegment(__m128 a,__m128 b,__m128 c,__m128 p,__m128 q)
{
	__m128 ab = _mm_sub_ps(b,a);
	__m128 ac = _mm_sub_ps(c,a);
	__m128 ap = _mm_sub_ps(p,a);
	__m128 aq = _mm_sub_ps(q,a);
}

inline bool boundingBoxCollidesPill(__m128 a,__m128 b,float radius, __m128i box) {

	//convert box packed as shorts to packed floats
	__m128 boxOrigin = _mm_cvtepi32_ps(_mm_srai_epi32(_mm_unpacklo_epi16(box, box), 0x10u));
	__m128 boxExtends = _mm_cvtepi32_ps(_mm_srai_epi32(_mm_unpackhi_epi16(box, box), 0x10u));

	//extract rotation
	__m128 cos = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(boxOrigin,boxOrigin,_MM_SHUFFLE(3,3,3,3)),
		_mm_set_ps(0,0,-0.000031,-0.000031)),_mm_set_ps(0,1,0,0));
	__m128 sin = _mm_mul_ps(_mm_shuffle_ps(boxExtends, boxExtends, _MM_SHUFFLE(3,3,3,3)), _mm_set_ps(0,0,-0.000031,0.000031));

	__m128 aOffset = _mm_sub_ps(a,boxOrigin);
	__m128 bOffset = _mm_sub_ps(b,boxOrigin);

	__m128 aRotated = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(aOffset, aOffset, _MM_SHUFFLE(2,3,0,1)), sin), _mm_mul_ps(aOffset, cos));
	__m128 bRotated = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(bOffset, bOffset, _MM_SHUFFLE(2,3,0,1)), sin), _mm_mul_ps(bOffset, cos));

	//early exit if either point is within the box
	if ((_mm_movemask_ps(_mm_cmpgt_ps(abs_ps(aRotated),boxExtends)) & 0x7) == 0)
		return true;
	if ((_mm_movemask_ps(_mm_cmpgt_ps(abs_ps(bRotated),boxExtends)) & 0x7) == 0)
		return true;

	__m128 direction = _mm_sub_ps(bRotated,aRotated);
	if (_mm_cvtss_f32(dotProduct_ps(direction,direction)) < 1e-6f)
		return false;

	__m128 boxExtendsInv = _mm_mul_ps(boxExtends,_mm_set1_ps(-1.f));

	int noSize = _mm_movemask_ps(_mm_cmplt_ps(abs_ps(direction),_mm_set1_ps(1e-6f)));
	if (noSize & 0x7)
	{
		int lessThan = _mm_movemask_ps(_mm_cmplt_ps(aRotated,boxExtendsInv));
		int greaterThan = _mm_movemask_ps(_mm_cmpgt_ps(aRotated,boxExtends));
		if (noSize & (lessThan | greaterThan) & 0x7)
			return false;
	}

	__m128 directionRcp = nr_rcp_ps(direction);
	__m128 t1 = _mm_mul_ps(_mm_sub_ps(boxExtendsInv,aRotated),directionRcp);
	__m128 t2 = _mm_mul_ps(_mm_sub_ps(boxExtends,aRotated),directionRcp);
	__m128 tMax = _mm_max_ps(_mm_max_ps(t1,t2),_mm_setzero_ps());
	__m128 tMin = _mm_min_ps(_mm_min_ps(t1,t2),_mm_set1_ps(1.f));

	tMax = _mm_max_ps(tMax,_mm_shuffle_ps(tMax,tMax,_MM_SHUFFLE(2,2,0,1)));
	tMax = _mm_max_ps(tMax,_mm_shuffle_ps(tMax,tMax,_MM_SHUFFLE(0,1,2,2)));

	tMin = _mm_min_ps(tMin,_mm_shuffle_ps(tMin,tMin,_MM_SHUFFLE(2,2,0,1)));
	tMin = _mm_min_ps(tMin,_mm_shuffle_ps(tMin,tMin,_MM_SHUFFLE(0,1,2,2)));

	if (_mm_movemask_ps(_mm_cmpgt_ps(tMin,tMax))&7)
		return false;

	return true;
}



inline __m128 loadVector3(Vector3* vec) {
	if (__builtin_cpu_supports("avx"))
	{
		return _mm_maskload_ps(&vec->x,_mm_set_epi32(0,~0,~0,~0));
	}else
	{
		return _mm_set_ps(0,vec->z,vec->y,vec->x);
	}

}