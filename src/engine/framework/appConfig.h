////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2020 Dusan Jocic <dusanjocic@msn.com>
//
// This file is part of the OpenWolf GPL Source Code.
// OpenWolf Source Code is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OpenWolf Source Code is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OpenWolf Source Code.  If not, see <http://www.gnu.org/licenses/>.
//
// -------------------------------------------------------------------------------------
// File name:   appConfig.h
// Created:
// Compilers:   Microsoft Visual C++ 2019, gcc (Ubuntu 8.3.0-6ubuntu1) 8.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __APPCONFIG_H__
#define __APPCONFIG_H__

//-----------------------------------------------------------------------------
// Hi and welcome to the application configuration file.
//-----------------------------------------------------------------------------

#ifndef PRODUCT_NAME
#define PRODUCT_NAME "Test App "
#endif //!PRODUCT_NAME

#ifndef PRODUCT_STAGE
#define PRODUCT_STAGE "0.0.1"
#endif //!PRODUCT_STAGE

#ifndef PRODUCT_NAME_UPPPER
#define PRODUCT_NAME_UPPPER "Test App" // Case, No spaces
#endif //!PRODUCT_NAME_UPPPER

#ifndef PRODUCT_NAME_LOWER
#define PRODUCT_NAME_LOWER "Test App" // No case, No spaces
#endif //!PRODUCT_NAME_LOWER

#ifndef PRODUCT_VERSION
#define PRODUCT_VERSION "0.0.1"
#endif //!PRODUCT_VERSION

#ifndef ENGINE_NAME
#define ENGINE_NAME "OpenWolf Engine"
#endif //!ENGINE_NAME

#ifndef ENGINE_VERSION
#define ENGINE_VERSION "0.6.0"
#endif //!ENGINE_VERSION

#ifndef CLIENT_WINDOW_TITLE
#define CLIENT_WINDOW_TITLE "Test App " PRODUCT_STAGE
#endif //!CLIENT_WINDOW_TITLE

#ifndef CLIENT_WINDOW_MIN_TITLE
#define CLIENT_WINDOW_MIN_TITLE "Test App " PRODUCT_STAGE
#endif //!CLIENT_WINDOW_MIN_TITLE

#ifndef Q3_VERSION
#define Q3_VERSION PRODUCT_NAME " " PRODUCT_VERSION
#endif //!Q3_VERSION

#ifndef Q3_ENGINE
#define Q3_ENGINE ENGINE_NAME " " ENGINE_VERSION
#endif //!Q3_ENGINE

#ifndef Q3_ENGINE_DATE
#define Q3_ENGINE_DATE __DATE__
#endif //!Q3_ENGINE_DATE

#ifndef GAMENAME_FOR_MASTER
#define GAMENAME_FOR_MASTER	PRODUCT_NAME_UPPPER
#endif //!GAMENAME_FOR_MASTER

#ifndef CONFIG_NAME
#define CONFIG_NAME "owconfig.cfg"
#endif //!CONFIG_NAME

#endif //!__APPCONFIG_H__
