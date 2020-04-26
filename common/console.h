/*
    Copyright 2018 php42

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#pragma once

#include <mutex>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#define COLOR_OK (FOREGROUND_GREEN)
#define COLOR_WARN (FOREGROUND_GREEN | FOREGROUND_RED)
#define COLOR_CRITICAL (FOREGROUND_RED)

class ScopedConsoleColorChanger
{
private:
    HANDLE screen_buffer_;
    CONSOLE_SCREEN_BUFFER_INFO info_;
    bool changed_;

public:
    ScopedConsoleColorChanger(WORD attributes) : changed_(false)
    {
        screen_buffer_ = CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);

        if(screen_buffer_ != INVALID_HANDLE_VALUE)
        {
            if(!GetConsoleScreenBufferInfo(screen_buffer_, &info_))
                return;
            if(SetConsoleTextAttribute(screen_buffer_, attributes))
                changed_ = true;
        }
    }

    ScopedConsoleColorChanger(const ScopedConsoleColorChanger&) = delete;
    ScopedConsoleColorChanger& operator=(const ScopedConsoleColorChanger&) = delete;

    ~ScopedConsoleColorChanger()
    {
        if(changed_)
            SetConsoleTextAttribute(screen_buffer_, info_.wAttributes);
        if(screen_buffer_ != INVALID_HANDLE_VALUE)
            CloseHandle(screen_buffer_);
    }
};

/* synchronization for console access (scoped ownership, recursive) */
class ScopedConsoleLock
{
private:
    static std::recursive_mutex mtx_;
    std::lock_guard<std::recursive_mutex> lock_;

public:
    ScopedConsoleLock() : lock_(mtx_) {}
    ScopedConsoleLock(const ScopedConsoleLock&) = delete;
    ScopedConsoleLock& operator=(const ScopedConsoleLock&) = delete;
};

/* scoped ownership of the console + change console text color (color reverted at end of life) */
class ScopedConsoleColorChangerThreadsafe : public ScopedConsoleLock, public ScopedConsoleColorChanger // C++ inheritance rules guarantee ScopedConsoleLock to be constructed first and destroyed last
{
    using ScopedConsoleColorChanger::ScopedConsoleColorChanger;
};

typedef ScopedConsoleColorChangerThreadsafe ScopedConsoleColorMT;
