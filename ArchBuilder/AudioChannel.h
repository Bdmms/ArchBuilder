#pragma once

#ifndef AUDIO_CHANNEL_H
#define AUDIO_CHANNEL_H

#include "core.h"
#include "sound.h"

#ifndef AUDIO_DOUBLE_PRECISION
	#define arcfloor floorf
	typedef float env_time;
	typedef float smpl_time;
	typedef float sample;
	typedef float sample_float;
	const sample SAMPLE_MIN = 0.0f;
	const sample SAMPLE_MAX = 1.0f;
	const sample DEFAULT_PAN = 0.5f;
	const smpl_time FREQ_OFF = 0.0f;
	const smpl_time FREQ_HIGH = 1.0f;
	const smpl_time PI = acos(-1.0f);
	const smpl_time PI2 = PI * 2.0f;
#else
	#define arcfloor floor
	typedef double env_time;
	typedef double smpl_time;
	typedef double sample;
	typedef double sample_float;
	const sample SAMPLE_MIN = 0.0;
	const sample SAMPLE_MAX = 1.0;
	const sample DEFAULT_PAN = 0.5;
	const smpl_time FREQ_OFF = 0.0;
	const smpl_time FREQ_HIGH = 1.0;
	const smpl_time PI = acos(-1.0);
	const smpl_time PI2 = PI * 2.0;
#endif

typedef sample (*channel_output)(const smpl_time& pos);
typedef void (*sampler)(void* channel);

enum class EnvMode
{
	OFF = 0,
	RELEASE = 1,
	SUSTAIN = 2,
	DECAY = 3,
	HOLD = 4,
	ATTACK = 5
};

static float silent(const float& pos) { return SAMPLE_MIN; }
static float noise(const float& pos) { return (float)rand() / RAND_MAX; }
static float squareWave75(const float& pos) { return pos < 0.75f ? 1.0f : 0.0f; }
static float squareWave50(const float& pos) { return pos < 0.5f ? 1.0f : 0.0f; }
static float squareWave25(const float& pos) { return pos < 0.25f ? 1.0f : 0.0f; }
static float triangleWave(const float& pos) { return pos < 0.5f ? pos * 2.0f : 0.0f; }
static float sawtoothWave(const float& pos) { return pos - floorf(pos); }
static float sineWave(const float& pos) { return sin(pos * PI2); }
static float highSineWave(const float& pos) { return sin(2.0f * pos * PI2); }
static float GBSquareWave0(const float& pos) { return (pos < 0.5f ? sin(pos * PI2 * 0.5f) - 1.0f : cos(pos * PI2 * 0.5f) + 1.0f); }
static float GBSquareWave1(const float& pos) { return (pos < 0.666f ? sin(pos * PI2 * 0.375f) - 1.0f : cos(pos * PI2 * 0.75f) + 1.0f); }
static float GenesisSBPPZ(const float& pos)  { return sin(PI / (50.0f * (pos - 0.5f))); }

namespace SoundDriver
{
	static constexpr void sampleFM(void* channel);
	static constexpr void sampleSF(void* channel);
};

struct SoundChannel
{
	Sample* loop_sample = nullptr;
	EnvMode mode = EnvMode::OFF;
	float env_offset = 0.0f;

	channel_output output;
	sampler audio_driver = SoundDriver::sampleFM;

	smpl_time sample_offset = FREQ_OFF;
	smpl_time frequency = FREQ_OFF;
	sample volume = SAMPLE_MIN;
	sample stereo_pan = SAMPLE_MIN;
	sample left_sample = SAMPLE_MIN;
	sample right_sample = SAMPLE_MIN;

	constexpr SoundChannel() : output(silent) {}
	constexpr SoundChannel(channel_output out) : output(out) {}
};

namespace SoundDriver
{
	constexpr sample SAMPLE_DIVISOR = 1.0f / 0x7FFF;

	static constexpr sample envelope(EnvMode& mode, float& env_offset, const Envelope& inst)
	{
		switch (mode)
		{
		case EnvMode::ATTACK: // This mode causes problems for now, so just skip to the next mode
			if (env_offset < inst.attackVolEnv)
				return env_offset / inst.attackVolEnv;
			env_offset -= inst.attackVolEnv;
			mode = EnvMode::HOLD;

		case EnvMode::HOLD:
			if (env_offset < inst.holdVolEnv)
				return PEAK_ENV;
			env_offset -= inst.holdVolEnv;
			mode = EnvMode::DECAY;

		case EnvMode::DECAY:
			if (env_offset < inst.decayVolEnv)
				return (PEAK_ENV - inst.sustainVolEnv * env_offset / inst.decayVolEnv);
			env_offset -= inst.decayVolEnv;
			mode = EnvMode::SUSTAIN;

		case EnvMode::SUSTAIN:
			return PEAK_ENV - inst.sustainVolEnv;

		case EnvMode::RELEASE:
			if (env_offset <= inst.releaseVolEnv)
				return (PEAK_ENV - inst.sustainVolEnv) * (PEAK_ENV - env_offset / inst.releaseVolEnv);
			env_offset = 0.0f;
			mode = EnvMode::OFF;

		default:
			return SAMPLE_MIN;
		}
	}

	static constexpr void sampleFM(void* channel)
	{
		SoundChannel& chan = *((SoundChannel*)channel);
		if (chan.mode != EnvMode::ATTACK) return;

		sample generatedSample = chan.volume * chan.output(chan.sample_offset);
		chan.left_sample = generatedSample * (DEFAULT_PAN - chan.stereo_pan);
		chan.right_sample = generatedSample * (DEFAULT_PAN + chan.stereo_pan);
		chan.sample_offset += chan.frequency;
		chan.sample_offset -= arcfloor(chan.sample_offset);
	}

	static constexpr void sampleSF(void* channel)
	{
		SoundChannel& chan = *((SoundChannel*)channel);
		if (chan.mode == EnvMode::OFF || chan.loop_sample == nullptr) return;

		sample generatedSample = chan.volume * envelope(chan.mode, chan.env_offset, chan.loop_sample->envelope)
			* chan.loop_sample->inst[(int)(chan.sample_offset * chan.loop_sample->length)] * SAMPLE_DIVISOR;

		chan.left_sample = generatedSample * (DEFAULT_PAN - chan.stereo_pan);
		chan.right_sample = generatedSample * (DEFAULT_PAN + chan.stereo_pan);

		// TODO: Check if ENV_TIME is constant
		chan.env_offset += ENV_TIME;
		chan.sample_offset += chan.frequency;

		if (chan.sample_offset < FREQ_HIGH)
			return;
		else if (chan.loop_sample->loop)
			chan.sample_offset -= arcfloor(chan.sample_offset);
		else if (chan.sample_offset >= chan.loop_sample->ending_offset)
			chan.mode = EnvMode::OFF;
	}

	static constexpr void start(SoundChannel& channel)
	{
		channel.mode = EnvMode::ATTACK;
		channel.env_offset = 0.0f;
	}

	static constexpr void stop(SoundChannel& channel)
	{
		channel.mode = (channel.mode != EnvMode::OFF ? EnvMode::RELEASE : EnvMode::OFF);
		channel.env_offset = 0.0f;
	}
};

#endif