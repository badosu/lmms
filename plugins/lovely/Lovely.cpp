/*
 * Lovely.cpp - LV2 instrument host for LMMS
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

#include "engine.h"
#include "song.h"
#include "InstrumentTrack.h"
#include "InstrumentPlayHandle.h"
#include "text_float.h"

#include "Lv2Plugin.h"

#include "Lovely.h"
#include "LovelySubPluginFeatures.h"

#include "embed.cpp"




extern "C"
{


static PluginPixmapLoader logo( "logo" );
static LovelySubPluginFeatures subPluginFeatures( Plugin::Instrument );


Plugin::Descriptor PLUGIN_EXPORT lovely_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"Lovely",
	QT_TRANSLATE_NOOP( "pluginBrowser", "Host for LV2 instruments" ),
	"Hannu Haahti <grejppi/at/gmail/dot/com>",
	0x0001,
	Plugin::Instrument,
	&logo,
	NULL,
	&subPluginFeatures
};


Plugin * PLUGIN_EXPORT lmms_plugin_main( Model *, void * data )
{
	return new LovelyInstrument( static_cast<InstrumentTrack *>( data ) );
}

}




//~ QPixmap * LovelyView::s_artwork = NULL;




LovelyInstrument::LovelyInstrument( InstrumentTrack * track ) :
	Instrument( track, &lovely_plugin_descriptor ),
	m_plugin( NULL ),
	m_pluginMutex(),
	m_view( NULL )
{
	InstrumentPlayHandle * handle = new InstrumentPlayHandle( this );
	engine::mixer()->addPlayHandle( handle );
}




LovelyInstrument::~LovelyInstrument()
{
	if( m_plugin )
	{
		delete m_plugin;
		m_plugin = NULL;
	}
	engine::mixer()->removePlayHandles( instrumentTrack() );
}




PluginView * LovelyInstrument::instantiateView( QWidget * parent )
{
	return new LovelyView( this, parent );
}




/* bool LovelyInstrument::handleMidiEvent( const MidiEvent & ev, const MidiTime & time )
{
	bool r = true;

	m_pluginMutex.lock();
	if( m_plugin && m_plugin->instance() )
	{
		r = m_plugin->writeEvent( time, ev );
	}
	m_pluginMutex.unlock();

	return r;
} */




void LovelyInstrument::play( sampleFrame * buffer )
{
	m_pluginMutex.lock();
	if( !m_plugin || !m_plugin->instance() )
	{
		m_pluginMutex.unlock();
		return;
	}

	fpp_t nframes = engine::mixer()->framesPerPeriod();
	m_plugin->resizeBuffers( nframes );

	float * left = m_plugin->buffer( LeftOut );
	float * right = m_plugin->buffer( RightOut );

	m_plugin->run( nframes );

	for( fpp_t i = 0; i < nframes; ++i )
	{
		buffer[i][0] = left[i];
		buffer[i][1] = right[i];
	}

	instrumentTrack()->processAudioBuffer( buffer, nframes, NULL );
	m_pluginMutex.unlock();
}




void LovelyInstrument::playNote( NotePlayHandle * n, sampleFrame * )
{
	m_pluginMutex.lock();
	if( n->totalFramesPlayed() )
	{
		m_pluginMutex.unlock();
		return;
	}

	int * key = new int( instrumentTrack()->masterKey( n->midiKey() ) );
	n->m_pluginData = static_cast<void *>( key );

	if( m_plugin && m_plugin->instance() )
	{
		MidiEvent ev( MidiNoteOn );
		ev.setKey( *key );
		ev.setVelocity( n->volumeLevel( 0 ) * instrumentTrack()->midiPort()->baseVelocity() );
		m_plugin->writeEvent( n->offset(), ev );
	}
	m_pluginMutex.unlock();
}




void LovelyInstrument::deleteNotePluginData( NotePlayHandle * n )
{
	m_pluginMutex.lock();
	int * key = static_cast<int *>( n->m_pluginData );
	if( m_plugin && m_plugin->instance() )
	{
		MidiEvent ev( MidiNoteOff );
		ev.setKey( *key );
		m_plugin->writeEvent( 0, ev );
	}
	delete key;
	m_pluginMutex.unlock();
}




