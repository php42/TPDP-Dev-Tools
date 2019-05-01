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

#include "filesystem.h"
#include <cassert>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <filesystem>
#include <fstream>
#endif // _WIN32


bool copy_file(const std::string& src, const std::string& dest)
{
#ifdef _WIN32
    return (CopyFileA(src.c_str(), dest.c_str(), FALSE) != 0);
#else
    std::error_code ec;
    std::filesystem::copy_file(src, dest, ec);
    return !ec;
#endif // _WIN32
}

bool copy_file(const std::wstring& src, const std::wstring& dest)
{
#ifdef _WIN32
    return (CopyFileW(src.c_str(), dest.c_str(), FALSE) != 0);
#else
    std::error_code ec;
    std::filesystem::copy_file(src, dest, ec);
    return !ec;
#endif // _WIN32
}

FileBuf read_file(const std::wstring& file, std::size_t& size)
{
#ifdef _WIN32
    HANDLE infile;

    infile = CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if(infile == INVALID_HANDLE_VALUE)
        return FileBuf();

    LARGE_INTEGER len;
    DWORD bytes_read;
    if(!GetFileSizeEx(infile, &len))
    {
        CloseHandle(infile);
        return FileBuf();
    }

    FileBuf buf(new char[(unsigned int)len.QuadPart]);

    if(!ReadFile(infile, buf.get(), (DWORD)len.QuadPart, &bytes_read, NULL))
    {
        CloseHandle(infile);
        return FileBuf();
    }

    CloseHandle(infile);

    if(bytes_read != (DWORD)len.QuadPart)
    {
        return FileBuf();
    }

    size = (std::size_t)len.QuadPart;
    return buf;
#else
    std::filesystem::path path(file);
    std::ifstream infile(path.native(), std::ios::binary | std::ios::ate);
    if(!infile)
        return FileBuf();

    std::size_t len = infile.tellg();

    if(len == 0)
        return FileBuf();

    infile.seekg(0);

    FileBuf buf(new char[len]);

    if(!infile.read(buf.get(), len))
    {
        return FileBuf();
    }

    size = len;
    return buf;
#endif // _WIN32
}

