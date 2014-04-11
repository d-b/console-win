/***
 *   Project: console-win
 *   Copyright (C) Daniel Bloemendal. All rights reserved.
 *
 *   C++ console interface
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

// STL
#include <vector>
#include <string>
#include <exception>

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

// Project
#include "exceptions.hpp"
#include "mail.hpp"

namespace db
{
    class CONSOLE_API console {
    public:

        struct rgb { unsigned char red, green, blue; };

       /**
        * Construct a new console
        *
        * @param title the window title for the console
        * @param background the rgb colour to use for the background
        * @param buffers the number of buffers to reserve for storing i/o
        */
        console(std::wstring title, HICON icon, rgb background, int buffers);
        
       /**
        * Destroys internal resources used by the console
        */
        virtual ~console();

       /**
        * Show or hide the console
        *
        * @param visible whether or not the console should be visible
        */
        void show(bool visible = true);

       /**
        * Returns the whether or not the console is visible
        */
        bool visible();

       /**
        * Toggles the visibility of console
        */
        void toggle();

       /**
        * Write rich text to the console
        *
        * @param richtext the rich text to write to the console
        * @param timeout how long to wait, in milliseconds, for a successful write
        * @return whether or not the write succeeded
        */
        bool write(const std::wstring& richtext, unsigned long timeout);

       /**
        * Read text sent from the console
        *
        * @param buffer a buffer to hold the text
        * @param timeout how long to wait, in milliseconds, for a successful read
        * @return whether or not the read succeeded
        */
        bool read(std::wstring& buffer, unsigned long timeout);

    private:

        //
        // Internal thread context
        //
        bool _thread_initialize();
        bool _thread_messagepump();
        void _thread_handler_mail(mail::string& mail);
        LRESULT _thread_handler_message(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        LRESULT _thread_handler_input(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        void _thread_resize(DWORD width, DWORD height);
        bool _thread_finalize();

        //
        // Windows callbacks
        //
        static LRESULT CALLBACK _callback_winproc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        static DWORD CALLBACK _calback_threadproc(void* self);

        //
        // Helpers
        //
        static HINSTANCE _get_instance();

        //
        // Stored settings
        //
        std::wstring _title;
        HICON _icon;

        //
        // Handles
        //
        HINSTANCE _hinstance;
        HANDLE _thread_console;
        HANDLE _event_initialized;
        HWND _hwnd_console;
        HWND _hwnd_console_input;
        HWND _hwnd_console_output;

        //
        // Message passing
        //
        mail _mail_input;
        mail _mail_output;
    };
}
