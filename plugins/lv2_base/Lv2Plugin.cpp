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


std::vector<const char *> Lv2Plugin::s_uriMap;
std::vector<LilvNode *> Lv2Plugin::s_nodeMap;
std::vector<const char *> Lv2Plugin::s_pluginUris;

LilvPlugins * Lv2Plugin::s_plugins;
LilvWorld * Lv2Plugin::s_world;
long Lv2Plugin::s_refcount = 0;


#define NODE(X) s_nodeMap[X-1]


static LV2_URID_Map urid__map = { NULL, NULL };
static LV2_URID_Unmap urid__unmap = { NULL, NULL };

static LV2_Feature mapFeature = { LV2_URID__map, &urid__map };
static LV2_Feature unmapFeature = { LV2_URID__unmap, &urid__unmap };

/*
static LV2_Options_Option options[5];
static LV2_Feature optionsFeature = { LV2_OPTIONS__options, NULL };

static LV2_Feature bufSizeFeatures[3] = {
	{ LV2_BUF_SIZE__powerOf2BlockLength, NULL },
	{ LV2_BUF_SIZE__fixedBlockLength, NULL },
	{ LV2_BUF_SIZE__boundedBlockLength, NULL }
};
 */

static const LV2_Feature* features[7] = {
	&mapFeature, &unmapFeature,
	//~ &optionsFeature,
	//~ &bufSizeFeatures[0],
	//~ &bufSizeFeatures[1],
	//~ &bufSizeFeatures[2],
	NULL
};




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
			return 0;
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
	return lv2_evbuf_write( &iter, time, 0, Lv2Plugin::midi_MidiEvent, 3, msg );
}




Lv2Plugin::Lv2Plugin( const char * uri, double rate, fpp_t bufferSize ) :
	m_midiIn( -1 )
{
	if( s_refcount++ == 0 )
	{
		s_world = lilv_world_new();
		lilv_world_load_all( s_world );
		s_plugins = const_cast<LilvPlugins *>( lilv_world_get_all_plugins( s_world ) );

		s_uriMap.push_back( LV2_ATOM__AtomPort );
		s_uriMap.push_back( LV2_ATOM__Chunk );
		s_uriMap.push_back( LV2_ATOM__Float );
		s_uriMap.push_back( LV2_ATOM__Int );
		s_uriMap.push_back( LV2_ATOM__Sequence );
		s_uriMap.push_back( LV2_BUF_SIZE__maxBlockLength );
		s_uriMap.push_back( LV2_BUF_SIZE__minBlockLength );
		s_uriMap.push_back( LV2_BUF_SIZE__sequenceSize );
		s_uriMap.push_back( LV2_EVENT__EventPort );
		s_uriMap.push_back( LV2_CORE__AudioPort );
		s_uriMap.push_back( LV2_CORE__ControlPort );
		s_uriMap.push_back( LV2_CORE__InputPort );
		s_uriMap.push_back( LV2_CORE__OutputPort );
		s_uriMap.push_back( LV2_CORE__control );
		s_uriMap.push_back( LV2_CORE__name );
		s_uriMap.push_back( LV2_MIDI__MidiEvent );
		s_uriMap.push_back( LV2_PARAMETERS__sampleRate );
		s_uriMap.push_back( LV2_PORT_GROUPS__left );
		s_uriMap.push_back( LV2_PORT_GROUPS__right );
		s_uriMap.push_back( LILV_NS_RDFS "label" );
		s_uriMap.push_back( LV2_URID__map );
		s_uriMap.push_back( LV2_URID__unmap );

		urid__map.map = mapUri;
		urid__unmap.unmap = unmapUri;

		for( int i = 0; i < s_uriMap.size(); ++i )
		{
			LilvNode * node = lilv_new_uri( s_world, s_uriMap[i] );
			s_nodeMap.push_back( node );
		}

		LILV_FOREACH( plugins, i, s_plugins )
		{
			const LilvPlugin * plugin = lilv_plugins_get( s_plugins, i );
			const LilvNode * node = lilv_plugin_get_uri( plugin );
			s_pluginUris.push_back( lilv_node_as_uri( node ) );
		}

	}

	LilvNode * node = lilv_new_uri( s_world, uri );
	m_plugin = const_cast<LilvPlugin *>( lilv_plugins_get_by_uri( s_plugins, node ) );
	free( node );

	if( !instantiate( rate ) )
	{
		fprintf( stderr, "Could not instantiate <%s>\n", uri );
		return;
	}

	m_bufferSize = bufferSize;

	createPorts();
	activate();

	run( bufferSize );
}




