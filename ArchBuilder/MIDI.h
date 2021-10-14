#pragma once

#ifndef MIDI_H
#define MIDI_H

#include "sound.h"
#include "File.h"

constexpr u8 MESSAGE_TONE = 0;
constexpr u8 MESSAGE_VOLUME = 1;

constexpr u8 MIDI_NUM_CHANNELS = 16;

constexpr u32 MIDI_HEADER_ID = 0x6468544D;
constexpr u32 MIDI_CHUNK_HEADER_ID = 0x6B72544D;

// A chunk found within a .mid file
struct MidiChunk
{
	u32 id;
	u32 size;
	u8* buffer;
	MidiChunk() : id(0), size(0), buffer(nullptr) {}
	~MidiChunk() { if (buffer != nullptr) delete[] buffer; }
};

// An object that contains the .mid file data
class MidiFile : public ARCFile
{
	/**
	 * @brief Loads a MidiFile
	 * @param path - path of the file
	 * @return whether the operation was successful
	*/
	bool load(const char* path) override;

public:
	u32 id;
	u32 header_size;
	u16 format_type;
	u16 num_tracks;
	u16 time_division;
	MidiChunk* chunks;

	MidiFile(const char* path) : chunks(nullptr), id(0), header_size(6), format_type(0), num_tracks(0), time_division(0)
	{
		valid = load(path);
	}
	~MidiFile() { if(chunks == nullptr) delete[] chunks; }
};

// The structure of a midi event
struct MidiEvent
{
	u8* message;
	u64 tick;
	u32 size;
	u8  status;
};

// Prints out the event inline
static std::ostream& operator<<(std::ostream& os, const MidiEvent& event)
{
	std::cout << "Event @" << event.tick << " (" << std::hex;
	for (u32 e = 0; e < event.size; e++)
	{
		if (event.message[0] == 0xFF && e > 2)
			std::cout << event.message[e];
		else
		{
			std::cout << "0x" << (u32)event.message[e];
			if (e != event.size - 1) std::cout << ", ";
		}
	}
	std::cout << std::dec << ")\n";
	return os;
}

// The list of midi events
typedef std::vector<MidiEvent> MidiTrack;

// The sequence for processing midi events
class MidiSequence : public ARCFile
{
	bool addNextEvent(u8*& data, const u8* data_end, MidiTrack& track, u64& tick);
	bool convertToTrack(const MidiChunk& chunk, MidiTrack& track);
	bool load(const char* filepath) override;

public:
	const MidiFile midi;
	u32 size;
	u16 time_division;
	MidiTrack* tracks;

	MidiSequence(const char* filepath) : midi(filepath), tracks(nullptr)
	{
		size = midi.num_tracks;
		time_division = midi.time_division;
		valid = load(filepath);
	}
	~MidiSequence()
	{ 
		if (tracks != nullptr) 
			delete[] tracks;
	}
};

// A static class for processing sequence data
class MidiSystem
{
public:
	constexpr static bool isMessageNote(const MidiEvent& event) { return (event.status & 0xF0) == 0x80 || (event.status & 0xF0) == 0x90; }
	constexpr static bool equals(const MidiEvent& event, const u8& channel, const u8& tone) { return (event.message[0] & 0x0F) == channel && event.message[1] == tone; }
	static u64 nextEndTick(const MidiTrack& track, const u32 index, const u8 channel, const u8& tone);
	static const MidiEvent* findMessage(const MidiTrack& track, const u8 m0, const u8 m1);
	static const MidiEvent* findMessage(const MidiSequence& sequence, const u8 m0, const u8 m1);
};

#endif
