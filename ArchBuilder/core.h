#pragma once

#ifndef ARCH_CORE_H
#define ARCH_CORE_H

#include <vector>
#include <list>
#include <iostream>
#include <fstream>
#include <immintrin.h>

typedef unsigned long u64;	// 64-bit Integer
typedef unsigned int u32;	// 32-bit Integer
typedef unsigned short u16;	// 16-bit Integer
typedef unsigned char u8;	// 8-bit Integer

namespace arc 
{
	// Reverses the endian of a 16-bit integer
	constexpr u16 REu16(u16 x) { return ((x & 0xFF00) >> 8) | ((x & 0xFF) << 8); }
	// Reverses the endian of a 32-bit integer
	constexpr u32 REu32(u32 x) { return ((x & 0xFF000000) >> 24) | ((x & 0xFF0000) >> 8) | ((x & 0xFF00) << 8) | ((x & 0xFF) << 24); }
	// Interrprets the 1st 4 characters of a string as an integer
	constexpr u32 ID(const char* str) { return *((const u32*)str); }

	// An integer based exponential function
	constexpr int ipow(const int x, const int p)
	{
		if (p < 0) return 0;
		int product = x;
		for (int i = 1; i < p; i++)
			product *= x;
		return product;
	}

	static char TEMP_ID_BUFFER[5] = "\0\0\0\0";

	inline const char* toString(const u32 id)
	{
		memcpy(TEMP_ID_BUFFER, &id, 4);
		return TEMP_ID_BUFFER;
	}

	constexpr u32 HEX_BUFFER_SIZE = 20;

	// Converts an integer into hexadecimal using a fixed size
	constexpr const char* toHex(char* buffer, const u32 x, const u8 b)
	{
		u32 len = (b >> 2) + 3;
		if (len > 2)
		{
			buffer[0] = '0';
			buffer[1] = 'x';
			for (unsigned char i = 0, bi = b - 4; i < (b >> 2); i++, bi -= 4)
			{
				char l = (x >> bi) & 0xF;
				buffer[2 + i] = l < 10 ? ('0' + l) : ('A' + l - 10);
			}
			buffer[2 + (b >> 2)] = '\0';
		}
		return buffer;
	}

	struct vec8
	{
		__m256 vec;
		vec8() : vec(_mm256_set1_ps(0.0f)) { }
		vec8(const __m256& v) : vec(v) {}
		vec8(const float value) { vec = _mm256_set1_ps(value); }

		inline void setoffset(const float& value, const float& offset)
		{
			vec = _mm256_set_ps(
				value,
				value + offset,
				value + offset * 2.0f,
				value + offset * 3.0f,
				value + offset * 4.0f,
				value + offset * 5.0f,
				value + offset * 6.0f,
				value + offset * 7.0f
			);
		}

		constexpr float& operator[](const u32 i) { return vec.m256_f32[i]; }
		inline void operator*=(const vec8& v) { vec = _mm256_mul_ps(vec, v.vec); }
		inline void operator/=(const vec8& v) { vec = _mm256_div_ps(vec, v.vec); }
		inline void operator+=(const vec8& v) { vec = _mm256_add_ps(vec, v.vec); }
		inline void operator-=(const vec8& v) { vec = _mm256_sub_ps(vec, v.vec); }
		inline void operator=(const float& value) { vec = _mm256_set1_ps(value); }
	};

	struct vec4
	{
		__m256d vec;
		vec4() : vec(_mm256_set1_pd(0.0)) { }
		vec4(const __m256d& v) : vec(v) {}
		vec4(const double value) { vec = _mm256_set1_pd(value); }

		inline void setoffset(const double& value, const double& offset)
		{
			vec = _mm256_set_pd(
				value,
				value + offset,
				value + offset * 2.0f,
				value + offset * 3.0f
			);
		}

		constexpr double& operator[](const u32 i) { return vec.m256d_f64[i]; }
		inline void operator*=(const vec4& v) { vec = _mm256_mul_pd(vec, v.vec); }
		inline void operator/=(const vec4& v) { vec = _mm256_div_pd(vec, v.vec); }
		inline void operator+=(const vec4& v) { vec = _mm256_add_pd(vec, v.vec); }
		inline void operator-=(const vec4& v) { vec = _mm256_sub_pd(vec, v.vec); }
		inline void operator=(const double& value) { vec = _mm256_set1_pd(value); }
	};

	static inline vec8 operator+(const vec8& v1, const vec8& v2) { return _mm256_add_ps(v1.vec, v2.vec); }
	static inline vec8 operator-(const vec8& v1, const vec8& v2) { return _mm256_sub_ps(v1.vec, v2.vec); }
	static inline vec8 operator/(const vec8& v1, const vec8& v2) { return _mm256_div_ps(v1.vec, v2.vec); }
	static inline vec8 operator*(const vec8& v1, const vec8& v2) { return _mm256_mul_ps(v1.vec, v2.vec); }
	static inline vec8 operator*(const vec8& vec, const float& value) { return _mm256_mul_ps(vec.vec, _mm256_set1_ps(value)); }

	static inline vec4 operator+(const vec4& v1, const vec4& v2) { return _mm256_add_pd(v1.vec, v2.vec); }
	static inline vec4 operator-(const vec4& v1, const vec4& v2) { return _mm256_sub_pd(v1.vec, v2.vec); }
	static inline vec4 operator/(const vec4& v1, const vec4& v2) { return _mm256_div_pd(v1.vec, v2.vec); }
	static inline vec4 operator*(const vec4& v1, const vec4& v2) { return _mm256_mul_pd(v1.vec, v2.vec); }
	static inline vec4 operator*(const vec4& vec, const double& value) { return _mm256_mul_pd(vec.vec, _mm256_set1_pd(value)); }
};

#endif