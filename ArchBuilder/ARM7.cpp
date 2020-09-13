
#include "ARM7.h"

// ARM standard
const Instruction ARM_UND(UND, "UND", 0, 0x0E000010, 0x06000010, "Undefined");
const Instruction ARM_B  (B  , "B"  , 1, 0x0F000000, 0x0A000000, "Branch");
const Instruction ARM_BL (BL , "BL" , 1, 0x0F000000, 0x0B000000, "Branch with Link");
const Instruction ARM_BX (BX , "BX" , 1, 0x0FFFFFF0, 0x012FFF10, "Branch with Exchange");
const Instruction ARM_BLX(BLX, "BLX", 1, 0x0FFFFFF0, 0x012FFF30, "Branch with Link and Exchange");
const Instruction ARM_BXL(BXL, "BXL", 1, 0x0F000000, 0x0BA00000, "Branch with Link and Exchange using Immediate");

const Instruction ARM_AND(AND, "AND", 3, 0x0DE00000, 0x00000000, "AND (Rd = Rm & Rs)");
const Instruction ARM_EOR(EOR, "EOR", 3, 0x0DE00000, 0x00200000, "Exclusive OR (Rd = Rm ^ Rs)");
const Instruction ARM_SUB(SUB, "SUB", 3, 0x0DE00000, 0x00400000, "Subtract(Rd = Rm - Rs)");
const Instruction ARM_RSB(RSB, "RSB", 3, 0x0DE00000, 0x00600000, "Reverse subtract (Rd = Rs - Rm)");
const Instruction ARM_ADD(ADD, "ADD", 3, 0x0DE00000, 0x00800000, "Add (Rd = Rm + Rs)");
const Instruction ARM_ADC(ADC, "ADC", 3, 0x0DE00000, 0x00A00000, "Add with carry  (Rd = Rm + Rs + C)");
const Instruction ARM_SBC(SBC, "SBC", 3, 0x0DE00000, 0x00C00000, "Subtract with carry (Rd = Rm - Rs + C - 1)");
const Instruction ARM_RSC(RSC, "RSC", 3, 0x0DE00000, 0x00E00000, "Reverse subtract with carry (Rd = Rs - Rm + C - 1)");
const Instruction ARM_TST(TST, "TST", 2, 0x0DE00000, 0x01000000, "Test bits");
const Instruction ARM_TEQ(TEQ, "TEQ", 2, 0x0DE00000, 0x01200000, "Test bitwise equality");
const Instruction ARM_CMP(CMP, "CMP", 2, 0x0DE00000, 0x01400000, "Compare");
const Instruction ARM_CMN(CMN, "CMN", 2, 0x0DE00000, 0x01600000, "Compare Negative");
const Instruction ARM_ORR(ORR, "ORR", 3, 0x0DE00000, 0x01800000, "OR (Rd = Rm | Rs)");
const Instruction ARM_MOV(MOV, "MOV", 2, 0x0DE00000, 0x01A00000, "Move register or constant (Rd = Rm)");
const Instruction ARM_BIC(BIC, "BIC", 3, 0x0DE00000, 0x01C00000, "Bit Clear (Rd = Rm & ~Rs)");
const Instruction ARM_MVN(MVN, "MVN", 2, 0x0DE00000, 0x01E00000, "Move negative register (Rd = ~Rm)");

const Instruction ARM_MUL(MUL, "MUL", 3, 0x0FE000F0, 0x00000090, "Multiply (Rd = Rm * Rs)");
const Instruction ARM_MLA(MLA, "MLA", 4, 0x0FE000F0, 0x00200090, "Multiply accumulate (Rd = Rm * Rs + Rn)");

const Instruction ARM_LDR(LDR, "LDR", 2, 0x0C500000, 0x04100000, "Load register from memory (word)");
const Instruction ARM_STR(STR, "STR", 2, 0x0C500000, 0x04000000, "Store register to memory (word)");

const Instruction ARM_SWP(SWP, "SWP", 2, 0x0FF00FF0, 0x01000090, "Swap register with memory (word)");

// Additional
const Instruction STD_PRINT(PRINT, "PRINT", 1, 0x00000000, 0xFFFFFFFF, "Prints value (std::cout)");
const Instruction STD_EXPLAIN(EXPLAIN, "EXPLAIN", 1, 0x00000000, 0xFFFFFFFF, "Prints description of of an instruction mnemonic");

