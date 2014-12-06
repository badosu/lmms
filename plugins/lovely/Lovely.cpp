/*
 * Lovely.cpp - LV2 instrument host for LMMS
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


#include "lmmsconfig.h"

#include "Engine.h"
#include "Song.h"
#include "InstrumentTrack.h"
#include "InstrumentPlayHandle.h"
#include "TextFloat.h"

#include "Lv2NoteHandle.h"

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
	m_view( NULL ),
	m_id( 0 )
{
	InstrumentPlayHandle * handle = new InstrumentPlayHandle( this, track );
	Engine::mixer()->addPlayHandle( handle );
	lv2_atom_forge_init( &m_forge, &lv2()->urid__map );
}




LovelyInstrument::~LovelyInstrument()
{
	if( m_plugin )
	{
		delete m_plugin;
		m_plugin = NULL;
	}
	Engine::mixer()->removePlayHandles( instrumentTrack() );
}




PluginView * LovelyInstrument::instantiateView( QWidget * parent )
{
	return new LovelyView( this, parent );
}




static bool lessThan( Lv2NoteEvent & a, Lv2NoteEvent & b )
{
	return a.first < b.first;
}




void LovelyInstrument::play( sampleFrame * buffer )
{
	m_pluginMutex.lock();
	if( !m_plugin || !m_plugin->instance() )
	{
		m_pluginMutex.unlock();
		return;
	}

	m_plugin->setBaseVelocity( instrumentTrack()->midiPort()->baseVelocity() );
	const fpp_t nframes = Engine::mixer()->framesPerPeriod();
	m_plugin->resizeBuffers( nframes );

	LV2_Atom_Forge_Frame frame;
	LV2_Atom_Sequence * atomSequence = static_cast<LV2_Atom_Sequence *>( m_plugin->buffer( AtomIn ) );

	lv2_atom_sequence_clear( atomSequence );
	memset( LV2_ATOM_CONTENTS( LV2_Atom_Sequence, atomSequence ), 0, lv2()->s_sequenceSize );

	lv2_atom_forge_set_buffer( &m_forge, reinterpret_cast<uint8_t *>( atomSequence ), lv2()->s_sequenceSize );
	lv2_atom_forge_sequence_head( &m_forge, &m_frame, 0 );

	qSort( m_events.begin(), m_events.end(), lessThan );
	for( int i = 0; i < m_events.size(); ++i )
	{
		Lv2NoteEvent * ev = &m_events[i];

		lv2_atom_forge_frame_time( &m_forge, ev->first );
		lv2_atom_forge_object( &m_forge, &frame, 0, note_NoteEvent );

		for( int p = 0; p < ev->second.size(); ++p )
		{
			Lv2NoteProperty * prop = &ev->second[p];
			lv2_atom_forge_key( &m_forge, prop->first );
			switch( prop->first )
			{
				case note_id:
					lv2_atom_forge_int( &m_forge, prop->second.intValue );
					break;
				case note_gate:
					lv2_atom_forge_bool( &m_forge, prop->second.intValue );
					break;
				case note_frequency:
				case note_velocity:
				case note_stereoPanning:
				default:
					lv2_atom_forge_float( &m_forge, prop->second.floatValue );
					break;
			}
		}
		lv2_atom_forge_pop( &m_forge, &frame );
	}
	lv2_atom_forge_pop( &m_forge, &m_frame );
	m_events.clear();

	float * left = static_cast<float *>( m_plugin->buffer( LeftOut ) );
	float * right = static_cast<float *>( m_plugin->buffer( RightOut ) );

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
	if( !m_plugin )
	{
		return;
	}

	m_pluginMutex.lock();

	Lv2NoteHandle * nh;
	Lv2NoteObject object;

	Lv2NoteObject ev;

	Lv2NotePropertyValue pvID;
	Lv2NotePropertyValue pvGate;
	Lv2NotePropertyValue pvFrequency;
	Lv2NotePropertyValue pvVelocity;
	Lv2NotePropertyValue pvPanning;
	pvID.intValue = 0;

	if( n->totalFramesPlayed() == 0 || n->m_pluginData == NULL )
	{
		uint32_t atomIn = m_plugin->descriptor()->portIndex( AtomIn );
		Lv2PortDescriptor * eventPort = m_plugin->descriptor()->portDescriptor( atomIn );

		float frequency = ( eventPort->eventType() == EventTypeNote ) ? n->frequency() : n->unpitchedFrequency();

		nh = new Lv2NoteHandle( frequency / 2.0f, n->getVolume() / 100.0f, n->getPanning() / 100.0f, newNoteID() );
		n->m_pluginData = static_cast<void *>( nh );

		pvID.intValue = nh->id();
		ev.push_back( Lv2NoteProperty( note_id, pvID ) );

		pvGate.intValue = 1;
		ev.push_back( Lv2NoteProperty( note_gate, pvGate ) );

		pvFrequency.floatValue = nh->frequency();
		ev.push_back( Lv2NoteProperty( note_frequency, pvFrequency ) );

		pvVelocity.floatValue = nh->velocity();
		ev.push_back( Lv2NoteProperty( note_velocity, pvVelocity ) );

		pvPanning.floatValue = nh->panning();
		ev.push_back( Lv2NoteProperty( note_stereoPanning, pvPanning ) );
	}
	else
	{
		nh = static_cast<Lv2NoteHandle *>( n->m_pluginData );

		nh->setFrequency( n->frequency() / 2.0f );
		nh->setVelocity( n->getVolume() / 100.0f );
		nh->setPanning( n->getPanning() / 100.0f );

		float * frequency = NULL;
		float * velocity = NULL;
		float * panning = NULL;

		nh->query( &frequency, &velocity, &panning );

		if( n->isReleased() )
		{
			if( nh->gate() )
			{
				pvID.intValue = nh->id();
				ev.push_back( Lv2NoteProperty( note_id, pvID ) );

				nh->setGate( false );
				pvGate.intValue = 0;
				ev.push_back( Lv2NoteProperty( note_gate, pvGate ) );
			}
		}
		else if( frequency || velocity || panning )
		{
			pvID.intValue = nh->id();
			ev.push_back( Lv2NoteProperty( note_id, pvID ) );

			if( frequency )
			{
				pvFrequency.floatValue = *frequency;
				ev.push_back( Lv2NoteProperty( note_frequency, pvFrequency ) );
			}
			if( velocity )
			{
				pvVelocity.floatValue = *velocity;
				ev.push_back( Lv2NoteProperty( note_velocity, pvVelocity ) );
			}
			if( panning )
			{
				pvPanning.floatValue = *panning;
				ev.push_back( Lv2NoteProperty( note_stereoPanning, pvPanning ) );
			}
		}
	}

	if( pvID.intValue )
	{
		m_events.push_back( Lv2NoteEvent( n->noteOffset(), ev ) );
	}

	m_pluginMutex.unlock();
}




void LovelyInstrument::deleteNotePluginData( NotePlayHandle * n )
{
	m_pluginMutex.lock();
	Lv2NoteHandle * nh = static_cast<Lv2NoteHandle *>( n->m_pluginData );
	if( nh->gate() )
	{
		Lv2NoteObject object;
		Lv2NoteObject ev;
		Lv2NotePropertyValue pvID;
		Lv2NotePropertyValue pvGate;

		pvID.intValue = nh->id();
		ev.push_back( Lv2NoteProperty( note_id, pvID ) );

		pvGate.intValue = 0;
		ev.push_back( Lv2NoteProperty( note_gate, pvGate ) );

		m_events.push_back( Lv2NoteEvent( n->offset(), ev ) );
	}
	delete nh;
	m_pluginMutex.unlock();
}




void LovelyInstrument::saveSettings( QDomDocument & doc, QDomElement & self )
{
	if( !m_plugin || !m_plugin->instance() )
	{
		return;
	}

	self.setAttribute( "uri", m_uri );
	m_plugin->saveState();

	QDomCDATASection cdata = doc.createCDATASection( m_plugin->stateString() );
	QDomElement state = doc.createElement( "state" );
	state.appendChild( cdata );
	self.appendChild( state );
}




void LovelyInstrument::loadSettings( const QDomElement & self )
{
	Engine::mixer()->removePlayHandles( instrumentTrack(), false );
	m_pluginMutex.lock();

	loadPlugin( self.attribute( "uri" ).toUtf8().constData() );
	m_uri = self.attribute( "uri" );

	QDomElement state = self.firstChildElement( "state" );
	if( !state.isNull() )
	{
		m_plugin->loadState( state.text().toUtf8().constData() );
	}
	else if( self.hasAttribute( "state" ) )
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
		m_plugin = new Lv2Plugin( descriptor, Engine::mixer()->processingSampleRate(), Engine::mixer()->framesPerPeriod() );
		m_plugin->run( Engine::mixer()->framesPerPeriod() );
	}
	m_uri = QString( uri );

	if( m_view )
	{
		m_view->findPresets();
	}
}




const uint32_t LovelyInstrument::newNoteID()
{
	do
	{
		++m_id;
	}
	while( m_id == 0 );
	return m_id;
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

	if( !m_instrument->m_plugin )
	{
		return;
	}

	m_presetModel.addItem( m_instrument->m_plugin->descriptor()->name(), NULL );
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
