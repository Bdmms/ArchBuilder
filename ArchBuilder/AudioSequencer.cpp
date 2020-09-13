#include "AudioSequencer.h"

bool Sequencer::handleEvent(const MidiEvent& event)
{
	const u8 chan = event.message[MESSAGE_STATUS] & 0x0F;
	SoundChannel& channel = *channels[chan];

	switch (event.message[MESSAGE_STATUS] & 0xF0)
	{
	case STATUS_CHANNEL_OFF:
		channel.stop();
		break;

	case STATUS_CHANNEL_ON:
		channel.bindTone(MESSAGE_TONE);
		channel.start(lookup_frequency[event.message[MESSAGE_TONE]], ((sample)event.message[MESSAGE_VOLUME] / 0x7F));
		break;

	case STATUS_NOTE_AFTERTOUCH:
		break;

	case STATUS_CHANNEL_CONTROLLER:
		switch (event.message[1])
		{
		case CONTROLLER_CHANNEL_MODULATION: break;
		case CONTROLLER_CHANNEL_DATA_ENTRY_MSB: break;
		case CONTROLLER_CHANNEL_VOLUME: channel.setVolume((sample)event.message[2] / 0x7F); break;
		case CONTROLLER_CHANNEL_PAN: channel.setPan(((sample)event.message[2] / 0x7F) - 0.5f); break;
		case CONTROLLER_CHANNEL_DATA_ENTRY_LSB: break;
		case CONTROLLER_NON_REGISTERED_PARAM_LSB: break;
		case CONTROLLER_NON_REGISTERED_PARAM_MSB: break;
		case CONTROLLER_REGISTERED_PARAM_LSB: break;
		case CONTROLLER_REGISTERED_PARAM_MSB: break;
		default: std::cout << "UNKNOWN CONTROLLER " << (u32)event.message[1] << ": " << (u32)event.message[2] << "\n";
		}
		break;

	case STATUS_CHANNEL_BANK_CHANGE:
		if (soundbank != nullptr && soundbank[event.message[1]].sample != nullptr)
		{
			std::cout << "BANK CHANGE: " << (int)chan << " -> " << (u32)event.message[1] << std::endl;
			SoundChannel* temp = channels[chan];
			channels[chan] = new SampleGenerator(soundbank[event.message[1]], lookup_frequency);
			delete temp;
			break;
		}
		break;

	case STATUS_CHANNEL_AFTERTOUCH:
		break;

	case STATUS_CHANNEL_PITCHBEND:
		channel.setBend(1.0f + (smpl_time)((event.message[1] & 0x7F) | ((event.message[2] & 0x7F) << 7) - 8192) / 8192.0f * DUALTONE);
		break;

	case STATUS_SYSTEM_EXCLUSIVE:
		if (event.message[MESSAGE_STATUS] == STATUS_META_MESSAGE)
		{
			switch (event.message[1])
			{
			case META_TEXT: break;
			case META_COPYRIGHT: break;
			case META_TRACK_NAME: break;
			case META_INSTRUMENT_NAME: break;
			case META_LYRICS: break;
			case META_END_OF_TRACK: return true;
			case META_TEMPO_CHANGE:
				if (!(time_division & 0x8000))
				{
					u64 tempo = 0;
					for (u8 b = 0; b < event.message[2]; b++)
					{
						tempo = tempo << 8;
						tempo |= event.message[3 + b];
					}

					float sample_tempo = (float)CD_SAMPLE_RATE * (float)tempo * TEMPO_MODIFIER;
					tick_frequency = (1000000.0f * time_division) / sample_tempo; // samples per ticks
				}
				break;
			case META_TIME_SIGNATURE: break;
			case META_SEQUENCER_SPECIFIC: break;
			default: std::cout << "UNKNOWN META MESSAGE: " << (u32)event.message[1] << std::endl;
			}
		}
		break;

	default: std::cout << "UNKNOWN EVENT STATUS 0x" << std::hex << (u32)event.message[MESSAGE_STATUS] << std::dec << " AT " << sequence_offset << std::endl;
	}

	return false;
}
