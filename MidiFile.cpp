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
	unsigned long tempo;
	short meta = *inPos++;
	unsigned long metalen;
	int sizeRead = ReadVariableLength(inPos, &metalen);
	
	switch(meta)
	{
	case 0x2F:
		AddEvent(track, 0, deltaTime, 0xFF, meta, 0, 0);
		break;
	case 0x51:
		tempo = (*inPos++ << 16);
		tempo += (*inPos++ << 8);
		tempo += *inPos++;
		AddEvent(track, 0, deltaTime, 0xFF, meta, 0, tempo);
		break;
	default:
		inPos += metalen;
		printf("Unrecognized meta event %d", meta);
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
			printf("SysEx Message");
			unsigned long datalen;
			int bytesRead = ReadVariableLength(inPos, &datalen);
			inPos += datalen;
		}
		break;
	case 0xF1:
		// MIDI Time code.
		printf("MIDI Time Code");
		inPos += 1;
		break;
	case 0xF3:
		// Song select.
		printf("Song Select");
		inPos += 1;
		break;
	case 0xF2:
		printf("Song Position");
		inPos += 2;
		break;
	default:
		printf("Unrecognized message %d", message);
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
	case 0x80:
		// Note off.
		break;
	case 0x90:
		// Note on.
		break;
	case 0xA0:
		// Aftertouch
		break;
	case 0xB0:
		// Control change
		value1 = *inPos++;
		value2 = *inPos++;
		AddEvent(track, message & 0x0F, deltaTime, message, value1, 0, 0);
		break;
	case 0xC0:
		// Program change.
		break;
	case 0xD0:
		// Channel aftertouch pressure
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
	MIDIEvent* midiEvent = new MIDIEvent();
	midiEvent->timeDelta = timedelta;
	midiEvent->message = message;
	midiEvent->channel = channel;
	midiEvent->value1 = value1;
	midiEvent->value2 = value2;
	midiEvent->lval = lval;
	_midiTracks[track]->push_back(midiEvent);
}

// Class methods.
MidiFile::MidiFile()
{
    _format = TYPE0;
	_midiData = NULL;
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
			printf("MIDI File is invalid, lacks MThd header.");
		}
		int chunkSize = Read4Bytes(&(_midiData[4]));
		if( chunkSize != 6 )
		{
			printf("MThd header size is incorrect.");
		}
		_format = Read2Bytes(&(_midiData[8]));
		_numTracks = Read2Bytes(&(_midiData[10]));
		_timeDivision = Read2Bytes(&(_midiData[12]));
	}
	else
	{
		printf("MIDI File lacks header. Is invalid.");
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
		//memset(chars, 0, 4);
		//memcpy(&(_midiData[14]), chars, 4);
		if( memcmp(chars, "MTrk", 4) != 0 )
		{
			printf("MIDI File lacks correct track data.");
			break;
		}
		unsigned long trackSize = Read4Bytes(&(_midiData[ptr]));
		ptr += 4;
		_midiTracks.push_back(new std::list<MIDIEvent*>());
		if( currentTrack == 0 && ptr != 22 )
		{
			printf("Math error.");
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
		unsigned long deltaTime;
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
	int numEvents = _midiTracks[track]->size();
	printf("Loaded track %d with %d events.", track, numEvents);
	return true;
}

int MidiFile::GetNumEvents()
{
	int events = 0;
	for( int i = 0; i < _midiTracks.size(); i++ )
	{
		events += _midiTracks[i]->size();
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
	for( int i = 0; i < _midiTracks.size(); i++ )
	{
		currenttick = 0;
		std::list<MIDIEvent*>::iterator iter;
		for( iter = _midiTracks[i]->begin(); iter != _midiTracks[i]->end(); iter++ )
		{
			currenttick += (*iter)->timeDelta;
		}
		if( currenttick > highesttick )
		{
			highesttick = currenttick;
		}
	}
	printf("MIDI file length: %d ticks, time division %d", highesttick, _timeDivision);
	if( _timeDivision > 0 )
	{
		return highesttick / _timeDivision;
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