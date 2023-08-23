// MIDI Format Sound Player

#include <regex>

#include <dbg.h>
#include <convar.h>
#include <hl_sdk/engine/APIProxy.h>

#include "midi_sound_player.h"

//-----------------------------------------------------------------------------
// Vars
//-----------------------------------------------------------------------------

const float midi_notes_freq[ 128 ] =
{
	8.18f, // 0 : C-1
	8.66f, // 1 : C#
	9.18f, // 2 : D
	9.72f, // 3 : D#
	10.30f, // 4 : E
	10.91f, // 5 : F
	11.56f, // 6 : F#
	12.25f, // 7 : G
	12.98f, // 8 : G#
	13.75f, // 9 : A
	14.57f, // 10 : A#
	15.43f, // 11 : B
	16.35f, // 12 : C0
	17.32f, // 13 : C#
	18.35f, // 14 : D
	19.45f, // 15 : D#
	20.60f, // 16 : E
	21.83f, // 17 : F
	23.12f, // 18 : F#
	24.50f, // 19 : G
	25.96f, // 20 : G#
	27.50f, // 21 : A0
	29.14f, // 22 : A#0/Bb0
	30.87f, // 23 : B0
	32.70f, // 24 : C1
	34.65f, // 25 : C#1/Db1
	36.71f, // 26 : D1
	38.89f, // 27 : D#1/Eb1
	41.20f, // 28 : E1
	43.65f, // 29 : F1
	46.25f, // 30 : F#1/Gb1
	49.00f, // 31 : G1
	51.91f, // 32 : G#1/Ab1
	55.00f, // 33 : A1
	58.27f, // 34 : A#1/Bb1
	61.74f, // 35 : B1
	65.41f, // 36 : C2
	69.30f, // 37 : C#2/Db2
	73.42f, // 38 : D2
	77.78f, // 39 : D#2/Eb2
	82.41f, // 40 : E2
	87.31f, // 41 : F2
	92.50f, // 42 : F#2/Gb2
	98.00f, // 43 : G2
	103.83f, // 44 : G#2/Ab2
	110.00f, // 45 : A2
	116.54f, // 46 : A#2/Bb2
	123.47f, // 47 : B2
	130.81f, // 48 : C3
	138.59f, // 49 : C#3/Db3
	146.83f, // 50 : D3
	155.56f, // 51 : D#3/Eb3
	164.81f, // 52 : E3
	174.61f, // 53 : F3
	185.00f, // 54 : F#3/Gb3
	196.00f, // 55 : G3
	207.65f, // 56 : G#3/Ab3
	220.00f, // 57 : A3
	233.08f, // 58 : A#3/Bb3
	246.94f, // 59 : B3
	261.63f, // 60 : C4
	277.18f, // 61 : C#4/Db4
	293.66f, // 62 : D4
	311.13f, // 63 : D#4/Eb4
	329.63f, // 64 : E4
	349.23f, // 65 : F4
	369.99f, // 66 : F#4/Gb4
	392.00f, // 67 : G4
	415.30f, // 68 : G#4/Ab4
	440.00f, // 69 : A4
	466.16f, // 70 : A#4/Bb4
	493.88f, // 71 : B4
	523.25f, // 72 : C5
	554.37f, // 73 : C#5/Db5
	587.33f, // 74 : D5
	622.25f, // 75 : D#5/Eb5
	659.26f, // 76 : E5
	698.46f, // 77 : F5
	739.99f, // 78 : F#5/Gb5
	783.99f, // 79 : G5
	830.61f, // 80 : G#5/Ab5
	880.00f, // 81 : A5
	932.33f, // 82 : A#5/Bb5
	987.77f, // 83 : B5
	1046.50f, // 84 : C6
	1108.73f, // 85 : C#6/Db6
	1174.66f, // 86 : D6
	1244.51f, // 87 : D#6/Eb6
	1318.51f, // 88 : E6
	1396.91f, // 89 : F6
	1479.98f, // 90 : F#6/Gb6
	1567.98f, // 91 : G6
	1661.22f, // 92 : G#6/Ab6
	1760.00f, // 93 : A6
	1864.66f, // 94 : A#6/Bb6
	1975.53f, // 95 : B6
	2093.00f, // 96 : C7
	2217.46f, // 97 : C#7/Db7
	2349.32f, // 98 : D7
	2489.02f, // 99 : D#7/Eb7
	2637.02f, // 100 : E7
	2793.83f, // 101 : F7
	2959.96f, // 102 : F#7/Gb7
	3135.96f, // 103 : G7
	3322.44f, // 104 : G#7/Ab7
	3520.00f, // 105 : A7
	3729.31f, // 106 : A#7/Bb7
	3951.07f, // 107 : B7
	4186.01f, // 108 : C8
	4434.92f, // 109 : C#8/Db8
	4698.64f, // 110 : D8
	4978.03f, // 111 : D#8/Eb8
	5274.04f, // 112 : E8
	5587.65f, // 113 : F8
	5919.91f, // 114 : F#8/Gb8
	6271.93f, // 115 : G8
	6644.88f, // 116 : G#8/Ab8
	7040.00f, // 117 : A8
	7458.62f, // 118 : A#8/Bb8
	7902.13f, // 119 : B8
	8372.02f, // 120 : C9
	8869.84f, // 121 : C#9/Db9
	9397.27f, // 122 : D9
	9956.06f, // 123 : D#9/Eb9
	10548.08f, // 124 : E9
	11175.30f, // 125 : F9
	11839.82f, // 126 : F#9/Gb9
	12543.85f // 127 : G9
};

