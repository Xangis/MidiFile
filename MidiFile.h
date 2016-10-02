#ifndef _MIDIFILE_H_
#define _MIDIFILE_H_

#include "MidiEvent.h"
#include <list>
#include <vector>

enum MIDI_FILE_FORMAT {
TYPE0,
TYPE1,
TYPE2
};

class MidiFile
{
public:
    MidiFile();
	bool Load(const char* filename);
	bool ReadTrack(int track, unsigned int dataPtr, unsigned int length);
	int GetNumEvents();
	int GetNumTracks();
	int GetLength();
	int GetSize();
	int GetType();
private:
	bool ParseMetaEvent(int track, unsigned long deltaTime, unsigned char* inPos);
	bool ParseSysCommon(int track, unsigned long deltaTime, unsigned char* inPos, unsigned short message);
	bool ParseChannelMessage(int track, unsigned long deltaTime, unsigned char* inPos, unsigned short message);
	void AddEvent(unsigned short track, unsigned short channel, unsigned long timedelta, unsigned short message, unsigned short value1, unsigned short value2, unsigned long lval);
    short _format;
	bool _loaded;
    unsigned char* _midiData;
    unsigned long _size;
    short _numTracks;
    unsigned short _timeDivision;
	std::vector<std::list<MIDIEvent*>*> _midiTracks;
};

#endif