void LovelyInstrument::saveSettings( QDomDocument & doc, QDomElement & self )
{
	self.setAttribute( "uri", m_uri );
	m_plugin->saveState();
	self.setAttribute( "state", QString( m_plugin->stateString() ) );
}




void LovelyInstrument::loadSettings( const QDomElement & self )
{
	m_pluginMutex.lock();

	loadPlugin( self.attribute( "uri" ).toUtf8().constData() );
	m_uri = self.attribute( "uri" );

	if( self.hasAttribute( "state" ) )
    {
		m_plugin->loadState( self.attribute( "state" ).toUtf8().constData() );
	}

	m_pluginMutex.unlock();
}




void LovelyInstrument::loadPlugin( const char * uri )
{
	if( m_plugin )
	{
		delete m_plugin;
		m_plugin = NULL;
	}

	Lv2PluginDescriptor * descriptor = lv2()->descriptor( uri );
	if( !descriptor )
	{
		fprintf( stderr, "Can't find <%s>\n", uri );
	}
	else
	{
		m_plugin = new Lv2Plugin( descriptor, engine::mixer()->processingSampleRate(), engine::mixer()->framesPerPeriod() );
		m_plugin->run( engine::mixer()->framesPerPeriod() );
	}
	m_uri = QString( uri );

	if( m_view )
	{
		m_view->findPresets();
	}
}




LovelyView::LovelyView( Instrument * instrument, QWidget * parent ) :
	InstrumentView( instrument, parent ),
	m_instrument( static_cast<LovelyInstrument *>( instrument ) ),
	m_presetModel( NULL, "Preset" ),
	m_presetList( this, "Preset" ),
	m_pluginList( this )
{
	m_pluginList.setFixedHeight( 200 );
	connect( &m_pluginList, SIGNAL( itemDoubleClicked(QListWidgetItem*) ), this, SLOT( loadFromList(QListWidgetItem*) ) );

	Plugin::Descriptor::SubPluginFeatures::KeyList kl;
	m_instrument->descriptor()->subPluginFeatures->listSubPluginKeys( instrument->descriptor(), kl );
	for( unsigned i = 0; i < kl.size(); ++i )
	{
		m_pluginList.addItem( kl[i].name );
	}
	m_instrument->m_view = this;

	connect( &m_presetModel, SIGNAL( dataChanged() ), this, SLOT( loadPreset() ) );
	m_presetList.setModel( &m_presetModel );
	m_presetList.move( 16, 216 );
	m_presetList.setFixedWidth( 224 );
	findPresets();
}




LovelyView::~LovelyView()
{
	m_instrument->m_view = NULL;
}




void LovelyView::findPresets()
{
	m_presetModel.clear();
	m_presetModel.addItem( "None", NULL );

	if( !m_instrument->m_plugin )
	{
		return;
	}

	for( int i = 0; i < m_instrument->m_plugin->descriptor()->numPresets(); ++i )
	{
		m_presetModel.addItem( m_instrument->m_plugin->descriptor()->preset( i )->name(), NULL );
	}
}




void LovelyView::loadFromList( QListWidgetItem * item )
{
	m_instrument->m_pluginMutex.lock();

	Plugin::Descriptor::SubPluginFeatures::KeyList kl;
	m_instrument->descriptor()->subPluginFeatures->listSubPluginKeys( m_instrument->descriptor(), kl );
	m_instrument->loadPlugin( kl[m_pluginList.currentRow()].attributes["uri"].toUtf8().constData() );

	m_instrument->m_pluginMutex.unlock();
}




void LovelyView::loadPreset()
{
	if( !m_presetModel.value() )
	{
		return;
	}

	m_instrument->m_pluginMutex.lock();
	m_instrument->m_plugin->loadPreset( m_presetModel.value() - 1 );
	m_instrument->m_pluginMutex.unlock();
}




#include "moc_Lovely.cxx"
