#ifndef _MIDIEVENT_H_
#define _MIDIEVENT_H_

struct MIDIEvent
{
	unsigned long timeDelta;
	unsigned short message;
	unsigned short channel;
	unsigned short value1;
	unsigned short value2;
	unsigned long lval;
};

#endif