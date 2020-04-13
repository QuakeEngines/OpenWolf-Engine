#
# CMakeLists.txt
#
# cmake required (http://www.cmake.org/)
#

cmake_minimum_required( VERSION 2.8 )

if( POLICY CMP0017 )
  cmake_policy( SET CMP0017 NEW )
endif()

include( CMakeDependentOption )
include( cmake/precompiledHeaders.cmake )

set( GAME_APP_NAME "" CACHE            STRING "The Application name")
set( GAME_APP_VERSION "" CACHE         STRING "The Application version")
set( GAME_APP_STAGE "" CACHE           STRING "The Application stage")

if("${GAME_APP_NAME}" STREQUAL "")
	message(FATAL_ERROR "Please set GAME_APP_NAME first")
endif()

if("${GAME_APP_VERSION}" STREQUAL "")
	message(FATAL_ERROR "Please set GAME_APP_VERSION first")
endif()

if("${GAME_APP_STAGE}" STREQUAL "")
	message(FATAL_ERROR "Please set GAME_APP_STAGE first")
endif()

project(${GAME_APP_NAME} C CXX)

# This line has to appear before 'PROJECT' in order to be able to disable incremental linking
set( MSVC_INCREMENTAL_DEFAULT ON )

set( CMAKE_BUILD_TYPE Release CACHE STRING "Build configuration (Debug, Release, RelWithDebInfo, MinSizeRel)" )

