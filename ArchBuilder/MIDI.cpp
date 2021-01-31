#include "MIDI.h"

using namespace arc;

// Last status parsed in midi file
u8 last_status = 0;

// Loads a MidiFile
// @Param path = path of the file
// @Return whether the operation was successful
bool MidiFile::load(const char* path)
{
	std::cout << "\nLOADING " << path << std::endl;
	std::ifstream stream;
	stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);

	try
	{
		stream.open(path, std::ios::in | std::ios::binary);

		// Header Chunk
		stream.read((char*)&id, 14); // Note the dependence on data member order

		// Swap from Big Endian to Little Endian
		header_size = REu32(header_size);
		format_type = REu16(format_type);
		num_tracks = REu16(num_tracks);
		time_division = REu16(time_division);

		// Check if data is valid
		if (id != 0x6468544D) throw "Invalid header ID";
		if (header_size != 6) throw "Invalid header size";
		if (!num_tracks)      throw "Invalid number of tracks";
		if (!time_division)   throw "Invalid time division";
		std::cout << "   NUM TRACKS = " << num_tracks << std::endl;
		std::cout << "  FORMAT TYPE = " << format_type << std::endl;
		std::cout << "TIME DIVISION = " << time_division << std::endl;

		// Track Chunks
		chunks = new MidiChunk[num_tracks];
		for (u16 i = 0; i < num_tracks; i++)
		{
			stream.read((char*)(chunks + i), 8);

			// Check if track chunk is valid
			if (chunks[i].size == 0) throw "Invalid chunk size";
			if (chunks[i].id != 0x6B72544D) throw "Invalid chunk ID";

			// Copy over data from file
			chunks[i].size = REu32(chunks[i].size);
			chunks[i].buffer = new u8[chunks[i].size];
			stream.read((char*)(chunks[i].buffer), chunks[i].size);
			std::cout << "TRACK " << i << " (" << chunks[i].size << " BYTES)\n";
		}

		stream.close();
		return true;
	}
	catch (const char* err)
	{
		std::cout << "Error while reading MidiFile " << path << ":\n";
		std::cout << err << std::endl;
		return false;
	}
	catch (std::ifstream::failure e) 
	{ 
		std::cout << "Error - " << path << " cannot be read!\n"; 
		return false;
	}
}

// Prints the data contained within the MidiEvent
// @Param id = identifier used to distiguish print statements
void MidiEvent::print(const u32 id) const
{
	std::cout << "Event " << id << ": @" << tick << " (" << std::hex;
	for (u32 e = 0; e < size; e++)
	{
		if (message[0] == 0xFF && e > 2)
			std::cout << message[e];
		else
		{
			std::cout << "0x" << (u32)message[e];
			if (e != size - 1) std::cout << ", ";
		}
	}
	std::cout << std::dec << ")\n";
}

// Loads the Sequence from a MidiFile
// @Param filepath = name of the file (not used)
// @Return whether the operation was successful
bool MidiSequence::load(const char* filepath)
{
	try
	{
		// MidiFile must be valid and the sequence must be empty
		if (!midi.isValid() || tracks != nullptr) throw "Invalid MidiFile loaded";

		// Fills all of the tracks in the sequence
		tracks = new MidiTrack[size];
		for (u16 i = 0; i < size; i++)
			if (!convertToTrack(midi.chunks[i], tracks[i]))
				throw "Cannot load next MidiEvent";
		return true;
	}
	catch (const char* err)
	{
		std::cout << "Error while loading MidiSequence from " << filepath << ":\n";
		std::cout << err << std::endl;
		return false;
	}
}

