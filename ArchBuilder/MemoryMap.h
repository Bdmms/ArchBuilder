#pragma once

#ifndef MEMORY_MAP_H
#define MEMORY_MAP_H

#include "core.h"

class MemoryComp
{
protected:
	const char* name;
	const unsigned int address;
	const unsigned int size;
	unsigned char* memory;
	bool sub_mem;

public:
	unsigned int cs;
	unsigned int cs_mask;
	unsigned int mask;

	MemoryComp(const char* nm, unsigned int add, unsigned int sz) : name(nm), address(add), size(sz), sub_mem(false)
	{
		memory = new unsigned char[size];

		if (size == 0) exit(-1); // Size cannot be 0

		unsigned int bits = -1;
		for (; sz != 0; sz = sz >> 1, bits++)
			if ((sz & 0xFFFFFFFE) && (sz & 1))
				sub_mem = true;

		mask = arc::ipow(2, bits) - 1; // Determines mask applied to the memory
		cs_mask = ~mask;
		cs = address & cs_mask; // Determines mask applied to the memory
	}

	~MemoryComp() { delete[] memory; }

	unsigned char* getPointer(const unsigned int ptr) 
	{ 
		return memory + (ptr & mask); 
	}

	const char* getName() const { return name; }
	unsigned int getCapacity() const { return size; }
	unsigned int getAddress() const { return address; }

	virtual void update() {};
	virtual void printDescription() {
		std::cout << "\n--- " << name << " ---\n";
		std::cout << "ADDRESS: 0x" << std::hex << address << " [cs = 0x" << cs << "]" << std::endl;
	}
};

constexpr unsigned int BUFFER_SIZE = 16777216; // 16 MB
class ROM : public MemoryComp
{
public:
	ROM(const char* nm, unsigned int add, unsigned int sz) : MemoryComp(nm, add, sz) {  }

	void loadROM(const char* path)
	{
		std::cout << "\nLOADING " << path << std::endl;
		std::ifstream stream;
		stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);

		try
		{
			stream.open(path, std::ios::in | std::ios::binary);
			stream.read((char*)memory, BUFFER_SIZE);
			stream.close();
		}
		catch (std::ifstream::failure e) { std::cout << path << " cannot be read!\n"; }
	}

	const unsigned char* getROM() const { return memory; }

	virtual void printDescription() override {
		std::cout << "\n--- " << name << " ---\n";
		std::cout << "ADDRESS: 0x" << std::hex << address << std::endl;
		std::cout << "CAPACITY: " << size / 1024 << "KB\n";
	}
};

class RAM : public MemoryComp
{
public:
	RAM(const char* nm, unsigned int add, unsigned int sz) : MemoryComp(nm, add, sz) { }

	virtual void printDescription() override {
		std::cout << "\n--- " << name << " ---\n";
		std::cout << "ADDRESS: 0x" << std::hex << address << std::endl;
		std::cout << "CAPACITY: " << size / 1024 << "KB\n";
	}
};

class Mirror : public MemoryComp {
public:
	Mirror(const char* nm, unsigned int add, unsigned int sz) : MemoryComp(nm, add, sz) { }
};

class IOPort8 : public MemoryComp{
public:
	IOPort8(const char* nm, unsigned int add) : MemoryComp(nm, add, 1) {}
};

class IOPort16: public MemoryComp{
	unsigned short value;
public:
	IOPort16(const char* nm, unsigned int add) : MemoryComp(nm, add, 2) { value = 0; }
};

class IOPort32 : public MemoryComp{
	unsigned int value;
public:
	IOPort32(const char* nm, unsigned int add) : MemoryComp(nm, add, 4) { value = 0; }
};

class MemoryMap {
private:
	std::vector<MemoryComp*> map;
	unsigned char* defmem;
	unsigned char bits;
	unsigned int address_mask;
	unsigned int size;

public:
	MemoryMap(unsigned int b) : bits(b)
	{
		defmem = new unsigned char;
		size = arc::ipow(2, b);
		address_mask = size - 1;
	}

	~MemoryMap()
	{
		delete defmem;
		for (unsigned int i = 0; i < map.size(); i++)
			delete map[i];
	}

	void addComponent(MemoryComp* comp)
	{
		map.push_back(comp);
	}

	void printDescription() {

		std::cout << std::endl;
		std::cout << "--------------------------------------------------------------\n";
		std::cout << "  NAME\t\t|  ADDRESS    |  CAPACITY	|  CHIP SELECT\n";
		std::cout << "--------------------------------------------------------------\n";

		for (unsigned int i = 0; i < map.size(); i++)
		{
			size_t sz = std::strlen(map[i]->getName());

			std::cout << "  " << map[i]->getName();
			for (size_t j = sz; j < 14; j++)
				std::cout << " ";

			char buffer[arc::HEX_BUFFER_SIZE];
			std::cout << "|  " << arc::toHex(buffer, map[i]->getAddress(), bits) << "  |  " << std::dec;

			if (map[i]->getCapacity() >= 1048576)
				std::cout << (map[i]->getCapacity() / 1048576) << " MB";
			else if(map[i]->getCapacity() >= 1024)
				std::cout << (map[i]->getCapacity() / 1024) << " KB";
			else
				std::cout << map[i]->getCapacity() << " B";

			std::cout << "		|  " << arc::toHex(buffer, map[i]->cs_mask, bits) << std::endl;
		}
	}

	//unsigned char* getPointer(const unsigned int ptr) { return memory + (ptr & address_mask); }

	unsigned char* getPointer(const unsigned int ptr) 
	{ 
		for(unsigned int i = 0; i < map.size(); i++)
			if((ptr & map[i]->cs_mask) == map[i]->cs)
				return map[i]->getPointer(ptr); 
		return defmem;
	}

	unsigned int readInt(const unsigned int address)
	{
		for (unsigned int i = 0; i < map.size(); i++)
			if ((address & map[i]->cs_mask) == map[i]->cs)
				return *((unsigned int*)map[i]->getPointer(address));
		return 0;
	}
};

#endif