if( MSVC )
  set( CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /fp:precise" )
  set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /LARGEADDRESSAWARE" )
  set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LARGEADDRESSAWARE /SUBSYSTEM:CONSOLE" )
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" )
else()
  set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse4.1 -pipe -Wno-write-strings -Wno-narrowing -fpermissive -Wsizeof-array-argument -I/usr/include/freetype2" )
  set( CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g" )
  set( CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} -DNDEBUG" )
  set( CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DNDEBUG -ffast-math -fomit-frame-pointer -fno-strict-aliasing" )
  set( CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -DNDEBUG -ffast-math -fomit-frame-pointer -fno-strict-aliasing -g" )
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif()

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS}" )
set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${CMAKE_C_FLAGS_DEBUG}" )
set( CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} ${CMAKE_C_FLAGS_MINSIZEREL}" )
set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${CMAKE_C_FLAGS_RELEASE}" )
set( CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${CMAKE_C_FLAGS_RELWITHDEBINFO}" )
set( CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "" ) # Need this to get rid of -rdynamic linker flag set by default by cmake
set( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" )

# Options
option( BUILD_CLIENT                   "Build Application client"                          ON )
option( BUILD_SERVER                   "Build Application dedicated server"                ON )
option( BUILD_AUTOUPDATE_SERVER        "Build Application AutoUpdate server"               ON )
option( BUILD_MASTER_SERVER            "Build master server"                               ON )
option( BUILD_OWMAP                    "Build Mapping tool"                                ON )

# Package info
set( CPACK_PACKAGE_DESCRIPTION_SUMMARY "Application client" )
set( CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.txt" )
set( CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/GPL.txt" )
include( CPack )

set( MOUNT_DIR ${CMAKE_SOURCE_DIR}/src/engine )
set( LIB_DIR ${CMAKE_SOURCE_DIR}/src/libs )
set( TOOLS_DIR ${CMAKE_SOURCE_DIR}/src/tools )

message(STATUS "writing ${MOUNT_DIR}/framework/appConfig.h")
CONFIGURE_FILE("cmake/appConfig.h.in" "${MOUNT_DIR}/framework/appConfig.h" @ONLY)

find_package( PkgConfig )

if( MSVC )
	add_definitions( -D_CRT_SECURE_NO_WARNINGS )
endif()

set( CONSOLE "tty" )

# Determine platform
if( MSVC )
	set( PLATFORM "win32" )
	set( CONSOLE "win32" )
elseif( UNIX )
	if( ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" )
		add_definitions( -DMACOS_X -DMACOS_X_ACCELERATION_HACK -D USE_FILE32API -DMACOS_X_CURSOR_HACK )
	endif()
	set( PLATFORM "unix" )
endif()

# Determine arch
if( ${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64" )
	set( BUILD_ARCH "AMD64" )
else()
	set( BUILD_ARCH ${CMAKE_SYSTEM_PROCESSOR} )
endif()

######################
# Define Build Files #
######################

set( SHAREDLIST_HEADERS
	${MOUNT_DIR}/qcommon/q_platform.h
	${MOUNT_DIR}/qcommon/q_shared.h
)

set( SHAREDLIST_SOURCES
	${MOUNT_DIR}/qcommon/q_shared.cpp
	${MOUNT_DIR}/qcommon/q_math.cpp
)

set( RENDERSYSTEM_HEADERS 
	${MOUNT_DIR}/framework/Puff.h
	${MOUNT_DIR}/renderSystem/r_precompiled.h
	${MOUNT_DIR}/renderSystem/iqm.h
	${MOUNT_DIR}/renderSystem/qgl.h
	${MOUNT_DIR}/renderSystem/r_animation.h
	${MOUNT_DIR}/renderSystem/r_bsp_tech3.h
	${MOUNT_DIR}/renderSystem/r_cmds.h
	${MOUNT_DIR}/renderSystem/r_curve.h
	${MOUNT_DIR}/renderSystem/r_dsa.h
	${MOUNT_DIR}/renderSystem/r_extensions.h
	${MOUNT_DIR}/renderSystem/r_extramath.h
	${MOUNT_DIR}/renderSystem/r_extratypes.h
	${MOUNT_DIR}/renderSystem/r_fbo.h
	${MOUNT_DIR}/renderSystem/r_flares.h
	${MOUNT_DIR}/renderSystem/r_font.h
	${MOUNT_DIR}/renderSystem/r_glimp.h
	${MOUNT_DIR}/renderSystem/r_glsl.h
	${MOUNT_DIR}/renderSystem/r_image.h
	${MOUNT_DIR}/renderSystem/r_image_dds.h
	${MOUNT_DIR}/renderSystem/r_image_jpg.h
	${MOUNT_DIR}/renderSystem/r_image_png.h
	${MOUNT_DIR}/renderSystem/r_image_tga.h
	${MOUNT_DIR}/renderSystem/r_init.h
	${MOUNT_DIR}/renderSystem/r_light.h
	${MOUNT_DIR}/renderSystem/r_local.h
	${MOUNT_DIR}/renderSystem/r_main.h
	${MOUNT_DIR}/renderSystem/r_marks.h
	${MOUNT_DIR}/renderSystem/r_mesh.h
	${MOUNT_DIR}/renderSystem/r_model.h
	${MOUNT_DIR}/renderSystem/r_model_iqm.h
	${MOUNT_DIR}/renderSystem/r_noise.h
	${MOUNT_DIR}/renderSystem/r_postprocess.h
	${MOUNT_DIR}/renderSystem/r_scene.h
	${MOUNT_DIR}/renderSystem/r_shade.h
	${MOUNT_DIR}/renderSystem/r_shader.h
	${MOUNT_DIR}/renderSystem/r_shadows.h
	${MOUNT_DIR}/renderSystem/r_skins.h
	${MOUNT_DIR}/renderSystem/r_sky.h
	${MOUNT_DIR}/renderSystem/r_splash.h
	${MOUNT_DIR}/renderSystem/r_surface.h
	${MOUNT_DIR}/renderSystem/r_types.h
	${MOUNT_DIR}/renderSystem/r_vao.h
	${MOUNT_DIR}/renderSystem/r_world.h
	${MOUNT_DIR}/API/renderer_api.h
)

set( RENDERSYSTEM_SOURCES
	${MOUNT_DIR}/framework/Puff.cpp
	${MOUNT_DIR}/renderSystem/r_api.cpp
	${MOUNT_DIR}/renderSystem/r_precompiled.cpp
	${MOUNT_DIR}/renderSystem/r_animation.cpp
	${MOUNT_DIR}/renderSystem/r_backend.cpp
	${MOUNT_DIR}/renderSystem/r_bsp_tech3.cpp
	${MOUNT_DIR}/renderSystem/r_cmds.cpp
	${MOUNT_DIR}/renderSystem/r_curve.cpp
	${MOUNT_DIR}/renderSystem/r_dsa.cpp
	${MOUNT_DIR}/renderSystem/r_extensions.cpp
	${MOUNT_DIR}/renderSystem/r_extramath.cpp
	${MOUNT_DIR}/renderSystem/r_fbo.cpp
	${MOUNT_DIR}/renderSystem/r_flares.cpp
	${MOUNT_DIR}/renderSystem/r_font.cpp
	${MOUNT_DIR}/renderSystem/r_glimp.cpp
	${MOUNT_DIR}/renderSystem/r_glsl.cpp
	${MOUNT_DIR}/renderSystem/r_image.cpp
	${MOUNT_DIR}/renderSystem/r_image_dds.cpp
	${MOUNT_DIR}/renderSystem/r_image_jpg.cpp
	${MOUNT_DIR}/renderSystem/r_image_png.cpp
	${MOUNT_DIR}/renderSystem/r_image_tga.cpp
	${MOUNT_DIR}/renderSystem/r_init.cpp
	${MOUNT_DIR}/renderSystem/r_light.cpp
	${MOUNT_DIR}/renderSystem/r_main.cpp
	${MOUNT_DIR}/renderSystem/r_marks.cpp
	${MOUNT_DIR}/renderSystem/r_mesh.cpp
	${MOUNT_DIR}/renderSystem/r_model.cpp
	${MOUNT_DIR}/renderSystem/r_model_iqm.cpp
	${MOUNT_DIR}/renderSystem/r_noise.cpp
	${MOUNT_DIR}/renderSystem/r_postprocess.cpp
	${MOUNT_DIR}/renderSystem/r_scene.cpp
	${MOUNT_DIR}/renderSystem/r_shade.cpp
	${MOUNT_DIR}/renderSystem/r_shader.cpp
	${MOUNT_DIR}/renderSystem/r_shadows.cpp
	${MOUNT_DIR}/renderSystem/r_skins.cpp
	${MOUNT_DIR}/renderSystem/r_sky.cpp
	${MOUNT_DIR}/renderSystem/r_surface.cpp
	${MOUNT_DIR}/renderSystem/r_vao.cpp
	${MOUNT_DIR}/renderSystem/r_world.cpp
)

set( SERVERLIST_HEADERS
	${MOUNT_DIR}/server/server.h
	${MOUNT_DIR}/server/serverCcmds.h
	${MOUNT_DIR}/server/serverClient.h
	${MOUNT_DIR}/server/serverCrypto.h
	${MOUNT_DIR}/server/serverGame.h
	${MOUNT_DIR}/server/serverWorld.h
	${MOUNT_DIR}/server/serverSnapshot.h
	${MOUNT_DIR}/server/serverNetChan.h
	${MOUNT_DIR}/server/serverInit.h
	${MOUNT_DIR}/server/serverMain.h
	${MOUNT_DIR}/server/serverDemo.h
	${MOUNT_DIR}/server/serverOACS.h
	${MOUNT_DIR}/server/serverWallhack.h
	${MOUNT_DIR}/API/sgame_api.h
	${MOUNT_DIR}/API/serverClient_api.h
	${MOUNT_DIR}/API/serverCrypto_api.h
	${MOUNT_DIR}/API/serverGame_api.h
	${MOUNT_DIR}/API/serverWorld_api.h
	${MOUNT_DIR}/API/serverSnapshot_api.h
	${MOUNT_DIR}/API/serverNetChan_api.h
	${MOUNT_DIR}/API/serverInit_api.h
	${MOUNT_DIR}/API/serverMain_api.h
	${MOUNT_DIR}/API/serverDemo_api.h
	${MOUNT_DIR}/API/serverCcmds_api.h
)

set( SERVERLIST_SOURCES
	${MOUNT_DIR}/server/serverCcmds.cpp
	${MOUNT_DIR}/server/serverClient.cpp
	${MOUNT_DIR}/server/serverCrypto.cpp
	${MOUNT_DIR}/server/serverGame.cpp
	${MOUNT_DIR}/server/serverWorld.cpp
	${MOUNT_DIR}/server/serverWallhack.cpp
	${MOUNT_DIR}/server/serverSnapshot.cpp
	${MOUNT_DIR}/server/serverNetChan.cpp
	${MOUNT_DIR}/server/serverInit.cpp
	${MOUNT_DIR}/server/serverMain.cpp
	${MOUNT_DIR}/server/serverDemo.cpp
	${MOUNT_DIR}/server/serverOACS.cpp
)

set( COLLISIONMODEL_HEADERS
	${MOUNT_DIR}/cm/cm_local.h
	${MOUNT_DIR}/cm/cm_patch.h
	${MOUNT_DIR}/cm/cm_polylib.h
	${MOUNT_DIR}/API/cm_api.h
)

set( COLLISIONMODEL_SOURCES
	${MOUNT_DIR}/cm/cm_load_tech3.cpp
	${MOUNT_DIR}/cm/cm_patch.cpp
	${MOUNT_DIR}/cm/cm_polylib.cpp
	${MOUNT_DIR}/cm/cm_test.cpp
	${MOUNT_DIR}/cm/cm_trace.cpp
	${MOUNT_DIR}/cm/cm_trisoup.cpp
)

set( DOWNLOADLIST_HEADERS
	${MOUNT_DIR}/download/downloadLocal.h
)

set( DOWNLOADLIST_SOURCES
	${MOUNT_DIR}/download/downloadMain.cpp
) 

set( QCOMMONLIST_HEADERS
	${MOUNT_DIR}/API/download_api.h
	${MOUNT_DIR}/qcommon/q_shared.h
	${MOUNT_DIR}/qcommon/qcommon.h
	${MOUNT_DIR}/qcommon/qfiles.h
)

set( QCOMMONLIST_SOURCES
	${MOUNT_DIR}/qcommon/common.cpp
	${MOUNT_DIR}/qcommon/msg.cpp
)

set( FRAMEWORKS_HEADERS
	${MOUNT_DIR}/framework/appConfig.h
	${MOUNT_DIR}/API/FileSystem_api.h
	${MOUNT_DIR}/API/CVarSystem_api.h
	${MOUNT_DIR}/API/CmdSystem_api.h
	${MOUNT_DIR}/API/CmdBuffer_api.h
	${MOUNT_DIR}/API/CmdDelay_api.h
	${MOUNT_DIR}/API/MD4_api.h
	${MOUNT_DIR}/API/threads_api.h
	${MOUNT_DIR}/API/download_api.h
	${MOUNT_DIR}/API/Parse_api.h
	${MOUNT_DIR}/API/download_api.h
	${MOUNT_DIR}/API/consoleHistory_api.h
	${MOUNT_DIR}/API/network_api.h
	${MOUNT_DIR}/API/NetworkChain_api.h
	${MOUNT_DIR}/API/Memory_api.h
	${MOUNT_DIR}/framework/Huffman.h
	${MOUNT_DIR}/framework/FileSystem.h
	${MOUNT_DIR}/framework/CVarSystem.h
	${MOUNT_DIR}/framework/CmdSystem.h
	${MOUNT_DIR}/framework/CmdBuffer.h
	${MOUNT_DIR}/framework/CmdDelay.h
	${MOUNT_DIR}/framework/IOAPI.h
	${MOUNT_DIR}/framework/MD4.h
	${MOUNT_DIR}/framework/Puff.h
	${MOUNT_DIR}/framework/Unzip.h
	${MOUNT_DIR}/framework/SurfaceFlags_Tech3.h
	${MOUNT_DIR}/framework/types.h
	${MOUNT_DIR}/framework/Threads.h
	${MOUNT_DIR}/framework/Network.h
	${MOUNT_DIR}/framework/NetworkChain.h
	${MOUNT_DIR}/framework/Parse.h
	${MOUNT_DIR}/framework/ConsoleHistory.h
	${MOUNT_DIR}/framework/Memory.h
)

set( FRAMEWORKS_SOURCES
	${MOUNT_DIR}/framework/FileSystem.cpp
	${MOUNT_DIR}/framework/CVarSystem.cpp
	${MOUNT_DIR}/framework/CmdSystem.cpp
	${MOUNT_DIR}/framework/CmdBuffer.cpp
	${MOUNT_DIR}/framework/CmdDelay.cpp
	${MOUNT_DIR}/framework/IOAPI.cpp
	${MOUNT_DIR}/framework/MD4.cpp
	${MOUNT_DIR}/framework/Puff.cpp
	${MOUNT_DIR}/framework/Unzip.cpp
	${MOUNT_DIR}/framework/Huffman.cpp
	${MOUNT_DIR}/framework/Threads.cpp
	${MOUNT_DIR}/framework/Network.cpp
	${MOUNT_DIR}/framework/NetworkChain.cpp
	${MOUNT_DIR}/framework/Parse.cpp
	${MOUNT_DIR}/framework/ConsoleHistory.cpp
	${MOUNT_DIR}/framework/Memory.cpp
)

if(USE_OPENSSL)
	find_package(OpenSSL REQUIRED)
	TARGET_INCLUDE_DIRECTORIES(${OPENSSL_INCLUDE_DIR})
	add_definitions( -DUSE_OPENSSL )
endif()

set( CLIENTLIST_HEADERS
	${MOUNT_DIR}/API/cgame_api.h
	${MOUNT_DIR}/client/clientAVI.h
	${MOUNT_DIR}/API/clientAVI_api.h
	${MOUNT_DIR}/client/clientGame.h
	${MOUNT_DIR}/API/clientGame_api.h
	${MOUNT_DIR}/API/clientLAN_api.h
	${MOUNT_DIR}/client/clientLAN.h
	${MOUNT_DIR}/API/clientGUI_api.h
	${MOUNT_DIR}/client/clientGUI.h
	${MOUNT_DIR}/client/clientBrowser.h
	${MOUNT_DIR}/API/clientScreen_api.h
	${MOUNT_DIR}/client/clientScreen.h
	${MOUNT_DIR}/client/clientParse.h
	${MOUNT_DIR}/client/clientNetworkChain.h
	${MOUNT_DIR}/API/clientMain_api.h
	${MOUNT_DIR}/client/clientMain.h
	${MOUNT_DIR}/API/clientKeys_api.h
	${MOUNT_DIR}/client/clientKeys.h
)

set( CLIENTLIST_SOURCES
	${MOUNT_DIR}/client/cl_cin.cpp
	${MOUNT_DIR}/client/cl_console.cpp
	${MOUNT_DIR}/client/cl_input.cpp
	${MOUNT_DIR}/client/clientKeys.cpp
	${MOUNT_DIR}/client/clientMain.cpp
	${MOUNT_DIR}/client/clientNetworkChain.cpp
	${MOUNT_DIR}/client/clientScreen.cpp
	${MOUNT_DIR}/client/clientAVI.cpp
	${MOUNT_DIR}/client/clientGame.cpp
	${MOUNT_DIR}/client/clientGUI.cpp
	${MOUNT_DIR}/client/clientBrowser.cpp
	${MOUNT_DIR}/client/clientLAN.cpp
	${MOUNT_DIR}/client/clientParse.cpp
)

set( AUDIO_HEADERS
	${MOUNT_DIR}/audio/s_codec.h
	${MOUNT_DIR}/audio/s_dmahd.h
	${MOUNT_DIR}/audio/s_local.h
	${MOUNT_DIR}/API/sound_api.h
)

set( AUDIO_SOURCES
	${MOUNT_DIR}/audio/s_adpcm.cpp
	${MOUNT_DIR}/audio/s_codec.cpp
	${MOUNT_DIR}/audio/s_codec_wav.cpp
	${MOUNT_DIR}/audio/s_codec_ogg.cpp
	${MOUNT_DIR}/audio/s_dma.cpp
	${MOUNT_DIR}/audio/s_dmahd.cpp
	${MOUNT_DIR}/audio/s_main.cpp
	${MOUNT_DIR}/audio/s_mem.cpp
	${MOUNT_DIR}/audio/s_mix.cpp
	${MOUNT_DIR}/audio/s_openal.cpp
	${MOUNT_DIR}/audio/s_wavelet.cpp
)

set( PLATFORMLIST_SOURCES
  ${MOUNT_DIR}/platform/Windows/systemWindows.cpp
  ${MOUNT_DIR}/platform/Linux/systemUnix.cpp
  ${MOUNT_DIR}/platform/systemMain.cpp
  ${MOUNT_DIR}/platform/systemThreads.cpp
  ${MOUNT_DIR}/console/consoleCurses.cpp
  ${MOUNT_DIR}/console/consoleLogging.cpp
  ${MOUNT_DIR}/platform/systemInput.cpp
  ${MOUNT_DIR}/platform/systemSound.cpp
)

SET ( PLATFORM_DEDICATED_SOURCES
    ${MOUNT_DIR}/platform/systemMain.cpp
	${MOUNT_DIR}/platform/Windows/systemWindows.cpp
    ${MOUNT_DIR}/console/consoleLogging.cpp
    ${MOUNT_DIR}/console/consoleCurses.cpp
    ${MOUNT_DIR}/platform/Linux/systemUnix.cpp
    ${MOUNT_DIR}/platform/systemMain.cpp
)

SET ( PLATFORMLIST_HEADERS
    ${MOUNT_DIR}/platform/Windows/resource.h
	${MOUNT_DIR}/platform/systemLocal.h
	${MOUNT_DIR}/API/system_api.h
	${MOUNT_DIR}/API/consoleCurses_api.h
    ${MOUNT_DIR}/console/consoleCurses.h
	${MOUNT_DIR}/API/consoleLogging_api.h
    ${MOUNT_DIR}/console/consoleLogging.h
	${MOUNT_DIR}/API/systemThreads_api.h
    ${MOUNT_DIR}/platform/systemThreads.h
)

set( NULL_SERVER_HEADERS
	${MOUNT_DIR}/API/threads_api.h
	${MOUNT_DIR}/framework/Threads.h
	${MOUNT_DIR}/null/null_serverprecompiled.h
	${MOUNT_DIR}/API/systemThreads_api.h
    ${MOUNT_DIR}/platform/systemThreads.h
)

set( NULL_SERVER_SOURCES
	${MOUNT_DIR}/null/null_client.cpp
	${MOUNT_DIR}/null/null_input.cpp
	${MOUNT_DIR}/null/null_serverprecompiled.cpp
	${MOUNT_DIR}/framework/Threads.cpp
    ${MOUNT_DIR}/platform/systemThreads.cpp
)

set( NULL_AUTOUPDATESERVER_HEADERS
	${MOUNT_DIR}/API/threads_api.h
	${MOUNT_DIR}/framework/Threads.h
	${MOUNT_DIR}/null/null_autoprecompiled.h
	${MOUNT_DIR}/API/systemThreads_api.h
    ${MOUNT_DIR}/platform/systemThreads.h

)

set( NULL_AUTOUPDATESERVER_SOURCES
	${MOUNT_DIR}/null/null_client.cpp
	${MOUNT_DIR}/null/null_input.cpp
	${MOUNT_DIR}/null/null_autoprecompiled.cpp
	${MOUNT_DIR}/framework/Threads.cpp
    ${MOUNT_DIR}/platform/systemThreads.cpp
)


set( OWMAPLIST_HEADERS
  ${TOOLS_DIR}/owmap/bspfile.h
  ${TOOLS_DIR}/owmap/bytebool.h
  ${TOOLS_DIR}/owmap/cmdlib.h
  ${TOOLS_DIR}/owmap/ddslib.h
  ${TOOLS_DIR}/owmap/bspfile_abstract.h
  ${TOOLS_DIR}/owmap/etclib.h
  ${TOOLS_DIR}/owmap/filematch.h
  ${TOOLS_DIR}/owmap/game__null.h
  ${TOOLS_DIR}/owmap/game_tremulous.h
  ${TOOLS_DIR}/owmap/globaldefs.h
  ${TOOLS_DIR}/owmap/imagelib.h
  ${TOOLS_DIR}/owmap/inout.h
  ${TOOLS_DIR}/owmap/mathlib.h
  ${TOOLS_DIR}/owmap/md4.h
  ${TOOLS_DIR}/owmap/md5lib.h
  ${TOOLS_DIR}/owmap/polylib.h
  ${TOOLS_DIR}/owmap/polyset.h
  ${TOOLS_DIR}/owmap/q3map2.h
  ${TOOLS_DIR}/owmap/qfiles.h
  ${TOOLS_DIR}/owmap/qthreads.h
  ${TOOLS_DIR}/owmap/scriplib.h
  ${TOOLS_DIR}/owmap/surfaceflags.h
  ${TOOLS_DIR}/owmap/trilib.h
  ${TOOLS_DIR}/owmap/assets_loader.hpp
  ${TOOLS_DIR}/owmap/stb_image_write.h
  ${TOOLS_DIR}/owmap/libs/assetsys/assetsys.h
  ${TOOLS_DIR}/owmap/libs/assetsys/strpool.h
  ${TOOLS_DIR}/owmap/libs/catch2/catch.hpp
  ${TOOLS_DIR}/owmap/libs/filesystem/fwd.h
  ${TOOLS_DIR}/owmap/libs/filesystem/path.h
  ${TOOLS_DIR}/owmap/libs/filesystem/resolver.h
  ${TOOLS_DIR}/owmap/libs/jpeg-compressor/jpgd.h
  ${TOOLS_DIR}/owmap/libs/jpeg-compressor/jpge.h
  ${TOOLS_DIR}/owmap/libs/lodepng/lodepng.h
  ${TOOLS_DIR}/owmap/libs/lodepng/lodepng_util.h
  ${TOOLS_DIR}/owmap/libs/msdirent/msdirent.h
  ${TOOLS_DIR}/owmap/libs/picomodel/src/picointernal.h
  ${TOOLS_DIR}/owmap/libs/picomodel/src/picomodel.h
  ${TOOLS_DIR}/owmap/libs/picomodel/src/platfdefs.h
  ${TOOLS_DIR}/owmap/libs/picomodel/src/pm_fm.h
  ${TOOLS_DIR}/owmap/libs/picomodel/src/lwo/lwo2.h
  ${TOOLS_DIR}/owmap/libs/table-builder/table_builder.hpp
  ${TOOLS_DIR}/owmap/libs/tinyformat/tinyformat.h
)

set( OWMAPLIST_SOURCES
  ${TOOLS_DIR}/owmap/assets_loader.cpp
  ${TOOLS_DIR}/owmap/bbox.cpp
  ${TOOLS_DIR}/owmap/brush.cpp
  ${TOOLS_DIR}/owmap/brush_primit.cpp
  ${TOOLS_DIR}/owmap/bsp.cpp
  ${TOOLS_DIR}/owmap/bsp_analyze.cpp
  ${TOOLS_DIR}/owmap/bsp_info.cpp
  ${TOOLS_DIR}/owmap/bsp_scale.cpp
  ${TOOLS_DIR}/owmap/bspfile_abstract.cpp
  ${TOOLS_DIR}/owmap/bspfile_obsp.cpp
  ${TOOLS_DIR}/owmap/cmdlib.cpp
  ${TOOLS_DIR}/owmap/convert_ase.cpp
  ${TOOLS_DIR}/owmap/convert_bsp.cpp
  ${TOOLS_DIR}/owmap/convert_map.cpp
  ${TOOLS_DIR}/owmap/convert_obj.cpp
  ${TOOLS_DIR}/owmap/ddslib.cpp
  ${TOOLS_DIR}/owmap/decals.cpp
  ${TOOLS_DIR}/owmap/etclib.cpp
  ${TOOLS_DIR}/owmap/exportents.cpp
  ${TOOLS_DIR}/owmap/facebsp.cpp
  ${TOOLS_DIR}/owmap/filematch.cpp
  ${TOOLS_DIR}/owmap/fixaas.cpp
  ${TOOLS_DIR}/owmap/fog.cpp
  ${TOOLS_DIR}/owmap/help.cpp
  ${TOOLS_DIR}/owmap/image.cpp
  ${TOOLS_DIR}/owmap/imagelib.cpp
  ${TOOLS_DIR}/owmap/inout.cpp
  ${TOOLS_DIR}/owmap/leakfile.cpp
  ${TOOLS_DIR}/owmap/light.cpp
  ${TOOLS_DIR}/owmap/light_bounce.cpp
  ${TOOLS_DIR}/owmap/light_trace.cpp
  ${TOOLS_DIR}/owmap/light_ydnar.cpp
  ${TOOLS_DIR}/owmap/lightmaps_ydnar.cpp
  ${TOOLS_DIR}/owmap/line.cpp
  ${TOOLS_DIR}/owmap/m4x4.cpp
  ${TOOLS_DIR}/owmap/main.cpp
  ${TOOLS_DIR}/owmap/map.cpp
  ${TOOLS_DIR}/owmap/mathlib.cpp
  ${TOOLS_DIR}/owmap/md4.cpp
  ${TOOLS_DIR}/owmap/md5lib.cpp
  ${TOOLS_DIR}/owmap/mesh.cpp
  ${TOOLS_DIR}/owmap/minimap.cpp
  ${TOOLS_DIR}/owmap/model.cpp
  ${TOOLS_DIR}/owmap/patch.cpp
  ${TOOLS_DIR}/owmap/path_init.cpp
  ${TOOLS_DIR}/owmap/polylib.cpp
  ${TOOLS_DIR}/owmap/portals.cpp
  ${TOOLS_DIR}/owmap/prtfile.cpp
  ${TOOLS_DIR}/owmap/ray.cpp
  ${TOOLS_DIR}/owmap/scriplib.cpp
  ${TOOLS_DIR}/owmap/shaders.cpp
  ${TOOLS_DIR}/owmap/surface.cpp
  ${TOOLS_DIR}/owmap/surface_extra.cpp
  ${TOOLS_DIR}/owmap/surface_foliage.cpp
  ${TOOLS_DIR}/owmap/surface_fur.cpp
  ${TOOLS_DIR}/owmap/surface_meta.cpp
  ${TOOLS_DIR}/owmap/threads.cpp
  ${TOOLS_DIR}/owmap/tjunction.cpp
  ${TOOLS_DIR}/owmap/tree.cpp
  ${TOOLS_DIR}/owmap/trilib.cpp
  ${TOOLS_DIR}/owmap/vis.cpp
  ${TOOLS_DIR}/owmap/visflow.cpp
  ${TOOLS_DIR}/owmap/writebsp.cpp
  ${TOOLS_DIR}/owmap/libs/jpeg-compressor/jpgd.cpp
  ${TOOLS_DIR}/owmap/libs/jpeg-compressor/jpge.cpp
  ${TOOLS_DIR}/owmap/libs/lodepng/lodepng.cpp
  ${TOOLS_DIR}/owmap/libs/lodepng/lodepng_util.cpp
  ${TOOLS_DIR}/owmap/libs/picomodel/src/picointernal.c
  ${TOOLS_DIR}/owmap/libs/picomodel/src/picomodel.c
  ${TOOLS_DIR}/owmap/libs/picomodel/src/picomodules.c
  ${TOOLS_DIR}/owmap/libs/picomodel/src/pm_3ds.c
  ${TOOLS_DIR}/owmap/libs/picomodel/src/pm_ase.c
  ${TOOLS_DIR}/owmap/libs/picomodel/src/pm_fm.c
  ${TOOLS_DIR}/owmap/libs/picomodel/src/pm_lwo.c
  ${TOOLS_DIR}/owmap/libs/picomodel/src/pm_md2.c
  ${TOOLS_DIR}/owmap/libs/picomodel/src/pm_md3.c
  ${TOOLS_DIR}/owmap/libs/picomodel/src/pm_mdc.c
  ${TOOLS_DIR}/owmap/libs/picomodel/src/pm_mdl.c
  ${TOOLS_DIR}/owmap/libs/picomodel/src/pm_ms3d.c
  ${TOOLS_DIR}/owmap/libs/picomodel/src/pm_obj.c
  ${TOOLS_DIR}/owmap/libs/picomodel/src/pm_terrain.c
  ${TOOLS_DIR}/owmap/libs/picomodel/src/lwo/clip.c
  ${TOOLS_DIR}/owmap/libs/picomodel/src/lwo/envelope.c
  ${TOOLS_DIR}/owmap/libs/picomodel/src/lwo/list.c
  ${TOOLS_DIR}/owmap/libs/picomodel/src/lwo/lwio.c
  ${TOOLS_DIR}/owmap/libs/picomodel/src/lwo/lwo2.c
  ${TOOLS_DIR}/owmap/libs/picomodel/src/lwo/lwob.c
  ${TOOLS_DIR}/owmap/libs/picomodel/src/lwo/pntspols.c
  ${TOOLS_DIR}/owmap/libs/picomodel/src/lwo/surface.c
  ${TOOLS_DIR}/owmap/libs/picomodel/src/lwo/vecmath.c
  ${TOOLS_DIR}/owmap/libs/picomodel/src/lwo/vmap.c
  ${TOOLS_DIR}/owmap/libs/table-builder/table_builder.cpp
)

set( OWMASTERSERVERLIST_HEADERS
	${TOOLS_DIR}/master/clients.h
	${TOOLS_DIR}/master/games.h
	${TOOLS_DIR}/master/common.h
	${TOOLS_DIR}/master/messages.h
	${TOOLS_DIR}/master/servers.h
	${TOOLS_DIR}/master/system.h
)

set( OWMASTERSERVERLIST_SOURCES
	${TOOLS_DIR}/master/clients.cpp
	${TOOLS_DIR}/master/games.cpp
	${TOOLS_DIR}/master/common.cpp
	${TOOLS_DIR}/master/messages.cpp
	${TOOLS_DIR}/master/servers.cpp
	${TOOLS_DIR}/master/system.cpp
	${TOOLS_DIR}/master/dpmaster.cpp
)

set( CLIENT_SOURCES
	${MOUNT_DIR}/platform/systemLauncher.cpp
)


####################
# End of file list #
####################

if( MSVC )
	add_definitions( -DWINVER=0x501 )
	#set( OS_LIBRARIES m winmm wsock32 gdi32 ole32 ws2_32 psapi Iphlpapi advapi32 shell32 user32 )
	set( OS_LIBRARIES winmm ws2_32 psapi )
	set( EXTRALIBS dinput dsound )
elseif( UNIX )
	if( CMAKE_SYSTEM_NAME STREQUAL "FreeBSD" )
		set( OS_LIBRARIES m z )
	elseif( CMAKE_SYSTEM_NAME STREQUAL "Darwin" )
		set( OS_LIBRARIES dl m z )
		# FIXME This looks wrong.
		set( OS_LIBRARIES ${OS_LIBRARIES} "-framework AGL -framework OpenGL -framework Carbon -framework IOKit" )
	else()
		set( OS_LIBRARIES ${CMAKE_DL_LIBS} m z rt )
	endif()
endif()

################
# Build Client #
################

if( BUILD_CLIENT )
	# Setup target client

	# Dushan - This is how visual studio directories (or filters) are generated
	if( MSVC )
		SOURCE_GROUP( Header\ Files\\SharedList                               FILES ${SHAREDLIST_HEADERS} )
		SOURCE_GROUP( Header\ Files\\Framework                                FILES ${FRAMEWORKS_HEADERS} )
		SOURCE_GROUP( Header\ Files\\Server                                   FILES ${SERVERLIST_HEADERS} )
		SOURCE_GROUP( Header\ Files\\CM                                       FILES ${COLLISIONMODEL_HEADERS} )
		SOURCE_GROUP( Header\ Files\\QCommon                                  FILES ${QCOMMONLIST_HEADERS} )
		SOURCE_GROUP( Header\ Files\\Download                                 FILES ${DOWNLOADLIST_HEADERS} )
		SOURCE_GROUP( Header\ Files\\Client                                   FILES ${CLIENTLIST_HEADERS} )
		SOURCE_GROUP( Header\ Files\\Audio                                    FILES ${AUDIO_HEADERS} )
		SOURCE_GROUP( Header\ Files\\Platform                                 FILES ${PLATFORMLIST_HEADERS} )
		SOURCE_GROUP( Header\ Files\\Database                                 FILES ${DATABASELIST_HEADERS} )
		SOURCE_GROUP( Source\ Files\\SharedList                               FILES ${SHAREDLIST_SOURCES} )
		SOURCE_GROUP( Source\ Files\\Server                                   FILES ${SERVERLIST_SOURCES} )
		SOURCE_GROUP( Source\ Files\\CM                                       FILES ${COLLISIONMODEL_SOURCES} )
		SOURCE_GROUP( Source\ Files\\QCommon                                  FILES ${QCOMMONLIST_SOURCES} )
		SOURCE_GROUP( Source\ Files\\Download                                 FILES ${DOWNLOADLIST_SOURCES} )
		SOURCE_GROUP( Source\ Files\\Framework                                FILES ${FRAMEWORKS_SOURCES} )
		SOURCE_GROUP( Source\ Files\\Client                                   FILES ${CLIENTLIST_SOURCES} )
		SOURCE_GROUP( Source\ Files\\Audio                                    FILES ${AUDIO_SOURCES} )
		SOURCE_GROUP( Source\ Files\\Platform                                 FILES ${PLATFORMLIST_SOURCES} )
	endif( MSVC )
  
  
	# This section finds and provides REQUIRED dependencies to target engine
	find_package( SDL REQUIRED )
	find_package( CURL REQUIRED )
	find_package( Zlib REQUIRED )
	find_package( BZ2 REQUIRED )
	find_package( OpenAL REQUIRED )
	find_package( OggVorbis REQUIRED )
	find_package( Sodium REQUIRED )
  
	add_library( engine SHARED  ${SHAREDLIST_HEADERS} ${FRAMEWORKS_HEADERS} ${QCOMMONLIST_HEADERS} ${SERVERLIST_HEADERS} ${COLLISIONMODEL_HEADERS} ${CLIENTLIST_HEADERS} ${AUDIO_HEADERS} ${SHAREDLIST_SOURCES} ${SERVERLIST_SOURCES} ${COLLISIONMODEL_SOURCES} ${QCOMMONLIST_SOURCES} ${CLIENTLIST_SOURCES} ${AUDIO_SOURCES} ${FRAMEWORKS_SOURCES} ${PLATFORMLIST} ${PLATFORMLIST_SOURCES} ${PLATFORMLIST_HEADERS} ${MOUNT_DIR}/framework/precompiled.h ${MOUNT_DIR}/framework/precompiled.cpp ${DOWNLOADLIST_HEADERS} ${DOWNLOADLIST_SOURCES} )

if( MSVC )
	SET_TARGET_PROPERTIES( engine PROPERTIES LINK_FLAGS "/DEF:\"${MOUNT_DIR}/platform/engineMain.def" )
endif(MSVC)
		
if(MSVC)
	set_property( TARGET engine APPEND PROPERTY COMPILE_DEFINITIONS _AMD64_ WIN32 _AMD64 _WIN64 __WIN64__ NDEBUG _WINDOWS USE_CRYPTO HAVE_CONFIG_H USE_INCREASED_ENTITIES HAVE_BZIP2 SODIUM_STATIC )
	ADD_PRECOMPILED_HEADER( engine "${MOUNT_DIR}/framework/precompiled.h" "${MOUNT_DIR}/framework/precompiled.cpp" )
elseif( UNIX )
	set_property( TARGET engine APPEND PROPERTY COMPILE_DEFINITIONS NDEBUG __LINUX__ __linux__ __amd64__ __x86_64__ LINUX USE_CRYPTO HAVE_CONFIG_H USE_INCREASED_ENTITIES HAVE_BZIP2 SODIUM_STATIC )
endif()
  
	set_target_properties( engine PROPERTIES OUTPUT_NAME "engine.${BUILD_ARCH}" PREFIX "" LINKER_LANGUAGE CXX )

	
	set( CURSES_NEED_NCURSES 1 ) # Tells FindCurses that ncurses is required
	# Determine platform
	if( MSVC )
		find_package( PDCurses REQUIRED )
	elseif( UNIX )
		find_package( Curses REQUIRED )
	endif()

	TARGET_INCLUDE_DIRECTORIES( engine PRIVATE ${OPENGL_INCLUDE_DIR} ${SDL2_INCLUDE_DIR} ${ZLIB_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS} ${PNG_INCLUDE_DIR} ${PDCURSES_INCLUDE_DIR} ${MOUNT_DIR} ${JPEG_INCLUDE_DIR} ${BZ2_INCLUDE_DIR} ${OPENAL_INCLUDE_DIR} ${OGG_INCLUDE_DIR} ${VORBIS_INCLUDE_DIR} ${TREMGAME_DIR} ${SODIUM_INCLUDE_DIR} )

	TARGET_LINK_LIBRARIES( engine ${OPENGL_LIBRARIES} ${SDL2_LIBRARY} ${ZLIB_LIBRARIES} ${CURL_LIBRARIES} ${PNG_LIBRARIES} ${PDCURSES_LIBRARY} ${JPEG_LIBRARIES} ${LINK_LIBRARY} ${BZ2_LIBRARY} ${OPENAL_LIBRARY} ${VORBIS_LIBRARY} ${VORBISFILE_LIBRARY} ${SODIUM_LIBRARIES})

	if( ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" )
		link_directories( /opt/local/lib )
	endif()

	TARGET_INCLUDE_DIRECTORIES( engine PRIVATE ${CURSES_INCLUDE_DIR} )
	target_link_libraries( engine ${CURSES_LIBRARIES} )

	if ( MSVC)
		target_link_libraries( engine ${OS_LIBRARIES} )
	else ( UNIX )
		target_link_libraries( engine ${OS_LIBRARIES} )
	endif() 

	add_library( renderSystem SHARED ${SHAREDLIST_HEADERS} ${SHAREDLIST_SOURCES} ${RENDERSYSTEM_HEADERS} ${RENDERSYSTEM_SOURCES} )
	if( MSVC )
		SOURCE_GROUP( Header\ Files\\SharedList                               FILES ${SHAREDLIST_HEADERS} )
		SOURCE_GROUP( Header\ Files\\RenderSystem                             FILES ${RENDERSYSTEM_HEADERS} )
		SOURCE_GROUP( Source\ Files\\SharedList                               FILES ${SHAREDLIST_SOURCES} )
		SOURCE_GROUP( Source\ Files\\RenderSystem                             FILES ${RENDERSYSTEM_SOURCES} )

		ADD_PRECOMPILED_HEADER( renderSystem "${MOUNT_DIR}/renderSystem/r_precompiled.h" "${MOUNT_DIR}/renderSystem/r_precompiled.cpp" )
		SET_TARGET_PROPERTIES( renderSystem PROPERTIES LINK_FLAGS "/DEF:\"${MOUNT_DIR}/renderSystem/renderSystem.def" )
	endif( MSVC )

	# This section finds and provides REQUIRED dependencies to target engine
	find_package( OpenGL REQUIRED )
	find_package( JPEG REQUIRED )
	find_package( SDL REQUIRED )
	find_package( Freetype REQUIRED )
	find_package( PNG REQUIRED )

	TARGET_INCLUDE_DIRECTORIES( renderSystem PRIVATE ${OPENGL_INCLUDE_DIR} ${FREETYPE_INCLUDE_DIRS} ${SDL2_INCLUDE_DIR} ${PNG_INCLUDE_DIR} ${MOUNT_DIR} ${JPEG_INCLUDE_DIR} )
	
	TARGET_LINK_LIBRARIES( renderSystem ${OPENGL_LIBRARIES} ${SDL2_LIBRARY} ${FREETYPE_LIBRARIES} ${PNG_LIBRARIES} ${JPEG_LIBRARIES} ${LINK_LIBRARY})
	
	if(MSVC)
		set_property( TARGET renderSystem PROPERTY COMPILE_DEFINITIONS _AMD64_ WIN32 _AMD64 _WIN64 __WIN64__ RENDERSYSTEM BUILD_FREETYPE HAVE_BOOLEAN  )
	elseif( UNIX )
		set_property( TARGET renderSystem PROPERTY COMPILE_DEFINITIONS LINUX __amd64__ __linux__ __x86_64__ __LINUX__ RENDERSYSTEM BUILD_FREETYPE HAVE_BOOLEAN   )
	endif()
	
	set_target_properties( renderSystem PROPERTIES
		OUTPUT_NAME "renderSystem${BUILD_ARCH}"
		PREFIX ""
		LIBRARY_OUTPUT_DIRECTORY "main"
	)
	
	# Dushan - This is how visual studio directories (or filters) are generated
	if( MSVC )
		SOURCE_GROUP( Source\ Files\\Client                               FILES ${CLIENT_SOURCES} )
	endif( MSVC )
  
	add_executable( client ${CLIENT_SOURCES} )

	TARGET_INCLUDE_DIRECTORIES( client PRIVATE ${SDL2_INCLUDE_DIR} )

	TARGET_LINK_LIBRARIES( client ${SDL2_LIBRARY} ${SDL2MAIN_LIBRARY} )

if(MSVC)
	set_property( TARGET client APPEND PROPERTY COMPILE_DEFINITIONS _AMD64_ WIN32 _AMD64 _WIN64 __WIN64__ NDEBUG _WINDOWS HAVE_CONFIG_H  __STDC_CONSTANT_MACROS )
elseif( UNIX )
	set_property( TARGET client APPEND PROPERTY COMPILE_DEFINITIONS NDEBUG __LINUX__ __linux__ __amd64__ __x86_64__ LINUX __STDC_CONSTANT_MACROS )
endif()
  
	set_target_properties( client PROPERTIES OUTPUT_NAME "${GAME_APP_NAME}.${BUILD_ARCH}" PREFIX "" LINKER_LANGUAGE CXX )
if( MSVC )
	set_target_properties( client PROPERTIES LINK_FLAGS  /SUBSYSTEM:CONSOLE )
endif( MSVC )
	
	install( TARGETS client
		RUNTIME DESTINATION bin
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib
	)

endif()

################
# Build Server #
################

if( BUILD_SERVER )
	# Setup the target Server
	add_executable( server ${SHAREDLIST_HEADERS} ${QCOMMONLIST_HEADERS} ${FRAMEWORKS_HEADERS} ${DOWNLOADLIST_HEADERS} ${SERVERLIST_HEADERS} ${COLLISIONMODEL_HEADERS} ${CLIENTLIST_HEADERS} ${SHAREDLIST_SOURCES} ${SERVERLIST_SOURCES} ${DOWNLOADLIST_SOURCES} ${COLLISIONMODEL_SOURCES} ${QCOMMONLIST_SOURCES} ${FRAMEWORKS_SOURCES} ${PLATFORMLIST_HEADERS} ${PLATFORM_DEDICATED_SOURCES} ${NULL_SERVER_SOURCES} ${NULL_SERVER_HEADERS} )

	if(MSVC)
		set_property( TARGET server APPEND PROPERTY COMPILE_DEFINITIONS _AMD64_ WIN32 _AMD64 _WIN64 __WIN64__ NDEBUG DEDICATED _WINDOWS HAVE_CONFIG_H USE_INCREASED_ENTITIES  __STDC_CONSTANT_MACROS HAVE_BOOLEAN HAVE_BZIP2 SODIUM_STATIC )
        ADD_PRECOMPILED_HEADER( server "${MOUNT_DIR}/null/null_serverprecompiled.h" "${MOUNT_DIR}/null/null_serverprecompiled.cpp" )

	elseif( UNIX )
		set_property( TARGET server APPEND PROPERTY COMPILE_DEFINITIONS NDEBUG __LINUX__ __linux__ __amd64__ __x86_64__ LINUX DEDICATED HAVE_CONFIG_H USE_INCREASED_ENTITIES __STDC_CONSTANT_MACROS BUILD_FREETYPE HAVE_BZIP2 SODIUM_STATIC )
	endif()
	
	set_target_properties( server PROPERTIES OUTPUT_NAME "dedicated.${BUILD_ARCH}" PREFIX "" LINKER_LANGUAGE CXX )
	install( TARGETS server
		RUNTIME DESTINATION bin
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib
	)
	# The following section finds the REQUIRED dependencies and makes them available to the target
	find_package( SDL REQUIRED )
	find_package( CURL REQUIRED )
	find_package( Zlib REQUIRED )
	find_package( BZ2 REQUIRED )
	find_package( Sodium REQUIRED )
	
	TARGET_INCLUDE_DIRECTORIES( server PRIVATE ${SDL2_INCLUDE_DIR} ${ZLIB_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS} ${PDCURSES_INCLUDE_DIR} ${MOUNT_DIR} ${BZ2_INCLUDE_DIR} ${TREMGAME_DIR} ${SODIUM_INCLUDE_DIR} )
	target_link_libraries( server ${OS_LIBRARIES} ${SDL2_LIBRARY} ${ZLIB_LIBRARIES} ${CURL_LIBRARIES} ${PDCURSES_LIBRARY} ${PDCURSES_LIBRARY} ${BZ2_LIBRARY} ${SODIUM_LIBRARIES})
	# The following section finds the OPTIONAL dependencies and makes them available to the target

	set( CURSES_NEED_NCURSES 1 ) # Tells FindCurses that ncurses is required
	# Determine platform
	if( MSVC )
		find_package( PDCurses REQUIRED )
	elseif( UNIX )
		find_package( Curses REQUIRED )
	endif()
	set_property( TARGET server APPEND PROPERTY COMPILE_DEFINITIONS USE_CURSES )
	TARGET_INCLUDE_DIRECTORIES( server PRIVATE ${CURSES_INCLUDE_DIR} )
	target_link_libraries( server ${CURSES_LIBRARIES} )

endif()

###########################
# Build Autoupdate Server #
###########################

if( BUILD_AUTOUPDATE_SERVER )
	# Setup the target Server
	add_executable( autoupdateserver ${SHAREDLIST_HEADERS} ${FRAMEWORKS_HEADERS} ${DOWNLOADLIST_HEADERS}
	${SERVERLIST_HEADERS} ${COLLISIONMODEL_HEADERS} ${CLIENTLIST_HEADERS} ${DOWNLOADLIST_SOURCES}
	${QCOMMONLIST_HEADERS} ${SHAREDLIST_SOURCES} ${SERVERLIST_SOURCES} ${COLLISIONMODEL_SOURCES} 
	${QCOMMONLIST_SOURCES} ${PLATFORMLIST_HEADERS} ${FRAMEWORKS_SOURCES}
	${PLATFORM_DEDICATED_SOURCES} ${NULL_AUTOUPDATESERVER_SOURCES} ${NULL_AUTOUPDATESERVER_HEADERS} )

	target_link_libraries( autoupdateserver ${OS_LIBRARIES} )
	if(MSVC)
        set_property( TARGET autoupdateserver APPEND PROPERTY COMPILE_DEFINITIONS DEDICATED _AMD64_ WIN32 _AMD64 _WIN64 __WIN64__ UPDATE_SERVER SODIUM_STATIC )
        ADD_PRECOMPILED_HEADER( autoupdateserver "${MOUNT_DIR}/null/null_autoprecompiled.h" "${MOUNT_DIR}/null/null_autoprecompiled.cpp" )
	elseif( UNIX )
		set_property( TARGET autoupdateserver APPEND PROPERTY COMPILE_DEFINITIONS NDEBUG __LINUX__ __linux__ __amd64__ __x86_64__ LINUX DEDICATED UPDATE_SERVER SODIUM_STATIC )
	endif()
	install( TARGETS autoupdateserver
		RUNTIME DESTINATION bin
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib
	)
	# The following section finds the REQUIRED dependencies and makes them available to the target
	find_package( SDL REQUIRED )
	find_package( CURL REQUIRED )
	find_package( Zlib REQUIRED )
	find_package( Sodium REQUIRED )
	
	TARGET_INCLUDE_DIRECTORIES( autoupdateserver PRIVATE ${SDL2_INCLUDE_DIR} ${ZLIB_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS} ${PDCURSES_INCLUDE_DIR} ${MOUNT_DIR} ${TREMGAME_DIR} ${SODIUM_INCLUDE_DIR})
	target_link_libraries( autoupdateserver ${SDL2_LIBRARY} ${ZLIB_LIBRARIES} ${CURL_LIBRARIES} ${PDCURSES_LIBRARY} ${PDCURSES_LIBRARY} ${SODIUM_LIBRARIES})
	# The following section finds the OPTIONAL dependencies and makes them available to the target

	set( CURSES_NEED_NCURSES 1 ) # Tells FindCurses that ncurses is required
	# Determine platform
	if( MSVC )
		find_package( PDCurses REQUIRED )
	elseif( UNIX )
		find_package( Curses REQUIRED )
	endif()
	set_property( TARGET autoupdateserver APPEND PROPERTY COMPILE_DEFINITIONS USE_CURSES )
	TARGET_INCLUDE_DIRECTORIES( autoupdateserver PRIVATE ${CURSES_INCLUDE_DIR} )
	target_link_libraries( autoupdateserver ${CURSES_LIBRARIES} )

endif()

#########################
# Build OWMasterServer  #
#########################

if( BUILD_MASTER_SERVER )
	# Dushan - This is how visual studio directories (or filters) are generated
	if( MSVC )
		SOURCE_GROUP( Header\ Files\\OWMaster                                 FILES ${OWMASTERSERVERLIST_HEADERS} )
		SOURCE_GROUP( Source\ Files\\OWMaster                                 FILES ${OWMASTERSERVERLIST_SOURCES} )
	endif( MSVC )

	# Setup OWMaster target
	add_executable( OWMaster ${OWMASTERSERVERLIST_SOURCES} ${OWMASTERSERVERLIST_HEADERS} )
	if(MSVC)
		set_property( TARGET OWMaster APPEND PROPERTY COMPILE_DEFINITIONS _AMD64_ NDEBUG _WIN32 WIN32 _AMD64 _WIN64 __WIN64__  )
	elseif( UNIX )
		set_property( TARGET OWMaster APPEND PROPERTY COMPILE_DEFINITIONS __LINUX__ _LINUX_ LINUX )
	endif() 

	find_package( GEOIP REQUIRED )
	
	TARGET_INCLUDE_DIRECTORIES( OWMaster PRIVATE ${TOOLS_DIR}/master ${GEOIP_INCLUDE_DIR})

	if(MSVC)
		target_link_libraries( OWMaster wsock32 ws2_32 ${GEOIP_LIBRARIES} )
	elseif( UNIX )
		target_link_libraries( OWMaster -ltdb -lm ${GEOIP_LIBRARIES} )
	endif() 

	set_target_properties( OWMaster PROPERTIES OUTPUT_NAME "OWMaster.${BUILD_ARCH}" PREFIX "" )

	install( TARGETS OWMaster
		RUNTIME DESTINATION bin
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib
	)
	endif()

################
# Build OWMap  #
################
if( BUILD_OWMAP )
  # Dushan - This is how visual studio directories (or filters) are generated
  if( MSVC )
    SOURCE_GROUP( Header\ Files\\OWMap                                    FILES ${OWMAPLIST_HEADERS} )
    SOURCE_GROUP( Source\ Files\\OWMap                                    FILES ${OWMAPLIST_SOURCES} )
  endif( MSVC )

   # Setup OWMap target
  add_executable( OWMap ${OWMAPLIST_SOURCES} ${OWMAPLIST_HEADERS} )
  #set_property( TARGET OWMap APPEND PROPERTY COMPILE_DEFINITIONS _AMD64_ NDEBUG _WIN32 WIN32 _AMD64 _WIN64 __WIN64__ USE_OPENGL Q3MAP2 )
 if(MSVC)
  set_property( TARGET OWMap APPEND PROPERTY COMPILE_DEFINITIONS _AMD64_ NDEBUG _WIN32 WIN32 _AMD64 _WIN64 __WIN64__ Q3MAP2 )
 elseif( UNIX )
  set_property( TARGET OWMap APPEND PROPERTY COMPILE_DEFINITIONS __LINUX__ _LINUX_ LINUX Q3MAP2 )
 endif() 
  
  # This section finds and provides REQUIRED dependencies to OWMap target
  find_package( OpenGL REQUIRED )
  find_package( JPEG REQUIRED )
  find_package( PNG REQUIRED )
  find_package( ZLIB REQUIRED )
  
  TARGET_INCLUDE_DIRECTORIES( OWMap PRIVATE ${TOOLS_DIR}/owmap ${TOOLS_DIR}/owmap/libs/assetsys ${TOOLS_DIR}/owmap/libs/catch2 ${TOOLS_DIR}/owmap/libs/filesystem ${TOOLS_DIR}/owmap/libs/jpeg-compressor ${TOOLS_DIR}/owmap/libs/lodepng ${TOOLS_DIR}/owmap/libs/msdirent ${TOOLS_DIR}/owmap/libs/picomodel/src ${TOOLS_DIR}/owmap/libs/picomodel/lwo ${TOOLS_DIR}/owmap/libs/table-builder ${TOOLS_DIR}/owmap/libs/tinyformat ${OPENGL_INCLUDE_DIR} ${JPEG_INCLUDE_DIR} ${PNG_INCLUDE_DIR} ${ZLIB_INCLUDE_DIRS} ${MOUNT_DIR} )
  
 if(MSVC)
  target_link_libraries( OWMap ${OPENGL_LIBRARIES} ${JPEG_LIBRARIES} ${PNG_LIBRARIES} ${ZLIB_LIBRARIES} wsock32 ws2_32 )
 elseif( UNIX )
 target_link_libraries( OWMap ${OPENGL_LIBRARIES} ${JPEG_LIBRARIES} ${PNG_LIBRARIES} ${ZLIB_LIBRARIES} -lpthread )
 endif() 
 
  set_target_properties( OWMap PROPERTIES OUTPUT_NAME "OWMap.${BUILD_ARCH}" PREFIX "" )

install( TARGETS OWMap
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
  )
endif()