const Instruction* ARM7TDMI::getPrintInstruction()
{
	return &STD_PRINT;
}

void ARM7TDMI::interpret(std::string line)
{
	releaseTempPointers();
	const std::vector<std::string> parsed = parseCommand(line);
	const Instruction* instr = findInstructionWithMnemonic(parsed.size() > 0 ? parsed[0] : "null");

	if (parsed.size() - 1 < instr->min_arg)
	{
		std::cout << "Error: missing arguments!" << std::endl;
		return;
	}

	unsigned int tmp;
	unsigned int* Rd = parsed.size() > 1 ? getVariable(parsed[1]) : r;
	unsigned int* Rm = parsed.size() > 2 ? getVariable(parsed[2]) : r;
	unsigned int* Rs = parsed.size() > 3 ? getVariable(parsed[3]) : r;
	unsigned int* Rn = parsed.size() > 4 ? getVariable(parsed[4]) : r;

	switch (instr->id) 
	{
	default: std::cout << "Error: unknown command!" << std::endl;

	case B	: r[15] = *Rd; return;
	case BL : r[14] = r[15];  r[15] = *Rd; return;
	case BX : r[15] = *Rd; THUMB = r[15] & 0x1; return;
	case BLX: r[14] = r[15]; THUMB = r[15] & 0x1; r[15] = *Rd; return;
	case AND: *Rd = *Rm & *Rs; return;
	case SUB: *Rd = *Rm - *Rs; return;
	case ADD: *Rd = *Rm + *Rs; return;
	case ADC: *Rd = *Rm + *Rs + V; return;
	case SBC: *Rd = *Rm - *Rs + V - 1; return;
	case RSC: *Rd = *Rs - *Rm + V - 1; return;

	case TST: 
		tmp = (*Rd) & (*Rm); 
		Z = tmp == 0;
		C = tmp >= 0;
		N = tmp < 0;
		return; 

	case TEQ: 
		tmp = (*Rd) ^ (*Rm); 
		Z = tmp == 0;
		C = tmp >= 0;
		N = tmp < 0;
		return; 

	case CMP:
		tmp = (*Rd) - (*Rm);
		Z = tmp == 0;
		C = tmp >= 0;
		N = tmp < 0;
		V = tmp > *Rd;
		return;

	case CMN:
		tmp = (*Rd) + (*Rm);
		Z = tmp == 0;
		C = tmp >= 0;
		N = tmp < 0;
		V = tmp < *Rd;
		return;

	case ORR: *Rd = *Rm | *Rs; return;
	case MOV: *Rd = *Rm; return;
	case BIC: *Rd = *Rm & ~(*Rs); return;
	case MVN: *Rd = ~(*Rm); return;

	case MUL: *Rd = (*Rm) * (*Rs); return;
	case MLA: *Rd = (*Rm) * (*Rs) + (*Rn); return;

	case LDR: 
		*Rd = *Rm; 
		//if (parsed.size() > 3) *Rm += *Rs; // POST-FIX
		return;
	case STR: 
		*Rm = *Rd; 
		//if (parsed.size() > 3) *Rm += *Rs; // POST-FIX
		return;

	case SWP: 
		tmp = *Rd;
		*Rd = *Rm;
		*Rm = tmp;
		return;

	// OTHER ACTIONS
	case PRINT: 
		std::cout << arc::toHex(*Rd, bits) << std::endl; return; // Changed to arc may have caused errors
		//if(parsed.size() > 2)* Rd += *Rm; // POST-FIX
		return;

	case EXPLAIN: std::cout << findInstructionWithMnemonic(parsed[1])->description << std::endl;
	}
}

std::string ARM7TDMI::identify(unsigned int op)
{
	for (unsigned int i = 0; i < instructions->size(); i++)
	{
		const Instruction* in = instructions->getInstructionById(i);
		if ((op & in->mask) == in->code)
		{
			std::string str = " [";
			str += in->mnemonic;
			str += " - ";
			str += in->description;
			str += "]";
			return str;
		}
	}

	return " [UNKNOWN INSTRUCTION]";
}

void ARM7TDMI::execute(unsigned short op)
{

}

