
#include "Processor.h"
#include "ARM7.h"
#include "DisplayAdapter.h"
#include "AudioAdapter.h"

int main(int argc, char* argv[])
{
	ARCAudioStream::playFrom("AUDIO\\SND_BGM_M_DUNGEON_HONOU_02.mid");
	return 404;

	ARM7TDMI p1(60); // 60 ns ~= 16.8 MHz
	MemoryMap map(28); // 28-bit address space
	//DisplayAdaptor adaptor(map.getPointer(0x4000000));
	ROM rom_A("ROM/FLASH", 0x8000000, 0x2000000);
	//ROM rom_B("ROM/FLASH", 0xA000000, 0x2000000);
	//ROM rom_C("ROM/FLASH", 0xC000000, 0x2000000);

	map.addComponent(new ROM("SYSTEM ROM", 0x0, 0x400));
	map.addComponent(new RAM("ONBOARD WRAM", 0x2000000, 0x40000));
	map.addComponent(new RAM("INCHIP WRAM", 0x3000000, 0x8000));
	// LCD Registers
	map.addComponent(new IOPort16("DISPCNT", 0x4000000));
	map.addComponent(new IOPort16("DISPSTAT", 0x4000004));
	map.addComponent(new IOPort16("VCOUNT", 0x4000006));
	map.addComponent(new IOPort16("BG0CNT", 0x4000008));
	map.addComponent(new IOPort16("BG1CNT", 0x400000A));
	map.addComponent(new IOPort16("BG2CNT", 0x400000C));
	map.addComponent(new IOPort16("BG3CNT", 0x400000E));
	map.addComponent(new IOPort16("BG0HOFS", 0x4000010));
	map.addComponent(new IOPort16("BG0VOFS", 0x4000012));
	map.addComponent(new IOPort16("BG1HOFS", 0x4000014));
	map.addComponent(new IOPort16("BG1VOFS", 0x4000016));
	map.addComponent(new IOPort16("BG2HOFS", 0x4000018));
	map.addComponent(new IOPort16("BG2VOFS", 0x400001A));
	map.addComponent(new IOPort16("BG3HOFS", 0x400001C));
	map.addComponent(new IOPort16("BG3VOFS", 0x400001E));
	map.addComponent(new IOPort16("BG2PA", 0x4000020));
	map.addComponent(new IOPort16("BG2PB", 0x4000022));
	map.addComponent(new IOPort16("BG2PC", 0x4000024));
	map.addComponent(new IOPort16("BG2PD", 0x4000026));
	map.addComponent(new IOPort32("BG2X", 0x4000028));
	map.addComponent(new IOPort32("BG2Y", 0x400002C));
	map.addComponent(new IOPort16("BG3PA", 0x4000030));
	map.addComponent(new IOPort16("BG3PB", 0x4000032));
	map.addComponent(new IOPort16("BG3PC", 0x4000034));
	map.addComponent(new IOPort16("BG3PD", 0x4000036));
	map.addComponent(new IOPort32("BG3X", 0x4000038));
	map.addComponent(new IOPort32("BG3Y", 0x400003C));
	map.addComponent(new IOPort16("WIN0H", 0x4000040));
	map.addComponent(new IOPort16("WIN1H", 0x4000042));
	map.addComponent(new IOPort16("WIN0V", 0x4000044));
	map.addComponent(new IOPort16("WIN1V", 0x4000046));
	map.addComponent(new IOPort16("WININ", 0x4000048));
	map.addComponent(new IOPort16("WINOUT", 0x400004A));
	map.addComponent(new IOPort16("MOSAIC", 0x400004C));
	map.addComponent(new IOPort16("BLDCNT", 0x4000050));
	map.addComponent(new IOPort16("BLDALPHA", 0x4000052));
	map.addComponent(new IOPort16("BLDY", 0x4000054));
	// Sound Registers
	map.addComponent(new IOPort16("SOUND1CNT_L", 0x4000060));
	map.addComponent(new IOPort16("SOUND1CNT_H", 0x4000062));
	map.addComponent(new IOPort16("SOUND1CNT_X", 0x4000064));
	map.addComponent(new IOPort16("SOUND2CNT_L", 0x4000068));
	map.addComponent(new IOPort16("SOUND2CNT_H", 0x400006C));
	map.addComponent(new IOPort16("SOUND3CNT_L", 0x4000070));
	map.addComponent(new IOPort16("SOUND3CNT_H", 0x4000072));
	map.addComponent(new IOPort16("SOUND3CNT_X", 0x4000074));
	map.addComponent(new IOPort16("SOUND4CNT_L", 0x4000078));
	map.addComponent(new IOPort16("SOUND4CNT_H", 0x400007C));
	map.addComponent(new IOPort16("SOUNDBIAS", 0x4000088));
	map.addComponent(new RAM("WAVE_RAM", 0x4000090, 0x20));
	map.addComponent(new IOPort32("FIFO_A", 0x40000A0));
	map.addComponent(new IOPort32("FIFO_B", 0x40000A4));
	// DMA Transfer Channels
	map.addComponent(new IOPort32("DMA0SAD", 0x40000B0));
	map.addComponent(new IOPort32("DMA0DAD", 0x40000B4));
	map.addComponent(new IOPort16("DMA0CNT_L", 0x40000B8));
	map.addComponent(new IOPort16("DMA0CNT_H", 0x40000BA));
	map.addComponent(new IOPort32("DMA1SAD", 0x40000BC));
	map.addComponent(new IOPort32("DMA1DAD", 0x40000C0));
	map.addComponent(new IOPort16("DMA1CNT_L", 0x40000C4));
	map.addComponent(new IOPort16("DMA1CNT_H", 0x40000C6));
	map.addComponent(new IOPort32("DMA2SAD", 0x40000C8));
	map.addComponent(new IOPort32("DMA2DAD", 0x40000CC));
	map.addComponent(new IOPort16("DMA2CNT_L", 0x40000D0));
	map.addComponent(new IOPort16("DMA2CNT_H", 0x40000D2));
	map.addComponent(new IOPort32("DMA3SAD", 0x40000D4));
	map.addComponent(new IOPort32("DMA3DAD", 0x40000D8));
	map.addComponent(new IOPort16("DMA3CNT_L", 0x40000DC));
	map.addComponent(new IOPort16("DMA3CNT_H", 0x40000DE));
	// Timer Registers
	map.addComponent(new IOPort16("TM0CNT_L", 0x4000100));
	map.addComponent(new IOPort16("TM0CNT_H", 0x4000102));
	map.addComponent(new IOPort16("TM1CNT_L", 0x4000104));
	map.addComponent(new IOPort16("TM1CNT_H", 0x4000106));
	map.addComponent(new IOPort16("TM2CNT_L", 0x4000108));
	map.addComponent(new IOPort16("TM2CNT_H", 0x400010A));
	map.addComponent(new IOPort16("TM3CNT_L", 0x400010C));
	map.addComponent(new IOPort16("TM3CNT_H", 0x400010E));
	// Serial Communication (1)
	map.addComponent(new IOPort32("SIODATA32", 0x4000120));
	map.addComponent(new IOPort16("SIOMULTI0", 0x4000120));
	map.addComponent(new IOPort16("SIOMULTI1", 0x4000122));
	map.addComponent(new IOPort16("SIOMULTI2", 0x4000124));
	map.addComponent(new IOPort16("SIOMULTI3", 0x4000126));
	map.addComponent(new IOPort16("SIOCNT", 0x4000128));
	map.addComponent(new IOPort16("SIOMLT_SEND", 0x400012A));
	map.addComponent(new IOPort16("SIODATA8", 0x400012A));
	// Keypad Input
	map.addComponent(new IOPort16("KEYINPUT", 0x4000130));
	map.addComponent(new IOPort16("KEYCNT", 0x4000132));
	// Serial Communication (2)
	map.addComponent(new IOPort16("RCNT", 0x4000134));
	map.addComponent(new IOPort16("JOYCNT", 0x4000140));
	map.addComponent(new IOPort32("JOY_RECV", 0x4000150));
	map.addComponent(new IOPort32("JOY_TRANS", 0x4000154));
	map.addComponent(new IOPort16("JOY_STAT", 0x4000158));
	// Interrupt Control Registers
	map.addComponent(new IOPort16("IE", 0x4000200));
	map.addComponent(new IOPort16("IF", 0x4000202));
	map.addComponent(new IOPort16("WAITCNT", 0x4000204));
	map.addComponent(new IOPort16("IME", 0x4000208));
	map.addComponent(new IOPort8("POSTFLG", 0x4000300));
	map.addComponent(new IOPort8("HALTCNT", 0x4000301));
	map.addComponent(new IOPort16("?? (0x0FF)", 0x4000410));
	map.addComponent(new IOPort32("MEM_CNT", 0x4000800));

	/*
	for (int i = 1; i < 256; i++)
		map.addComponent(new IOPort32("MEM_CNT (MIR)", 0x4000800 + (i << 16)));
	*/

	map.addComponent(new RAM("CGRAM", 0x5000000, 0x400));
	map.addComponent(new RAM("VRAM", 0x6000000, 0x18000));
	map.addComponent(new RAM("OAM", 0x7000000, 0x400));
	map.addComponent(&rom_A);
	//map.addComponent(&rom_B);
	//map.addComponent(&rom_C);
	map.addComponent(new ROM("SRAM", 0xE000000, 0x1000));

	p1.setMemoryMap(&map);
	p1.printDescription();
	map.printDescription();

	rom_A.loadROM("ROMS/1997_FE8.gba");
	p1.readHeader(rom_A);

	std::cout << "\n[Waiting for execution]\n";

	p1.start();

	/*
	std::string command = "";
	while (command != "SWI")
	{
		std::string command;
		std::getline(std::cin, command);
		p1.interpret(command);
	}*/
}