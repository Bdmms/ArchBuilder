#pragma once

#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <vector>

struct Instruction
{
	const char* mnemonic; 
	const char* description;
	const unsigned int mask;
	const unsigned int code;
	const unsigned int id;
	const unsigned int min_arg;
	Instruction(unsigned int i, const char* mn, const unsigned int min, unsigned int msk, unsigned int op, const char* desc)
		: mnemonic(mn), description(desc), min_arg(min), mask(msk), code(op), id(i) {}
};

struct InstructionSuffix
{
	const char* mnemonic;
	const char* description;
	const unsigned int value;
	InstructionSuffix(const char* mn, const char* desc, unsigned int val) : mnemonic(mn), description(desc), value(val) {}
};

class InstructionSet
{
private:
	const char* name;
	unsigned int opcode_mask;
	unsigned int suffix_mask;
	std::vector<Instruction> set;
	std::vector<InstructionSuffix> suffix;

public:
	InstructionSet(const char* nm, unsigned int mask, unsigned int suffix) : name(nm), opcode_mask(mask), suffix_mask(suffix)
	{
		
	}

	void addSuffix(const char* mnemonic, unsigned int val, const char* desc)
	{
		suffix.push_back(InstructionSuffix(mnemonic, desc, val));
	}

	void addInstruction(const Instruction& instr)
	{
		set.push_back(instr);
	}

	const unsigned int addInstruction(const char* mnemonic, const unsigned int min, const unsigned int mask, const unsigned int code, const char* desc)
	{
		unsigned int id = (unsigned int) set.size();
		set.push_back(Instruction(id, mnemonic, min, mask, code, desc));
		return id;
	}

	const InstructionSuffix getSuffix(unsigned int i) const { return suffix[i]; }
	const Instruction* getInstructionById(unsigned int i) const { return &set[i]; }
	const unsigned int size() const { return (unsigned int) set.size(); }
	const char* getName() const { return name; }
	const unsigned int getOpCodeMask() const { return opcode_mask; }
	const unsigned int getSuffixMask() const { return suffix_mask; }
};

#endif