void ARM7TDMI::execute(unsigned int op)
{
	unsigned int cond = op & instructions->getSuffixMask();

	switch (cond)
	{
	case 0x00000000: if (Z) break; else return;
	case 0x10000000: if (Z) return; else break;
	case 0x20000000: if (C) break; else return;
	case 0x30000000: if (C) return; else break;
	case 0x40000000: if (N) break; else return;
	case 0x50000000: if (N) return; else break;
	case 0x60000000: if (V) break; else return;
	case 0x70000000: if (V) return; else break;
	case 0x80000000: if (C && !Z) break; else return;
	case 0x90000000: if (!C || Z) break; else return;
	case 0xA0000000: if (N == V) break; else return;
	case 0xB0000000: if (N != V) break; else return;
	case 0xC0000000: if (!Z && (N == V)) break; else return;
	case 0xD0000000: if (Z || (N != V)) break; else return;
	case 0xE0000000: break;
	case 0xF0000000: return;
	}

	//unsigned int id = 0x06000010;
	for (unsigned int i = 0; i < instructions->size(); i++)
	{
		const Instruction* in = instructions->getInstructionById(i);
		if ((op & in->mask) == in->code)
		{
			switch (i)
			{
			case UND: undefined(op);
			case BL:
				r[14] = r[15] + 4;
			case B: 
				r[15] += (op & 0x800000 ? op & 0x7FFFFF : -(int((op & 0x7FFFFF) << 2)));
				break;

			case BLX:
				r[14] = r[15] + 4;
			case BX:
				THUMB = r[op & 0xF] & 0x01;
				r[15] = r[op & 0xF] & 0xFFFFFFFE;
				break;
			}

			return;
		}
	}

	undefined(op);
}

