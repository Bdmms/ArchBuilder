#include "core.h"

#define AUTO_GARBAGE_COLLECTION

#ifdef AUTO_GARBAGE_COLLECTION
std::vector<const char*> garbage;
#endif

using namespace arc;

void arc::cleanUp()
{
#ifdef AUTO_GARBAGE_COLLECTION
	while (!garbage.empty())
	{
		delete[] garbage.back();
		garbage.pop_back();
	}
#endif
}

constexpr float arc::fpow(const float x, const int p)
{
	float product = 1;
	if(p > 0)
		for (int i = 0; i < p; i++)
			product *= x;
	else
		for (int i = 0; i < -p; i++)
			product /= x;
	return product;
}

constexpr void arc::cprint(const char* str, const u32 sz)
{
	for (u32 i = 0; i < sz; i++)
		if (str[i] > 32 && str[i] < 128)
			std::cout << str[i];
		else
			std::cout << " ";
}

void arc::cprintln(const char* str, const u32 sz)
{
	cprint(str, sz);
	std::cout << std::endl;
}

constexpr bool arc::equals(const char* a, const char* b, const u32 sz)
{
	bool equal = 1;
	for (u32 i = 0; i < sz; i++)
		equal &= (a[i] == b[i]);
	return equal;
}

const char* arc::toHex(const u32 x, const unsigned char& b)
{
	u32 len = (b >> 2) + 3;
	char* hex = new char[len];
	if (len > 2)
	{
		hex[0] = '0';
		hex[1] = 'x';
		for (unsigned char i = 0, bi = b - 4; i < (b >> 2); i++, bi -= 4)
		{
			char l = (x >> bi) & 0xF;
			hex[2 + i] = l < 10 ? ('0' + l) : ('A' + l - 10);
		}
		hex[2 + (b >> 2)] = '\0';
	}
#ifdef AUTO_GARBAGE_COLLECTION
	garbage.push_back(hex);
#endif
	return hex;
}

const char* arc::concat(const char** strArr, const size_t sz)
{
	size_t length = 1;
	size_t* lenArr = new size_t[sz];
	for (size_t i = 0; i < sz; i++)
	{
		lenArr[i] = strlen(strArr[i]);
		length += lenArr[i];
	}

	char* str = new char[length];
	size_t offset = 0;
	for (size_t i = 0; i < sz; i++)
	{
		memcpy(str + offset, strArr[i], lenArr[i]);
		offset += lenArr[i];
	}

	str[length - 1] = '\0';
#ifdef AUTO_GARBAGE_COLLECTION
	garbage.push_back(str);
#endif
	return str;
}

inline void vec8::setoffset(const float& value, const float& offset)
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

inline void vec4::setoffset(const double& value, const double& offset)
{
	vec = _mm256_set_pd(
		value,
		value + offset,
		value + offset * 2.0f,
		value + offset * 3.0f
	);
}

vec8 arc::operator+(const vec8& v1, const vec8& v2) { return _mm256_add_ps(v1.vec, v2.vec); }
vec8 arc::operator-(const vec8& v1, const vec8& v2) { return _mm256_sub_ps(v1.vec, v2.vec); }
vec8 arc::operator/(const vec8& v1, const vec8& v2) { return _mm256_div_ps(v1.vec, v2.vec); }
vec8 arc::operator*(const vec8& v1, const vec8& v2) { return _mm256_mul_ps(v1.vec, v2.vec); }
vec8 arc::operator*(const vec8& vec, const float& value) { return _mm256_mul_ps(vec.vec, _mm256_set1_ps(value)); }

vec4 arc::operator+(const vec4& v1, const vec4& v2) { return _mm256_add_pd(v1.vec, v2.vec); }
vec4 arc::operator-(const vec4& v1, const vec4& v2) { return _mm256_sub_pd(v1.vec, v2.vec); }
vec4 arc::operator/(const vec4& v1, const vec4& v2) { return _mm256_div_pd(v1.vec, v2.vec); }
vec4 arc::operator*(const vec4& v1, const vec4& v2) { return _mm256_mul_pd(v1.vec, v2.vec); }
vec4 arc::operator*(const vec4& vec, const double& value) { return _mm256_mul_pd(vec.vec, _mm256_set1_pd(value)); }