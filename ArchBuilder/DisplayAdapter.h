#pragma once

#ifndef DISPLAY_ADAPTER_H
#define DISPLAY_ADAPTER_H

//#include <windows.h>
//#include <gl/GLU.h>
//#include <gl/GL.h>
//#include "gl.h"

#include <SFML/Graphics.hpp>
#include <thread>
#include "MemoryMap.h"

class DisplayAdaptor
{
private:
    unsigned char* memory;
	std::thread updater;

public:
	DisplayAdaptor(unsigned char* ptr);
	~DisplayAdaptor()
	{
		updater.join();
	}

	void update();
};

#endif
