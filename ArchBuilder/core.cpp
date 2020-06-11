#include "core.h"

u32 arc::pow(const int x, const int p)
{
	if (p < 0) return 0;
	u32 product = 1;
	for (unsigned int i = 0; i < p; i++)
		product *= x;
	return product;
}

float arc::fpow(const float x, const int p)
{
	float product = 1;
	if(p > 0)
		for (unsigned int i = 0; i < p; i++)
			product *= x;
	else
		for (unsigned int i = 0; i < -p; i++)
			product /= x;
	return product;
}

char* arc::toHex(const u32 x, const unsigned char& b)
{
	u8 len = 3 + (b >> 2);
	char* hex = new char[len];
	hex[0] = '0';
	hex[1] = 'x';
	for (unsigned char i = 0, bi = b - 4; i < (b >> 2); i++, bi -= 4)
	{
		char l = (x >> bi) & 0xF;
		hex[2 + i] = l < 10 ? ('0' + l) : ('A' + l - 10);
	}
	hex[2 + (b >> 2)] = '\0';
	return hex;
}