/*
    Copyright (C) 2016 php42

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once
#include <string>
#include <cstdint>
#include <memory>
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
