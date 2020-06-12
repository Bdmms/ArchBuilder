#pragma once

#ifndef MIDI_H
#define MIDI_H

#include "core.h"

struct MidiChunk
{
	u32 id;
	u32 size;
	u8* buffer;
	MidiChunk() : id(0), size(0), buffer(nullptr) {}
	~MidiChunk() { if (buffer != nullptr) delete[] buffer; }
};

struct MidiFile
{
	u32 id;
	u32 header_size;
	u16 format_type;
	u16 num_tracks;
	u16 time_division;
	MidiChunk* chunks;
	bool valid;

	MidiFile(const char* path) : chunks(nullptr), id(0), header_size(6), format_type(0), num_tracks(0), time_division(0), valid(false)
	{
		std::cout << "\nLOADING " << path << std::endl;
		std::ifstream stream;
		stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);

		try
		{
			stream.open(path, std::ios::in | std::ios::binary);

			// Header Chunk
			stream.read((char*)this, 14);
			header_size = REu32(header_size);
			format_type = REu16(format_type);
			num_tracks = REu16(num_tracks);
			time_division = REu16(time_division);
			if (id != 0x6468544D) throw "Invalid Header ID";
			if (header_size != 6) throw "Invalid Header Size";
			std::cout << "   NUM TRACKS = " << num_tracks << std::endl;
			std::cout << "  FORMAT TYPE = " << format_type << std::endl;
			std::cout << "TIME DIVISION = " << time_division << std::endl;

			// Track Chunks
			chunks = new MidiChunk[num_tracks];
			for (u16 i = 0; i < num_tracks; i++)
			{
				stream.read((char*)(chunks + i), 8);
				chunks[i].size = REu32(chunks[i].size);
				if (chunks[i].id != 0x6B72544D) throw "Invalid Chunk ID";

				chunks[i].buffer = new u8[chunks[i].size];
				stream.read((char*)(chunks[i].buffer), chunks[i].size);
				std::cout << "TRACK " << i  << " (" << chunks[i].size << " BYTES)\n";
			}

			stream.close();
			valid = true;
		}
		catch (const char* err) { std::cout << "Error - " << err << std::endl; }
		catch (std::ifstream::failure e) { std::cout << path << " cannot be read!\n"; }
	}

	~MidiFile()
	{
		if(chunks == nullptr) delete[] chunks;
	}
};

struct MidiEvent
{
	u64 tick;
	u32 size;
	u8* message;
};

typedef std::vector<MidiEvent> MidiTrack;

struct MidiSequence
{
	MidiTrack* tracks = nullptr;
	u32 size = 0;
	u16 time_division = 120;

	MidiSequence() {}
	MidiSequence(const char* filepath) { load(filepath); }

	inline void load(const char* filepath)
	{
		MidiFile midi(filepath);

		if (!midi.valid || tracks != nullptr) return;
		size = midi.num_tracks;
		time_division = midi.time_division;
		tracks = new MidiTrack[size];

		for (u16 i = 0; i < size; i++)
		{
#ifdef MIDI_READOUT
			std::cout << "--- TRACK " << i << " ---\n";
#endif

			MidiChunk& chunk = midi.chunks[i];
			MidiTrack& track = tracks[i];
			u64 tick = 0;
			u32 num_events = 0;

			u32 b = 0;
			while (b < chunk.size)
			{
				MidiEvent ev;

				// 0-6 = value, 7 = byte extension
				u32 delta_time = chunk.buffer[b] & 0x7F;
				while (chunk.buffer[b++] & 0x80)
				{
					delta_time = delta_time << 7;
					delta_time += chunk.buffer[b] & 0x7F;
				}

				tick += delta_time;
				ev.tick = tick;
				u8 event_type = chunk.buffer[b++];

				if (event_type == 0xFF) // Meta Event
				{
					u8 type = chunk.buffer[b++];
					u8 length = chunk.buffer[b++];
					ev.size = length + 3;
					ev.message = new u8[ev.size];
					ev.message[0] = event_type;
					ev.message[1] = type;
					ev.message[2] = length;
					for (u32 param = 0; param < length && b < chunk.size; param++)
						ev.message[3 + param] = chunk.buffer[b++];
				}
				else if (event_type == 0xF0) // SysEx Event
				{
					u8 length = chunk.buffer[b++];
					ev.size = length + 2;
					ev.message = new u8[ev.size];
					ev.message[0] = event_type;
					ev.message[1] = length;
					for (u32 param = 0; param < length && b < chunk.size; param++)
						ev.message[2 + param] = chunk.buffer[b++];
				}
				else // Midi Channel Event
				{
					switch (event_type & 0xF0)
					{
					case 0xF0:	ev.size = 1; break;
					case 0xD0:
					case 0xC0:	ev.size = 2; break;
					default:	ev.size = 3; break;
					}

					ev.message = new u8[ev.size];
					ev.message[0] = event_type;
					for (u32 param = 1; param < ev.size && b < chunk.size; param++)
						ev.message[param] = chunk.buffer[b++];
				}

				// Read out event
#ifdef MIDI_READOUT
				std::cout << "Event " << track.size() << ": @" << ev.tick << " (" << std::hex;
				for (u32 e = 0; e < ev.size; e++)
				{
					if (ev.message[0] == 0xFF && e > 2)
						std::cout << ev.message[e];
					else
					{
						std::cout << "0x" << (u32)ev.message[e];
						if (e != ev.size - 1) std::cout << ", ";
					}
				}
				std::cout << std::dec << ")\n";
#endif

				track.push_back(ev);
			}

			std::cout << std::dec;
		}
	}

	~MidiSequence() { if(tracks == nullptr) delete[] tracks; }
};

class MidiSystem
{
public:
	inline static u64 nextEndTick(const MidiTrack& track, const u32 index, const u8 channel, const u8& tone)
	{
		for (u32 i = index; i < track.size(); i++)
		{
			if (((track[i].message[0] & 0xF0) == 0x80 || (track[i].message[0] & 0xF0) == 0x90) &&
				(track[i].message[0] & 0x0F) == channel && track[i].message[1] == tone)
				return track[i].tick;
		}
		return 0;
	}

	inline static u64 findTempo(const MidiSequence& sequence)
	{
		for (u32 t = 0; t < sequence.size; t++)
		{
			MidiTrack& track = sequence.tracks[t];
			for (u32 m = 0; m < track.size(); m++)
			{
				MidiEvent& event = track[m];
				if (event.size > 2 && event.message[0] == 0xFF && event.message[1] == 0x51)
				{
					u64 tempo = 0;
					u8 size = event.message[2];
					for (u8 b = 0; b < size; b++)
					{
						tempo = tempo << 8;
						tempo |= event.message[3 + b];
					}
					return tempo; // x us per quarter beat
				}
			}
		}
		std::cout << "NO TEMPO CHANGE DETECTED: DEFAULT USED" << std::endl;
		return 1000000;
	}
};


#endif
