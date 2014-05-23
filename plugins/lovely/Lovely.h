/*
 * Lovely.h - LV2 instrument host for LMMS
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


#ifndef LOVELY_H
#define LOVELY_H

#include <QtCore/QMutex>

#include <QListWidget>
#include "combobox.h"

#include "Instrument.h"
#include "InstrumentView.h"




class LovelyInstrument;
class LovelyView;




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

	virtual Flags flags() const { return IsSingleStreamed | IsMidiBased; }

	//~ virtual bool handleMidiEvent( const MidiEvent & event, const MidiTime & time );
	virtual void play( sampleFrame * buffer );
	virtual void playNote( NotePlayHandle * n, sampleFrame * );
	virtual void deleteNotePluginData( NotePlayHandle * n );

	void saveSettings( QDomDocument & doc, QDomElement & self );
	void loadSettings( const QDomElement & self );

	void loadPlugin( const char * uri );

private:
	Lv2Plugin * m_plugin;
	QMutex m_pluginMutex;

	QString m_uri;

	LovelyView * m_view;

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

	ComboBoxModel m_presetModel;
	comboBox m_presetList;

	QListWidget m_pluginList;
};

#endif