// From a given location, interprets data as a MidiEvent and adds it to an event list
// @Param data =  current location of the buffer cursor
// @Param data_end = location of the buffer's end, used to determine an overflow
// @Param track = list of Midievents to add MidiEvent to
// @Param tick = current offset of the event tick
// @Return whether the data could be interpreted
bool MidiSequence::addNextEvent(u8*& data, const u8* data_end, MidiTrack& track, u64& tick)
{
	MidiEvent ev;

	try
	{
		// 0-6 = value, 7 = byte extension
		u32 delta_time = (*data) & 0x7F;
		while (*(data++) & 0x80)
		{
			if (data >= data_end) throw "Exceeded buffer size when interpreting MidiEvent tick length";
			delta_time = delta_time << 7;
			delta_time += (*data) & 0x7F;
		}

		// Set the tick location forward based on change in time
		tick += delta_time;
		ev.tick = tick;
		u8 buffer_push = 0;

		// Check if status byte exists
		if (data[0] & 0x80)
		{
			// Set message to point to buffer
			ev.message = data + 1;
			ev.status = data[0];
			last_status = data[0];
			buffer_push = 1;
		}
		else
		{
			if (last_status == 0) throw "Missing status byte at start of file";

			// Set message to point to buffer
			ev.message = data;
			ev.status = last_status;
		}

		// Determine size based on the status id
		if (ev.status == 0xFF) ev.size = ev.message[1] + 2; // TODO: Dynamic event size
		else if ((ev.status & 0xF0) == 0xF0)
		{
			for (ev.size = 0; ev.message[ev.size] != 0xF7; ev.size++); // Count until EOX
			ev.size++;
		}
		else if ((ev.status & 0xE0) == 0xC0) ev.size = 1;
		else ev.size = 2;

		// Check data bytes for invalid values
		if ((ev.status & 0xF0) != 0xF0)
		{
			int i = 1;
			for (u32 i = 0; i < ev.size; i++)
				if (ev.message[i] & 0x80)
					throw "Invalid data byte assigned to midi message parameter";
		}

		// Move buffer cursor and add event to list
		buffer_push += ev.size;
		data += buffer_push;
		if (data > data_end)
		{ 
			throw "Exceeded buffer size when interpreting MidiEvent parameters"; 
		}

		track.push_back(ev);

#ifdef MIDI_READOUT
		ev.print(track.size());
#endif
		return true;
	}
	catch (const char* err)
	{
		std::cout << err << std::endl;
		return false;
	}
}

// Interprets a Track Chunk when filling a MidiTrack
// @Param chunk = the Track chunk that contains th Track data
// @Param track = the list of events to fill
// @Return whether the conversion was successful
bool MidiSequence::convertToTrack(const MidiChunk& chunk, MidiTrack& track)
{
#ifdef MIDI_READOUT
	std::cout << "--- TRACK " << i << " ---\n";
#endif
	u64 tick = 0;
	u8* buffer_cursor = chunk.buffer;
	u8* cursor_end = chunk.buffer + chunk.size;

	try
	{
		// Continues to add MidiEvents until the end of the buffer is reached
		while (buffer_cursor < cursor_end)
			if (!addNextEvent(buffer_cursor, cursor_end, track, tick))
				throw false;
		return true;
	}
	catch (bool err) { return err; }
}

// Subdivides the chunk recursively into smaller chunks
// @Param level = current tree level of the subdivision
// @Return whether the subdivision was successful
bool SF2Chunk::subdivide(const u32 level)
{
	try
	{
		// Print out the tree structure
		for (u32 i = 0; i < level; i++) std::cout << "\t";
		arc::cprint((char*)&id, 4);
		std::cout << ":";
		arc::cprint((char*)&name, 4);
		std::cout << " (" << size << " bytes) \n";

		// Loop until all subchunks are identified
		u32 b = 4;
		while(b < size)
		{
			// Copy the chunk header data over and set the data pointer
			SF2Chunk subchunk;
			subchunk.id = *((u32*)(data + b));
			subchunk.size = *((u32*)(data + b + 4));
			subchunk.name = *((u32*)(data + b + 8));
			subchunk.data = data + b + 8;

			// Check if subchunk is valid
			if (!isValidChunkID(subchunk.id)) break;
			if (subchunk.size == 0) throw "Invalid subchunk size";
			if (subchunk.size >= size) throw "Subchunk size is too large";

			// Subdivide the subchunks
			if (!subchunk.subdivide(level + 1)) 
				return false;

			// Add subchunk to list and increment the buffer offset
			subchunks.push_back(subchunk);
			b += subchunk.size + 8;
		}

		return true;
	}
	catch (const char* err)
	{
		std::cout << err << std::endl;
		return false;
	}
}

