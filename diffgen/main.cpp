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
#include <boost/program_options.hpp>
#include <iostream>
#include <thread>
#include "../common/console.h"
#include "../common/version.h"
#include "../common/textconvert.h"
#include <chrono>
#include <sstream>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <algorithm>

#ifndef VERSION_STRING
#define VERSION_STRING "Unknown Version"
#endif // !VERSION_STRING

#define EXIT_OK     0
#define EXIT_ERROR  1

static void print_desc(const boost::program_options::options_description& desc)
{
    std::stringstream s;
    desc.print(s);
    std::wcout << utf_widen(s.str()) << std::endl;
}

int wmain(int argc, wchar_t *argv[])
{
    std::wstring input_path, output_path, diff_path;
    int threads;
    bool success = false;

    _setmode(_fileno(stdout), _O_U8TEXT);
    _setmode(_fileno(stderr), _O_U8TEXT);

    std::wcout << L"TPDP Dev-Tools " VERSION_STRING  " DiffGen" << std::endl;

    try
    {
        boost::program_options::variables_map opts;
        boost::program_options::options_description desc("Usage");
        desc.add_options()
            ("help,h", "Display this help text\n")
            ("version,v", "Display version info and exit\n")
            ("input-path,i", boost::program_options::wvalue(&input_path), "Path to the root folder of the input directory (the unmodified game folder)\n")
            ("output-path,o", boost::program_options::wvalue(&output_path), "Path to the root folder of the output directory (the \"romhack\" folder)\n")
            ("extract,e", "Extract data files from the game folder located at input-path to the directory specified by output-path\n")
            ("diff,d", boost::program_options::wvalue(&diff_path)->implicit_value(L"diff.bin", "\"diff.bin\""), "Generate a diff between the original game data located at input-path and the extracted files located at output-path\n")
            ("patch,p", "Patch the original game data located at input-path with the diff file located at output-path\n")
            ("repack,r", "Insert files located at output-path into the original game data located at input-path (used to merge modifications back into the game without needing to create a patch)\n")
            ("threads,j", boost::program_options::wvalue(&threads)->default_value(std::thread::hardware_concurrency()), "Maximum number of concurrent threads to use\n");

        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), opts);
        boost::program_options::notify(opts);

        threads = std::clamp(threads, 2, 64);
        if(threads < 6)
            threads = (int)(threads * 1.5);

        if(opts.empty() || opts.count("help"))
        {
            print_desc(desc);
            return EXIT_OK;
        }

        if(opts.count("version"))
            return EXIT_OK;

        auto num_options = opts.count("extract") + opts.count("diff") + opts.count("patch") + opts.count("repack");

        if(num_options > 1)
        {
            std::wcout << L"Invalid argument: please specify only one operation." << std::endl;
            std::wcout << L"--extract, --diff, --repack, and --patch are mutually exclusive.\n" << std::endl;
            print_desc(desc);
            return EXIT_ERROR;
        }
        else if(num_options == 0)
        {
            std::wcout << L"Invalid argument: please specify an operation.\n" << std::endl;
            print_desc(desc);
            return EXIT_ERROR;
        }

        if(input_path.empty())
        {
            std::wcout << L"Invalid argument: please specify input path." << std::endl;
            print_desc(desc);
            return EXIT_ERROR;
        }

        if(output_path.empty())
        {
            std::wcout << L"Invalid argument: please specify output path." << std::endl;
            print_desc(desc);
            return EXIT_ERROR;
        }

        if(opts.count("diff") && diff_path.empty())
        {
            std::wcout << L"Invalid argument: please specify diff path." << std::endl;
            print_desc(desc);
            return EXIT_ERROR;
        }

        // check for jank installs
        for(int i = 1; i < 7; ++i)
        {
            auto arc_name = (L"gn_dat" + std::to_wstring(i));
            auto arc_path = Path(input_path) / L"dat" / (arc_name + L".arc");
            auto folder_path = Path(input_path) / L"dat" / arc_name;

            if(std::filesystem::exists(arc_path) && std::filesystem::is_regular_file(arc_path)) // this is what we expect
                continue;

            if(std::filesystem::exists(folder_path) && std::filesystem::is_directory(folder_path)) // if this exists and the above doesn't, it's borked
            {
                std::wcerr << L"You seem to have an old pirated copy of the game with broken dat files." << std::endl;
                std::wcerr << L"gn_dat1-6.arc must exist in in your dat folder (and NOT as folders)." << std::endl;
                std::wcerr << L"Please get a better copy of the game." << std::endl;
                std::wcerr << L"Operation aborted." << std::endl;
                return EXIT_ERROR;
            }
        }

        auto begin = std::chrono::high_resolution_clock::now();
        if(opts.count("extract"))
            success = extract(input_path, output_path, threads);
        else if(opts.count("diff"))
            success = diff(input_path, output_path, diff_path, threads);
        else if(opts.count("patch"))
            success = patch(input_path, output_path);
        else if(opts.count("repack"))
            success = repack(input_path, output_path, threads);

        if(success)
        {
            auto end = std::chrono::high_resolution_clock::now();
            auto seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end - begin);
            std::wcout << L"Done." << std::endl;
            std::wcout << L"Finished in " << seconds.count() << L" seconds." << std::endl;
        }
        else
        {
            ScopedConsoleColorChanger color(COLOR_CRITICAL);
            std::wcerr << L"Operation aborted." << std::endl;
        }
    }
    catch(const std::exception& ex)
    {
        ScopedConsoleColorChanger color(COLOR_CRITICAL);
        std::wcerr << L"Error: " << utf_widen(ex.what()) << std::endl;
        return EXIT_ERROR;
    }
    catch(...)
    {
        ScopedConsoleColorChanger color(COLOR_CRITICAL);
        std::wcerr << L"An unknown error occurred" << std::endl;
        return EXIT_ERROR;
    }

    return success ? EXIT_OK : EXIT_ERROR;
}
