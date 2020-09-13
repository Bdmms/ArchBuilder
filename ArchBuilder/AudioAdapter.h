#pragma once

#ifndef AUDIO_ADAPTER_H
#define AUDIO_ADAPTER_H

//#define MIDI_READOUT

#include <SFML/Audio.hpp>
#include "AudioSequencer.h"

#ifdef AUDIO_DOUBLE_PRECISION
constexpr u32 SAMPLES_PER_CALL = 8;
#else
constexpr u32 SAMPLES_PER_CALL = 16;
#endif

class ARCAudioStream : public sf::SoundStream
{
private:
	SoundChannel** channels;
	Sequencer sequencer;
	sample** channel_buffer;
	short* buffer;

public:
	const u32 chunk_size;
	const u32 num_channels;
	const sample per_channel_volume;

	static void playToChannels(SoundChannel** channels, const char* filename, const bool& oscilloscope = true, Soundfont* sf2 = nullptr);
	static void playFrom(const char* filename, const bool& sampled = true, const bool& oscilloscope = true);

	ARCAudioStream(SoundChannel** chs, const u32 sz, const u32 chans) : 
		channels(chs), sequencer(chs), chunk_size(sz), num_channels(chans), per_channel_volume((sample_float)(0xFFFF / num_channels))
	{ 
		channel_buffer = new sample*[num_channels];
		buffer = new short[chunk_size];

		for (u32 i = 0; i < num_channels; i++)
		{
			#ifdef AUDIO_PARALLEL
				#ifdef AUDIO_DOUBLE_PRECISION
					channel_buffer[i] = new sample[chunk_size >> 2];
				#else
					channel_buffer[i] = new sample[chunk_size >> 3];
				#endif
			#else
				channel_buffer[i] = new sample[chunk_size];
			#endif
		}

		initialize(2, OUT_SAMPLE_RATE);
	}
	~ARCAudioStream() 
	{ 
		for (u32 i = 0; i < num_channels; i++)
			delete[] channel_buffer[i];
		delete[] buffer; 
		delete[] channel_buffer;
	}

	constexpr short getSample(const u32& i) { return buffer[i]; }
	constexpr sample* getChannelBuffer(const u32& c) { return channel_buffer[c]; }

	constexpr Sequencer& getSequencer() { return sequencer; }
	constexpr void toggleChannel(const u8 c) { if (c < num_channels) channels[c]->muted = !channels[c]->muted; }

	inline void setSoundChannel(SoundChannel* channel, const u32 i)
	{
		delete channels[i];
		channels[i] = channel;
	}

	bool onGetData(Chunk& data) override
	{
#ifndef AUDIO_PARALLEL
		for (u32 i = 0; i < chunk_size; i += 2)
		{
			sequencer.tick();
			buffer[i    ] = 0;
			buffer[i + 1] = 0;
			for (u32 c = 0; c < num_channels; c++)
			{
				sample& left = channel_buffer[c][i];
				sample& right = channel_buffer[c][i + 1];
				channels[c]->getSample(left, right);
				buffer[i    ] += (short)(left  * per_channel_volume);
				buffer[i + 1] += (short)(right * per_channel_volume);
			}
		}
#else
		for (u32 i = 0, b = 0; i < chunk_size; i += SAMPLES_PER_CALL, b += 2)
		{
			sample leftTotal = 0;
			sample rightTotal = 0;

			for (u32 x = 0; x < SAMPLES_PER_CALL; )
			{
				sequencer.tick();
				buffer[i + x++] = 0;
				buffer[i + 1 + x++] = 0;
			}

			for (u32 c = 0; c < num_channels; c++)
			{
				sample& left = channel_buffer[c][b];
				sample& right = channel_buffer[c][b + 1];
				channels[c]->getSample(left, right);
				leftTotal += left * per_channel_volume;
				rightTotal += right * per_channel_volume;
			}

			buffer[i + 0] += (short)leftTotal[0];
			buffer[i + 2] += (short)leftTotal[1];
			buffer[i + 4] += (short)leftTotal[2];
			buffer[i + 6] += (short)leftTotal[3];
			buffer[i + 1] += (short)rightTotal[0];
			buffer[i + 3] += (short)rightTotal[1];
			buffer[i + 5] += (short)rightTotal[2];
			buffer[i + 7] += (short)rightTotal[3];

		#ifndef AUDIO_DOUBLE_PRECISION
			buffer[i + 8] += (short)leftTotal[4];
			buffer[i + 10] += (short)leftTotal[5];
			buffer[i + 12] += (short)leftTotal[6];
			buffer[i + 14] += (short)leftTotal[7];
			buffer[i + 9] += (short)rightTotal[4];
			buffer[i + 11] += (short)rightTotal[5];
			buffer[i + 13] += (short)rightTotal[6];
			buffer[i + 15] += (short)rightTotal[7];
		#endif

			i += SAMPLES_PER_CALL;
		}
#endif

		data.samples = buffer;
		data.sampleCount = chunk_size; // size of chunk in samples
		return true;
	}

	inline void onSeek(sf::Time timeOffset) override
	{
		// Called when setPlayingOffset is called
		std::cout << "WARNING - THIS IS NOT A SEQUENCER\n";
	}
};

#endif