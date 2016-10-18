#include "MidiTrack.h"

MidiTrack::MidiTrack()
{
	_tickPosition = 0;
	_title = 0;
	_tickPointer = _midiEvents.begin();
}

MidiTrack::~MidiTrack()
{
	if( _title != 0 )
	{
		delete[] _title;
	}
	for( std::list<MIDIEvent*>::reverse_iterator it = _midiEvents.rbegin(); it != _midiEvents.rend(); it++ )
	{
		delete (*it);
	}
}

void MidiTrack::MoveToTick(unsigned int tick)
{
	if( tick = 0 )
	{
		_tickPointer = _midiEvents.begin();
	}
	else
	{
		if( _tickPosition < tick )
		{
			// Advance forward.
		}
		else
		{
			// Start at the beginning and advance forward.
			_tickPointer = _midiEvents.begin();
		}
		std::list<MIDIEvent*>::iterator* _lastMidiEvent = &_tickPointer;
		for( ; _tickPointer != _midiEvents.end(); _tickPointer++ )
		{
			if( (*_tickPointer)->absoluteTime > tick )
			{
				break;
			}
			_lastMidiEvent = &_tickPointer;
		}
		_tickPointer = *_lastMidiEvent;
	}
	_tickPosition = tick;
}

std::list<MIDIEvent*> MidiTrack::AdvanceToTick(unsigned int tick)
{
	if( tick < _tickPosition )
	{
		MoveToTick(tick);
		return std::list<MIDIEvent*>();
	}
	else
	{
		std::list<MIDIEvent*> events;
		std::list<MIDIEvent*>::iterator* _lastMidiEvent = &_tickPointer;
		for( ; _tickPointer != _midiEvents.end(); _tickPointer++ )
		{
			if( (*_tickPointer)->absoluteTime > tick )
			{
				break;
			}
			events.push_back(*_tickPointer);
			_lastMidiEvent = &_tickPointer;
		}
		_tickPosition = tick;
		return events;
	}
}

unsigned int MidiTrack::GetNumEvents()
{
	return _midiEvents.size();
}

const unsigned char* MidiTrack::GetTitle()
{
	return _title;
}

MIDIEvent* MidiTrack::GetLastEvent()
{
	if( GetNumEvents() < 1 )
	{
		return NULL;
	}
	std::list<MIDIEvent*>::iterator iter = _midiEvents.end();
	return *(--iter);
}

void MidiTrack::AddEvent(MIDIEvent* midiEvent)
{
	_midiEvents.push_back(midiEvent);
}

void MidiTrack::SetTitle(const unsigned char* title)
{
	_title = title;
}

// This should not exist, and should not be called because it makes private data public.
std::list<MIDIEvent*>* MidiTrack::GetEvents()
{
	return &_midiEvents;
}