/*
 * Lv2Plugin.cpp - LV2 support for LMMS
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


#include "lmmsconfig.h"

#include "Engine.h"
#include "Mixer.h"

#include "Lv2Plugin.h"




Lv2Port::Lv2Port() :
	m_descriptor( NULL ),
	m_plugin( NULL ),
	m_buffer( NULL ),
	m_atomSequence( NULL ),
	m_state( NULL )
{
}




Lv2Port::~Lv2Port()
{
}




void Lv2Port::cleanup()
{
	if( m_buffer )
	{
		free( static_cast<void *>( m_buffer ) );
		m_buffer = NULL;
	}
	if( m_atomSequence )
	{
		free( static_cast<void *>( m_atomSequence ) );
		m_buffer = NULL;
	}
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
			return static_cast<void *>( m_atomSequence );
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
			break;
		default:
			break;
	}
	m_frame = 0;
}




void Lv2Port::convertEvents()
{
	if( type() != TypeEvent )
	{
		return;
	}

	if( eventType() != EventTypeMidi )
	{
		return;
	}

	LV2_Atom_Sequence * processed = m_noteConverter.process( m_atomSequence, m_plugin->baseVelocity() );
	memcpy( m_atomSequence, processed, sizeof( LV2_Atom_Sequence ) + lv2()->s_sequenceSize );
}




Lv2Plugin::Lv2Plugin( Lv2PluginDescriptor * descriptor, double rate, fpp_t bufferSize ) :
	m_descriptor( descriptor ),
	m_instance( NULL ),
	m_bufferSize( bufferSize ),
	m_stateString( NULL )
{
	lv2()->setRate( rate );
	lv2()->setBufferSize( Engine::mixer()->framesPerPeriod() );

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
			m_ports[p].cleanup();
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
		port.m_plugin = this;

		switch( portdesc->type() )
		{
			case TypeControl:
				port.m_value = portdesc->defaultValue();
				break;
			case TypeAudio:
				port.m_buffer = static_cast<float *>( calloc( m_bufferSize, sizeof( float ) ) );
				break;
			case TypeEvent:
				port.m_atomSequence = static_cast<LV2_Atom_Sequence *>( calloc( 1, sizeof( LV2_Atom_Sequence ) + lv2()->s_sequenceSize ) );
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

	lv2()->setRate( Engine::mixer()->framesPerPeriod() );
	resizeBuffers( nframes );
	for( int i = 0; i < m_ports.size(); ++i )
	{
		m_ports[i].convertEvents();
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
