////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2018 - 2020 Dusan Jocic <dusanjocic@msn.com>
//
// This file is part of OpenWolf.
//
// OpenWolf is free software; you can redistribute it
// and / or modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the License,
// or (at your option) any later version.
//
// OpenWolf is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OpenWolf; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110 - 1301  USA
//
// -------------------------------------------------------------------------------------
// File name:   precompiled.h
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __R_PRECOMPILED_H__
#define __R_PRECOMPILED_H__

#pragma once

//Dushan
//FIX ALL THIS
#include <emmintrin.h>
#include <xmmintrin.h>
#ifdef _WIN32
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

#include <fcntl.h>
#include <algorithm>

#include <signal.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>

#include <iostream>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <float.h>

#ifndef _WIN32
#include <sys/ioctl.h>
#endif

#ifdef __LINUX_
#include <stdint.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <pwd.h>
#include <libgen.h>
#include <fcntl.h>
#include <fenv.h>
#endif

#ifdef _WIN32
#include <SDL_syswm.h>
#include <io.h>
#include <shellapi.h>
#include <timeapi.h>
#include <windows.h>
#include <direct.h>
#include <lmerr.h>
#include <lmcons.h>
#include <lmwksta.h>
#include <conio.h>
#include <wincrypt.h>
#include <shlobj.h>
#include <psapi.h>
#include <float.h>
#include <Shobjidl.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ws2spi.h>
#include <platform/windows/resource.h>
#pragma fenv_access (on)
#else
#include <sys/types.h>
#include <dirent.h>
#include <libgen.h>
#include <fenv.h>
#include <pwd.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <ifaddrs.h>
#endif

#ifdef _WIN32
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_opengl_glext.h>
#include <SDL_thread.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_opengl_glext.h>
#include <SDL2/SDL_thread.h>
#endif

#ifdef _WIN32
#include <freetype/ft2build.h>
#else
#include <freetype2/ft2build.h>
#endif
#undef getch

#ifdef __LINUX__
//hack
typedef int boolean;
#endif
extern "C"
{
#include <jpeglib.h>
}

#include <framework/appConfig.h>
#include <renderSystem/qgl.h>
#include <framework/types.h>
#include <framework/SurfaceFlags_Tech3.h>
#include <qcommon/q_platform.h>
#include <qcommon/q_shared.h>
#include <framework/LibraryTemplates.hpp>
#include <qcommon/qfiles.h>
#include <API/threads_api.h>
#include <framework/Threads.h>
#include <API/Memory_api.h>
#include <API/cm_api.h>
#include <API/CmdBuffer_api.h>
#include <API/CmdSystem_api.h>
#include <API/system_api.h>
#include <API/FileSystem_api.h>
#include <API/CVarSystem_api.h>
#include <qcommon/qcommon.h>
#include <API/clientAVI_api.h>
#include <API/clientMain_api.h>
#include <API/renderer_api.h>

#include <renderSystem/iqm.h>
#include <renderSystem/r_extramath.h>
#include <renderSystem/r_image.h>
#include <renderSystem/r_fbo.h>
#include <renderSystem/r_local.h>
#include <renderSystem/r_backend.h>
#include <renderSystem/r_cmdsTemplate.hpp>
#include <renderSystem/r_animation.h>
#include <renderSystem/r_postprocess.h>
#include <renderSystem/r_bsp_tech3.h>
#include <renderSystem/r_cmds.h>
#include <renderSystem/r_curve.h>
#include <renderSystem/r_dsa.h>
#include <renderSystem/r_extensions.h>
#include <renderSystem/r_flares.h>
#include <renderSystem/r_font.h>
#include <renderSystem/r_glimp.h>
#include <renderSystem/r_glsl.h>
#include <renderSystem/r_image_dds.h>
#include <renderSystem/r_image_jpg.h>
#include <renderSystem/r_image_png.h>
#include <renderSystem/r_image_tga.h>
#include <renderSystem/r_init.h>
#include <renderSystem/r_light.h>
#include <renderSystem/r_main.h>
#include <renderSystem/r_marks.h>
#include <renderSystem/r_mesh.h>
#include <renderSystem/r_model.h>
#include <renderSystem/r_model_iqm.h>
#include <renderSystem/r_splash.h>
#include <renderSystem/r_noise.h>
#include <renderSystem/r_extratypes.h>
#include <renderSystem/r_scene.h>
#include <renderSystem/r_shade.h>
#include <renderSystem/r_shader.h>
#include <renderSystem/r_surface.h>
#include <renderSystem/r_shadows.h>
#include <renderSystem/r_skins.h>
#include <renderSystem/r_sky.h>
#include <renderSystem/r_vao.h>
#include <renderSystem/r_world.h>

#include <framework/Puff.h>
#include <framework/SurfaceFlags_Tech3.h>
#include <API/system_api.h>

#include <API/systemThreads_api.h>

#endif //!__R_PRECOMPILED_H__
