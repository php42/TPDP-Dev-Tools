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
#include <filesystem>
#include <exception>

struct DiffgenException : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

typedef std::filesystem::path Path;

bool extract(const Path& input, const Path& output);
bool diff(const Path& input, const Path& output, const Path& diff_path, int diff_mode);
bool patch(const Path& input, const Path& output);
bool repack(const Path& input, const Path& output);
