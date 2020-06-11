#include "AudioAdapter.h"
#include <cmath>

constexpr float PI = 3.141592654f;
constexpr float PI2 = PI * 2;

float squareWave(const float pos) { return pos < 0.5f ? 1.0f : 0.0f; }
float triangleWave(const float pos) { return pos < 0.5f ? pos * 2 : -pos * 2; }
float sawtoothWave(const float pos) { return pos; }
float sineWave(const float pos) { return sin(pos * PI2); }
float kickSquareWave(const float pos) { return (pos < 0.5f ? sin(pos * PI2 * 0.5f) - 1.0f : cos(pos * PI2 * 0.5f) + 1.0f); }

float compWave(const float pos) 
{ 
	if (pos < 0.33333f) return sin(pos * PI2 * 1.5f);
	if (pos < 0.66666f) return -0.2f * sin(pos * PI2 * 3.0f);
	return -sin(pos * PI2 * 1.5f);
}

float fluteWave(const float pos)
{
	if (pos < 0.5f) return -sin(pos * PI2);
	return 0.5f * sin(pos * PI2 * 2.0f);
}

const u32 smp_sz = 256;
float* wave_sample = new float[smp_sz];
float sampleWave(const float pos) { return wave_sample[(u32)(pos * smp_sz)]; }

void ARCAudioStream::test()
{
	WaveGenerator sound(squareWave);
	Sequence sequence;
	Sequencer seq(&sound, &sequence);

	for (u32 i = 0; i < 8; i++)
		sequence.addEvent(0.4f * i, 0.4f, 3000, A4 + i);

	ARCAudioStream stream(&seq);
	stream.play();
	sf::sleep(sf::seconds(10.0f));
	stream.stop();
}