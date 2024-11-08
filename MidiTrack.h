#ifndef _MIDITRACK_H_
#define _MIDITRACK_H_

#include "MidiEvent.h"
#include <list>

class MidiTrack
{
public:
	MidiTrack();
	~MidiTrack();
	void MoveToTick(unsigned int tick);
	std::list<MIDIEvent*> AdvanceToTick(unsigned int tick);
	unsigned int GetNumEvents();
	const unsigned char* GetTitle();
	void SetTitle(const unsigned char* title);
	MIDIEvent* GetLastEvent();
	void AddEvent(MIDIEvent*);
	std::list<MIDIEvent*>* GetEvents();
	int _assignedChannel; // Zero-based index of assigned channel. 9 = Channel 10.
private:
	std::list<MIDIEvent*> _midiEvents;
	const unsigned char* _title;
	std::list<MIDIEvent*>::iterator _tickPointer;
	unsigned int _tickPosition;
};

#endif