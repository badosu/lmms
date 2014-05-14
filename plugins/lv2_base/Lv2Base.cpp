/*
 * Lv2Base.cpp - basic LV2 support for LMMS
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


#include "Lv2Base.h"


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT lv2base_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"LV2 Base",
	"Basic LV2 support for LMMS",
	"Hannu Haahti <grejppi/at/gmail/dot/com>",
	0x0100,
	Plugin::Library,
	NULL,
	NULL
};

}




//
// LV2Base
// ------------------------------------------------------------------------


QVector<const char *> Lv2Base::s_uriMap;
QVector<LilvNode *> Lv2Base::s_nodeMap;
QVector<Lv2PluginDescriptor *> Lv2Base::s_descriptors;

LilvWorld * Lv2Base::s_world;
LilvPlugins * Lv2Base::s_plugins;


static LV2_URID_Map urid__map = { NULL, NULL };
static LV2_URID_Unmap urid__unmap = { NULL, NULL };

static LV2_Feature mapFeature = { LV2_URID__map, &urid__map };
static LV2_Feature unmapFeature = { LV2_URID__unmap, &urid__unmap };

//~ static LV2_Options_Option options[5];
//~ static LV2_Feature optionsFeature = { LV2_OPTIONS__options, NULL };

//~ static LV2_Feature bufSizeFeatures[3] = {
	//~ { LV2_BUF_SIZE__powerOf2BlockLength, NULL },
	//~ { LV2_BUF_SIZE__fixedBlockLength, NULL },
	//~ { LV2_BUF_SIZE__boundedBlockLength, NULL }
//~ };

const LV2_Feature* Lv2Base::s_features[] = {
	&mapFeature, &unmapFeature,
	//~ &optionsFeature,
	//~ &bufSizeFeatures[0],
	//~ &bufSizeFeatures[1],
	//~ &bufSizeFeatures[2],
	NULL
};




Lv2Base::Lv2Base()
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
		Lv2PluginDescriptor * desc = new Lv2PluginDescriptor( plugin );
		s_descriptors.push_back( desc );
	}
}




Lv2Base::~Lv2Base()
{
	for( int i = 0; i < s_nodeMap.size(); ++i )
	{
		lilv_node_free( s_nodeMap[i] );
	}
	for( int i = 0; i < s_descriptors.size(); ++i )
	{
		delete s_descriptors[i];
	}
	s_nodeMap.clear();
	s_uriMap.clear();
	s_descriptors.clear();
	lilv_world_free( s_world );
}




LV2_URID Lv2Base::mapUri( LV2_URID_Map_Handle handle, const char * uri )
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




const char* Lv2Base::unmapUri( LV2_URID_Unmap_Handle handle, LV2_URID urid )
{
	if( !urid || urid > s_uriMap.size() )
	{
		return NULL;
	}
	return s_uriMap[urid - 1];
}




bool Lv2Base::featureIsSupported( const char * uri )
{
	for( int i = 0; s_features[i]; ++i )
	{
		if( !strcmp( s_features[i]->URI, uri ) )
		{
			return true;
		}
	}
	return false;
}




Lv2PluginDescriptor * Lv2Base::descriptor( const char * uri )
{
	for( unsigned long i = 0; i < s_descriptors.size(); ++i )
	{
		if( !strcmp( s_descriptors[i]->uri(), uri ) )
		{
			return s_descriptors[i];
		}
	}
	return NULL;
}




// We need a single instance of this
static Lv2Base lv2;

// and a single way to find it
Lv2Base * findLv2()
{
	return &lv2;
}




//
// LV2PluginDescriptor
// ------------------------------------------------------------------------


Lv2PluginDescriptor::Lv2PluginDescriptor( const LilvPlugin * plugin ) :
	m_plugin( plugin )
{
	LilvNode * node;
	LilvPort * port;

	m_numPorts = lilv_plugin_get_num_ports( m_plugin );

	// Compatibility
	m_compatible = true;
	LilvNodes * required = lilv_plugin_get_required_features( m_plugin );
	LILV_FOREACH( nodes, node_iter, required )
	{
		node = const_cast<LilvNode *>( lilv_nodes_get( required, node_iter ) );
		if( !lv2.featureIsSupported( lilv_node_as_uri( node ) ) )
		{
			m_compatible = false;
		}
	}
	lilv_nodes_free( required );

	// Port ranges
	float * mins = new float[numPorts()];
	float * maxs = new float[numPorts()];
	float * defs = new float[numPorts()];
	lilv_plugin_get_port_ranges_float( m_plugin, mins, maxs, defs );

	// Determine correct input and output ports
	for( int i = 0; i < NumPortDesignations; ++i )
	{
		m_portIndex[i] = -1;
	}

	port = const_cast<LilvPort *>( lilv_plugin_get_port_by_designation( m_plugin, lv2.node( lv2_InputPort ), lv2.node( pg_left ) ) );
	if( port )
	{
		m_portIndex[LeftIn] = lilv_port_get_index( m_plugin, port );
	}

	port = const_cast<LilvPort *>( lilv_plugin_get_port_by_designation( m_plugin, lv2.node( lv2_InputPort ), lv2.node( pg_right ) ) );
	if( port )
	{
		m_portIndex[RightIn] = lilv_port_get_index( m_plugin, port );
	}

	port = const_cast<LilvPort *>( lilv_plugin_get_port_by_designation( m_plugin, lv2.node( lv2_OutputPort ), lv2.node( pg_left ) ) );
	if( port )
	{
		m_portIndex[LeftOut] = lilv_port_get_index( m_plugin, port );
	}

	port = const_cast<LilvPort *>( lilv_plugin_get_port_by_designation( m_plugin, lv2.node( lv2_OutputPort ), lv2.node( pg_right ) ) );
	if( port )
	{
		m_portIndex[RightOut] = lilv_port_get_index( m_plugin, port );
	}

	port = const_cast<LilvPort *>( lilv_plugin_get_port_by_designation( m_plugin, lv2.node( lv2_InputPort ), lv2.node( lv2_control ) ) );
	if( port )
	{
		m_portIndex[EventsIn] = lilv_port_get_index( m_plugin, port );
	}

	port = const_cast<LilvPort *>( lilv_plugin_get_port_by_designation( m_plugin, lv2.node( lv2_OutputPort ), lv2.node( lv2_control ) ) );
	if( port )
	{
		m_portIndex[EventsOut] = lilv_port_get_index( m_plugin, port );
	}

	for( uint32_t p = 0; p < numPorts(); ++p )
	{
		Lv2PortDescriptor * portdesc = new Lv2PortDescriptor();
		port = const_cast<LilvPort *>( lilv_plugin_get_port_by_index( m_plugin, p ) );

		portdesc->m_symbol = lilv_node_as_string( lilv_port_get_symbol( m_plugin, port ) );
		node = const_cast<LilvNode *>( lilv_port_get_name( m_plugin, port ) );
		portdesc->m_name = lilv_node_as_string( node );
		free( node );

		// Flow
		if( lilv_port_is_a( m_plugin, port, lv2.node( lv2_InputPort ) ) )
		{
			portdesc->m_flow = FlowInput;
		}
		else if( lilv_port_is_a( m_plugin, port, lv2.node( lv2_OutputPort ) ) )
		{
			portdesc->m_flow = FlowOutput;
		}

		// Type
		if( lilv_port_is_a( m_plugin, port, lv2.node( lv2_ControlPort ) ) )
		{
			portdesc->m_type = TypeControl;
			portdesc->m_minimum = mins[p];
			portdesc->m_maximum = maxs[p];
			portdesc->m_default = defs[p];
		}
		else if( lilv_port_is_a( m_plugin, port, lv2.node( lv2_AudioPort ) ) )
		{
			portdesc->m_type = TypeAudio;
			switch( portdesc->flow() )
			{
				case FlowInput:
					if( portIndex( LeftIn ) == -1 )
					{
						m_portIndex[LeftIn] = p;
					}
					else if( portIndex( RightIn ) == -1 )
					{
						m_portIndex[RightIn] = p;
					}
					break;
				case FlowOutput:
					if( portIndex( LeftOut ) == -1 )
					{
						m_portIndex[LeftOut] = p;
					}
					else if( portIndex( RightOut ) == -1 )
					{
						m_portIndex[RightOut] = p;
					}
					break;
				default:
					m_compatible = false;
					break;
			}
		}
		else if( lilv_port_is_a( m_plugin, port, lv2.node( atom_AtomPort ) ) || lilv_port_is_a( m_plugin, port, lv2.node( ev_EventPort ) ) )
		{
			portdesc->m_type = TypeEvent;
			portdesc->m_evbufType = lilv_port_is_a( m_plugin, port, lv2.node( atom_AtomPort ) ) ? LV2_EVBUF_ATOM : LV2_EVBUF_EVENT;

			switch( portdesc->flow() )
			{
				case FlowInput:
					if( portIndex( EventsIn ) == -1 )
					{
						m_portIndex[EventsIn] = p;
					}
					break;
				case FlowOutput:
					if( portIndex( EventsOut ) == -1 )
					{
						m_portIndex[EventsOut] = p;
					}
					break;
				default:
					m_compatible = false;
					break;
			}
		}

		if( portIndex( LeftIn ) != -1 && portIndex( RightIn ) == -1 )
		{
			m_portIndex[RightIn] = portIndex( LeftIn );
		}

		if( portIndex( LeftOut ) != -1 && portIndex( RightOut ) == -1 )
		{
			m_portIndex[RightOut] = portIndex( LeftOut );
		}

		m_ports.push_back( portdesc );
	}

	if( !m_compatible )
	{
		fprintf( stderr, "incompatible: <%s>\n", uri() );
	}

	delete[] mins;
	delete[] maxs;
	delete[] defs;
}




Lv2PluginDescriptor::~Lv2PluginDescriptor()
{
	for( int i = 0; i < m_ports.size(); ++i )
	{
		delete m_ports[i];
	}
	m_ports.clear();
}

