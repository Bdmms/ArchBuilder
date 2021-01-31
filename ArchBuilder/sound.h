#pragma once

#ifndef SOUND_H
#define SOUND_H

#include "core.h"

constexpr float CENT = 1.000577790f;
constexpr float CENTIBEL = 1.011579454f;
constexpr float DECIBEL = 1.122018454f;
constexpr float TIMECENT = 1.000577790f;

constexpr u32 CD_SAMPLE_RATE = 441000; // sample rate (samples / sec) : 44.1 KHz = CD Quality
constexpr u32 OUT_SAMPLE_RATE = 441000;
constexpr float PEAK_ENV = 1.0f;
constexpr float ENV_TIME = TIMECENT / OUT_SAMPLE_RATE;
constexpr u32 PITCH_SHIFT = 16;

// An object containing all time parameters of a sound envelope
struct Envelope
{
	float attackVolEnv = 0.0f;
	float holdVolEnv = 0.0f;
	float decayVolEnv = 0.0f;
	float sustainVolEnv = 0.0f;
	float releaseVolEnv = 0.0f;
};

struct Sample
{
	Envelope envelope;
	short* inst = nullptr;
	float initial_offset = 0.0f;
	float ending_offset = 1.0f;
	float length = 1.0f;
	bool loop = true;

	Sample() {}
	Sample(Sample* smpl)
	{
		envelope = smpl->envelope;
		inst = smpl->inst;
		initial_offset = smpl->initial_offset;
		ending_offset = smpl->ending_offset;
		length = smpl->length;
		loop = smpl->loop;
	}
};

#endif
