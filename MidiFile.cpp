#include "MidiFile.h"
#include <cstdio>
//#include <stdio.h>
#include <string.h>

unsigned long Read4Bytes(unsigned char* data)
{
	return (data[0] << 24) + (data[1] << 16) + (data[2] << 8) + data[3];
}

short Read2Bytes(unsigned char* data)
{
	return (data[0] << 8) + data[1];
}

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
		if( currentTrack == 0 && ptr != 22 )
		{
			printf("Math error.");
		}
		currentTrack += 1;
		ptr += trackSize;
	}

	_loaded = true;

	return true;
}
