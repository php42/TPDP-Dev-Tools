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
#include <chrono>

#ifndef VERSION_STRING
#define VERSION_STRING "Unknown Version"
#endif // !VERSION_STRING


int wmain(int argc, wchar_t *argv[])
{
    std::wstring input_path, output_path, diff_path;
    int diff_mode, threads;
    bool success = false;

    std::cout << "TPDP Dev-Tools " VERSION_STRING  " DiffGen"<< std::endl;

    try
    {
        boost::program_options::variables_map opts;
        boost::program_options::options_description desc("Usage");
        desc.add_options()
            ("help,h", "Display this help text\n")
            ("version,v", "Display version info and exit\n")
            ("input-path,i", boost::program_options::wvalue(&input_path), "path to the root folder of the input directory (the unmodified game folder)\n")
            ("output-path,o", boost::program_options::wvalue(&output_path), "path to the root folder of the output directory (the \"romhack\" folder)\n")
            ("extract,e", "extract data files from the game folder located at input-path to the directory specified by output-path\n")
            ("diff,d", boost::program_options::wvalue(&diff_path)->implicit_value(L"diff.bin", "\"diff.bin\""), "Generate a diff between the original game data located at input-path and the extracted files located at output-path\n")
            ("patch,p", "Patch the original game data located at input-path with the diff file located at output-path\n")
            ("repack,r", "Insert files located at output-path into the original game data located at input-path (used to merge modifications back into the game without needing to generate a diff)\n")
            ("diff-mode,m", boost::program_options::wvalue(&diff_mode)->default_value(1), "mode to use when generating a diff\n1: default, diff applied on per-file basis\n2: diff applied on the whole archive\nmode 2 is required for adding files to the archive\n")
            ("threads,j", boost::program_options::wvalue(&threads)->default_value(std::thread::hardware_concurrency()), "maximum number of concurrent threads to use for compression\n");

        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), opts);
        boost::program_options::notify(opts);

        if(opts.empty() || opts.count("help"))
        {
            desc.print(std::cout);
            return EXIT_SUCCESS;
        }

        if(opts.count("version"))
            return EXIT_SUCCESS;

        auto num_options = opts.count("extract") + opts.count("diff") + opts.count("patch") + opts.count("repack");

        if(num_options > 1)
        {
            std::cout << "Invalid argument: please specify only one operation." << std::endl;
            std::cout << "--extract, --diff, --repack, and --patch are mutually exclusive.\n" << std::endl;
            desc.print(std::cout);
            return EXIT_FAILURE;
        }
        else if(num_options == 0)
        {
            std::cout << "Invalid argument: please specify an operation.\n" << std::endl;
            desc.print(std::cout);
            return EXIT_FAILURE;
        }

        if(input_path.empty())
        {
            std::cout << "Invalid argument: please specify input path." << std::endl;
            desc.print(std::cout);
            return EXIT_FAILURE;
        }

        if(output_path.empty())
        {
            std::cout << "Invalid argument: please specify output path." << std::endl;
            desc.print(std::cout);
            return EXIT_FAILURE;
        }

        if(opts.count("diff") && diff_path.empty())
        {
            std::cout << "Invalid argument: please specify diff path." << std::endl;
            desc.print(std::cout);
            return EXIT_FAILURE;
        }

        if(diff_mode > 2 || diff_mode < 1)
        {
            std::cout << "Invalid argument: diff-mode must be either 1 or 2." << std::endl;
            desc.print(std::cout);
            return EXIT_FAILURE;
        }

        auto begin = std::chrono::high_resolution_clock::now();
        if(opts.count("extract"))
            success = extract(input_path, output_path);
        else if(opts.count("diff"))
            success = diff(input_path, output_path, diff_path, diff_mode, threads);
        else if(opts.count("patch"))
            success = patch(input_path, output_path);
        else if(opts.count("repack"))
            success = repack(input_path, output_path);

        if(success)
        {
            auto end = std::chrono::high_resolution_clock::now();
            auto seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end - begin);
            std::cout << "Done." << std::endl;
            std::cout << "Finished in " << seconds.count() << " seconds." << std::endl;
        }
        else
        {
            ScopedConsoleColorChanger color(COLOR_CRITICAL);
            std::cerr << "Operation aborted." << std::endl;
        }
    }
    catch(const std::exception& ex)
    {
        ScopedConsoleColorChanger color(COLOR_CRITICAL);
        std::cout << "Error: " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch(...)
    {
        ScopedConsoleColorChanger color(COLOR_CRITICAL);
        std::cout << "An unknown error occurred" << std::endl;
        return EXIT_FAILURE;
    }

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
