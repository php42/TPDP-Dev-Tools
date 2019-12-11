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

#include "gamedata.h"
#include "../common/endian.h"
#include "../common/textconvert.h"
#include"../common/filesystem.h"
#include <cassert>
#include <utility>
#include <stdexcept>
#include <algorithm>

static const unsigned int g_style_levels[] = {0, 0, 0, 0, 30, 36, 42, 49, 56, 63, 70};
static const unsigned int g_base_levels[] = {7, 10, 14, 19, 24};

static const int g_cost_exp_modifiers[] = { 70, 85, 100, 115, 130 };
static const int g_cost_exp_modifiers_ynk[] = { 85, 92, 100, 107, 115 };

static const wchar_t *g_style_names[] = {
    L"None",
    L"Normal",
    L"Power",
    L"Defense",
    L"Assist",
    L"Speed",
    L"Extra",
};

static const wchar_t *g_element_names[] = {
    L"None",
    L"Void",
    L"Fire",
    L"Water",
    L"Nature",
    L"Earth",
    L"Steel",
    L"Wind",
    L"Electric",
    L"Light",
    L"Dark",
    L"Nether",
    L"Poison",
    L"Fighting",
    L"Illusion",
    L"Sound",
    L"Dream",
    L"Warped",
};

static const wchar_t *g_skill_type_names[] = {
    L"Focus",
    L"Spread",
    L"Status",
};

static constexpr auto g_num_styles = sizeof(g_style_names) / sizeof(g_style_names[0]);
static constexpr auto g_num_elements = sizeof(g_element_names) / sizeof(g_element_names[0]);
static constexpr auto g_num_skill_types = sizeof(g_skill_type_names) / sizeof(g_skill_type_names[0]);

