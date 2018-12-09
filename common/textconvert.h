/*
	Copyright (C) 2016 php42

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once
#include <string>

/* conversions between shift-jis and unicode */
std::wstring sjis_to_utf(const std::string& str);
std::wstring sjis_to_utf(const char *begin, const char *end);
std::string utf_to_sjis(const std::wstring& str);

/* UTF-8 to wchar_t */
std::wstring utf_widen(const std::string& str);

/* wchar_t to UTF-8 */
std::string utf_narrow(const std::wstring& str);

/* convenience functions */
static inline std::string sjis_to_utf8(const std::string& str) { return utf_narrow(sjis_to_utf(str)); }
static inline std::string sjis_to_utf8(const char *begin, const char *end) { return utf_narrow(sjis_to_utf(begin, end)); }
static inline std::string utf8_to_sjis(const std::string& str) { return utf_to_sjis(utf_widen(str)); }