CMidiSoundPlayer g_MidiSoundPlayer;

//-----------------------------------------------------------------------------
// ConCommands / ConVars
//-----------------------------------------------------------------------------

CON_COMMAND( sc_msp_play, "Play a MIDI format file" )
{
	if ( args.ArgC() >= 2 )
	{
		g_MidiSoundPlayer.Parse( args[ 1 ] );
	}
}

//-----------------------------------------------------------------------------
// CMidiSoundPlayer
//-----------------------------------------------------------------------------

CMidiSoundPlayer::CMidiSoundPlayer()
{
	m_bPlaying = false;
	m_iTicks = 0;
	m_iNoteEndTicks = -1;
	m_iPulsesPerQuarter = 0;
	m_iTempo = 500000;
	m_iTicksPerSecond = 0;
}

const char *zalupa[] =
{
	"98.0,19,impulse 100",
	"146.83,28,impulse 100",
	"246.94,48,impulse 100",
	"220.0,42,impulse 100",
	"246.94,48,impulse 100",
	"146.83,28,impulse 100",
	"246.94,48,impulse 100",
	"146.83,28,impulse 100",
	"98.0,19,impulse 100",
	"146.83,28,impulse 100",
	"246.94,48,impulse 100",
	"220.0,42,impulse 100",
	"246.94,48,impulse 100",
	"146.83,28,impulse 100",
	"246.94,48,impulse 100",
	"146.83,28,impulse 100",
	"98.0,19,impulse 100",
	"164.81,32,impulse 100",
	"261.63,50,impulse 100",
	"246.94,48,impulse 100",
	"261.63,50,impulse 100",
	"164.81,32,impulse 100",
	"261.63,50,impulse 100",
	"164.81,32,impulse 100",
	"98.0,19,impulse 100",
	"164.81,32,impulse 100",
	"261.63,50,impulse 100",
	"246.94,48,impulse 100",
	"261.63,50,impulse 100",
	"164.81,32,impulse 100",
	"261.63,50,impulse 100",
	"164.81,32,impulse 100",
	"98.0,1,impulse 100",
	"98.0,14,impulse 100",
	"98.0,1,impulse 100",
	"98.0,3,impulse 100",
	"185.0,36,impulse 100",
	"261.63,50,impulse 100",
	"246.94,48,impulse 100",
	"261.63,50,impulse 100",
	"185.0,36,impulse 100",
	"261.63,50,impulse 100",
	"185.0,36,impulse 100",
	"98.0,19,impulse 100",
	"185.0,36,impulse 100",
	"261.63,50,impulse 100",
	"246.94,48,impulse 100",
	"261.63,50,impulse 100",
	"185.0,36,impulse 100",
	"261.63,50,impulse 100",
	"185.0,36,impulse 100",
	"98.0,19,impulse 100",
	"196.0,38,impulse 100",
	"246.94,48,impulse 100",
	"220.0,42,impulse 100",
	"246.94,48,impulse 100",
	"196.0,3,impulse 100",
	"196.0,35,impulse 100",
	"246.94,48,impulse 100",
	"196.0,38,impulse 100",
	"98.0,19,impulse 100",
	"196.0,38,impulse 100",
	"246.94,48,impulse 100",
	"220.0,42,impulse 100",
	"246.94,48,impulse 100",
	"196.0,38,impulse 100",
	"246.94,48,impulse 100",
	"185.0,36,impulse 100",
	"98.0,19,impulse 100",
	"164.81,1,impulse 100",
	"164.81,1,impulse 100",
	"164.81,30,impulse 100",
	"246.94,31,impulse 100"
};

