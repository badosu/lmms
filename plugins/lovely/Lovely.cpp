
#include "lmmsconfig.h"

#include "engine.h"
#include "InstrumentTrack.h"
#include "InstrumentPlayHandle.h"
#include "text_float.h"

#include "Lv2Plugin.h"

#include "Lovely.h"
#include "embed.cpp"




static Lv2Base * lv2 = findLv2();




extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT lovely_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"Lovely",
	QT_TRANSLATE_NOOP( "pluginBrowser", "Host for LV2 instruments" ),
	"Hannu Haahti <grejppi/at/gmail/dot/com>",
	0x0001,
	Plugin::Instrument,
	new PluginPixmapLoader( "logo" ),
	NULL,
	NULL,
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
	if( m_plugin )
	{
		delete m_plugin;
	}

	Lv2PluginDescriptor * descriptor = lv2->descriptor( self.attribute( "uri" ).toUtf8().constData() );
	if( !descriptor )
	{
		fprintf( stderr, "Can't find <%s>\n", self.attribute( "uri" ).toUtf8().constData() );
	}
	else
	{
		m_plugin = new Lv2Plugin( descriptor, engine::mixer()->processingSampleRate(), engine::mixer()->framesPerPeriod() );
	}

	m_uri = self.attribute( "uri" );
	m_pluginMutex.unlock();
}




LovelyView::LovelyView( Instrument * instrument, QWidget * parent ) :
	InstrumentView( instrument, parent )
{
	m_instrument = static_cast<LovelyInstrument *>( instrument );
}




LovelyView::~LovelyView()
{
}




#include "moc_Lovely.cxx"
