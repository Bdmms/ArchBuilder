#include "AudioController.h"

bool MidiController::handleEvent(const MidiEvent& event)
{
	u8 chan = event.status & 0x0F;
	SoundChannel& channel = channels[chan];
	ChannelData& cdata = data[chan];

	switch (event.status & 0xF0)
	{
	case STATUS_CHANNEL_OFF:
		SoundDriver::stop(channel);
		break;

	case STATUS_CHANNEL_ON:
	{
		// PITCH SHIFT
		smpl_time pitchCorrectness = bindTone(channel, cdata, event.message[MESSAGE_TONE]);

		cdata.freq = lookup_frequency[event.message[MESSAGE_TONE]] * pitchCorrectness;
		cdata.volume = (sample)event.message[MESSAGE_VOLUME] / 0x7F;

		channel.frequency = cdata.freq * cdata.freq_mod;
		channel.volume = cdata.volume * cdata.vol_mod;
		channel.sample_offset = cdata.initial_offset;

		SoundDriver::start(channel);
		break;
	}

	case STATUS_NOTE_AFTERTOUCH:
		break;

	case STATUS_CHANNEL_CONTROLLER:
		switch (event.message[0])
		{
		case CONTROLLER_CHANNEL_BANK_SELECT: 
			selected_bank = event.message[1]; 
			break;
		case CONTROLLER_CHANNEL_MODULATION: break;
		case CONTROLLER_CHANNEL_DATA_ENTRY_MSB: break;
		case CONTROLLER_CHANNEL_VOLUME: 
			cdata.vol_mod = (sample)event.message[1] / 0x7F;
			channel.volume = cdata.volume * cdata.vol_mod;
			break;
		case CONTROLLER_CHANNEL_PAN: 
			channel.stereo_pan = ((sample)event.message[1] / 0x7F) - DEFAULT_PAN; 
			break;
		case CONTROLLER_CHANNEL_DATA_ENTRY_LSB: break;
		case CONTROLLER_NON_REGISTERED_PARAM_LSB: break;
		case CONTROLLER_NON_REGISTERED_PARAM_MSB: break;
		case CONTROLLER_REGISTERED_PARAM_LSB: break;
		case CONTROLLER_REGISTERED_PARAM_MSB: break;
		default: std::cout << "UNKNOWN CONTROLLER " << (u32)event.message[0] << ": " << (u32)event.message[1] << "\n";
		}
		break;

	case STATUS_CHANNEL_BANK_CHANGE:
		SoundDriver::stop(channel);
		if (soundbank != nullptr)
		{
			if (soundbank[selected_bank][event.message[0]].sample == nullptr)
			{
				std::cout << "Error - Missing Bank: " << (u32)event.message[0] << std::endl;
				return false;
			}

			data[chan].inst = &soundbank[selected_bank][event.message[0]];
			channel.loop_sample = nullptr;
			channel.audio_driver = SoundDriver::sampleSF;
		}
		break;

	case STATUS_CHANNEL_AFTERTOUCH:
		break;

	case STATUS_CHANNEL_PITCHBEND:
		cdata.freq_mod = pow(2.0f, 1.0f * ((int)((event.message[0] & 0x7F) | ((event.message[1] & 0x7F) << 7)) - 8192) / 49152.0f);
		channel.frequency = cdata.freq * cdata.freq_mod;
		break;

	case STATUS_SYSTEM_EXCLUSIVE:
		if (event.status == STATUS_META_MESSAGE)
		{
			switch (event.message[0])
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
					for (u8 b = 0; b < event.message[1]; b++)
					{
						tempo = tempo << 8;
						tempo |= event.message[2 + b];
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

	default: std::cout << "UNKNOWN EVENT STATUS 0x" << std::hex << (u32)event.status << std::dec << " AT " << sequence_offset << std::endl;
	}

	return false;
}