FileBuf read_file(const std::string& file, std::size_t& size)
{
#ifdef _WIN32
    HANDLE infile;

    infile = CreateFileA(file.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if(infile == INVALID_HANDLE_VALUE)
        return FileBuf();

    LARGE_INTEGER len;
    DWORD bytes_read;
    if(!GetFileSizeEx(infile, &len))
    {
        CloseHandle(infile);
        return FileBuf();
    }

    FileBuf buf(new char[(unsigned int)len.QuadPart]);

    if(!ReadFile(infile, buf.get(), (DWORD)len.QuadPart, &bytes_read, NULL))
    {
        CloseHandle(infile);
        return FileBuf();
    }

    CloseHandle(infile);

    if(bytes_read != (DWORD)len.QuadPart)
    {
        return FileBuf();
    }

    size = (std::size_t)len.QuadPart;
    return buf;
#else
    std::ifstream infile(file, std::ios::binary | std::ios::ate);
    if(!infile)
        return FileBuf();

    std::size_t len = infile.tellg();

    if(len == 0)
        return FileBuf();

    infile.seekg(0);

    FileBuf buf(new char[len]);

    if(!infile.read(buf.get(), len))
    {
        return FileBuf();
    }

    size = len;
    return buf;
#endif // _WIN32
}

AlignedFileBuf aligned_read_file(const std::wstring& file, std::size_t& size, std::size_t alignment)
{
    HANDLE infile;

    assert((alignment != 0) && ((alignment & (alignment - 1)) == 0)); // alignment must be power of 2
    if((alignment == 0) || ((alignment & (alignment - 1)) != 0))
        return AlignedFileBuf();

    infile = CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if(infile == INVALID_HANDLE_VALUE)
        return AlignedFileBuf();

    LARGE_INTEGER len;
    DWORD bytes_read;
    if(!GetFileSizeEx(infile, &len))
    {
        CloseHandle(infile);
        return AlignedFileBuf();
    }

    auto ptr = _aligned_malloc((std::size_t)len.QuadPart, alignment);
    if(ptr == nullptr)
        return AlignedFileBuf();

    AlignedFileBuf buf((char*)ptr, _aligned_free);

    if(!ReadFile(infile, buf.get(), (DWORD)len.QuadPart, &bytes_read, NULL))
    {
        CloseHandle(infile);
        return AlignedFileBuf();
    }

    CloseHandle(infile);

    if(bytes_read != (DWORD)len.QuadPart)
    {
        return AlignedFileBuf();
    }

    size = (std::size_t)len.QuadPart;
    return buf;
}

AlignedFileBuf aligned_read_file(const std::string& file, std::size_t& size, std::size_t alignment)
{
    HANDLE infile;

    assert((alignment != 0) && ((alignment & (alignment - 1)) == 0)); // alignment must be power of 2
    if((alignment == 0) || ((alignment & (alignment - 1)) != 0))
        return AlignedFileBuf();

    infile = CreateFileA(file.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if(infile == INVALID_HANDLE_VALUE)
        return AlignedFileBuf();

    LARGE_INTEGER len;
    DWORD bytes_read;
    if(!GetFileSizeEx(infile, &len))
    {
        CloseHandle(infile);
        return AlignedFileBuf();
    }

    auto ptr = _aligned_malloc((std::size_t)len.QuadPart, alignment);
    if(ptr == nullptr)
        return AlignedFileBuf();

    AlignedFileBuf buf((char*)ptr, _aligned_free);

    if(!ReadFile(infile, buf.get(), (DWORD)len.QuadPart, &bytes_read, NULL))
    {
        CloseHandle(infile);
        return AlignedFileBuf();
    }

    CloseHandle(infile);

    if(bytes_read != (DWORD)len.QuadPart)
    {
        return AlignedFileBuf();
    }

    size = (std::size_t)len.QuadPart;
    return buf;
}

bool create_directory(const std::string& dir)
{
#ifdef _WIN32
    return ((CreateDirectoryA(dir.c_str(), NULL) != 0) || (GetLastError() == ERROR_ALREADY_EXISTS));
#else
    std::error_code ec;
    return std::filesystem::create_directory(dir, ec);
#endif // _WIN32
}

bool create_directory(const std::wstring& dir)
{
#ifdef _WIN32
    return ((CreateDirectoryW(dir.c_str(), NULL) != 0) || (GetLastError() == ERROR_ALREADY_EXISTS));
#else
    std::error_code ec;
    return std::filesystem::create_directory(dir, ec);
#endif // _WIN32
}

bool write_file(const std::string& file, const void *buf, std::size_t len)
{
#ifdef _WIN32
    HANDLE outfile = CreateFileA(file.c_str(), GENERIC_WRITE, /*FILE_SHARE_READ*/ 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if(outfile == INVALID_HANDLE_VALUE)
		return false;

    DWORD bytes_written = 0;
    bool ret = (WriteFile(outfile, buf, (DWORD)len, &bytes_written, NULL) != 0);
    ret = (ret && (bytes_written == (DWORD)len));
    CloseHandle(outfile);
    return ret;
#else
    std::ofstream outfile(file, std::ios::binary);
    if(!outfile)
        return false;

    if(!outfile.write((const char*)buf, len))
        return false;
    return true;
#endif // _WIN32
}

bool write_file(const std::wstring& file, const void *buf, std::size_t len)
{
#ifdef _WIN32
    HANDLE outfile = CreateFileW(file.c_str(), GENERIC_WRITE, /*FILE_SHARE_READ*/ 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if(outfile == INVALID_HANDLE_VALUE)
		return false;

    DWORD bytes_written = 0;
    bool ret = (WriteFile(outfile, buf, (DWORD)len, &bytes_written, NULL) != 0);
    ret = (ret && (bytes_written == (DWORD)len));
    CloseHandle(outfile);
    return ret;
#else
    std::filesystem::path path(file);
    std::ofstream outfile(path.native(), std::ios::binary);
    if(!outfile)
        return false;

    if(!outfile.write((const char*)buf, len))
        return false;
    return true;
#endif // _WIN32
}

bool path_exists(const std::string& path)
{
#ifdef _WIN32
    return (GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES);
#else
    std::error_code ec;
    return (std::filesystem::exists(path, ec) && !ec);
#endif
}

bool path_exists(const std::wstring& path)
{
#ifdef _WIN32
    return (GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES);
#else
    std::error_code ec;
    return (std::filesystem::exists(path, ec) && !ec);
#endif
}
