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

#include "legacy.h"
#include <boost/algorithm/string.hpp>
#include "../common/filesystem.h"
#include "../common/textconvert.h"
#include "../common/console.h"
#include "../common/endian.h"
#include <google/vcencoder.h>
#include <google/vcdecoder.h>
#include <libtpdp.h>
#include <iostream>
#include <boost/crc.hpp>
#include <tuple>
#include <fstream>
#include <thread>
#include <future>
#include <mutex>
#include <optional>
#include <unordered_map>
#include "zlib.h"

constexpr uint8_t DIFF_FILE_VERSION = 2;
static const char DIFF_FILE_MAGIC[] = { 'T','P','D','P','D','i','f','f' };
constexpr std::size_t INFLATE_BUF_SIZE = 16384;

namespace algo = boost::algorithm;
namespace fs = std::filesystem;

enum CompressionType
{
    COMPRESSION_NONE = 0,
    COMPRESSION_ZLIB = 1
};

/* enter the spaghetti zone */

class DiffFileHeaderV1
{
public:
    char magic[sizeof(DIFF_FILE_MAGIC)];
    uint8_t version;
    uint8_t mode;
    uint8_t ynk;

    DiffFileHeaderV1() = default;
    DiffFileHeaderV1(const void *src) { read(src); }

    static constexpr auto size = sizeof(magic) + 3;

    void read(const void *src)
    {
        auto s = (const unsigned char*)src;
        memcpy(magic, s, sizeof(magic));
        version = s[sizeof(magic)];
        mode = s[sizeof(magic) + 1];
        ynk = s[sizeof(magic) + 2];
    }

    void write(void *dst) const
    {
        auto d = (unsigned char*)dst;
        memcpy(d, magic, sizeof(magic));
        d[sizeof(magic)] = version;
        d[sizeof(magic) + 1] = mode;
        d[sizeof(magic) + 2] = ynk;
    }

    void write(std::ofstream& dst) const
    {
        char buf[size];
        write(buf);
        dst.write(buf, size);
    }
};

class DiffFileHeader
{
public:
    char magic[sizeof(DIFF_FILE_MAGIC)];
    uint8_t version;
    uint8_t mode;
    uint8_t ynk;
    uint8_t compression;

    DiffFileHeader() = default;
    DiffFileHeader(const void *src, bool v1_compat) { read(src, v1_compat); }

    static constexpr auto size = sizeof(magic) + 4;

    void read(const void *src, bool v1_compat)
    {
        auto s = (const unsigned char*)src;
        memcpy(magic, s, sizeof(magic));
        version = s[sizeof(magic)];
        mode = s[sizeof(magic) + 1];
        ynk = s[sizeof(magic) + 2];
        if(v1_compat)
            compression = COMPRESSION_NONE;
        else
            compression = s[sizeof(magic) + 3];
    }

    void write(void *dst) const
    {
        auto d = (unsigned char*)dst;
        memcpy(d, magic, sizeof(magic));
        d[sizeof(magic)] = version;
        d[sizeof(magic) + 1] = mode;
        d[sizeof(magic) + 2] = ynk;
        d[sizeof(magic) + 3] = compression;
    }

    void write(std::ofstream& dst) const
    {
        char buf[size];
        write(buf);
        dst.write(buf, size);
    }

    void init()
    {
        memcpy(magic, DIFF_FILE_MAGIC, sizeof(magic));
        version = DIFF_FILE_VERSION;
        mode = 0;
        ynk = 0;
        compression = COMPRESSION_NONE;
    }
};

class DiffHeader
{
public:
    uint32_t crc;
    uint32_t data_len;
    uint8_t arc_num;

    static constexpr auto size = sizeof(crc) + sizeof(data_len) + sizeof(arc_num);

    DiffHeader() = default;
    DiffHeader(const void *src) { read(src); }

    void read(const void *src)
    {
        auto s = (const unsigned char*)src;
        crc = read_le32(s);
        data_len = read_le32(&s[4]);
        arc_num = s[8];
    }

