#ifndef _MIDIFILE_H_
#define _MIDIFILE_H_

#include "MidiEvent.h"
#include "MidiTrack.h"
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
	~MidiFile();
	bool Load(const char* filename);
	bool Save(const char* filename);
	bool ReadTrack(int track, unsigned int dataPtr, unsigned int length);
	int GetNumEvents();
	int GetNumTracks();
	int GetLength();
	int GetLengthInTicks();
	int GetSize();
	int GetType();
	int GetPPQN();
	double GetBPM();
	double GetPulseLength(); // Gets pulse length in seconds.
	MidiTrack* GetTrackData(unsigned int track);
	int WriteTrackDataToBuffer(unsigned char* data, int size, int track);
    void SetBPM(double bpm);
private:
	int ParseMetaEvent(int track, unsigned long deltaTime, unsigned char* inPos, bool * trackDone);
	int ParseSysCommon(int track, unsigned long deltaTime, unsigned char* inPos, unsigned short message);
	int ParseChannelMessage(int track, unsigned long deltaTime, unsigned char* inPos, unsigned short message);
	void AddEvent(unsigned short track, unsigned short channel, unsigned long timedelta, unsigned short message, unsigned short value1, unsigned short value2, unsigned long lval);
    short _format;
	bool _loaded;
    unsigned char* _midiData;
	unsigned char* _trackName;
    unsigned long _size;
    unsigned short _numTracks;
	double _tempo;
    short _timeDivision;
	unsigned char _timeSignatureNumerator;
	unsigned char _timeSignatureDenominator;
	unsigned char _timeSignatureTicksPerClick;
	unsigned char _timeSignatureThirtysecondNotesPerMidiQuarter;
	std::vector<MidiTrack*> _midiTracks;
};

#endif
