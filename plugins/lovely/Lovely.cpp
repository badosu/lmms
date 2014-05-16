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
	m_pluginMutex()
{
	InstrumentPlayHandle * handle = new InstrumentPlayHandle( this );
	engine::mixer()->addPlayHandle( handle );
}




LovelyInstrument::~LovelyInstrument()
{
	engine::mixer()->removePlayHandles( instrumentTrack() );
	if( m_plugin )
	{
		delete m_plugin;
	}
}




PluginView * LovelyInstrument::instantiateView( QWidget * parent )
{
	return new LovelyView( this, parent );
}




bool LovelyInstrument::handleMidiEvent( const MidiEvent & ev, const MidiTime & time )
{
	bool r = true;

	m_pluginMutex.lock();
	if( m_plugin && m_plugin->valid() )
	{
		r = m_plugin->writeEvent( ev, time );
	}
	m_pluginMutex.unlock();

	return r;
}




void LovelyInstrument::play( sampleFrame * buffer )
{
	m_pluginMutex.lock();
	if( !m_plugin || !m_plugin->valid() )
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




void LovelyInstrument::saveSettings( QDomDocument & doc, QDomElement & self )
{
	self.setAttribute( "uri", m_uri );
}




void LovelyInstrument::loadSettings( const QDomElement & self )
{
	m_pluginMutex.lock();

	loadPlugin( self.attribute( "uri" ).toUtf8().constData() );
	m_uri = self.attribute( "uri" );

	m_pluginMutex.unlock();
}




void LovelyInstrument::loadPlugin( const char * uri )
{
	if( m_plugin )
	{
		delete m_plugin;
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
	m_uri = uri;
}




LovelyView::LovelyView( Instrument * instrument, QWidget * parent ) :
	InstrumentView( instrument, parent ),
	m_listWidget( this )
{
	m_instrument = static_cast<LovelyInstrument *>( instrument );
	//~ m_listWidget.setFixedWidth( 200 );
	m_listWidget.setFixedHeight( 200 );
	connect( &m_listWidget, SIGNAL( itemDoubleClicked(QListWidgetItem*) ), this, SLOT( loadFromList(QListWidgetItem*) ) );

	Plugin::Descriptor::SubPluginFeatures::KeyList kl;
	m_instrument->descriptor()->subPluginFeatures->listSubPluginKeys( instrument->descriptor(), kl );
	for( unsigned i = 0; i < kl.size(); ++i )
	{
		m_listWidget.addItem( kl[i].name );
	}
}




LovelyView::~LovelyView()
{
}




void LovelyView::loadFromList( QListWidgetItem * item )
{
	m_instrument->m_pluginMutex.lock();

	Plugin::Descriptor::SubPluginFeatures::KeyList kl;
	m_instrument->descriptor()->subPluginFeatures->listSubPluginKeys( m_instrument->descriptor(), kl );
	m_instrument->loadPlugin( kl[m_listWidget.currentRow()].attributes["uri"].toUtf8().constData() );
	m_instrument->m_pluginMutex.unlock();
}




#include "moc_Lovely.cxx"
