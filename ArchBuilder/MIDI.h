#pragma once

#ifndef MIDI_H
#define MIDI_H

#include "sound.h"

constexpr u8 MESSAGE_TONE = 0;
constexpr u8 MESSAGE_VOLUME = 1;

constexpr u8 MIDI_NUM_CHANNELS = 16;
constexpr u32 NUM_CHUNK_IDS = 24;

// SF2 Chunk IDs
const u32 SF2_CHUNK_IDS[NUM_CHUNK_IDS] = {
	// Required
	arc::ID("LIST"), arc::ID("ifil"), arc::ID("isng"), arc::ID("INAM"), arc::ID("ICRD"), arc::ID("ISFT"), arc::ID("smpl"), arc::ID("phdr"),
	arc::ID("pbag"), arc::ID("pmod"), arc::ID("pgen"), arc::ID("inst"), arc::ID("ibag"), arc::ID("imod"), arc::ID("igen"), arc::ID("shdr"),

	// Optional
	arc::ID("irom"), arc::ID("iver"), arc::ID("IENG"), arc::ID("IPRD"), arc::ID("ICOP"), arc::ID("ICMT"), arc::ID("ISFT"), arc::ID("sm24")
};

// Generator operation IDs
enum Generator
{
	GEN_START_ADDRESS_OFFSET = 0,
	GEN_REVERB = 16, // Unused
	GEN_PAN = 17,
	GEN_ATTACK_VOL_ENVELOPE = 34,
	GEN_HOLD_VOL_ENVELOPE = 35,
	GEN_DECAY_VOL_ENVELOPE = 36,
	GEN_SUSTAIN_VOL_ENVELOPE = 37,
	GEN_RELEASE_VOL_ENVELOPE = 38,
	GEN_INSTRUMENT = 41,
	GEN_KEY_RANGE = 43,
	GEN_VELOCITY_RANGE = 44, // Unused
	GEN_INITIAL_ATTENUATION = 48, // Unused
	GEN_SAMPLE_ID = 53,
	GEN_SAMPLE_MODE = 54,
	GEN_SCALE_TUNING = 56, // Unused
	GEN_EXCLUSIVE_CLASS = 57, // Unused
	GEN_OVERRIDE_ROOT_KEY = 58 // Unused
};

// A readable file
class ARCFile
{
protected:
	bool valid = false;
	virtual bool load(const char*) = 0;
public:
	constexpr bool isValid() const { return valid; }
};

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
	u64 tick;
	u32 size;
	u8* message;
	u8  status;

	inline void print(const u32 id) const;
};

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

// The structure of a chunk within a .sf2 file
struct SF2Chunk
{
	u32 id = 0;
	u32 size = 0;
	u32 name = 0;
	u8* data = nullptr;
	std::vector<SF2Chunk> subchunks;

	bool subdivide(const u32 level);

	constexpr const SF2Chunk* findTag(const u32& tagID) const
	{
		if (id == tagID)
			return this;
		for (u32 i = 0; i < subchunks.size(); i++)
		{
			const SF2Chunk* temp = subchunks[i].findTag(tagID);
			if (temp != nullptr)
				return temp;
		}
		return nullptr;
	}

	constexpr static bool isValidChunkID(const u32& id)
	{
		bool valid = 0;
		for (u32 i = 0; i < NUM_CHUNK_IDS; i++)
			valid |= (id == SF2_CHUNK_IDS[i]);
		return valid;
	}
};

// An object containing data from a .sf2 file
class SF2File : public ARCFile
{
	bool load(const char* filepath) override;

public:
	SF2Chunk main_chunk;

	SF2File(const char* filepath) { valid = load(filepath); }
	constexpr const SF2Chunk* getChunkWithID(const u32& id) const { return main_chunk.findTag(id); }
};

// SF2 data types
typedef unsigned short SFSampleLink;
typedef short genAmountType; // 0-7 = LoByte, 8 - 15 = HiByte (or a single short value)

// Chunk that contains bank data
constexpr u32 PHDR_CHUNK_SIZE = 38;
struct PHDRChunk
{
	u8 achPresetName[20];
	u16 wPreset;
	u16 wBank;
	u16 wPresetBagNdx;
	u32 dwLibrary;
	u32 dwGenre;
	u32 dwMorphology;
};

// Chunk that points to other chunks
constexpr u32 BAG_CHUNK_SIZE = 4;
struct BAGChunk
{
	u16 wGenIndex;
	u16 wModIndex;
};

