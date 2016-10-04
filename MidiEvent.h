#ifndef _MIDIEVENT_H_
#define _MIDIEVENT_H_

struct MIDIEvent
{
	unsigned long absoluteTime; // All time deltas before this one, added up, plus the delta for this event.
	unsigned long timeDelta;
	unsigned short message;
	unsigned short channel;
	unsigned short value1;
	unsigned short value2;
	unsigned long lval;
};

#endif