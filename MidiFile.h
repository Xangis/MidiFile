#ifndef _MIDIFILE_H_
#define _MIDIFILE_H_

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
private:
    short _format;
	bool _loaded;
    unsigned char* _midiData;
    unsigned long _size;
    short _numTracks;
    unsigned short _timeDivision;
};

#endif
