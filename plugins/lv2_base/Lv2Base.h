/*
 * Lv2Base.h - basic LV2 support for LMMS
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


#include "Plugin.h"
#include "embed.h"

#include <lilv/lilv.h>

#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/buf-size/buf-size.h>
#include <lv2/lv2plug.in/ns/ext/atom/util.h>
#include <lv2/lv2plug.in/ns/ext/event/event.h>
#include <lv2/lv2plug.in/ns/ext/midi/midi.h>
#include <lv2/lv2plug.in/ns/ext/options/options.h>
#include <lv2/lv2plug.in/ns/ext/parameters/parameters.h>
#include <lv2/lv2plug.in/ns/ext/port-groups/port-groups.h>
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>
#include <lv2/lv2plug.in/ns/lv2core/lv2.h>

#include <QVector>

#include "lv2_evbuf.h"




enum Lv2PortFlow {
	FlowUnknown,
	FlowInput,
	FlowOutput,
};

enum Lv2PortType {
	TypeUnknown,
	TypeControl,
	TypeAudio,
	TypeEvent
};

enum URIs
{
	atom_AtomPort = 1,
	atom_Chunk,
	atom_Float,
	atom_Int,
	atom_Sequence,
	bufsz_maxBlockLength,
	bufsz_minBlockLength,
	bufsz_sequenceSize,
	ev_EventPort,
	lv2_AudioPort,
	lv2_ControlPort,
	lv2_InputPort,
	lv2_OutputPort,
	lv2_control,
	lv2_name,
	midi_MidiEvent,
	param_sampleRate,
	pg_left,
	pg_right,
	rdfs_label,
	urid_map,
	urid_unmap
};

enum PortDesignation
{
	LeftIn,
	RightIn,
	LeftOut,
	RightOut,
	EventsIn,
	EventsOut,
	NumPortDesignations
};



class PLUGIN_EXPORT Lv2PortDescriptor
{
public:
	Lv2PortDescriptor() {}
	~Lv2PortDescriptor() {}

	Lv2PortType type() const { return m_type; }
	Lv2PortFlow flow() const { return m_flow; }

	float minimum() const { return m_minimum; }
	float maximum() const { return m_maximum; }
	float defaultValue() const { return m_default; }

	const char * name() const { return m_name; }
	const char * symbol() const { return m_symbol; }

	LV2_Evbuf_Type evbufType() const { return m_evbufType; }

private:
	Lv2PortType m_type;
	Lv2PortFlow m_flow;

	float m_minimum;
	float m_maximum;
	float m_default;

	const char * m_symbol;
	const char * m_name;

	LV2_Evbuf_Type m_evbufType;

	friend class Lv2Port;
	friend class Lv2PluginDescriptor;
};




class PLUGIN_EXPORT Lv2PluginDescriptor
{
public:
	Lv2PluginDescriptor( const LilvPlugin * plugin );
	~Lv2PluginDescriptor();

	uint32_t numPorts() { return m_numPorts; }
	int32_t portIndex( PortDesignation designation ) { return m_portIndex[designation]; }
	Lv2PortDescriptor * portDescriptor( uint32_t index ) { return m_ports[index]; }

	bool compatible() const { return m_compatible; }

	const char * uri() const { return lilv_node_as_uri( lilv_plugin_get_uri( m_plugin ) ); }
	const char * name() const
	{
		LilvNode * node = lilv_plugin_get_name( m_plugin );
		const char * name = lilv_node_as_string( node );
		lilv_node_free( node );
		return name;
	}

private:
	QVector<Lv2PortDescriptor *> m_ports;
	const LilvPlugin * m_plugin;
	uint32_t m_numPorts;
	int32_t m_portIndex[NumPortDesignations];
	bool m_compatible;

	const char * m_uri;

	friend class Lv2Plugin;
};




class PLUGIN_EXPORT Lv2Base
{
public:
	Lv2Base();
	~Lv2Base();

	LilvWorld * world() const { return s_world; }
	LilvPlugins * plugins() const { return s_plugins; }
	LilvNode * node( URIs id ) { return s_nodeMap[id - 1]; }

	bool featureIsSupported( const char * uri );
	static const LV2_Feature* s_features[];

	Lv2PluginDescriptor * descriptor( const char * uri );

private:
	static QVector<const char *> s_uriMap;
	static QVector<LilvNode *> s_nodeMap;
	static QVector<Lv2PluginDescriptor *> s_descriptors;

	static LV2_URID mapUri( LV2_URID_Map_Handle handle, const char * uri );
	static const char* unmapUri( LV2_URID_Unmap_Handle handle, LV2_URID urid );

	static LilvWorld * s_world;
	static LilvPlugins * s_plugins;
};




Lv2Base * findLv2();
