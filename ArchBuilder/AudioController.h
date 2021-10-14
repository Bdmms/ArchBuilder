#pragma once

#ifndef AUDIO_SEQUENCER_H
#define AUDIO_SEQUENCER_H

#include "MIDI.h"
#include "SF2.h"
#include "AudioDriver.h"

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

enum MidiControl
{
	CONTROLLER_CHANNEL_BANK_SELECT = 0x00,
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

struct Track
{
	const MidiTrack& track;
	u32 eventID;
	bool ended;

	constexpr Track(const MidiTrack& seq) : track(seq), eventID(0), ended(false) { }
};

struct ConvertedSample : public Sample
{
	smpl_time pitchCorrect = FREQ_HIGH;

	ConvertedSample( FilteredSample* smpl_ptr, sample_float lookup_frequency[128] ) : Sample(smpl_ptr)
	{
		const SHDRChunk& shdr = *(smpl_ptr->shdr);
		u32 shifted_pitch = shdr.byOriginalPitch - PITCH_SHIFT;
		sample_float ratio_sample_length = (sample_float)smpl_ptr->length * lookup_frequency[shifted_pitch > 127 ? 60 : shifted_pitch];
		sample_float ratio_sample_rate = (sample_float)shdr.dwSampleRate / CD_SAMPLE_RATE;
		pitchCorrect = (ratio_sample_rate / ratio_sample_length) * pow(CENT, (float)shdr.chPitchCorrection);
	}
};

class Controller
{
public:
	bool paused = false;

	virtual void setChannel(const u32 i, SoundChannel* channel) = 0;
	virtual SoundChannel& getChannel(const u32 i) = 0;

	virtual u32 numChannels() = 0;
	virtual void reset() = 0;
	virtual void tick() = 0;
	virtual void skip(u64 ticks) = 0;
	virtual void setTick(const u64 tick) = 0;

	virtual void set(const u8 channel, const u8 state, const u8 freq, const u8 volume) {}
};

class MidiController : public Controller
{
protected:
	SoundChannel* channels[MIDI_NUM_CHANNELS];
	Soundbank* soundbank = nullptr;
	float tick_frequency = 1.0f;
	float tick_offset = 0.0f;
	u64 sequence_offset = 0;
	u16 time_division = 1;
	u8 selected_bank = 0;

public:
	MidiController() : channels()
	{
		for (u32 i = 0; i < MIDI_NUM_CHANNELS; i++)
		{
			channels[i] = new FMSingleStreamChannel();
		}
	}

	bool handleEvent(const MidiEvent& event);
	
	virtual void setChannel(const u32 i, SoundChannel* channel)
	{
		delete channels[i];
		channels[i] = channel;
	}

	virtual SoundChannel& getChannel(const u32 i) override { return *(channels[i]); }
	virtual u32 numChannels() override { return MIDI_NUM_CHANNELS; }
	virtual void setTick(const u64 tick) override {}
	virtual void skip(u64 ticks) override {}

	constexpr void apply(Soundbank* sounds) { soundbank = sounds; }

	virtual void tick() override { }

	virtual void reset() override
	{
		for (u32 i = 0; i < MIDI_NUM_CHANNELS; i++)
		{
			channels[i]->stop();
		}
	}

	virtual void set(const u8 channel, const u8 state, const u8 tone, const u8 volume) override
	{
		MidiEvent ev;
		ev.size = 3;
		ev.message = new u8[ev.size];
		ev.status = state | channel;
		ev.message[MESSAGE_TONE] = tone;
		ev.message[MESSAGE_VOLUME] = volume;
		handleEvent(ev);
		delete[] ev.message;
	}
};

class MidiSequencer : public MidiController
{
	std::vector<Track> sequencers;

public:
	MidiSequencer(const MidiSequence& seq)
	{
		time_division = seq.time_division;

		if (time_division & 0x8000) // frames per second
		{
			u32 PPQ = (int)((seq.time_division & 0x7F00) >> 8) * (int)(seq.time_division & 0xFF);
			tick_frequency = (float)(PPQ) / (CD_SAMPLE_RATE * TEMPO_MODIFIER);
		}

		for (u32 i = 0; i < seq.size; i++)
			sequencers.push_back(Track(seq.tracks[i]));
	}

	inline void eventAt(Track& seq)
	{
		const MidiTrack& track = seq.track;
		while (seq.eventID < track.size() && sequence_offset >= track[seq.eventID].tick)
			seq.ended = handleEvent(track[seq.eventID++]);
	}

	virtual void setTick(const u64 tick) override { sequence_offset = tick; }
	virtual void skip(u64 ticks) override { sequence_offset += ticks; }

	virtual void tick() override
	{
		if (tick_offset >= 1.0f)
		{
			bool ended = 1;
			for (u32 s = 0; s < sequencers.size(); s++)
			{
				eventAt(sequencers[s]);
				ended &= sequencers[s].ended;
			}

			// Resets if all sequencers are done
			if (ended)
				reset();

			tick_offset -= floorf(tick_offset);
			sequence_offset++;
		}

		if (!paused)
			tick_offset += tick_frequency;
	}

	virtual void reset() override
	{
		sequence_offset = 0;
		for (u32 s = 0; s < sequencers.size(); s++)
		{
			for (u32 i = 0; i < MIDI_NUM_CHANNELS; i++)
			{
				channels[i]->stop();
			}

			sequencers[s].eventID = 0;
			sequencers[s].ended = false;
		}
	}
};

#endif