    void write(void *dst) const
    {
        auto d = (unsigned char*)dst;
        write_le32(d, crc);
        write_le32(&d[4], data_len);
        d[8] = arc_num;
    }

    void write(std::ofstream& dst) const
    {
        char buf[size];
        write(buf);
        dst.write(buf, size);
    }
};

typedef std::tuple<uint32_t, std::string, std::string, uint8_t> FileDiff;
typedef std::tuple<uint32_t, std::string, uint8_t, bool> ArcDiff;

// Cheap hack, don't look
static bool compress(const Path& path)
{
    try
    {
        std::size_t orig_sz;
        auto file = read_file(path.wstring(), orig_sz);
        if(!file)
            throw DiffgenException("Failed to read file.");

        if(orig_sz <= DiffFileHeader::size)
            throw DiffgenException("File corrupt.");

        // Allocate no more than original file size,
        // we'll abort if compression expands the data.
        std::unique_ptr<char[]> out = std::make_unique<char[]>(orig_sz);
        DiffFileHeader hdr;
        hdr.read(file.get(), false);
        if(memcmp(hdr.magic, DIFF_FILE_MAGIC, sizeof(DIFF_FILE_MAGIC)) != 0)
            throw DiffgenException("Bad signature.");
        else if(hdr.version != DIFF_FILE_VERSION)
            throw DiffgenException("Wrong diff version.");

        hdr.compression = COMPRESSION_ZLIB;
        hdr.write(out.get());

        z_stream strm = { 0 };
        strm.zalloc = Z_NULL;
        strm.zfree = Z_NULL;
        strm.opaque = Z_NULL;

        auto strm_sz = orig_sz - DiffFileHeader::size;
        int ret = deflateInit(&strm, Z_BEST_COMPRESSION);
        if(ret != Z_OK)
            throw DiffgenException("zlib initialization failed.");

        strm.avail_in = (uInt)strm_sz;
        strm.next_in = (Bytef*)&file.get()[DiffFileHeader::size];
        strm.avail_out = (uInt)strm_sz;
        strm.next_out = (Bytef*)&out.get()[DiffFileHeader::size];

        ret = deflate(&strm, Z_FINISH);
        std::size_t avail_out = strm.avail_out;
        auto out_sz = strm_sz - avail_out;
        deflateEnd(&strm);
        file.reset();

        if(avail_out == 0)
        {
            ScopedConsoleLock lock;
            std::cout << "Note: using uncompressed stream for reduced file size." << std::endl;
            return true;
        }
        else if(ret != Z_STREAM_END)
        {
            throw DiffgenException("Deflate returned: " + std::to_string(ret));
        }

        std::cout << "Compression ratio: " << ((double)out_sz / (double)strm_sz) << std::endl;
        if(!write_file(path.wstring(), out.get(), out_sz + DiffFileHeader::size))
            throw DiffgenException("Failed to write to file.");
    }
    catch(const std::exception& ex)
    {
        ScopedConsoleColorMT color(COLOR_CRITICAL);
        std::cerr << "Error compressing file: " << path.string() << std::endl;
        std::cerr << ex.what() << std::endl;
        return false;
    }

    return true;
}

static std::unique_ptr<char[]> decompress(const void *src, const std::size_t len, std::size_t& out_sz)
{
    int ret;
    z_stream strm = { 0 };
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = (uInt)len;
    strm.next_in = (Bytef*)src;
    ret = inflateInit(&strm);
    if(ret != Z_OK)
        throw DiffgenException("Inflate Initialization failed.");

    auto out_capacity = std::max(len, INFLATE_BUF_SIZE);
    out_sz = 0;
    auto buf = std::make_unique<char[]>(out_capacity);

    do
    {
        auto avail = (out_capacity - out_sz);
        if(avail < INFLATE_BUF_SIZE)
        {
            try
            {
                out_capacity += INFLATE_BUF_SIZE;
                auto temp = std::make_unique<char[]>(out_capacity);
                memcpy(temp.get(), buf.get(), out_sz);
                buf = std::move(temp);
                avail = (out_capacity - out_sz);
            }
            catch(const std::exception&)
            {
                inflateEnd(&strm);
                throw;
            }
        }

        strm.avail_out = (uInt)avail;
        strm.next_out = (Bytef*)&buf.get()[out_sz];
        ret = inflate(&strm, Z_NO_FLUSH);
        if((ret != Z_OK) && (ret != Z_STREAM_END))
        {
            inflateEnd(&strm);
            throw DiffgenException("Inflate returned: " + std::to_string(ret));
        }

        out_sz += (avail - strm.avail_out);
    } while(strm.avail_out == 0);

    inflateEnd(&strm);

    if(ret != Z_STREAM_END)
        throw DiffgenException("Incomplete stream.");

    return buf;
}

