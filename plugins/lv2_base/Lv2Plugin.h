/*
 * Lv2Plugin.h - LV2 support for LMMS
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

#include <vector>
#include <QtGlobal>

#include "MidiEvent.h"
#include "MidiTime.h"

#include "lv2_evbuf.h"



class Lv2Port
{
public:
	enum Flow {
		FlowUnknown,
		FlowInput,
		FlowOutput,
	};

	enum Type {
		TypeUnknown,
		TypeControl,
		TypeAudio,
		TypeEvent
	};

	Flow flow() const
	{
		return m_flow;
	}

	Type type() const
	{
		return m_type;
	}

	const char * symbol() const
	{
		return m_symbol;
	}

	const char * name() const
	{
		return m_name;
	}

	void setValue( float value )
	{
		m_value = value;
	}

	float value() const
	{
		return m_value;
	}

	void * buffer();
	void reset();

	bool writeEvent( const MidiEvent& event, const MidiTime& time );

private:
	Flow m_flow;
	Type m_type;
	const char * m_symbol;
	const char * m_name;

	float m_minimum;
	float m_maximum;
	float m_default;
	float m_value;

	LV2_Evbuf * m_evbuf;
	float * m_buffer;

	friend class Lv2Plugin;
};




class Lv2Plugin
{
public:
	Lv2Plugin( const char * uri, double rate, fpp_t bufferSize );
	~Lv2Plugin();

	const std::vector<const char *> pluginUris()
	{
		return s_pluginUris;
	}

	inline bool valid()
	{
		return !!m_instance;
	}

	bool instantiate( double rate );
	void createPorts();

	inline void activate()
	{
		lilv_instance_activate( m_instance );
	}

	inline void deactivate()
	{
		lilv_instance_deactivate( m_instance );
	}

	inline void cleanup()
	{
		lilv_instance_free( m_instance );
	}

	inline bool writeEvent( const MidiEvent& event, const MidiTime& time )
	{
		if( m_midiIn != -1 )
		{
			return m_ports[m_midiIn].writeEvent( event, time );
		}
		return false;
	}

	inline uint32_t numPorts()
	{
		if( m_plugin )
		{
			return lilv_plugin_get_num_ports( m_plugin );
		}
		return 0;
	}

	inline float * inputBuffer( int index, bool wrap )
	{
		int i = wrap ? index % qMax( m_inputs.size(), 1ul ) : index;
		if( m_inputs[i] )
		{
			return static_cast<float *>( m_ports[m_inputs[i]].buffer() );
		}
		return NULL;
	}

	inline float * outputBuffer( int index, bool wrap )
	{
		int i = wrap ? index % qMax( m_outputs.size(), 1ul ) : index;
		if( m_outputs[i] )
		{
			return static_cast<float *>( m_ports[m_outputs[i]].buffer() );
		}
		return NULL;
	}

	void resizeBuffers( fpp_t newSize );
	void run( const fpp_t nframes );

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

private:
	static std::vector<const char *> s_uriMap;
	static std::vector<LilvNode *> s_nodeMap;
	static std::vector<const char *> s_pluginUris;

	static LV2_URID mapUri( LV2_URID_Map_Handle handle, const char * uri );
	static const char* unmapUri( LV2_URID_Unmap_Handle handle, LV2_URID urid );

	bool featureIsSupported( const char * uri );

	static LilvWorld * s_world;
	static LilvPlugins * s_plugins;
	static long s_refcount;

	LilvPlugin * m_plugin;
	LilvInstance * m_instance;
	std::vector<Lv2Port> m_ports;

	fpp_t m_bufferSize;

	uint32_t m_midiIn;
	std::vector<uint32_t> m_inputs;
	std::vector<uint32_t> m_outputs;

};
