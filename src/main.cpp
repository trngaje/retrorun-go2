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

#include "globals.h"
#include "video.h"
#include "audio.h"
#include "input.h"

#include <unistd.h>

//#include <go2/queue.h>

#include <linux/dma-buf.h>
#include <sys/ioctl.h>

#include "libretro.h"
#include <dlfcn.h>
#include <cstdarg>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <exception>
#include <getopt.h>
#include <map>
#include <vector>

#define EGL_EGLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <drm/drm_fourcc.h>
#include <sys/time.h>
#include <go2/input.h>

// for config by trngaje
#include <libconfig.h>
#include <libgen.h>
#include <sys/stat.h>
#include "config.h"

#define RETRO_DEVICE_ATARI_JOYSTICK RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 1)


extern go2_battery_state_t batteryState;
extern unsigned char ucDPAD_rotate; //trngaje
extern unsigned char ucScreen_rotate; //trngaje
extern unsigned char ucRom_rotate;

retro_hw_context_reset_t retro_context_reset;

const char* opt_savedir = ".";
const char* opt_systemdir = ".";
const char* opt_configdir = "./retrorun.cfg";
float opt_aspect = 0.0f;
int opt_backlight = -1;
int opt_volume = -1;
bool opt_restart = false;
const char* arg_core = "";
const char* arg_rom = "";

typedef std::map<std::string, std::string> varmap_t ;
varmap_t variables;

struct option longopts[] = {
	{ "savedir", required_argument, NULL, 's' },
    { "systemdir", required_argument, NULL, 'd' },
    { "aspect", required_argument, NULL, 'a' },
    { "backlight", required_argument, NULL, 'b' },
    { "volume", required_argument, NULL, 'v' },
	{ "config", required_argument, NULL, 'c' }, // added by trngaje
    { "restart", no_argument, NULL, 'r' },
    { "triggers", no_argument, NULL, 't' },
    { 0, 0, 0, 0 }};


static struct {
	void * handle;
	bool initialized;

	void (*retro_init)(void);
	void (*retro_deinit)(void);
	unsigned (*retro_api_version)(void);
	void (*retro_get_system_info)(struct retro_system_info * info);
	void (*retro_get_system_av_info)(struct retro_system_av_info * info);
	void (*retro_set_controller_port_device)(unsigned port, unsigned device);
	void (*retro_reset)(void);
	void (*retro_run)(void);
	size_t (*retro_serialize_size)(void);
	bool (*retro_serialize)(void *data, size_t size);
	bool (*retro_unserialize)(const void *data, size_t size);
	//	void retro_cheat_reset(void);
	//	void retro_cheat_set(unsigned index, bool enabled, const char *code);
	bool (*retro_load_game)(const struct retro_game_info * game);
	//	bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info);
	void (*retro_unload_game)(void);
	//	unsigned retro_get_region(void);
	void* (*retro_get_memory_data)(unsigned id);
	size_t (*retro_get_memory_size)(unsigned id);
} g_retro;


