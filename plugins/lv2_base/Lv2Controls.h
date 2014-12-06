/*
 * Lv2Controls.h - LV2 plugin controls
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


#ifndef LV2CONTROLS_H
#define LV2CONTROLS_H

#include <QMap>
#include <QPair>

#include "Knob.h"

#include "Lv2Plugin.h"




class PLUGIN_EXPORT Lv2Controls
{
	Q_OBJECT

public:
	Lv2Controls( Lv2Plugin * plugin );
	~Lv2Controls();

private:
	Lv2Plugin * m_plugin;
	QMap<uint32_t, Knob *> m_knobs;
};

#endif