/* worker thread for mode 1 diff generation */
static std::vector<FileDiff> fileworker(const Path *input, const std::vector<Path> *rel_paths, const libtpdp::Archive *arc, int arc_num, std::size_t begin, std::size_t end)
{
    std::vector<FileDiff> diffs;

    for(auto i = begin; i < end; ++i)
    {
        const Path& relative_path = (*rel_paths)[i];
        Path path = (*input) / relative_path;

        auto it = arc->find(utf_to_sjis(relative_path.wstring()));
        if(it >= arc->end())
        {
            ScopedConsoleColorMT color(COLOR_WARN);
            std::cerr << "Skipping file: " << relative_path.string() << std::endl;
            std::cerr << "File not present in archive. Adding files is not supported in mode 1." << std::endl;
            continue;
        }

        auto src_file = arc->get_file(it);
        if(!src_file)
            throw DiffgenException("Error extracting file: " + relative_path.string());

        std::size_t sz;
        auto dst_file = read_file(path.wstring(), sz);
        if(!dst_file)
            throw DiffgenException("Failed to read file: " + path.string());

        if((src_file.size() == sz) && (memcmp(src_file.data(), dst_file.get(), sz) == 0))
            continue;

        boost::crc_32_type src_crc;

        src_crc.process_bytes(src_file.data(), src_file.size());

        std::string diff_output;
        open_vcdiff::VCDiffEncoder encoder(src_file.data(), src_file.size());
        if(!encoder.Encode(dst_file.get(), sz, &diff_output))
            throw DiffgenException("Error generating diff for file: " + relative_path.string());

        diffs.emplace_back((uint32_t)src_crc.checksum(), relative_path.string() + '\0', std::move(diff_output), (uint8_t)arc_num);
    }

    return diffs;
}

