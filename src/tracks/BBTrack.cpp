/*
 * BBTrack.cpp - implementation of class BBTrack and bbTCO
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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

#include <QDomElement>
#include <QColorDialog>
#include <QMenu>
#include <QPainter>

#include "BBEditor.h"
#include "BBTrack.h"
#include "BBTrackContainer.h"
#include "embed.h"
#include "Engine.h"
#include "gui_templates.h"
#include "MainWindow.h"
#include "Mixer.h"
#include "RenameDialog.h"
#include "Song.h"
#include "SongEditor.h"
#include "templates.h"
#include "TrackLabelButton.h"



BBTrack::infoMap BBTrack::s_infoMap;


BBTCO::BBTCO( Track * _track ) :
	TrackContentObject( _track ),
	m_color( 128, 128, 128 ),
	m_useStyleColor( true )
{
	tact_t t = Engine::getBBTrackContainer()->lengthOfBB( bbTrackIndex() );
	if( t > 0 )
	{
		saveJournallingState( false );
		changeLength( MidiTime( t, 0 ) );
		restoreJournallingState();
	}
}




BBTCO::~BBTCO()
{
}




void BBTCO::saveSettings( QDomDocument & doc, QDomElement & element )
{
	element.setAttribute( "name", name() );
	if( element.parentNode().nodeName() == "clipboard" )
	{
		element.setAttribute( "pos", -1 );
	}
	else
	{
		element.setAttribute( "pos", startPosition() );
	}
	element.setAttribute( "len", length() );
	element.setAttribute( "muted", isMuted() );
	element.setAttribute( "color", color() );
	
	if( m_useStyleColor )
	{
		element.setAttribute( "usestyle", 1 );
	}
	else
	{
		element.setAttribute( "usestyle", 0 );
	}
}




void BBTCO::loadSettings( const QDomElement & element )
{
	setName( element.attribute( "name" ) );
	if( element.attribute( "pos" ).toInt() >= 0 )
	{
		movePosition( element.attribute( "pos" ).toInt() );
	}
	changeLength( element.attribute( "len" ).toInt() );
	if( element.attribute( "muted" ).toInt() != isMuted() )
	{
		toggleMute();
	}

	if( element.hasAttribute( "color" ) )
	{
		setColor( QColor( element.attribute( "color" ).toUInt() ) );
	}
	
	if( element.hasAttribute( "usestyle" ) )
	{
		if( element.attribute( "usestyle" ).toUInt() == 1 ) 
		{
			m_useStyleColor = true;
		}
		else
		{
			m_useStyleColor = false;
		}
	}
	else
	{
		if( m_color.rgb() == qRgb( 128, 182, 175 ) || m_color.rgb() == qRgb( 64, 128, 255 ) ) // old or older default color
		{
			m_useStyleColor = true;
		}
		else
		{
			m_useStyleColor = false;
		}
	}
}



int BBTCO::bbTrackIndex()
{
	return dynamic_cast<BBTrack *>( getTrack() )->index();
}



TrackContentObjectView * BBTCO::createView( TrackView * _tv )
{
	return new BBTCOView( this, _tv );
}










BBTCOView::BBTCOView( TrackContentObject * _tco, TrackView * _tv ) :
	TrackContentObjectView( _tco, _tv ),
	m_bbTCO( dynamic_cast<BBTCO *>( _tco ) )
{
}




BBTCOView::~BBTCOView()
{
}




void BBTCOView::constructContextMenu( QMenu * _cm )
{
	QAction * a = new QAction( embed::getIconPixmap( "bb_track" ),
					tr( "Open in Beat+Bassline-Editor" ),
					_cm );
	_cm->insertAction( _cm->actions()[0], a );
	connect( a, SIGNAL( triggered( bool ) ),
			this, SLOT( openInBBEditor() ) );
	_cm->insertSeparator( _cm->actions()[1] );
	_cm->addSeparator();
	_cm->addAction( embed::getIconPixmap( "reload" ), tr( "Reset name" ),
						this, SLOT( resetName() ) );
	_cm->addAction( embed::getIconPixmap( "edit_rename" ),
						tr( "Change name" ),
						this, SLOT( changeName() ) );
	_cm->addAction( embed::getIconPixmap( "colorize" ),
			tr( "Change color" ), this, SLOT( changeColor() ) );
	_cm->addAction( embed::getIconPixmap( "colorize" ),
			tr( "Reset color to default" ), this, SLOT( resetColor() ) );
}




void BBTCOView::mouseDoubleClickEvent( QMouseEvent * )
{
	openInBBEditor();
}




void BBTCOView::paintEvent( QPaintEvent * )
{
	QPainter p( this );

	QColor col;
	if( m_bbTCO->getTrack()->isMuted() || m_bbTCO->isMuted() )
	{
		col = QColor( 160, 160, 160 );
	}
	else if ( m_bbTCO->m_useStyleColor )
	{
		col =  p.pen().brush().color();
	}
	else
	{
		col = m_bbTCO->colorObj();
	}

	if( isSelected() == true )
	{
		col.setRgb( qMax( col.red() - 128, 0 ), qMax( col.green() - 128, 0 ), 255 );
	}

	QLinearGradient lingrad( 0, 0, 0, height() );
	lingrad.setColorAt( 0, col.light( 130 ) );
	lingrad.setColorAt( 1, col.light( 70 ) );
	p.fillRect( rect(), lingrad );

	tact_t t = Engine::getBBTrackContainer()->lengthOfBB( m_bbTCO->bbTrackIndex() );
	if( m_bbTCO->length() > MidiTime::ticksPerTact() && t > 0 )
	{
		for( int x = static_cast<int>( t * pixelsPerTact() );
								x < width()-2;
			x += static_cast<int>( t * pixelsPerTact() ) )
		{
			p.setPen( col.light( 80 ) );
			p.drawLine( x, 1, x, 5 );
			p.setPen( col.light( 120 ) );
			p.drawLine( x, height() - 6, x, height() - 2 );
		}
	}

	p.setPen( col.lighter( 130 ) );
	p.drawRect( 1, 1, rect().right()-2, rect().bottom()-2 );

	p.setPen( col.darker( 300 ) );
	p.drawRect( 0, 0, rect().right(), rect().bottom() );

	p.setFont( pointSize<8>( p.font() ) );
	
	p.setPen( QColor( 0, 0, 0 ) );
	p.drawText( 4, p.fontMetrics().height()+1, m_bbTCO->name() );
	p.setPen( textColor() );
	p.drawText( 3, p.fontMetrics().height(), m_bbTCO->name() );
	
	if( m_bbTCO->isMuted() )
	{
		p.drawPixmap( 3, p.fontMetrics().height() + 1,
				embed::getIconPixmap( "muted", 16, 16 ) );
	}
}




void BBTCOView::openInBBEditor()
{
	Engine::getBBTrackContainer()->setCurrentBB( m_bbTCO->bbTrackIndex() );

	Engine::mainWindow()->toggleBBEditorWin( true );
}




void BBTCOView::resetName()
{
	m_bbTCO->setName( m_bbTCO->getTrack()->name() );
}




void BBTCOView::changeName()
{
	QString s = m_bbTCO->name();
	RenameDialog rename_dlg( s );
	rename_dlg.exec();
	m_bbTCO->setName( s );
}




void BBTCOView::changeColor()
{
	QColor new_color = QColorDialog::getColor( m_bbTCO->m_color );
	if( ! new_color.isValid() )
	{
		return;
	}
	if( isSelected() )
	{
		QVector<selectableObject *> selected =
				Engine::songEditor()->selectedObjects();
		for( QVector<selectableObject *>::iterator it =
							selected.begin();
						it != selected.end(); ++it )
		{
			BBTCOView * bb_tcov = dynamic_cast<BBTCOView *>( *it );
			if( bb_tcov )
			{
				bb_tcov->setColor( new_color );
			}
		}
	}
	else
	{
		setColor( new_color );
	}
}


/** \brief Makes the BB pattern use the colour defined in the stylesheet */
void BBTCOView::resetColor()
{
	if( ! m_bbTCO->m_useStyleColor )
	{
		m_bbTCO->m_useStyleColor = true;
		Engine::getSong()->setModified();
		update();
	}
	BBTrack::clearLastTCOColor();
}



