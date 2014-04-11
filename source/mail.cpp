/***
 *   Project: console-win
 *   Copyright (C) Daniel Bloemendal. All rights reserved.
 *
 *   Mail implementation
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

namespace db
{
    mail::mail(int mailboxes) {
        _sem_filled = CreateSemaphore(NULL, 0, mailboxes, NULL);
        _sem_empty = CreateSemaphore(NULL, mailboxes, mailboxes, NULL);
        _next_empty = _next_filled = 0;
        _boxes.resize(mailboxes);
    }

    bool mail::send(const string& mail, unsigned long timeout) {
        // Wait for an empty mailbox
        if (WaitForSingleObject(_sem_empty, timeout) != WAIT_OBJECT_0) return false;

        // Fill mailbox with message
        _lock.acquire();
        _boxes[_next_empty].assign(mail);
        _next_empty = (_next_empty + 1) % _boxes.size();
        _lock.release();

        // Flag reader
        ReleaseSemaphore(_sem_filled, 1, NULL);
        return true;
    }

    bool mail::recv(string& buffer, unsigned long timeout) {
        // Wait for a filled mailbox
        if (WaitForSingleObject(_sem_filled, timeout) != WAIT_OBJECT_0) return false;
        
        // Read mail in mailbox
        _lock.acquire();
        buffer = _boxes[_next_filled];
        _next_filled = (_next_filled + 1) % _boxes.size();
        _lock.release();

        // Flag writer
        ReleaseSemaphore(_sem_empty, 1, NULL);
        return true;
    }

    bool mail::recv(message& message, unsigned long timeout) {
        // Status for window messages
        BOOL status;

        // Initial check for mail
        DWORD result = WaitForSingleObject(_sem_filled, 0);

        for (;;) {
            // See if there is any mail to receive
            if (result == WAIT_OBJECT_0) {
                // Read mail in mailbox
                message.type = MESSAGE_Mail;
                _lock.acquire();
                message.mail = _boxes[_next_filled];
                _next_filled = (_next_filled + 1) % _boxes.size();
                _lock.release();

                // Flag writer
                ReleaseSemaphore(_sem_empty, 1, NULL);
                return true;
            }

            // See if there are any messages to dispatch
            if ((status = PeekMessage(&message.windows, 0, 0, 0, PM_REMOVE)) != 0) {
                message.status = status;
                message.type = (message.windows.message != WM_QUIT)
                    ? MESSAGE_Windows : MESSAGE_Quit;
                return true;
            }

            // Wait for Window messages & mail
            result = MsgWaitForMultipleObjectsEx(1, &_sem_filled, timeout, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
            if (result != WAIT_OBJECT_0 && result != WAIT_OBJECT_0 + 1) return false;
        }
    }

    mail::~mail() {
        CloseHandle(_sem_empty);
        CloseHandle(_sem_filled);
    }
}
