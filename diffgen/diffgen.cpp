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

#include "diffgen.h"
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
#include <unordered_map>

constexpr uint8_t DIFF_FILE_VERSION = 1;
static const char DIFF_FILE_MAGIC[] = { 'T','P','D','P','D','i','f','f' };

namespace algo = boost::algorithm;
namespace fs = std::filesystem;

/* enter the spaghetti zone */

/* TODO: try to combine the diff_archive() and diff_files() methods into one,
 * as they share a lot of common logic */

#pragma pack(push, 1)
struct DiffFileHeader
{
    char magic[sizeof(DIFF_FILE_MAGIC)];
    uint8_t version;
    uint8_t mode;
    uint8_t ynk;
};

struct DiffHeader
{
    uint32_t crc;
    uint32_t data_len;
    uint8_t arc_num;
};
#pragma pack(pop)

static bool diff_files(const Path& input, const Path& output, const Path& diff_path)
{
    std::vector<std::tuple<uint32_t, std::string, std::string, uint8_t>> diffs;
    libtpdp::Archive arc;
    bool suppress_json_warning = false;
    bool ynk = false;

    Path in_dir(input / L"dat");
    for(int i = 1; i < 7; ++i)
    {
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

        Path dir(output / arc_name);
        for(auto& entry : fs::recursive_directory_iterator(dir))
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

            auto temp = entry.path().wstring();
            temp = temp.substr(dir.wstring().size());

            Path relative_path(temp);

            auto it = arc.find(utf_to_sjis(relative_path.wstring()));
            if(it >= arc.end())
            {
                ScopedConsoleColorChanger color(COLOR_WARN);
                std::cerr << "Skipping file: " << relative_path.string() << std::endl;
                std::cerr << "File not present in archive. Adding files is not supported in mode 1." << std::endl;
                continue;
            }

            auto src_file = arc.get_file(it);
            if(!src_file)
            {
                ScopedConsoleColorChanger color(COLOR_CRITICAL);
                std::cerr << "Error extracting file: " << relative_path.string() << std::endl;
                std::cerr << "From archive: " << arc_path.string() << std::endl;
                return false;
            }

            std::size_t sz;
            auto dst_file = read_file(entry.path().wstring(), sz);
            if(!dst_file)
                throw DiffgenException("Failed to read file: " + entry.path().string());

            boost::crc_32_type src_crc, dst_crc;

            src_crc.process_bytes(src_file.data(), src_file.size());
            dst_crc.process_bytes(dst_file.get(), sz);

            if(src_crc.checksum() == dst_crc.checksum())
                continue;

            std::string diff_output;
            open_vcdiff::VCDiffEncoder encoder(src_file.data(), src_file.size());
            if(!encoder.Encode(dst_file.get(), sz, &diff_output))
            {
                ScopedConsoleColorChanger color(COLOR_CRITICAL);
                std::cerr << "Error generating diff for file: " << relative_path.string() << std::endl;
                std::cerr << "From archive: " << arc_path.string() << std::endl;
                return false;
            }

            diffs.emplace_back((uint32_t)src_crc.checksum(), relative_path.string() + '\0', std::move(diff_output), i);
        }
    }

    if(diffs.empty())
    {
        ScopedConsoleColorChanger color(COLOR_WARN);
        std::cerr << "Source and target are identical, no diffs to output!" << std::endl;
        return false;
    }

    DiffFileHeader header;
    header.mode = 1;
    header.version = DIFF_FILE_VERSION;
    header.ynk = ynk ? 1 : 0;
    memcpy(header.magic, DIFF_FILE_MAGIC, sizeof(DIFF_FILE_MAGIC));

    std::ofstream diff_file(diff_path, std::ios::binary | std::ios::trunc);
    diff_file.write((char*)&header, sizeof(header));

    /* XXX: this is a disaster pls fix */
    for(auto& i : diffs)
    {
        uint32_t crc = (uint32_t)std::get<0>(i);
        std::string& name(std::get<1>(i));
        std::string& data(std::get<2>(i));
        uint8_t arc_num = std::get<3>(i);
        std::size_t total_size = 9 + name.size() + data.size();

        auto buf = std::make_unique<char[]>(total_size);

        write_le32(buf.get(), crc);
        write_le32(buf.get() + 4, (uint32_t)data.size());
        *(buf.get() + 8) = arc_num;
        memcpy(buf.get() + 9, name.data(), name.size());
        memcpy(buf.get() + 9 + name.size(), data.data(), data.size());

        if(!diff_file.write(buf.get(), total_size))
            throw DiffgenException("Failed to write to file: " + diff_path.string());
    }

    return true;
}