//-----------------------------------------------------------------------------
// CMidiSoundPlayer::Parse
//-----------------------------------------------------------------------------

void CMidiSoundPlayer::Parse( const char *pszFilename )
{
	//int ticks = 0;

	//for ( int i = 0; i < M_ARRAYSIZE( zalupa ); i++ )
	//{
	//	int c = 0;

	//	char *str = strdup( zalupa[ i ] );
	//	char *pch;

	//	double freq = 0.0;
	//	int frames = 0;

	//	pch = strtok( str, "," );
	//	while ( pch != NULL )
	//	{
	//		c++;

	//		if ( c == 1 )
	//		{
	//			freq = atof( pch );
	//		}
	//		else if ( c == 2 )
	//		{
	//			frames = atoi( pch );
	//		}

	//		pch = strtok( NULL, "," );
	//	}

	//	free( str );

	//	//98.0 - G2 - 43
	//	//146.83 - D3 - 50
	//	//164.81 - E3 - 52
	//	//185.0 - F#3 / Gb3 - 54
	//	//196.0 - G3 - 55
	//	//220.0 - A3 - 57
	//	//246.94 - B3 - 59
	//	//261.63 - C4 - 60

	//	ticks += 96;

	//	if ( freq == 98.0 )
	//	{
	//		Msg( "2, %d, Note_on_c, 0, %d, %d\n", ticks - 96, 43, frames );
	//		Msg( "2, %d, Note_off_c, 0, %d, 0\n", ticks, 43 );
	//	}
	//	else if ( freq == 146.83 )
	//	{
	//		Msg( "2, %d, Note_on_c, 0, %d, %d\n", ticks - 96, 50, frames );
	//		Msg( "2, %d, Note_off_c, 0, %d, 0\n", ticks, 50 );
	//	}
	//	else if ( freq == 164.81 )
	//	{
	//		Msg( "2, %d, Note_on_c, 0, %d, %d\n", ticks - 96, 52, frames );
	//		Msg( "2, %d, Note_off_c, 0, %d, 0\n", ticks, 52 );
	//	}
	//	else if ( freq == 185.0 )
	//	{
	//		Msg( "2, %d, Note_on_c, 0, %d, %d\n", ticks - 96, 54, frames );
	//		Msg( "2, %d, Note_off_c, 0, %d, 0\n", ticks, 54 );
	//	}
	//	else if ( freq == 196.0 )
	//	{
	//		Msg( "2, %d, Note_on_c, 0, %d, %d\n", ticks - 96, 55, frames );
	//		Msg( "2, %d, Note_off_c, 0, %d, 0\n", ticks, 55 );
	//	}
	//	else if ( freq == 220.0 )
	//	{
	//		Msg( "2, %d, Note_on_c, 0, %d, %d\n", ticks - 96, 57, frames );
	//		Msg( "2, %d, Note_off_c, 0, %d, 0\n", ticks, 57 );
	//	}
	//	else if ( freq == 246.94 )
	//	{
	//		Msg( "2, %d, Note_on_c, 0, %d, %d\n", ticks - 96, 59, frames );
	//		Msg( "2, %d, Note_off_c, 0, %d, 0\n", ticks, 59 );
	//	}
	//	else if ( freq == 261.63 )
	//	{
	//		Msg( "2, %d, Note_on_c, 0, %d, %d\n", ticks - 96, 60, frames );
	//		Msg( "2, %d, Note_off_c, 0, %d, 0\n", ticks, 60 );
	//	}
	//	else
	//	{
	//		Warning( "Unrecognized freq: %f\n", freq );
	//	}
	//}

	//return;

	static char szBuffer[ 512 ];

	snprintf( szBuffer, M_ARRAYSIZE( szBuffer ), "sven_internal/midi_csv/%s.csv", pszFilename );

	FILE *file = fopen( szBuffer, "r" );

	if ( file != NULL )
	{
		int state = 0;
		int last_ticks = 0;
		bool found_header = false;
		bool found_tempo = false;
		bool found_program_start = false;
		bool found_notes = false;

		// Init
		m_bPlaying = false;
		m_iTicks = 0;
		m_iNoteEndTicks = -1;
		m_iPulsesPerQuarter = 0;
		m_iTempo = 500000;
		m_iTicksPerSecond = 0;
		m_notes.clear();

		// Regex patterns
		static std::regex regex_header( "0, 0, Header, [0-2]+, [0-9]+, ([0-9]+)" );
		static std::regex regex_tempo( "[0-9]+, [0-9]+, Tempo, ([0-9]+)" );
		static std::regex regex_program( "[0-9]+, [0-9]+, Program_c, 0, 0" );
		static std::regex regex_note_on( "[0-9]+, ([0-9]+), Note_on_c, [0-9]+, ([0-9]+), ([0-9]+)" );
		static std::regex regex_note_off( "[0-9]+, ([0-9]+), Note_off_c, [0-9]+, ([0-9]+), ([0-9]+)" );

		// Parse file
		while ( fgets( szBuffer, sizeof( szBuffer ), file ) )
		{
			std::cmatch match;

			if ( state == 0 ) // Looking for the header
			{
				if ( std::regex_search( szBuffer, match, regex_header ) )
				{
					m_iPulsesPerQuarter = atoi( match[ 1 ].str().c_str() );

					found_header = true;
					state = 1;
				}
			}
			else if ( state == 1 ) // Looking for the tempo
			{
				if ( std::regex_search( szBuffer, match, regex_tempo ) )
				{
					m_iTempo = atoi( match[ 1 ].str().c_str() );
					m_iTicksPerSecond = int( 1.0 / ( ( static_cast<double>( m_iTempo ) / static_cast<double>( m_iPulsesPerQuarter ) ) / 1000000.0 ) );

					found_tempo = true;
					state = 2;
				}
			}
			else if ( state == 2 ) // Looking for the program start
			{
				if ( std::regex_search( szBuffer, match, regex_program ) )
				{
					found_program_start = true;
					state = 3;
				}
			}
			else if ( state == 3 ) // Looking for notes
			{
				midi_note_t note;
				bool found_note = false;

				if ( std::regex_search( szBuffer, match, regex_note_on ) )
				{
					note.tick = atoi( match[ 1 ].str().c_str() );
					note.on = 1;
					note.note = atoi( match[ 2 ].str().c_str() );
					note.velocity = atoi( match[ 3 ].str().c_str() );

					found_note = true;
				}
				//else if ( std::regex_search( szBuffer, match, regex_note_off ) )
				//{
				//	note.tick = atoi( match[ 1 ].str().c_str() );
				//	note.on = 0;
				//	note.note = atoi( match[ 2 ].str().c_str() );
				//	note.velocity = atoi( match[ 3 ].str().c_str() );

				//	found_note = true;
				//}

				if ( found_note )
				{
					m_notes.push_back( note );
					last_ticks = note.tick;
					found_notes = true;
				}
			}
		}

		if ( found_header && found_tempo && found_program_start && found_notes )
		{
			Msg( "Successfully parsed a MIDI format file. " );
			Msg( "Ticks: %d | TPS: %d | Total duration: %.1f\n", last_ticks, m_iTicksPerSecond, last_ticks * ( 1.f / (float)m_iTicksPerSecond ) );

			m_bPlaying = true;
		}
		else
		{
			Warning( "Failed to parse a MIDI format file\n" );
			m_notes.clear();
		}

		fclose( file );
	}
}

