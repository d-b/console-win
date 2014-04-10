/***
 *   Project: console-win
 *   Copyright (C) Daniel Bloemendal. All rights reserved.
 *
 *   Mail interface
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

namespace db
{
    class mail {
    public:
        typedef std::wstring string;
        enum type { MESSAGE_Mail, MESSAGE_Windows, MESSAGE_Quit };
        
        struct message {
            type type;
            string mail;
            MSG windows;
            DWORD status;
        };

        mail(int mailboxes); virtual ~mail();
        bool send(const string& mail, unsigned long timeout);
        bool recv(string& buffer, unsigned long timeout);
        bool recv(message& message, unsigned long timeout);

    private:
        std::vector<string> _boxes;
        int _next_filled;
        int _next_empty;
        HANDLE _sem_empty;
        HANDLE _sem_filled;
        CRITICAL_SECTION _lock;
    };
}
