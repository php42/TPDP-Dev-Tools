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

#include <stdexcept>
#include <string>
#include <cstdint>
#include <memory>
#include <filesystem>
#include "zip.h"

class ZipError : public std::runtime_error
{
    using runtime_error::runtime_error;
};

class ZipWrongFmt : public ZipError
{
    using ZipError::ZipError;
};

struct ZipEntry
{
    std::string name;
    zip_uint64_t index;
};

// Container for files extracted from a zip archive
class ZipFile
{
private:
    std::unique_ptr<unsigned char[]> data_;
    std::size_t sz_, capacity_;

public:
    ZipFile() : data_(), sz_(0), capacity_(0) {}
    ZipFile(ZipFile&&) = default;
    ZipFile(const ZipFile&) = delete;
    ZipFile& operator=(ZipFile&&) = default;
    ZipFile& operator=(const ZipFile&) = delete;

    unsigned char *data() { return data_.get(); }
    const unsigned char *data() const { return data_.get(); }
    auto size() const { return sz_; }

    void reserve(std::size_t sz);
    std::size_t capacity() const { return capacity_; }

    void resize(std::size_t sz);

    explicit operator bool() const { return data_ != nullptr; }
};

class ZipArchive
{
private:
    zip_t *arc_;
    zip_source_t *src_;
    zip_error_t err_;

public:
    ZipArchive() : arc_(nullptr), src_(nullptr) { err_ = { 0 }; }
    ~ZipArchive() { close(); }

    void open(const std::filesystem::path& filepath, int flags = ZIP_RDONLY);
    void close();

    std::vector<ZipEntry> get_file_table();

    ZipEntry add_file(const std::string& filepath, const std::filesystem::path& data_src, zip_flags_t flags = ZIP_FL_OVERWRITE | ZIP_FL_ENC_UTF_8);
    ZipFile get_file(const ZipEntry& entry);
};
