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

bool MidiFile::ParseMetaEvent(int track, unsigned long deltaTime, unsigned char* inPos)
{
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
		AddEvent(track, 0, deltaTime, 0xFF, meta, 0, 0);
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
			int calculatedTempo = 60000000 / tempo;
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
	return true;
}

bool MidiFile::ParseSysCommon(int track, unsigned long deltaTime, unsigned char* inPos, unsigned short message)
{
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
	return true;
}

bool MidiFile::ParseChannelMessage(int track, unsigned long deltaTime, unsigned char* inPos, unsigned short message)
{
	unsigned short value1;
	unsigned short value2;

	switch( message & 0xF0 )
	{
	case 0x80: // Note off.
	case 0x90: // Note on.
	case 0xA0: // Aftertouch.
	case 0xB0: // Control change.
		value1 = *inPos++;
		value2 = *inPos++;
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
	return true;
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
	FILE* file = fopen(filename, "r");

	fseek(file, 0, SEEK_END);
	_size = ftell(file);
	unsigned int numRead = 0;
	fseek(file, 0, 0);
	if( _midiData != NULL )
	{
		delete[] _midiData;
	}
	_midiData = new unsigned char[_size];
	numRead = fread(_midiData, _size, 1, file);

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
			printf("MIDI File lacks correct track data.\n");
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

	while( ptr < endPtr )
	{
		unsigned long deltaTime = 0;
		ptr += ReadVariableLength(ptr, &deltaTime);
		msg = *ptr;
		if( (msg & 0xF0) == 0xF0 )
		{
			ptr++;
			if( msg == 0xFF )
			{
				ParseMetaEvent(track, deltaTime, ptr);
			}
			else
			{
				ParseSysCommon(track, deltaTime, ptr, msg);
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
			ParseChannelMessage(track, deltaTime, ptr, msg);
		}
	}
	int numEvents = _midiTracks[track]->GetNumEvents();
	printf("Loaded track %d with %d events.\n", track, numEvents);
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
	printf("MIDI file length: %d ticks, time division %d\n", highesttick, _timeDivision);
	if( _timeDivision > 0 )
	{
		double songLength = pulseLength * highesttick;
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
	//if( _timeSignatureThirtysecondNotesPerMidiQuarter != 8 )
	//{
	//	return (_timeSignatureThirtysecondNotesPerMidiQuarter * _tempo) / 8;
	//	// Divided by 4 seems more accurate. Not sure why.
	//	return (_timeSignatureThirtysecondNotesPerMidiQuarter * _tempo) / 4;
	//}
	if( _tempo != 0 )
	{
		return _tempo;
	}
	return 120.0;
}

double MidiFile::GetPulseLength()
{
	double pulseLength = 60.0 / ( (double)GetPPQN() * GetBPM());
	return pulseLength;
}
