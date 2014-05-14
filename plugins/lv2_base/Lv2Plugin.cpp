/*
 * Lv2Plugin.cpp - LV2 support for LMMS
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


#include "lmmsconfig.h"
#include "Lv2Plugin.h"




static Lv2Base * lv2 = findLv2();




void * Lv2Port::buffer()
{
	switch( type() )
	{
		case TypeControl:
			return static_cast<void *>( &m_value );
		case TypeAudio:
			return static_cast<void *>( m_buffer );
		case TypeEvent:
			return lv2_evbuf_get_buffer( m_evbuf );
		default:
			return NULL;
	}
}

void Lv2Port::reset()
{
	switch( type() )
	{
		case TypeEvent:
			lv2_evbuf_reset( m_evbuf, flow() == FlowInput );
			break;
		default:
			break;
	}
}




bool Lv2Port::writeEvent( const MidiEvent& event, const MidiTime& time )
{
	uint8_t msg[3];

	msg[0] = event.type();
	msg[1] = event.key() & 0x7f;
	msg[2] = event.velocity();

	LV2_Evbuf_Iterator iter = lv2_evbuf_end( m_evbuf );
	return lv2_evbuf_write( &iter, time, 0, midi_MidiEvent, 3, msg );
}




Lv2Plugin::Lv2Plugin( Lv2PluginDescriptor * descriptor, double rate, fpp_t bufferSize ) :
	m_descriptor( descriptor ),
	m_bufferSize( bufferSize )
{
	if( !instantiate( rate ) )
	{
		fprintf( stderr, "Could not instantiate <%s>\n", m_descriptor->uri() );
		return;
	}

	activate();
}




Lv2Plugin::~Lv2Plugin()
{
	if( m_instance )
	{
		deactivate();
		cleanup();

		for( uint32_t p = 0; p < m_ports.size(); ++p )
		{
			if( m_ports[p].type() == TypeAudio && m_ports[p].m_buffer )
			{
				free( m_ports[p].m_buffer );
			}
		}
	}
}




bool Lv2Plugin::instantiate( double rate )
{
	for( uint32_t p = 0; p < numPorts(); ++p )
	{
		Lv2Port port;
		Lv2PortDescriptor * portdesc = m_descriptor->portDescriptor( p );
		port.m_descriptor = portdesc;

		switch( portdesc->type() )
		{
			case TypeControl:
				port.m_value = portdesc->defaultValue();
				break;
			case TypeAudio:
				port.m_buffer = static_cast<float *>( calloc( m_bufferSize, sizeof( float ) ) );
				break;
			case TypeEvent:
				port.m_evbuf = lv2_evbuf_new( 1024, portdesc->evbufType(), atom_Chunk, atom_Sequence );
				port.reset();
				break;
			default:
				break;
		}
		m_ports.push_back( port );
	}
	m_instance = lilv_plugin_instantiate( m_descriptor->m_plugin, rate, lv2->s_features );
	return !!m_instance;
}




void Lv2Plugin::resizeBuffers( fpp_t newSize )
{
	if( m_bufferSize >= newSize )
	{
		return;
	}

	m_bufferSize = newSize;
	for( uint32_t p = 0; p < m_ports.size(); ++p )
	{
		if( m_ports[p].type() == TypeAudio )
		{
			m_ports[p].m_buffer = static_cast<float *>( realloc( m_ports[p].m_buffer, sizeof( float ) * newSize ) );
			memset( m_ports[p].m_buffer, 0, sizeof( float ) * newSize );
		}
	}
}




void Lv2Plugin::run( const fpp_t nframes )
{
	if( !m_instance )
	{
		return;
	}

	resizeBuffers( nframes );
	for( int i = 0; i < m_ports.size(); ++i )
	{
		lilv_instance_connect_port( m_instance, i, m_ports[i].buffer() );
	}

	lilv_instance_run( m_instance, static_cast<uint32_t>( nframes ) );

	for( int i = 0; i < m_ports.size(); ++i )
	{
		m_ports[i].reset();
	}
}
