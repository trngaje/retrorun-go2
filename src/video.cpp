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

#include "video.h"

#include "libretro.h"

#include <stdlib.h>
#include <stdio.h>
#include <exception>
#include <string.h>

#include <go2/display.h>
#include <go2/input.h>
#include <go2/audio.h>

#define EGL_EGLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <drm/drm_fourcc.h>

#include "config.h"
#include "fonts.h"

// included 9p icons for ES
#include "BatteryIcons.h"
#include "VolumeIcons.h"
#include "OdroidImage.h"
#include "SdUsbIcons.h"
#include "WifiIcons.h"
#include "BrightnessIcon.h"

#define FBO_DIRECT 1
#define ALIGN(val, align)	(((val) + (align) - 1) & ~((align) - 1))

extern float opt_aspect;
extern int opt_backlight;

go2_display_t* display;
go2_surface_t* surface;
go2_surface_t* status_surface;
go2_surface_t* display_surface;
go2_frame_buffer_t* frame_buffer;
go2_presenter_t* presenter;
go2_context_t* context3D;
float aspect_ratio;
uint32_t color_format;

bool isOpenGL = false;
int GLContextMajor = 0;
int GLContextMinor = 0;
GLuint fbo;
int hasStencil = false;
extern bool input_exit_requested;
extern bool input_pause_requested;

bool screenshot_requested = false;
bool snapandexit_requested = false;
int prevBacklight;

extern retro_hw_context_reset_t retro_context_reset;

extern unsigned char ucDPAD_rotate; // 0 (default), 1, 2, 3 = degree 0,90,180,270
unsigned char ucScreen_rotate=0; //trngaje
unsigned char ucRom_rotate=0; 

int go2_w,go2_h;

void video_configure(const struct retro_game_geometry* geom)
{
	printf("video_configure: base_width=%d, base_height=%d, max_width=%d, max_height=%d, aspect_ratio=%f\n",
        geom->base_width, geom->base_height,
        geom->max_width, geom->max_height,
        geom->aspect_ratio);

    
    display = go2_display_create();
    go2_w = go2_display_height_get(display);
    go2_h = go2_display_width_get(display);

    // old
    //presenter = go2_presenter_create(display, DRM_FORMAT_XRGB8888, 0xff080808);  // ABGR
	// new
	presenter = go2_presenter_create(display, DRM_FORMAT_RGB565, 0xff080808);  // ABGR

    if (opt_backlight > -1)
    {
        go2_display_backlight_set(display, (uint32_t)opt_backlight);
    }
    else
    {
        opt_backlight = go2_display_backlight_get(display);
    }
    prevBacklight = opt_backlight;    


    aspect_ratio = opt_aspect; // == 0.0f ? geom->aspect_ratio : opt_aspect;

    if (isOpenGL)
    {        
        go2_context_attributes_t attr;
        attr.major = 3;
        attr.minor = 2;
        attr.red_bits = 5;
        attr.green_bits = 6;
        attr.blue_bits = 5;
        attr.alpha_bits = 0;
        attr.depth_bits = 24;
        attr.stencil_bits = 8;

        context3D = go2_context_create(display, geom->base_width, geom->base_height, &attr);
        //context3D = go2_context_create(display, geom->max_width, geom->max_height, &attr);
        go2_context_make_current(context3D);

#ifndef FBO_DIRECT
#if 0
        GLuint colorBuffer;
        glGenRenderbuffers(1, &colorBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, colorBuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8_OES, geom->max_width, geom->max_height);
#else
        surface = go2_surface_create(display, geom->base_width, geom->base_height, DRM_FORMAT_RGB565);
        if (!surface)
        {
            printf("go2_surface_create failed.\n");
            throw std::exception();
        }

        int drmfd = go2_surface_prime_fd(surface);
        printf("drmfd=%d\n", drmfd);

        EGLint img_attrs[] = {
            EGL_WIDTH, geom->base_width,
            EGL_HEIGHT, geom->base_height,
            EGL_LINUX_DRM_FOURCC_EXT, DRM_FORMAT_RGB565,
            EGL_DMA_BUF_PLANE0_FD_EXT, drmfd,
            EGL_DMA_BUF_PLANE0_OFFSET_EXT, 0,
            EGL_DMA_BUF_PLANE0_PITCH_EXT, go2_surface_stride_get(surface),
            EGL_NONE
        };

        PFNEGLCREATEIMAGEKHRPROC p_eglCreateImageKHR = (PFNEGLCREATEIMAGEKHRPROC)eglGetProcAddress("eglCreateImageKHR");
        if (!p_eglCreateImageKHR) abort();

        EGLImageKHR image = p_eglCreateImageKHR((EGLDisplay)go2_context_egldisplay_get(context3D), EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, 0, img_attrs);
        fprintf(stderr, "EGLImageKHR = %p\n", image);

        GLuint texture2D;
        glGenTextures(1, &texture2D);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture2D);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        PFNGLEGLIMAGETARGETTEXTURE2DOESPROC p_glEGLImageTargetTexture2DOES = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglGetProcAddress("glEGLImageTargetTexture2DOES");
        if (!p_glEGLImageTargetTexture2DOES) abort();

        p_glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image);   
