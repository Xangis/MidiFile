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
private:
    int _format;
};

#endif
