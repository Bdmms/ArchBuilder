#pragma once

#ifndef AUDIO_CHANNEL_H
#define AUDIO_CHANNEL_H

#include "core.h"
#include "MIDI.h"

constexpr u32 CD_SAMPLE_RATE = 441000; // sample rate (samples / sec) : 44.1 KHz = CD Quality
constexpr u32 OUT_SAMPLE_RATE = 441000;
constexpr float PEAK_ENV = 1.0f;
constexpr u32 PITCH_SHIFT = 16;

#ifdef AUDIO_PARALLEL
	#ifndef AUDIO_DOUBLE_PRECISION
		constexpr float ENV_TIME = TIMECENT / CD_SAMPLE_RATE * 4.0f;
		#define arcfloor(x) _mm256_floor_ps(x.vec)
		typedef arc::vec8 env_time;
		typedef arc::vec8 smpl_time;
		typedef arc::vec8 sample;
	#else
		constexpr float ENV_TIME = TIMECENT / CD_SAMPLE_RATE * 8.0f;
		#define arcfloor(x) _mm256_floor_pd(x.vec)
		typedef arc::vec4 env_time;
		typedef arc::vec4 smpl_time;
		typedef arc::vec4 sample;
	#endif
#else
	constexpr float ENV_TIME = TIMECENT / CD_SAMPLE_RATE;
	#ifndef AUDIO_DOUBLE_PRECISION
		#define arcfloor floorf
		typedef float env_time;
		typedef float smpl_time;
		typedef float sample;
	#else
		#define arcfloor floor
		typedef double env_time;
		typedef double smpl_time;
		typedef double sample;
	#endif
#endif

#ifndef AUDIO_DOUBLE_PRECISION
	typedef float sample_float;
	const sample SAMPLE_MIN = 0.0f;
	const sample SAMPLE_MAX = 1.0f;
	const sample DEFAULT_PAN = 0.5f;
	const smpl_time FREQ_OFF = 0.0f;
	const smpl_time FREQ_HIGH = 1.0f;
	const smpl_time PI = acos(-1.0f);
	const smpl_time PI2 = PI * 2.0f;
#else
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
static float squareWave(const float& pos) { return pos < 0.5f ? 1.0f : 0.0f; }
static float triangleWave(const float& pos) { return pos < 0.5f ? pos * 2.0f : 0.0f; }
static float sawtoothWave(const float& pos) { return pos - floorf(pos); }
static float sineWave(const float& pos) { return sin(pos * PI2); }
static float GBSquareWave0(const float& pos) { return (pos < 0.5f ? sin(pos * PI2 * 0.5f) - 1.0f : cos(pos * PI2 * 0.5f) + 1.0f); }
static float GBSquareWave1(const float& pos) { return (pos < 0.666f ? sin(pos * PI2 * 0.375f) - 1.0f : cos(pos * PI2 * 0.75f) + 1.0f); }
static float cotModWave(const float& pos)
{
	if (pos == 0.0f) return 0.0f;
	float st = 1.0f / tan(pos * PI);
	return st - floorf(st);
}

static float GenesisSBPPZ(const float& pos) 
{ 
	return sin(PI2 / (50.0f * (pos - 0.5f)));
}

class SoundChannel
{
protected:
	channel_output output;
	smpl_time sample_offset;
	smpl_time frequency;
	smpl_time frq_modifier;
	sample volume;
	sample stereo_pan;
	sample channel_vol;

public:
	bool muted;

	constexpr SoundChannel() : output(silent), muted(false),
		sample_offset(FREQ_OFF), frequency(FREQ_OFF), frq_modifier(FREQ_HIGH), stereo_pan(SAMPLE_MIN), volume(SAMPLE_MIN), channel_vol(SAMPLE_MAX) {}
	constexpr SoundChannel(channel_output out) : output(out), muted(false),
		sample_offset(FREQ_OFF), frequency(FREQ_OFF), frq_modifier(FREQ_HIGH), stereo_pan(SAMPLE_MIN), volume(SAMPLE_MIN), channel_vol(SAMPLE_MAX) {}

	constexpr void setBend(const smpl_time freq) { frq_modifier = freq; }
	constexpr void setVolume(const sample vol) { channel_vol = vol; }
	constexpr void setPan(const sample pan) { stereo_pan = pan; }