#endif

        GLuint depthBuffer;
        glGenRenderbuffers(1, &depthBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24_OES, geom->max_width, geom->max_height);

        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
#if 0
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorBuffer);
#else
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,	texture2D, 0);
#endif
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);

        GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (fboStatus != GL_FRAMEBUFFER_COMPLETE)
        {
            printf("FBO: Not Complete.\n");
            throw std::exception();
        }
#endif

        retro_context_reset();
    }
    else
    {
        if (surface) abort();

        int aw = ALIGN(geom->max_width, 32);
        int ah = ALIGN(geom->max_height, 32);
        printf ("video_configure: aw=%d, ah=%d\n", aw, ah);

        if (color_format == DRM_FORMAT_RGBA5551)
        {
            surface = go2_surface_create(display, aw, ah, DRM_FORMAT_RGB565);
        }
        else
        {
            surface = go2_surface_create(display, aw, ah, color_format);
        }

        if (!surface)
        {
            printf("go2_surface_create failed.\n");
            throw std::exception();
        }
        
	
        
        //printf("video_configure: rect=%d, %d, %d, %d\n", y, x, h, w);
    }
	
	status_surface = go2_surface_create(display, go2_w/*480*/, go2_h/*320*/, DRM_FORMAT_RGB565);	
	if (!status_surface)
	{
		printf("[trngaje] go2_surface_create failed.:status_surface\n");
		throw std::exception();
	}	
}

void video_deinit()
{

}


uintptr_t core_video_get_current_framebuffer()
{
    //printf("core_video_get_current_framebuffer\n");

#ifndef FBO_DIRECT
    return fbo;
#else
    return 0;
#endif
}


extern int fps;
extern go2_battery_state_t batteryState;
extern int opt_backlight;
extern int opt_volume;

void showbattery(int x, int y)
{
	// Battery level
	const uint8_t* src = battery_image.pixel_data;
	int src_stride = 32 * sizeof(short);

	uint8_t* dst = (uint8_t*)go2_surface_map(status_surface);
	int dst_stride = go2_surface_stride_get(status_surface);

	//go2_battery_state_t batteryState;
	//go2_input_battery_read(input, &batteryState);

	int batteryIndex;
	if (batteryState.level == 1)
	{
		batteryIndex = 0;
	}
	else if (batteryState.level <= 5)
	{
		batteryIndex = 1;
	}
	else if (batteryState.level <= 10)
	{
		batteryIndex = 2;
	}
	else if (batteryState.level <= 15)
	{
		batteryIndex = 3;
	}
	else if (batteryState.level <= 20)
	{
		batteryIndex = 4;
	}
	else if (batteryState.level <= 25)
	{
		batteryIndex = 5;
	}
	else if (batteryState.level <= 30)
	{
		batteryIndex = 6;
	}
	else if (batteryState.level <= 35)
	{
		batteryIndex = 7;
	}
	else if (batteryState.level <= 40)
	{
		batteryIndex = 8;
	}
	else if (batteryState.level <= 45)
	{
		batteryIndex = 9;
	}
	else if (batteryState.level <= 50)
	{
		batteryIndex = 10;
	}
	else if (batteryState.level <= 55)
	{
		batteryIndex = 11;
	}
	else if (batteryState.level <= 60)
	{
		batteryIndex = 12;
	}
	else if (batteryState.level <= 65)
	{
		batteryIndex = 13;
	}
	else if (batteryState.level <= 70)
	{
		batteryIndex = 14;
	}
	else if (batteryState.level <= 75)
	{
		batteryIndex = 15;
	}
	else if (batteryState.level <= 80)
	{
		batteryIndex = 16;
	}
	else if (batteryState.level <= 85)
	{
		batteryIndex = 17;
	}
	else if (batteryState.level <= 90)
	{
		batteryIndex = 18;
	}
	else if (batteryState.level <= 95)
	{
		batteryIndex = 19;
	}
	else if (batteryState.level == 100)
	{
		batteryIndex = 20;
	}				
	else
	{
		batteryIndex = 20;
	}
	
	src += (batteryIndex * 16 * src_stride);
	dst += x * sizeof(short) + y * dst_stride;

	for (int y = 0; y < 16; ++y)
	{
		memcpy(dst, src, 32 * sizeof(short));

		src += src_stride;
		dst += dst_stride;
	}
}