// Chunk that contains generator data
constexpr u32 GEN_CHUNK_SIZE = 4;
struct GENChunk
{
	u16 sfGenOper;
	genAmountType genAmount;
};

// Chunk that contains instrument data
constexpr u32 INST_CHUNK_SIZE = 22;
struct INSTChunk
{
	u8 achInstName[20];
	u16 wInstBagIndex;
};

// Chunk that contains sample data
constexpr u32 SHDR_CHUNK_SIZE = 46;
struct SHDRChunk
{
	u8 achSampleName[20];
	u32 dwStart;
	u32 dwEnd;
	u32 dwStartLoop;
	u32 dwEndLoop;
	u32 dwSampleRate;
	u8 byOriginalPitch;
	char chPitchCorrection;
	u16 wSampleLink;
	SFSampleLink sfSampleType;
};

// An object linking the SHDR chunk to its sample
struct FilteredSample : public Sample
{
	const SHDRChunk* shdr = nullptr;
	short* sample = nullptr;
	float volume_pan = 1.0f;
	float tuning = 1.0f;
	u8 minKey = 0;
	u8 maxKey = 127;

	constexpr void loadSample(const SHDRChunk* shdr_chunk, short* smpl)
	{
		shdr = shdr_chunk;
		sample = smpl + shdr->dwStart;
		length = (float)(shdr->dwEndLoop - shdr->dwStartLoop);
		
		if (length) // Not equal 0
		{
			inst = smpl + shdr->dwStartLoop;
			initial_offset = -(float)(shdr->dwStartLoop - shdr->dwStart) / length;
			ending_offset = (float)(shdr->dwEnd - shdr->dwStartLoop) / length;
		}
		else
		{
			inst = sample;
			length = (float)size();
			loop = false;
		}
	}

	constexpr int size() const { return shdr->dwEnd - shdr->dwStart;  }
	constexpr bool isEmpty() const { return inst == nullptr; }

	void applyGenerator(GENChunk& gen, const SF2Chunk* samples, short* smpl);
};

// An object that contains all of the generator parameters that apply to a sample or set of samples
struct Instrument
{
	FilteredSample* sample[128];
	bool empty;

	constexpr Instrument() : sample(), empty(true) { for (u32 i = 0; i < 128; i++) sample[i] = nullptr; }
	
	constexpr FilteredSample* getSample(const u8 tone) const
	{ 
		return sample[tone & 0x7F]; 
	}

	constexpr void addSample(FilteredSample* smpl)
	{ 
		if (smpl->shdr == nullptr) return;

		// Assign all tones to sample
		if (empty)
			for (u32 i = 0; i < 128; i++) sample[i & 0x7F] = smpl;
		// Assign range of tones to sample
		else
			for (u32 i = smpl->minKey; i <= smpl->maxKey && i < 128; i++) sample[i & 0x7F] = smpl;

		empty = false;
	}
};

struct GeneratorMap
{
	u16 map[64];
	bool used[64];

	GeneratorMap() : map(), used()
	{
		for (u32 i = 0; i < 64; i++)
			used[i] = false;
	}

	void put(const GENChunk* chunk)
	{
		map[chunk->sfGenOper & 0x3F] = chunk->genAmount;
		used[chunk->sfGenOper & 0x3F] = true;
	}

	void applyTo(FilteredSample& sample, const SF2Chunk* shdr_chunk, short* smpl)
	{
		GENChunk chunk;
		for (u32 i = 0; i < 64; i++)
		{
			if (used[i])
			{
				chunk.sfGenOper = i;
				chunk.genAmount = map[i];
				sample.applyGenerator(chunk, shdr_chunk, smpl);
			}
		}
	}
};

constexpr u32 NUM_BANKS = 32;
typedef Instrument Soundbank[128];

// Soundfont object, which contains a soundbank
class Soundfont : public ARCFile
{
	const SF2File file;
	Soundbank soundbank[NUM_BANKS];
	FilteredSample* gen_samples;
	u32 version = 0;
	u32 smpl_size = 0;
	short* smpl_u16 = nullptr;
	char* engine_name = nullptr;
	char* soundbank_name = nullptr;

	bool load(const char* filepath) override;
	bool fillBank();

public:
	Soundfont(const char* filepath) : file(filepath) { valid = load(filepath); }
	Soundbank* getSoundbanks() { return soundbank; }
};

#endif
