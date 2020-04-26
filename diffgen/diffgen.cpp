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
#include "../common/console.h"
#include "../common/endian.h"
#include <boost/algorithm/string.hpp>
#include <libtpdp.h>
#include <iostream>
#include <tuple>
#include <thread>
#include <future>
#include <regex>
#include <unordered_map>
#include <algorithm>

namespace algo = boost::algorithm;
namespace fs = std::filesystem;

typedef std::pair<Path, Path> DiffPair;
typedef std::tuple<int, Path, Path> DiffTuple;

static std::wostream& operator<<(std::wostream& os, const fs::path& p)
{
    os << p.wstring();
    return os;
}

/* worker thread for diff generation */
static std::vector<DiffPair> worker(Path input, const std::vector<Path> *rel_paths, const libtpdp::Archive *arc, std::size_t begin, std::size_t end)
{
    std::vector<DiffPair> diffs;

    for(auto i = begin; i < end; ++i)
    {
        const Path& relative_path = (*rel_paths)[i];
        Path path = input / relative_path;

        auto it = arc->find(utf_to_sjis(relative_path.wstring()));
        if(it >= arc->end())
        {
            {
                ScopedConsoleColorMT color(COLOR_OK);
                std::wcout << L"Adding new file: " << relative_path << std::endl;
            }
            diffs.push_back({ relative_path, path });
            continue;
        }

        auto src_file = arc->get_file(it);
        if(!src_file)
            throw DiffgenException("Error extracting file: " + utf_narrow(relative_path.wstring()));

        std::size_t sz;
        auto dst_file = read_file(path.wstring(), sz);
        if(!dst_file)
            throw DiffgenException("Failed to read file: " + utf_narrow(path.wstring()));

        if((src_file.size() == sz) && (memcmp(src_file.data(), dst_file.get(), sz) == 0))
            continue;

        diffs.push_back({ relative_path, path });
    }

    return diffs;
}

static std::vector<DiffTuple> get_diffs(const Path& in_dir, const Path& output, int threads)
{
    std::vector<DiffTuple> diffs;
    libtpdp::Archive arc;
    bool suppress_json_warning = false;

    for(int i = 1; i < 7; ++i)
    {
        auto arc_name = (L"gn_dat" + std::to_wstring(i) + L".arc");
        auto arc_path = in_dir / arc_name;
        auto out_dir = output / arc_name;
        std::vector<Path> rel_paths;

        if(!fs::exists(arc_path) || !fs::is_regular_file(arc_path))
        {
            std::wcerr << L"File not found: " << arc_path << std::endl;
            continue;
        }

        if(!fs::exists(out_dir) || !fs::is_directory(out_dir))
        {
            std::wcerr << L"Missing directory: " << out_dir << std::endl;
            continue;
        }

        try
        {
            std::wcout << L">> " << arc_path << std::endl;
            arc.open(arc_path.wstring());
        }
        catch(const libtpdp::ArcError&)
        {
            ScopedConsoleColorChanger color(COLOR_CRITICAL);
            std::wcerr << L"Failed to open file: " << arc_path << std::endl;
            throw;
        }

        for(auto& entry : fs::recursive_directory_iterator(out_dir))
        {
            if(!entry.is_regular_file())
                continue;

            auto rel = entry.path().lexically_relative(out_dir);
            if(rel.empty())
                continue;

            if(algo::iequals(entry.path().extension().wstring(), L".json"))
            {
                if(!suppress_json_warning)
                {
                    ScopedConsoleColorChanger color(COLOR_WARN);
                    std::wcerr << L"Skipping json files..." << std::endl;
                    suppress_json_warning = true;
                }
                continue;
            }

            rel_paths.push_back(std::move(rel));
        }

        std::vector<std::future<std::vector<std::pair<Path, Path>>>> futures;
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
        catch(const std::exception&)
        {
            ScopedConsoleColorMT color(COLOR_CRITICAL);
            std::wcerr << L"Exception processing archive: " << arc_path << std::endl;
            throw;
        }
    }

    return diffs;
}

