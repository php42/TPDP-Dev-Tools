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

#include <algorithm>
#include <sys/stat.h>
#include <cassert>
#include "ziparchive.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

constexpr std::size_t ENTRY_READ_BUF_SIZE = 1024 * 32; // 32K
constexpr unsigned int STAT_MASK = ZIP_STAT_NAME | ZIP_STAT_SIZE | ZIP_STAT_INDEX;

void ZipFile::reserve(std::size_t sz)
{
    if(!data_)
    {
        data_ = std::make_unique<unsigned char[]>(sz);
        capacity_ = sz;
        sz_ = 0;
        return;
    }

    if(sz <= capacity_)
        return;

    auto buf = std::make_unique<unsigned char[]>(sz);
    memcpy(buf.get(), data_.get(), std::min(sz, sz_));
    data_ = std::move(buf);
    capacity_ = sz;
}

void ZipFile::resize(std::size_t sz)
{
    if(sz > capacity_)
        reserve(sz + ENTRY_READ_BUF_SIZE); // over-allocate to avoid reallocations
    sz_ = sz;
}

void ZipArchive::open(const std::filesystem::path& filepath, int flags)
{
    close();
    zip_error_init(&err_);

    src_ = zip_source_win32w_create(filepath.c_str(), 0, 0, &err_);
    if(src_ == nullptr)
        throw ZipError(zip_error_strerror(&err_));

    arc_ = zip_open_from_source(src_, flags, &err_);
    if(arc_ == nullptr)
    {
        zip_source_free(src_);
        src_ = nullptr;

        if(zip_error_code_zip(&err_) == ZIP_ER_NOZIP)
            throw ZipWrongFmt(zip_error_strerror(&err_));
        else
            throw ZipError(zip_error_strerror(&err_));
    }
}

void ZipArchive::close()
{
    if(arc_)
    {
        if(zip_close(arc_) != 0)
            zip_discard(arc_);
        arc_ = nullptr;
        src_ = nullptr; // src_ automatically freed along with arc_
    }

    zip_error_fini(&err_);
    err_ = { 0 };
}

std::vector<ZipEntry> ZipArchive::get_file_table()
{
    auto num_entries = zip_get_num_entries(arc_, 0);
    if(num_entries <= 0)
        return {};

    std::vector<ZipEntry> files;
    files.reserve(num_entries);

    for(std::size_t i = 0; i < (std::size_t)num_entries; ++i)
    {
        zip_stat_t s = { 0 };
        zip_stat_init(&s);

        if(zip_stat_index(arc_, (zip_uint64_t)i, ZIP_FL_ENC_RAW, &s) != 0)
        {
            auto err = zip_strerror(arc_);
            throw ZipError(std::string("Zip decoder failed to read entry ") + std::to_string(i) + ": " + err);
        }
        else if((s.valid & STAT_MASK) != STAT_MASK)
        {
            throw ZipError(std::string("Zip stat missing required fields at index: ") + std::to_string(i));
        }

        zip_uint8_t opsys;
        zip_uint32_t attr;
        if(zip_file_get_external_attributes(arc_, i, 0, &opsys, &attr) != 0)
        {
            auto err = zip_strerror(arc_);
            throw ZipError(std::string("Zip decoder failed to read entry attributes at index ") + std::to_string(i) + ": " + err);
        }

        if(opsys == ZIP_OPSYS_DOS)
        {
            if(attr & FILE_ATTRIBUTE_DIRECTORY)
                continue;
        }
        else if(attr & S_IFDIR)
        {
            continue;
        }

        files.push_back({ s.name, s.index });
    }

    return files;
}

ZipEntry ZipArchive::add_file(const std::string& filepath, const std::filesystem::path& data_src, zip_flags_t flags)
{
    assert(arc_ != nullptr);
    if(arc_ == nullptr)
        throw ZipError("Null zip archive");

    auto src = zip_source_win32w_create(data_src.c_str(), 0, 0, &err_);
    if(src == nullptr)
        throw ZipError(zip_error_strerror(&err_));

    auto index = zip_file_add(arc_, filepath.c_str(), src, flags);
    if(index < 0)
    {
        zip_source_free(src);
        throw ZipError(zip_strerror(arc_));
    }

    return { filepath, (zip_uint64_t)index };
}

ZipFile ZipArchive::get_file(const ZipEntry& entry)
{
    ZipFile ret;
    zip_stat_t s = { 0 };
    zip_stat_init(&s);

    auto status = zip_stat_index(arc_, entry.index, ZIP_FL_ENC_RAW, &s);
    if(status != 0)
    {
        throw ZipError(zip_strerror(arc_));
    }
    else if((s.valid & STAT_MASK) != STAT_MASK)
    {
        throw ZipError("Zip stat missing required fields");
    }

    auto file = zip_fopen_index(arc_, s.index, ZIP_FL_ENC_RAW);
    if(file == nullptr)
        throw ZipError(zip_strerror(arc_));

    try
    {
        ret.reserve((s.size > 0) ? (std::size_t)s.size : ENTRY_READ_BUF_SIZE);

        for(;;)
        {
            if(ret.size() >= ret.capacity())
                ret.reserve(ret.size() + ENTRY_READ_BUF_SIZE);

            auto len = zip_fread(file, &ret.data()[ret.size()], ret.capacity() - ret.size());
            if(len == 0)
            {
                break;
            }
            else if(len < 0)
            {
                throw ZipError(zip_file_strerror(file));
            }

            ret.resize(ret.size() + len);
        }
    }
    catch(...)
    {
        zip_fclose(file);
        throw;
    }

    zip_fclose(file);

    if(!ret || !ret.size())
        throw ZipError("Unknown error");

    return ret;
}