void showvolume(int x, int y)
{
	// Volume level
	const uint8_t*  src = (uint8_t *)(volume_image.pixel_data);
	int src_stride = 32 * sizeof(short);

	uint8_t* dst = (uint8_t*)go2_surface_map(status_surface);
	int dst_stride = go2_surface_stride_get(status_surface);

	uint32_t volume = opt_volume; //go2_audio_volume_get(NULL);

	int volumeIndex;
	if (volume == 0)
	{
		volumeIndex = 0;
	}
	else if (volume <= 5)
	{
		volumeIndex = 1;
	}
	else if (volume <= 10)
	{
		volumeIndex = 2;
	}
	else if (volume <= 15)
	{
		volumeIndex = 3;
	}
	else if (volume <= 20)
	{
		volumeIndex = 4;
	}
	else if (volume <= 25)
	{
		volumeIndex = 5;
	}
	else if (volume <= 30)
	{
		volumeIndex = 6;
	}
	else if (volume <= 35)
	{
		volumeIndex = 7;
	}
	else if (volume <= 40)
	{
		volumeIndex = 8;
	}
	else if (volume <= 45)
	{
		volumeIndex = 9;
	}
	else if (volume <= 50)
	{
		volumeIndex = 10;
	}
	else if (volume <= 55)
	{
		volumeIndex = 11;
	}
	else if (volume <= 60)
	{
		volumeIndex = 12;
	}
	else if (volume <= 65)
	{
		volumeIndex = 13;
	}
	else if (volume <= 70)
	{
		volumeIndex = 14;
	}
	else if (volume <= 75)
	{
		volumeIndex = 15;
	}
	else if (volume <= 80)
	{
		volumeIndex = 16;
	}
	else if (volume <= 85)
	{
		volumeIndex = 17;
	}
	else if (volume <= 90)
	{
		volumeIndex = 18;
	}
	else if (volume <= 95)
	{
		volumeIndex = 19;
	}
	else if (volume == 100)
	{
		volumeIndex = 20;
	}
	else
	{
		volumeIndex = 20;
	}
	
	src += (volumeIndex * 16 * src_stride);
	dst += x * sizeof(short) + y * dst_stride;

	for (int y = 0; y < 16; ++y)
	{
		memcpy(dst, src, 32 * sizeof(short));

		src += src_stride;
		dst += dst_stride;
	}

	
}

void showbrightness(int x, int y)
{
	

	// Brightness level
	const uint8_t* src = brightness_image.pixel_data;
	int src_stride = 32 * sizeof(short);

	uint8_t* dst = (uint8_t*)go2_surface_map(status_surface);
	int dst_stride = go2_surface_stride_get(status_surface);

	int brightnessIndex = 0;
	int brightness = 0;
#if 0
	int fd;
	char buffer[10];
	fd = open("/sys/class/backlight/backlight/brightness", O_RDONLY);
	if (fd > 0)
	{
		memset(buffer, 0, 10);
		ssize_t count = read(fd, buffer, 10);
		if( count > 0 )
		{
			brightness = atoi(buffer);
			brightness = brightness*100/255;
		}
		close(fd);
	}
#endif			
	brightness = opt_backlight;

	if (brightness == 0)
	{
		brightnessIndex = 0;
	}
	else if (brightness <= 5)
	{
		brightnessIndex = 1;
	}
	else if (brightness <= 10)
	{
		brightnessIndex = 2;
	}
	else if (brightness <= 15)
	{
		brightnessIndex = 3;
	}
	else if (brightness <= 20)
	{
		brightnessIndex = 4;
	}
	else if (brightness <= 25)
	{
		brightnessIndex = 5;
	}
	else if (brightness <= 30)
	{
		brightnessIndex = 6;
	}
	else if (brightness <= 35)
	{
		brightnessIndex = 7;
	}
	else if (brightness <= 40)
	{
		brightnessIndex = 8;
	}
	else if (brightness <= 45)
	{
		brightnessIndex = 9;
	}
	else if (brightness <= 50)
	{
		brightnessIndex = 10;
	}
	else if (brightness <= 55)
	{
		brightnessIndex = 11;
	}
	else if (brightness <= 60)
	{
		brightnessIndex = 12;
	}
	else if (brightness <= 65)
	{
		brightnessIndex = 13;
	}
	else if (brightness <= 70)
	{
		brightnessIndex = 14;
	}
	else if (brightness <= 75)
	{
		brightnessIndex = 15;
	}
	else if (brightness <= 80)
	{
		brightnessIndex = 16;
	}
	else if (brightness <= 85)
	{
		brightnessIndex = 17;
	}
	else if (brightness <= 90)
	{
		brightnessIndex = 18;
	}
	else if (brightness <= 95)
	{
		brightnessIndex = 19;
	}
	else if (brightness == 100)
	{
		brightnessIndex = 20;
	}
	else
	{
		brightnessIndex = 20;
	}
	
	src += (brightnessIndex * 16 * src_stride);
	dst += x * sizeof(short) + y * dst_stride;

	for (int y = 0; y < 16; ++y)
	{
		memcpy(dst, src, 32 * sizeof(short));

		src += src_stride;
		dst += dst_stride;
	}
		
}