static bool diff_archive(const Path& input, const Path& output, const Path& diff_path)
{
    std::unordered_map<int, std::pair<uint32_t, std::string>> diffs;
    libtpdp::Archive arc;
    Path in_dir(input / L"dat");
    bool suppress_json_warning = false;
    bool ynk = false;

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

        try
        {
            std::cout << ">> " << arc_path.string() << std::endl;
            arc.open(arc_path.wstring());
            new_arc.open(arc_path.wstring());
        }
        catch(const libtpdp::ArcError& ex)
        {
            ScopedConsoleColorChanger color(COLOR_CRITICAL);
            std::cerr << "Failed to open file: " << arc_path.string() << std::endl;
            std::cerr << ex.what() << std::endl;
            return false;
        }

        ynk = arc.is_ynk();

        Path dir(output / arc_name);
        for(auto& entry : fs::recursive_directory_iterator(dir))
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

            auto temp = entry.path().wstring();
            temp = temp.substr(dir.wstring().size());

            Path relative_path(temp);

            auto it = arc.find(utf_to_sjis(relative_path.wstring()));
            if(it >= arc.end())
            {
                {
                    ScopedConsoleColorChanger color(COLOR_OK);
                    std::cout << "Inserting new file: " << relative_path.string() << std::endl;
                }

                std::size_t sz;
                auto dst_file = read_file(entry.path().wstring(), sz);
                if(!dst_file)
                    throw DiffgenException("Failed to read file: " + entry.path().string());

                auto new_it = new_arc.insert(dst_file.get(), sz, utf_to_sjis(relative_path.wstring()));
                if(new_it >= new_arc.end())
                    throw DiffgenException("Failed to insert file: " + relative_path.string());

                continue;
            }

            auto src_file = arc.get_file(it);
            if(!src_file)
            {
                ScopedConsoleColorChanger color(COLOR_CRITICAL);
                std::cerr << "Error extracting file: " << relative_path.string() << std::endl;
                std::cerr << "From archive: " << arc_path.string() << std::endl;
                return false;
            }

            std::size_t sz;
            auto dst_file = read_file(entry.path().wstring(), sz);
            if(!dst_file)
                DiffgenException("Failed to read file: " + entry.path().string());

            boost::crc_32_type src_crc, dst_crc;

            src_crc.process_bytes(src_file.data(), src_file.size());
            dst_crc.process_bytes(dst_file.get(), sz);

            if(src_crc.checksum() == dst_crc.checksum())
                continue;

            auto new_it = new_arc.repack_file(utf_to_sjis(relative_path.wstring()), dst_file.get(), sz);
            if(new_it >= new_arc.end())
                throw DiffgenException("Error repacking file: " + relative_path.string());
        }

        boost::crc_32_type src_crc, dst_crc;

        src_crc.process_bytes(arc.data(), arc.size());
        dst_crc.process_bytes(new_arc.data(), new_arc.size());

        if(src_crc.checksum() == dst_crc.checksum())
            continue;

        std::string diff_output;
        open_vcdiff::VCDiffEncoder encoder(arc.data(), arc.size());
        if(!encoder.Encode(new_arc.data(), new_arc.size(), &diff_output))
            throw DiffgenException("Error generating diff for file: " + arc_path.string());

        diffs[i] = { src_crc.checksum(), std::move(diff_output) };
    }

    if(diffs.empty())
    {
        ScopedConsoleColorChanger color(COLOR_WARN);
        std::cerr << "Source and target are identical, no diffs to output!" << std::endl;
        return false;
    }

    DiffFileHeader header;
    header.mode = 2;
    header.version = DIFF_FILE_VERSION;
    header.ynk = ynk ? 1 : 0;
    memcpy(header.magic, DIFF_FILE_MAGIC, sizeof(DIFF_FILE_MAGIC));

    std::ofstream diff_file(diff_path, std::ios::binary | std::ios::trunc);
    diff_file.write((char*)&header, sizeof(header));

    for(auto& it : diffs)
    {
        DiffHeader diff_header;
        diff_header.arc_num = it.first;
        diff_header.crc = it.second.first;
        diff_header.data_len = (uint32_t)it.second.second.size();
        diff_file.write((char*)&diff_header, sizeof(diff_header));
        diff_file.write(it.second.second.data(), it.second.second.size());
    }

    return true;
}

bool extract(const Path& input, const Path& output)
{
    libtpdp::Archive arc;
    Path in_dir(input / L"dat");
    for(int i = 1; i < 7; ++i)
    {
        auto arc_name = (L"gn_dat" + std::to_wstring(i) + L".arc");
        auto arc_path = in_dir / arc_name;

        if(!fs::exists(arc_path) || !fs::is_regular_file(arc_path))
        {
            std::cerr << "File not found: " << arc_path.string() << std::endl;
            continue;
        }

        try
        {
            std::cout << "Extracting: " << arc_path.string() << std::endl;
            arc.open(arc_path.wstring());
        }
        catch(const libtpdp::ArcError& ex)
        {
            ScopedConsoleColorChanger color(COLOR_CRITICAL);
            std::cerr << "Failed to open file: " << arc_path.string() << std::endl;
            std::cerr << ex.what() << std::endl;
            return false;
        }

        Path out_dir(output / arc_name);

        for(auto it = arc.begin(); it != arc.end(); ++it)
        {
            if(arc.is_dir(it) || it->data_size == 0)
                continue;

            auto fp = sjis_to_utf(arc.get_path(it));
            if(fp.empty())
            {
                ScopedConsoleColorChanger color(COLOR_WARN);
                std::cerr << "Skipping unknown object at index: " << it.index() << std::endl;
                continue;
            }

            auto out_path = out_dir / fp;
            auto file = arc.get_file(it);
            if(!file)
            {
                ScopedConsoleColorChanger color(COLOR_CRITICAL);
                std::cerr << "Error extracting file: " << utf_to_sjis(fp) << std::endl;
                std::cerr << "From archive: " << arc_path.string() << std::endl;
                return false;
            }

            try
            {
                fs::create_directories(out_path.parent_path());
            }
            catch(const fs::filesystem_error& ex)
            {
                ScopedConsoleColorChanger color(COLOR_CRITICAL);
                std::cerr << "Failed to create directory: " << out_path.parent_path() << std::endl;
                std::cerr << ex.what() << std::endl;
                return false;
            }

            if(!write_file(out_path.wstring(), file.data(), file.size()))
                throw DiffgenException("Failed to write to file: " + out_path.string());
        }
    }

    return true;
}

