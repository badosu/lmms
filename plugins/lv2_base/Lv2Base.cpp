/*
 * Lv2Base.cpp - basic LV2 support for LMMS
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


QVector<QString> Lv2Base::s_uriMap;
QVector<LilvNode *> Lv2Base::s_nodeMap;
QVector<Lv2PluginDescriptor *> Lv2Base::s_descriptors;

LilvWorld * Lv2Base::s_world;
LilvPlugins * Lv2Base::s_plugins;

const int32_t Lv2Base::s_sequenceSize = 1024;
float Lv2Base::s_rate;
int32_t Lv2Base::s_nframes;


LV2_URID_Map Lv2Base::urid__map = { NULL, NULL };
LV2_URID_Unmap Lv2Base::urid__unmap = { NULL, NULL };

LV2_Feature Lv2Base::mapFeature = { LV2_URID__map, &Lv2Base::urid__map };
LV2_Feature Lv2Base::unmapFeature = { LV2_URID__unmap, &Lv2Base::urid__unmap };

LV2_Options_Option Lv2Base::options[5] =
{
	{
		LV2_OPTIONS_INSTANCE, 0, param_sampleRate,
		sizeof( float ), atom_Float, &Lv2Base::s_rate
	},
	{
		LV2_OPTIONS_INSTANCE, 0, bufsz_minBlockLength,
		sizeof( int32_t ), atom_Int, &Lv2Base::s_nframes
	},
	{
		LV2_OPTIONS_INSTANCE, 0, bufsz_maxBlockLength,
		sizeof( int32_t ), atom_Int, &Lv2Base::s_nframes
	},
	{
		LV2_OPTIONS_INSTANCE, 0, bufsz_sequenceSize,
		sizeof( int32_t ), atom_Int, &Lv2Base::s_sequenceSize
	},
	{ LV2_OPTIONS_INSTANCE, 0, 0, 0, 0, NULL }
};

LV2_Feature Lv2Base::optionsFeature = { LV2_OPTIONS__options, &Lv2Base::options };

LV2_Feature Lv2Base::bufSizeFeatures[3] =
{
	{ LV2_BUF_SIZE__powerOf2BlockLength, NULL },
	{ LV2_BUF_SIZE__fixedBlockLength, NULL },
	{ LV2_BUF_SIZE__boundedBlockLength, NULL }
};

const LV2_Feature* Lv2Base::s_features[] =
{
	&Lv2Base::mapFeature, &Lv2Base::unmapFeature,
	&Lv2Base::optionsFeature,
	&Lv2Base::bufSizeFeatures[0],
	&Lv2Base::bufSizeFeatures[1],
	&Lv2Base::bufSizeFeatures[2],
	NULL
};




Lv2Base::Lv2Base()
{
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




void Lv2Base::init()
{
	s_world = lilv_world_new();
	lilv_world_load_all( s_world );
	s_plugins = const_cast<LilvPlugins *>( lilv_world_get_all_plugins( s_world ) );

	s_uriMap.push_back( LV2_ATOM__AtomPort );
	s_uriMap.push_back( LV2_ATOM__Chunk );
	s_uriMap.push_back( LV2_ATOM__Double );
	s_uriMap.push_back( LV2_ATOM__Float );
	s_uriMap.push_back( LV2_ATOM__Int );
	s_uriMap.push_back( LV2_ATOM__Long );
	s_uriMap.push_back( LV2_ATOM__Object );
	s_uriMap.push_back( LV2_ATOM__Sequence );
	s_uriMap.push_back( LV2_BUF_SIZE__maxBlockLength );
	s_uriMap.push_back( LV2_BUF_SIZE__minBlockLength );
	s_uriMap.push_back( LV2_BUF_SIZE__sequenceSize );
	s_uriMap.push_back( LV2_CORE__AudioPort );
	s_uriMap.push_back( LV2_CORE__ControlPort );
	s_uriMap.push_back( LV2_CORE__InputPort );
	s_uriMap.push_back( LV2_CORE__InstrumentPlugin );
	s_uriMap.push_back( LV2_CORE__OutputPort );
	s_uriMap.push_back( LV2_CORE__control );
	s_uriMap.push_back( LV2_CORE__name );
	s_uriMap.push_back( LV2_MIDI__MidiEvent );
	s_uriMap.push_back( LV2_NOTE__NoteEvent );
	s_uriMap.push_back( LV2_NOTE__id );
	s_uriMap.push_back( LV2_NOTE__frequency );
	s_uriMap.push_back( LV2_NOTE__gate );
	s_uriMap.push_back( LV2_NOTE__stereoPanning );
	s_uriMap.push_back( LV2_NOTE__velocity );
	s_uriMap.push_back( LV2_PARAMETERS__sampleRate );
	s_uriMap.push_back( LV2_PORT_GROUPS__left );
	s_uriMap.push_back( LV2_PORT_GROUPS__right );
	s_uriMap.push_back( LV2_PRESETS__Preset );
	s_uriMap.push_back( LILV_NS_RDFS "label" );
	s_uriMap.push_back( LV2_URID__map );
	s_uriMap.push_back( LV2_URID__unmap );

	urid__map.map = mapUri;
	urid__unmap.unmap = unmapUri;

	for( int i = 0; i < s_uriMap.size(); ++i )
	{
		LilvNode * node = lilv_new_uri( s_world, s_uriMap[i].toUtf8().constData() );
		s_nodeMap.push_back( node );
	}

	LILV_FOREACH( plugins, i, s_plugins )
	{
		const LilvPlugin * plugin = lilv_plugins_get( s_plugins, i );
		Lv2PluginDescriptor * desc = new Lv2PluginDescriptor( plugin );
		if( desc->isCompatible() )
		{
			s_descriptors.push_back( desc );
		}
		else
		{
			delete desc;
		}
	}
}




LV2_URID Lv2Base::mapUri( LV2_URID_Map_Handle handle, const char * uri )
{
	for( int i = 0; i < s_uriMap.size(); ++i )
	{
		if( !strcmp( uri, s_uriMap[i].toUtf8().constData() ) )
		{
			return i + 1;
		}
	}

	QString qs( uri );
	s_uriMap.push_back( qs );
	return s_uriMap.size();
}




const char* Lv2Base::unmapUri( LV2_URID_Unmap_Handle handle, LV2_URID urid )
{
	if( !urid || urid > s_uriMap.size() )
	{
		return NULL;
	}
	return s_uriMap[urid - 1].toUtf8().constData();
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




Lv2PluginDescriptor * Lv2Base::descriptor( uint32_t index )
{
	return s_descriptors[index];
}




// We need a single instance of this
static Lv2Base lv2Base;

// and a single way to get it
Lv2Base * lv2()
{
	if( !lv2Base.world() )
	{
		lv2Base.init();
	}
	return &lv2Base;
}




//
// LV2PluginDescriptor
// ------------------------------------------------------------------------

#define GET_PORT(PLUGIN, CLASS, DESIGNATION) const_cast<LilvPort *>( \
	lilv_plugin_get_port_by_designation( ( PLUGIN ), \
	lv2()->node( ( CLASS ) ), lv2()->node( ( DESIGNATION ) ) ) );

Lv2PluginDescriptor::Lv2PluginDescriptor( const LilvPlugin * plugin ) :
	m_plugin( plugin )
{
	LilvNode * node;
	LilvPort * port;

	m_numPorts = lilv_plugin_get_num_ports( m_plugin );

	// Compatibility
	m_isCompatible = true;
	LilvNodes * required = lilv_plugin_get_required_features( m_plugin );
	LILV_FOREACH( nodes, node_iter, required )
	{
		node = const_cast<LilvNode *>( lilv_nodes_get( required, node_iter ) );
		if( !lv2()->featureIsSupported( lilv_node_as_uri( node ) ) )
		{
			m_isCompatible = false;
		}
	}
	lilv_nodes_free( required );

	const LilvPluginClass * cls = lilv_plugin_get_class( m_plugin );
	m_isInstrument = lilv_node_equals( lilv_plugin_class_get_uri( cls ), lv2()->node( lv2_InstrumentPlugin ) );

	// Port ranges
	float * minimums = new float[numPorts()];
	float * maximums = new float[numPorts()];
	float * defaults = new float[numPorts()];
	lilv_plugin_get_port_ranges_float( m_plugin, minimums, maximums, defaults );

	// Determine correct input and output ports
	for( int i = 0; i < NumPortDesignations; ++i )
	{
		m_portIndex[i] = -1;
	}

	port = GET_PORT( m_plugin, lv2_InputPort, pg_left );
	if( port )
	{
		m_portIndex[LeftIn] = lilv_port_get_index( m_plugin, port );
	}

	port = GET_PORT( m_plugin, lv2_InputPort, pg_right );
	if( port )
	{
		m_portIndex[RightIn] = lilv_port_get_index( m_plugin, port );
	}

	port = GET_PORT( m_plugin, lv2_OutputPort, pg_left );
	if( port )
	{
		m_portIndex[LeftOut] = lilv_port_get_index( m_plugin, port );
	}

	port = GET_PORT( m_plugin, lv2_OutputPort, pg_right );
	if( port )
	{
		m_portIndex[RightOut] = lilv_port_get_index( m_plugin, port );
	}

	port = GET_PORT( m_plugin, lv2_InputPort, lv2_control );
	if( port )
	{
		m_portIndex[EventsIn] = lilv_port_get_index( m_plugin, port );
	}

	port = GET_PORT( m_plugin, lv2_OutputPort, lv2_control );
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
		if( lilv_port_is_a( m_plugin, port, lv2()->node( lv2_InputPort ) ) )
		{
			portdesc->m_flow = FlowInput;
		}
		else if( lilv_port_is_a( m_plugin, port, lv2()->node( lv2_OutputPort ) ) )
		{
			portdesc->m_flow = FlowOutput;
		}

		// Type
		if( lilv_port_is_a( m_plugin, port, lv2()->node( lv2_ControlPort ) ) )
		{
			portdesc->m_type = TypeControl;
			portdesc->m_minimum = minimums[p];
			portdesc->m_maximum = maximums[p];
			portdesc->m_default = defaults[p];
		}
		else if( lilv_port_is_a( m_plugin, port, lv2()->node( lv2_AudioPort ) ) )
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
					m_isCompatible = false;
					break;
			}
		}
		else if( lilv_port_is_a( m_plugin, port, lv2()->node( atom_AtomPort ) ) )
		{
			portdesc->m_type = TypeEvent;
			if( lilv_port_supports_event( m_plugin, port, lv2()->node( note_NoteEvent ) ) )
			{
				portdesc->m_eventType = EventTypeNote;
			}
			else if( lilv_port_supports_event( m_plugin, port, lv2()->node( midi_MidiEvent ) ) )
			{
				portdesc->m_eventType = EventTypeMidi;
			}
			else
			{
				portdesc->m_eventType = EventTypeUnknown;
				if( m_isInstrument )
				{
					m_isCompatible = false;
				}
			}

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
					m_isCompatible = false;
					break;
			}
		}
		else
		{
			portdesc->m_type = TypeUnknown;
			m_isCompatible = false;
		}

		m_ports.push_back( portdesc );
	}

	if( portIndex( LeftIn ) != -1 && portIndex( RightIn ) == -1 )
	{
		m_portIndex[RightIn] = portIndex( LeftIn );
	}

	if( portIndex( LeftOut ) != -1 && portIndex( RightOut ) == -1 )
	{
		m_portIndex[RightOut] = portIndex( LeftOut );
	}

	// Currently we can't do anything with a plugin that outputs no sound
	if( portIndex( LeftOut ) == -1 )
	{
		m_isCompatible = false;
	}

	delete[] minimums;
	delete[] maximums;
	delete[] defaults;

	findPresets();
}




Lv2PluginDescriptor::~Lv2PluginDescriptor()
{
	for( int i = 0; i < m_ports.size(); ++i )
	{
		delete m_ports[i];
	}
	m_ports.clear();

	for( int i = 0; i < m_presets.size(); ++i )
	{
		delete m_presets[i];
	}
	m_presets.clear();
}




static bool lessThan( Lv2Preset * a, Lv2Preset * b )
{
	// you filthy C coder

	char * aa = const_cast<char *>( a->name() );
	char * bb = const_cast<char *>( b->name() );

	while( *aa == *bb )
	{
		if( *aa == '\0' || *bb == '\0' )
		{
			break;
		}
		++aa;
		++bb;
	}

	return *aa < *bb;
}




void Lv2PluginDescriptor::findPresets()
{
	if( m_presets.size() )
	{
		return;
	}

	LilvNodes * presets = lilv_plugin_get_related( m_plugin, lv2()->node( pset_Preset ) );

	LILV_FOREACH( nodes, i, presets )
	{
		const LilvNode * preset = lilv_nodes_get( presets, i );
		lilv_world_load_resource( lv2()->world(), preset );

		LilvNodes * labels = lilv_world_find_nodes( lv2()->world(), preset, lv2()->node( rdfs_label ), NULL );
		if( labels )
		{
			const LilvNode * label = lilv_nodes_get_first( labels );
			Lv2Preset * pset = new Lv2Preset( lilv_node_duplicate( preset ), strdup( lilv_node_as_string( label ) ) );
			m_presets.push_back( pset );
			lilv_nodes_free( labels );
		}
	}

	lilv_nodes_free( presets );
	qStableSort( m_presets.begin(), m_presets.end(), lessThan );
}
