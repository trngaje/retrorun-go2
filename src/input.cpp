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

#include "input.h"

#include "globals.h"
#include "video.h"
#include "libretro.h"

#include <go2/input.h>
#include <stdio.h>

#include "config.h"

extern int opt_backlight;
extern int opt_volume;

bool input_exit_requested = false;
bool input_save_requested = false;
bool input_load_requested = false;
bool input_reset_requested = false;
unsigned char ucDPAD_rotate = 0; // 0 (default), 1, 2, 3 = degree 0,90,180,270
extern unsigned char ucScreen_rotate; //trngaje
extern float aspect_ratio;

go2_battery_state_t batteryState;

#if 0
static go2_input_state_t* gamepadState;
static go2_input_state_t* prevGamepadState;
#else
static go2_gamepad_state_t gamepadState;
static go2_gamepad_state_t prevGamepadState;
#endif

static go2_input_t* input;
static bool has_triggers = false;

int16_t getstateButton(int id);

#if 0
void input_gamepad_read(go2_input_state_t* outState)
{
    if (!input)
    {
        input = go2_input_create();

        if (go2_input_features_get(input) & Go2InputFeatureFlags_Triggers)
        {
            has_triggers = true;

            printf("input: Hardware triggers enabled.\n");
        }

        gamepadState = go2_input_state_create();
        prevGamepadState = go2_input_state_create();
    }

	go2_input_state_read(input, outState);
}
#else
void input_gamepad_read(go2_gamepad_state_t* out_gamepadState)
{
    if (!input)
    {
        input = go2_input_create();
    }

	go2_input_gamepad_read(input, out_gamepadState);
    prevGamepadState = *out_gamepadState;
}

unsigned int getinputstate(void)
{
	unsigned int uiStates=0x0;
	
	if (gamepadState.buttons.b)
		uiStates |= 1;
	else
		uiStates &= ~1;
	
	if (gamepadState.buttons.a)
		uiStates |= 1 << 1;
	else
		uiStates &= ~ (1 << 1);

	if (gamepadState.buttons.x)
		uiStates |= 1 << 2;
	else
		uiStates &= ~ (1 << 2);

	if (gamepadState.buttons.y)
		uiStates |= 1 << 3;
	else
		uiStates &= ~ (1 << 3);

	if (gamepadState.buttons.top_left)
		uiStates |= 1 << 4;
	else
		uiStates &= ~ (1 << 4);

	if (gamepadState.buttons.top_right)
		uiStates |= 1 << 5;
	else
		uiStates &= ~ (1 << 5);	
	
	if (gamepadState.dpad.up)
		uiStates |= 1 << 6;
	else
		uiStates &= ~ (1 << 6);	
	
	if (gamepadState.dpad.down)
		uiStates |= 1 << 7;
	else
		uiStates &= ~ (1 << 7);	
	
	if (gamepadState.dpad.left)
		uiStates |= 1 << 8;
	else
		uiStates &= ~ (1 << 8);

	if (gamepadState.dpad.right)
		uiStates |= 1 << 9;
	else
		uiStates &= ~ (1 << 9);	

	if (gamepadState.buttons.f1)
		uiStates |= 1 << 10;
	else
		uiStates &= ~ (1 << 10);	

	if (gamepadState.buttons.f2)
		uiStates |= 1 << 11;
	else
		uiStates &= ~ (1 << 11);

	if (gamepadState.buttons.f3)
		uiStates |= 1 << 12;
	else
		uiStates &= ~ (1 << 12);

	if (gamepadState.buttons.f4)
		uiStates |= 1 << 13;
	else
		uiStates &= ~ (1 << 13);	
	
	if (gamepadState.buttons.f5)
		uiStates |= 1 << 14;
	else
		uiStates &= ~ (1 << 14);

	if (gamepadState.buttons.f6)
		uiStates |= 1 << 15;
	else
		uiStates &= ~ (1 << 15);

	return uiStates;
}
#endif

