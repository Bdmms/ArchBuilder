#include "AudioAdapter.h"
#include <cmath>
#include <SFML/Graphics.hpp>

constexpr const char* FILE_LOCATION_AUDIO = "AUDIO\\";
constexpr const char* FILE_LOCATION_SOUNDBANK = "AUDIO\\soundsf2\\";
constexpr const char* FILE_EXTENSION_MID = ".mid";
constexpr const char* FILE_EXTENSION_SF2 = ".sf2";

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

	while (stream.getStatus() == ARCAudioStream::Playing)
		sf::sleep(sf::seconds(0.1f));
}

void ARCAudioStream::playToChannels(SoundChannel** channels, const char* filename, const bool& oscilloscope, Soundfont* sf2)
{
	const char* midArr[3] = { FILE_LOCATION_AUDIO, filename, FILE_EXTENSION_MID };
	const char* mid_filename = arc::concat(midArr, 3);
	MidiSequence seq(mid_filename);

	if (!seq.isValid()) return;

	u32 buffer_size = CD_SAMPLE_RATE / 60;
	ARCAudioStream stream(channels, buffer_size * 2, MIDI_NUM_CHANNELS); // chunk size and num of channels
	stream.getSequencer().load(seq);

	if (sf2 != nullptr)
		stream.getSequencer().apply(sf2->getSoundbank());

	playStream(stream, buffer_size, oscilloscope);
}

void ARCAudioStream::playFrom(const char* filename, const bool& sampled, const bool& oscilloscope)
{
	SoundChannel* channels[MIDI_NUM_CHANNELS];
	for (u8 i = 0; i < MIDI_NUM_CHANNELS; i++)
		channels[i] = new Oscillator(squareWave);
	
	const char* midArr[3] = { FILE_LOCATION_AUDIO, filename, FILE_EXTENSION_MID };
	const char* mid_filename = arc::concat(midArr, 3);
	MidiSequence seq(mid_filename);

	u32 buffer_size = CD_SAMPLE_RATE / 60;
	ARCAudioStream stream(channels, buffer_size * 2, MIDI_NUM_CHANNELS); // chunk size and num of channels
	stream.getSequencer().load(seq);

	if (sampled)
	{
		const char* sf2Arr[3] = { FILE_LOCATION_SOUNDBANK, filename, FILE_EXTENSION_SF2 };
		const char* sf2_filename = arc::concat(sf2Arr, 3);
		Soundfont soundfont(sf2_filename);

		if (!soundfont.isValid()) return;
		stream.getSequencer().apply(soundfont.getSoundbank());
	}

	playStream(stream, buffer_size, oscilloscope);

	// Release the heap
	for (u8 i = 0; i < 16; i++)
		delete channels[i];
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
					MidiEvent ev;
					ev.size = 3;
					ev.message = new u8[ev.size];
					ev.message[MESSAGE_STATUS] = STATUS_CHANNEL_ON | channel;
					ev.message[MESSAGE_TONE] = octave + key;
					ev.message[MESSAGE_VOLUME] = 100;
					stream.getSequencer().handleEvent(ev);
					delete[] ev.message;
				}
				else if (event.key.code == sf::Keyboard::Up)  octave = (octave + 12) & 0x7F;
				else if (event.key.code == sf::Keyboard::Down) octave = (octave - 12) & 0x7F;
				else if (event.key.code == sf::Keyboard::Left)  channel = (channel - 1) & 0x0F;
				else if (event.key.code == sf::Keyboard::Right) channel = (channel + 1) & 0x0F;
				else if (event.key.code == sf::Keyboard::P)		stream.getSequencer().paused = !stream.getSequencer().paused;
				else if (event.key.code == sf::Keyboard::S)		stream.getSequencer().skip( 1000 );
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

				MidiEvent ev;
				ev.size = 3;
				ev.message = new u8[ev.size];
				ev.message[MESSAGE_STATUS] = STATUS_CHANNEL_OFF | channel;
				ev.message[MESSAGE_TONE] = octave + event.key.code - sf::Keyboard::Num0;
				ev.message[MESSAGE_VOLUME] = 0;
				stream.getSequencer().handleEvent(ev);
				delete[] ev.message;
				break;
			}
		}

		for (u32 i = 0; i < MIDI_NUM_CHANNELS; i++)
		{
			u32 offset = div_width * (i % 4) + div_height * frame_width * (i / 4);
			drawBuffer(stream.getChannelBuffer(i), buffer_size, frame + offset, div_width, div_height, frame_width);
		}

		texture.update((sf::Uint8*)frame);
		window.draw(sprite);
		window.display();
	}
	delete[] frame;
}
