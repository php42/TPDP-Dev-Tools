/*
    Copyright 2019 php42

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

#pragma once
#include <string>

/* conversions between shift-jis and unicode */
std::wstring sjis_to_utf(const std::string& str);
std::wstring sjis_to_utf(const char *str, std::size_t sz);
std::string utf_to_sjis(const std::wstring& str);

/* UTF-8 to wchar_t */
std::wstring utf_widen(const std::string& str);

/* wchar_t to UTF-8 */
std::string utf_narrow(const std::wstring& str);

/* convenience functions */
static inline std::string sjis_to_utf8(const std::string& str) { return utf_narrow(sjis_to_utf(str)); }
static inline std::string sjis_to_utf8(const char *str, std::size_t sz) { return utf_narrow(sjis_to_utf(str, sz)); }
static inline std::string utf8_to_sjis(const std::string& str) { return utf_to_sjis(utf_widen(str)); }