void core_input_poll(void)
{
	unsigned int uiCurrentInputState=0;
	static unsigned int uiPrevInputState=0;
	
    if (!input)
    {
        input = go2_input_create();
    }

#if 0
    // Swap current/previous state
    go2_input_state_t* tempState = prevGamepadState;
    prevGamepadState = gamepadState;
    gamepadState = tempState;

    // Read inputs
	go2_input_state_read(input, gamepadState);
    go2_input_battery_read(input, &batteryState);

    if (go2_input_state_button_get(prevGamepadState, Go2InputButton_F1) == ButtonState_Released &&
        go2_input_state_button_get(gamepadState, Go2InputButton_F1) == ButtonState_Pressed)
    {
        input_exit_requested = true;
    }

    // if (!prevGamepadState.buttons.f2 && gamepadState.buttons.f2)
    // {
    //     screenshot_requested = true;
    // }

    if (go2_input_state_button_get(gamepadState, Go2InputButton_F4) == ButtonState_Pressed)
    {
        if (go2_input_state_button_get(gamepadState, Go2InputButton_DPadUp) == ButtonState_Pressed &&
            go2_input_state_button_get(prevGamepadState, Go2InputButton_DPadUp) == ButtonState_Released)
        {
            opt_backlight += 10;
            if (opt_backlight > 100) opt_backlight = 100;
            
            printf("Backlight+ = %d\n", opt_backlight);
        }
        else if (go2_input_state_button_get(gamepadState, Go2InputButton_DPadDown) == ButtonState_Pressed &&
                 go2_input_state_button_get(prevGamepadState, Go2InputButton_DPadDown) == ButtonState_Released)
        {
            opt_backlight -= 10;
            if (opt_backlight < 1) opt_backlight = 1;

            printf("Backlight- = %d\n", opt_backlight);
        }

        if (go2_input_state_button_get(gamepadState, Go2InputButton_DPadRight) == ButtonState_Pressed &&
            go2_input_state_button_get(prevGamepadState, Go2InputButton_DPadRight) == ButtonState_Released)
        {
            opt_volume += 5;
            if (opt_volume > 100) opt_volume = 100;

            printf("Volume+ = %d\n", opt_volume);
        }
        else if (go2_input_state_button_get(gamepadState, Go2InputButton_DPadLeft) == ButtonState_Pressed &&
                 go2_input_state_button_get(prevGamepadState, Go2InputButton_DPadLeft) == ButtonState_Released)
        {
            opt_volume -= 5;
            if (opt_volume < 0) opt_volume = 0;

            printf("Volume- = %d\n", opt_volume);
        }
    }
#else

	go2_input_gamepad_read(input, &gamepadState);
    go2_input_battery_read(input, &batteryState);

	uiCurrentInputState = getinputstate();	// detect changing of buttons by trngaje
	if (uiCurrentInputState != uiPrevInputState)
	{
		//printf("core_input_poll: 0x%x\n", uiCurrentInputState);

		if (getstateButton(cfgf.hot_key))
		{
			if (getstateButton(cfgf.exit_key))
			{
				if (cfgf.autosnapshot)
					snapandexit_requested = true; // exit after take snapshot in roms/<emulators>/snap/<roms>.png
				else
					input_exit_requested = true;	// exit
			}
			
			if (gamepadState.buttons.b)	// for reset
			{
				input_reset_requested = true;
			}
			
			if (getstateButton(cfgf.screen_shot_key))
			{
				screenshot_requested = true;	// screen capture
			}	
			
			if (getstateButton(cfgf.screen_rotate_key))
			{
				ucDPAD_rotate = (ucDPAD_rotate + 1) % 4;
				ucScreen_rotate = (4 + ucScreen_rotate - 1) % 4;
			}

			if (getstateButton(cfgf.fullscreen_key))
			{
				if (aspect_ratio != 0.0f)
					aspect_ratio = 0.0f;
				else
					aspect_ratio = 1.0f;
				
				printf("[trngaje] aspect_ratio=%f\n", aspect_ratio);
			}			
		
			if (getstateButton(cfgf.bl_plus_key))
			{
				opt_backlight += 10;
				if (opt_backlight > 100) opt_backlight = 100;
				
				printf("Backlight+ = %d\n", opt_backlight);
			}
			else if (getstateButton(cfgf.bl_minus_key))
			{
				opt_backlight -= 10;
				if (opt_backlight < 1) opt_backlight = 1;

				printf("Backlight- = %d\n", opt_backlight);
			}

			if (getstateButton(cfgf.vol_up_key))
			{
				opt_volume += 5;
				if (opt_volume > 100) opt_volume = 100;

				printf("Volume+ = %d\n", opt_volume);
			}
			else if (getstateButton(cfgf.vol_down_key))
			{
				opt_volume -= 5;
				if (opt_volume < 0) opt_volume = 0;

				printf("Volume- = %d\n", opt_volume);
			}		

			if (getstateButton(cfgf.save_key))
			{
				input_save_requested = true;
			}
			else if (getstateButton(cfgf.load_key))
			{
				input_load_requested = true;
			}
			
		}

		uiPrevInputState = uiCurrentInputState;
	}


#endif
}