	virtual void bindTone(const u8& tone) {}
	virtual void start(const sample_float freq, const sample vol) = 0;
	virtual void stop() = 0;
	virtual void getSample(sample& leftSample, sample& rightSample) = 0;
};

class Oscillator : public SoundChannel
{
	bool enabled;

public:
	constexpr Oscillator() : enabled(false) {}
	constexpr Oscillator(channel_output out) : SoundChannel(out), enabled(false) {}

	inline void start(const sample_float freq, const sample vol) override
	{ 
		frequency = freq;
		volume = vol;
		enabled = true; 

	#ifdef AUDIO_PARALLEL
		sample_offset.setoffset(0.0, freq);
	#endif
	}

	inline void stop() override { enabled = false; }
	inline void getSample(sample& leftSample, sample& rightSample) override
	{
		if (!enabled || muted)
		{
			leftSample = SAMPLE_MIN;
			rightSample = SAMPLE_MIN;
			return;
		}

		sample generatedSample = volume * channel_vol * output(sample_offset);
		leftSample = generatedSample * (DEFAULT_PAN - stereo_pan);
		rightSample = generatedSample * (DEFAULT_PAN + stereo_pan);
		sample_offset += frequency * frq_modifier;
		sample_offset -= arcfloor(sample_offset);
	}
};

class EnvelopeChannel : public SoundChannel
{
protected:
	float env_offset = 0.0f;
	EnvMode mode = EnvMode::ATTACK;

public:
	constexpr EnvelopeChannel() {}
	constexpr EnvelopeChannel(channel_output out) : SoundChannel(out) {}
	inline void stop() override { mode = (mode != EnvMode::OFF ? EnvMode::RELEASE : EnvMode::OFF); env_offset = 0.0f; }
};

class EnvelopeOscillator : public EnvelopeChannel
{
public:
	const Envelope& envelope;

	constexpr EnvelopeOscillator(channel_output out, const Envelope& sound) : EnvelopeChannel(out), envelope(sound) {}

	inline void start(const sample_float freq, const sample vol) override
	{
		frequency = freq;
		volume = vol;
		mode = EnvMode::ATTACK;
		env_offset = 0.0f;

	#ifdef AUDIO_PARALLEL
		sample_offset.setoffset(0.0, freq);
	#endif
	}

	void getSample(sample& leftSample, sample& rightSample) override;
};

class SampleGenerator : public EnvelopeChannel
{
protected:
	const Instrument& instrument;
	const ChunkSample* loop_sample = nullptr;
	const sample_float* freq_lookup;
	smpl_time pitchCorrectness = FREQ_HIGH;

public:
	constexpr SampleGenerator(const SampleGenerator& gen) : instrument(gen.instrument), pitchCorrectness(gen.pitchCorrectness), freq_lookup(gen.freq_lookup) { }
	constexpr SampleGenerator(const Instrument& sound, const sample_float* freq_lookup) : instrument(sound), freq_lookup(freq_lookup) { }

	inline void bindTone(const u8& tone) override
	{
		loop_sample = instrument.getSample(tone);

		if (loop_sample->chunk->chPitchCorrection != 0)
			std::cout << "WARNING! PITCH CORRECTNESS IN EFFECT!\n";

		u32 shifted_pitch = loop_sample->chunk->byOriginalPitch - PITCH_SHIFT;
		sample_float ratio_sample_length = (sample_float)loop_sample->length * freq_lookup[shifted_pitch > 127 ? 60 : shifted_pitch];
		sample_float ratio_sample_rate = (sample_float)loop_sample->chunk->dwSampleRate / CD_SAMPLE_RATE;
		pitchCorrectness = (ratio_sample_rate / ratio_sample_length);
	}

	inline void start(const sample_float freq, const sample vol) override
	{ 
		frequency = freq;
		volume = vol;
		mode = EnvMode::ATTACK; 
		env_offset = 0.0f;

	#ifndef AUDIO_PARALLEL
		sample_offset = loop_sample->initial_offset;
	#else
		sample_offset.setoffset(instrument.initial_offset, freq);
	#endif
	}

	void getSample(sample& leftSample, sample& rightSample) override;
};

#endif