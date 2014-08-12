/*
 * Lv2Base.h - basic LV2 support for LMMS
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


#ifndef LV2BASE_H
#define LV2BASE_H

#include "Plugin.h"
#include "embed.h"

#include <lilv/lilv.h>

#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>
#include <lv2/lv2plug.in/ns/ext/atom/util.h>
#include <lv2/lv2plug.in/ns/ext/buf-size/buf-size.h>
#include <lv2/lv2plug.in/ns/ext/midi/midi.h>
#include <lv2/lv2plug.in/ns/ext/options/options.h>
#include <lv2/lv2plug.in/ns/ext/parameters/parameters.h>
#include <lv2/lv2plug.in/ns/ext/port-groups/port-groups.h>
#include <lv2/lv2plug.in/ns/ext/presets/presets.h>
#include <lv2/lv2plug.in/ns/ext/state/state.h>
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>
#include <lv2/lv2plug.in/ns/lv2core/lv2.h>

#include "ext/note/note.h"

#include <QVector>
#include <QString>




enum Lv2PortFlow
{
	FlowUnknown,
	FlowInput,
	FlowOutput
};

enum Lv2PortType
{
	TypeUnknown,
	TypeControl,
	TypeAudio,
	TypeEvent
};

enum Lv2EventType
{
	EventTypeUnknown,
	EventTypeMidi,
	EventTypeNote
};

enum URIs
{
	atom_AtomPort = 1,
	atom_Chunk,
	atom_Double,
	atom_Float,
	atom_Int,
	atom_Long,
	atom_Object,
	atom_Sequence,
	bufsz_maxBlockLength,
	bufsz_minBlockLength,
	bufsz_sequenceSize,
	lv2_AudioPort,
	lv2_ControlPort,
	lv2_InputPort,
	lv2_InstrumentPlugin,
	lv2_OutputPort,
	lv2_control,
	lv2_name,
	midi_MidiEvent,
	note_NoteEvent,
	note_id,
	note_frequency,
	note_gate,
	note_stereoPanning,
	note_velocity,
	param_sampleRate,
	pg_left,
	pg_right,
	pset_Preset,
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
	Lv2EventType eventType() const { return m_eventType; }

	float minimum() const { return m_minimum; }
	float maximum() const { return m_maximum; }
	float defaultValue() const { return m_default; }

	const char * name() const { return m_name; }
	const char * symbol() const { return m_symbol; }

private:
	Lv2PortType m_type;
	Lv2PortFlow m_flow;
	Lv2EventType m_eventType;


	float m_minimum;
	float m_maximum;
	float m_default;

	const char * m_symbol;
	const char * m_name;

	friend class Lv2Port;
	friend class Lv2PluginDescriptor;
};




class PLUGIN_EXPORT Lv2Preset
{
public:
	Lv2Preset( const LilvNode * node, const char * name ) : m_node( node ), m_name( name ) {}
	~Lv2Preset() {}

	const LilvNode * node() { return m_node; }
	const char * name() { return m_name; }

private:
	const LilvNode * m_node;
	const char * m_name;
};




class PLUGIN_EXPORT Lv2PluginDescriptor
{
public:
	Lv2PluginDescriptor( const LilvPlugin * plugin );
	~Lv2PluginDescriptor();

	const LilvPlugin * plugin() const { return m_plugin; }

	uint32_t numPorts() { return m_numPorts; }
	int32_t portIndex( PortDesignation designation ) { return m_portIndex[designation]; }
	Lv2PortDescriptor * portDescriptor( uint32_t index ) { return m_ports[index]; }

	bool isCompatible() const { return m_isCompatible; }
	bool isInstrument() const { return m_isInstrument; }

	const char * uri() const { return lilv_node_as_uri( lilv_plugin_get_uri( m_plugin ) ); }
	const char * name() const
	{
		LilvNode * node = lilv_plugin_get_name( m_plugin );
		const char * name = lilv_node_as_string( node );
		lilv_node_free( node );
		return name;
	}

	int numPresets() const { return m_presets.size(); };
	Lv2Preset * preset( int index ) { return m_presets[index]; }
	void findPresets();

private:
	const LilvPlugin * m_plugin;
	

	QVector<Lv2PortDescriptor *> m_ports;
	uint32_t m_numPorts;
	int32_t m_portIndex[NumPortDesignations];

	QVector<Lv2Preset *> m_presets;

	bool m_isCompatible;
	bool m_isInstrument;

	const char * m_uri;

	friend class Lv2Plugin;
};




class PLUGIN_EXPORT Lv2Base
{
public:
	Lv2Base();
	~Lv2Base();

	void init();

	LilvWorld * world() const { return s_world; }
	LilvPlugins * plugins() const { return s_plugins; }
	LilvNode * node( URIs id ) { return s_nodeMap[id - 1]; }

	bool featureIsSupported( const char * uri );

	static LV2_URID_Map urid__map;
	static LV2_URID_Unmap urid__unmap;

	static LV2_Feature mapFeature;
	static LV2_Feature unmapFeature;

	static LV2_Options_Option options[5];
	static LV2_Feature optionsFeature;

	static LV2_Feature bufSizeFeatures[3];

	static const LV2_Feature* s_features[];

	Lv2PluginDescriptor * descriptor( const char * uri );
	Lv2PluginDescriptor * descriptor( uint32_t index );
	uint32_t numberOfPlugins() const { return s_descriptors.size(); }

	static LV2_URID mapUri( LV2_URID_Map_Handle handle, const char * uri );
	static const char* unmapUri( LV2_URID_Unmap_Handle handle, LV2_URID urid );

	static const int32_t s_sequenceSize;

	static void setRate( float rate ) { s_rate = rate; }
	static void setBufferSize( int32_t nframes ) { s_nframes = nframes; }

private:
	static QVector<QString> s_uriMap;
	static QVector<LilvNode *> s_nodeMap;
	static QVector<Lv2PluginDescriptor *> s_descriptors;

	static LilvWorld * s_world;
	static LilvPlugins * s_plugins;

	static float s_rate;
	static int32_t s_nframes;
};




Lv2Base * lv2();

#endif
