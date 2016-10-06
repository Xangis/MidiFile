#ifndef _MIDITRACK_H_
#define _MIDITRACK_H_

#include "MidiEvent.h"
#include <list>

class MidiTrack
{
public:
	MidiTrack();
	~MidiTrack();
	unsigned char* _title;
	std::list<MIDIEvent*> _midiEvents;
};

#endif