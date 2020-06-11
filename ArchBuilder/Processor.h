#pragma once

#ifndef PROCESSOR_H
#define PROCESSOR_H

#include "InstructionSet.h"
#include "MemoryMap.h"
#include <string>

class Processor
{
protected:
	MemoryMap* mem_map;
	InstructionSet* instructions;
	unsigned char bits;
	unsigned int* r;
	unsigned int num_reg;
	unsigned long clock_time;

	// Helper for converting to hex
	char* toHex(unsigned int x) {
		char* hex = new char[3 + (long)(bits >> 2)];
		hex[0] = '0';
		hex[1] = 'x';
		for (unsigned char i = 0, bi = bits - 4; i < bits >> 2; i++, bi -= 4)
		{
			char l = (x >> bi) & 0xF;
			hex[2 + i] = l < 10 ? ('0' + l) : ('A' + l - 10);
		}
		hex[2 + (bits >> 2)] = '\0';
		return hex;
	}

	// Helper for removing characters from strings
	void remove(std::string& s, char c)
	{
		size_t c_pos = s.find(c);
		while (c_pos != std::string::npos) {
			s.replace(c_pos, 1, "");
			c_pos = s.find(c);
		}
	}

	size_t findClosingBracket(const std::string& line, const size_t pos)
	{
		for (size_t i = pos + 1; i < line.length(); i++)
		{
			if (line[i] == '[')
				i = findClosingBracket(line, i);
			else if (line[i] == ']')
				return i;
		}

		return 0xFFFFFFFF;
	}

	// Helper for parsing a script line
	const std::vector<std::string> parseCommand(std::string command)
	{
		remove(command, ',');
		remove(command, ';');

		std::vector<std::string> parsedWords;
		size_t l_index = 0;
		size_t r_index = command.find(' ');

		// Continues to parse until their are no more spaces in string
		while (r_index != std::string::npos)
		{
			parsedWords.push_back(command.substr(l_index, r_index - l_index));
			l_index = r_index + 1;

			if (command[l_index] == '[')
			{
				r_index = findClosingBracket(command, l_index);

				if (r_index > command.length())
					r_index = std::string::npos;
				else
					r_index = command.find(' ', r_index);
			}
			else
				r_index = command.find(' ', l_index);
		}

		parsedWords.push_back(command.substr(l_index));
		return parsedWords;
	}

public:
	Processor(unsigned char b, unsigned long clock, unsigned int regs)
		: bits(b), num_reg(regs), clock_time(clock), mem_map(new MemoryMap(0))
	{
		instructions = nullptr;
		r = new unsigned int[num_reg];
	}

	void setMemoryMap(MemoryMap* map)
	{
		delete mem_map;
		mem_map = map;
	}

	void loadInstructionSet(InstructionSet* set) 
	{
		instructions = set;

		std::cout << "Loading " << set->getName() << "...\n";
		std::cout << "Configuring processor...\n\n";
	}

	~Processor()
	{
		delete[] r;
	}

	inline void printDescription() 
	{
		char* opMsk = toHex(instructions->getOpCodeMask());
		char* sfMsk = toHex(instructions->getSuffixMask());
		std::cout << "--- " << getName() << " ---\n";
		std::cout << "   ARCHITECTURE: " << instructions->getName() << " (" << (int)bits << "-bit)" << std::endl;
		std::cout << "PROCESSOR SPEED: " << 1.0f / ((double)clock_time / 1000) << " MHz\n";
		std::cout << "  NUM REGISTERS: " << num_reg << std::endl;
		std::cout << "NUM INSTRCTIONS: " << instructions->size() << std::endl;
		std::cout << "    OPCODE MASK: " << opMsk << std::endl;
		std::cout << "COND FIELD MASK: " << sfMsk << std::endl;
		delete[] opMsk;
		delete[] sfMsk;
	}

	virtual const char* getName() = 0;
};

#endif