int16_t getstateButton(int id)
{
	switch (id)
    {
	case OGA_PHYSICAL_B:
		return gamepadState.buttons.b;
		break;
	case OGA_PHYSICAL_A:
		return gamepadState.buttons.a;
		break;
	case OGA_PHYSICAL_X:
		return gamepadState.buttons.x;
		break;
	case OGA_PHYSICAL_Y:
		return gamepadState.buttons.y;
		break;
	case OGA_PHYSICAL_L:
		return gamepadState.buttons.top_left;
		break;
	case OGA_PHYSICAL_R:
		return gamepadState.buttons.top_right;
		break;
	case OGA_PHYSICAL_UP:
		return gamepadState.dpad.up;
		break;
	case OGA_PHYSICAL_DOWN:
		return gamepadState.dpad.down;
		break;
	case OGA_PHYSICAL_LEFT:
		return gamepadState.dpad.left;
		break;
	case OGA_PHYSICAL_RIGHT:
		return gamepadState.dpad.right;
		break;
	case OGA_PHYSICAL_F1:
		return gamepadState.buttons.f1;
		break;
	case OGA_PHYSICAL_F2:
		return gamepadState.buttons.f2;
		break;
	case OGA_PHYSICAL_F3:
		return gamepadState.buttons.f3;
		break;
	case OGA_PHYSICAL_F4:
		return gamepadState.buttons.f4;
		break;
	case OGA_PHYSICAL_F5:
		return gamepadState.buttons.f5;
		break;
	case OGA_PHYSICAL_F6:
		return gamepadState.buttons.f6;
		break;

	default:
		;
	}
	return 0;		
}

extern int bindkeys[16];

