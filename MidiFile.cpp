#include "MidiFile.h"
#include <cstdio>
#include <string.h>

// Utility functions.
unsigned long Read4Bytes(unsigned char* data)
{
	return (data[0] << 24) + (data[1] << 16) + (data[2] << 8) + data[3];
}

short Read2Bytes(unsigned char* data)
{
	return (data[0] << 8) + data[1];
}

void Write4Bytes(FILE* fp, unsigned char* data)
{
	fprintf(fp, "%c%c%c%c", data[0], data[1], data[2], data[3]);
}

void Write4Bytes(FILE* fp, unsigned long value)
{
	unsigned char* data = (unsigned char*)&value;
	fprintf(fp, "%c%c%c%c", data[3], data[2], data[1], data[0]);
}

void Write2Bytes(FILE* fp, unsigned char* data)
{
	fprintf(fp, "%c%c", data[0], data[1]);
}

void Write2Bytes(FILE* fp, unsigned short value)
{
	unsigned char* data = (unsigned char*)&value;
	fprintf(fp, "%c%c", data[1], data[0]);
}

int WriteVariableLength(register unsigned long value, unsigned char* output)
{
   int charsWritten = 0;
   register unsigned long buffer;
   buffer = value & 0x7F;

   while ( (value >>= 7) )
   {
     buffer <<= 8;
     buffer |= ((value & 0x7F) | 0x80);
   }

   while (true)
   {
      output[charsWritten] = buffer;
      ++charsWritten;
      if (buffer & 0x80)
      {
          buffer >>= 8;
      }
      else
      {
          break;
      }
   }
   return charsWritten;
}


// Takes a buffer and an unsigned long value. Reads a variable-length value into the value
// field and returns the number of bytes read.
unsigned long ReadVariableLength(unsigned char* inPos, unsigned long* value)
{
	unsigned char* pos = inPos;
	*value = 0;
	do
	{
		*value = (*value << 7) + (*inPos & 0x7F);
	}
	while( *inPos++ & 0x80 );
	return inPos - pos;
}

int MidiFile::ParseMetaEvent(int track, unsigned long deltaTime, unsigned char* inPos, bool * trackDone)
{
    unsigned char* initialPosition = inPos;
	unsigned long tempo = 0;
	short meta = *inPos++;
	unsigned long metalen = 0;
	int sizeRead = ReadVariableLength(inPos, &metalen);

	switch(meta)
	{
	case 0x01:
		{
			unsigned char* text = new unsigned char[metalen+1];
			text[metalen] = 0;
			memcpy(text, inPos+1, metalen);
			printf("Text event: %s\n", text);
			inPos += metalen;
            delete[] text;
			break;
		}
	case 0x03:
		{
			unsigned char* title = new unsigned char[metalen+1];
			title[metalen] = 0;
			memcpy(title, inPos+1, metalen);
			printf("Track name: %s\n", title);
			_midiTracks[track]->SetTitle(title);
			inPos += metalen;
		}
		break;
	case 0x20:
		{
			unsigned char midiPort = *inPos++;
			printf("MIDI Channel Prefix: %d.\n", midiPort);
			break;
		}
	case 0x21:
		{
			unsigned char midiPort = *inPos++;
			printf("MIDI Port %d.\n", midiPort);
			break;
		}
	case 0x2F:
		*trackDone = true;
		//AddEvent(track, 0, deltaTime, 0xFF, meta, 0, 0);
		break;
	case 0x51:
		{
			unsigned char byte1 = *inPos++;
			unsigned char byte2 = *inPos++;
			unsigned char byte3 = *inPos++;
			unsigned char byte4 = *inPos++;
			tempo = byte2;
			tempo = (tempo << 8) + byte3;
			tempo = (tempo << 8) + byte4;
			double calculatedTempo = 60000000.0 / tempo;
			printf("Tempo: %d value, BPM = %d, size read = %d, metalen = %d, byte1 = %d, byte2 = %d, byte3 = %d, byte4 = %d\n", tempo, calculatedTempo, sizeRead, metalen, byte1, byte2, byte3, byte4);
			// Use the first occurrence of a tempo as the overall song tempo.
			if( _tempo == 0 )
			{
				_tempo = calculatedTempo;
			}
			AddEvent(track, 0, deltaTime, 0xFF, meta, 0, tempo);
		}
		break;
	case 0x54:
		inPos += metalen;
		printf("SMPTE offset message, length %d\n", metalen);
		break;
	case 0x58:
		{
			_timeSignatureNumerator = *inPos++; // Beats per measure.
			_timeSignatureDenominator = *inPos++; // Beat division.
			_timeSignatureTicksPerClick = *inPos++; // Clock ticks per quarter note (MIDI ticks per metronome click).
			_timeSignatureThirtysecondNotesPerMidiQuarter = *inPos++; // Should be 8 for normal tempo. 16 for double speed, 4 for half.
			printf("Time signature %d / %d with %d ticks per click and %d thirtyseconds per quarter note.\n", 
				_timeSignatureNumerator, _timeSignatureDenominator, _timeSignatureTicksPerClick, _timeSignatureThirtysecondNotesPerMidiQuarter);
			break;
		}
	case 0x59:
		{
			char sharpFlat = *inPos++;
			unsigned char majorMinor = *inPos++;
			printf("Key signature: %d sharp/flat, %d major/minor (0=major, 1=minor).\n", sharpFlat, majorMinor);
		}
		break;
	default:
		inPos += metalen;
		printf("Unrecognized meta event %d with length %d\n", meta, metalen);
		break;
	}
	return inPos - initialPosition;
}

