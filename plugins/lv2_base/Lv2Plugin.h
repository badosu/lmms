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

#include <QVector>
#include <QtGlobal>

#include "MidiEvent.h"
#include "MidiTime.h"
#include "Lv2Base.h"

#include "lv2_evbuf.h"




class Lv2Port
{
public:
	Lv2PortFlow flow() const { return m_descriptor->flow(); }
	Lv2PortType type() const { return m_descriptor->type(); }
	const char * symbol() const { return m_descriptor->symbol(); }
	const char * name() const { return m_descriptor->name(); }

	void setValue( float value ) { m_value = value; }
	float value() const { return m_value; }

	void * buffer();
	void reset();

	bool writeEvent( const MidiEvent& event, const MidiTime& time );

private:
	Lv2PortDescriptor * m_descriptor;

	float m_value;
	LV2_Evbuf * m_evbuf;
	float * m_buffer;

	LilvState * m_state;

	friend class Lv2Plugin;
};




class Lv2Plugin
{
public:
	Lv2Plugin( Lv2PluginDescriptor * descriptor, double rate, fpp_t bufferSize );
	~Lv2Plugin();

	bool instantiate( double rate );
	void createPorts();

	const LilvPlugin * plugin() const { return m_descriptor->plugin(); }
	LilvInstance * instance() const { return m_instance; }

	inline void activate() { lilv_instance_activate( m_instance ); }
	inline void deactivate() { lilv_instance_deactivate( m_instance ); }
	inline void cleanup() { lilv_instance_free( m_instance ); }

	inline bool writeEvent( const MidiEvent& event, const MidiTime& time )
	{
		if( m_descriptor->portIndex( EventsIn ) != -1 )
		{
			return m_ports[m_descriptor->portIndex( EventsIn )].writeEvent( event, time );
		}
		return false;
	}

	uint32_t numPorts() { return m_descriptor->numPorts(); }
	Lv2Port * port( const char * symbol );

	void resizeBuffers( fpp_t newSize );
	void run( const fpp_t nframes );

	float * buffer( PortDesignation designation ) { return static_cast<float *>( m_ports[m_descriptor->portIndex( designation )].buffer() ); }

	void loadState( const char * stateString );
	void saveState();
	const char * stateString() { return m_stateString; }

private:
	Lv2PluginDescriptor * m_descriptor;
	LilvInstance * m_instance;
	QVector<Lv2Port> m_ports;

	fpp_t m_bufferSize;

	LilvState * m_state;
	const char * m_stateString;
};