// Loads the .sf2 file
// @Param filepath = path of the file
// @Return whether the operation was successful
bool SF2File::load(const char* filepath)
{
	std::cout << "\nLOADING " << filepath << std::endl;
	std::ifstream stream;
	stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);

	try
	{
		stream.open(filepath, std::ios::in | std::ios::binary);

		// Check if header data is valid
		stream.read((char*)&main_chunk, 8);
		if (main_chunk.id != arc::ID("RIFF")) throw "Invalid RIFF ID";
		if (main_chunk.size == 0) throw "Invalid chunk size";

		// Copy the data from the file
		main_chunk.data = new u8[main_chunk.size];
		stream.read((char*)main_chunk.data, main_chunk.size);
		main_chunk.name = *((u32*)main_chunk.data);
		stream.close();

		// Subdivide the main chunk into smaller chunks
		if (!main_chunk.subdivide(0)) throw "Unable to subdivide the main chunk";
		else return true;
	}
	catch (const char* err)
	{
		std::cout << "Error while loading SF2 File " << filepath << ":\n";
		std::cout << err << std::endl;
		return false;
	}
	catch (std::ifstream::failure e)
	{ 
		std::cout << filepath << " cannot be read!\n"; 
		return false;
	}
}

// Applies a generator to the sample
// @Param gen = generator chunk
// @Param samples = list of samples
void FilteredSample::applyGenerator(GENChunk& gen, const SF2Chunk* samples, short* smpl)
{
	u32 shdr_loc = gen.genAmount * SHDR_CHUNK_SIZE;
	SHDRChunk* shdr = (SHDRChunk*)(samples->data + shdr_loc);

	// Each generator has an id that specifies its parameter
	switch (gen.sfGenOper)
	{
	case GEN_START_ADDRESS_OFFSET:	break;
	case GEN_REVERB:				break;
	case GEN_PAN:
		volume_pan = 0.001f * gen.genAmount;
		if (volume_pan > 0.5f) volume_pan = 0.5f;
		if (volume_pan < -0.5f) volume_pan = -0.5f;
		break;
	case GEN_ATTACK_VOL_ENVELOPE:	envelope.attackVolEnv = std::pow(2.0f, (float)gen.genAmount / 1200.0f); break;
	case GEN_HOLD_VOL_ENVELOPE:		envelope.holdVolEnv = std::pow(2.0f, (float)gen.genAmount / 1200.0f); break;
	case GEN_DECAY_VOL_ENVELOPE:	envelope.decayVolEnv = std::pow(2.0f, (float)gen.genAmount / 1200.0f); break;
	case GEN_SUSTAIN_VOL_ENVELOPE:	envelope.sustainVolEnv = gen.genAmount > 0 ? ((float)gen.genAmount / 1000.0f) : 0.0f; break;
	case GEN_RELEASE_VOL_ENVELOPE:	envelope.releaseVolEnv = std::pow(2.0f, (float)gen.genAmount / 1200.0f); break;
	case GEN_INSTRUMENT:			break;
	case GEN_KEY_RANGE:				minKey = (gen.genAmount & 0xFF); maxKey = ((gen.genAmount & 0xFF00) >> 8); break;
	case GEN_VELOCITY_RANGE:		break;
	case GEN_INITIAL_ATTENUATION:	break;
	case GEN_SAMPLE_ID:				loadSample(shdr, smpl); break;
	case GEN_SAMPLE_MODE:			loop = gen.genAmount & 1; break;
	case GEN_SCALE_TUNING:			tuning = gen.genAmount / 100.0f; break;
	case GEN_EXCLUSIVE_CLASS:		break; // FOR SNES.sf2 only
	case GEN_OVERRIDE_ROOT_KEY:		break;
	default:	std::cout << "\t\tUNKNOWN GENERATOR: " << gen.sfGenOper << std::endl;
	}
}

