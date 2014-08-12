/*
 * Lv2NoteConverter.cpp - Convert LV2 note events to MIDI events
 *
 * Copyright (c) 2014 Hannu Haahti <grejppi/at/gmail.com>
 *
 * This file is part of LMMS - http://lmms.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */


#include <math.h>
#include "Lv2NoteConverter.h"




static inline uint8_t hz2key( float hz )
{
	return static_cast<uint8_t>( round( 69 + ( 12 * log2( hz / 440.0 ) ) ) );
}




Lv2NoteConverter::Lv2NoteConverter()
{
	lv2_atom_forge_init( &m_forge, &lv2()->urid__map );
	m_outBuffer = static_cast<LV2_Atom_Sequence *>( calloc( 1, sizeof( LV2_Atom_Sequence ) + lv2()->s_sequenceSize ) );
	for( int i = 0; i < 128; ++i )
	{
		m_ids[i] = 0;
	}
}




Lv2NoteConverter::~Lv2NoteConverter()
{
}




LV2_Atom_Sequence * Lv2NoteConverter::process( const LV2_Atom_Sequence * inBuffer, uint8_t baseVelocity )
{
	m_inBuffer = inBuffer;
	m_baseVelocity = baseVelocity;

	uint16_t midiVelocity;

	lv2_atom_sequence_clear( m_outBuffer );
	memset( LV2_ATOM_CONTENTS( LV2_Atom_Sequence, m_outBuffer ), 0, lv2()->s_sequenceSize );

	lv2_atom_forge_set_buffer( &m_forge, reinterpret_cast<uint8_t *>( m_outBuffer ), lv2()->s_sequenceSize );
	lv2_atom_forge_sequence_head( &m_forge, &m_frame, 0 );

	LV2_ATOM_SEQUENCE_FOREACH( m_inBuffer, ev )
	{
		if( ev->body.type == atom_Object )
		{
			LV2_Atom_Object * object = reinterpret_cast<LV2_Atom_Object *>( &ev->body );
			if( object->body.otype != note_NoteEvent )
			{
				continue;
			}

			const LV2_Atom_Int * id = NULL;
			const LV2_Atom_Bool * gate = NULL;
			const LV2_Atom_Float * frequency = NULL;
			const LV2_Atom_Float * velocity = NULL;

			LV2_Atom_Object_Query q[] =
			{
				{ note_id,        reinterpret_cast<const LV2_Atom **>( &id ) },
				{ note_gate,      reinterpret_cast<const LV2_Atom **>( &gate ) },
				{ note_frequency, reinterpret_cast<const LV2_Atom **>( &frequency ) },
				{ note_velocity,  reinterpret_cast<const LV2_Atom **>( &velocity ) },
				LV2_ATOM_OBJECT_QUERY_END
			};
			lv2_atom_object_query( object, q );

			// A note event is useless without an ID, and MIDI does only
			// note-on and note-off.
			if( !id || !gate )
			{
				continue;
			}

			uint8_t msg[3] = { 0xff };

			if( gate->body )
			{
				// note-on without a note?
				if( !frequency )
				{
					continue;
				}

				msg[0] = LV2_MIDI_MSG_NOTE_ON;
				msg[1] = hz2key( frequency->body );

				midiVelocity = qMin( qMax( 0, static_cast<int>( velocity->body * baseVelocity ) ), 127 );
				msg[2] = midiVelocity & 0x7f;

				// ignore if this MIDI key is already playing
				if( m_ids[msg[1]] )
				{
					continue;
				}

				m_ids[msg[1]] = id->body;
			}
			else
			{
				msg[0] = LV2_MIDI_MSG_NOTE_OFF;

				for( uint8_t i = 0; i < 128; ++i )
				{
					if( m_ids[i] == id->body )
					{
						msg[1] = i;
						m_ids[i] = 0;
						break;
					}
				}

				msg[2] = 0;

				// ignore if no playing note was found with this ID
				if( msg[1] == 0xff )
				{
					continue;
				}
			}

			// finally send the event
			lv2_atom_forge_frame_time( &m_forge, ev->time.frames );
			lv2_atom_forge_atom( &m_forge, 3, midi_MidiEvent );
			lv2_atom_forge_write( &m_forge, msg, 3 );
		}
	}

	lv2_atom_forge_pop( &m_forge, &m_frame );
	return m_outBuffer;
}
