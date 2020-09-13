#include "AudioChannel.h"

constexpr sample applyEnvelope(EnvMode& mode, float& env_offset, const Envelope& inst)
{
	switch (mode)
	{
	case EnvMode::ATTACK:
		if (env_offset <= inst.attackVolEnv)
			return env_offset / inst.attackVolEnv;
		env_offset -= inst.attackVolEnv;
		mode = EnvMode::HOLD;

	case EnvMode::HOLD:
		if (env_offset <= inst.holdVolEnv)
			return PEAK_ENV;
		env_offset -= inst.holdVolEnv;
		mode = EnvMode::DECAY;

	case EnvMode::DECAY:
		if (env_offset <= inst.decayVolEnv)
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

void EnvelopeOscillator::getSample(sample& leftSample, sample& rightSample)
{
	if (mode == EnvMode::OFF || muted)
	{
		leftSample = SAMPLE_MIN;
		rightSample = SAMPLE_MIN;
		return;
	}

	sample generatedSample = volume * channel_vol * applyEnvelope(mode, env_offset, envelope);
	env_offset += ENV_TIME;
	leftSample = generatedSample * (DEFAULT_PAN - stereo_pan);
	rightSample = generatedSample * (DEFAULT_PAN + stereo_pan);
	sample_offset += frequency * frq_modifier;
	sample_offset -= arcfloor(sample_offset);
}

constexpr sample SAMPLE_DIVISOR = 1.0f / 0x7FFF;

void SampleGenerator::getSample(sample& leftSample, sample& rightSample)
{
	leftSample = SAMPLE_MIN;
	rightSample = SAMPLE_MIN;

	if (mode == EnvMode::OFF || muted || loop_sample == nullptr) 
		return;

	sample generatedSample = volume * channel_vol * applyEnvelope(mode, env_offset, instrument.envelope)
		* loop_sample->inst[(int)(sample_offset * loop_sample->length)] * SAMPLE_DIVISOR;

	leftSample = generatedSample * (DEFAULT_PAN - stereo_pan);
	rightSample = generatedSample * (DEFAULT_PAN + stereo_pan);

	env_offset += ENV_TIME;
	sample_offset += frequency * frq_modifier * pitchCorrectness;

	if (instrument.loop && sample_offset > FREQ_HIGH)
		sample_offset -= arcfloor(sample_offset);
	else if(sample_offset >= loop_sample->ending_offset)
		mode = EnvMode::OFF;
}