// Loads the Soundfont from a SF2 file
// @Param filepath = path of the file (not used)
// @Return whether the operation was successful
bool Soundfont::load(const char* filepath)
{
	try
	{
		// SF2 file must be valid
		if (!file.isValid()) throw "Invalid SF2 File loaded";

		// Obtains and validates references to chunk data
		const SF2Chunk* ifil = file.getChunkWithID(ID("ifil"));
		const SF2Chunk* isng = file.getChunkWithID(ID("isng"));
		const SF2Chunk* inam = file.getChunkWithID(ID("INAM"));
		const SF2Chunk* smpl = file.getChunkWithID(ID("smpl"));
		if (ifil == nullptr) throw "Missing \"ifil\" chunk";
		if (isng == nullptr) throw "Missing \"isng\" chunk";
		if (inam == nullptr) throw "Missing \"INAM\" chunk";
		if (smpl == nullptr) throw "Missing \"smpl\" chunk";

		// Copies over header data
		version = *((u32*)ifil->data);
		engine_name = (char*)isng->data;
		soundbank_name = (char*)inam->data;

		std::cout << "VERSION: " << (version >> 16) << "." << (version & 0xFF) << std::endl;
		std::cout << "ENGINE: " << engine_name << std::endl;
		std::cout << "SOUNDBANK: " << soundbank_name << std::endl;

		// Sets pointer to sample data
		smpl_size = smpl->size >> 1;
		smpl_u16 = (short*)smpl->data;

		// Fills the Soundbank of the Soundfont
		if (!fillBank()) throw "Unable to create Soundbank";
		return true;
	}
	catch (const char* err)
	{
		std::cout << "Error while reading Soundbank from " << filepath << ":\n";
		std::cout << err << std::endl;
		return false;
	}
}

