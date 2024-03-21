#pragma once

#if defined _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <cstring>
#include <cstdio>
#include <cstdint>

#define SMF_RET_ERR_FILE_NOT_FOUND -1
#define SMF_RET_ERR_REACHED_EOF -2
#define SMF_RET_ERR_WROND_MAGIC -3
#define SMF_RET_ERR_WRONG_LENGTH -4
#define SMF_RET_ERR_WRONG_FORMAT -5
#define SMF_RET_ERR_WRONG_COUNT -6
#define SMF_RET_ERR_WRONG_RESOLUTION -7
#define SMF_RET_ERR_TRACK_NOT_FOUND -8
#define SMF_RET_OK 0

enum eMidiMessageType {
	MidiMessageType_NoteOff = 0x80,
	MidiMessageType_NoteOn = 0x90,
	MidiMessageType_PolyphonicKeyPressure = 0xA0,
	MidiMessageType_ControlChange = 0xB0,
	MidiMessageType_ProgramChange = 0xC0,
	MidiMessageType_ChannelPressure = 0xD0,
	MidiMessageType_PitchBend = 0xE0,
	MidiMessageType_BeginSystemExclusive = 0xF0,
	MidiMessageType_MidiTimeCode = 0xF1,
	MidiMessageType_SongPosition = 0xF2,
	MidiMessageType_SongSelect = 0xF3,
	MidiMessageType_EndSystemExclusive = 0xF7,
	MidiMessageType_ActiveSensing = 0xFE,
	MidiMessageType_MetaEvent = 0xFF,
};

class SMFPlayer
{
public:
	SMFPlayer(void (*cb)(uint8_t*, size_t)) {
		smfFile = NULL;
		midiMessageCallback = cb;
		delta = 0;
		memset(message, 0x00, sizeof(message));
		memset(meta, 0x00, sizeof(meta));
		resolution = 48;
		status = 0x00;
		tempo = 0;
		deltaBase = 0;
	}

	int Open(const char* fileName);
	int Play(uint32_t* nextDelta);
	int Close(void);
private:
	void (*midiMessageCallback)(uint8_t*, size_t);

	inline int get1(uint8_t*);
	inline int get2(uint16_t*);
	inline int get4(uint32_t*);

	inline int getVarLen(uint32_t*);
	inline int getMessage(uint8_t*, size_t*);

	uint32_t tempo;
	uint16_t resolution;
	uint32_t delta;
	uint32_t deltaBase;
	uint8_t meta[256];
	uint8_t message[256];
	uint8_t status;
#if defined _WIN32
	FILE* smfFile;
#endif
};