/* worker thread for mode 2 diff generation */
static std::optional<ArcDiff> arcworker(Path input, Path output, int arc_num)
{
    libtpdp::Archive arc, new_arc;
    bool suppress_json_warning = false;

    try
    {
        {
            ScopedConsoleLock lock;
            std::cout << ">> " << input.string() << std::endl;
        }
        arc.open(input.wstring());
    }
    catch(const libtpdp::ArcError& ex)
    {
        throw DiffgenException("Failed to open file: " + input.string() + "\r\n" + ex.what());
    }

    for(auto& entry : fs::recursive_directory_iterator(output))
    {
        if(!entry.is_regular_file())
            continue;

        if(algo::iequals(entry.path().extension().string(), ".json"))
        {
            if(!suppress_json_warning)
            {
                ScopedConsoleColorMT color(COLOR_WARN);
                std::cerr << "Skipping json files..." << std::endl;
                suppress_json_warning = true;
            }
            continue;
        }

        /* kind of a hack to get the path relative to
         * the root of the archive */
        auto temp = entry.path().wstring();
        temp = temp.substr(output.wstring().size());
        while(temp.find_first_of(L"\\/") == 0)
            temp.erase(0, 1);

        Path relative_path(temp);

        auto it = arc.find(utf_to_sjis(relative_path.wstring()));
        if(it >= arc.end())
        {
            {
                ScopedConsoleColorMT color(COLOR_OK);
                std::cout << "Inserting new file: " << entry.path().string() << std::endl;
            }

            std::size_t sz;
            auto dst_file = read_file(entry.path().wstring(), sz);
            if(!dst_file)
                throw DiffgenException("Failed to read file: " + entry.path().string());

            if(!new_arc)
                new_arc = libtpdp::Archive(arc.data(), arc.size(), arc.is_ynk());

            auto new_it = new_arc.insert(dst_file.get(), sz, utf_to_sjis(relative_path.wstring()));
            if(new_it >= new_arc.end())
                throw DiffgenException("Failed to insert file: " + entry.path().string());

            continue;
        }

        auto src_file = arc.get_file(it);
        if(!src_file)
            throw DiffgenException("Error extracting file: " + relative_path.string() + "\r\nFrom archive: " + input.string());

        std::size_t sz;
        auto dst_file = read_file(entry.path().wstring(), sz);
        if(!dst_file)
            throw DiffgenException("Failed to read file: " + entry.path().string());

        if((src_file.size() == sz) && (memcmp(src_file.data(), dst_file.get(), sz) == 0))
            continue;

        if(!new_arc)
            new_arc = libtpdp::Archive(arc.data(), arc.size(), arc.is_ynk());

        auto new_it = new_arc.repack_file(utf_to_sjis(relative_path.wstring()), dst_file.get(), sz);
        if(new_it >= new_arc.end())
            throw DiffgenException("Error repacking file: " + relative_path.string());
    }

    if(!new_arc)
        return {};

    boost::crc_32_type crc;
    crc.process_bytes(arc.data(), arc.size());

    std::string diff_output;
    open_vcdiff::VCDiffEncoder encoder(arc.data(), arc.size());
    if(!encoder.Encode(new_arc.data(), new_arc.size(), &diff_output))
        throw DiffgenException("Error generating diff for file: " + input.string());

    return ArcDiff(crc.checksum(), std::move(diff_output), (uint8_t)arc_num, arc.is_ynk());
}