Lv2Plugin::~Lv2Plugin()
{
	if( m_instance )
	{
		deactivate();
		cleanup();

		for( uint32_t p = 0; p < m_ports.size(); ++p )
		{
			if( m_ports[p].type() == Lv2Port::TypeAudio && m_ports[p].m_buffer )
			{
				free( m_ports[p].m_buffer );
			}
		}
	}

	if( --s_refcount == 0 )
	{
		for( int i = 0; i < s_nodeMap.size(); ++i )
		{
			lilv_node_free( s_nodeMap[i] );
		}
		s_nodeMap.clear();
		s_uriMap.clear();
		s_pluginUris.clear();
		lilv_world_free( s_world );
	}
}




void Lv2Plugin::createPorts()
{
	LilvNode * node;
	LilvPort * portdesc;

	float* mins = new float[numPorts()];
	float* maxs = new float[numPorts()];
	float* defs = new float[numPorts()];
	lilv_plugin_get_port_ranges_float( m_plugin, mins, maxs, defs );

	for( uint32_t p = 0; p < numPorts(); ++p )
	{
		Lv2Port port;

		portdesc = const_cast<LilvPort *>( lilv_plugin_get_port_by_index( m_plugin, p ) );

		port.m_symbol = lilv_node_as_string( lilv_port_get_symbol( m_plugin, portdesc ) );
		node = const_cast<LilvNode *>( lilv_port_get_name( m_plugin, portdesc ) );
		port.m_name = lilv_node_as_string( node );
		free( node );

		// Flow
		if( lilv_port_is_a( m_plugin, portdesc, NODE( lv2_InputPort ) ) )
		{
			port.m_flow = Lv2Port::FlowInput;
		}
		else if( lilv_port_is_a( m_plugin, portdesc, NODE( lv2_OutputPort ) ) )
		{
			port.m_flow = Lv2Port::FlowOutput;
		}

		// Type
		if( lilv_port_is_a( m_plugin, portdesc, NODE( lv2_ControlPort ) ) )
		{
			port.m_type = Lv2Port::TypeControl;
			port.m_minimum = mins[p];
			port.m_maximum = maxs[p];
			port.setValue( defs[p] );
		}
		else if( lilv_port_is_a( m_plugin, portdesc, NODE( lv2_AudioPort ) ) )
		{
			port.m_type = Lv2Port::TypeAudio;
			port.m_buffer = static_cast<float *>( malloc( sizeof( float ) * m_bufferSize ) );
			memset( port.m_buffer, 0, sizeof( float ) * m_bufferSize );
			switch( port.flow() )
			{
				case Lv2Port::FlowInput:
					m_inputs.push_back( p );
					break;
				case Lv2Port::FlowOutput:
					m_outputs.push_back( p );
					break;
				default:
					break;
			}
		}
		if( lilv_port_is_a( m_plugin, portdesc, NODE( atom_AtomPort ) ) || lilv_port_is_a( m_plugin, portdesc, NODE( ev_EventPort ) ) )
		{
			port.m_type = Lv2Port::TypeEvent;
			if( m_midiIn == -1 && port.flow() == Lv2Port::FlowInput )
			{
				m_midiIn = p;
			}
			LV2_Evbuf_Type type = lilv_port_is_a( m_plugin, portdesc, NODE( atom_AtomPort ) ) ? LV2_EVBUF_ATOM : LV2_EVBUF_EVENT;
			port.m_evbuf = lv2_evbuf_new( 1024, type, atom_Chunk, atom_Sequence );
			lv2_evbuf_reset( port.m_evbuf, port.flow() == Lv2Port::FlowInput );
		}

		m_ports.push_back( port );
	}

	delete[] mins;
	delete[] maxs;
	delete[] defs;
}




bool Lv2Plugin::instantiate( double rate )
{
	m_instance = lilv_plugin_instantiate( m_plugin, rate, features );
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
		if( m_ports[p].type() == Lv2Port::TypeAudio )
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




LV2_URID Lv2Plugin::mapUri( LV2_URID_Map_Handle handle, const char * uri )
{
	for( int i = 0; i < s_uriMap.size(); ++i )
	{
		if( !strcmp( uri, s_uriMap[i] ) )
		{
			return i + 1;
		}
	}
	s_uriMap.push_back( uri );
	return s_uriMap.size();
}




const char* Lv2Plugin::unmapUri( LV2_URID_Unmap_Handle handle, LV2_URID urid )
{
	if( !urid || urid > s_uriMap.size() )
	{
		return NULL;
	}
	return s_uriMap[urid - 1];
}




bool Lv2Plugin::featureIsSupported( const char * uri )
{
	for( int i = 0; features[i]; ++i )
	{
		if( !strcmp( features[i]->URI, uri ) )
		{
			return true;
		}
	}
	return false;
}