#define load_sym(V, S) do {\
        if (!((*(void**)&V) = dlsym(g_retro.handle, #S))) \
        { \
            printf("[noarch] Failed to load symbol '" #S "'': %s", dlerror()); \
            abort(); \
        } \
	} while (0)

#define load_retro_sym(S) load_sym(g_retro.S, S)


static void core_log(enum retro_log_level level, const char* fmt, ...)
{
	char buffer[4096] = {
		0
	};
	
    static const char * levelstr[] = {
		"dbg",
		"inf",
		"wrn",
		"err"
	};
	
    va_list va;

	va_start(va, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, va);
	va_end(va);

	if (level == 0)
		return;

	if (cfgf.showcorelog)
	{
		fprintf(stdout, "[%s] %s", levelstr[level], buffer);
		fflush(stdout);
	}

#if 0
	if (level == RETRO_LOG_ERROR)
		exit(EXIT_FAILURE);
#endif
}

static __eglMustCastToProperFunctionPointerType get_proc_address(const char* sym)
{
    __eglMustCastToProperFunctionPointerType result = eglGetProcAddress(sym);
    //printf("get_proc_address: sym='%s', result=%p\n", sym, (void*)result);

    return result;
}

static bool core_environment(unsigned cmd, void* data)
{
	bool* bval;
	unsigned *rotatevalue;
	
	switch (cmd)
    {
        case RETRO_ENVIRONMENT_GET_FASTFORWARDING:
            bval = (bool*)data;
            *bval = false;
            return true;

        case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
        {
            struct retro_log_callback * cb = (struct retro_log_callback * ) data;
            cb->log = core_log;
            break;
        }

        case RETRO_ENVIRONMENT_GET_CAN_DUPE:
            bval = (bool*)data;
            *bval = true;
            break;

        case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
        {
            const enum retro_pixel_format fmt = *(enum retro_pixel_format *)data;
            printf("RETRO_ENVIRONMENT_SET_PIXEL_FORMAT: fmt=0x%x\n", (int)fmt);

            switch (fmt)
            {
            case RETRO_PIXEL_FORMAT_0RGB1555:
                color_format = DRM_FORMAT_RGBA5551;
                break;
                
            case RETRO_PIXEL_FORMAT_RGB565:
                color_format = DRM_FORMAT_RGB565;
                break;
            
            case RETRO_PIXEL_FORMAT_XRGB8888:
                color_format = DRM_FORMAT_XRGB8888;
                break;

            default:
                return false;
            }

            return true;
        }

        case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
            *(const char**)data = opt_systemdir;
            return true;

        case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
            *(const char**)data = opt_savedir;
            return true;

        case RETRO_ENVIRONMENT_SET_HW_RENDER:
        {
            retro_hw_render_callback* hw = (retro_hw_render_callback*)data;

            printf("RETRO_ENVIRONMENT_SET_HW_RENDER\n");
            isOpenGL = true;
            GLContextMajor = hw->version_major;
            GLContextMinor = hw->version_minor;
            retro_context_reset = hw->context_reset;

            hw->get_current_framebuffer = core_video_get_current_framebuffer;
            hw->get_proc_address = (retro_hw_get_proc_address_t)get_proc_address;

            printf("HWRENDER: context_type=%d, major=%d, minor=%d\n",
                hw->context_type, GLContextMajor, GLContextMinor);

            return true;
        }

        case RETRO_ENVIRONMENT_SET_VARIABLES:
        {
            retro_variable* var = (retro_variable*)data;
            while (var->key != NULL)
            {
                std::string key = var->key;

                const char* start = strchr(var->value, ';');
                start += 2;

                std::string value;
                while(*start != '|' && *start != 0)
                {
                    value += *start;
                    ++start;
                }

                variables[key] = value;
                printf("SET_VAR: %s=%s\n", key.c_str(), value.c_str());
                ++var;
            }

            break;
        }

        case RETRO_ENVIRONMENT_GET_VARIABLE:
        {
            retro_variable* var = (retro_variable*)data;
            printf("ENV_VAR: %s\n", var->key);

            if (strcmp(var->key, "fbneo-neogeo-mode") == 0)
            {
                var->value = "UNIBIOS";
                return true;
            }
			else if (strcmp(var->key, "fbneo-vertical-mode") == 0)
			{
#if 0
                var->value = "enabled";
#else
				var->value = "disabled";
#endif
                return true;				
			}
			else if (strcmp(var->key, "mame2003-plus_skip_disclaimer") == 0)
			{
                var->value = "enabled";

                return true;				
			}		
			else if (strcmp(var->key, "mame2003-plus_skip_warnings") == 0)
			{
                var->value = "enabled";

                return true;				
			}			
			else if (strcmp(var->key, "mame2003-plus_machine_timing") == 0)
			{
                var->value = "disabled";

                return true;				
			}	
			else if (strcmp(var->key, "mame_current_skip_gameinfo") == 0)
			{
                var->value = "enabled";

                return true;				
			}
			else if (strcmp(var->key, "mame_current_skip_warnings") == 0)
			{
                var->value = "enabled";

                return true;				
			}			
			else if (strcmp(var->key, "mame_current_frame_skip") == 0)
			{
                var->value = "1";

                return true;				
			}
		
            else if (strcmp(var->key, "atari800_resolution") == 0)
            {
                var->value = "336x240";
                return true;
            }
            else if (strcmp(var->key, "atari800_system") == 0)
            {
                var->value = "5200";
                return true;
            }
            else if (strcmp(var->key, "mgba_sgb_borders") == 0)
            {
                var->value = "OFF";
                return true;
            }
			else if (strcmp(var->key, "parallel-n64-gfxplugin") == 0)
			{
                var->value = "rice";
                return true;				
			}
			else if (strcmp(var->key, "parallel-n64-framerate") == 0)
			{
                var->value = "fullspeed";
                return true;				
			}	
			else if (strcmp(var->key, "parallel-n64-screensize") == 0)
			{
                var->value = "640x480";
                return true;				
			}		
			else if (strcmp(var->key, "fbneo-dipswitch-bublbobl-Lives") == 0)
			{
                var->value = "5"; // 1,2,3,5
                return true;				
			}	
			else if (strcmp(var->key, "fbneo-dipswitch-sboblboblf-Game") == 0)
			{
                var->value = "Bobble Bobble"; // Super Bobble Bobble (default)
                return true;				
			}	
			else if (strcmp(var->key, "fbneo-dipswitch-sboblboblf-Lives") == 0)
			{
                var->value = "5"; // 1,2,3,5 , good (cheat)
                return true;				
			}	
			else if (strcmp(var->key, "fbneo-dipswitch-sboblboblf-Difficulty") == 0)
			{
                var->value = "Easy"; // Normal (default)
                return true;				
			}			
			else if (strcmp(var->key, "fbneo-dipswitch-dland-Lives") == 0)
			{
                var->value = "100 (Cheat)"; // 1,2,3,5 , good
                return true;				
			}			
			else if (strcmp(var->key, "fbneo-dipswitch-bublbobl-Bonus_Life") == 0)
			{
                var->value = "20K 80K 300K";
                return true;				
			}	
			else if (strcmp(var->key, "fbneo-dipswitch-bublbobl-Difficulty") == 0)
			{
                var->value = "Easy"; //Easy, Normal, Hard, Very_Hard
                return true;				
			}	
#if 0			
			else if (strcmp(var->key, "fbneo-dipswitch-bublbobl-Mode") == 0)
			{
                var->value = "Game, Japanese"; 
                return true;				
			}
#endif			
	        else{
                varmap_t::iterator iter = variables.find(var->key);
                if (iter != variables.end())
                {
                    var->value = iter->second.c_str();
                    printf("ENV_VAR (default): %s=%s\n", var->key, var->value);

                    return true;
                }

            }

            return false;
        }

		case RETRO_ENVIRONMENT_SET_ROTATION:
			//Sets screen rotation of graphics.
			//Valid values are 0, 1, 2, 3, which rotates screen by 0, 90, 180,
			//270 degrees counter-clockwise respectively.
//			int *rotatevalue = (int *)data;
//			printf("SET_VAR:RETRO_ENVIRONMENT_SET_ROTATION:%d\n", rotatevalue);
			rotatevalue = (unsigned *)data;
//			*rotatevalue = 0;
			printf("SET_VAR:RETRO_ENVIRONMENT_SET_ROTATION:%d\n", *rotatevalue);

			ucRom_rotate = (unsigned char) (*rotatevalue);
			if (!cfgf.autovertical) // keep normal orientation 
			{
				ucScreen_rotate = ucRom_rotate;
				ucDPAD_rotate = 0;
			}
			else{

#if 1 //def TRNGAJE_OGA_CLONED_RG351P
				if (ucRom_rotate == 2 || ucRom_rotate == 1) // for rg351p
#else
				if (ucRom_rotate == 2 || ucRom_rotate == 3) // for oga	
#endif
				{
#if 0
					// for RG351P
					ucDPAD_rotate = (ucRom_rotate) % 4;
					ucScreen_rotate = 0;				
#else					
					ucDPAD_rotate = (ucRom_rotate + 2) % 4;
					ucScreen_rotate = (4 + 0 - 2) % 4;
#endif
				}
				else{
#if 0
					ucScreen_rotate = 2;
					ucDPAD_rotate = (ucRom_rotate + 2) % 4;	
#else					
					ucScreen_rotate = 0;
					ucDPAD_rotate = ucRom_rotate;	
#endif					
				}
				
			}
			
			printf("[trngaje] ucDPAD_rotate=%d\n", ucDPAD_rotate);
			
            return true;
		
        default:
            core_log(RETRO_LOG_DEBUG, "Unhandled env #%u", cmd);
            return false;
	}

	return true;
}

static void core_load(const char* sofile)
{
	void (*set_environment)(retro_environment_t) = NULL;
	void (*set_video_refresh)(retro_video_refresh_t) = NULL;
	void (*set_input_poll)(retro_input_poll_t) = NULL;
	void (*set_input_state)(retro_input_state_t) = NULL;
	void (*set_audio_sample)(retro_audio_sample_t) = NULL;
	void (*set_audio_sample_batch)(retro_audio_sample_batch_t) = NULL;

	memset(&g_retro, 0, sizeof(g_retro));
	g_retro.handle = dlopen(sofile, RTLD_LAZY);

	if (!g_retro.handle)
    {        
		printf("Failed to load core: %s\n", dlerror());
        throw std::exception();
    }

	dlerror();

	load_retro_sym(retro_init);
	load_retro_sym(retro_deinit);
	load_retro_sym(retro_api_version);
	load_retro_sym(retro_get_system_info);
	load_retro_sym(retro_get_system_av_info);
	load_retro_sym(retro_set_controller_port_device);
	load_retro_sym(retro_reset);
	load_retro_sym(retro_run);
	load_retro_sym(retro_load_game);
	load_retro_sym(retro_unload_game);
	load_retro_sym(retro_serialize_size);
	load_retro_sym(retro_serialize);
	load_retro_sym(retro_unserialize);
    load_retro_sym(retro_get_memory_data);
    load_retro_sym(retro_get_memory_size);

	load_sym(set_environment, retro_set_environment);
	load_sym(set_video_refresh, retro_set_video_refresh);
	load_sym(set_input_poll, retro_set_input_poll);
	load_sym(set_input_state, retro_set_input_state);
	load_sym(set_audio_sample, retro_set_audio_sample);
	load_sym(set_audio_sample_batch, retro_set_audio_sample_batch);

	set_environment(core_environment);
	set_video_refresh(core_video_refresh);
	set_input_poll(core_input_poll);
	set_input_state(core_input_state);
	set_audio_sample(core_audio_sample);
	set_audio_sample_batch(core_audio_sample_batch);

	g_retro.retro_init();
	g_retro.initialized = true;

	printf("Core loaded\n");

    g_retro.retro_set_controller_port_device(0, RETRO_DEVICE_JOYPAD);

    struct retro_system_info system = {
		 0, 0, 0, false, false
	};
    g_retro.retro_get_system_info(&system);
    printf("core_load: library_name='%s'\n", system.library_name);

    if (strcmp(system.library_name, "Atari800") == 0)
    {
        Retrorun_Core = RETRORUN_CORE_ATARI800;
        g_retro.retro_set_controller_port_device(0, RETRO_DEVICE_ATARI_JOYSTICK);
    }
}

static void core_load_game(const char * filename)
{
	struct retro_system_timing timing = {
		60.0f, 10000.0f
	};
	struct retro_game_geometry geom = {
		100, 100, 100, 100, 1.0f
	};
	struct retro_system_av_info av = {
		geom, timing
	};
	struct retro_system_info system = {
		0, 0, 0, false, false
		//0, 0, 0, true, true
	};
	struct retro_game_info info = {
		filename,
		0,
		0,
		NULL
	};
	FILE * file = fopen(filename, "rb");

	if (!file)
		goto libc_error;

	fseek(file, 0, SEEK_END);
	info.size = ftell(file);
	rewind(file);

	g_retro.retro_get_system_info( & system);

	if (!system.need_fullpath) {
		info.data = malloc(info.size);

		if (!info.data || !fread((void * ) info.data, info.size, 1, file))
			goto libc_error;
	}

	if (!g_retro.retro_load_game( & info))
    {
		printf("The core failed to load the content.\n");
        abort();
    }

	g_retro.retro_get_system_av_info( & av);
	video_configure(&av.geometry);
	audio_init(av.timing.sample_rate);

	return;

	libc_error:
		printf("Failed to load content '%s'\n", filename);
        abort();
}

static void core_unload()
{
	if (g_retro.initialized)
		g_retro.retro_deinit();

	if (g_retro.handle)
		dlclose(g_retro.handle);
}

static const char* FileNameFromPath(const char* fullpath)
{
    // Find last slash
    const char* ptr = strrchr(fullpath,'/');
    if (!ptr)
    {
        ptr = fullpath;
    }
    else
    {
        ++ptr;
    } 

    return ptr;   
}

static char* PathCombine(const char* path, const char* filename)
{
    int len = strlen(path);
    int total_len = len + strlen(filename);

    char* result = NULL;

    if (path[len-1] != '/')
    {
        ++total_len;
        result = (char*)calloc(total_len + 1, 1);
        strcpy(result, path);
        strcat(result, "/");
        strcat(result, filename);
    }
    else
    {
        result = (char*)calloc(total_len + 1, 1);
        strcpy(result, path);
        strcat(result, filename);
    }
    
    return result;
}

static int LoadState(const char* saveName)
{
    FILE* file = fopen(saveName, "rb");
	if (!file)
		return -1;

	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	rewind(file);

    if (size < 1) return -1;

    void* ptr = malloc(size);
    if (!ptr) abort();

    size_t count = fread(ptr, 1, size, file);
    if ((size_t)size != count)
    {
        free(ptr);
        abort();
    }

    fclose(file);

    g_retro.retro_unserialize(ptr, size);
    free(ptr);

    return 0;
}

static int LoadSram(const char* saveName)
{
    FILE* file = fopen(saveName, "rb");
	if (!file)
		return -1;

	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	rewind(file);

    size_t sramSize = g_retro.retro_get_memory_size(0);
    if (size < 1) return -1;
    if (size != sramSize)
    {
        printf("LoadSram: File size mismatch (%d != %d)\n", size, sramSize);
        return -1;
    }

    void* ptr = g_retro.retro_get_memory_data(0);
    if (!ptr) abort();

    size_t count = fread(ptr, 1, size, file);
    if ((size_t)size != count)
    {
        abort();
    }

    fclose(file);

    return 0;
}

static void SaveState(const char* saveName)
{
    size_t size = g_retro.retro_serialize_size();
    
    void* ptr = malloc(size);
    if (!ptr) abort();

    g_retro.retro_serialize(ptr, size);

    FILE* file = fopen(saveName, "wb");
	if (!file)
    {
        free(ptr);
		abort();
    }

    size_t count = fwrite(ptr, 1, size, file);
    if (count != size)
    {
        free(ptr);
        abort();
    }

    fclose(file);
    free(ptr);
}

static void SaveSram(const char* saveName)
{
    size_t size = g_retro.retro_get_memory_size(0);
    if (size < 1) return;
    
    void* ptr = g_retro.retro_get_memory_data(0);
    if (!ptr) abort();

 
    FILE* file = fopen(saveName, "wb");
	if (!file)
    {
		abort();
    }

    size_t count = fwrite(ptr, 1, size, file);
    if (count != size)
    {
        abort();
    }

    fclose(file);
}

typedef struct 
{
	char name[20];
	int index;
} input_button_type;

const input_button_type custom_input_buttons[] =
{	{"input_b", RETRO_DEVICE_ID_JOYPAD_B},
	{"input_y", RETRO_DEVICE_ID_JOYPAD_Y},
	{"input_select", RETRO_DEVICE_ID_JOYPAD_SELECT},
	{"input_start", RETRO_DEVICE_ID_JOYPAD_START},    
	{"input_up", RETRO_DEVICE_ID_JOYPAD_UP},
	{"input_down", RETRO_DEVICE_ID_JOYPAD_DOWN},
	{"input_left", RETRO_DEVICE_ID_JOYPAD_LEFT},
	{"input_right", RETRO_DEVICE_ID_JOYPAD_RIGHT},
	{"input_a", RETRO_DEVICE_ID_JOYPAD_A},
	{"input_x", RETRO_DEVICE_ID_JOYPAD_X},
	{"input_l", RETRO_DEVICE_ID_JOYPAD_L},
	{"input_r", RETRO_DEVICE_ID_JOYPAD_R},
	{"input_l2", RETRO_DEVICE_ID_JOYPAD_L2},
	{"input_r2", RETRO_DEVICE_ID_JOYPAD_R2},
	{"input_l3", RETRO_DEVICE_ID_JOYPAD_L3},
    {"input_r3", RETRO_DEVICE_ID_JOYPAD_R3}};

// move to input.h
#if 0
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
#endif
const input_button_type physical_input_buttons[] =
{   {"b", OGA_PHYSICAL_B},
	{"a", OGA_PHYSICAL_A},
	{"x", OGA_PHYSICAL_X},
	{"y", OGA_PHYSICAL_Y},
	{"lb", OGA_PHYSICAL_L},
	{"rb",OGA_PHYSICAL_R},
	{"up", OGA_PHYSICAL_UP},
	{"down", OGA_PHYSICAL_DOWN},
	{"left", OGA_PHYSICAL_LEFT},
	{"right", OGA_PHYSICAL_RIGHT},
	{"f1", OGA_PHYSICAL_F1},
	{"f2", OGA_PHYSICAL_F2},
	{"f3", OGA_PHYSICAL_F3},
	{"f4", OGA_PHYSICAL_F4},
	{"f5", OGA_PHYSICAL_F5},
	{"f6", OGA_PHYSICAL_F6}};

int getindexofkey(const char *name)
{
	int i;
	for (i=0; i<sizeof(physical_input_buttons) / sizeof(input_button_type); i++)
	{
		if (strcmp(name, physical_input_buttons[i].name) == 0)
			return physical_input_buttons[i].index;
	}
	
	return -1;
}

int bindkeys[16] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

cfg_file_type cfgf = {-1, -1, -1, -1, -1, -1, -1, -1, -1};


// by trnaje
bool read_config_file(const char *cConfigFile)
{
	printf("read_config_file\n");
	
	config_t cfg, *cf;
	//int AutoSaveEnabled=0;
	//char inputname[256]="";
	const char *base = NULL;
	double aspectratio=0;
	int i, index;
	
	cf = &cfg;
	config_init(cf);
	if (!config_read_file(cf, cConfigFile)) {
		fprintf(stderr, "retrorun.cfg: %s:%d - %s\n",
			config_error_file(cf),
			config_error_line(cf),
			config_error_text(cf));
		config_destroy(cf);
		return false;
	}
	
	config_lookup_bool(cf, "auto_save", &cfgf.autosave);
	config_lookup_bool(cf, "auto_vertical", &cfgf.autovertical);
	config_lookup_bool(cf, "auto_snapshot_before_exit", &cfgf.autosnapshot);
	config_lookup_bool(cf, "show_core_log", &cfgf.showcorelog);
	config_lookup_float(cf, "aspectratio", &aspectratio);

	config_lookup_bool(cf, "show_status_icons", &cfgf.show_status_icons);
	config_lookup_bool(cf, "show_status_texts", &cfgf.show_status_texts);
	config_lookup_int(cf, "icon_battery_x", &cfgf.icon_battery_x);
	config_lookup_int(cf, "icon_battery_y", &cfgf.icon_battery_y);
	config_lookup_int(cf, "icon_volume_x", &cfgf.icon_volume_x);
	config_lookup_int(cf, "icon_volume_y", &cfgf.icon_volume_y);
	config_lookup_int(cf, "icon_brightness_x", &cfgf.icon_brightness_x);
	config_lookup_int(cf, "icon_brightness_y", &cfgf.icon_brightness_y);
	config_lookup_int(cf, "status_x", &cfgf.status_x);
	config_lookup_int(cf, "status_y", &cfgf.status_y);

	for (i=0; i<sizeof(custom_input_buttons) / sizeof(input_button_type); i++)
	{
		if (config_lookup_string(cf, custom_input_buttons[i].name, &base))
		{
			index = getindexofkey(base);
			bindkeys[custom_input_buttons[i].index] = index;
			// display binding
			printf("%s : %d -> %s :%d\n", custom_input_buttons[i].name, custom_input_buttons[i].index, base, index);			
		}			
	}
	
	if (config_lookup_string(cf, "input_enable_hotkey_btn", &base))
	{
		index = getindexofkey(base);
		cfgf.hot_key = index;
		// display binding
		printf("hot_key :%d\n", index);			
	}		

	if (config_lookup_string(cf, "input_exit_emulator_btn", &base))
	{
		index = getindexofkey(base);
		cfgf.exit_key = index;
		// display binding
		printf("exit_key :%d\n", index);			
	}

	if (config_lookup_string(cf, "input_save_state_btn", &base))
	{
		index = getindexofkey(base);
		cfgf.save_key = index;
		// display binding
		printf("save_key :%d\n", index);			
	}

	if (config_lookup_string(cf, "input_load_state_btn", &base))
	{
		index = getindexofkey(base);
		cfgf.load_key = index;
		// display binding
		printf("load_key :%d\n", index);			
	}

	if (config_lookup_string(cf, "input_backlight_plus_btn", &base))
	{
		index = getindexofkey(base);
		cfgf.bl_plus_key = index;
		// display binding
		printf("bl_plus_key :%d\n", index);			
	}

	if (config_lookup_string(cf, "input_backlight_minus_btn", &base))
	{
		index = getindexofkey(base);
		cfgf.bl_minus_key = index;
		// display binding
		printf("bl_minus_key :%d\n", index);			
	}

	if (config_lookup_string(cf, "input_volume_up_btn", &base))
	{
		index = getindexofkey(base);
		cfgf.vol_up_key = index;
		// display binding
		printf("vol_up_key :%d\n", index);			
	}

	if (config_lookup_string(cf, "input_volume_down_btn", &base))
	{
		index = getindexofkey(base);
		cfgf.vol_down_key = index;
		// display binding
		printf("vol_down_key :%d\n", index);			
	}

	if (config_lookup_string(cf, "dir_snapshot", &base))
	{
		strcpy(cfgf.cdir_snapshot, base);
		// display binding
		printf("dir_snapshot :%s\n", cfgf.cdir_snapshot);			
	}	

	if (config_lookup_string(cf, "input_screenshot_btn", &base))
	{
		index = getindexofkey(base);
		cfgf.screen_shot_key = index;
		// display binding
		printf("screen_shot_key :%d\n", index);			
	}

	if (config_lookup_string(cf, "input_screen_rotate_btn", &base))
	{
		index = getindexofkey(base);
		cfgf.screen_rotate_key = index;
		// display binding
		printf("screen_rotate_key :%d\n", index);			
	}

	if (config_lookup_string(cf, "input_fullscreen_btn", &base))
	{
		index = getindexofkey(base);
		cfgf.fullscreen_key = index;
		// display binding
		printf("fullscreen_key :%d\n", index);			
	}

	if (config_lookup_string(cf, "input_reset_btn", &base))
	{
		index = getindexofkey(base);
		cfgf.reset_key = index;
		// display binding
		printf("reset_key :%d\n", index);	
	}
	
	if (config_lookup_string(cf, "input_pause_btn", &base))
	{
		index = getindexofkey(base);
		cfgf.pause_key = index;
		// display binding
		printf("pause_key :%d\n", index);			
	}
	
	for(i=0; i<sizeof(bindkeys) / sizeof(int); i++)
	{
		printf("binds[%d] = %d\n", i, bindkeys[i]);
	}

	//printf("AutoSaveEnabled = %d, aspectratio = %f\n", AutoSaveEnabled, aspectratio);


#if 0		
	if (config_lookup_string(cf, "ldap.base", &base))
			printf("Host: %s\n", base);
	retries = config_lookup(cf, "ldap.retries");
		count = config_setting_length(retries);
	printf("I have %d retries:\n", count);
		for (n = 0; n < count; n++) {
			printf("\t#%d. %d\n", n + 1,
				config_setting_get_int_elem(retries, n));
		}
#endif
	config_destroy(cf);
	
	return true;
}


int fps = 0;

int main(int argc, char *argv[])
{
    //printf("argc=%d, argv=%p\n", argc, argv);


    // Init
#if 0

	if (argc < 3)
    {
		printf("Usage: %s <core> <game>", argv[0]);
        exit(1);
    }

	core_load(argv[1]);
    core_load_game(argv[2]);

#else

    int c;
    int option_index = 0;

	while ((c = getopt_long(argc, argv, "s:d:a:b:v:c:rt", longopts, &option_index)) != -1)
	{
		switch (c)
		{
			case 's':
				opt_savedir = optarg;
				break;

			case 'd':
				opt_systemdir = optarg;
				break;

			case 'a':
				opt_aspect = atof(optarg);
				break;

            case 'b':
                opt_backlight = atoi(optarg);
                break;
            
            case 'v':
                opt_volume = atoi(optarg);
                break;

			case 'c':
				opt_configdir = optarg;
				break;

            case 'r':
                opt_restart = true;
                break;

            case 't':
                opt_triggers = true;
                break;

			default:
				printf("Unknown option. '%s'\n", longopts[option_index].name);
                exit(EXIT_FAILURE);
		}
	}

    printf("opt_save='%s', opt_systemdir='%s', opt_configdir='%s', opt_aspect=%f\n", 
		opt_savedir, opt_systemdir, opt_configdir, opt_aspect);


    int remaining_args = argc - optind;
    int remaining_index = optind;
    printf("remaining_args=%d\n", remaining_args);

    if (remaining_args < 2)
    {
		printf("Usage: %s [-s savedir] [-d systemdir] [-a aspect] core rom", argv[0]);
        exit(EXIT_FAILURE);
    }

    //return 0;
    if (optind < argc)
    {
        printf ("non-option ARGV-elements: ");
        while (optind < argc)
            printf ("%s ", argv[optind++]);
        putchar ('\n');
    }

    arg_core = argv[remaining_index++];
    arg_rom = argv[remaining_index++];

	read_config_file(opt_configdir); // by trngaje

	core_load(arg_core);
    core_load_game(arg_rom);

#endif

    // Overrides
    printf("Checking overrides.\n");

    go2_gamepad_state_t gamepadState;
    input_gamepad_read(&gamepadState);

    if (gamepadState.buttons.f1)
    {
        printf("Forcing restart due to button press (F1).\n");
        opt_restart = true;
    }


    // State
    const char* fileName = FileNameFromPath(arg_rom);  // same to basename
    
    char* saveName = (char*)malloc(strlen(fileName) + 4 + 1);
    strcpy(saveName, fileName);
	
	strcpy(cfgf.cromname, fileName); // trngaje;
	
	// temp
	char* dirstr = strdup(arg_rom);
	char* basestr = strdup(arg_rom);
	char* dname = dirname(dirstr);
	
	// remove extension from basename
	char* bname = basename(basestr);
	char *bname_dot = strchr(bname, '.');
	int bname_offset = bname_dot - bname;
	bname[bname_offset] = '\0';
	
	sprintf(cfgf.csnapnamewithrompath, "%s/snap/%s.png", dname, bname);
	//printf("[trngaje] dname = %s\n", dname);
	//printf("[trngaje] bname = %s\n", bname);
	printf("[trngaje] csnapnamewithrompath = %s\n", cfgf.csnapnamewithrompath);
	
	strcat(dname, "/snap");
	struct stat sb;
	if (stat(dname, &sb) == -1) 
	{ 
		printf("[trngaje] stat access error\n");
//	}
//	else
//	{
//		if ((sb.st_mode & S_IFMT) != S_IFDIR)
//		{
			printf("[trngaje] %s is not exist\n", dname);
			
			if (mkdir(dname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0)
			{
				printf("[trngaje] success to make dir:%s\n", cfgf.csnapnamewithrompath);
			}
			else
			{
				printf("[trngaje] fail to make dir:%s\n", cfgf.csnapnamewithrompath);
			}
//		}
	}


	
	if (dirstr)
		free(dirstr);
	if (basestr)
		free(basestr);
	
	char* savePath;
	char* sramName;
	char* sramPath;
	if (cfgf.autosave)
	{
		strcat(saveName, ".sav");

		savePath = PathCombine(opt_savedir, saveName);
		printf("savePath='%s'\n", savePath);
		
		sramName = (char*)malloc(strlen(fileName) + 4 + 1);
		strcpy(sramName, fileName);
		strcat(sramName, ".srm");

		sramPath = PathCombine(opt_savedir, sramName);
		printf("sramPath='%s'\n", sramPath);


		if (opt_restart)
		{
			printf("Restarting.\n");
		}
		else
		{
			printf("Loading.\n");
			LoadState(savePath);
		}

		if (cfgf.autosave)
			LoadSram(sramPath);
	}

    printf("Entering render loop.\n");

    const char* batteryStateDesc[] = { "UNK", "DSC", "CHG", "FUL" };

    struct timeval startTime;
    struct timeval endTime;
    double elapsed = 0;
    int totalFrames = 0;
    bool isRunning = true;
    while(isRunning)
    {
        gettimeofday(&startTime, NULL);

        if (input_exit_requested)
            isRunning = false;

		if (input_reset_requested)
		{
			input_reset_requested = false;
			g_retro.retro_reset();
		}			
		

		if (input_save_requested)
		{
			input_save_requested=false;
			SaveState(savePath);			
		}
		
		if (input_load_requested)
		{
			input_load_requested=false;
			LoadState(savePath);
		}
	
        if (!input_pause_requested)
        {		
			g_retro.retro_run();
        }
		else
		{
            // must poll to unpause
            core_input_poll();			
		}
		
        gettimeofday(&endTime, NULL);
        ++totalFrames;

        double seconds = (endTime.tv_sec - startTime.tv_sec);
	    double milliseconds = ((double)(endTime.tv_usec - startTime.tv_usec)) / 1000000.0;

        elapsed += seconds + milliseconds;

        if (elapsed >= 1.0)
        {
            fps = (int)(totalFrames / elapsed);
 //           printf("FPS: %i, BATT: %d [%s]\n", fps, batteryState.level, batteryStateDesc[batteryState.status]);

            totalFrames = 0;
            elapsed = 0;
        }
    }

	if (cfgf.autosave)
	{
		SaveSram(sramPath);
		free(sramPath);
		free(sramName);

		SaveState(savePath);
		free(savePath);
		free(saveName);
	}
	
    return 0;
}