static bool diff_files(const Path& input, const Path& output, const Path& diff_path, int threads)
{
    std::vector<FileDiff> diffs;
    libtpdp::Archive arc;
    bool suppress_json_warning = false;
    bool ynk = false;
    if(threads < 1)
        threads = 1;

    if(threads > 1)
        std::cout << "Using up to " << threads << " concurrent threads" << std::endl;

    Path in_dir(input / L"dat");
    for(int i = 1; i < 7; ++i)
    {
        auto arc_name = (L"gn_dat" + std::to_wstring(i) + L".arc");
        auto arc_path = in_dir / arc_name;
        auto out_dir = output / arc_name;
        std::vector<Path> rel_paths;

        if(!fs::exists(arc_path) || !fs::is_regular_file(arc_path))
        {
            std::cerr << "File not found: " << arc_path.string() << std::endl;
            continue;
        }

        if(!fs::exists(out_dir) || !fs::is_directory(out_dir))
        {
            std::cerr << "Missing directory: " << out_dir.string() << std::endl;
            continue;
        }

        try
        {
            std::cout << ">> " << arc_path.string() << std::endl;
            arc.open(arc_path.wstring());
        }
        catch(const libtpdp::ArcError& ex)
        {
            ScopedConsoleColorChanger color(COLOR_CRITICAL);
            std::cerr << "Failed to open file: " << arc_path.string() << std::endl;
            std::cerr << ex.what() << std::endl;
            return false;
        }

        ynk = arc.is_ynk();

        for(auto& entry : fs::recursive_directory_iterator(out_dir))
        {
            if(!entry.is_regular_file())
                continue;

            if(algo::iequals(entry.path().extension().string(), ".json"))
            {
                if(!suppress_json_warning)
                {
                    ScopedConsoleColorChanger color(COLOR_WARN);
                    std::cerr << "Skipping json files..." << std::endl;
                    suppress_json_warning = true;
                }
                continue;
            }

            /* kind of a hack to get the path relative to
             * the root of the archive */
            auto temp = entry.path().wstring();
            temp = temp.substr(out_dir.wstring().size());
            while(temp.find_first_of(L"\\/") == 0)
                temp.erase(0, 1);

            rel_paths.emplace_back(temp);
        }

        std::vector<std::future<std::vector<FileDiff>>> futures;
        auto arc_threads = threads;
        auto files_per_thread = rel_paths.size() / arc_threads;
        if(files_per_thread < 2)
            arc_threads = 1;

        auto flags = (arc_threads > 1) ? std::launch::async : std::launch::deferred;

        std::size_t begin = 0;
        for(int j = 0; j < arc_threads; ++j)
        {
            auto end = (j == (arc_threads - 1)) ? rel_paths.size() : (begin + files_per_thread);
            futures.emplace_back(std::async(flags, fileworker, &out_dir, &rel_paths, &arc, i, begin, end));
            begin = end;
        }

        try
        {
            for(auto& j : futures)
            {
                auto val = j.get();
                for(auto& k : val)
                    diffs.emplace_back(std::move(k));
            }
        }
        catch(const std::exception& ex)
        {
            ScopedConsoleColorMT color(COLOR_CRITICAL);
            std::cerr << arc_path.string() << ": Exception" << std::endl;
            std::cerr << ex.what() << std::endl;
            for(auto& j : futures)
                if(j.valid())
                    j.wait();

            return false;
        }
    }

    if(diffs.empty())
    {
        ScopedConsoleColorChanger color(COLOR_WARN);
        std::cerr << "Source and target are identical, no diffs to output!" << std::endl;
        return false;
    }

    DiffFileHeader file_header;
    file_header.init();
    file_header.mode = 1;
    file_header.ynk = ynk ? 1 : 0;

    std::ofstream diff_file(diff_path, std::ios::binary | std::ios::trunc);
    file_header.write(diff_file);

    for(auto& i : diffs)
    {
        auto&[crc, name, data, arc_num] = i;
        DiffHeader header;

        header.crc = crc;
        header.data_len = (uint32_t)data.size();
        header.arc_num = arc_num;

        header.write(diff_file);
        diff_file.write(name.data(), name.size());
        diff_file.write(data.data(), data.size());

        if(!diff_file)
            throw DiffgenException("Failed to write to file: " + diff_path.string());
    }

    diff_file.close();
    std::cout << "Compressing..." << std::endl;
    return compress(diff_path);
}

static bool diff_archive(const Path& input, const Path& output, const Path& diff_path, int threads)
{
    std::vector<ArcDiff> diffs;
    std::vector<std::future<std::optional<ArcDiff>>> futures;
    Path in_dir(input / L"dat");
    if(threads < 1)
        threads = 1;
    if(threads > 6)
        threads = 6;

    if(threads > 1)
        std::cout << "Using up to " << threads << " concurrent threads" << std::endl;

    auto flags = (threads > 1) ? std::launch::async : std::launch::deferred;

    try
    {
        for(int i = 1; i < 7; ++i)
        {
            libtpdp::Archive new_arc;
            auto arc_name = (L"gn_dat" + std::to_wstring(i) + L".arc");
            auto arc_path = in_dir / arc_name;
            auto out_dir = output / arc_name;

            if(!fs::exists(arc_path) || !fs::is_regular_file(arc_path))
            {
                std::cerr << "File not found: " << arc_path.string() << std::endl;
                continue;
            }

            if(!fs::exists(out_dir) || !fs::is_directory(out_dir))
            {
                std::cerr << "Missing directory: " << out_dir.string() << std::endl;
                continue;
            }

            if(futures.size() >= threads)
            {
                while(!futures.empty())
                {
                    auto temp = futures.back().get();
                    if(temp)
                        diffs.push_back(std::move(temp.value()));
                    futures.pop_back();
                }
            }

            futures.push_back(std::async(flags, arcworker, arc_path, out_dir, i));
        }

        while(!futures.empty())
        {
            auto temp = futures.back().get();
            if(temp)
                diffs.push_back(std::move(temp.value()));
            futures.pop_back();
        }
    }
    catch(const std::exception&)
    {
        for(auto& i : futures)
        {
            if(i.valid())
                i.wait();
        }

        throw;
    }

    if(diffs.empty())
    {
        ScopedConsoleColorChanger color(COLOR_WARN);
        std::cerr << "Source and target are identical, no diffs to output!" << std::endl;
        return false;
    }

    DiffFileHeader file_header;
    file_header.init();
    file_header.mode = 2;
    file_header.ynk = std::get<3>(diffs[0]) ? 1 : 0;

    std::ofstream diff_file(diff_path, std::ios::binary | std::ios::trunc);
    file_header.write(diff_file);

    for(auto& i : diffs)
    {
        auto&[crc, data, arc_num, ynk] = i;
        DiffHeader diff_header;

        diff_header.crc = crc;
        diff_header.data_len = (uint32_t)data.size();
        diff_header.arc_num = arc_num;

        diff_header.write(diff_file);
        diff_file.write(data.data(), data.size());
    }

    diff_file.close();
    std::cout << "Compressing..." << std::endl;
    return compress(diff_path);
}

