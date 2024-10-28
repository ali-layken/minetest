/*
Minetest
Copyright (C) 2014 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef __SWITCH__
#error This file may only be compiled for switch!
#endif

#include "util/numeric.h"
#include "porting.h"
#include "porting_switch.h"
#include <switch.h>

#include <sstream>
#include <exception>
#include <cstdlib>

#ifdef GPROF
#include "prof.h"
#endif


namespace porting {

float getDisplayDensity()
{
	return 209;
}

v2u32 getDisplaySize()
{
	return v2u32(1280, 720);
}

core::stringw showTextInputDialog(const std::string &hint, const std::string &current, int editType)
{
	char tmpoutstr[256] = {0};
	wchar_t output[256] = {0};
	Result rc = 0;
	SwkbdConfig kbd;
	rc = swkbdCreate(&kbd, 0);

	if(editType == 3)
	{
		swkbdConfigMakePresetPassword(&kbd);
	}
	swkbdConfigSetGuideText(&kbd, hint.c_str());
	swkbdConfigSetInitialText(&kbd, current.c_str());
	rc = swkbdShow(&kbd, tmpoutstr, sizeof(tmpoutstr));
	swkbdClose(&kbd);
	mbstowcs(output, tmpoutstr, strlen(tmpoutstr));
	return core::stringw(output);
}

bool hasPhysicalKeyboardSwitch()
{
	return false;
}
}
