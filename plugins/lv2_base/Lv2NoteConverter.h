/*
 * Lv2NoteConverter.h - Convert LV2 note events to MIDI events
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


#ifndef LV2NOTECONVERTER_H
#define LV2NOTECONVERTER_H

#include "Lv2Base.h"




class Lv2NoteConverter
{
public:
	Lv2NoteConverter();
	~Lv2NoteConverter();

	LV2_Atom_Sequence * process( const LV2_Atom_Sequence * inBuffer, uint8_t baseVelocity );

private:
	const LV2_Atom_Sequence * m_inBuffer;
	LV2_Atom_Sequence * m_outBuffer;

	uint8_t m_baseVelocity;

	uint32_t m_ids[128];
	LV2_Atom_Forge m_forge;
	LV2_Atom_Forge_Frame m_frame;
};




#endif
