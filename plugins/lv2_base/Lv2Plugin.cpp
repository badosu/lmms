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

#include "engine.h"
#include "Mixer.h"

#include "Lv2Plugin.h"




Lv2Port::Lv2Port() :
	m_descriptor( NULL ),
	m_buffer( NULL ),
	m_evbuf( NULL ),
	m_state( NULL )
{
}




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
			fprintf( stderr, "type for port `%s' unknown, returning NULL\n", symbol() );
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
	m_frame = 0;
}




void Lv2Port::writeEvent( const f_cnt_t time, const MidiEvent& event )
{
	QPair<f_cnt_t, MidiEvent> qp( time, event );
	m_rawbuf.push_back( qp );
}




static bool lessThan( const QPair<f_cnt_t, MidiEvent> a, const QPair<f_cnt_t, MidiEvent> b )
{
	return a.first < b.first;
}




void Lv2Port::sortEvents()
{
	if( type() != TypeEvent )
	{
		return;
	}

	qStableSort( m_rawbuf.begin(), m_rawbuf.end(), lessThan );
	LV2_Evbuf_Iterator iter = lv2_evbuf_begin( m_evbuf );
	for( uint32_t i = 0; i < m_rawbuf.size(); ++i )
	{
		uint8_t msg[3];

		msg[0] = m_rawbuf[i].second.type();
		msg[1] = m_rawbuf[i].second.key() & 0x7f;
		msg[2] = m_rawbuf[i].second.velocity();

		lv2_evbuf_write( &iter, m_rawbuf[i].first, 0, midi_MidiEvent, 3, msg );
	}
	m_rawbuf.clear();
}




Lv2Plugin::Lv2Plugin( Lv2PluginDescriptor * descriptor, double rate, fpp_t bufferSize ) :
	m_descriptor( descriptor ),
	m_instance( NULL ),
	m_bufferSize( bufferSize ),
	m_stateString( NULL )
{
	lv2()->setRate( rate );
	lv2()->setBufferSize( engine::mixer()->framesPerPeriod() );

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
				port.m_evbuf = lv2_evbuf_new( lv2()->s_sequenceSize, portdesc->evbufType(), atom_Chunk, atom_Sequence );
				port.reset();
				break;
			default:
				break;
		}
		m_ports.push_back( port );
	}
	m_instance = lilv_plugin_instantiate( m_descriptor->m_plugin, rate, lv2()->s_features );
	return !!m_instance;
}




Lv2Port * Lv2Plugin::port( const char * symbol )
{
	for( int i = 0; i < m_ports.size(); ++i )
	{
		if( !strcmp( symbol, m_ports[i].symbol() ) )
		{
			return &m_ports[i];
		}
	}
	return NULL;
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
	lv2()->setBufferSize( newSize );
}




void Lv2Plugin::run( const fpp_t nframes )
{
	if( !m_instance )
	{
		return;
	}

	lv2()->setRate( engine::mixer()->framesPerPeriod() );
	resizeBuffers( nframes );
	for( int i = 0; i < m_ports.size(); ++i )
	{
		m_ports[i].sortEvents();
		lilv_instance_connect_port( m_instance, i, m_ports[i].buffer() );
	}

	lilv_instance_run( m_instance, static_cast<uint32_t>( nframes ) );

	for( int i = 0; i < m_ports.size(); ++i )
	{
		m_ports[i].reset();
	}
}




static void setPortValue( const char * symbol, void * data, const void * value, uint32_t size, uint32_t type )
{
	Lv2Plugin * self = static_cast<Lv2Plugin *>( data );
	Lv2Port * port = self->port( symbol );
	if( !port )
	{
		fprintf( stderr, "error: Preset port `%s' is missing\n", symbol );
		return;
	}

	float fvalue;
	if( type == atom_Float )
	{
		fvalue = *static_cast<const float *>( value );
	}
	else if( type == atom_Double )
	{
		fvalue = *static_cast<const double *>( value );
	}
	else if( type == atom_Int )
	{
		fvalue = *static_cast<const int32_t*>( value );
	}
	else if( type == atom_Long )
	{
		fvalue = *static_cast<const int64_t*>( value );
	}
	else
	{
		fprintf( stderr, "error: Preset `%s' value has bad type\n", symbol );
		return;
	}

	port->setValue( fvalue );
}




void Lv2Plugin::loadState( const char * stateString )
{
	LilvState * state = lilv_state_new_from_string( lv2()->world(), &lv2()->urid__map, stateString );
	lilv_state_restore( state, m_instance, setPortValue, this, 0, NULL );
	lilv_state_free( state );
}




static const void * getPortValue( const char * symbol, void * data, uint32_t * size, uint32_t * type )
{
	Lv2Plugin * self = static_cast<Lv2Plugin *>( data );
	Lv2Port * port = self->port( symbol );

	if( port && port->flow() == FlowInput && port->type() == TypeControl )
	{
		*size = sizeof( float );
		*type = atom_Float;
		return port->buffer();
	}
	*size = 0;
	*type = 0;
	return NULL;
}




void Lv2Plugin::saveState()
{
	LilvState * state = lilv_state_new_from_instance( plugin(), instance(), &lv2()->urid__map, NULL, NULL, NULL, NULL, getPortValue, this, LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE, NULL );
	m_stateString = lilv_state_to_string( lv2()->world(), &lv2()->urid__map, &lv2()->urid__unmap, state, "urn:lmms:state", NULL );
	lilv_state_free( state );
}




void Lv2Plugin::loadPreset( int index )
{
	const LilvNode * preset = descriptor()->preset( index )->node();
	LilvState * state = lilv_state_new_from_world( lv2()->world(), &lv2()->urid__map, preset );
	if( state )
	{
		lilv_state_restore( state, m_instance, setPortValue, this, 0, NULL );
	}
	lilv_state_free( state );
}
