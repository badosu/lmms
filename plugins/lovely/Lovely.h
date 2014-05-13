
#ifndef LOVELY_H
#define LOVELY_H

#include "Instrument.h"
#include "InstrumentView.h"




class LovelyInstrument : public Instrument
{
	Q_OBJECT
public:
	LovelyInstrument( InstrumentTrack * track );
	virtual ~LovelyInstrument();

	virtual QString nodeName() const {
		return "lovely";
	};
	virtual PluginView * instantiateView( QWidget * parent );

	inline virtual bool isMidiBased() const
	{
		return true;
	}

	virtual bool handleMidiEvent( const MidiEvent & event, const MidiTime & time );
	virtual void play( sampleFrame * buffer );

	void saveSettings( QDomDocument & doc, QDomElement & self ) {}
	void loadSettings( const QDomElement & self ) {}

private:
	Lv2Plugin * m_plugin;
};




class LovelyView : public InstrumentView
{
	Q_OBJECT
public:
	LovelyView( Instrument * instrument, QWidget * parent );
	virtual ~LovelyView() {}
};

#endif
