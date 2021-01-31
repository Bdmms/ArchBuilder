#pragma once

#ifndef AUDIO_ADAPTER_H
#define AUDIO_ADAPTER_H

//#define MIDI_READOUT

#include <SFML/Audio.hpp>
#include "AudioController.h"

class ARCAudioStream : public sf::SoundStream
{
private:
	Controller* controller;
	sample** channel_buffer;
	short* buffer;
	sample per_channel_volume;
	u32 num_channels;

public:
	const u32 chunk_size;

	static void playKeyboard(Soundfont* sf2 = nullptr, const bool& oscilloscope = true);
	static void playFrom(const char* filename, Soundfont* sf2 = nullptr, const bool& oscilloscope = true);
	static void playFile(const char* filename, const bool& sampled = true, const bool& oscilloscope = true);

	ARCAudioStream(Controller* controller, const u32 sz) : controller(controller), chunk_size(sz)
	{ 
		num_channels = controller->numChannels();
		per_channel_volume = (sample_float)(0xFFFF / num_channels);

		channel_buffer = new sample*[num_channels];
		buffer = new short[chunk_size];

		for (u32 i = 0; i < num_channels; i++)
		{
			channel_buffer[i] = new sample[chunk_size];
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
	constexpr Controller* getController() { return controller; }

	bool onGetData(Chunk& data) override
	{
		SoundChannel* channels = controller->getChannels();

		for (u32 i = 0; i < chunk_size; i += 2)
		{
			controller->tick();
			buffer[i    ] = 0;
			buffer[i + 1] = 0;
			for (u32 c = 0; c < num_channels; c++)
			{
				SoundChannel& channel = channels[c];
				channel.audio_driver(channels + c);

				sample* cbuffer = channel_buffer[c] + i;
				*(cbuffer++) = channel.left_sample;
				*(cbuffer--) = channel.right_sample;

				buffer[i    ] += (short)(*(cbuffer++) * per_channel_volume);
				buffer[i + 1] += (short)(*(cbuffer--) * per_channel_volume);
			}
		}

		data.samples = buffer;
		data.sampleCount = chunk_size; // size of chunk in samples
		return true;
	}

	inline void onSeek(sf::Time timeOffset) override
	{
		// Called when setPlayingOffset is called
	}
};

#endif