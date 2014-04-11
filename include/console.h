/***
 *   Project: console-win
 *   Copyright (C) Daniel Bloemendal. All rights reserved.
 *
 *   C console interface
 *
 *   This file is part of console-win.
 *
 *   console-win is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   console-win is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with console-win.  If not, see <http://www.gnu.org/licenses/>.
 ***/

#pragma once

// Windows
#include <Windows.h>

#ifdef CONSOLE_DYNAMIC
#ifdef CONSOLE_EXPORTS
#define CONSOLE_API __declspec(dllexport)
#else
#define CONSOLE_API __declspec(dllimport)
#endif
#else
#define CONSOLE_API
#endif

// Opaque console type
typedef struct CONSOLE CONSOLE;

CONSOLE_API int  console_create(CONSOLE** console);
CONSOLE_API void console_show(CONSOLE* console, int visible);
CONSOLE_API int  console_visible(CONSOLE* console);
CONSOLE_API void console_toggle(CONSOLE* console);
CONSOLE_API int  console_write(CONSOLE* console, const wchar_t* text);
CONSOLE_API int  console_write_utf8(CONSOLE* console, const char* text);
CONSOLE_API int  console_read(CONSOLE* console, const wchar_t* buffer, size_t length);
CONSOLE_API int  console_destroy(CONSOLE* console);