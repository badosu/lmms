/*
 * Lv2NoteHandle.h - LV2 note events
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


#ifndef LV2NOTEHANDLE_H
#define LV2NOTEHANDLE_H

#include "Lv2Base.h"




class Lv2NoteHandle
{
public:
	Lv2NoteHandle( float frequency, float velocity, float panning, uint32_t id );
	~Lv2NoteHandle();

	const uintptr_t id() const { return m_id; }
	void query( float ** frequency, float ** velocity, float ** panning );

	void setGate( bool value ) { m_gate = value; }
	void setFrequency( float value ) { m_frequency = value; }
	void setVelocity( float value ) { m_velocity = value; }
	void setPanning( float value ) { m_panning = value; }

	const bool gate() const { return m_gate; }
	const float frequency() const { return m_frequency; }
	const float velocity() const { return m_velocity; }
	const float panning() const { return m_panning; }

private:
	bool m_gate;

	float m_frequency;
	float m_velocity;
	float m_panning;

	float m_oldFrequency;
	float m_oldVelocity;
	float m_oldPanning;

	uint32_t m_id;
};




#endif