bool diff(const Path& input, const Path& output, const Path& diff_path, int threads)
{
    if(threads < 1)
        threads = 1;

    if(threads > 1)
        std::wcout << L"Using up to " << threads << L" concurrent threads" << std::endl;

    Path in_dir(input / L"dat");
    auto diffs = get_diffs(in_dir, output, threads);

    if(diffs.empty())
    {
        ScopedConsoleColorChanger color(COLOR_WARN);
        std::wcerr << L"Source and target are identical, no diffs to output!" << std::endl;
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
        std::wcerr << L"Error writing to file: " << diff_path << std::endl;
        std::wcerr << utf_widen(ex.what()) << std::endl;
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
            std::wcerr << L"File not found: " << arc_path << std::endl;
            continue;
        }

        try
        {
            std::wcout << L"Extracting: " << arc_path << std::endl;
            arc.open(arc_path.wstring());
        }
        catch(const libtpdp::ArcError& ex)
        {
            ScopedConsoleColorChanger color(COLOR_CRITICAL);
            std::wcerr << L"Failed to open file: " << arc_path << std::endl;
            std::wcerr << utf_widen(ex.what()) << std::endl;
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
                std::wcerr << L"Skipping unknown object at index: " << it.index() << std::endl;
                continue;
            }

            auto out_path = out_dir / fp;
            auto file = arc.get_file(it);
            if(!file)
            {
                ScopedConsoleColorChanger color(COLOR_CRITICAL);
                std::wcerr << L"Error extracting file: " << fp << std::endl;
                std::wcerr << L"From archive: " << arc_path << std::endl;
                return false;
            }

            try
            {
                fs::create_directories(out_path.parent_path());
            }
            catch(const fs::filesystem_error& ex)
            {
                ScopedConsoleColorChanger color(COLOR_CRITICAL);
                std::wcerr << L"Failed to create directory: " << out_path.parent_path() << std::endl;
                std::wcerr << utf_widen(ex.what()) << std::endl;
                return false;
            }

            if(!write_file(out_path.wstring(), file.data(), file.size()))
            {
                ScopedConsoleColorChanger color(COLOR_WARN);
                std::wcerr << L"Failed to write to file: " << out_path << std::endl;
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
                    patches[num].emplace_back(entry, std::move(relpath));
            }
        }

        if(patches.empty())
            throw DiffgenException("No patches found.");

        std::size_t count = 0;
        for(auto& it : patches)
        {
            auto arcname = L"gn_dat" + std::to_wstring(it.first) + L".arc";
            auto arcpath = in_dir / arcname;
            if(!fs::exists(arcpath) || !fs::is_regular_file(arcpath))
            {
                ScopedConsoleColorChanger color(COLOR_CRITICAL);
                std::wcerr << L"Missing file: " << arcpath << std::endl;
                continue;
            }

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
                std::wcerr << L"Error writing to file: " << arcpath << std::endl;
                std::wcerr << utf_widen(ex.what()) << std::endl;
                return false;
            }
        }

        std::wcout << L"Patched " << count << L" files." << std::endl;
        if(count == 0)
        {
            ScopedConsoleColorChanger color(COLOR_CRITICAL);
            std::wcerr << L"Could not apply any patches." << std::endl;
            return false;
        }
    }
    catch(const ZipError& ex)
    {
        ScopedConsoleColorChanger color(COLOR_CRITICAL);
        std::wcerr << L"Error reading from file: " << output << std::endl;
        std::wcerr << utf_widen(ex.what()) << std::endl;
        return false;
    }

    return true;
}


bool repack(const Path& input, const Path& output, int threads)
{
    std::unordered_map<int, std::vector<DiffPair>> patches;

    if(threads < 1)
        threads = 1;

    if(threads > 1)
        std::wcout << L"Using up to " << threads << L" concurrent threads" << std::endl;

    Path in_dir(input / L"dat");

    {
        auto diffs = get_diffs(in_dir, output, threads);

        if(diffs.empty())
            throw DiffgenException("Source and target are identical!");

        for(auto& i : diffs)
        {
            auto&[arc_num, rel_path, data_path] = i;
            patches[arc_num].push_back({ std::move(rel_path), std::move(data_path) });
        }
    }

    Path arc_path;
    try
    {
        for(const auto& i : patches)
        {
            auto arc_num = i.first;
            arc_path = in_dir / (L"gn_dat" + std::to_wstring(arc_num) + L".arc");

            libtpdp::Archive arc;
            arc.open(arc_path.wstring());

            for(const auto& j : i.second)
            {
                const auto&[rel_path, data_path] = j;

                std::size_t sz;
                auto file = read_file(data_path.wstring(), sz);
                if(!file)
                    throw DiffgenException("Failed to read file: " + utf_narrow(data_path.wstring()));

                auto filename = utf_to_sjis(rel_path.wstring());
                auto it = arc.find(filename);
                if(it >= arc.end())
                {
                    auto pos = arc.insert(file.get(), sz, filename);
                    if(pos >= arc.end())
                        throw DiffgenException("Failed to insert file: " + utf_narrow(data_path.wstring()));
                }
                else
                {
                    auto pos = arc.repack_file(it, file.get(), sz);
                    if(pos >= arc.end())
                        throw DiffgenException("Failed to repack file: " + utf_narrow(data_path.wstring()));
                }
            }

            arc.save(arc_path.wstring());
        }
    }
    catch(const libtpdp::ArcError& ex)
    {
        ScopedConsoleColorChanger color(COLOR_CRITICAL);
        std::wcerr << L"Error writing to file: " << arc_path << std::endl;
        std::wcerr << utf_widen(ex.what()) << std::endl;
        return false;
    }

    return true;
}
