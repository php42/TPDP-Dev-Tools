/*
    Copyright 2019 php42

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
#include <string>
#include <cstdint>
#include "typedefs.h"

bool copy_file(const std::string& src, const std::string& dest);
bool copy_file(const std::wstring& src, const std::wstring& dest);

bool create_directory(const std::string& dir);
bool create_directory(const std::wstring& dir);

/* read an entire file into memory. returns a unique_ptr to the memory block, or NULL on failure.
 * after a successful call, 'size' will contain the size of the returned memory block. */
FileBuf read_file(const std::string& file, std::size_t& size);
FileBuf read_file(const std::wstring& file, std::size_t& size);

/* read an entire file into memory. returns a unique_ptr to the memory block, or NULL on failure.
 * after a successful call, 'size' will contain the size of the returned memory block.
 * allocated with _aligned_malloc, deleter is _aligned_free
 * this is micro-optimazation nonsense for SSE/AVX routines */
AlignedFileBuf aligned_read_file(const std::string& file, std::size_t& size, std::size_t alignment);
AlignedFileBuf aligned_read_file(const std::wstring& file, std::size_t& size, std::size_t alignment);

bool write_file(const std::string& file, const void *buf, std::size_t len);
bool write_file(const std::wstring& file, const void *buf, std::size_t len);

bool path_exists(const std::string& path);
bool path_exists(const std::wstring& path);
