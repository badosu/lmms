
#include "lmmsconfig.h"

#include "engine.h"
#include "InstrumentTrack.h"
#include "InstrumentPlayHandle.h"
#include "text_float.h"

#include "Lv2Plugin.h"

#include "Lovely.h"
#include "embed.cpp"




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
	m_plugin( NULL )
{
	InstrumentPlayHandle * handle = new InstrumentPlayHandle( this );
	engine::mixer()->addPlayHandle( handle );
	m_plugin = new Lv2Plugin( "http://www.openavproductions.com/sorcer", engine::mixer()->processingSampleRate(), engine::mixer()->framesPerPeriod() );
}




LovelyInstrument::~LovelyInstrument()
{
	engine::mixer()->removePlayHandles( instrumentTrack() );
	delete m_plugin;
}




PluginView * LovelyInstrument::instantiateView( QWidget * parent )
{
	return new LovelyView( this, parent );
}




bool LovelyInstrument::handleMidiEvent( const MidiEvent & ev, const MidiTime & time )
{
	return m_plugin->writeEvent( ev, time );
}




void LovelyInstrument::play( sampleFrame * buffer )
{
	fpp_t nframes = engine::mixer()->framesPerPeriod();
	m_plugin->resizeBuffers( nframes );

	float * left = m_plugin->outputBuffer( 0, false );
	float * right = m_plugin->outputBuffer( 1, true );

	m_plugin->run( nframes );

	for( fpp_t i = 0; i < nframes; ++i )
	{
		buffer[i][0] = left[i];
		buffer[i][1] = right[i];
	}

	instrumentTrack()->processAudioBuffer( buffer, nframes, NULL );
}




#include "moc_Lovely.cxx"