int MidiFile::ParseSysCommon(int track, unsigned long deltaTime, unsigned char* inPos, unsigned short message)
{
    unsigned char* initialPosition = inPos;
	switch(message)
	{
	case 0xF0:
		{
			unsigned long datalen;
			int bytesRead = ReadVariableLength(inPos, &datalen);
			printf("SysEx Message, length %d\n", datalen);
			inPos += datalen;
		}
		break;
	case 0xF1:
		// MIDI Time code.
		printf("MIDI Time Code message.\n");
		inPos += 1;
		break;
	case 0xF3:
		// Song select.
		printf("Song Select SysCommon message.\n");
		inPos += 1;
		break;
	case 0xF2:
        {
            unsigned char byte1 = *inPos++;
            unsigned char byte2 = *inPos++;
		    printf("Song Position SysCommon message, byte1 = %d, byte2 = %d.\n", byte1, byte2);
		    break;
        }
	default:
		printf("Unrecognized SysCommon message %d\n", message);
		break;
	}
	return inPos - initialPosition;
}

int MidiFile::ParseChannelMessage(int track, unsigned long deltaTime, unsigned char* inPos, unsigned short message)
{
    unsigned char* initialPosition = inPos;

    unsigned short value1;
	unsigned short value2;

	switch( message & 0xF0 )
	{
	case 0x80: // Note off.
	case 0x90: // Note on.
	case 0xA0: // Aftertouch.
	case 0xB0: // Control change.
		value1 = *inPos++; // Key or controller
		value2 = *inPos++; // Velocity, pressure, or value.
		AddEvent(track, message & 0x0F, deltaTime, message, value1, value2, 0);
		break;
	case 0xC0: // Program change.
	case 0xD0: // Channel aftertouch pressure
		value1 = *inPos++;
		AddEvent(track, message & 0x0F, deltaTime, message, value1, 0, 0);
		break;
	case 0xE0:
		// Pitch bend
		value1 = *inPos++;
		value1 = (value1 << 7) + *inPos++;
		AddEvent(track, message & 0x0F, deltaTime, message, value1, 0, 0);
		break;
	}
	return inPos - initialPosition;
}

