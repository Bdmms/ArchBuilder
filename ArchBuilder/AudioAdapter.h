#pragma once

#ifndef AUDIO_ADAPTER_H
#define AUDIO_ADAPTER_H

#include <SFML/Audio.hpp>
#include "core.h"

constexpr u32 SAMPLE_CHUNK_SIZE = 441000;
constexpr u32 A4 = 440; // frequency of the cycle (cycle / sec)
constexpr u32 CD_SAMPLE_RATE = 441000; // sample rate (samples / sec) : 44.1 KHz = CD Quality
constexpr u32 A4t = CD_SAMPLE_RATE / A4; // # of samples per cycle (samples / cycle)

constexpr float BASE_FREQ = 1.0595f;

#define A0 0
#define C1 4
#define C2 16
#define C3 28
#define C4 40
#define D4b 41
#define D4 42
#define E4b 43
#define E4 44
#define F4 45
#define G4b 46
#define G4 47
#define A4b 48
#define A4 49
#define B4b 50
#define B4 51
#define C5 52
#define C6 64
#define C7 76
#define C8 88
#define C9 90

class WaveGenerator
{
	float (*wave)(const float);
	float sample_offset;

public:
	WaveGenerator(float (*func)(const float)) : wave(func), sample_offset(0.0f) {}

	inline short getSample(const u32& len, const short vol)
	{
		short sample = (short)(vol * wave(sample_offset));
		sample_offset += 1.0f / len;
		sample_offset -= floorf(sample_offset);
		return sample;
	}

	inline void fillSamples(short* samples, const u32 chunk_size, const u32& len, const short vol)
	{
		float inv_len = 1.0f / len;
		for (u32 i = 0; i < chunk_size; i++)
		{
			samples[i] = (short)(vol * wave(sample_offset));
			sample_offset += inv_len;
			sample_offset -= floorf(sample_offset);
		}
	}
};

struct SeqEvent
{
	u64 tick;
	u32 length;
	u16 volume;
	u8 tone;
};

struct Sequence
{
private:
	std::vector<SeqEvent> sequence;
public:
	void addEvent(float t, float len, u16 vol, u8 tone)
	{
		SeqEvent seq;
		seq.tick = (u64)(t * CD_SAMPLE_RATE);
		seq.length = (u32)(len * CD_SAMPLE_RATE);
		seq.volume = vol;
		seq.tone = tone;
		sequence.push_back(seq);
	}

	u32 size() const { return sequence.size(); }
	SeqEvent& at(const u32& i) { return sequence[i]; }
};

class Sequencer
{
	WaveGenerator* sound;
	Sequence* sequence;
	short* samples;
	u16 freq_table[128];
	u64 sequence_offset;
	u32 eventID;

public:
	Sequencer(WaveGenerator* s, Sequence* seq) : sound(s), sequence(seq), eventID(0), sequence_offset(0)
	{
		samples = new short[SAMPLE_CHUNK_SIZE];

		for (int i = 0; i < 128; i++)
		{
			float frequency = pow(BASE_FREQ, i - 49) * 440.0f;
			freq_table[i] = (u16)(CD_SAMPLE_RATE / frequency);
		}
	}
	~Sequencer() { delete[] samples; }

	inline short nextSample()
	{
		if (eventID >= sequence->size()) return 0;

		SeqEvent& nextEvent = sequence->at(eventID);
		short sample = sequence_offset < nextEvent.tick ? 0 : sound->getSample(freq_table[nextEvent.tone], nextEvent.volume);

		sequence_offset++;
		while (nextEvent.tick + nextEvent.length <= sequence_offset)
		{
			eventID++;
			if (eventID >= sequence->size()) return sample;
			nextEvent = sequence->at(eventID);
		}

		return sample;
	}

	inline short* nextChunk()
	{
		if (eventID >= sequence->size())
		{
			for (u32 i = 0; i < SAMPLE_CHUNK_SIZE; i++)
				samples[i] = 0;
			return samples;
		}

		SeqEvent& nextEvent = sequence->at(eventID);
		u32 chunk_offset = 0;
		u32 chunk_remain = SAMPLE_CHUNK_SIZE;
		u64 end_point = sequence_offset + SAMPLE_CHUNK_SIZE; // Ending sample of current chunk

		// Attempts to fill all samples before returning
		while (chunk_remain > 0)
		{
			// Fill in Gap Time (until next event)
			if (sequence_offset < nextEvent.tick)
			{
				u64 gapSize = nextEvent.tick - sequence_offset > chunk_remain ? chunk_remain : nextEvent.tick - sequence_offset;
				for (u64 i = 0; i < gapSize; i++) samples[chunk_offset + i] = 0;
				chunk_remain -= gapSize;
				chunk_offset = SAMPLE_CHUNK_SIZE - chunk_remain;
				sequence_offset += gapSize;
			}

			// Calculate the size of data to fill in
			u32 event_length = nextEvent.tick + nextEvent.length - sequence_offset;
			u32 chunk_size = event_length > chunk_remain ? chunk_remain : event_length;
			sound->fillSamples(samples + chunk_offset, chunk_size, freq_table[nextEvent.tone], nextEvent.volume);
			chunk_remain -= chunk_size;
			chunk_offset = SAMPLE_CHUNK_SIZE - chunk_remain;

			// Move to the next value in the sequence
			sequence_offset += chunk_size;
			while (nextEvent.tick + nextEvent.length <= sequence_offset)
			{
				eventID++;

				if (eventID >= sequence->size())
				{
					sequence_offset = end_point;
					for (u32 i = chunk_offset; i < SAMPLE_CHUNK_SIZE; i++)
						samples[i] = 0;
					return samples;
				}

				nextEvent = sequence->at(eventID);
			}
		}

		sequence_offset = end_point;
		return samples;
	}
};

class ARCAudioStream : public sf::SoundStream
{
private:
	Sequencer* sequencer;

public:
	static void test();

	ARCAudioStream(Sequencer* s) : sequencer(s)
	{ 
		initialize(1, CD_SAMPLE_RATE);
	}

	virtual bool onGetData(Chunk& data)
	{
		data.samples = sequencer->nextChunk(); // ptr to sample chunk
		data.sampleCount = SAMPLE_CHUNK_SIZE; // size of chunk in samples
		return true;
	}

	virtual void onSeek(sf::Time timeOffset)
	{
		// Called when setPlayingOffset is called
	}
};

#endif