bool diff(const Path& input, const Path& output, const Path& diff_path, int diff_mode)
{
    if(diff_mode == 1)
        return diff_files(input, output, diff_path);
    else if(diff_mode == 2)
        return diff_archive(input, output, diff_path);

    std::cerr << "Error: Invalid diff-mode." << std::endl;
    return false;
}

bool patch(const Path& input, const Path& output)
{
    std::unordered_map<int, std::vector<std::tuple<uint32_t, std::string, std::string>>> diffs;
    libtpdp::Archive arc;

    std::size_t sz;
    auto diff_file = read_file(output.wstring(), sz);
    if(!diff_file)
        throw DiffgenException("Failed to read file: " + output.string());

    auto buf = diff_file.get();
    DiffFileHeader *file_header = (DiffFileHeader*)buf;

    if(sz < sizeof(DiffFileHeader) || std::memcmp(&file_header->magic, DIFF_FILE_MAGIC, sizeof(DIFF_FILE_MAGIC)) != 0)
        throw DiffgenException("Unrecognized file format: " + output.string());

    if(file_header->version != DIFF_FILE_VERSION)
    {
        ScopedConsoleColorChanger color(COLOR_CRITICAL);
        std::cerr << "Unsupported file version: " << output.string() << std::endl;
        std::cerr << "Provided diff file is version " << file_header->version << std::endl;
        std::cerr << "Supported version is " << DIFF_FILE_VERSION << std::endl;
        return false;
    }

    int mode = file_header->mode;
    if(mode < 1 || mode > 2)
    {
        ScopedConsoleColorChanger color(COLOR_CRITICAL);
        std::cerr << "Invalid diff mode: " << mode << std::endl;
        std::cerr << "Diff file may be corrupt." << std::endl;
        return false;
    }

    bool ynk = (file_header->ynk != 0);

    std::size_t pos = sizeof(DiffFileHeader);

    if(mode == 2)
    {
        while((sz - pos) >= sizeof(DiffHeader))
        {
            DiffHeader *header = (DiffHeader*)&buf[pos];
            std::string data;
            std::size_t data_len = header->data_len;
            auto crc = header->crc;
            int arc_num = header->arc_num;
            pos += sizeof(DiffHeader);

            if((sz - pos) < data_len)
                throw DiffgenException("File corrupt: " + output.string());

            data.assign(&buf[pos], data_len);
            pos += data_len;

            Path in_dir(input / L"dat");
            auto arc_name = (L"gn_dat" + std::to_wstring(arc_num) + L".arc");
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

            boost::crc_32_type src_crc;
            src_crc.process_bytes(arc.data(), arc.size());
            if(src_crc.checksum() != crc)
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
                std::cerr << "Error decoding diff for file: " << arc_path << std::endl;
                std::cerr << "Diff file may be corrupt, aborting" << std::endl;
                return false;
            }

            arc.close(); // avoid using 3x the file size in memory :(
            libtpdp::Archive new_arc(target.data(), target.size(), ynk);

            if(!new_arc.save(arc_path.wstring()))
                throw DiffgenException("Failed to write to file: " + arc_path.string());
        }

        return true;
    }

    while((sz - pos) >= sizeof(DiffHeader))
    {
        DiffHeader *header = (DiffHeader*)&buf[pos];
        std::string path;
        
        /* make sure the path field is null-terminated */
        for(auto j = pos + sizeof(DiffHeader); j < sz; ++j)
        {
            if(buf[j] == 0)
            {
                path = &buf[pos + sizeof(DiffHeader)];
                pos = j + 1;
                break;
            }
        }

        if(path.empty() || (sz - pos) < header->data_len || header->arc_num > 6 || header->arc_num < 1)
            throw DiffgenException("File corrupt: " + output.string());

        std::string data;
        data.assign(&buf[pos], header->data_len);
        diffs[header->arc_num].emplace_back(header->crc, std::move(path), std::move(data));
        pos += header->data_len;
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
            auto crc = std::get<0>(entry);
            std::string& path(std::get<1>(entry));
            std::string& data(std::get<2>(entry));

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
        }

        if(!arc.save(arc_path.wstring()))
            throw DiffgenException("Failed to write to file: " + arc_path.string());
    }

    return true;
}
