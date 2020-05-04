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

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define CP_SJIS 932
#include <Windows.h>
#include "textconvert.h"
#include <memory>

std::wstring sjis_to_utf(const std::string& str)
{
    std::wstring ret;

    if(str.empty())
        return ret;

    auto len = MultiByteToWideChar(CP_SJIS, MB_PRECOMPOSED, str.c_str(), (int)str.size(), NULL, 0);
    if(!len)
        return ret;

    std::unique_ptr<wchar_t[]> buf(new wchar_t[len]);
    if(!MultiByteToWideChar(CP_SJIS, MB_PRECOMPOSED, str.c_str(), (int)str.size(), buf.get(), len))
        return ret;

    ret.assign(buf.get(), len);

    return ret;
}

std::wstring sjis_to_utf(const char *str, std::size_t sz)
{
    std::wstring ret;

    auto len = MultiByteToWideChar(CP_SJIS, MB_PRECOMPOSED, str, (int)sz, NULL, 0);
    if(!len)
        return ret;

    std::unique_ptr<wchar_t[]> buf(new wchar_t[len]);
    if(!MultiByteToWideChar(CP_SJIS, MB_PRECOMPOSED, str, (int)sz, buf.get(), len))
        return ret;

    ret.assign(buf.get(), len);

    return ret;
}

std::string utf_to_sjis(const std::wstring& str)
{
    std::string ret;

    if(str.empty())
        return ret;

    auto len = WideCharToMultiByte(CP_SJIS, 0, str.c_str(), (int)str.size(), NULL, 0, NULL, NULL);
    if(!len)
        return ret;

    std::unique_ptr<char[]> buf(new char[len]);
    if(!WideCharToMultiByte(CP_SJIS, 0, str.c_str(), (int)str.size(), buf.get(), len, NULL, NULL))
        return ret;

    ret.assign(buf.get(), len);

    return ret;
}

std::wstring utf_widen(const std::string& str)
{
    std::wstring ret;

    if(str.empty())
        return ret;

    auto len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
    if(!len)
        return ret;

    std::unique_ptr<wchar_t[]> buf(new wchar_t[len]);
    if(!MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), buf.get(), len))
        return ret;

    ret.assign(buf.get(), len);

    return ret;
}

std::string utf_narrow(const std::wstring& str)
{
    std::string ret;

    if(str.empty())
        return ret;

    auto len = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0, NULL, NULL);
    if(!len)
        return ret;

    std::unique_ptr<char[]> buf(new char[len]);
    if(!WideCharToMultiByte(CP_UTF8, 0, str.c_str(), (int)str.size(), buf.get(), len, NULL, NULL))
        return ret;

    ret.assign(buf.get(), len);

    return ret;
}
