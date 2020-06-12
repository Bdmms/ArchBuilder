#pragma once

#ifndef AUDIO_ADAPTER_H
#define AUDIO_ADAPTER_H

//#define MIDI_READOUT

#include <SFML/Audio.hpp>
#include "core.h"
#include "MIDI.h"

constexpr u32 SAMPLE_CHUNK_SIZE = 441000;
constexpr u32 A4 = 440; // frequency of the cycle (cycle / sec)
constexpr u32 CD_SAMPLE_RATE = 441000; // sample rate (samples / sec) : 44.1 KHz = CD Quality

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

class SoundChannel
{
	float (*wave)(const float);
	float sample_offset;
	float frequency;
	bool enabled;

public:
	SoundChannel(float (*func)(const float)) : wave(func), sample_offset(0.0f), frequency(0.0f), enabled(false) {}

	inline short getSample(const float& len, const short vol)
	{
		short sample = (short)(vol * wave(sample_offset));
		sample_offset += 1.0f / len;
		sample_offset -= floorf(sample_offset);
		return sample;
	}

	inline void fillSamples(short* samples, const u32 chunk_size, const float& len, const short vol)
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
	u64 endTick;
	//u32 length;
	u16 volume;
	u8 tone;
};

struct Track
{
private:
	std::vector<SeqEvent> track;
public:
	Track() {}
	Track(MidiTrack* t, double samples_per_tick)
	{
		MidiTrack& trk = *t;
		for (u32 i = 0; i < trk.size(); i++)
		{
			MidiEvent& ev = trk[i];
			if ((ev.message[0] & 0xF0) == 0x90)
			{
				SeqEvent event;
				event.tick = (u64)(ev.tick * samples_per_tick);
				event.tone = ev.message[1];
				event.volume = ev.message[2] << 4;
				event.endTick = (u64)(MidiSystem::nextEndTick(trk, i + 1, ev.message[0] & 0xF, event.tone) * samples_per_tick);
				track.push_back(event);
			}
		}
	}

	void addEvent(float t, float len, u16 vol, u8 tone)
	{
		SeqEvent seq;
		seq.tick = (u64)(t * CD_SAMPLE_RATE);
		seq.endTick = seq.tick + (u32)(len * CD_SAMPLE_RATE);
		seq.volume = vol;
		seq.tone = tone;
		track.push_back(seq);
	}

	u32 size() const { return track.size(); }
	SeqEvent& at(const u32& i) { return track[i]; }
};

struct TrackBuilder : public Track
{
	float marker = 0.0f;
	void pushEvent(float len, u16 vol, u8 tone)
	{
		addEvent(marker, len, vol, tone);
		marker += len;
	}
};

class TrackSequencer
{
	SoundChannel* sound;
	Track* track;
	u32 eventID;
	u16 lastSample;
	bool enabled = true;
public:
	TrackSequencer() : sound(nullptr), track(nullptr), eventID(0), lastSample(0) { }
	TrackSequencer(SoundChannel* s, Track* seq) : sound(s), track(seq), eventID(0), lastSample(0) { }
	inline short nextSample(const u64& tick);
	inline bool atEnd() { return eventID >= track->size();}
	inline void reset() { eventID = 0; }
	inline void toggleEnable() { enabled = !enabled; }
};

class Sequencer
{
	std::vector<TrackSequencer> sequencers;
	u64 sequence_offset;
public:
	Sequencer() : sequence_offset(0) {}
	Sequencer(MidiSequence seq);

	inline void addSequencer(SoundChannel*& s, Track*& seq) { sequencers.push_back(TrackSequencer(s, seq)); }
	inline short nextSample()
	{
		short sample = 0;
		for (u32 s = 0; s < sequencers.size(); s++)
			sample += sequencers[s].nextSample(sequence_offset);
		sequence_offset++;
		return sample;
	}

	inline void toggleEnable(u32 t) 
	{ 
		if (t < sequencers.size()) 
		{
			sequencers[t].toggleEnable();
			std::cout << "CHANNEL " << t << " TOGGLED" << std::endl;
		}
	}

	inline bool isEnded()
	{
		bool ended = 1;
		for (u32 s = 0; s < sequencers.size(); s++)
			ended &= sequencers[s].atEnd();
		return ended;
	}

	inline void reset()
	{
		sequence_offset = 0;
		for (u32 s = 0; s < sequencers.size(); s++)
			sequencers[s].reset();
	}
};

class ARCAudioStream : public sf::SoundStream
{
private:
	Sequencer sequencer;
	short* buffer;

public:
	static void playFrom(const char* filename);

	ARCAudioStream()
	{ 
		buffer = new short[SAMPLE_CHUNK_SIZE];
		initialize(1, CD_SAMPLE_RATE);
	}
	ARCAudioStream(MidiSequence seq) : sequencer(seq)
	{
		buffer = new short[SAMPLE_CHUNK_SIZE];
		initialize(1, CD_SAMPLE_RATE);
	}
	~ARCAudioStream() { delete[] buffer; }

	inline Sequencer& getSequencer() { return sequencer; }
	inline void addSequence(SoundChannel* s, Track* seq) { sequencer.addSequencer(s, seq); }

	virtual bool onGetData(Chunk& data)
	{
		if (sequencer.isEnded())
			sequencer.reset();

		for (u32 i = 0; i < SAMPLE_CHUNK_SIZE; i++)
			buffer[i] = sequencer.nextSample();

		data.samples = buffer;
		data.sampleCount = SAMPLE_CHUNK_SIZE; // size of chunk in samples
		return true;
	}

	virtual void onSeek(sf::Time timeOffset)
	{
		// Called when setPlayingOffset is called
	}
};

#endif