//-----------------------------------------------------------------------------
// CMidiSoundPlayer::CreateMove
//-----------------------------------------------------------------------------

const char *pzdc[] =
{
	"0.010000001,9",
	"0.00001,1,+use;wait;-use;wait",
	"0.003405298645,32,+use;wait;-use;wait",
	"0.00001,1,cl_stopsound",
	"0.010000001,2",
	"0.00001,1,+use;wait;-use;wait",
	"0.003405298645,30,+use;wait;-use;wait",
	"0.00001,1,cl_stopsound",
	"0.010000001,2",
	"0.00001,1,+use;wait;-use;wait",
	"0.001702620333,60,+use;wait;-use;wait",
	"0.00001,1,cl_stopsound",
	"0.010000001,15",
	"0.00001,1,+use;wait;-use;wait",
	"0.002272727273,40,+use;wait;-use;wait",
	"0.00001,1,cl_stopsound",
	"0.010000001,29",
	"0.00001,1,+use;wait;-use;wait",
	"0.002407897905,40,+use;wait;-use;wait",
	"0.00001,1,cl_stopsound",
	"0.010000001,15",
	"0.00001,1,+use;wait;-use;wait",
	"0.002551020408,40,+use;wait;-use;wait",
	"0.00001,1,cl_stopsound",
	"0.010000001,15",
	"0.00001,1,+use;wait;-use;wait",
	"0.00286344243,80,+use;wait;-use;wait",
	"0.00001,1,cl_stopsound",
	"0.010000001,2",
	"0.00001,1,+use;wait;-use;wait",
	"0.003405298645,30,+use;wait;-use;wait",
	"0.00001,1,cl_stopsound",
	"0.010000001,2",
	"0.00001,1,+use;wait;-use;wait",
	"0.00286344243,30,+use;wait;-use;wait",
	"0.00001,1,cl_stopsound",
	"0.010000001,3",
	"0.00001,1,+use;wait;-use;wait",
	"0.002551020408,40,+use;wait;-use;wait",
	"0.005,1,cl_stopsound",
	"0.010000001,9",
	"0.00001,1,+use;wait;-use;wait",
	"0.003405298645,32,+use;wait;-use;wait",
	"0.00001,1,cl_stopsound",
	"0.010000001,2",
	"0.00001,1,+use;wait;-use;wait",
	"0.003405298645,30,+use;wait;-use;wait",
	"0.00001,1,cl_stopsound",
	"0.010000001,2",
	"0.00001,1,+use;wait;-use;wait",
	"0.001702620333,60,+use;wait;-use;wait",
	"0.00001,1,cl_stopsound",
	"0.010000001,15",
	"0.00001,1,+use;wait;-use;wait",
	"0.002272727273,40,+use;wait;-use;wait",
	"0.00001,1,cl_stopsound",
	"0.010000001,29",
	"0.00001,1,+use;wait;-use;wait",
	"0.002407897905,40,+use;wait;-use;wait",
	"0.00001,1,cl_stopsound",
	"0.010000001,15",
	"0.00001,1,+use;wait;-use;wait",
	"0.002551020408,40,+use;wait;-use;wait",
	"0.00001,1,cl_stopsound",
	"0.010000001,15",
	"0.00001,1,+use;wait;-use;wait",
	"0.00286344243,80,+use;wait;-use;wait",
	"0.00001,1,cl_stopsound",
	"0.010000001,2",
	"0.00001,1,+use;wait;-use;wait",
	"0.003405298645,30,+use;wait;-use;wait",
	"0.00001,1,cl_stopsound",
	"0.010000001,2",
	"0.00001,1,+use;wait;-use;wait",
	"0.00286344243,30,+use;wait;-use;wait",
	"0.00001,1,cl_stopsound",
	"0.010000001,3",
	"0.00001,1,+use;wait;-use;wait",
	"0.002551020408,40,+use;wait;-use;wait",
	"0.005,1,cl_stopsound",
	"0.010000001,9",
	"0.00001,1,+use;wait;-use;wait",
	"0.003405298645,32,+use;wait;-use;wait",
	"0.00001,1,cl_stopsound",
	"0.010000001,2",
	"0.00001,1,+use;wait;-use;wait",
	"0.003405298645,30,+use;wait;-use;wait",
	"0.00001,1,cl_stopsound",
	"0.010000001,2",
	"0.00001,1,+use;wait;-use;wait",
	"0.001702620333,60,+use;wait;-use;wait",
	"0.00001,1,cl_stopsound",
	"0.010000001,15",
	"0.00001,1,+use;wait;-use;wait",
	"0.002272727273,40,+use;wait;-use;wait",
	"0.00001,1,cl_stopsound",
	"0.010000001,29",
	"0.00001,1,+use;wait;-use;wait",
	"0.002407897905,40,+use;wait;-use;wait",
	"0.00001,1,cl_stopsound",
	"0.010000001,15",
	"0.00001,1,+use;wait;-use;wait",
	"0.002551020408,40,+use;wait;-use;wait",
	"0.00001,1,cl_stopsound",
	"0.010000001,15",
	"0.00001,1,+use;wait;-use;wait",
	"0.00286344243,80,+use;wait;-use;wait",
	"0.00001,1,cl_stopsound",
	"0.010000001,2",
	"0.00001,1,+use;wait;-use;wait",
	"0.003405298645,30,+use;wait;-use;wait",
	"0.00001,1,cl_stopsound",
	"0.010000001,2",
	"0.00001,1,+use;wait;-use;wait",
	"0.00286344243,30,+use;wait;-use;wait",
	"0.00001,1,cl_stopsound",
	"0.010000001,3",
	"0.00001,1,+use;wait;-use;wait",
	"0.002551020408,40,+use;wait;-use;wait",
	"0.005,1,cl_stopsound"
};

