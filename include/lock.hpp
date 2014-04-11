/***
*   Project: console-win
*   Copyright (C) Daniel Bloemendal. All rights reserved.
*
*   Wrapper for critical sections
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
    class lock {
    public:
        lock() { InitializeCriticalSection(&_lock); }
        ~lock() { DeleteCriticalSection(&_lock); }
        void acquire() { EnterCriticalSection(&_lock); }
        void release() { LeaveCriticalSection(&_lock); }

    private:
        CRITICAL_SECTION _lock;
    };
}
