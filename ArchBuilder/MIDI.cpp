#include "MIDI.h"

using namespace arc;

// Last status parsed in midi file
u8 last_status = 0;

/**
 * @brief Loads a MidiFile
 * @param path - path of the file
 * @return whether the operation was successful
*/
bool MidiFile::load(const char* path) try
{
	std::cout << "\nLOADING " << path << std::endl;
	std::ifstream stream;
	stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	stream.open(path, std::ios::in | std::ios::binary);

	// Header Chunk
	stream.read((char*)&id, 14); // Note the dependence on data member order

	// Swap from Big Endian to Little Endian
	header_size = REu32(header_size);
	format_type = REu16(format_type);
	num_tracks = REu16(num_tracks);
	time_division = REu16(time_division);

	// Check if data is valid
	if ( id != MIDI_HEADER_ID )	throw std::exception("Invalid header ID");
	if ( header_size != 6 )		throw std::exception("Invalid header size");
	if ( !num_tracks)			throw std::exception("Invalid number of tracks");
	if ( !time_division )		throw std::exception("Invalid time division");
	std::cout << "   NUM TRACKS = " << num_tracks << std::endl;
	std::cout << "  FORMAT TYPE = " << format_type << std::endl;
	std::cout << "TIME DIVISION = " << time_division << std::endl;

	// Track Chunks
	chunks = new MidiChunk[num_tracks];
	for (u16 i = 0; i < num_tracks; i++)
	{
		stream.read((char*)(chunks + i), 8);

		// Check if track chunk is valid
		if (chunks[i].size == 0) throw std::exception("Invalid chunk size");
		if (chunks[i].id != MIDI_CHUNK_HEADER_ID) throw std::exception("Invalid chunk ID");

		// Copy over data from file
		chunks[i].size = REu32(chunks[i].size);
		chunks[i].buffer = new u8[chunks[i].size];
		stream.read((char*)(chunks[i].buffer), chunks[i].size);
		std::cout << "TRACK " << i << " (" << chunks[i].size << " bytes)\n";
	}

	stream.close();
	return true;
}
catch (const std::exception& e)
{
	std::cout << "Error while reading MidiFile " << path << ":\n";
	std::cout << e.what() << std::endl;
	return false;
}

/**
 * @brief Loads the Sequence from a MidiFile
 * @param filepath = name of the file (not used) 
 * @return whether the operation was successful
*/
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

constexpr u32 extendBytes(u8*& data, const u8* data_end)
{
	// 0-6 = value, 7 = byte extension
	u32 value = (*data) & 0x7F;

	while ( (*(data++) & 0x80) && (data < data_end) )
	{
		value = (value << 7) + (*data & 0x7F);
	}

	return value;
}

/**
 * @brief From a given location, interprets data as a MidiEvent and adds it to an event list
 * @param data - current location of the buffer cursor
 * @param data_end - location of the buffer's end, used to determine an overflow
 * @param track - list of Midievents to add MidiEvent to
 * @param tick - current offset of the event tick
 * @return whether the data could be interpreted
*/
bool MidiSequence::addNextEvent(u8*& data, const u8* data_end, MidiTrack& track, u64& tick) try
{
	u32 delta_time = extendBytes(data, data_end);
	if (data >= data_end) throw std::exception("Exceeded buffer size when interpreting MidiEvent tick length");

	// Set the tick location forward based on change in time
	tick += delta_time;

	MidiEvent ev(data, tick, 0, last_status);

	// Check if status byte exists
	if (data[0] & 0x80)
	{
		// Set message to point to buffer
		ev.message++;
		ev.status = data[0];
		data++;
	}
	else if (last_status == 0) throw std::exception("Missing status byte at start of file");

	last_status = ev.status;

	// Determine size based on the status id
	if (ev.status == 0xFF)
	{
		ev.size = ev.message[1] + 2; // TODO: Dynamic event size
	}
	else if ((ev.status & 0xF0) == 0xF0)
	{
		for (ev.size = 0; ev.message[ev.size] != 0xF7; ev.size++); // Count until EOX
		ev.size++;
	}
	else
	{
		ev.size = (ev.status & 0xE0) == 0xC0 ? 1 : 2;
	}

	// Check data bytes for invalid values
	if ((ev.status & 0xF0) != 0xF0)
	{
		int i = 1;
		for (u32 i = 0; i < ev.size; i++)
		{
			if (ev.message[i] & 0x80)
				throw std::exception("Invalid data byte assigned to midi message parameter");
		}
	}

	// Move buffer cursor and add event to list
	data += ev.size;
	if (data > data_end) throw std::exception("Exceeded buffer size when interpreting MidiEvent parameters");
	track.push_back(ev);

#ifdef MIDI_READOUT
	ev.print(track.size());
#endif
	return true;
}
catch (const std::exception& e) 
{
	std::cout << e.what();
	return false;
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

	// Continues to add MidiEvents until the end of the buffer is reached
	while (buffer_cursor < cursor_end)
	{
		if ( !addNextEvent(buffer_cursor, cursor_end, track, tick) ) return false;
	}
	return true;
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