bool __play = false;
int __play_count = 0;
bool __executing_frame = false;
int __execute_frames_count = 0;
std::string __execute_command;

CON_COMMAND( sc_play_test, "" )
{
	__play = true;
	__executing_frame = false;
	__play_count = 0;
}

CON_COMMAND( sc_play_test_stop, "" )
{
	__play = false;
	__executing_frame = false;
	__play_count = 0;
}

#include <IRender.h>

static std::vector<Vector> points;
static int points_it = 0;
static int points_it2 = 0;
static int circle_points = 1;
static int clock_points = 1;
static double clock_angle = 0.f;
static Vector clock_pos;
static Vector clock_ang;

CON_COMMAND( sc_giga_test, "" )
{
	if ( args.ArgC() < 10 )
		return;

	float localSpaceToWorld[ 3 ][ 4 ];

	Vector vecPoint;
	points_it = 0;

	float radius = 60.f;

	circle_points = atoi( args[ 1 ] );
	clock_points = atoi( args[ 2 ] );
	clock_angle = atof( args[ 3 ] );
	clock_pos = Vector( atof( args[ 4 ] ), atof( args[ 5 ] ), atof( args[ 6 ] ) );
	clock_ang = Vector( atof( args[ 7 ] ), atof( args[ 8 ] ), atof( args[ 9 ] ) );

	float delta_angle = 2.0 * M_PI / circle_points;

	CVar()->SetValue( "fps_max", 10 * ( circle_points + clock_points ) );
	points_it2 = 0;

	for ( float angle = 0.f; angle < 2.0 * M_PI; angle += delta_angle )
	{
		vecPoint.Zero();

		vecPoint.x = cos( angle );
		vecPoint.y = sin( angle );

		vecPoint *= radius;

		points.push_back( vecPoint );
	}

	float delta_radius = radius / clock_points;

	for ( float r = 0.f; r < radius; r += delta_radius )
	{
		vecPoint.Zero();

		vecPoint.x = cos( clock_angle );
		vecPoint.y = sin( clock_angle );

		vecPoint *= r;

		points.push_back( vecPoint );
	}

	AngleMatrix( clock_ang, localSpaceToWorld );

	localSpaceToWorld[ 0 ][ 3 ] = clock_pos.x;
	localSpaceToWorld[ 1 ][ 3 ] = clock_pos.y;
	localSpaceToWorld[ 2 ][ 3 ] = clock_pos.z;
	
	for ( const Vector &point : points )
	{
		Vector worldPoint;
		VectorTransform( point, localSpaceToWorld, worldPoint );

		*(Vector *)&point = worldPoint;

		//Render()->DrawBox( worldPoint, Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), { 232, 0, 232, 150 }, 5.f );
	}
}

