
#ifndef LOVELY_H
#define LOVELY_H

#include <QtCore/QMutex>

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

	virtual Flags flags() const { return IsSingleStreamed | IsMidiBased; }

	virtual bool handleMidiEvent( const MidiEvent & event, const MidiTime & time );
	virtual void play( sampleFrame * buffer );

	void saveSettings( QDomDocument & doc, QDomElement & self );
	void loadSettings( const QDomElement & self );

private:
	Lv2Plugin * m_plugin;
	QMutex m_pluginMutex;

	QString m_uri;

	friend class LovelyView;
};




class LovelyView : public InstrumentView
{
	Q_OBJECT
public:
	LovelyView( Instrument * instrument, QWidget * parent );
	virtual ~LovelyView();

private:
	LovelyInstrument * m_instrument;
};

#endif