void MidiFile::AddEvent(unsigned short track, unsigned short channel, unsigned long timedelta, unsigned short message, unsigned short value1, unsigned short value2, unsigned long lval)
{
	if( timedelta > 2000000 )
	{
		printf("Bad time delta for AddEvent: %d\n", timedelta);
	}
	MIDIEvent* midiEvent = new MIDIEvent();
	midiEvent->timeDelta = timedelta;
	if( _midiTracks[track]->GetNumEvents() > 0 )
	{
		MIDIEvent* lastEvent = _midiTracks[track]->GetLastEvent();
		if( lastEvent != NULL )
		{
			midiEvent->absoluteTime = lastEvent->absoluteTime + midiEvent->timeDelta;
		}
	}
	else
	{
		midiEvent->absoluteTime = midiEvent->timeDelta;
	}
	midiEvent->message = message;
	midiEvent->channel = channel;
	midiEvent->value1 = value1;
	midiEvent->value2 = value2;
	midiEvent->lval = lval;
	_midiTracks[track]->AddEvent(midiEvent);
}

// Class methods.
MidiFile::MidiFile()
{
	_tempo = 0;
    _format = TYPE0;
	_midiData = NULL;
	_trackName = NULL;
	_timeSignatureNumerator = -1;
	_timeSignatureDenominator = -1;
	_timeSignatureTicksPerClick = -1;
	_timeSignatureThirtysecondNotesPerMidiQuarter = -1;
}

MidiFile::~MidiFile()
{
	if( _midiData != NULL )
	{
		delete[] _midiData;
	}
	if( _trackName != NULL )
	{
		delete[] _trackName;
	}
	for( int i = 0; i < _midiTracks.size(); i++ )
	{
		delete _midiTracks[i];
	}
}

bool MidiFile::Load(const char* filename)
{
	FILE* file = fopen(filename, "rb");

	fseek(file, 0, SEEK_END);
	_size = ftell(file);
    printf( "File size is %d.", _size);
	fseek(file, 0, 0);
	if( _midiData != NULL )
	{
		delete[] _midiData;
	}
    // Allocate an extra byte and ensure the data is always null-terminated and initialized.
	_midiData = new unsigned char[_size+1];
    memset( _midiData, 0, _size+1);
	unsigned int numRead = 0;
	numRead = fread(_midiData, 1, _size, file);

	fclose(file);

	if( _size > 13 )
	{
		if( memcmp(_midiData, "MThd", 4) != 0 )
		{
			printf("MIDI File is invalid, lacks MThd header.\n");
		}
		int chunkSize = Read4Bytes(&(_midiData[4]));
		if( chunkSize != 6 )
		{
			printf("MThd header size is incorrect.\n");
		}
		_format = Read2Bytes(&(_midiData[8]));
		_numTracks = Read2Bytes(&(_midiData[10]));
		_timeDivision = Read2Bytes(&(_midiData[12]));
		if( _timeDivision < 0 )
		{
			printf("Time division is SMPTE, not BPM\n");
		}
	}
	else
	{
		printf("MIDI File lacks header. Is invalid.\n");
		_loaded = false;
		return false;
	}

	int ptr = 14;
	int currentTrack = 0;

    // Used for recovery when track size is incorrect.
    int lastTrackPtr = 0;

	while( ptr < _size && currentTrack < _numTracks )
	{
		char chars[4];
		chars[0] = _midiData[ptr];
		chars[1] = _midiData[ptr+1];
		chars[2] = _midiData[ptr+2];
		chars[3] = _midiData[ptr+3];
		ptr += 4;
		if( memcmp(chars, "MTrk", 4) != 0 )
		{
			printf("MIDI File lacks correct track data at position %d.\n", ptr-4);
            // This should always be true unless we have a pointer error.
            if( _midiData[_size] == 0 )
            {
                printf("Scanning for next track marker.\n");
                // Start searching right after the previous valid track pointer.
                int position = lastTrackPtr + 4;
                bool recovered = false;
                while( position < (_size - 4) )
                {
                    if( !memcmp(&(_midiData[position]), "MTrk", 4))
                    {
                        ptr = position;
                        recovered = true;
                        printf("MTrk header found at position %d\n", position);
                        break;
                    }
                    position++;
                }
                // If we recovered, try again.
                if( recovered )
                {
                    continue;
                }
            }
			break;
		}
		unsigned long trackSize = Read4Bytes(&(_midiData[ptr]));
		ptr += 4;
		_midiTracks.push_back(new MidiTrack());
		if( currentTrack == 0 && ptr != 22 )
		{
			printf("Math error.\n");
		}
		ReadTrack(currentTrack, ptr, trackSize);
		currentTrack += 1;
        // Store the last track pointer position, accounting for the length of MTrk and track size.
        lastTrackPtr = ptr - 8;
		ptr += trackSize;
	}

	_loaded = true;

	return true;
}