void BBTCOView::setColor( QColor new_color )
{
	if( new_color.rgb() != m_bbTCO->color() )
	{
		m_bbTCO->setColor( new_color );
		m_bbTCO->m_useStyleColor = false;
		Engine::getSong()->setModified();
		update();
	}
	BBTrack::setLastTCOColor( new_color );
}





QColor * BBTrack::s_lastTCOColor = NULL;

BBTrack::BBTrack( TrackContainer* tc ) :
	Track( Track::BBTrack, tc )
{
	int bbNum = s_infoMap.size();
	s_infoMap[this] = bbNum;

	setName( tr( "Beat/Bassline %1" ).arg( bbNum ) );
	Engine::getBBTrackContainer()->setCurrentBB( bbNum );
	Engine::getBBTrackContainer()->updateComboBox();

	connect( this, SIGNAL( nameChanged() ),
		Engine::getBBTrackContainer(), SLOT( updateComboBox() ) );
}




BBTrack::~BBTrack()
{
	Engine::mixer()->removePlayHandles( this );

	const int bb = s_infoMap[this];
	Engine::getBBTrackContainer()->removeBB( bb );
	for( infoMap::iterator it = s_infoMap.begin(); it != s_infoMap.end();
									++it )
	{
		if( it.value() > bb )
		{
			--it.value();
		}
	}
	s_infoMap.remove( this );

	// remove us from TC so bbTrackContainer::numOfBBs() returns a smaller
	// value and thus combobox-updating in bbTrackContainer works well
	trackContainer()->removeTrack( this );
	Engine::getBBTrackContainer()->updateComboBox();
}