// Fills the soundbank as much as possible from chunk data
// @Return whether the operation was successful
bool Soundfont::fillBank()
{
	try
	{
		// Obtains and validates references to chunk data
		const SF2Chunk* phdr_chunk = file.getChunkWithID(ID("phdr"));
		const SF2Chunk* pbag_chunk = file.getChunkWithID(ID("pbag"));
		const SF2Chunk* pgen_chunk = file.getChunkWithID(ID("pgen"));
		const SF2Chunk* inst_chunk = file.getChunkWithID(ID("inst"));
		const SF2Chunk* ibag_chunk = file.getChunkWithID(ID("ibag"));
		const SF2Chunk* igen_chunk = file.getChunkWithID(ID("igen"));
		const SF2Chunk* shdr_chunk = file.getChunkWithID(ID("shdr"));
		if (phdr_chunk == nullptr) throw "Missing \"phdr\" chunk";
		if (pbag_chunk == nullptr) throw "Missing \"pbag\" chunk";
		if (pgen_chunk == nullptr) throw "Missing \"pgen\" chunk";
		if (inst_chunk == nullptr) throw "Missing \"inst\" chunk";
		if (ibag_chunk == nullptr) throw "Missing \"ibag\" chunk";
		if (igen_chunk == nullptr) throw "Missing \"igen\" chunk";
		if (shdr_chunk == nullptr) throw "Missing \"shdr\" chunk";

		// Determine number of bags/generators
		u32 phdr_size = phdr_chunk->size / PHDR_CHUNK_SIZE;
		u32 pbag_size = pbag_chunk->size / BAG_CHUNK_SIZE;
		u32 pgen_size = pgen_chunk->size / GEN_CHUNK_SIZE;
		u32 inst_size = inst_chunk->size / INST_CHUNK_SIZE;
		u32 ibag_size = ibag_chunk->size / BAG_CHUNK_SIZE;
		u32 igen_size = igen_chunk->size / GEN_CHUNK_SIZE;
		u32 shdr_size = shdr_chunk->size / SHDR_CHUNK_SIZE;

		std::cout << phdr_size << " BANKS\n";
		std::cout << pbag_size << " PRESET GROUPS\n";
		std::cout << pgen_size << " PRESET GENERATORS\n";
		std::cout << inst_size << " INSTRUMENTS\n";
		std::cout << ibag_size << " INST. GROUPS\n";
		std::cout << igen_size << " INST. GENERATORS\n";
		std::cout << shdr_size << " SHDR CHUNKS\n";

		// # of PHDR chunks = # of PBAG chunks
		//if (phdr_size < pbag_size) throw "PHDR chunk is too small in comparison to PBAG chunk";

		GeneratorMap generators;

		// Create samples for soundbank
		gen_samples = new FilteredSample[ibag_size];
		for (u32 i = 0; i < ibag_size; i++)
		{
			u32 bagLoc = i * BAG_CHUNK_SIZE;
			BAGChunk* bag = (BAGChunk*)(ibag_chunk->data + bagLoc);

			// Determine the range of generators that apply to bank
			u32 nextBag = i + 1;
			u32 nbagLoc = nextBag * BAG_CHUNK_SIZE;
			BAGChunk* nIbag = (BAGChunk*)(ibag_chunk->data + nbagLoc);
			u16 nextGen = nextBag < ibag_size ? nIbag->wGenIndex : igen_size;

			// Add all generators in igen list
			//std::cout << "SAMPLE " << i << ":\n";
			for (u16 g = bag->wGenIndex; g < nextGen; g++)
			{
				u32 genLoc = g * GEN_CHUNK_SIZE;
				GENChunk* igen = (GENChunk*)(igen_chunk->data + genLoc);
				generators.put(igen);
				//std::cout << "\tGEN " << g << " (" << igen->sfGenOper << ", " << igen->genAmount << ")\n";
			}

			// Apply the generator to instrument
			generators.applyTo(gen_samples[i], shdr_chunk, smpl_u16);
		}

		std::vector<GENChunk> presets;

		// Iterate though every bank
		for (u32 i = 0; i < pbag_size; i++)
		{
			// Obtain PHDR/PBAG chunk
			u32 phdrLoc = i * PHDR_CHUNK_SIZE;
			u32 bagLoc = i * BAG_CHUNK_SIZE;
			PHDRChunk* phdr = (PHDRChunk*)(phdr_chunk->data + phdrLoc);
			BAGChunk* pbag = (BAGChunk*)(pbag_chunk->data + bagLoc);

			// Determine the range of generators that apply to bank
			u32 nextBag = i + 1;
			u32 nbagLoc = nextBag * BAG_CHUNK_SIZE;
			BAGChunk* nPbag = (BAGChunk*)(pbag_chunk->data + nbagLoc);
			u16 nextGen = nextBag < pbag_size ? nPbag->wGenIndex : pgen_size;

			// Iterate through every generator within range
			for (u16 g = pbag->wGenIndex; g < nextGen; g++)
			{
				u32 genLoc = g * GEN_CHUNK_SIZE;
				GENChunk* pgen = (GENChunk*)(pgen_chunk->data + genLoc);

				// If generator for bank points to an instrument chunk
				if (pgen->sfGenOper == GEN_INSTRUMENT)
				{
					// Obtain instrument chunk
					u32 instLoc = pgen->genAmount * INST_CHUNK_SIZE;
					INSTChunk* inst = (INSTChunk*)(inst_chunk->data + instLoc);

					u16 instBag = inst->wInstBagIndex;
					u16 nextInst = pgen->genAmount + 1;
					u32 nInstLoc = nextInst * INST_CHUNK_SIZE;
					INSTChunk* nInst = (INSTChunk*)(inst_chunk->data + nInstLoc);
					u16 nextIBag = nextInst < inst_size ? nInst->wInstBagIndex : ibag_size;

					std::cout << "INSTRUMENT " << pgen->genAmount << ": " << " (SAMPLE = " << instBag << ") (PATCH = " << phdr->wPreset << ") (BANK = " << phdr->wBank << ")\n";

					for (u16 smplIdx = instBag; smplIdx < nextIBag; smplIdx++)
					{
						// TODO: Apply all presets to selected sample without overwriting existing generators
						//for (u32 i = 0; i < presets.size(); i++)
						//	gen_samples[i].applyGenerator(presets[i], shdr_chunk, smpl_u16);

						soundbank[phdr->wBank][phdr->wPreset].addSample(&gen_samples[smplIdx]);
					}
				}
				else
				{
					bool newGen = true;
					for (u32 i = 0; i < presets.size(); i++)
					{
						if (presets[i].sfGenOper == pgen->sfGenOper)
						{
							presets[i].genAmount = pgen->genAmount;
							newGen = false;
						}
					}

					if(newGen)
						presets.push_back(*pgen);

					std::cout << "PGEN " << g << " (" << pgen->sfGenOper << ", " << pgen->genAmount << ")\n";
				}
			}
		}

		return true;
	}
	catch (const char* err)
	{
		std::cout << err << std::endl;
		return false;
	}
}

