#pragma once

#ifndef ARCH_CORE_H
#define ARCH_CORE_H

#include <vector>
#include <iostream>
#include <fstream>

typedef unsigned long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

namespace arc 
{
	u32 pow(const int x, const int p);
	float fpow(const float x, const int p);
	char* toHex(const u32 x, const unsigned char& b);
};

#endif