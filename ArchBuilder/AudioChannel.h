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
	constexpr sample SAMPLE_MIN = 0.0f;
	constexpr sample SAMPLE_MAX = 1.0f;
	constexpr sample DEFAULT_PAN = 0.5f;
	constexpr smpl_time FREQ_OFF = 0.0f;
	constexpr smpl_time FREQ_HIGH = 1.0f;
	constexpr smpl_time PI = 3.1415926535897932384626433f;
	constexpr smpl_time PI2 = PI * 2.0f;
#else
	#define arcfloor floor
	typedef double env_time;
	typedef double smpl_time;
	typedef double sample;
	typedef double sample_float;
	constexpr sample SAMPLE_MIN = 0.0;
	constexpr sample SAMPLE_MAX = 1.0;
	constexpr sample DEFAULT_PAN = 0.5;
	constexpr smpl_time FREQ_OFF = 0.0;
	constexpr smpl_time FREQ_HIGH = 1.0;
	constexpr smpl_time PI = 3.1415926535897932384626433;
	constexpr smpl_time PI2 = PI * 2.0;
#endif

typedef sample (*channel_output)(const smpl_time& pos);

enum class EnvMode
{
	OFF = 0,
	RELEASE = 1,
	SUSTAIN = 2,
	DECAY = 3,
	HOLD = 4,
	ATTACK = 5
};

struct ARCSoundStream
{
	EnvMode mode = EnvMode::OFF;
	float env_offset = 0.0f;
	void* sampleData = nullptr;

	smpl_time sample_offset = FREQ_OFF;
	smpl_time frequency = FREQ_OFF;
	sample volume = SAMPLE_MIN;
};

namespace FMSynth
{
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
	static float GenesisSBPPZ(const float& pos) { return sin(PI / (50.0f * (pos - 0.5f))); }

	static float tanCompWave(const float& pos) 
	{ 
		float val = tanf(pos * PI);
		int iVal = (int)val;
		val -= iVal;
		return val + 0.5f * sinf(pos * PI * 3.0f);
	}

	static float sinHarmonicWave(const float& pos)
	{
		float sample = 0.4f * sinf(pos * PI2);
		sample += 0.3f * sinf(pos * pos * PI2 * 1.5f);
		sample += 0.2f * sawtoothWave(pos);
		sample += 0.1f * sinf(pos * PI2 * 0.5f);
		return sample;
	}

	static float sudoWave(const float& pos)
	{
		return 0.25f * sinf(10.0f * pos * PI) + sinf(powf(3.24f - pos, 2.0f)) * (1.0f - pos);
		//return sinf(powf(pos - 0.5f, -0.4f)) * cosf(pos * PI2);
	}
}

namespace SoundDriver
{
	/**
	 * @brief Advances the envelope state of the channel
	 * @param mode - Envelope mode
	 * @param env_offset - offset time into the envelope
	 * @param inst - Envelope settings of instrument
	 * @return Magnitude of volume from the current envelope offset
	*/
	constexpr sample envelope(EnvMode& mode, float& env_offset, const Envelope& inst)
	{
		switch (mode)
		{
		case EnvMode::ATTACK: // This mode causes problems for now, so just skip to the next mode
			if (env_offset < inst.attackVolEnv)
				return env_offset / inst.attackVolEnv;
			env_offset -= inst.attackVolEnv;
			mode = EnvMode::HOLD;
			[[fallthrough]];

		case EnvMode::HOLD:
			if (env_offset < inst.holdVolEnv)
				return PEAK_ENV;
			env_offset -= inst.holdVolEnv;
			mode = EnvMode::DECAY;
			[[fallthrough]];

		case EnvMode::DECAY:
			if (env_offset < inst.decayVolEnv)
				return (PEAK_ENV - inst.sustainVolEnv * env_offset / inst.decayVolEnv);
			env_offset -= inst.decayVolEnv;
			mode = EnvMode::SUSTAIN;
			[[fallthrough]];

		case EnvMode::SUSTAIN:
			return PEAK_ENV - inst.sustainVolEnv;
			[[fallthrough]];

		case EnvMode::RELEASE:
			if (env_offset <= inst.releaseVolEnv)
				return (PEAK_ENV - inst.sustainVolEnv) * (PEAK_ENV - env_offset / inst.releaseVolEnv);
			env_offset = 0.0f;
			mode = EnvMode::OFF;
			[[fallthrough]];

		default:
			return SAMPLE_MIN;
		}
	}
};

#endif