// Finds next event that ends/starts a note of a given tone/channel
// @Param track = track to search through
// @Param index = starting from index of list
// @Param channel = channel that event applies to
// @Param tone = tone that event applies to
// @Return tick location of the next event
u64 MidiSystem::nextEndTick(const MidiTrack& track, const u32 index, const u8 channel, const u8& tone)
{
	for (size_t i = index; i < track.size(); i++)
		if (isMessageNote(track[i]) && equals(track[i], channel, tone))
			return track[i].tick;
	return 0;
}

// Finds a message with given parameters within a MidiTrack
// @Param track = track to search through
// @Param m0 = parameter 0 of event
// @Param m1 - parameter 1 of event
// @Return reference to MidiEvent
const MidiEvent* MidiSystem::findMessage(const MidiTrack& track, const u8 m0, const u8 m1)
{
	for (size_t m = 0; m < track.size(); m++)
		if (track[m].size > 2 && track[m].message[0] == m0 && track[m].message[1] == m1)
			return &track[m];
	return nullptr;
}

// Finds a message with given parameters within a MidiSequence
// @Param sequence = sequence to search through
// @Param m0 = parameter 0 of event
// @Param m1 - parameter 1 of event
// @Return reference to MidiEvent
const MidiEvent* MidiSystem::findMessage(const MidiSequence& sequence, const u8 m0, const u8 m1)
{
	// Iterates through each track
	const MidiEvent* rtrn_event = nullptr;
	for (size_t t = 0; t < sequence.size; t++)
	{
		// Finds message from each track
		const MidiEvent* event = findMessage(sequence.tracks[t], 0xFF, 0x51);
		if (event != nullptr)
		{
			// If multiple tracks have message, the last track is used
			if (event != nullptr)
				std::cout << "Duplicate Messages detected!";
			rtrn_event = event;
		}
	}
	return rtrn_event;
}

/*
// Attempts to upscale the waveform (Warning: waveform will NOT get deleted automatically)
void upscaleWaveform(u16 factor)
{
	u32 new_size = total_sample_length * factor;
	int sample_offset = (int)(initial_offset * sample_length);
	short* src_sample = inst_sample + sample_offset;
	short* new_sample = new short[new_size];

	for (u32 i = 0; i < new_size; i++)
	{
		u32 x = (u32)((double)i / new_size * total_sample_length);

		if (i % factor == 0 || i >= new_size - factor)
		{
			new_sample[i] = src_sample[x];
		}
		else
		{
			// Weight between last and current sample
			int lts = src_sample[x];
			int nxs = src_sample[x + 1];
			double weight = (double)(i % factor) / factor;

			new_sample[i] = (short)(lts * (1.0 - weight) + nxs * weight);
		}
	}

	sample_rate *= factor;
	sample_length *= factor;
	total_sample_length = new_size;

	sample_offset = (int)(initial_offset * sample_length);
	inst_sample = new_sample - sample_offset;
}*/
