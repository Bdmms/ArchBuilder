#pragma once

#ifndef MIDI_H
#define MIDI_H

#include "core.h"

constexpr u8 MESSAGE_STATUS = 0;
constexpr u8 MESSAGE_TONE = 1;
constexpr u8 MESSAGE_VOLUME = 2;

constexpr float CENT = 1.000577790f;
constexpr float CENTIBEL = 1.011579454f;
constexpr float DECIBEL = 1.122018454f;
constexpr float TIMECENT = 1.000577790f;

const float HALFSCALE	= pow(2.0f, 6.0f / 12.0f) - 1.0f;
const float QUADTONE	= pow(2.0f, 4.0f / 12.0f) - 1.0f;
const float TRITONE		= pow(2.0f, 3.0f / 12.0f) - 1.0f;
const float DUALTONE	= pow(2.0f, 2.0f / 12.0f) - 1.0f;
const float FULLTONE	= pow(2.0f, 1.0f / 12.0f) - 1.0f;
const float SEMITONE	= pow(2.0f, 0.5f / 12.0f) - 1.0f;

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
	GEN_REVERB = 16,
	GEN_PAN = 17,
	GEN_ATTACK_VOL_ENVELOPE = 34,
	GEN_HOLD_VOL_ENVELOPE = 35,
	GEN_DECAY_VOL_ENVELOPE = 36,
	GEN_SUSTAIN_VOL_ENVELOPE = 37,
	GEN_RELEASE_VOL_ENVELOPE = 38,
	GEN_INSTRUMENT = 41,
	GEN_KEY_RANGE = 43,
	GEN_VELOCITY_RANGE = 44,
	GEN_INITIAL_ATTENUATION = 48,
	GEN_SAMPLE_ID = 53,
	GEN_SAMPLE_MODE = 54,
	GEN_OVERRIDE_ROOT_KEY = 58
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
	constexpr static bool isMessageNote(const MidiEvent& event) { return (event.message[MESSAGE_STATUS] & 0xF0) == 0x80 || (event.message[MESSAGE_STATUS] & 0xF0) == 0x90; }
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

	inline constexpr const SF2Chunk* findTag(const u32& tagID) const
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
	inline constexpr const SF2Chunk* getChunkWithID(const u32& id) const { return main_chunk.findTag(id); }
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
	u8 chPitchCorrection;
	u16 wSampleLink;
	SFSampleLink sfSampleType;
};

// An object containing all time parameters of a sound envelope
struct Envelope
{
	float attackVolEnv = 0.0f;
	float holdVolEnv = 0.0f;
	float decayVolEnv = 0.0f;
	float sustainVolEnv = 0.0f;
	float releaseVolEnv = 0.0f;
};

// An object linking the SHDR chunk to its sample
struct ChunkSample
{
	const SHDRChunk* chunk;
	short* sample = nullptr;
	short* inst = nullptr;
	float initial_offset;
	float ending_offset;
	u32 length;
	bool loop = true;

	constexpr void create(const SHDRChunk* shdr, short* smpl)
	{
		chunk = shdr;
		sample = smpl + shdr->dwStart;
		length = shdr->dwEndLoop - shdr->dwStartLoop;

		if (!length)
		{
			length = size();
			loop = false;
		}

		inst = loop ? smpl + shdr->dwStartLoop : sample;
		initial_offset = loop ? -(float)(shdr->dwStartLoop - shdr->dwStart) / length : 0.0f;
		ending_offset = loop ? (float)(shdr->dwEnd - shdr->dwStartLoop) / length : 1.0f;
	}

	constexpr bool isEmpty() const { return sample == nullptr; }
	constexpr u32 size() const { return chunk->dwEnd - chunk->dwStart; }
};

// An object that contains all of the generator parameters that apply to a sample or set of samples
struct Instrument
{
	Envelope envelope;
	ChunkSample* sample[1];
	float volume_pan = 1.0f;
	bool loop = true;

	constexpr void loadSample(ChunkSample* smpl)
	{
		sample[0] = smpl;
		if ( smpl->length == smpl->size() )
			loop = false;
	}

	constexpr ChunkSample* getSample(const u8& tone) const { return sample[0]; }
	constexpr bool isEmpty() const { return sample[0] == nullptr; }

	void applyGenerator(GENChunk& gen, ChunkSample* samples);
};

// Soundfont object, which contains a soundbank
class Soundfont : public ARCFile
{
	const SF2File file;
	Instrument soundbank[128];
	ChunkSample* samples;
	u32 version = 0;
	u32 smpl_size = 0;
	short* smpl_u16 = nullptr;
	char* engine_name = nullptr;
	char* soundbank_name = nullptr;

	bool load(const char* filepath) override;
	void createInstrument(Instrument& instrument, const INSTChunk* inst, const SF2Chunk* ibag_chunk, const SF2Chunk* igen_chunk);
	bool fillBank();

public:
	Soundfont(const char* filepath) : file(filepath) { valid = load(filepath); }
	const Instrument* getSoundbank() { return soundbank; }
};

#endif
