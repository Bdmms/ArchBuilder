#include "AudioAdapter.h"
#include <cmath>
#include <SFML/Graphics.hpp>
#include <string.h>

// Disable warnings from SFML library
#pragma warning(disable : 26812)

using namespace std::literals::string_view_literals;
using namespace std::literals::string_literals;

const std::string FILE_LOCATION_AUDIO = "AUDIO\\"s;
const std::string FILE_LOCATION_SOUNDBANK = "AUDIO\\__SF2__\\"s;
const std::string FILE_EXTENSION_MID = ".mid"s;
const std::string FILE_EXTENSION_SF2 = ".sf2"s;

void showOscilloscope(ARCAudioStream& stream, const u32& buffer_size);

inline void playStream(ARCAudioStream& stream, const u32& buffer_size, const bool& oscilloscope)
{
	stream.play();

	if (oscilloscope)
		showOscilloscope(stream, buffer_size);
	else
	{
		char quit;
		std::cin >> quit;
	}

	stream.stop();

	// Wait for clean-up to complete
	while (stream.getStatus() == ARCAudioStream::Playing)
	{
		sf::sleep(sf::seconds(0.1f));
	}
}

void ARCAudioStream::playKeyboard(Soundfont* sf2, const bool& oscilloscope)
{
	u32 buffer_size = CD_SAMPLE_RATE / 60;
	MidiController controller;
	ARCAudioStream stream(&controller, buffer_size * 2); // chunk size and num of channels

	if (sf2 != nullptr)
		controller.apply(sf2->getSoundbanks());

	controller.setChannel(0, new FMSingleStreamChannel(FMSynth::squareWave50));
	for (int i = 0; i < 16; i++)
	{
		controller.set(i + 1, STATUS_CHANNEL_BANK_CHANGE, i, 0);
	}

	playStream(stream, buffer_size, oscilloscope);
}

void ARCAudioStream::playFrom(const char* filename, Soundfont* sf2, const bool& oscilloscope)
{
	const std::string filepath = FILE_LOCATION_AUDIO + filename + FILE_EXTENSION_MID;
	MidiSequence seq(filepath.c_str());

	if (!seq.isValid()) return;

	u32 buffer_size = CD_SAMPLE_RATE / 60;

	MidiSequencer controller(seq);
	ARCAudioStream stream( &controller, buffer_size * 2 ); // chunk size and num of channels

	if (sf2 != nullptr)
		controller.apply(sf2->getSoundbanks());
	else
	{
		for (u32 i = 0; i < 16; i++)
		{
			if (i == 8 || i == 9)	controller.setChannel(i, new FMSingleStreamChannel(FMSynth::noise));
			else		controller.setChannel(i, new FMSingleStreamChannel(FMSynth::sudoWave));
		}
	}

	playStream(stream, buffer_size, oscilloscope);
}

void ARCAudioStream::playFile(const char* filename, const bool& sampled, const bool& oscilloscope)
{
	const u32 buffer_size = CD_SAMPLE_RATE / 60;
	const std::string midipath = FILE_LOCATION_AUDIO + filename + FILE_EXTENSION_MID;

	MidiSequence seq(midipath.c_str());
	MidiSequencer controller(seq);
	ARCAudioStream stream( &controller, buffer_size * 2 ); // chunk size and num of channels

	if (sampled)
	{
		const std::string sf2path = FILE_LOCATION_SOUNDBANK + filename + FILE_EXTENSION_SF2;
		Soundfont* soundfont = new Soundfont(sf2path.c_str());

		if (!soundfont->isValid()) return;
		controller.apply(soundfont->getSoundbanks());

		playStream(stream, buffer_size, oscilloscope);
		delete soundfont;
	}
	else
	{
		for (u32 i = 0; i < 16; i++)
		{
			if(i == 9)	controller.setChannel(i, new FMSingleStreamChannel(FMSynth::noise));
			else		controller.setChannel(i, new FMSingleStreamChannel(FMSynth::sudoWave));
		}

		playStream(stream, buffer_size, oscilloscope);
	}
}

