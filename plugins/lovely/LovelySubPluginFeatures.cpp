/*
 * LovelySubPluginFeatures.cpp - LV2 instrument host for LMMS
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


#include <QtGui/QLabel>

#include "Lv2Plugin.h"
#include "LovelySubPluginFeatures.h"




LovelySubPluginFeatures::LovelySubPluginFeatures( Plugin::PluginTypes type ) :
	SubPluginFeatures( type )
{
	lv2();
}




void LovelySubPluginFeatures::fillDescriptionWidget( QWidget * parent, const Key * key ) const
{
	new QLabel( QWidget::tr( "Name: " ) + key->name, parent );
	new QLabel( "URI: " + key->attributes["uri"], parent );
}




void LovelySubPluginFeatures::listSubPluginKeys( const Plugin::Descriptor * desc, KeyList & kl ) const
{
	for( uint32_t i = 0; i < lv2()->numberOfPlugins(); ++i )
	{
		Lv2PluginDescriptor * lv2desc = lv2()->descriptor( i );
		if( lv2desc->isInstrument() )
		{
			Key::AttributeMap am;
			QString uri( lv2desc->uri() );
			QString name( lv2desc->name() );

			am["uri"] = uri;
			kl.push_back( Key( desc, name, am ) );
		}
	}
}
