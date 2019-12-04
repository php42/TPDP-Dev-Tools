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

#include "binedit.h"
#include <boost/program_options.hpp>
#include <iostream>
#include "../common/console.h"
#include "../common/version.h"
#include <chrono>

#ifndef VERSION_STRING
#define VERSION_STRING "Unknown Version"
#endif // !VERSION_STRING


int wmain(int argc, wchar_t *argv[])
{
    std::wstring input_path;
    bool success = false;

    std::cout << "TPDP Dev-Tools " VERSION_STRING  " BinEdit" << std::endl;

    try
    {
        boost::program_options::variables_map opts;
        boost::program_options::options_description desc("Usage");
        desc.add_options()
            ("help,h", "Display this help text\n")
            ("version,v", "Display version info and exit\n")
            ("input-path,i", boost::program_options::wvalue(&input_path), "path to the root folder of the extracted game files (output of diffgen.exe --extract)\n")
            ("convert,c", "recusively convert data files located at input-path to json.\nnote that input-path must point to files extracted with diffgen --extract, not the original game folder.\nconverted files are saved alongside the original (e.g. DollData.dbs -> DollData.json in the same folder)")
            ("patch,p", "recusively patch the data files located at input-path with their json counterparts in the same folder");

        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), opts);
        boost::program_options::notify(opts);

        if(opts.empty() || opts.count("help"))
        {
            desc.print(std::cout);
            return EXIT_SUCCESS;
        }

        if(opts.count("version"))
            return EXIT_SUCCESS;

        auto num_options = opts.count("convert") + opts.count("patch");

        if(num_options > 1)
        {
            std::cout << "Invalid argument: please specify only one operation." << std::endl;
            std::cout << "--convert and --patch are mutually exclusive.\n" << std::endl;
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

        auto begin = std::chrono::high_resolution_clock::now();
        if(opts.count("convert"))
            success = convert(input_path);
        else if(opts.count("patch"))
            success = patch(input_path);

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
        std::cerr << "Error: " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch(...)
    {
        ScopedConsoleColorChanger color(COLOR_CRITICAL);
        std::cerr << "An unknown error occurred" << std::endl;
        return EXIT_FAILURE;
    }

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