bool legacy_diff(const Path& input, const Path& output, const Path& diff_path, int diff_mode, int threads)
{
    switch(diff_mode)
    {
    case 1:
        return diff_files(input, output, diff_path, threads);

    case 2:
        return diff_archive(input, output, diff_path, threads);

    default:
        std::cerr << "Error: Invalid diff-mode." << std::endl;
        return false;
    }
}

bool legacy_patch(const Path& input, const Path& output)
{
    std::unordered_map<int, std::vector<std::tuple<uint32_t, std::string, std::string>>> diffs;
    libtpdp::Archive arc;

    std::size_t diff_count = 0;
    std::size_t diffs_applied = 0;

    std::size_t sz;
    auto diff_file = read_file(output.wstring(), sz);
    if(!diff_file || (sz < DiffFileHeader::size))
        throw DiffgenException("Failed to read file: " + output.string());

    auto buf = diff_file.get();
    DiffFileHeader file_header(buf, false);

    if(std::memcmp(file_header.magic, DIFF_FILE_MAGIC, sizeof(DIFF_FILE_MAGIC)) != 0)
        throw DiffgenException("Unrecognized file format: " + output.string());

    if((file_header.version != DIFF_FILE_VERSION) && (file_header.version != 1))
    {
        ScopedConsoleColorChanger color(COLOR_CRITICAL);
        std::cerr << "Unsupported file version: " << output.string() << std::endl;
        std::cerr << "Provided diff file is version " << (unsigned int)file_header.version << std::endl;
        std::cerr << "Supported version is " << (unsigned int)DIFF_FILE_VERSION << std::endl;
        if(file_header.version > DIFF_FILE_VERSION)
            std::cerr << "Please update to a newer version of TPDP-Dev-Tools." << std::endl;
        return false;
    }

    if(file_header.version < 2)
    {
        ScopedConsoleColorChanger color(COLOR_WARN);
        file_header.compression = COMPRESSION_NONE;
        std::cerr << "Note: using compatibilty mode for old v1 diff files." << std::endl;
    }
    else
    {
        ScopedConsoleColorChanger color(COLOR_WARN);
        std::cerr << "Note: using compatibilty mode for old v2 diff files." << std::endl;
    }

    int mode = file_header.mode;
    if(mode < 1 || mode > 2)
    {
        ScopedConsoleColorChanger color(COLOR_CRITICAL);
        std::cerr << "Unsupported diff mode: " << mode << std::endl;
        std::cerr << "Check for a newer version of TPDP-Dev-Tools." << std::endl;
        return false;
    }

    bool ynk = (file_header.ynk != 0);

    std::size_t pos = (file_header.version == 1) ? DiffFileHeaderV1::size : DiffFileHeader::size;

    if(file_header.compression)
    {
        if(file_header.compression != COMPRESSION_ZLIB)
        {
            ScopedConsoleColorChanger color(COLOR_CRITICAL);
            std::cerr << "Unsupported compression method: " << (unsigned int)file_header.compression << std::endl;
            std::cerr << "Check for a newer version of TPDP-Dev-Tools." << std::endl;
            return false;
        }

        auto strm_sz = (sz - DiffFileHeader::size);
        try
        {
            diff_file = decompress(&buf[DiffFileHeader::size], strm_sz, sz);
            buf = diff_file.get();
            pos = 0;
        }
        catch(const std::exception& ex)
        {
            ScopedConsoleColorChanger color(COLOR_CRITICAL);
            std::cerr << "Diff decompression failed: " << ex.what() << std::endl;
            return false;
        }
    }

    if(mode == 2)
    {
        while((sz - pos) >= DiffHeader::size)
        {
            DiffHeader header(&buf[pos]);
            std::string data;
            std::size_t data_len = header.data_len;
            pos += DiffHeader::size;

            if((sz - pos) < header.data_len || header.arc_num > 6 || header.arc_num < 1)
            {
                ScopedConsoleColorChanger color(COLOR_CRITICAL);
                std::cerr << "File corrupt: " << output.string() << std::endl;
                std::cerr << "Mode: " << mode << std::endl;
                std::cerr << "Archive number: " << (int)header.arc_num << std::endl;
                std::cerr << "Length field: " << std::hex << std::showbase << data_len << std::endl;
                std::cerr << "Remaining data: " << (sz - pos) << std::endl;
                std::cerr << "Header offset: " << (pos - DiffHeader::size) << std::endl;
                return false;
            }

            data.assign(&buf[pos], data_len);
            pos += data_len;

            Path in_dir(input / L"dat");
            auto arc_name = (L"gn_dat" + std::to_wstring(header.arc_num) + L".arc");
            auto arc_path = in_dir / arc_name;

            try
            {
                std::cout << ">> " << arc_path.string() << std::endl;
                arc.open(arc_path.wstring());
            }
            catch(const libtpdp::ArcError& ex)
            {
                ScopedConsoleColorChanger color(COLOR_CRITICAL);
                std::cerr << "Failed to open file: " << arc_path.string() << std::endl;
                std::cerr << ex.what() << std::endl;
                return false;
            }

            if(arc.is_ynk() != ynk)
                throw DiffgenException("Game version mismatch: attempting to patch base game with a diff for YnK/SoD or vice versa");

            ++diff_count;
            boost::crc_32_type src_crc;
            src_crc.process_bytes(arc.data(), arc.size());
            if(src_crc.checksum() != header.crc)
            {
                ScopedConsoleColorChanger color(COLOR_WARN);
                std::cerr << "CRC mismatch, skipping file: " << arc_path.string() << std::endl;
                continue;
            }

            std::string target;
            open_vcdiff::VCDiffDecoder decoder;
            if(!decoder.Decode(arc.data(), arc.size(), data, &target))
            {
                ScopedConsoleColorChanger color(COLOR_CRITICAL);
                std::cerr << "Error decoding diff for file: " << arc_path.string() << std::endl;
                std::cerr << "Diff file may be corrupt, aborting" << std::endl;
                return false;
            }

            arc.close(); // avoid using 3x the file size in memory :(
            libtpdp::Archive new_arc(target.data(), target.size(), ynk);

            if(!new_arc.save(arc_path.wstring()))
                throw DiffgenException("Failed to write to file: " + arc_path.string());
            ++diffs_applied;
        }

        std::cout << "Applied " << diffs_applied << "/" << diff_count << " diffs." << std::endl;
        if(diffs_applied == 0)
        {
            ScopedConsoleColorChanger color(COLOR_CRITICAL);
            std::cerr << "Failed to apply patch. Are you using the correct game version?" << std::endl;
            return false;
        }
        return true;
    }

    while((sz - pos) >= DiffHeader::size)
    {
        DiffHeader header(&buf[pos]);
        std::string path;
        
        /* make sure the path field is null-terminated */
        for(auto j = pos + DiffHeader::size; j < sz; ++j)
        {
            if(buf[j] == 0)
            {
                path = &buf[pos + DiffHeader::size];
                pos = j + 1;
                break;
            }
        }

        if(path.empty() || (sz - pos) < header.data_len || header.arc_num > 6 || header.arc_num < 1)
        {
            ScopedConsoleColorChanger color(COLOR_CRITICAL);
            std::cerr << "File corrupt: " << output.string() << std::endl;
            std::cerr << "Mode: " << mode << std::endl;
            std::cerr << "Archive number: " << (int)header.arc_num << std::endl;
            std::cerr << "Length field: " << std::hex << std::showbase << header.data_len << std::endl;
            std::cerr << "Remaining data: " << (sz - pos) << std::endl;
            std::cerr << "Pos: " << pos << std::endl;
            if(path.empty())
                std::cerr << "Relative path unterminated or empty." << std::endl;
            return false;
        }

        std::string data;
        data.assign(&buf[pos], header.data_len);
        diffs[header.arc_num].emplace_back(header.crc, std::move(path), std::move(data));
        pos += header.data_len;
    }

    if(diffs.empty())
    {
        ScopedConsoleColorChanger color(COLOR_CRITICAL);
        std::cerr << "Empty diff file: " << output.string() << std::endl;
        std::cerr << "No diffs to apply!" << std::endl;
        return false;
    }

    Path in_dir(input / L"dat");
    for(auto& it : diffs)
    {
        auto arc_name = (L"gn_dat" + std::to_wstring(it.first) + L".arc");
        auto arc_path = in_dir / arc_name;

        try
        {
            std::cout << ">> " << arc_path.string() << std::endl;
            arc.open(arc_path.wstring());
        }
        catch(const libtpdp::ArcError& ex)
        {
            ScopedConsoleColorChanger color(COLOR_CRITICAL);
            std::cerr << "Failed to open file: " << arc_path.string() << std::endl;
            std::cerr << ex.what() << std::endl;
            return false;
        }

        if(arc.is_ynk() != ynk)
            throw DiffgenException("Game version mismatch: attempting to patch base game with a diff for YnK/SoD or vice versa");

        for(auto& entry : it.second)
        {
            auto&[crc, path, data] = entry;

            auto file_iter = arc.find(path);
            if(file_iter >= arc.end())
            {
                ScopedConsoleColorChanger color(COLOR_WARN);
                std::cerr << "Skipping file: " << path << std::endl;
                std::cerr << "File not present in archive. Adding files is not supported in mode 1." << std::endl;
                continue;
            }

            auto file = arc.get_file(file_iter);
            if(!file)
            {
                ScopedConsoleColorChanger color(COLOR_CRITICAL);
                std::cerr << "Error extracting file: " << path << std::endl;
                std::cerr << "From archive: " << arc_path.string() << std::endl;
                return false;
            }

            ++diff_count;
            boost::crc_32_type src_crc;
            src_crc.process_bytes(file.data(), file.size());
            if(src_crc.checksum() != crc)
            {
                ScopedConsoleColorChanger color(COLOR_WARN);
                std::cerr << "CRC mismatch, skipping file: " << path << std::endl;
                continue;
            }

            std::string target;
            open_vcdiff::VCDiffDecoder decoder;
            if(!decoder.Decode(file.data(), file.size(), data, &target))
            {
                ScopedConsoleColorChanger color(COLOR_CRITICAL);
                std::cerr << "Error decoding diff for file: " << path << std::endl;
                std::cerr << "Diff file may be corrupt, aborting" << std::endl;
                return false;
            }

            if(arc.repack_file(file_iter, target.data(), target.size()) >= arc.end())
                throw DiffgenException("Error repacking file: " + path);
            ++diffs_applied;
        }

        if(!arc.save(arc_path.wstring()))
            throw DiffgenException("Failed to write to file: " + arc_path.string());
    }

    std::cout << "Applied " << diffs_applied << "/" << diff_count << " diffs." << std::endl;
    if(diffs_applied == 0)
    {
        ScopedConsoleColorChanger color(COLOR_CRITICAL);
        std::cerr << "Failed to apply patch. Are you using the correct game version?" << std::endl;
        return false;
    }
    return true;
}