ARM7TDMI::ARM7TDMI(unsigned long clock) : Processor(32, clock, 16)
{
	InstructionSet* set = new InstructionSet("ARM7", 0x0FFFFFFF, 0xF0000000);

	set->addSuffix("EQ", 0x00000000, "Z HIGH (equal)");
	set->addSuffix("NE", 0x10000000, "Z LOW (not equal)");
	set->addSuffix("CS", 0x20000000, "C HIGH (unsigned higher or same)");
	set->addSuffix("CC", 0x30000000, "C LOW (unsigned lower)");
	set->addSuffix("MI", 0x40000000, "N HIGH (negative)");
	set->addSuffix("PL", 0x50000000, "N LOW (positive or zero)");
	set->addSuffix("VS", 0x60000000, "V HIGH (overflow)");
	set->addSuffix("VC", 0x70000000, "V LOW (no overflow)");
	set->addSuffix("HI", 0x80000000, "C HIGH AND Z LOW (unsigned higher)");
	set->addSuffix("LS", 0x90000000, "C LOW OR Z HIGH (unsigned lower or equal)");
	set->addSuffix("GE", 0xA0000000, "N == V (greater or equal)");
	set->addSuffix("LT", 0xB0000000, "N != V (less than)");
	set->addSuffix("GT", 0xC0000000, "Z LOW AND N == V (greater than)");
	set->addSuffix("LE", 0xD0000000, "Z HIGH OR N != V (less than or equal)");
	set->addSuffix("AL", 0xE0000000, "Always");
	set->addSuffix("NV", 0xF0000000, "Never");

	set->addInstruction(ARM_UND);
	set->addInstruction(ARM_B);
	set->addInstruction(ARM_BL);
	set->addInstruction(ARM_BX);
	set->addInstruction(ARM_BLX);
	set->addInstruction(ARM_AND);
	set->addInstruction(ARM_EOR);
	set->addInstruction(ARM_SUB);
	set->addInstruction(ARM_RSB);
	set->addInstruction(ARM_ADD);
	set->addInstruction(ARM_ADC);
	set->addInstruction(ARM_SBC);
	set->addInstruction(ARM_RSC);
	set->addInstruction(ARM_TST);
	set->addInstruction(ARM_TEQ); 
	set->addInstruction(ARM_CMP);
	set->addInstruction(ARM_CMN);
	set->addInstruction(ARM_ORR);
	set->addInstruction(ARM_MOV);
	set->addInstruction(ARM_BIC);
	set->addInstruction(ARM_MVN);
	/*
	mapper[set->addInstruction("MRS", 0x0FBF0FFF, 0x010F0000, "Move PSR status/flags to register")] = &ARM7TDMI::undefined;
	mapper[set->addInstruction("MSR", 0x0FBFFFF0, 0x0129F000, "Move register to PSR status/flags")] = &ARM7TDMI::undefined;
	mapper[set->addInstruction("MSR_", 0x0DBFF000, 0x0129F000, "Move register or immediate to PSR flag bits only")] = &ARM7TDMI::undefined;
	*/
	set->addInstruction(ARM_MUL);
	set->addInstruction(ARM_MLA);
	/*
	mapper[set->addInstruction("SMLAL", 0x0FE000F0, 0x00E00090, "Multiply signed accumulate long")] = &ARM7TDMI::undefined;
	mapper[set->addInstruction("UMLAL", 0x0FE000F0, 0x00A00090, "Multiply unsigned accumulate long")] = &ARM7TDMI::undefined;
	mapper[set->addInstruction("SMULL", 0x0FE000F0, 0x00C00090, "Multiply signed long")] = &ARM7TDMI::undefined;
	mapper[set->addInstruction("UMULL", 0x0FE000F0, 0x00800090, "Multiply unsigned long")] = &ARM7TDMI::undefined;
	*/
	set->addInstruction(ARM_LDR);
	set->addInstruction(ARM_STR);
	/*
	mapper[set->addInstruction("LDRB", 0x0C500000, 0x04500000, "Load register from memory (byte)")] = &ARM7TDMI::undefined;
	mapper[set->addInstruction("STRB", 0x0C500000, 0x04400000, "Store register to memory (byte)")] = &ARM7TDMI::undefined;
	mapper[set->addInstruction("LDRH", 0x0E1000F0, 0x001000B0, "Load register from memory (half-word)")] = &ARM7TDMI::undefined;
	mapper[set->addInstruction("STRH", 0x0E1000F0, 0x000000B0, "Store register to memory (half-word)")] = &ARM7TDMI::undefined;
	mapper[set->addInstruction("LDRSB", 0x0E1000F0, 0x001000D0, "Load register from memory (sign extended byte)")] = &ARM7TDMI::undefined;
	mapper[set->addInstruction("LDRSH", 0x0E1000F0, 0x001000F0, "Load register from memory (sign extended halfword)")] = &ARM7TDMI::undefined;
	mapper[set->addInstruction("LDM", 0x0E100000, 0x08100000, "Load multiple registers")] = &ARM7TDMI::undefined;
	mapper[set->addInstruction("STM", 0x0E100000, 0x08000000, "Store multiple")] = &ARM7TDMI::undefined;
	*/
	set->addInstruction(ARM_SWP);
	/*
	mapper[set->addInstruction("SWPB", 0x0FF00FF0, 0x01400090, "Swap register with memory (byte)")] = &ARM7TDMI::undefined;
	mapper[set->addInstruction("SWI", 0x0F000000, 0x0F000000, "Software Interrupt")] = &ARM7TDMI::undefined;

	mapper[set->addInstruction("CDP", 0x0F000010, 0x0E000000, "Coprocessor Data Processing")] = &ARM7TDMI::undefined;
	mapper[set->addInstruction("LDC", 0x0E100000, 0x0D100000, "Load Coprocessor from memory")] = &ARM7TDMI::undefined;
	mapper[set->addInstruction("STC", 0x0E100000, 0x0D000000, "Store coprocessor register to memory")] = &ARM7TDMI::undefined;
	mapper[set->addInstruction("MRC", 0x0F100010, 0x0E100010, "Move from coprocessor register to CPU register")] = &ARM7TDMI::undefined;
	mapper[set->addInstruction("MCR", 0x0F100010, 0x0E000010, "Move from CPU register to coprocessor register")] = &ARM7TDMI::undefined;*/

	set->addInstruction(STD_PRINT);
	set->addInstruction(STD_EXPLAIN);

	loadInstructionSet(set);
}

void ARM7TDMI::start()
{
	for (int i = 0; i < 14; i++)
		r[i] = 0;
	r[15] = 0x8000000;

	std::string command = "";
	while (command != "STOP")
	{
		std::string command;
		std::getline(std::cin, command);

		unsigned int op = mem_map->readInt(r[15]);
		std::cout << arc::toHex(op, 32) << ": " << identify(op) << std::endl;;
		execute(op);
		r[15] += 4;
	}
}