// play _frames frames of given TCO within starting with _start
bool BBTrack::play( const MidiTime & _start, const fpp_t _frames,
					const f_cnt_t _offset, int _tco_num )
{
	if( isMuted() )
	{
		return false;
	}

	if( _tco_num >= 0 )
	{
		return Engine::getBBTrackContainer()->play( _start, _frames, _offset, s_infoMap[this] );
	}

	tcoVector tcos;
	getTCOsInRange( tcos, _start, _start + static_cast<int>( _frames / Engine::framesPerTick() ) );

	if( tcos.size() == 0 )
	{
		return false;
	}

	MidiTime lastPosition;
	MidiTime lastLen;
	for( tcoVector::iterator it = tcos.begin(); it != tcos.end(); ++it )
	{
		if( !( *it )->isMuted() &&
				( *it )->startPosition() >= lastPosition )
		{
			lastPosition = ( *it )->startPosition();
			lastLen = ( *it )->length();
		}
	}

	if( _start - lastPosition < lastLen )
	{
		return Engine::getBBTrackContainer()->play( _start - lastPosition, _frames, _offset, s_infoMap[this] );
	}
	return false;
}




TrackView * BBTrack::createView( TrackContainerView* tcv )
{
	return new BBTrackView( this, tcv );
}




TrackContentObject * BBTrack::createTCO( const MidiTime & _pos )
{
	BBTCO * bbtco = new BBTCO( this );
	if( s_lastTCOColor )
	{
		bbtco->setColor( *s_lastTCOColor );
		bbtco->setUseStyleColor( false );
	}
	return bbtco;
}




void BBTrack::saveTrackSpecificSettings( QDomDocument & _doc,
							QDomElement & _this )
{
//	_this.setAttribute( "icon", m_trackLabel->pixmapFile() );
/*	_this.setAttribute( "current", s_infoMap[this] ==
					engine::getBBEditor()->currentBB() );*/
	if( s_infoMap[this] == 0 &&
			_this.parentNode().parentNode().nodeName() != "clone" &&
			_this.parentNode().parentNode().nodeName() != "journaldata" )
	{
		( (JournallingObject *)( Engine::getBBTrackContainer() ) )->
						saveState( _doc, _this );
	}
	if( _this.parentNode().parentNode().nodeName() == "clone" )
	{
		_this.setAttribute( "clonebbt", s_infoMap[this] );
	}
}




