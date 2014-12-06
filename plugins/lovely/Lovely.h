/*
 * Lovely.h - LV2 instrument host for LMMS
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


#ifndef LOVELY_H
#define LOVELY_H

#include <QtCore/QMutex>

#include <QPair>
#include <QVector>

#include <QListWidget>
#include "ComboBox.h"

#include "Instrument.h"
#include "InstrumentView.h"

#include "Lv2Plugin.h"
#include "Lv2Controls.h"




class LovelyInstrument;
class LovelyView;




typedef union
{
	float floatValue;
	uint32_t intValue;
}
Lv2NotePropertyValue;

typedef QPair<LV2_URID, Lv2NotePropertyValue> Lv2NoteProperty;
typedef QVector<Lv2NoteProperty> Lv2NoteObject;
typedef QPair<f_cnt_t, Lv2NoteObject> Lv2NoteEvent;




class PLUGIN_EXPORT LovelyInstrument : public Instrument
{
	Q_OBJECT
public:
	LovelyInstrument( InstrumentTrack * track );
	virtual ~LovelyInstrument();

	virtual QString nodeName() const {
		return "lovely";
	};

	virtual PluginView * instantiateView( QWidget * parent );

	virtual Flags flags() const { return IsSingleStreamed; }

	virtual void play( sampleFrame * buffer );
	virtual void playNote( NotePlayHandle * n, sampleFrame * );
	virtual void deleteNotePluginData( NotePlayHandle * n );

	void saveSettings( QDomDocument & doc, QDomElement & self );
	void loadSettings( const QDomElement & self );

	void loadPlugin( const char * uri );

	const uint32_t newNoteID();

private:
	Lv2Plugin * m_plugin;
	QMutex m_pluginMutex;

	LV2_Atom_Forge m_forge;
	LV2_Atom_Forge_Frame m_frame;
	QVector<Lv2NoteEvent> m_events;

	QString m_uri;

	LovelyView * m_view;
	uint32_t m_id;

	friend class LovelyView;
};




class PLUGIN_EXPORT LovelyView : public InstrumentView
{
	Q_OBJECT
public:
	LovelyView( Instrument * instrument, QWidget * parent );
	virtual ~LovelyView();

	void findPresets();

public slots:
	void loadFromList( QListWidgetItem * item );
	void loadPreset();

private:
	LovelyInstrument * m_instrument;
	Lv2Controls * m_controls;

	ComboBoxModel m_presetModel;
	ComboBox m_presetList;

	QListWidget m_pluginList;
};




#endif
