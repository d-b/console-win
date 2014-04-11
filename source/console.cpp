/***
 *   Project: console-win
 *   Copyright (C) Daniel Bloemendal. All rights reserved.
 *
 *   Console implementation
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

#include "console.hpp"

//
// Rich edit control
//
#include <richedit.h>
#include "RichEditThemed.h"

namespace db
{
    // Constants
    enum {CONSOLE_IDC_OUTPUT = 101, 
          CONSOLE_IDC_INPUT  = 102,
          CONSOLE_MSG_QUIT   = WM_USER};

    // Name for console window class
    static const wchar_t* CONSOLE_WINDOW_CLASS = L"db::console";

    console::console(std::wstring title, HICON icon, rgb background, int buffers)
        : _title(title),
          _icon(icon),
          _event_initialized(NULL),
          _thread_console(NULL),
          _hwnd_console(NULL),
          _hwnd_console_input(NULL),
          _hwnd_console_output(NULL),
          _mail_input(buffers),
          _mail_output(buffers)
    {
        // Load the required libraries
        LoadLibrary(_T("msftedit.dll"));

        // Create initialization event
        _event_initialized = CreateEvent(NULL, FALSE, FALSE, NULL);
        win_exception::check(_event_initialized);

        // Start the main thread
        _thread_console = CreateThread(NULL, 0, _calback_threadproc, this, 0, NULL);
        win_exception::check(_thread_console);

        // Wait for initialization to complete
        WaitForSingleObject(_event_initialized, INFINITE);
    }

    console::~console() {
        // First destroy the console window
        if (_hwnd_console) SendMessage(_hwnd_console, CONSOLE_MSG_QUIT, 0, 0);

        // Wait for the thread to complete if it exists
        if (_thread_console) {
            WaitForSingleObject(_thread_console, INFINITE);
            CloseHandle(_thread_console);
        }

        // Cleanup remaining resources
        CloseHandle(_event_initialized);
    }

    void console::show(bool visible) {
        ShowWindow(_hwnd_console, visible ? SW_SHOW : SW_HIDE);
    }

    bool console::visible() {
        return IsWindowVisible(_hwnd_console);
    }

    void console::toggle() {
        show(!visible());
    }

    bool console::write(const std::wstring& richtext, unsigned long timeout) {
        return _mail_output.send(richtext, timeout);
    }

    bool console::read(std::wstring& buffer, unsigned long timeout) {
        return _mail_input.recv(buffer, timeout);
    }

    bool console::_thread_initialize() {
        //
        // Create the main terminal window
        //

        // Fetch the current instance
        HINSTANCE hinstance = _get_instance();

        // Setup the window class and register it
        WNDCLASSEX cls = { 0 };
        cls.cbSize = sizeof(cls);
        cls.style = CS_HREDRAW | CS_VREDRAW;
        cls.lpfnWndProc = _callback_winproc;
        cls.hIcon = _icon;
        cls.hIconSm = _icon;
        cls.hCursor = LoadCursor(NULL, IDC_ARROW);
        cls.hbrBackground = static_cast<HBRUSH>(GetSysColorBrush(COLOR_3DFACE));
        cls.lpszClassName = CONSOLE_WINDOW_CLASS;
        cls.hInstance = hinstance;
        RegisterClassEx(&cls);

        // Create the window for the terminal
        _hwnd_console = CreateWindowEx(
            0,
            CONSOLE_WINDOW_CLASS,
            _title.c_str(),
            WS_OVERLAPPEDWINDOW,

            // Size and position
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

            NULL,       // Parent window    
            NULL,       // Menu
            hinstance,  // Instance handle
            NULL        // Additional application data
            );

        // Add this instance to the window
        SetWindowLongPtr(_hwnd_console, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

        //
        // Create the edit boxes
        //

        // Create the output edit box
        _hwnd_console_output = CreateWindowEx(0, MSFTEDIT_CLASS, _T(""),
            WS_CHILD | WS_VISIBLE | WS_BORDER |           // Style
            ES_MULTILINE | WS_VSCROLL | ES_AUTOVSCROLL,   // ...
            0, 0, 0, 0,                                   // Geometry
            _hwnd_console,                                // Parent window
            reinterpret_cast<HMENU>(CONSOLE_IDC_OUTPUT),  // Control ID
            hinstance,                                    // Parent instance
            NULL                                          // Additional application data
            );

        // Create the input edit box
        _hwnd_console_input = CreateWindowEx(0, MSFTEDIT_CLASS, _T(""),
            WS_CHILD | WS_VISIBLE | WS_BORDER |           // Style
            ES_MULTILINE | ES_AUTOVSCROLL,                // ...
            0, 0, 0, 0,                                   // Geometry
            _hwnd_console,                                // Parent window
            reinterpret_cast<HMENU>(CONSOLE_IDC_INPUT),   // Control ID
            hinstance,                                    // Parent instance
            NULL                                          // Additional application data
            );

        // Set initial size
        RECT rect; GetWindowRect(_hwnd_console, &rect);
        _thread_resize(rect.right - rect.left, rect.bottom - rect.top);

        // Configure output
        SendMessage(_hwnd_console_output, EM_SETREADONLY, TRUE, 0);

        // Configure input
        SendMessage(_hwnd_console_input, EM_SETEVENTMASK, 0, ENM_KEYEVENTS | ENM_MOUSEEVENTS);
        SendMessage(_hwnd_console_input, EM_SETTEXTMODE, TM_PLAINTEXT, 0);

        // Set color scheme
        SendMessage(_hwnd_console_output, EM_SETBKGNDCOLOR, 0, RGB(230, 230, 230));
        SendMessage(_hwnd_console_input, EM_SETBKGNDCOLOR, 0, RGB(230, 230, 230));

        // Apply theme to edit controls
        CRichEditThemed::Attach(_hwnd_console_output);
        CRichEditThemed::Attach(_hwnd_console_input);
        
        // Show the window
        ShowWindow(_hwnd_console, SW_SHOW);

        // Set initialization event
        SetEvent(_event_initialized);

        // Successful initialization
        return TRUE;
    }
    
    bool console::_thread_messagepump() {
        mail::message msg; for (;;) {
            if (_mail_output.recv(msg, INFINITE)) {
                switch (msg.type) {
                case mail::type::MESSAGE_Mail:
                    _thread_handler_mail(msg.mail); break;
                case mail::type::MESSAGE_Windows:
                    TranslateMessage(&msg.windows);
                    DispatchMessage(&msg.windows);
                    break; // Continue to next message
                case mail::type::MESSAGE_Quit:
                    return TRUE;
                }
            }
        }
    }

    void console::_thread_handler_mail(mail::string &mail) {
        SETTEXTEX SetText;
        SetText.codepage = CP_WINUNICODE;
        SetText.flags = ST_SELECTION;
        CHARRANGE Range = { -1, -1 };
        SendMessage(_hwnd_console_output, EM_HIDESELECTION, TRUE, 0);
        SendMessage(_hwnd_console_output, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&Range));
        SendMessage(_hwnd_console_output, EM_SETTEXTEX, reinterpret_cast<WPARAM>(&SetText), reinterpret_cast<LPARAM>(mail.data()));
        SendMessage(_hwnd_console_output, WM_VSCROLL, SB_BOTTOM, 0);
        SendMessage(_hwnd_console_output, EM_HIDESELECTION, FALSE, 0);
    }

    LRESULT console::_thread_handler_message(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        LPNMHDR nmh = NULL;         // Control message
        LPMINMAXINFO minmax = NULL; // Minimum/maximum info
        MSGFILTER* inputmsg = NULL; // Input message

        switch (uMsg) {
        case WM_SIZE:
            _thread_resize(LOWORD(lParam), HIWORD(lParam)); break;
        case WM_GETMINMAXINFO:
            minmax = reinterpret_cast<LPMINMAXINFO>(lParam);
            minmax->ptMinTrackSize.x = 400;
            minmax->ptMinTrackSize.y = 400;
            break;
        case WM_NOTIFY:
            nmh = reinterpret_cast<LPNMHDR>(lParam);
            if (nmh->hwndFrom == _hwnd_console_input &&
                nmh->idFrom == CONSOLE_IDC_INPUT &&
                nmh->code == EN_MSGFILTER)
            { // Process the input control input message
                inputmsg = reinterpret_cast<MSGFILTER*>(nmh);
                return _thread_handler_input(hwnd, inputmsg->msg, inputmsg->wParam, inputmsg->lParam);
            }
            break;
        case WM_CLOSE:
            ShowWindow(_hwnd_console, SW_HIDE);
            break;
        case CONSOLE_MSG_QUIT:
            DestroyWindow(_hwnd_console);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }

        // Default result
        return 0;
    }

    LRESULT console::_thread_handler_input(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        // Check for send request
        bool send_message = false;
        switch (uMsg) {
        case WM_KEYDOWN:
            if (wParam == VK_RETURN)
                send_message = (GetKeyState(VK_CONTROL) >= 0) &&
                               (GetKeyState(VK_SHIFT)   >= 0);
            break;
        }

        // Deal with send request
        if (send_message) {
            // Get input text length
            GETTEXTLENGTHEX length_spec;
            length_spec.codepage = CP_WINUNICODE;
            length_spec.flags = GTL_DEFAULT;
            LRESULT length = SendMessage(_hwnd_console_input, EM_GETTEXTLENGTHEX, reinterpret_cast<WPARAM>(&length_spec), 0);
            if (length == E_INVALIDARG) return 0;
            length += 1; // Add 1 for the null terminator

            // Prepare a buffer and fill it with the text & send the text
            mail::string buffer;
            buffer.resize(length);
            GETTEXTEX fetch_spec = { 0 };
            fetch_spec.cb = length * sizeof(TCHAR);
            fetch_spec.codepage = CP_WINUNICODE;
            fetch_spec.flags = GT_DEFAULT;
            SendMessage(_hwnd_console_input, EM_GETTEXTEX, reinterpret_cast<WPARAM>(&fetch_spec), reinterpret_cast<LPARAM>(buffer.data()));
            _mail_input.send(buffer, INFINITE);

            // Clear existing text
            CHARRANGE range = { 0, -1 };
            SendMessage(_hwnd_console_input, EM_HIDESELECTION, TRUE, 0);
            SendMessage(_hwnd_console_input, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&range));
            SendMessage(_hwnd_console_input, WM_CLEAR, 0, 0);
            SendMessage(_hwnd_console_input, EM_HIDESELECTION, FALSE, 0);

            // The input was handled
            return 1;
        }

        // Pass event on to control
        return 0;
    }
    
    VOID console::_thread_resize(DWORD width, DWORD height) {
        SetWindowPos(_hwnd_console_output, NULL, 0, 0, width, height * 0.9, 0);
        SetWindowPos(_hwnd_console_input, NULL, 0, height * 0.9, width, height * 0.1, 0);
    }

    bool console::_thread_finalize() {
        return TRUE;
    }

    LRESULT console::_callback_winproc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        console* self = reinterpret_cast<console*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        return self->_thread_handler_message(hwnd, uMsg, wParam, lParam);
    }

    DWORD console::_calback_threadproc(void* pthis) {
        console* self = reinterpret_cast<console*>(pthis);
        if (self->_thread_initialize()) {
            self->_thread_messagepump();
            self->_thread_finalize();
        } return 0;
    }

    HINSTANCE console::_get_instance() {
        HINSTANCE instance = NULL;
        GetModuleHandleEx(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&_get_instance),
            &instance);
        return instance;
    }
}
