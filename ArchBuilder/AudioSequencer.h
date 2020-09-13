#pragma once

#ifndef AUDIO_SEQUENCER_H
#define AUDIO_SEQUENCER_H

#include "AudioChannel.h"

constexpr u8 C1 = 4;
constexpr u8 C2 = 16;
constexpr u8 C3 = 28;
constexpr u8 C4 = 40;
constexpr u8 C5 = 52;
constexpr u8 C6 = 64;
constexpr u8 C7 = 76;
constexpr u8 C8 = 88;
constexpr u8 C9 = 90;

constexpr float TEMPO_MODIFIER = 0.393f;

enum MidiStatus
{
	STATUS_CHANNEL_OFF = 0x80,
	STATUS_CHANNEL_ON = 0x90,
	STATUS_NOTE_AFTERTOUCH = 0xA0,
	STATUS_CHANNEL_CONTROLLER = 0xB0,
	STATUS_CHANNEL_BANK_CHANGE = 0xC0,
	STATUS_CHANNEL_AFTERTOUCH = 0xD0,
	STATUS_CHANNEL_PITCHBEND = 0xE0,
	STATUS_SYSTEM_EXCLUSIVE = 0xF0,
	STATUS_META_MESSAGE = 0xFF
};

enum MidiController
{
	CONTROLLER_CHANNEL_MODULATION = 0x01,
	CONTROLLER_CHANNEL_DATA_ENTRY_MSB = 0x06,
	CONTROLLER_CHANNEL_VOLUME = 0x07,
	CONTROLLER_CHANNEL_PAN = 0x0A,
	CONTROLLER_CHANNEL_DATA_ENTRY_LSB = 0x26,
	CONTROLLER_NON_REGISTERED_PARAM_LSB = 0x62,
	CONTROLLER_NON_REGISTERED_PARAM_MSB = 0x63,
	CONTROLLER_REGISTERED_PARAM_LSB = 0x64,
	CONTROLLER_REGISTERED_PARAM_MSB = 0x65
};

enum MidiMeta
{
	META_TEXT = 0x01,
	META_COPYRIGHT = 0x02,
	META_TRACK_NAME = 0x03,
	META_INSTRUMENT_NAME = 0x04,
	META_LYRICS = 0x05,
	META_MARKER = 0x06,
	META_CUE_MARKER = 0x07,
	META_DEVICE_NAME = 0x09,
	META_CHANNEL_PREFIX = 0x21,
	META_MIDI_PORT = 0x21,
	META_END_OF_TRACK = 0x2F,
	META_TEMPO_CHANGE = 0x51,
	META_SMPTE_OFFSET = 0x54,
	META_TIME_SIGNATURE = 0x58,
	META_KEY_SIGNATURE = 0x59,
	META_SEQUENCER_SPECIFIC = 0x7F
};

struct TrackSequence
{
	const MidiTrack& track;
	u32 eventID;
	bool ended;

	constexpr TrackSequence(const MidiTrack& seq) : track(seq), eventID(0), ended(false) { }
};

class Sequencer
{
	const Instrument* soundbank = nullptr;
	std::vector<TrackSequence> sequencers;
	SoundChannel** channels;
	float tick_frequency;
	float tick_offset;
	u64 sequence_offset;
	u16 time_division;

	sample_float lookup_frequency[128];

public:
	bool paused = false;

	Sequencer(SoundChannel** chans) : sequence_offset(0), tick_offset(0.0f), tick_frequency(1.0), channels(chans), time_division(1), lookup_frequency()
	{
		for (int i = 0; i < 128; i++)
		{
			int shift = i - 49; // -61
			double frequency = pow(2.0, sample_float(shift) / 12) * 440.0;
			lookup_frequency[i] = sample_float(frequency) / sample_float(CD_SAMPLE_RATE);
		}
	}

	bool handleEvent(const MidiEvent& event);

	constexpr void apply(const Instrument* sounds) { soundbank = sounds; }
	constexpr void setTick(const u64 tick) { sequence_offset = tick; }
	constexpr void skip(u64 ticks) { sequence_offset += ticks; }

	constexpr void load(const MidiSequence& seq)
	{
		time_division = seq.time_division;

		if (time_division & 0x8000) // frames per second
		{
			u32 PPQ = (int)((seq.time_division & 0x7F00) >> 8) * (int)(seq.time_division & 0xFF);
			tick_frequency = (float)(PPQ) / (CD_SAMPLE_RATE * TEMPO_MODIFIER);
		}

		for (u32 i = 0; i < seq.size; i++)
			sequencers.push_back(TrackSequence(seq.tracks[i]));
	}

	inline void eventAt(TrackSequence& seq)
	{
		const MidiTrack& track = seq.track;
		while (seq.eventID < track.size() && sequence_offset >= track[seq.eventID].tick)
			seq.ended = handleEvent(track[seq.eventID++]);
	}

	inline void tick()
	{
		if (tick_offset >= 1.0f)
		{
			bool ended = 1;
			for (u32 s = 0; s < sequencers.size(); s++)
			{
				eventAt(sequencers[s]);
				ended &= sequencers[s].ended;
			}

			if (ended)
				reset();

			tick_offset -= floorf(tick_offset);
			sequence_offset++;
		}

		if (!paused)
			tick_offset += tick_frequency;
	}

	inline void reset()
	{
		sequence_offset = 0;
		for (u32 s = 0; s < sequencers.size(); s++)
		{
			for (u32 i = 0; i < MIDI_NUM_CHANNELS; i++)
				channels[i]->stop();

			sequencers[s].eventID = 0;
			sequencers[s].ended = false;
		}
	}
};

#endif