void showstatus(int x, int y, int mode)
{
	// display texts for current status
	uint8_t* dst = (uint8_t*)go2_surface_map(status_surface);
	int dst_stride = go2_surface_stride_get(status_surface);
	
	if (input_pause_requested)
	{
		if (mode == 0)
			basic_text_out16(dst, dst_stride/2/*go2_w*//*480*//*width*/, x, y, "F:%d, B:%d, L:%d, V:%d <paused>", fps, batteryState.level, opt_backlight, opt_volume);
		else if (mode == 1)
			basic_text_out16(dst, dst_stride/2/*go2_w*//*480*//*width*/, x, y, "F:%d <paused>",  fps);			
	}
	else
	{
		if (mode == 0)
			basic_text_out16(dst, dst_stride/2/*go2_w*//*480*//*width*/, x, y, "F:%d, B:%d, L:%d, V:%d", fps, batteryState.level, opt_backlight, opt_volume);
		else if (mode == 1)
			basic_text_out16(dst, dst_stride/2/*go2_w*//*480*//*width*/, x, y, "F:%d",  fps);	
	}
}


void core_video_refresh(const void * data, unsigned width, unsigned height, size_t pitch)
{
    //printf("core_video_refresh: data=%p, width=%d, height=%d, pitch=%d\n", data, width, height, pitch);

    if (opt_backlight != prevBacklight)
    {
        go2_display_backlight_set(display, (uint32_t)opt_backlight);
        prevBacklight = opt_backlight;

        //printf("Backlight = %d\n", opt_backlight);
    }


    int x;
    int y;
    int w;
    int h;
	
    if (aspect_ratio >= 1.0f)
    {
        h = go2_display_width_get(display);
        
        w = h * aspect_ratio;
        w = (w > go2_display_height_get(display)) ? go2_display_height_get(display) : w;

        x = (go2_display_height_get(display) / 2) - (w / 2);
        y = 0;

        //printf("x=%d, y=%d, w=%d, h=%d\n", x, y, w, h);
    }
    else
    {
        x = 0;
        y = 0;
        w = go2_display_height_get(display);
        h = go2_display_width_get(display);
    }


    if (isOpenGL) // n64, saturn core excute this code
    {
        if (data != RETRO_HW_FRAME_BUFFER_VALID) return;
        
#if 0
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        
        glClearColor(1, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif


#ifdef FBO_DIRECT
        // Swap
        go2_context_swap_buffers(context3D);

        go2_surface_t* gles_surface = go2_context_surface_lock(context3D);

        int ss_w = go2_surface_width_get(gles_surface);
        int ss_h = go2_surface_height_get(gles_surface);
		

        if (screenshot_requested || snapandexit_requested)
        {
            printf("Screenshot.\n");

            //int ss_w = go2_surface_width_get(gles_surface);
            //int ss_h = go2_surface_height_get(gles_surface);
            //go2_surface_t* screenshot = go2_surface_create(display, ss_w, ss_h, DRM_FORMAT_RGB888);
			
			go2_surface_t* screenshot;
			if (ucRom_rotate == 0 || ucRom_rotate == 2)
				screenshot = go2_surface_create(display, w, h, DRM_FORMAT_RGB888);
			else if (ucRom_rotate == 1 || ucRom_rotate == 3)
				screenshot = go2_surface_create(display, h, w, DRM_FORMAT_RGB888);
			
            if (!screenshot)
            {
                printf("go2_surface_create failed.\n");
                throw std::exception();
            }


			if (ucRom_rotate == 0)
			{
				go2_surface_blit(gles_surface, 0, 0, width, height,
								 screenshot, 0, 0, w, h,
								 GO2_ROTATION_DEGREES_0);
			}
			else if (ucRom_rotate == 2)
			{
				go2_surface_blit(gles_surface, 0, 0, width, height,
								 screenshot, 0, 0, w, h,
								 GO2_ROTATION_DEGREES_180);
			}
			else if (ucRom_rotate == 1)
			{
				go2_surface_blit(gles_surface, 0, 0, width, height,
								 screenshot, 0, 0, h, w,
								 GO2_ROTATION_DEGREES_270);				
			}
			else if (ucRom_rotate == 3)
			{
				go2_surface_blit(gles_surface, 0, 0, width, height,
								 screenshot, 0, 0, h, w,
								 GO2_ROTATION_DEGREES_90);				
			}		


			if (snapandexit_requested)
			{
				// snap in rom directory
				go2_surface_save_as_png(screenshot, cfgf.csnapnamewithrompath);		
			}
			else 
			{
				// user defined path
				char cSnapshotFile[256];
				sprintf(cSnapshotFile, "%s/%s.png", cfgf.cdir_snapshot, cfgf.cromname);
				printf("cSnapshotFile=%s\n", cSnapshotFile);
				go2_surface_save_as_png(screenshot, cSnapshotFile);				
			}
			


            go2_surface_destroy(screenshot);

            screenshot_requested = false;
			
			if (snapandexit_requested)
			{
				snapandexit_requested = false;
				input_exit_requested = true;	// exit
			}
		}

		// clear status_surface
		{
			uint8_t* dst = (uint8_t*)go2_surface_map(status_surface);

			memset(dst, 0x0000, sizeof(short) * go2_w * go2_h /*480 * 320*/);
		}
		
		//status_surface
		go2_surface_blit(gles_surface, 0, 0, width, height, status_surface, x, y, w, h,
							 go2_rotation_t((4-ucScreen_rotate) % 4));

#if 0
		// display texts for current status
		{
			uint8_t* dst = (uint8_t*)go2_surface_map(status_surface);
			
			basic_text_out16(dst, go2_w/*480*/ /*width*/, 0, 32, "F:%d, B:%d, L:%d, V:%d", 
				fps, batteryState.level, opt_backlight, opt_volume);	
		}			
#endif

        go2_context_surface_unlock(context3D, gles_surface);
		
#if 0		
        go2_presenter_post(presenter,
                    gles_surface,
                    0, 0, width, height,
                    y, x, h, w,
                    GO2_ROTATION_DEGREES_270);
#endif
		
		if (snapandexit_requested)
		{
		//	snapandexit_requested = false;
		//	input_exit_requested = true;	// exit
		}		
 #else
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        go2_presenter_post(presenter,
                    surface,
                    0, 0, width, height,
                    y, x, h, w,
                    GO2_ROTATION_DEGREES_90);
#endif
        //glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    }
    else
    {
        if (!data) return;

        uint8_t* src = (uint8_t*)data;
        uint8_t* dst = (uint8_t*)go2_surface_map(surface);
        int bpp = go2_drm_format_get_bpp(go2_surface_format_get(surface)) / 8;

        int yy = height;
        while(yy > 0)
        {
            if (color_format == DRM_FORMAT_RGBA5551)
            {
                // uint16_t* src2 = (uint16_t*)src;
                // uint16_t* dst2 = (uint16_t*)dst;

                uint32_t* src2 = (uint32_t*)src;
                uint32_t* dst2 = (uint32_t*)dst;

                for (int x = 0; x < width / 2; ++x)
                {
                    // uint16_t pixel = src2[x];
                    // pixel = (pixel << 1) & (~0x1f) | pixel & 0x1f;
                    // dst2[x] = pixel;

                    uint32_t pixel = src2[x];
                    pixel = (pixel << 1) & (~0x1f001f) | pixel & 0x1f001f;
                    dst2[x] = pixel;
                }
            }
            else
            {
                memcpy(dst, src, width * bpp);
            }
            
            src += pitch;
            dst += go2_surface_stride_get(surface);
            
            --yy;
        }

        if (screenshot_requested || snapandexit_requested)
        {
            printf("Screenshot.\n");

            //int ss_w = go2_surface_width_get(surface);
            //int ss_h = go2_surface_height_get(surface);
            //go2_surface_t* screenshot = go2_surface_create(display, ss_w, ss_h, DRM_FORMAT_RGB888);
			
			go2_surface_t* screenshot;
			if (ucRom_rotate == 0 || ucRom_rotate == 2)
				screenshot = go2_surface_create(display, w, h, DRM_FORMAT_RGB888);
			else if (ucRom_rotate == 1 || ucRom_rotate == 3)
				screenshot = go2_surface_create(display, h, w, DRM_FORMAT_RGB888);
			
            if (!screenshot)
            {
                printf("go2_surface_create failed.\n");
                throw std::exception();
            }
#if 0
            go2_surface_blit(surface, 0, 0, ss_w, ss_h,
                             screenshot, 0, 0, ss_w, ss_h,
                             GO2_ROTATION_DEGREES_0);
#endif


			if (ucRom_rotate == 0)
			{
				go2_surface_blit(surface, 0, 0, width, height,
								 screenshot, 0, 0, w, h,
								 GO2_ROTATION_DEGREES_0);
			}
			else if (ucRom_rotate == 2)
			{
				go2_surface_blit(surface, 0, 0, width, height,
								 screenshot, 0, 0, w, h,
								 GO2_ROTATION_DEGREES_180);
			}
			else if (ucRom_rotate == 1)
			{
				go2_surface_blit(surface, 0, 0, width, height,
								 screenshot, 0, 0, h, w,
								 GO2_ROTATION_DEGREES_270);				
			}
			else if (ucRom_rotate == 3)
			{
				go2_surface_blit(surface, 0, 0, width, height,
								 screenshot, 0, 0, h, w,
								 GO2_ROTATION_DEGREES_90);				
			}		


			if (snapandexit_requested)
			{
				// snap in rom directory
				go2_surface_save_as_png(screenshot, cfgf.csnapnamewithrompath);		
			}
			else 
			{
				// user defined path
				char cSnapshotFile[256];
				sprintf(cSnapshotFile, "%s/%s.png", cfgf.cdir_snapshot, cfgf.cromname);
				printf("cSnapshotFile=%s\n", cSnapshotFile);
				go2_surface_save_as_png(screenshot, cSnapshotFile);				
			}
			


            go2_surface_destroy(screenshot);

            screenshot_requested = false;
			
			if (snapandexit_requested)
			{
				snapandexit_requested = false;
				input_exit_requested = true;	// exit
			}
        }

		// clear status_surface
		{
			uint8_t* dst = (uint8_t*)go2_surface_map(status_surface);

			memset(dst, 0x0000, sizeof(short) * go2_w * go2_h /*480 * 320*/);
		}
		
		//status_surface
		go2_surface_blit(surface, 0, 0, width, height, status_surface, x, y, w, h,
							 go2_rotation_t((4-ucScreen_rotate) % 4));
		//printf("bpp=%d, width=%d, height=%d, w=%d, h=%d\n", bpp, width, height, h, w);
		
	}	

	if (cfgf.show_status_texts > 0)
	{
		//showstatus(0, 32, 1); // x, y, mode(1):fps only
		showstatus(cfgf.status_x, cfgf.status_y, 0); // x, y, mode(1):fps only
	}
	
	if (cfgf.show_status_icons > 0)
	{
#if 0
		showbattery(go2_w/*480*/ - 32, 0);
		showvolume(0, 0);
		showbrightness(64, 0);
#endif
		showbattery(cfgf.icon_battery_x, cfgf.icon_battery_y);
		showvolume(cfgf.icon_volume_x, cfgf.icon_volume_y);
		showbrightness(cfgf.icon_brightness_x, cfgf.icon_brightness_y);
	}

	
#if 0		
        go2_presenter_post(presenter,
                           surface,
                           0, 0, width, height,
                           y, x, h, w,
                           go2_rotation_t((4+(unsigned char)GO2_ROTATION_DEGREES_270-ucScreen_rotate) % 4));
#else
        go2_presenter_post(presenter,
                           status_surface,
                           0, 0, go2_w, go2_h, /*480, 320,*/
                           0, 0, go2_h, go2_w, /*320, 480,*/
                           GO2_ROTATION_DEGREES_270);
	
#endif

    //}
}



