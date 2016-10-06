#include "MidiTrack.h"

MidiTrack::MidiTrack()
{
	_title = 0;
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
