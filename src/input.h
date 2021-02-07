#pragma once

/*
retrorun-go2 - libretro frontend for the ODROID-GO Advance
Copyright (C) 2020  OtherCrashOverride

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <stdint.h>
#include <stdbool.h>
#include "go2/input.h"


extern bool input_exit_requested;
extern bool input_save_requested;
extern bool input_load_requested;
extern bool input_reset_requested;
extern bool input_pause_requested;

void input_gamepad_read(go2_gamepad_state_t* out_gamepadState);
void core_input_poll(void);
int16_t core_input_state(unsigned port, unsigned device, unsigned index, unsigned id);


// added by trngaje

enum{
	OGA_PHYSICAL_B=0,
	OGA_PHYSICAL_A,
	OGA_PHYSICAL_X,
	OGA_PHYSICAL_Y,
	OGA_PHYSICAL_L,
	OGA_PHYSICAL_R,
	OGA_PHYSICAL_UP,
	OGA_PHYSICAL_DOWN,
	OGA_PHYSICAL_LEFT,
	OGA_PHYSICAL_RIGHT,
	OGA_PHYSICAL_F1,
	OGA_PHYSICAL_F2,
	OGA_PHYSICAL_F3,
	OGA_PHYSICAL_F4,
	OGA_PHYSICAL_F5,
	OGA_PHYSICAL_F6
};