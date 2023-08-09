// MIDI Format Sound Player

#ifndef MIDI_SOUND_PLAYER_H
#define MIDI_SOUND_PLAYER_H

#pragma once

#include <hl_sdk/common/usercmd.h>

#include <base_feature.h>
#include <stdio.h>

#include <vector>

//-----------------------------------------------------------------------------
// Structures
//-----------------------------------------------------------------------------

struct midi_note_t
{
	int tick;
	//unsigned char channel;
	unsigned char on;
	unsigned char note;
	unsigned char velocity;
};

//-----------------------------------------------------------------------------
// CMidiSoundPlayer
//-----------------------------------------------------------------------------

class CMidiSoundPlayer : public CBaseFeature
{
public:
	CMidiSoundPlayer();

	void Parse( const char *pszFilename );
	void CreateMove( float frametime, usercmd_t *cmd, int active );

private:
	bool m_bPlaying;

	int m_iTicks;
	int m_iNoteEndTicks;
	int m_iPulsesPerQuarter; // PPQ
	int m_iTempo;
	int m_iTicksPerSecond; // assumse it's game's FPS

	std::vector<midi_note_t> m_notes;
};

extern CMidiSoundPlayer g_MidiSoundPlayer;

#endif // MIDI_SOUND_PLAYER_H