namespace libtpdp
{

static_assert(ELEMENT_MAX == g_num_elements);
static_assert(STYLE_MAX == g_num_styles);
static_assert(SKILL_TYPE_MAX == g_num_skill_types);

void SkillData::read(const void *data)
{
	const uint8_t *buf = (const uint8_t*)data;

	element = buf[32];
	power = buf[33];
	accuracy = buf[34];
	sp = buf[35];
	priority = buf[36];
	type = buf[37];
    zero = buf[38];
	effect_id = read_le16(&buf[39]);
	effect_chance = buf[41];
    effect_type = buf[42];
	effect_target = buf[43];
    ynk_id = read_le16(&buf[44]);
}

void SkillData::write(void *data) const
{
    uint8_t *buf = (uint8_t*)data;

    buf[32] = element;
    buf[33] = power;
    buf[34] = accuracy;
    buf[35] = sp;
    buf[36] = priority;
    buf[37] = type;
    buf[38] = zero;
    write_le16(&buf[39], effect_id);
    buf[41] = effect_chance;
    buf[42] = effect_type;
    buf[43] = effect_target;
    write_le16(&buf[44], ynk_id);
}

StyleData::StyleData() : style_type(0), element1(0), element2(0), lv100_skill(0)
{
	memset(base_stats, 0, sizeof(base_stats));
    memset(abilities, 0, sizeof(abilities));
    memset(style_skills, 0, sizeof(style_skills));
    memset(skill_compat_table, 0, sizeof(skill_compat_table));
    memset(lv70_skills, 0, sizeof(lv70_skills));
}

void StyleData::read(const void *data)
{
	const uint8_t *buf = (const uint8_t*)data;

	style_type = buf[0];
	element1 = buf[1];
	element2 = buf[2];

	for(int i = 0; i < 6; ++i)
		base_stats[i] = buf[3 + i];

	abilities[0] = read_le16(&buf[9]);
	abilities[1] = read_le16(&buf[11]);

	for(int i = 0; i < 11; ++i)
		style_skills[i] = read_le16(&buf[17 + (i * 2)]);

    lv100_skill = read_le16(&buf[0x2D]);

	for(int i = 0; i < 16; ++i)
		skill_compat_table[i] = buf[49 + i];

	for(int i = 0; i < 8; ++i)
		lv70_skills[i] = read_le16(&buf[65 + (i * 2)]);
}

void StyleData::write(void * data) const
{
    uint8_t *buf = (uint8_t*)data;

    buf[0] = style_type;
    buf[1] = element1;
    buf[2] = element2;

    for(int i = 0; i < 6; ++i)
        buf[3 + i] = base_stats[i];

    write_le16(&buf[9], abilities[0]);
    write_le16(&buf[11], abilities[1]);

    for(int i = 0; i < 11; ++i)
        write_le16(&buf[17 + (i * 2)], style_skills[i]);

    write_le16(&buf[0x2D], lv100_skill);

    for(int i = 0; i < 16; ++i)
        buf[49 + i] = skill_compat_table[i];

    for(int i = 0; i < 8; ++i)
        write_le16(&buf[65 + (i * 2)], lv70_skills[i]);
}

std::wstring StyleData::style_string() const
{
    return libtpdp::style_string(style_type);
}

std::wstring style_string(unsigned int style)
{
    assert(style < STYLE_MAX);

    if(style < g_num_styles)
        return g_style_names[style];
    else
        return L"Unknown";
}

PuppetData::PuppetData()
{
	cost = 0;
	memset(base_skills, 0, sizeof(base_skills));
	memset(item_drop_table, 0, sizeof(item_drop_table));
	id = 0;
}

void PuppetData::read(const void *data)
{
	const uint8_t *buf = (const uint8_t*)data;

	cost = buf[32];

	for(int i = 0; i < 5; ++i)
		base_skills[i] = read_le16(&buf[33 + (i * 2)]);

	for(int i = 0; i < 4; ++i)
		item_drop_table[i] = read_le16(&buf[43 + (i * 2)]);

    /* there's an ID here, but it's wrong (?) */
	id = read_le16(&buf[51]);

	for(int i = 0; i < 4; ++i)
		styles[i].read(&buf[93 + (i * STYLE_DATA_SIZE)]);
}

void PuppetData::write(void *data) const
{
    uint8_t *buf = (uint8_t*)data;

    buf[32] = cost;

    for(int i = 0; i < 5; ++i)
        write_le16(&buf[33 + (i * 2)], base_skills[i]);

    for(int i = 0; i < 4; ++i)
        write_le16(&buf[43 + (i * 2)], item_drop_table[i]);

    //write_le16(&buf[51], id);

    for(int i = 0; i < 4; ++i)
        styles[i].write(&buf[93 + (i * STYLE_DATA_SIZE)]);
}

int PuppetData::level_to_learn(unsigned int style_index, unsigned int skill_id) const
{
    if(skill_id == 0)
        return 0;
    if(style_index > 3)
        return -1;
    if(styles[style_index].style_type == 0)
        return -1;

    for(int i = 0; i < 5; ++i)
    {
        if(skill_id == base_skills[i])
            return g_base_levels[i];
    }

    for(int i = 0; i < 11; ++i)
    {
        if(skill_id == styles[style_index].style_skills[i])
            return g_style_levels[i];
    }

    for(auto i : styles[style_index].lv70_skills)
    {
        if(skill_id == i)
            return 70;
    }

    if(skill_id == styles[style_index].lv100_skill)
        return 100;

    if(style_index > 0)
        return level_to_learn(0, skill_id);

    return -1;
}

int PuppetData::max_style_index() const
{
	int result = 0;
	for(int i = 0; i < 4; ++i)
	{
		if(styles[i].style_type)
			result = i;
		else
			break;
	}

	return result;
}

bool ItemData::parse(const CSVEntry& data, bool ynk)
{
    if((!ynk) && (data.size() != 11))
        return false;
    else if((ynk) && (data.size() != 12))
        return false;

    if(data[0].empty() || !iswdigit(data[0][0]))
        return false;

    try
    {
        id = std::stoi(data[0]);
        name = data[1];
        price = std::stoi(data[2]);
        type = std::stoi(data[3]);	/* 255 = unimplemented (id 0 "nothing" also uses this value) */
        combat = (std::stoi(data[4]) != 0);
        common = (std::stoi(data[5]) != 0);
        can_discard = (std::stoi(data[6]) != 0);
        held = (std::stoi(data[7]) != 0);
        reincarnation = (std::stoi(data[8]) != 0);
        skill_id = std::stoi(data[9]);
    }
    catch(const std::exception&)
    {
        return false;
    }

    if(!ynk)
        description = data[10];
    else
        description = data[11];

    return true;
}

bool CSVFile::parse(const void *data, std::size_t len)
{
    clear();

    std::wstring file = sjis_to_utf((const char*)data, len);

    int num_splits = 0;

    /* determine field count from first line. */
    for(auto& i : file)
    {
        if(i == L',')
            ++num_splits;
        else if(i == L'\n')
            break;
    }

    std::size_t str_begin = 0, str_end = file.find(L"\r\n");
    while(str_end != std::wstring::npos)
    {
        CSVEntry entry;
        std::size_t split_begin = str_begin, split_end = file.find(L',', str_begin);
        for(int i = 0; i < num_splits; ++i)
        {
            if(split_end > str_end)
            {
                clear();
                return false;
            }

            entry.push_back(file.substr(split_begin, split_end - split_begin));
            split_begin = split_end + 1;
            split_end = file.find(L',', split_begin);
        }

        entry.push_back(file.substr(split_begin, str_end - split_begin));
        value_map_.push_back(std::move(entry));
        str_begin = str_end + 2;
        str_end = file.find(L"\r\n", str_begin);
    }

    return true;
}

std::string CSVFile::to_string() const
{
    std::wstring temp;

    for(const auto& i : value_map_)
    {
        if(i.empty())
            continue;

        for(std::size_t j = 0; j < i.size() - 1; ++j)
        {
            temp += i[j];
            temp += L',';
        }
        temp += i.back();
        temp += L"\r\n";
    }

    return utf_to_sjis(temp);
}

std::wstring element_string(unsigned int element)
{
    assert(element < ELEMENT_MAX);

    if(element < g_num_elements)
        return g_element_names[element];
    else
        return L"Unknown";
}

void MADData::read(const void *data)
{
    const uint8_t *buf = (const uint8_t*)data;

    memcpy(tilesets, buf, sizeof(tilesets));
    weather = buf[8];
    overworld_theme = buf[9];
    battle_background = buf[10];
    forbid_bike = buf[11];
    encounter_type = buf[12];
    unknown = buf[13];

    /* area name */
    memcpy(location_name, &buf[0x59], sizeof(location_name));

    /* normal grass */
    memcpy(puppet_ids, &buf[0x0E], sizeof(puppet_ids));
    memcpy(puppet_levels, &buf[0x22], sizeof(puppet_levels));
    memcpy(puppet_styles, &buf[0x2c], sizeof(puppet_styles));
    memcpy(puppet_ratios, &buf[0x36], sizeof(puppet_ratios));

    /* blue grass */
    memcpy(special_puppet_ids, &buf[0x40], sizeof(special_puppet_ids));
    memcpy(special_puppet_levels, &buf[0x4a], sizeof(special_puppet_levels));
    memcpy(special_puppet_styles, &buf[0x4f], sizeof(special_puppet_styles));
    memcpy(special_puppet_ratios, &buf[0x54], sizeof(special_puppet_ratios));
}

void MADData::write(void *data) const
{
    uint8_t *buf = (uint8_t*)data;

    memcpy(buf, tilesets, sizeof(tilesets));
    buf[8] = weather;
    buf[9] = overworld_theme;
    buf[10] = battle_background;
    buf[11] = forbid_bike;
    buf[12] = encounter_type;
    buf[13] = unknown;

    /* area name */
    memcpy(&buf[0x59], location_name, sizeof(location_name));

    /* normal grass */
    memcpy(&buf[0x0E], puppet_ids, sizeof(puppet_ids));
    memcpy(&buf[0x22], puppet_levels, sizeof(puppet_levels));
    memcpy(&buf[0x2c], puppet_styles, sizeof(puppet_styles));
    memcpy(&buf[0x36], puppet_ratios, sizeof(puppet_ratios));

    /* blue grass */
    memcpy(&buf[0x40], special_puppet_ids, sizeof(special_puppet_ids));
    memcpy(&buf[0x4a], special_puppet_levels, sizeof(special_puppet_levels));
    memcpy(&buf[0x4f], special_puppet_styles, sizeof(special_puppet_styles));
    memcpy(&buf[0x54], special_puppet_ratios, sizeof(special_puppet_ratios));
}

void MADData::clear_encounters()
{
	memset(puppet_ids, 0, sizeof(puppet_ids));
	memset(puppet_levels, 0, sizeof(puppet_levels));
	memset(puppet_ratios, 0, sizeof(puppet_ratios));
	memset(puppet_styles, 0, sizeof(puppet_styles));
	memset(special_puppet_ids, 0, sizeof(special_puppet_ids));
	memset(special_puppet_levels, 0, sizeof(special_puppet_levels));
	memset(special_puppet_ratios, 0, sizeof(special_puppet_ratios));
	memset(special_puppet_styles, 0, sizeof(special_puppet_styles));
}

void MADEncounter::read(const MADData& data, int i, bool special)
{
	assert(i < 10);
	assert((i < 5) || !special);
	index = i;

	if(special)
	{
		id = data.special_puppet_ids[i];
		level = data.special_puppet_levels[i];
		style = data.special_puppet_styles[i];
		weight = data.special_puppet_ratios[i];
	}
	else
	{
		id = data.puppet_ids[i];
		level = data.puppet_levels[i];
		style = data.puppet_styles[i];
		weight = data.puppet_ratios[i];
	}
}

void MADEncounter::write(MADData& data, int i, bool special) const
{
	assert(i < 10);
	assert((i < 5) || !special);

	if(special)
	{
		data.special_puppet_ids[i] = id;
		data.special_puppet_levels[i] = level;
		data.special_puppet_styles[i] = style;
		data.special_puppet_ratios[i] = weight;
	}
	else
	{
		data.puppet_ids[i] = id;
		data.puppet_levels[i] = level;
		data.puppet_styles[i] = style;
		data.puppet_ratios[i] = weight;
	}
}

void ChipData::read(const void *data)
{
    auto chip = (const uint8_t*)data;

    for(std::size_t i = 0; i < 256; ++i)
    {
        index_map_[i] = chip[i * 26];
    }
}

void FMFData::read(std::wstring path)
{
    data_ = read_file(path, sz_);

    if(!data_ || (sz_ < 20))
        throw std::runtime_error("Failed to read FMF file.");

    payload_len = read_le32(&data_[4]);
    map_width = read_le32(&data_[8]);
    map_height = read_le32(&data_[12]);
    num_layers = (uint8_t)data_[0x12];
    unk1 = data_[0x10];
    unk2 = data_[0x11];
    unk3 = data_[0x13];

    auto stride = map_width * map_height * 2;

    if((stride * num_layers) != payload_len)
        throw std::runtime_error("Corrupt FMF file.");

    for(std::size_t i = 0; i < num_layers; ++i)
    {
        auto offset = (stride * i) + 20;
        layers_.push_back((uint8_t*)&data_[offset]);
    }
}

void FMFData::write(std::wstring path)
{
    write_le32(&data_[4], (uint32_t)payload_len);
    write_le32(&data_[8], (uint32_t)map_width);
    write_le32(&data_[12], (uint32_t)map_height);
    data_[0x12] = (uint8_t)num_layers;
    data_[0x10] = unk1;
    data_[0x11] = unk2;
    data_[0x13] = unk3;

    if(!write_file(path, data_.get(), sz_))
        throw std::runtime_error("Failed to write FMF file.");
}

void FMFData::resize(std::size_t width, std::size_t height)
{
    const auto new_payload_len = width * height * num_layers * 2;
    const auto new_sz = payload_len + 20;

    const auto layer_sz = map_width * map_height * 2;
    const auto new_layer_sz = width * height * 2;
    const auto stride = map_width * 2;
    const auto new_stride = width * 2;

    auto new_buf = std::make_unique<char[]>(new_sz);
    memset(new_buf.get(), 0, new_sz);

    for(std::size_t i = 0; i < num_layers; ++i)
    {
        auto layer = get_layer(i);
        auto new_layer = &new_buf[(i * new_layer_sz) + 20];

        for(std::size_t j = 0; j < std::min(map_height, height); ++j)
        {
            auto row = &layer[j * stride];
            auto new_row = &new_layer[j * new_stride];
            memcpy(new_row, row, std::min(stride, new_stride));
        }
    }

    payload_len = new_payload_len;
    sz_ = new_sz;
    map_width = width;
    map_height = height;

    write_le32(&new_buf[4], (uint32_t)payload_len);
    write_le32(&new_buf[8], (uint32_t)map_width);
    write_le32(&new_buf[12], (uint32_t)map_height);
    new_buf[0x12] = (uint8_t)num_layers;
    new_buf[0x10] = unk1;
    new_buf[0x11] = unk2;
    new_buf[0x13] = unk3;

    data_ = std::move(new_buf);

    layers_.clear();
    for(std::size_t i = 0; i < num_layers; ++i)
    {
        auto offset = (new_layer_sz * i) + 20;
        layers_.push_back((uint8_t*)&data_[offset]);
    }
}

void OBSData::read(std::wstring path)
{
    data_ = read_file(path, sz_);
    if(!data_ || (sz_ < 20480))
        throw std::runtime_error("Failed to read OBS file.");
}

void OBSData::write(std::wstring path) const
{
    if(!write_file(path, data_.get(), sz_))
        throw std::runtime_error("Failed to write OBS file.");
}

void OBSEntry::read(const void *data)
{
    const uint8_t *buf = (const uint8_t*)data;
    object_id = read_le16(buf);
    type = buf[2];
    unknown = buf[3];
    event_arg = read_le16(&buf[4]);
    event_index = read_le16(&buf[6]);
    memcpy(flags, &buf[8], sizeof(flags));
}

void OBSEntry::write(void *data) const
{
    uint8_t *buf = (uint8_t*)data;
    write_le16(buf, object_id);
    buf[2] = type;
    buf[3] = unknown;
    write_le16(&buf[4], event_arg);
    write_le16(&buf[6], event_index);
    memcpy(&buf[8], flags, sizeof(flags));
}

std::wstring skill_type_string(unsigned int type)
{
    assert(type < SKILL_TYPE_MAX);

    if(type < g_num_skill_types)
        return g_skill_type_names[type];
    else
        return L"Unknown";
}

unsigned int level_from_exp(unsigned int cost, unsigned int exp, bool ynk)
{
    assert(cost < 5);
    int ret = 1;
    while(exp_for_level(cost, ret + 1, ynk) <= exp)
        ++ret;

    if(ret > 100)
        ret = 100;
    return ret;
}

unsigned int exp_for_level(unsigned int cost, unsigned int level, bool ynk)
{
    assert(cost < 5);
    if(level <= 1)
        return 0;

    const int *mods = (ynk) ? g_cost_exp_modifiers_ynk : g_cost_exp_modifiers;

    unsigned int ret = level * level * level * (unsigned int)mods[cost] / 100u;

    return ret;
}

}