// Reads a single track from a MIDI file. Returns true on success, false on fail.
bool MidiFile::ReadTrack(int track, unsigned int dataPtr, unsigned int length)
{
	if( _midiData == NULL )
	{
		return false;
	}
	unsigned char* ptr = &(_midiData[dataPtr]);
	unsigned char* endPtr = ptr + length;
	unsigned char lastMsg = 0;
	short msg;
	// Track the note frequency on a track so we can assign a channel if it makes sense to do so.
	int channelFrequency[16];
	for(int i = 0; i < 16; i++)
	{
		channelFrequency[i] = 0;
	}

	bool trackDone = false;
	while( ptr < endPtr && !trackDone )
	{
		unsigned long deltaTime = 0;
		ptr += ReadVariableLength(ptr, &deltaTime);
		msg = *ptr;
		if( (msg & 0xF0) == 0xF0 )
		{
			ptr++;
			if( msg == 0xFF )
			{
				ptr += ParseMetaEvent(track, deltaTime, ptr, &trackDone);
			}
			else
			{
				ptr += ParseSysCommon(track, deltaTime, ptr, msg);
			}
		}
		else
		{
			if( msg & 0x80 )
			{
				ptr++;
				lastMsg = msg;
			}
			else
			{
				msg = lastMsg;
			}
			ptr += ParseChannelMessage(track, deltaTime, ptr, msg);
			// Increment channel message count.
			int channel = msg & 0x0F;
			channelFrequency[channel] += 1;
		}
	}
	int numEvents = _midiTracks[track]->GetNumEvents();
	printf("Loaded track %d with %d events.\n", track, numEvents);
	// TODO: Use channel message count to assign a channel. There's probably a cleaner way to do this.
	int channelsFound = 0;
	for( int i = 0; i < 16; i++ )
	{
		if( channelFrequency[i] > 0 )
		{
			channelsFound += 1;
		}
	}
	if( channelsFound == 1 )
	{
		for( int i = 0; i < 16; i++ )
		{
			if( channelFrequency[i] > 0 )
			{
				_midiTracks[track]->_assignedChannel = i;
			}
		}
	}
	return true;
}

int MidiFile::GetNumEvents()
{
	int events = 0;
	for( int i = 0; i < _midiTracks.size(); i++ )
	{
		events += _midiTracks[i]->GetNumEvents();
	}
	return events;
}

int MidiFile::GetNumTracks()
{
	return _midiTracks.size();
}

int MidiFile::GetLength()
{
	int highesttick = 0;
	int currenttick = 0;
	double pulseLength = GetPulseLength();
	int ticks = GetLengthInTicks();

	printf("MIDI file length: %d ticks, time division %d\n", ticks, _timeDivision);
	if( _timeDivision > 0 )
	{
		double songLength = pulseLength * ticks;
		return (int)songLength;
	}
	return -1;
}

