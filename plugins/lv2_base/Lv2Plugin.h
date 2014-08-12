/*
 * Lv2Plugin.h - LV2 support for LMMS
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


#ifndef LV2PLUGIN_H
#define LV2PLUGIN_H

#include <QVector>
#include <QtGlobal>

#include "MidiEvent.h"
#include "MidiTime.h"
#include "Lv2Base.h"
#include "Lv2NoteConverter.h"




class Lv2Plugin;




class PLUGIN_EXPORT Lv2Port
{
public:
	Lv2Port();
	~Lv2Port();

	void cleanup();

	Lv2PortFlow flow() const { return m_descriptor->flow(); }
	Lv2PortType type() const { return m_descriptor->type(); }
	Lv2EventType eventType() const { return m_descriptor->eventType(); }

	const char * symbol() const { return m_descriptor->symbol(); }
	const char * name() const { return m_descriptor->name(); }

	void setValue( float value ) { m_value = value; }
	float value() const { return m_value; }

	void * buffer();
	void reset();

	void writeEvent( const f_cnt_t time, const MidiEvent& event );
	void convertEvents();

private:
	Lv2PortDescriptor * m_descriptor;
	Lv2Plugin * m_plugin;

	float m_value;
	float * m_buffer;
	LV2_Atom_Sequence * m_atomSequence;

	Lv2NoteConverter m_noteConverter;

	f_cnt_t m_frame;

	LilvState * m_state;

	friend class Lv2Plugin;
};




class PLUGIN_EXPORT Lv2Plugin
{
public:
	Lv2Plugin( Lv2PluginDescriptor * descriptor, double rate, fpp_t bufferSize );
	~Lv2Plugin();

	bool instantiate( double rate );
	void createPorts();

	const LilvPlugin * plugin() const { return m_descriptor->plugin(); }
	LilvInstance * instance() const { return m_instance; }
	Lv2PluginDescriptor * descriptor() const { return m_descriptor; }

	inline void activate() { lilv_instance_activate( m_instance ); }
	inline void deactivate() { lilv_instance_deactivate( m_instance ); }
	inline void cleanup() { lilv_instance_free( m_instance ); }

	inline void writeEvent( const f_cnt_t time, const MidiEvent& event )
	{
		if( m_descriptor->portIndex( EventsIn ) != -1 )
		{
			m_ports[m_descriptor->portIndex( EventsIn )].writeEvent( time, event );
		}
	}

	uint32_t numPorts() { return m_descriptor->numPorts(); }
	Lv2Port * port( const char * symbol );

	void resizeBuffers( fpp_t newSize );
	void run( const fpp_t nframes );

	void * buffer( PortDesignation designation ) { return m_ports[m_descriptor->portIndex( designation )].buffer(); }

	void loadState( const char * stateString );
	void saveState();
	const char * stateString() { return m_stateString; }

	void loadPreset( int index );

	void setBaseVelocity( uint8_t value ) { m_baseVelocity = value; }
	const uint8_t baseVelocity() const { return m_baseVelocity; }

private:
	Lv2PluginDescriptor * m_descriptor;
	LilvInstance * m_instance;
	QVector<Lv2Port> m_ports;

	uint8_t m_baseVelocity;

	fpp_t m_bufferSize;

	const char * m_stateString;
};

#endif
