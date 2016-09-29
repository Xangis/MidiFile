#include "MidiFile.h"
#include <cstdio>
//#include <stdio.h>
#include <string.h>

int Read4Bytes(unsigned char* data)
{
	return (data[0] << 24) + (data[1] << 16) + (data[2] << 8) + data[4];
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
	unsigned int size = ftell(file);
	unsigned int numRead = 0;
	fseek(file, 0, 0);
	if( _midiData == NULL )
	{
		_midiData = new unsigned char[size];
		numRead = fread(_midiData, size, 1, file);
	}

	fclose(file);

	if( size > 13 )
	{
		if( memcmp(_midiData, "MThd", 4) != 0 )
		{
			printf("MIDI File is invalid, lacks MThd header.");
		}
		_size = Read4Bytes(&(_midiData[4]));
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

	if( ptr < size && currentTrack < _numTracks )
	{
		char chars[4];
		chars[0] = _midiData[14];
		chars[1] = _midiData[15];
		chars[2] = _midiData[16];
		chars[3] = _midiData[17];
		//memset(chars, 0, 4);
		//memcpy(&(_midiData[14]), chars, 4);
		if( memcmp(chars, "MTrk", 4) != 0 )
		{
			printf("MIDI File lacks correct track data.");
		}
		int trackSize = Read4Bytes(&(_midiData[18]));
		currentTrack += 1;
	}

	_loaded = true;

	return true;
}
