#include "SF2.h"

using namespace arc;

// Subdivides the chunk recursively into smaller chunks
// @Param level = current tree level of the subdivision
// @Return whether the subdivision was successful
bool SF2Chunk::subdivide(const u32 level = 0) try
{
#ifdef SF2_READOUT
	// Print out the tree structure
	for (u32 i = 0; i < level; i++) std::cout << "\t";
	std::cout << arc::toString(id) << ":" << arc::toString(name) << " (" << size << " bytes) \n";
#endif

	// Loop until all subchunks are identified
	u32 b = 4;
	while (b < size)
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
catch (const std::exception& ex)
{
	std::cout << ex.what() << std::endl;
	return false;
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
		if (!main_chunk.subdivide()) throw "Unable to subdivide the main chunk";
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

					if (newGen)
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