int16_t core_input_state(unsigned port, unsigned device, unsigned index, unsigned id)
{
#if 0
	// new
	
    //int16_t result;

    // if (port || index || device != RETRO_DEVICE_JOYPAD)
    //         return 0;

    if (!Retrorun_UseAnalogStick)
    {
        // Map thumbstick to dpad
        const float TRIM = 0.35f;
        
        go2_thumb_t thumb = go2_input_state_thumbstick_get(gamepadState, Go2InputThumbstick_Left);

        if (thumb.y < -TRIM) go2_input_state_button_set(gamepadState, Go2InputButton_DPadUp, ButtonState_Pressed);
        if (thumb.y > TRIM) go2_input_state_button_set(gamepadState, Go2InputButton_DPadDown, ButtonState_Pressed);
        if (thumb.x < -TRIM) go2_input_state_button_set(gamepadState, Go2InputButton_DPadLeft, ButtonState_Pressed);
        if (thumb.x > TRIM) go2_input_state_button_set(gamepadState, Go2InputButton_DPadRight, ButtonState_Pressed);
    }

/*
#define RETRO_DEVICE_ID_JOYPAD_B        0
#define RETRO_DEVICE_ID_JOYPAD_Y        1
#define RETRO_DEVICE_ID_JOYPAD_SELECT   2
#define RETRO_DEVICE_ID_JOYPAD_START    3
#define RETRO_DEVICE_ID_JOYPAD_UP       4
#define RETRO_DEVICE_ID_JOYPAD_DOWN     5
#define RETRO_DEVICE_ID_JOYPAD_LEFT     6
#define RETRO_DEVICE_ID_JOYPAD_RIGHT    7
#define RETRO_DEVICE_ID_JOYPAD_A        8
#define RETRO_DEVICE_ID_JOYPAD_X        9
#define RETRO_DEVICE_ID_JOYPAD_L       10
#define RETRO_DEVICE_ID_JOYPAD_R       11
#define RETRO_DEVICE_ID_JOYPAD_L2      12
#define RETRO_DEVICE_ID_JOYPAD_R2      13
#define RETRO_DEVICE_ID_JOYPAD_L3      14
#define RETRO_DEVICE_ID_JOYPAD_R3      15
*/

    if (port == 0)
    {
        if (device == RETRO_DEVICE_JOYPAD)
        {
            switch (id)
            {
                case RETRO_DEVICE_ID_JOYPAD_B:
                    return go2_input_state_button_get(gamepadState, Go2InputButton_B);
                    break;
                
                case RETRO_DEVICE_ID_JOYPAD_Y:
                    return go2_input_state_button_get(gamepadState, Go2InputButton_Y);
                    break;

                case RETRO_DEVICE_ID_JOYPAD_SELECT:
                    return go2_input_state_button_get(gamepadState, Go2InputButton_F3);
                    break;

                case RETRO_DEVICE_ID_JOYPAD_START:
                    return go2_input_state_button_get(gamepadState, Go2InputButton_F4);
                    break;

                case RETRO_DEVICE_ID_JOYPAD_UP:
                    return go2_input_state_button_get(gamepadState, Go2InputButton_DPadUp);
                    break;

                case RETRO_DEVICE_ID_JOYPAD_DOWN:
                    return go2_input_state_button_get(gamepadState, Go2InputButton_DPadDown);
                    break;

                case RETRO_DEVICE_ID_JOYPAD_LEFT:
                    return go2_input_state_button_get(gamepadState, Go2InputButton_DPadLeft);
                    break;

                case RETRO_DEVICE_ID_JOYPAD_RIGHT:
                    return go2_input_state_button_get(gamepadState, Go2InputButton_DPadRight);
                    break;

                case RETRO_DEVICE_ID_JOYPAD_A:
                    return go2_input_state_button_get(gamepadState, Go2InputButton_A);
                    break;

                case RETRO_DEVICE_ID_JOYPAD_X:
                    return go2_input_state_button_get(gamepadState, Go2InputButton_X);
                    break;

                case RETRO_DEVICE_ID_JOYPAD_L:
                    if (has_triggers)
                    {
                        return go2_input_state_button_get(gamepadState, Go2InputButton_TopLeft);
                    }
                    else
                    {
                        return opt_triggers ? go2_input_state_button_get(gamepadState, Go2InputButton_F5) :
                            go2_input_state_button_get(gamepadState, Go2InputButton_TopLeft);
                    }
                    break;

                case RETRO_DEVICE_ID_JOYPAD_R:
                    if (has_triggers)
                    {
                        return go2_input_state_button_get(gamepadState, Go2InputButton_TopRight);
                    }
                    else
                    {
                        return opt_triggers ? go2_input_state_button_get(gamepadState, Go2InputButton_F6) :
                            go2_input_state_button_get(gamepadState, Go2InputButton_TopRight);
                    }
                    break;

                case RETRO_DEVICE_ID_JOYPAD_L2:
                    if (has_triggers)
                    {
                        return go2_input_state_button_get(gamepadState, Go2InputButton_TriggerLeft);
                    }
                    else
                    {
                        return opt_triggers ? go2_input_state_button_get(gamepadState, Go2InputButton_TopLeft) :
                            go2_input_state_button_get(gamepadState, Go2InputButton_F5);
                    }
                    break;

                case RETRO_DEVICE_ID_JOYPAD_R2:
                    if (has_triggers)
                    {
                        return go2_input_state_button_get(gamepadState, Go2InputButton_TriggerRight);
                    }
                    else
                    {
                        return opt_triggers ? go2_input_state_button_get(gamepadState, Go2InputButton_TopRight) :
                            go2_input_state_button_get(gamepadState, Go2InputButton_F6);
                    }
                    break;

                default:
                    return 0;
                    break;
            }
        }
        else if (Retrorun_UseAnalogStick && device == RETRO_DEVICE_ANALOG && index == RETRO_DEVICE_INDEX_ANALOG_LEFT)
        {
            go2_thumb_t thumb = go2_input_state_thumbstick_get(gamepadState, Go2InputThumbstick_Left);

            if (thumb.x > 1.0f)
                thumb.x = 1.0f;
            else if (thumb.x < -1.0f)
                thumb.x = -1.0f;
            
            if (thumb.y > 1.0f)
                thumb.y = 1.0f;
            else if (thumb.y < -1.0f)
                thumb.y = -1.0f;

            switch (id)
            {
                case RETRO_DEVICE_ID_ANALOG_X:
                    return thumb.x * 0x7fff;
                    break;
                
                case RETRO_DEVICE_ID_JOYPAD_Y:
                    return thumb.y * 0x7fff;
                    break;
                    
                default:
                    return 0;
                    break;
            }
        }
        
    }
#else
	const float TRIM = 0.5f;
    //int16_t result;

    // if (port || index || device != RETRO_DEVICE_JOYPAD)
    //         return 0;
#if 0
    if (!Retrorun_UseAnalogStick)
    {
        // Map thumbstick to dpad
        const float TRIM = 0.35f;
     
        if (gamepadState.thumb.y < -TRIM) 
			gamepadState.dpad.up = ButtonState_Pressed;
        else if (gamepadState.thumb.y > TRIM) 
			gamepadState.dpad.down = ButtonState_Pressed;
		else if (gamepadState.dpad.up || gamepadState.dpad.down)
			printf("[trngaje] up=%d, down=%d\n", gamepadState.dpad.up, gamepadState.dpad.down);
		
        if (gamepadState.thumb.x < -TRIM) 
			gamepadState.dpad.left = ButtonState_Pressed;
        else if (gamepadState.thumb.x > TRIM) 
			gamepadState.dpad.right = ButtonState_Pressed;
		else if (gamepadState.dpad.left || gamepadState.dpad.right)
			printf("[trngaje] left=%d, right=%d\n", gamepadState.dpad.left, gamepadState.dpad.right);
		
    }
#endif
    if (port == 0)
    {
        if (device == RETRO_DEVICE_JOYPAD)
        {
            switch (id)
            {
                case RETRO_DEVICE_ID_JOYPAD_B:
                case RETRO_DEVICE_ID_JOYPAD_Y:
                case RETRO_DEVICE_ID_JOYPAD_SELECT:
                case RETRO_DEVICE_ID_JOYPAD_START:
                //case RETRO_DEVICE_ID_JOYPAD_UP:
                //case RETRO_DEVICE_ID_JOYPAD_DOWN:
                //case RETRO_DEVICE_ID_JOYPAD_LEFT:
                //case RETRO_DEVICE_ID_JOYPAD_RIGHT:
                case RETRO_DEVICE_ID_JOYPAD_A:
                case RETRO_DEVICE_ID_JOYPAD_X:
                case RETRO_DEVICE_ID_JOYPAD_L:
                case RETRO_DEVICE_ID_JOYPAD_R:
                case RETRO_DEVICE_ID_JOYPAD_L2:
                case RETRO_DEVICE_ID_JOYPAD_R2:
					return (getstateButton(bindkeys[id])); // added by trngaje
                    break;
                case RETRO_DEVICE_ID_JOYPAD_UP:
					if (ucDPAD_rotate == 0)
						return (gamepadState.thumb.y < -TRIM) ? ButtonState_Pressed : gamepadState.dpad.up;
					else if (ucDPAD_rotate == 1)
						return (gamepadState.thumb.x > TRIM) ? ButtonState_Pressed : gamepadState.dpad.right;
					else if (ucDPAD_rotate == 2)
						return (gamepadState.thumb.y > TRIM) ? ButtonState_Pressed : gamepadState.dpad.down;
					else if (ucDPAD_rotate == 3)
						return (gamepadState.thumb.x < -TRIM) ? ButtonState_Pressed : gamepadState.dpad.left;
                    break;

                case RETRO_DEVICE_ID_JOYPAD_DOWN:
					if (ucDPAD_rotate == 0)
						return (gamepadState.thumb.y > TRIM) ? ButtonState_Pressed : gamepadState.dpad.down;
					else if (ucDPAD_rotate == 1)
						return (gamepadState.thumb.x < -TRIM) ? ButtonState_Pressed : gamepadState.dpad.left;
					else if (ucDPAD_rotate == 2)
						return (gamepadState.thumb.y < -TRIM) ? ButtonState_Pressed : gamepadState.dpad.up;
					else if (ucDPAD_rotate == 3)
						return (gamepadState.thumb.x > TRIM) ? ButtonState_Pressed : gamepadState.dpad.right;
                    break;

                case RETRO_DEVICE_ID_JOYPAD_LEFT:
					if (ucDPAD_rotate == 0)
						return (gamepadState.thumb.x < -TRIM) ? ButtonState_Pressed : gamepadState.dpad.left;
					else if (ucDPAD_rotate == 1)
						return (gamepadState.thumb.y < -TRIM) ? ButtonState_Pressed : gamepadState.dpad.up;
					else if (ucDPAD_rotate == 2)
						return (gamepadState.thumb.x > TRIM) ? ButtonState_Pressed : gamepadState.dpad.right;
					else if (ucDPAD_rotate == 3)
						return (gamepadState.thumb.y > TRIM) ? ButtonState_Pressed : gamepadState.dpad.down;
                    break;

                case RETRO_DEVICE_ID_JOYPAD_RIGHT:
					if (ucDPAD_rotate == 0)
						return (gamepadState.thumb.x > TRIM) ? ButtonState_Pressed : gamepadState.dpad.right;
					else if (ucDPAD_rotate == 1)
						return (gamepadState.thumb.y > TRIM) ? ButtonState_Pressed : gamepadState.dpad.down;
					else if (ucDPAD_rotate == 2)
						return (gamepadState.thumb.x < -TRIM) ? ButtonState_Pressed : gamepadState.dpad.left;
					else if (ucDPAD_rotate == 3)
						return (gamepadState.thumb.y < -TRIM) ? ButtonState_Pressed : gamepadState.dpad.up;
                    break;                    
                    
					
#if 0
                case RETRO_DEVICE_ID_JOYPAD_B:
                    return gamepadState.buttons.b;
                    break;
                
                case RETRO_DEVICE_ID_JOYPAD_Y:
                    return gamepadState.buttons.y;
                    break;

                case RETRO_DEVICE_ID_JOYPAD_SELECT:
                    return gamepadState.buttons.f3;
                    break;

                case RETRO_DEVICE_ID_JOYPAD_START:
                    return gamepadState.buttons.f4;
                    break;

                case RETRO_DEVICE_ID_JOYPAD_UP:
                    return gamepadState.dpad.up;
                    break;

                case RETRO_DEVICE_ID_JOYPAD_DOWN:
                    return gamepadState.dpad.down;
                    break;

                case RETRO_DEVICE_ID_JOYPAD_LEFT:
                    return gamepadState.dpad.left;
                    break;

                case RETRO_DEVICE_ID_JOYPAD_RIGHT:
                    return gamepadState.dpad.right;
                    break;

                case RETRO_DEVICE_ID_JOYPAD_A:
                    return gamepadState.buttons.a;
                    break;

                case RETRO_DEVICE_ID_JOYPAD_X:
                    return gamepadState.buttons.x;
                    break;

                case RETRO_DEVICE_ID_JOYPAD_L:
                    return opt_triggers ? gamepadState.buttons.f5 : gamepadState.buttons.top_left;
                    break;

                case RETRO_DEVICE_ID_JOYPAD_R:
                    return opt_triggers ? gamepadState.buttons.f6 : gamepadState.buttons.top_right;
                    break;

                case RETRO_DEVICE_ID_JOYPAD_L2:
                    return opt_triggers ? gamepadState.buttons.top_left : gamepadState.buttons.f5;
                    break;

                case RETRO_DEVICE_ID_JOYPAD_R2:
                    return opt_triggers ? gamepadState.buttons.top_right : gamepadState.buttons.f6;
                    break;
#endif
                default:
                    return 0;
                    break;
            }
        }
        else if (device == RETRO_DEVICE_ANALOG && index == RETRO_DEVICE_INDEX_ANALOG_LEFT)
        {
			// check boundary by trngaje
			// libgo2 has wrong codes

		
#if 0		
			if (gamepadState.thumb.x > 1.0)
			{
				//printf("[trngaje] wrong value thumb.x=%f\n", gamepadState.thumb.x);
				gamepadState.thumb.x = 1.0;
			}
			else if (gamepadState.thumb.x < -1.0)
			{
				//printf("[trngaje] wrong value thumb.x=%f\n", gamepadState.thumb.x);
				gamepadState.thumb.x = -1.0;
			}
			
			if (gamepadState.thumb.y > 1.0)
			{
				//printf("[trngaje] wrong value thumb.y=%f\n", gamepadState.thumb.y);
				gamepadState.thumb.y = 1.0;
			}
			else if (gamepadState.thumb.y < -1.0)
			{
				//printf("[trngaje] wrong value thumb.y=%f\n", gamepadState.thumb.y);
				gamepadState.thumb.y = -1.0;
			}
			
			// set deadzone as 0.3 for arkanoid
			if (gamepadState.thumb.x > -0.3f &&  gamepadState.thumb.x < 0.3f)
				gamepadState.thumb.x = 0;
			
			if (gamepadState.thumb.y > -0.3f &&  gamepadState.thumb.y < 0.3f)
				gamepadState.thumb.y = 0;
			
            switch (id)
            {
                case RETRO_DEVICE_ID_ANALOG_X:
					if (ucDPAD_rotate == 0)
						return gamepadState.thumb.x * 0x7fff;
					else if (ucDPAD_rotate == 1)
						return gamepadState.thumb.y * 0x7fff;
					else if (ucDPAD_rotate == 2)
						return (gamepadState.thumb.x * (-1.0)) * 0x7fff;
					else if (ucDPAD_rotate == 3)
						return (gamepadState.thumb.y * (-1.0)) * 0x7fff;
                    break;
                
                case RETRO_DEVICE_ID_JOYPAD_Y:
					if (ucDPAD_rotate == 0)
						return gamepadState.thumb.y * 0x7fff;
					else if (ucDPAD_rotate == 1)
						return (gamepadState.thumb.x * (-1.0)) * 0x7fff;
					else if (ucDPAD_rotate == 2)
						return (gamepadState.thumb.y * (-1.0)) * 0x7fff;
					else if (ucDPAD_rotate == 3)
						return gamepadState.thumb.x * 0x7fff;
                    break;
                    
                default:
                    return 0;
                    break;
            }
#else
			float temp_thumb_x, temp_thumb_y;
			temp_thumb_x = gamepadState.thumb.x;
			temp_thumb_y = gamepadState.thumb.y;
			
			if (temp_thumb_x > 1.0f)
			{
				//printf("[trngaje] wrong value thumb.x=%f\n", gamepadState.thumb.x);
				temp_thumb_x = 1.0;
			}
			else if (temp_thumb_x < -1.0f)
			{
				//printf("[trngaje] wrong value thumb.x=%f\n", gamepadState.thumb.x);
				temp_thumb_x = -1.0f;
			}
			
			if (temp_thumb_y > 1.0f)
			{
				//printf("[trngaje] wrong value thumb.y=%f\n", gamepadState.thumb.y);
				temp_thumb_y = 1.0f;
			}
			else if (temp_thumb_y < -1.0f)
			{
				//printf("[trngaje] wrong value thumb.y=%f\n", gamepadState.thumb.y);
				temp_thumb_y = -1.0f;
			}
			
			// set deadzone as 0.3 for arkanoid
			if (temp_thumb_x > -0.3f &&  temp_thumb_x < 0.3f)
				temp_thumb_x = 0;
			
			if (temp_thumb_y > -0.3f &&  temp_thumb_y < 0.3f)
				temp_thumb_y = 0;
			
            switch (id)
            {
                case RETRO_DEVICE_ID_ANALOG_X:
					if (ucDPAD_rotate == 0)
						return temp_thumb_x * 0x7fff;
					else if (ucDPAD_rotate == 1)
						return temp_thumb_y * 0x7fff;
					else if (ucDPAD_rotate == 2)
						return (temp_thumb_x * (-1.0f)) * 0x7fff;
					else if (ucDPAD_rotate == 3)
						return (temp_thumb_y * (-1.0f)) * 0x7fff;
                    break;
                
                case RETRO_DEVICE_ID_JOYPAD_Y:
					if (ucDPAD_rotate == 0)
						return temp_thumb_y * 0x7fff;
					else if (ucDPAD_rotate == 1)
						return (temp_thumb_x * (-1.0f)) * 0x7fff;
					else if (ucDPAD_rotate == 2)
						return (temp_thumb_y * (-1.0f)) * 0x7fff;
					else if (ucDPAD_rotate == 3)
						return temp_thumb_x * 0x7fff;
                    break;
                    
                default:
                    return 0;
                    break;
            }
#endif
        }
        
    }
#endif
    return 0;
}
