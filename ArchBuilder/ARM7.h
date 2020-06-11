#pragma once

#ifndef ARM7_H
#define ARM7_H

#include "Processor.h"

enum ARM {
	UND = 0x00,
	B = 0x01,
	BL = 0x02,
	BX = 0x03,
	BLX = 0xF0,
	BXL = 0x2F,
	AND = 0x04,
	EOR = 0x05,
	SUB = 0x06,
	RSB = 0x07,
	ADD = 0x08,
	ADC = 0x09,
	SBC = 0x0A,
	RSC = 0x0B,
	TST = 0x0C,
	TEQ = 0x0D,
	CMP = 0x0E,
	CMN = 0x0F,
	ORR = 0x10,
	MOV = 0x11,
	BIC = 0x12,
	MVN = 0x13,
	LDR = 0x1D,
	STR = 0x1E,
	MUL = 0x22,
	MLA = 0x23,
	SWP = 0x2E,

	PRINT = 0xFE,
	EXPLAIN = 0xFF
};

class ARM7TDMI : public Processor
{
private:
	std::vector<unsigned int*> garbage; // Used to track memory to be released
	bool Z = 0; // Equal
	bool C = 0; // Unsigned Higher or equal
	bool N = 0; // Unsigned Lower (Negative)
	bool V = 0; // Overflow
	bool THUMB = 0; // THUMB Mode

	void releaseTempPointers()
	{
		while (!garbage.empty())
		{
			delete garbage.back();
			garbage.pop_back();
		}
	}

	unsigned int* makeTempPointer(const unsigned int val)
	{
		garbage.push_back(new unsigned int(val));
		return garbage.back();
	}

	unsigned int* getVariable(const std::string& word)
	{
		if (word.length() <= 1) return makeTempPointer(0);

		switch (word[0])
		{
		case '[': 
		{
			bool WRITE_BACK = word[word.length() - 1] == '!';

			std::string arg = WRITE_BACK ? word.substr(1, word.length() - 3) : word.substr(1, word.length() - 2);
			std::vector<std::string> parsed = parseCommand(arg);
			unsigned int* arg_0 = parsed.size() > 0 ? getVariable(parsed[0]) : r;

			if (parsed.size() > 1)
			{
				unsigned int* arg_1 = getVariable(parsed[1]);
				if (WRITE_BACK)
				{
					*arg_0 += *arg_1;
					return (unsigned int*)mem_map->getPointer((*arg_0));
				}
				else
					return (unsigned int*)mem_map->getPointer((*arg_0) + (*arg_1));
			}
			else
				return (unsigned int*)mem_map->getPointer(*arg_0);
		}

		default:
		case '#': return makeTempPointer(atoi(word.substr(1).c_str()));

		case 'R':
		case 'r':
		{
			unsigned int reg = atoi(word.substr(1).c_str());
			if (reg > 0xF)
			{
				std::cout << "Error: Register id is too large!" << std::endl;
				return r;
			}
			else
				return r + reg;
		}
		}
	}

	const Instruction* getPrintInstruction();

	const Instruction* findInstructionWithMnemonic(std::string mnemonic)
	{
		for (unsigned int i = 0; i < mnemonic.size(); i++)
			mnemonic[i] = std::toupper(mnemonic[i]);

		for (unsigned int i = 0; i < instructions->size(); i++){
			const Instruction* instr = instructions->getInstructionById(i);
			if (instr->mnemonic == mnemonic)
				return instr;
		}

		return instructions->getInstructionById(UND);
	}

public:
	ARM7TDMI(unsigned long clock);
	~ARM7TDMI() {}

	void undefined(unsigned int& op)
	{
		std::cout << "Executing unknown instruction 0x" << std::hex << op << std::endl;
	}

	void readHeader(const ROM& rom)
	{
		const unsigned char* buffer = rom.getROM();

		// 0x00 - Branch Instruction
		std::cout << "\nROM BRANCH CODE: 0x" << std::hex << ((unsigned int*)buffer)[0] << identify(((unsigned int*)buffer)[0]) << std::endl;

		// 0x04 - Logo
		unsigned char* logo = new unsigned char[156];
		for (unsigned int i = 0; i < 156; i++)
			logo[i] = buffer[0x04 + i];

		// 0xA0 - Game Title
		std::cout << std::dec << "TITLE: ";
		for (unsigned int i = 0; i < 12; i++)
			std::cout << buffer[0xA0 + i];
		std::cout << std::endl;

		// 0xAC - Game Code
		std::cout << std::dec << "GAME CODE: ";
		for (unsigned int i = 0; i < 4; i++)
			std::cout << buffer[0xAC + i];
		std::cout << std::endl;

		// 0xB0 - Maker Code
		std::cout << "MAKER: " << *((unsigned short*)(&buffer[0xB0])) << std::endl;

		if (buffer[0xB2] != 0x96) std::cout << "ERROR - INVALID ROM HEADER!" << std::endl;

		std::cout << "CHECKSUM: " << std::dec << (int)buffer[0xBD] << std::endl;
		std::cout << "MULTIPLAY BRANCH CODE: 0x" << std::hex << *((unsigned int*)&buffer[0xC0]) << identify(*((unsigned int*)&buffer[0xC0])) << std::endl;
		std::cout << "BOOT MODE: " << std::dec << (int)buffer[0xC4] << std::endl;
		std::cout << "SLAVE ID #: " << std::dec << (int)buffer[0xC5] << std::endl;
		std::cout << "JOYBUS BRANCH CODE: 0x" << std::hex << *((unsigned int*)&buffer[0xE0]) << identify(*((unsigned int*)&buffer[0xE0])) << std::endl;

		delete[] logo;
		return;
	}

	void start();
	void interpret(std::string line);
	std::string identify(unsigned int opcode);
	void execute(unsigned int opcode);
	void execute(unsigned short opcode);
	const char* getName() { return "ARM7TDMI"; }
};

#endif
