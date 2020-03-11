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

#include "diffgen.h"
#include "ziparchive.h"
#include "legacy.h"
#include "../common/filesystem.h"
#include "../common/textconvert.h"
#include "../common/endian.h"
#include <boost/algorithm/string.hpp>
#include <libtpdp.h>
#include <iostream>
#include <tuple>
#include <thread>
#include <future>
#include <regex>
#include <unordered_map>

namespace algo = boost::algorithm;
namespace fs = std::filesystem;

std::recursive_mutex ScopedConsoleLock::mtx_;

/* worker thread for diff generation */
static std::vector<std::pair<Path,Path>> worker(Path input, const std::vector<Path> *rel_paths, const libtpdp::Archive *arc, std::size_t begin, std::size_t end)
{
    std::vector<std::pair<Path,Path>> diffs;

    for(auto i = begin; i < end; ++i)
    {
        const Path& relative_path = (*rel_paths)[i];
        Path path = input / relative_path;

        auto it = arc->find(utf_to_sjis(relative_path.wstring()));
        if(it >= arc->end())
        {
            {
                ScopedConsoleColorMT color(COLOR_OK);
                std::cout << "Adding new file: " << relative_path.string() << std::endl;
            }
            diffs.push_back({ relative_path, path });
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

        diffs.push_back({ relative_path, path });
    }

    return diffs;
}

bool diff(const Path& input, const Path& output, const Path& diff_path, int threads)
{
    std::vector<std::tuple<int,Path,Path>> diffs;
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

            auto rel = entry.path().lexically_relative(out_dir);
            if(rel.empty())
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

            rel_paths.push_back(std::move(rel));
        }

        std::vector<std::future<std::vector<std::pair<Path,Path>>>> futures;
        auto arc_threads = threads;
        auto files_per_thread = rel_paths.size() / arc_threads;
        if(files_per_thread < 2)
            arc_threads = 1;

        std::size_t begin = 0;
        for(int j = 0; j < arc_threads; ++j)
        {
            auto end = (j == (arc_threads - 1)) ? rel_paths.size() : (begin + files_per_thread);
            futures.emplace_back(std::async(std::launch::async, worker, out_dir, &rel_paths, &arc, begin, end));
            begin = end;
        }

        for(auto& j : futures)
            if(j.valid())
                j.wait();

        try
        {
            for(auto& j : futures)
            {
                auto val = j.get();
                for(auto& k : val)
                    diffs.emplace_back(i, std::move(k.first), std::move(k.second));
            }
        }
        catch(const std::exception& ex)
        {
            ScopedConsoleColorMT color(COLOR_CRITICAL);
            std::cerr << arc_path.string() << ": Exception" << std::endl;
            std::cerr << ex.what() << std::endl;

            return false;
        }
    }

    if(diffs.empty())
    {
        ScopedConsoleColorChanger color(COLOR_WARN);
        std::cerr << "Source and target are identical, no diffs to output!" << std::endl;
        return false;
    }

    ZipArchive zip;

    try
    {
        zip.open(diff_path, ZIP_CREATE | ZIP_TRUNCATE);

        for(auto& i : diffs)
        {
            auto &[arc_num, rel_path, data_path] = i;
            auto arc_name = "gn_dat" + std::to_string(arc_num) + ".arc";
            auto path = utf_narrow((arc_name / rel_path).wstring());
            [[maybe_unused]] auto entry = zip.add_file(path, data_path);
        }

        zip.close();
    }
    catch(const std::exception& ex)
    {
        ScopedConsoleColorMT color(COLOR_CRITICAL);
        std::cerr << "Error writing to file: " << diff_path.string() << std::endl;
        std::cerr << ex.what() << std::endl;
        return false;
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
            {
                ScopedConsoleColorChanger color(COLOR_WARN);
                std::cerr << "Failed to write to file: " << out_path.string() << std::endl;
            }
        }
    }

    return true;
}

bool patch(const Path& input, const Path& output)
{
    ZipArchive zip;
    std::unordered_map<int, std::vector<std::pair<ZipEntry,std::string>>> patches;
    Path in_dir = input / L"dat";

    try
    {
        try
        {
            zip.open(output);
        }
        catch(const ZipWrongFmt&)
        {
            return legacy_patch(input, output);
        }

        auto files = zip.get_file_table();

        std::regex regex("^gn_dat(\\d)\\.arc(?:\\\\|/)+(.+)", std::regex::icase);
        for(auto& entry : files)
        {
            std::smatch matches;
            if(std::regex_search(entry.name, matches, regex))
            {
                auto num = std::stol(matches.str(1));
                auto relpath = matches.str(2);
                if(num >= 1 && num <= 6)
                    patches[num].push_back({ entry, relpath });
            }
        }

        if(patches.empty())
            throw DiffgenException("No patches found.");

        std::size_t count = 0;
        for(auto& it : patches)
        {
            auto arcname = L"gn_dat" + std::to_wstring(it.first) + L".arc";
            auto arcpath = in_dir / arcname;

            try
            {
                libtpdp::Archive arc;
                arc.open(arcpath.wstring());

                for(auto& i : it.second)
                {
                    auto file = zip.get_file(i.first);
                    auto pos = arc.find(utf8_to_sjis(i.second));
                    if(pos < arc.end())
                    {
                        pos = arc.repack_file(pos, file.data(), file.size());
                        if(pos >= arc.end())
                            throw DiffgenException("Failed to repack file: " + i.second);
                    }
                    else
                    {
                        pos = arc.insert(file.data(), file.size(), utf8_to_sjis(i.second));
                        if(pos >= arc.end())
                            throw DiffgenException("Failed to insert file: " + i.second);
                    }

                    ++count;
                }

                arc.save(arcpath.wstring());
            }
            catch(const libtpdp::ArcError& ex)
            {
                ScopedConsoleColorChanger color(COLOR_CRITICAL);
                std::cerr << "Error writing to file: " << arcpath.string() << std::endl;
                std::cerr << ex.what() << std::endl;
                return false;
            }
        }

        std::cout << "Patched " << count << " files." << std::endl;
    }
    catch(const ZipError& ex)
    {
        ScopedConsoleColorChanger color(COLOR_CRITICAL);
        std::cerr << "Error reading from file: " << output.string() << std::endl;
        std::cerr << ex.what() << std::endl;
        return false;
    }

    return true;
}


bool repack(const Path& input, const Path& output)
{
    libtpdp::Archive arc;
    bool suppress_json_warning = false;

    Path in_dir(input / L"dat");
    for(int i = 1; i < 7; ++i)
    {
        auto arc_name = (L"gn_dat" + std::to_wstring(i) + L".arc");
        auto arc_path = in_dir / arc_name;
        auto out_dir = output / arc_name;
        bool dirty = false;

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

                auto new_it = arc.insert(dst_file.get(), sz, utf_to_sjis(relative_path.wstring()));
                if(new_it >= arc.end())
                    throw DiffgenException("Failed to insert file: " + relative_path.string());

                dirty = true;
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

            if((src_file.size() != sz) || (std::memcmp(src_file.data(), dst_file.get(), sz) != 0))
            {
                auto new_it = arc.repack_file(it, dst_file.get(), sz);
                if(new_it >= arc.end())
                    throw DiffgenException("Failed to repack file: " + relative_path.string());
                dirty = true;
            }
        }

        if(dirty)
        {
            if(!arc.save(arc_path.wstring()))
            {
                ScopedConsoleColorChanger color(COLOR_CRITICAL);
                std::cerr << "Error writing to archive: " << arc_path.string() << std::endl;
                return false;
            }
        }
    }

    return true;
}