void drawBuffer(const sample* buffer, const u32& buffer_size, u32* frame, const u32 width, const u32 height, const u32& scanline)
{
	u32 half_height = (height >> 1);
	u32 curr_sample = half_height - (int)(half_height * buffer[0]);

	for (u32 x = 1; x < width; x++)
	{
		u32 next_sample = half_height - (int)(half_height * buffer[(u32)((float)x / width * buffer_size)]);
		frame[x + curr_sample * scanline] = 0xFFFFFFFF;

		if (curr_sample < next_sample)
			for (u32 y = curr_sample; y < next_sample && y < height; y++)
				frame[x + y * scanline] = 0xFFFFFFFF;
		else
			for (u32 y = next_sample; y < curr_sample && y < height; y++)
				frame[x + y * scanline] = 0xFFFFFFFF;

		curr_sample = next_sample;
	}
}

inline void showOscilloscope(ARCAudioStream& stream, const u32& buffer_size)
{
	u32 div_width = 300;
	u32 div_height = 150;
	u32 frame_width = div_width * 4;
	u32 frame_height = div_height * 4;
	u32 frame_size = (frame_width * frame_height);
	u32* frame = new u32[frame_size];
	bool key_pressed[16] = { false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false };
	sf::RenderWindow window(sf::VideoMode(frame_width, frame_height), "GBA Synthesizer");
	sf::Texture texture;
	texture.create(frame_width, frame_height);
	sf::Sprite sprite(texture);
	sf::Event event;
	bool exit = false;
	bool view = false;
	u8 channel = 0;
	u8 octave = C5;

	window.setActive(true);
	window.setVerticalSyncEnabled(true);
	window.setFramerateLimit(60);

	while (!exit)
	{
		for (u32 i = 0; i < frame_size; i++) frame[i] = 0xFF000000;

		while (window.pollEvent(event))
		{
			switch (event.type)
			{
			case sf::Event::Closed: window.close(); break;
			case sf::Event::KeyPressed:
				if (event.key.code >= sf::Keyboard::Num0 && event.key.code <= sf::Keyboard::Num9 || event.key.code == sf::Keyboard::Z)
				{
					u8 key = (event.key.code - sf::Keyboard::Num0) & 0xF;
					if (key_pressed[key])
						break;

					key_pressed[key] = true;
					stream.getController()->set(channel, STATUS_CHANNEL_ON, octave + key, 100);
				}
				else if (event.key.code == sf::Keyboard::Up)    octave = (octave + 12) & 0x7F;
				else if (event.key.code == sf::Keyboard::Down)  octave = (octave - 12) & 0x7F;
				else if (event.key.code == sf::Keyboard::Left)  channel = (channel - 1) & 0x0F;
				else if (event.key.code == sf::Keyboard::Right) channel = (channel + 1) & 0x0F;
				else if (event.key.code == sf::Keyboard::P)		stream.getController()->paused = !stream.getController()->paused;
				else if (event.key.code == sf::Keyboard::S)		stream.getController()->skip(1000);
				else if (event.key.code == sf::Keyboard::V)		view = !view;
				exit = event.key.code == sf::Keyboard::Q;
				break;
			case sf::Event::KeyReleased:
				if (event.key.code >= sf::Keyboard::Num0 && event.key.code <= sf::Keyboard::Num9 || event.key.code == sf::Keyboard::Z)
					key_pressed[(event.key.code - sf::Keyboard::Num0) & 0xF] = false;
				key_pressed[15] = false;

				for (u32 i = 0; i < 11; i++)
					if (key_pressed[i])
						key_pressed[15] = true;

				if (key_pressed[15])
					break;

				stream.getController()->set(channel, STATUS_CHANNEL_OFF, octave + event.key.code - sf::Keyboard::Num0, 0);
				break;
			}
		}

		if (view)
		{
			drawBuffer(stream.getChannelBuffer(channel), buffer_size, frame, frame_width, frame_height, frame_width);
		}
		else
		{ 
			for (u32 i = 0; i < MIDI_NUM_CHANNELS; i++)
			{
				u32 offset = div_width * (i % 4) + div_height * frame_width * (i / 4);
				drawBuffer(stream.getChannelBuffer(i), buffer_size, frame + offset, div_width, div_height, frame_width);
			}
		}

		texture.update((sf::Uint8*)frame);
		window.draw(sprite);
		window.display();
	}
	delete[] frame;
}
