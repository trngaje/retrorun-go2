#!/bin/bash
CONFIG_FILE_PATH=/home/odroid/develop/retrorun-go2/retrorun.cfg

if [ "$1" = "fbneo" ]; then
CORE_PATH=/home/odroid/.config/retroarch/cores/fbneo_libretro.so
elif [ "$1" = "fbalpha2012_neogeo" ]; then
CORE_PATH=/home/odroid/.config/retroarch/cores/fbalpha2012_neogeo_libretro.so
elif [ "$1" = "mame2003_plus" ]; then
CORE_PATH=/home/odroid/.config/retroarch/cores/mame2003_plus_libretro.so
elif [ "$1" = "mame2010" ]; then
CORE_PATH=/home/odroid/.config/retroarch/cores/mame2010_libretro.so
elif [ "$1" = "mame2015" ]; then
CORE_PATH=/home/odroid/.config/retroarch/cores/mame2015_libretro.so
elif [ "$1" = "hbmame" ]; then
CORE_PATH=/home/odroid/.config/retroarch/cores/hbmame_libretro.so
elif [ "$1" = "bluemsx" ]; then
CORE_PATH=/home/odroid/.config/retroarch/cores/bluemsx_libretro.so
elif [ "$1" = "snes9x2010" ]; then
CORE_PATH=/home/odroid/.config/retroarch/cores/snes9x2010_libretro.so
elif [ "$1" = "snes9x" ]; then
CORE_PATH=/home/odroid/.config/retroarch/cores/snes9x_libretro.so

#atomiswave
elif [ "$1" = "atomiswave" ]; then
CORE_PATH=/home/odroid/.config/retroarch/cores/flycast_libretro.so

#nes
elif [ "$1" = "fceumm" ]; then
CORE_PATH=/home/odroid/.config/retroarch/cores/fceumm_libretro.so
elif [ "$1" = "nestopia" ]; then
CORE_PATH=/home/odroid/.config/retroarch/cores/nestopia_libretro.so

#gb gbc
elif [ "$1" = "gambatte" ]; then
CORE_PATH=/home/odroid/.config/retroarch/cores/gambatte_libretro.so
elif [ "$1" = "genesis_plus_gx" ]; then
CORE_PATH=/home/odroid/.config/retroarch/cores/genesis_plus_gx_libretro.so

#gba
elif [ "$1" = "gpsp" ]; then
CORE_PATH=/home/odroid/.config/retroarch/cores/gpsp_libretro.so
elif [ "$1" = "mgba" ]; then
CORE_PATH=/home/odroid/.config/retroarch/cores/mgba_libretro.so
elif [ "$1" = "vba_next" ]; then
CORE_PATH=/home/odroid/.config/retroarch/cores/vba_next_libretro.so

#n64
#elif [ "$1" = "glupen64" ]; then
#32bit ?
#CORE_PATH=/home/odroid/.config/retroarch/cores/glupen64_libretro.so
elif [ "$1" = "parallel_n64" ]; then
CORE_PATH=/home/odroid/.config/retroarch/cores/parallel_n64_libretro.so

#dreamcast
elif [ "$1" = "flycast" ]; then
CORE_PATH=/home/odroid/.config/retroarch/cores/flycast_libretro.so
elif [ "$1" = "dreamcast" ]; then
CORE_PATH=/home/odroid/.config/retroarch/cores/flycast_libretro.so

#saturn
elif [ "$1" = "yabasanshiro" ]; then
CORE_PATH=/home/odroid/.config/retroarch/cores/yabasanshiro_libretro.so

#pcengine
elif [ "$1" = "mednafen_pce_fast" ]; then
CORE_PATH=/home/odroid/.config/retroarch/cores/mednafen_pce_fast_libretro.so
elif [ "$1" = "mednafen_supergrafx" ]; then
CORE_PATH=/home/odroid/.config/retroarch/cores/mednafen_supergrafx_libretro.so
elif [ "$1" = "mednafen_pcfx" ]; then
CORE_PATH=/home/odroid/.config/retroarch/cores/mednafen_pcfx_libretro.so
elif [ "$1" = "atari800" ]; then
CORE_PATH=/home/odroid/.config/retroarch/cores/atari800_libretro.so
#elif [ "$1" = "desmume2015" ]; then
#32bit?
#nds
#CORE_PATH=/home/odroid/.config/retroarch/cores/desmume2015_libretro.so
elif [ "$1" = "dosbox" ]; then
CORE_PATH=/home/odroid/.config/retroarch/cores/dosbox_libretro.so
elif [ "$1" = "dosbox_svn" ]; then
CORE_PATH=/home/odroid/.config/retroarch/cores/dosbox_svn_libretro.so
elif [ "$1" = "easyrpg" ]; then
CORE_PATH=/home/odroid/.config/retroarch/cores/easyrpg_libretro.so
elif [ "$1" = "nx" ]; then
CORE_PATH=/home/odroid/.config/retroarch/cores/nxengine_libretro.so
#Script Creation Utility for Maniac Mansion Virtual Machine
elif [ "$1" = "scummvm" ]; then
CORE_PATH=/home/odroid/.config/retroarch/cores/scummvm_libretro.so

#atari lynx
elif [ "$1" = "handy" ]; then
CORE_PATH=/home/odroid/.config/retroarch/cores/handy_libretro.so
elif [ "$1" = "mednafen_lynx" ]; then
CORE_PATH=/home/odroid/.config/retroarch/cores/mednafen_lynx_libretro.so

#atari2600
elif [ "$1" = "stella2014" ]; then
CORE_PATH=/home/odroid/.config/retroarch/cores/stella2014_libretro.so

#atari7800
elif [ "$1" = "prosystem" ]; then
CORE_PATH=/home/odroid/.config/retroarch/cores/prosystem_libretro.so

#neogeo pocket
elif [ "$1" = "mednafen_ngp" ]; then
CORE_PATH=/home/odroid/.config/retroarch/cores/mednafen_ngp_libretro.so

#virtual boy
elif [ "$1" = "mednafen_vb" ]; then
CORE_PATH=/home/odroid/.config/retroarch/cores/mednafen_vb_libretro.so

#wonder swan
elif [ "$1" = "mednafen_wswan" ]; then
CORE_PATH=/home/odroid/.config/retroarch/cores/mednafen_wswan_libretro.so

#NEC PC98
elif [ "$1" = "nekop2" ]; then
CORE_PATH=/home/odroid/.config/retroarch/cores/nekop2_libretro.so

#pc8801
elif [ "$1" = "quasi88" ]; then
CORE_PATH=/home/odroid/.config/retroarch/cores/quasi88_libretro.so

#ps2
elif [ "$1" = "vecx" ]; then
CORE_PATH=/home/odroid/.config/retroarch/cores/vecx_libretro.so

#psx
elif [ "$1" = "duckstation" ]; then
CORE_PATH=/home/odroid/.config/retroarch/cores/duckstation_libretro.so

#commodore 64
elif [ "$1" = "vice_x64" ]; then
CORE_PATH=/home/odroid/.config/retroarch/cores/vice_x64_libretro.so
elif [ "$1" = "x1" ]; then
CORE_PATH=/home/odroid/.config/retroarch/cores/x1_libretro.so

fi

/home/odroid/develop/retrorun-go2/retrorun -c $CONFIG_FILE_PATH -s /home/odroid/retrorundata -d /roms/bios $CORE_PATH "$2" 

 
