#pragma once


// header for config file

typedef struct 
{
	int autosave;
	int autovertical;
	int autosnapshot;
	int showcorelog;
	
	int hot_key;
	int exit_key;
	int save_key;
	int load_key;
	int bl_plus_key;
	int bl_minus_key;
	int vol_up_key;
	int vol_down_key;
	int screen_shot_key;
	int screen_rotate_key;
	int fullscreen_key;
	int reset_key;
	int pause_key;
	
	int show_status_icons;
	int show_status_texts;
	int icon_battery_x;
	int icon_battery_y;
	int icon_volume_x;
	int icon_volume_y;
	int icon_brightness_x;
	int icon_brightness_y;
	int status_x;
	int status_y;
	
	char csnapnamewithrompath[256];
	char cdir_snapshot[256];
	char cromname[256];
} cfg_file_type;

extern cfg_file_type cfgf;