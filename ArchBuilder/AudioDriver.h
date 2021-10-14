#pragma once

#ifndef AUDIO_DRIVER_H
#define AUDIO_DRIVER_H

#include "AudioChannel.h"
#include "SF2.h"

struct FreqLookUpTable
{
	smpl_time frequency[128];

	constexpr FreqLookUpTable() : frequency()
	{
		for (int i = 0; i < 128; i++)
		{
			int shift = i - 49; // -61
			double freq = pow(2.0, smpl_time(shift) / 12) * 440.0;
			frequency[i] = smpl_time(freq) / smpl_time(CD_SAMPLE_RATE);
			//lookup_frequency[i] = 1.0f;
		}
	}
};

constexpr sample SAMPLE_DIVISOR = 1.0f / 0x7FFF;
const FreqLookUpTable freqTable;

/**
 * @brief Interface for controlling the channels
*/
class SoundChannel
{
protected:
	sample volume = SAMPLE_MAX;
	sample stereo_pan = SAMPLE_MIN;
	smpl_time freq_mod = FREQ_HIGH;
public:
	sample left_sample = SAMPLE_MIN;
	sample right_sample = SAMPLE_MIN;

	virtual void start(const smpl_time frequency, const sample vol, const smpl_time init_offset = FREQ_OFF) = 0;
	virtual void start(const u8 tone, const sample vol) = 0;
	virtual void stop(const u8 tone) = 0;
	virtual void stop() = 0;
	virtual void tick() = 0;

	constexpr void setVolume(const sample vol) { volume = vol; }
	constexpr void setPitchBend(const smpl_time freq) { freq_mod = freq; }
	constexpr void setPanning(const sample pan) { stereo_pan = pan; }
};

/**
 * @brief Abstract class for controlling a single channel
*/
class SingleStreamChannel : public SoundChannel
{
protected:
	ARCSoundStream stream;
	u8 bindedTone;

public:

	virtual void start(const smpl_time frequency, const sample vol, const smpl_time init_offset = FREQ_OFF) override
	{
		stream.frequency = frequency;
		stream.volume = vol;
		stream.sample_offset = init_offset;
		stream.mode = EnvMode::ATTACK;
		stream.env_offset = 0.0f;
	}

	virtual void start(const u8 tone, const sample vol) override
	{
		bindedTone = tone;
		start(freqTable.frequency[tone], vol);
	}

	virtual void stop() override
	{
		stream.mode = (stream.mode != EnvMode::OFF ? EnvMode::RELEASE : EnvMode::OFF);
		stream.env_offset = FREQ_OFF;
	}

	virtual void stop(const u8 tone) override
	{
		if(tone == bindedTone) stop();
	}
};

template <u32 NumStreams>
class MultiStreamChannel : public SoundChannel
{
protected:
	ARCSoundStream streams[NumStreams];
	u8 streamBindings[NumStreams];
	std::list<u8> activeStreams;

	MultiStreamChannel() : streams(), streamBindings() {}

	constexpr u8 findStream(const u8 binding)
	{
		for (u8 i = 0; i < NumStreams; i++)
		{
			if (streamBindings[i] == binding) return i;
		}
		return 0xFF;
	}

	constexpr u8 findFreeStream()
	{
		for (u8 i = 0; i < NumStreams; i++)
		{
			if (streamBindings[i] == 0) return i;
		}

		// Stop the last created stream and return it
		u8 activeID = activeStreams.front();
		activeStreams.pop_front();
		return activeID;
	}

public:

	virtual void start(const smpl_time frequency, const sample vol, const smpl_time init_offset = FREQ_OFF) override
	{
		// Do nothing (TODO)
	}

	virtual void stop() override
	{
		for (ARCSoundStream& stream : streams)
		{
			stream.mode = stream.mode != EnvMode::OFF ? EnvMode::RELEASE : EnvMode::OFF;
			stream.env_offset = FREQ_OFF;
		}
	}

	virtual void stop(const u8 tone) override
	{
		u8 activeID = findStream(tone);
		if (activeID == 0xFF) return;

		streams[activeID].mode = streams[activeID].mode != EnvMode::OFF ? EnvMode::RELEASE : EnvMode::OFF;
		streams[activeID].env_offset = FREQ_OFF;
		streamBindings[activeID] = 0;
		activeStreams.remove(activeID);
	}
};

/**
 * @brief Driver for controlling a single sampled channel
*/
class SFSingleStreamChannel : public SingleStreamChannel
{
protected:
	const Instrument& inst;

public:
	SFSingleStreamChannel(const Instrument& inst) : inst(inst) {}

	virtual void start(const u8 tone, const sample vol) override
	{
		FilteredSample* smpl_ptr = inst.getSample(tone);
		if (smpl_ptr == nullptr) return;

		stream.sampleData = smpl_ptr;
		const SHDRChunk* shdr = smpl_ptr->shdr;

		smpl_time ratio_sample_length = (sample_float)smpl_ptr->length * freqTable.frequency[shdr->byOriginalPitch - PITCH_SHIFT];
		smpl_time ratio_sample_rate = (sample_float)shdr->dwSampleRate / CD_SAMPLE_RATE;
		smpl_time initial_offset = smpl_ptr->initial_offset;
		smpl_time pitchCorrect = (ratio_sample_rate / ratio_sample_length) * pow(CENT, (float)shdr->chPitchCorrection);

		bindedTone = tone;
		SingleStreamChannel::start(freqTable.frequency[tone] * pitchCorrect, vol, initial_offset);
	}

