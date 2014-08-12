/*
 * Lv2NoteHandle.cpp - LV2 note events
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


#include "Lv2NoteHandle.h"




Lv2NoteHandle::Lv2NoteHandle( float frequency, float velocity, float panning, uint32_t id ) :
	m_gate( true ),
	m_frequency( frequency ),
	m_velocity( velocity ),
	m_panning( panning ),
	m_oldFrequency( frequency - 1 ),
	m_oldVelocity( velocity - 1 ),
	m_oldPanning( panning - 1 ),
	m_id( id )
{
}




Lv2NoteHandle::~Lv2NoteHandle() {}




void Lv2NoteHandle::query( float ** frequency, float ** velocity, float ** panning )
{
	*frequency = ( m_frequency != m_oldFrequency ) ? &m_frequency : NULL;
	m_oldFrequency = m_frequency;

	*velocity = ( m_velocity != m_oldVelocity ) ? &m_velocity : NULL;
	m_oldVelocity = m_velocity;

	*panning = ( m_panning != m_oldPanning ) ? &m_panning : NULL;
	m_oldPanning = m_panning;
}
