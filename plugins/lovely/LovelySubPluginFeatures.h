/*
 * LovelySubPluginFeatures.h - LV2 instrument host for LMMS
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


#ifndef LOVELY_SUBPLUGIN_FEATURES_H
#define LOVELY_SUBPLUGIN_FEATURES_H

#include "Plugin.h"


class PLUGIN_EXPORT LovelySubPluginFeatures : public Plugin::Descriptor::SubPluginFeatures
{
public:
	LovelySubPluginFeatures( Plugin::PluginTypes type );
	virtual void fillDescriptionWidget( QWidget * parent, const Key * key ) const;
	virtual void listSubPluginKeys( const Plugin::Descriptor * desc, KeyList & kl ) const;
};


#endif