	virtual void tick() override
	{
		Sample* loop_sample = (Sample*)stream.sampleData;
		if (stream.mode == EnvMode::OFF || loop_sample == nullptr) return;

		sample generatedSample = volume * stream.volume * SoundDriver::envelope(stream.mode, stream.env_offset, loop_sample->envelope)
			* loop_sample->inst[(int)(stream.sample_offset * loop_sample->length)] * SAMPLE_DIVISOR;

		left_sample = generatedSample * (DEFAULT_PAN - stereo_pan);
		right_sample = generatedSample * (DEFAULT_PAN + stereo_pan);

		// TODO: Check if ENV_TIME is constant
		stream.env_offset += ENV_TIME;
		stream.sample_offset += stream.frequency * freq_mod;

		if (stream.sample_offset < FREQ_HIGH)
			return;
		else if (loop_sample->loop)
			stream.sample_offset -= arcfloor(stream.sample_offset);
		else if (stream.sample_offset >= loop_sample->ending_offset)
			stream.mode = EnvMode::OFF;
	}
};

constexpr u32 NUM_STREAMS = 5;

class SFMultiStreamChannel : public MultiStreamChannel<NUM_STREAMS>
{
protected:
	const Instrument& inst;

public:
	SFMultiStreamChannel(const Instrument& inst) : inst(inst) {}

	virtual void start(const u8 tone, const sample vol) override
	{
		FilteredSample* smpl_ptr = inst.getSample(tone);
		if (smpl_ptr == nullptr) return;

		u32 activeID = findStream(tone);
		if (activeID == 0xFF) activeID = findFreeStream();

		ARCSoundStream& stream = streams[activeID];
		streamBindings[activeID] = tone;
		activeStreams.emplace_back(activeID);

		const SHDRChunk* shdr = smpl_ptr->shdr;

		smpl_time ratio_sample_length = (sample_float)smpl_ptr->length * freqTable.frequency[shdr->byOriginalPitch - PITCH_SHIFT];
		smpl_time ratio_sample_rate = (sample_float)shdr->dwSampleRate / CD_SAMPLE_RATE;
		smpl_time initial_offset = smpl_ptr->initial_offset;
		smpl_time pitchCorrect = (ratio_sample_rate / ratio_sample_length) * pow(CENT, (float)shdr->chPitchCorrection);

		stream.sampleData = smpl_ptr;
		stream.frequency = freqTable.frequency[tone] * pitchCorrect;
		stream.volume = vol;
		stream.sample_offset = initial_offset;
		stream.mode = EnvMode::ATTACK;
		stream.env_offset = 0.0f;
	}

	virtual void tick() override
	{
		left_sample = SAMPLE_MIN;
		right_sample = SAMPLE_MIN;

		for (u32 i = 0; i < NUM_STREAMS; i++)
		{
			ARCSoundStream& stream = streams[i];
			Sample* loop_sample = (Sample*)stream.sampleData;
			if (stream.mode == EnvMode::OFF || loop_sample == nullptr) continue;

			sample generatedSample = volume * stream.volume * SoundDriver::envelope(stream.mode, stream.env_offset, loop_sample->envelope)
				* loop_sample->inst[(int)(stream.sample_offset * loop_sample->length)] * SAMPLE_DIVISOR;

			left_sample += generatedSample * (DEFAULT_PAN - stereo_pan);
			right_sample += generatedSample * (DEFAULT_PAN + stereo_pan);

			// TODO: Check if ENV_TIME is constant
			stream.env_offset += ENV_TIME;
			stream.sample_offset += stream.frequency * freq_mod;

			if (stream.sample_offset < FREQ_HIGH)
				continue;
			else if (loop_sample->loop)
				stream.sample_offset -= arcfloor(stream.sample_offset);
			else if (stream.sample_offset >= loop_sample->ending_offset)
				stream.mode = EnvMode::OFF;
		}
	}
};

class FMSingleStreamChannel : public SingleStreamChannel
{
public:
	FMSingleStreamChannel(channel_output output = FMSynth::silent)
	{
		stream.sampleData = output;
	}

	virtual void tick() override
	{
		if (stream.mode != EnvMode::ATTACK) return;

		channel_output fmOutput = (channel_output)stream.sampleData;
		sample generatedSample = volume * stream.volume * fmOutput(stream.sample_offset);
		left_sample = generatedSample * (DEFAULT_PAN - stereo_pan);
		right_sample = generatedSample * (DEFAULT_PAN + stereo_pan);
		stream.sample_offset += stream.frequency * freq_mod;
		stream.sample_offset -= arcfloor(stream.sample_offset);
	}
};

#endif
