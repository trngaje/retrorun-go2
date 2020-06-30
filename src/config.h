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
	
	char csnapnamewithrompath[256];
	char cdir_snapshot[256];
	char cromname[256];
} cfg_file_type;

extern cfg_file_type cfgf;