void BBTrack::loadTrackSpecificSettings( const QDomElement & _this )
{
/*	if( _this.attribute( "icon" ) != "" )
	{
		m_trackLabel->setPixmapFile( _this.attribute( "icon" ) );
	}*/

	if( _this.hasAttribute( "clonebbt" ) )
	{
		const int src = _this.attribute( "clonebbt" ).toInt();
		const int dst = s_infoMap[this];
		Engine::getBBTrackContainer()->createTCOsForBB( dst );
		TrackContainer::TrackList tl =
					Engine::getBBTrackContainer()->tracks();
		// copy TCOs of all tracks from source BB (at bar "src") to destination
		// TCOs (which are created if they do not exist yet)
		for( TrackContainer::TrackList::iterator it = tl.begin();
							it != tl.end(); ++it )
		{
			( *it )->getTCO( src )->copy();
			( *it )->getTCO( dst )->paste();
		}
		setName( tr( "Clone of %1" ).arg(
					_this.parentNode().toElement().attribute( "name" ) ) );
	}
	else
	{
		QDomNode node = _this.namedItem(
					TrackContainer::classNodeName() );
		if( node.isElement() )
		{
			( (JournallingObject *)Engine::getBBTrackContainer() )->
					restoreState( node.toElement() );
		}
	}
/*	doesn't work yet because BBTrack-ctor also sets current bb so if
	bb-tracks are created after this function is called, this doesn't
	help at all....
	if( _this.attribute( "current" ).toInt() )
	{
		engine::getBBEditor()->setCurrentBB( s_infoMap[this] );
	}*/
}




// return pointer to BBTrack specified by _bb_num
BBTrack * BBTrack::findBBTrack( int _bb_num )
{
	for( infoMap::iterator it = s_infoMap.begin(); it != s_infoMap.end();
									++it )
	{
		if( it.value() == _bb_num )
		{
			return it.key();
		}
	}
	return NULL;
}




void BBTrack::swapBBTracks( Track * _track1, Track * _track2 )
{
	BBTrack * t1 = dynamic_cast<BBTrack *>( _track1 );
	BBTrack * t2 = dynamic_cast<BBTrack *>( _track2 );
	if( t1 != NULL && t2 != NULL )
	{
		qSwap( s_infoMap[t1], s_infoMap[t2] );
		Engine::getBBTrackContainer()->swapBB( s_infoMap[t1],
								s_infoMap[t2] );
		Engine::getBBTrackContainer()->setCurrentBB( s_infoMap[t1] );
	}
}









BBTrackView::BBTrackView( BBTrack * _bbt, TrackContainerView* tcv ) :
	TrackView( _bbt, tcv ),
	m_bbTrack( _bbt )
{
	setFixedHeight( 32 );
	// drag'n'drop with bb-tracks only causes troubles (and makes no sense
	// too), so disable it
	setAcceptDrops( false );

	m_trackLabel = new TrackLabelButton( this, getTrackSettingsWidget() );
	m_trackLabel->setIcon( embed::getIconPixmap( "bb_track" ) );
	m_trackLabel->move( 3, 1 );
	m_trackLabel->show();
	connect( m_trackLabel, SIGNAL( clicked( bool ) ),
			this, SLOT( clickedTrackLabel() ) );
	setModel( _bbt );
}




BBTrackView::~BBTrackView()
{
	Engine::getBBEditor()->removeBBView( BBTrack::s_infoMap[m_bbTrack] );
}




bool BBTrackView::close()
{
	Engine::getBBEditor()->removeBBView( BBTrack::s_infoMap[m_bbTrack] );
	return TrackView::close();
}




void BBTrackView::clickedTrackLabel()
{
	Engine::getBBTrackContainer()->setCurrentBB( m_bbTrack->index() );
	Engine::getBBEditor()->show();
/*	foreach( bbTrackView * tv,
			trackContainerView()->findChildren<bbTrackView *>() )
	{
		tv->m_trackLabel->update();
	}*/

}







