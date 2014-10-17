/*
 * Lv2Controls.cpp - LV2 plugin controls
 *
 * Copyright (c) 2014 Hannu Haahti <grejppi/at/gmail.com>
 *
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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


#include "Lv2Controls.h"




Lv2Controls::Lv2Controls( Lv2Plugin * plugin ) :
	m_plugin( plugin )
{
	int numPorts = m_plugin->descriptor()->numPorts();
	for( int p = 0; p < numPorts; ++p )
	{
		Lv2PortDescriptor * port = m_plugin->descriptor()->portDescriptor( p );
		if( port->type() == TypeControl && port->flow() == FlowInput )
		{
			knob * knob = new knob();
			m_knobs[p] = knob;
		}
	}
}




Lv2Controls::~Lv2Controls()
{
	for( int p = 0; p < m_knobs.size(); ++p )
	{
		delete m_knobs[p];
	}
	m_knobs.clear();
}
