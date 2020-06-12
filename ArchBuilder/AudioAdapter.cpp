#include "AudioAdapter.h"
#include <cmath>

constexpr float PI = 3.141592654f;
constexpr float PI2 = PI * 2;

float FREQ_TABLE[128];

float noise(const float pos) { return (float) rand() / 0xFFFF; }
float squareWave(const float pos) { return pos < 0.5f ? 1.0f : 0.0f; }
float triangleWave(const float pos) { return pos < 0.5f ? pos * 2 : -pos * 2; }
float sawtoothWave(const float pos) { return pos - floorf(pos); }
float sineWave(const float pos) { return sin(pos * PI2); }
float kickSquareWave(const float pos) { return (pos < 0.5f ? sin(pos * PI2 * 0.5f) - 1.0f : cos(pos * PI2 * 0.5f) + 1.0f); }

float tanWave(const float pos) 
{ 
	float w = tan(pos * PI2);
	if (w > 1.0f) return 1.0f;
	else if (w < -1.0f) return -1.0f;
	else return w;
}

float compWave0(const float pos) 
{ 
	//float w = 0;
	//for (int i = 1; i < 10; i++)
	//	w += sineWave(pos * i * 2);
	//return w / 6;
	return 0.0f;
}

float compWave1(const float pos)
{
	float w = 0;
	for (int i = 1; i < 10; i++)
		w += triangleWave(pos * i) * sineWave(pos * i * 2);
	return w / 30;
}

float fluteWave(const float pos)
{
	if (pos < 0.5f) return -sin(pos * PI2);
	return 0.5f * sin(pos * PI2 * 2.0f);
}

const u32 smp_sz = 512;
float* wave_sample = new float[smp_sz];
float sampleWave(const float pos) { return wave_sample[(u32)(pos * smp_sz) & 0x1FF]; }

void ARCAudioStream::playFrom(const char* filename)
{
	// Setup Frequency table
	for (int i = 0; i < 128; i++)
	{
		double frequency = pow(2, (float)(i - 49) / 12) * 440.0;
		FREQ_TABLE[i] = (float) CD_SAMPLE_RATE / frequency;
	}

	MidiSequence seq(filename);
	ARCAudioStream stream(seq);
	
	stream.play();

	char input = '0';
	while (input != 'q')
	{
		std::cin >> input;
		if (input >= '0' && input <= '9')
			stream.getSequencer().toggleEnable(input - '0');
		if (input >= 'a' && input <= 'f')
			stream.getSequencer().toggleEnable(input - 'a' + 10);
	}

	stream.stop();
}

Sequencer::Sequencer(MidiSequence seq) : sequence_offset(0)
{
	double sec_per_tick;

	if (seq.time_division & 0x8000) // frames per second
	{
		u32 PPQ = (int)((seq.time_division & 0x7F00) >> 8) * (int)(seq.time_division & 0xFF);
		sec_per_tick = 1000000.0 / (double)(PPQ);
	}
	else // ticks per beat
		sec_per_tick = (double) MidiSystem::findTempo(seq) / (double)(seq.time_division & 0x7FFF);

	sec_per_tick = sec_per_tick / 1000000.0 * CD_SAMPLE_RATE;
	std::cout << "MODE: " << (seq.time_division & 0x8000) << std::endl;
	std::cout << sec_per_tick << " samples / tick" << std::endl;

	for (u32 i = 0; i < seq.size; i++)
	{
		if(i >= 6)
			sequencers.push_back(TrackSequencer(new SoundChannel(noise), new Track(&seq.tracks[i], sec_per_tick * 0.4)));
		else
			sequencers.push_back(TrackSequencer(new SoundChannel(squareWave), new Track(&seq.tracks[i], sec_per_tick * 0.4)));
	}
}

short TrackSequencer::nextSample(const u64& tick)
{
	if (!enabled || eventID >= track->size())
		return 0;

	SeqEvent& nextEvent = track->at(eventID);
	while (eventID + 1 < track->size() && tick >= track->at(eventID + 1).tick)
	{
		eventID++;
		if (eventID >= track->size()) return lastSample;
		nextEvent = track->at(eventID);
	}

	lastSample = tick < nextEvent.tick ? 0 : sound->getSample(FREQ_TABLE[nextEvent.tone], nextEvent.volume);
	if (nextEvent.endTick <= tick)
		eventID++;

	/*
	while (nextEvent.tick + nextEvent.length <= tick)
	{
		eventID++;
		if (eventID >= track->size()) return lastSample;
		nextEvent = track->at(eventID);
	}*/

	return lastSample;
}