int MidiFile::GetSize()
{
	return _size;
}

int MidiFile::GetType()
{
	return _format;
}

MidiTrack* MidiFile::GetTrackData(int track)
{
	if( track < _midiTracks.size() )
	{
		return _midiTracks[track];
	}
}

int MidiFile::GetPPQN()
{
	return _timeDivision;
}

double MidiFile::GetBPM()
{
	if( _tempo != 0 )
	{
		return _tempo;
	}
	return 120.0;
}

/**
* This is intended to be used to change the tempo in realtime. As tempo messages come by,
* this value should be changed.
*
* This will cause timing to be recalculated as the song plays.
*/
void MidiFile::SetBPM(double bpm)
{
    _tempo = bpm;
}

/**
* Returns the pulse length in seconds.
*/
double MidiFile::GetPulseLength()
{
	double quartersPerSecond = GetBPM() / 60.0;
	double ticksPerSecond = (double)GetPPQN() * quartersPerSecond;
	double secondsPerTick = 1.0 / ticksPerSecond;
	//double pulseLength = 60.0 / ( (double)GetPPQN() * GetBPM());
	return secondsPerTick;
}


int MidiFile::GetLengthInTicks()
{
	int highesttick = 0;
	int currenttick = 0;
	double pulseLength = GetPulseLength();

	for( int i = 0; i < _midiTracks.size(); i++ )
	{
		currenttick = 0;
		MIDIEvent* lastEvent = _midiTracks[i]->GetLastEvent();

		if( lastEvent != NULL )
		{
			currenttick = lastEvent->absoluteTime;
		}

		if( currenttick > highesttick )
		{
			highesttick = currenttick;
		}	

		if( currenttick != 0 )
		{
			int tracklength = pulseLength * currenttick;
			printf( "Track length using tempo method is %d seconds\n", tracklength );
		}
	}
	return highesttick;
}


bool MidiFile::Save(const char* filename)
{
	FILE* file = fopen(filename, "w");

	fprintf(file, "MThd");

	Write4Bytes(file, 6);
	Write2Bytes(file, _format);
	Write2Bytes(file, _numTracks);
	Write2Bytes(file, GetPPQN());

	for( int i = 0; i < _numTracks; i++ )
	{
		// TODO: String all the events together to create the track data buffer
		// We need to know the number of bytes beforehand.
		unsigned char* data = new unsigned char[32768]; // Max track size -- this needs to be unlimited.
		memset( data, 0, 32768 );
		int numBytesWritten = WriteTrackDataToBuffer(data, 32768, i);
		fprintf(file, "MTrk");
		Write4Bytes(file, numBytesWritten);
        fwrite(data, numBytesWritten, 1, file);
		delete data;
	}

	fclose(file);
	return true;
}

int MidiFile::WriteTrackDataToBuffer(unsigned char* data, int size, int track)
{
    int dataPtr = 0;
    int numEvents = 0;
	std::list<MIDIEvent*>* events = _midiTracks[track]->GetEvents();
    for( std::list<MIDIEvent*>::iterator it = events->begin(); it != events->end(); it++ )
    {
        MIDIEvent* evt = *it;
        if( evt->message >= 144 && evt->message < 176 )
        {
            dataPtr += WriteVariableLength(evt->timeDelta, data+dataPtr);
            data[dataPtr] = (unsigned char)evt->message;
            data[dataPtr+1] = (unsigned char)evt->value1;
            data[dataPtr+2] = (unsigned char)evt->value2;
            dataPtr += 3;
            numEvents++;
        }
        else
        {
            printf( "Unrecognized message %d not written.", evt->message );
        }
    }
    // Write "end of track" marker.
	data[dataPtr] = 0xFF;
	data[dataPtr+1] = 0x2F;
	data[dataPtr+2] = 0;
    dataPtr += 3;
    return dataPtr;
}