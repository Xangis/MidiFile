#include "MidiTrack.h"

MidiTrack::MidiTrack()
{
	_title = NULL;
}

MidiTrack::~MidiTrack()
{
	if( _title != NULL )
	{
		delete[] _title;
	}
	for( std::list<MIDIEvent*>::reverse_iterator it = _midiEvents.rbegin(); it != _midiEvents.rend(); it++ )
	{
		delete (*it);
	}
}