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

#include "puppet.h"
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cassert>
#include "../common/textconvert.h"
#include "../common/endian.h"

static const wchar_t *g_mark_names[] = {
    L"None",
    L"Red",
    L"Blue",
    L"Black",
    L"White",
    L"Green"
};

static const wchar_t *g_costume_names[] = {
    L"Normal",
    L"Alt Color",
    L"Alt Outfit",
    L"Wedding Dress"
};

static constexpr auto g_num_marks = sizeof(g_mark_names) / sizeof(g_mark_names[0]);
static constexpr auto g_num_costumes = sizeof(g_costume_names) / sizeof(g_costume_names[0]);

namespace libtpdp
{

static_assert(g_num_marks == MARK_MAX);
static_assert(g_num_costumes == COSTUME_MAX);

Puppet::Puppet()
{
	memset(this, 0, sizeof(Puppet));

	/* apparent default values for empty party puppets */
	happiness = 1;
	level = 1;
	hp = 0x0B;
}

void Puppet::read(const void *data, bool party)
{
	const uint8_t *buf = (const uint8_t*)data;

	trainer_id = read_le32(buf);
	secret_id = read_le32(&buf[4]);
	memcpy(trainer_name_raw_, &buf[8], sizeof(trainer_name_raw_));
    //trainer_name_raw_[31] = 0; // ensure null-terminated
	catch_location = read_le16(&buf[0x28]);
	caught_year = buf[0x2a];
	caught_month = buf[0x2b];
	caught_day = buf[0x2c];
	caught_hour = buf[0x2d];
	caught_minute = buf[0x2e];
	memcpy(puppet_nickname_raw_, &buf[0x2f], sizeof(puppet_nickname_raw_));
    //puppet_nickname_raw_[31] = 0; // ensure null-terminated
	puppet_id = read_le16(&buf[0x4f]);
	style_index = buf[0x51];
	ability_index = buf[0x52];
	mark = buf[0x53];
	for(int i = 0; i < 6; ++i)
		ivs[i] = (buf[0x54 + (i / 2)] >> ((i % 2) * 4)) & 0x0f;
	unknown_0x57 = buf[0x57];
	exp = read_le32(&buf[0x58]);
	happiness = read_le16(&buf[0x5c]);
	pp = read_le16(&buf[0x5e]);
	costume_index = buf[0x60];
	memcpy(evs, &buf[0x61], sizeof(evs));
	held_item_id = read_le16(&buf[0x67]);
	for(int i = 0; i < 4; ++i)
		skills[i] = read_le16(&buf[0x69 + (i * 2)]);
	memcpy(unknown_0x71, &buf[0x71], sizeof(unknown_0x71));

	if(party)
	{
		level = buf[0x95];
		hp = read_le16(&buf[0x96]);
		memcpy(sp, &buf[0x98], sizeof(sp));
		memcpy(status_effects, &buf[0x9c], sizeof(status_effects));
		unknown_0x9e = buf[0x9e];
	}
	else
	{
		level = 0;
		hp = 0;
		memset(sp, 0, sizeof(sp));
		memset(status_effects, 0, sizeof(status_effects));
		unknown_0x9e = 0;
	}
}

void Puppet::write(void *data, bool party) const
{
	uint8_t *buf = (uint8_t*)data;

	write_le32(buf, trainer_id);
	write_le32(&buf[4], secret_id);
	memcpy(&buf[8], trainer_name_raw_, sizeof(trainer_name_raw_));
	write_le16(&buf[0x28], catch_location);
	buf[0x2a] = caught_year;
	buf[0x2b] = caught_month;
	buf[0x2c] = caught_day;
	buf[0x2d] = caught_hour;
	buf[0x2e] = caught_minute;
	memcpy(&buf[0x2f], puppet_nickname_raw_, sizeof(puppet_nickname_raw_));
	write_le16(&buf[0x4f], puppet_id);
	buf[0x51] = style_index;
	buf[0x52] = ability_index;
	buf[0x53] = mark;
	memset(&buf[0x54], 0, 3);
	for(int i = 0; i < 6; ++i)
		buf[0x54 + (i / 2)] |= (ivs[i] << ((i % 2) * 4));
	buf[0x57] = unknown_0x57;
	write_le32(&buf[0x58], exp);
	write_le16(&buf[0x5c], happiness);
	write_le16(&buf[0x5e], pp);
	buf[0x60] = costume_index;
	memcpy(&buf[0x61], evs, sizeof(evs));
	write_le16(&buf[0x67], held_item_id);
	for(int i = 0; i < 4; ++i)
		write_le16(&buf[0x69 + (i * 2)], skills[i]);
	memcpy(&buf[0x71], unknown_0x71, sizeof(unknown_0x71));

	if(party)
	{
		buf[0x95] = level;
		write_le16(&buf[0x96], hp);
		memcpy(&buf[0x98], sp, sizeof(sp));
		memcpy(&buf[0x9c], status_effects, sizeof(status_effects));
		buf[0x9e] = unknown_0x9e;
	}
}

std::wstring Puppet::trainer_name() const
{
    if(memchr(trainer_name_raw_, 0, 32) != NULL) // check for null-terminator
        return sjis_to_utf(trainer_name_raw_);
    else
        return {};
}

void Puppet::set_trainer_name(const std::wstring& name)
{
	std::string str = utf_to_sjis(name);
	snprintf(trainer_name_raw_, 32, "%s", str.c_str());
}

std::wstring Puppet::puppet_nickname() const
{
    if(memchr(puppet_nickname_raw_, 0, 32) != NULL) // check for null-terminator
        return sjis_to_utf(puppet_nickname_raw_);
    else
        return {};
}

void Puppet::set_puppet_nickname(const std::wstring& name)
{
	std::string str = utf_to_sjis(name);
	snprintf(puppet_nickname_raw_, 32, "%s", str.c_str());
}

void Puppet::set_puppet_nickname(const std::string& name)
{
    snprintf(puppet_nickname_raw_, 32, "%s", name.c_str());
}

std::wstring puppet_mark_string(unsigned int mark)
{
    assert(mark < MARK_MAX);

    if(mark < g_num_marks)
        return g_mark_names[mark];
    else
        return L"Unknown";
}

std::wstring puppet_costume_string(unsigned int index)
{
    assert(index < COSTUME_MAX);

    if(index < g_num_costumes)
        return g_costume_names[index];
    else
        return L"Unknown";
}

}
