/*
 * lv2_base.cpp - basic LV2 support for LMMS
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


#include "Plugin.h"
#include "embed.h"


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT lv2base_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"LV2 Base",
	"Basic LV2 support for LMMS",
	"Hannu Haahti <grejppi/at/gmail/dot/com>",
	0x0100,
	Plugin::Library,
	NULL,
	NULL
};

}