CON_COMMAND( sc_giga_test_stop, "" )
{
	points.clear();
	points_it = 0;
}

void CMidiSoundPlayer::CreateMove( float frametime, usercmd_t *cmd, int active )
{
	if ( !points.empty() )
	{
		if ( points_it >= (int)points.size() )
		{
			points_it = 0;
			points.clear();
			points_it2++;

			float localSpaceToWorld[ 3 ][ 4 ];

			Vector vecPoint;

			float radius = 60.f;
			float delta_angle = 2.0 * M_PI / circle_points;

			clock_angle += ( 2.0 * M_PI * ( 1.0 / double( ( circle_points + clock_points ) ) ) );
			Msg("%f\n", clock_angle );

			for ( float angle = 0.f; angle < 2.0 * M_PI; angle += delta_angle )
			{
				vecPoint.Zero();

				vecPoint.x = cos( angle );
				vecPoint.y = sin( angle );

				vecPoint *= radius;

				points.push_back( vecPoint );
			}

			float delta_radius = radius / clock_points;

			for ( float r = 0.f; r < radius; r += delta_radius )
			{
				vecPoint.Zero();

				vecPoint.x = cos( clock_angle );
				vecPoint.y = sin( clock_angle );

				vecPoint *= r;

				points.push_back( vecPoint );
			}

			AngleMatrix( clock_ang, localSpaceToWorld );

			localSpaceToWorld[ 0 ][ 3 ] = clock_pos.x;
			localSpaceToWorld[ 1 ][ 3 ] = clock_pos.y;
			localSpaceToWorld[ 2 ][ 3 ] = clock_pos.z;

			for ( const Vector &point : points )
			{
				Vector worldPoint;
				VectorTransform( point, localSpaceToWorld, worldPoint );

				*(Vector *)&point = worldPoint;

				//Render()->DrawBox( worldPoint, Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), { 232, 0, 232, 150 }, 5.f );
			}
		}

		Vector vecPoint = points[ points_it ];
		Vector vecEyes = g_pPlayerMove->origin + g_pPlayerMove->view_ofs;

		Vector vecAngles;
		Vector vecDir = vecPoint - vecEyes;

		VectorAngles( vecDir, vecAngles );
		vecAngles.z = 0.f;

		//char command_buffer[ 64 ];
		//snprintf( command_buffer, M_ARRAYSIZE( command_buffer ), "setang %f %f 0;npc_moveto", vecAngles.x, vecAngles.y );
		//g_pEngineFuncs->ClientCmd( command_buffer );

		g_pEngineFuncs->SetViewAngles( vecAngles );
		g_pEngineFuncs->ClientCmd( "npc_moveto" );

		points_it++;
	}

	//if ( __play )
	//{
	//	if ( __executing_frame )
	//	{
	//		g_pEngineFuncs->ClientCmd( __execute_command.c_str() );

	//		__execute_frames_count--;

	//		if ( __execute_frames_count == 0 )
	//		{
	//			__executing_frame = false;
	//		}
	//	}

	//	if ( __play_count >= M_ARRAYSIZE( pzdc ) )
	//	{
	//		__play = false;
	//		__executing_frame = false;
	//		return;
	//	}

	//	int c = 0;

	//	char *str = strdup( pzdc[ __play_count ] );
	//	char *pch;

	//	double frametime = 0.0;
	//	int frames = 0;
	//	std::string command;

	//	pch = strtok( str, "," );
	//	while ( pch != NULL )
	//	{
	//		c++;

	//		if ( c == 1 )
	//		{
	//			frametime = atof( pch );
	//		}
	//		else if ( c == 2 )
	//		{
	//			frames = atoi( pch );
	//		}
	//		else if ( c == 3 )
	//		{
	//			command = pch;
	//		}

	//		pch = strtok( NULL, "," );
	//	}

	//	Msg("%d | %f | %d | %s ( %s )\n", __play_count, frametime, frames, command.c_str(), pzdc[ __play_count ] );

	//	free( str );

	//	if ( frames > 1 )
	//	{
	//		__executing_frame = true;
	//		__execute_frames_count = frames - 1;
	//		__execute_command = command.c_str();
	//	}

	//	if ( !command.empty() )
	//		g_pEngineFuncs->ClientCmd( command.c_str() );

	//	CVar()->SetValue( "fps_max", (float)( 1.0 / frametime ) );

	//	__play_count++;
	//}

	//if ( m_bPlaying )
	//{
	//	if ( m_iNoteEndTicks == -1 )
	//	{
	//		const midi_note_t &note = m_notes[ 0 ];

	//		CVar()->SetValue( "fps_max", midi_notes_freq[ note.note ] );
	//		//CVar()->SetValue( "sc_st_min_frametime", 1.f / midi_notes_freq[ note.note ] );

	//		m_iNoteEndTicks = m_iTicks + note.velocity;
	//	}
	//	else if ( m_iTicks < m_iNoteEndTicks )
	//	{
	//		const midi_note_t &note = m_notes[ 0 ];

	//		CVar()->SetValue( "fps_max", midi_notes_freq[ note.note ] );
	//		//CVar()->SetValue( "sc_st_min_frametime", 1.f / midi_notes_freq[ note.note ] );
	//		
	//	}
	//	else
	//	{
	//		m_notes.erase( m_notes.begin() );

	//		if ( m_notes.empty() )
	//		{
	//			m_bPlaying = false;
	//			return;
	//		}

	//		g_pEngineFuncs->ClientCmd( "cl_stopsound" );

	//		const midi_note_t &note = m_notes[ 0 ];

	//		CVar()->SetValue( "fps_max", midi_notes_freq[ note.note ] );
	//		//CVar()->SetValue( "sc_st_min_frametime", 1.f / midi_notes_freq[ note.note ] );

	//		m_iNoteEndTicks = m_iTicks + note.velocity;
	//	}

	//	cmd->impulse = 100;
	//	//g_pEngineFuncs->ClientCmd( "impulse 100" );

	//	m_iTicks++;
	//}

	//if ( m_bPlaying )
	//{
	//	static float start = 0.f;

	//	bool bPlay = false;
	//	bool bStop = false;

	//	if ( m_iTicks == 0 )
	//	{
	//		start = g_pEngineFuncs->GetClientTime();
	//		Msg( "Start time: %.3f\n", start );
	//	}

	//	for ( size_t i = 0; i < m_notes.size(); i++ )
	//	{
	//		const midi_note_t &note = m_notes[ i ];

	//		if ( note.tick > m_iTicks )
	//			break;

	//		CVar()->SetValue( "fps_max", midi_notes_freq[ note.note ] );

	//		if ( !bPlay )
	//		{
	//			g_pEngineFuncs->ClientCmd( "impulse 100" );
	//			bPlay = true;
	//		}

	//		//if ( note.on )
	//		//{
	//		//	if ( !bPlay )
	//		//	{
	//		//		g_pEngineFuncs->ClientCmd( "impulse 100" );
	//		//		//g_pEngineFuncs->ClientCmd( g_pEngineFuncs->RandomLong( 0, 1 ) ? "+use;wait;-use" : "impulse 100");
	//		//		bPlay = true;
	//		//	}
	//		//}
	//		//else
	//		//{
	//		//	if ( !bStop )
	//		//	{
	//		//		g_pEngineFuncs->ClientCmd( "cl_stopsound" );
	//		//		bStop = true;
	//		//	}
	//		//}
	//	}

	//	m_iTicks++;

	//	if ( m_iTicks > (int)m_notes.size() )
	//	{
	//		Msg( "End time: %.3f (duration: %.3f)\n", g_pEngineFuncs->GetClientTime(), g_pEngineFuncs->GetClientTime() - start );

	//		m_bPlaying = false;
	//		m_notes.clear();
	//	}
	//}
}