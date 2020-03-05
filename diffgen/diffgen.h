/*
    Copyright 2020 php42

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
#include <filesystem>
#include <exception>
#include <mutex>
#include "../common/console.h"

struct DiffgenException : public std::runtime_error
{
    using std::runtime_error::runtime_error;
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

typedef std::filesystem::path Path;

bool extract(const Path& input, const Path& output);
bool diff(const Path& input, const Path& output, const Path& diff_path, int threads);
bool patch(const Path& input, const Path& output);
bool repack(const Path& input, const Path& output);
