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

/* workaround for a bug in boost::property_tree (bad assert for json arrays) */
#ifndef NDEBUG
#define NDEBUG
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#undef NDEBUG
#else
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#endif

#include <boost/algorithm/string.hpp>
#include <boost/beast/core/detail/base64.hpp>
#include "../common/textconvert.h"
#include "../common/filesystem.h"
#include "../common/console.h"
#include "../common/endian.h"
#include "../common/version.h"
#include "../common/thread_pool.h"
#include <libtpdp.h>
#include "binedit.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <chrono>
#include <functional>
#include <utility>
#include <intrin.h>
#include <atomic>
#include <algorithm>

namespace algo = boost::algorithm;
namespace fs = std::filesystem;
namespace b64 = boost::beast::detail::base64;

constexpr unsigned int JSON_MAJOR = 1;
constexpr unsigned int JSON_MINOR = 3;

std::atomic_uint g_count_mad(0);
std::atomic_uint g_count_dod(0);
std::atomic_uint g_count_chp(0);
std::atomic_uint g_count_fmf(0);
std::atomic_uint g_count_obs(0);

static std::wostream& operator<<(std::wostream& os, const fs::path& p)
{
    os << p.wstring();
    return os;
}

static std::string base64_encode(const void *src, const std::size_t len)
{
    std::string ret;
    ret.resize(b64::encoded_size(len));
    auto ret_len = b64::encode(ret.data(), src, len);
    ret.resize(ret_len);

    return ret;
}

static std::size_t base64_decode(const std::string& str, void *dst, const std::size_t len)
{
    if(str.size() > b64::encoded_size(len))
        throw BineditException("Insufficient base64 buffer size.");

    auto ret = b64::decode(dst, str.c_str(), str.size());

    return ret.first;
}

static void save_as_utf8(const Path& out, boost::property_tree::ptree& tree, bool pretty_print = true)
{
    try
    {
        std::ofstream stream(out, std::ios::binary | std::ios::trunc);
        boost::property_tree::write_json(stream, tree, pretty_print);
    }
    catch(const boost::property_tree::json_parser_error& ex)
    {
        ScopedConsoleColorMT color(COLOR_WARN);
        std::wcerr << L"Error writing to file: " << out << std::endl;
        std::wcerr << utf_widen(ex.what()) << std::endl;
    }
    catch(const std::exception& ex)
    {
        ScopedConsoleColorMT color(COLOR_WARN);
        std::wcerr << L"Error writing to file: " << out << std::endl;
        std::wcerr << utf_widen(ex.what()) << std::endl;
    }
}

static void read_as_utf8(const Path& in, boost::property_tree::ptree& tree)
{
    try
    {
        std::ifstream stream(in, std::ios::binary);
        boost::property_tree::read_json(stream, tree);
    }
    catch(const boost::property_tree::json_parser_error& ex)
    {
        throw BineditException("Error reading file: " + in.string() + "\n" + ex.what());
    }
    catch(const std::exception& ex)
    {
        throw BineditException("Error reading file: " + in.string() + "\n" + ex.what());
    }
}

static unsigned int element_to_uint(const std::string& name)
{
    auto tmp = utf_widen(name); // converting text form to integer representation
    for(unsigned int i = libtpdp::ELEMENT_NONE; i < libtpdp::ELEMENT_MAX; ++i)
        if(algo::iequals(tmp, libtpdp::element_string(i)))
            return i;

    return (unsigned int)-1;
}

static unsigned int style_to_uint(const std::string& name)
{
    auto tmp = utf_widen(name); // converting text form to integer representation
    for(unsigned int i = libtpdp::STYLE_NONE; i < libtpdp::STYLE_MAX; ++i)
        if(algo::iequals(tmp, libtpdp::style_string(i)))
            return i;

    return (unsigned int)-1;
}

static unsigned int skill_type_to_uint(const std::string& name)
{
    auto tmp = utf_widen(name); // converting text form to integer representation
    for(unsigned int i = libtpdp::SKILL_TYPE_FOCUS; i < libtpdp::SKILL_TYPE_MAX; ++i)
        if(algo::iequals(tmp, libtpdp::skill_type_string(i)))
            return i;

    return (unsigned int)-1;
}

static unsigned int mark_to_uint(const std::string& name)
{
    auto tmp = utf_widen(name); // converting text form to integer representation
    for(unsigned int i = libtpdp::MARK_NONE; i < libtpdp::MARK_MAX; ++i)
        if(algo::iequals(tmp, libtpdp::puppet_mark_string(i)))
            return i;

    return (unsigned int)-1;
}

/* convert the DollData.dbs file containing the definitions
 * of all the puppets to json */
static void convert_nerds(const Path& in, const Path& out)
{
    std::size_t sz;
    auto file = read_file(in.wstring(), sz);

    if(file == nullptr)
        throw BineditException("failed to open file: " + in.string());

    {
        ScopedConsoleLock lock;
        std::wcout << in << L" >> " << out << std::endl;
    }

    boost::property_tree::ptree tree;

    for(std::size_t pos = 0; (sz - pos) >= libtpdp::PUPPET_DATA_SIZE; pos += libtpdp::PUPPET_DATA_SIZE)
    {
        libtpdp::PuppetData data(&file[pos]);
        data.id = (uint16_t)(pos / libtpdp::PUPPET_DATA_SIZE);

        if((data.styles[0].style_type | data.styles[1].style_type | data.styles[2].style_type | data.styles[3].style_type) == 0) // not a real puppet
            continue;

        boost::property_tree::ptree puppet;
        puppet.put("id", data.id);
        puppet.put("cost", data.cost);
        puppet.put("puppetdex_index", data.puppetdex_index);
        for(auto i : data.base_skills)
            puppet.add("base_skills.", i);
        for(auto i : data.item_drop_table)
            puppet.add("item_drop_table.", i);

        for(auto& style_data : data.styles)
        {
            boost::property_tree::ptree style;
            style.put("type", utf_narrow(style_data.style_string()));
            style.put("element1", utf_narrow(libtpdp::element_string(style_data.element1)));
            style.put("element2", utf_narrow(libtpdp::element_string(style_data.element2)));
            style.put("lvl100_skill", style_data.lv100_skill);
            for(auto i : style_data.abilities)
                style.add("abilities.", i);
            for(auto i : style_data.base_stats)
                style.add("base_stats.", i);
            for(auto i : style_data.style_skills)
                style.add("style_skills.", i);
            for(auto i : style_data.lv70_skills)
                style.add("lvl70_skills.", i);

            style.add_child("compatibility", {});
            for(int i = 0; i < 16; ++i)
            {
                for(int j = 0; j < 8; ++j)
                {
                    if(style_data.skill_compat_table[i] & (1 << j))
                        style.add("compatibility.", 385 + (8 * i) + j);
                }
            }

            puppet.add_child("styles.", style);
        }

        tree.add_child("puppets.", puppet);
    }

    save_as_utf8(out, tree);
}

/* apply json patch file to DollData.dbs */
static void patch_nerds(const Path& data, const Path& json)
{
    std::size_t sz;
    auto file = read_file(data.wstring(), sz);
    boost::property_tree::ptree tree;
    read_as_utf8(json, tree);

    if((file == nullptr) || sz < libtpdp::PUPPET_DATA_SIZE)
        throw BineditException("failed to open file: " + data.string());

    {
        ScopedConsoleLock lock;
        std::wcout << json << L" >> " << data << std::endl;
    }

    for(auto& it : tree.get_child("puppets")) // iterate puppets
    {
        auto& node = it.second;
        auto id = node.get<unsigned int>("id");

        // puppet id
        auto offset = (id * libtpdp::PUPPET_DATA_SIZE);
        if(id >= (sz / libtpdp::PUPPET_DATA_SIZE))
            throw BineditException("Puppet ID too large: " + std::to_string(id));

        libtpdp::PuppetData puppet(&file[offset]);
        puppet.id = (uint16_t)id;

        // puppet cost
        auto cost = node.get<uint8_t>("cost");
        if(cost > 4)
            throw BineditException("Invalid cost value: " + std::to_string(cost) + "\nAcceptable values are 0-4.");

        puppet.cost = cost;

        // puppetdex index
        puppet.puppetdex_index = node.get<uint16_t>("puppetdex_index");

        auto index = 0;

        // puppet base skills
        if(node.get_child("base_skills").size() != 5)
            throw BineditException("puppet " + std::to_string(id) + " must have 5 base skills");
        for(auto& c : node.get_child("base_skills"))
            puppet.base_skills[index++] = c.second.get_value<uint16_t>();
        index = 0;

        // item drops
        if(node.get_child("item_drop_table").size() != 4)
            throw BineditException("puppet " + std::to_string(id) + " must have 4 drop items");
        for(auto& c : node.get_child("item_drop_table"))
            puppet.item_drop_table[index++] = c.second.get_value<uint16_t>();

        // styles
        if(node.get_child("styles").size() != 4)
            throw BineditException("puppet " + std::to_string(id) + " must have 4 styles");
        auto style_index = 0;
        for(auto& s : node.get_child("styles"))
        {
            auto& style = s.second;
            auto& style_data = puppet.styles[style_index++];
            index = 0;

            // style type
            auto name = style.get<std::string>("type");
            style_data.style_type = (uint8_t)style_to_uint(name);
            if(style_data.style_type == -1)
                throw BineditException("Invalid style type: " + name + " for puppet: " + std::to_string(id));

            // element 1
            name = style.get<std::string>("element1");
            style_data.element1 = (uint8_t)element_to_uint(name);
            if(style_data.element1 == -1)
                throw BineditException("Invalid element: " + name + " for puppet: " + std::to_string(id));

            // element 2
            name = style.get<std::string>("element2");
            style_data.element2 = (uint8_t)element_to_uint(name);
            if(style_data.element2 == -1)
                throw BineditException("Invalid element: " + name + " for puppet: " + std::to_string(id));

            // level 100 skill
            style_data.lv100_skill = style.get<uint16_t>("lvl100_skill");

            // stats
            if(style.get_child("base_stats").size() != 6)
                throw BineditException("Puppet " + std::to_string(id) + " style " + std::to_string(style_index) + ": must have 6 base stats");
            for(auto& c : style.get_child("base_stats"))
                style_data.base_stats[index++] = c.second.get_value<uint8_t>();
            index = 0;

            // abilities
            if(style.get_child("abilities").size() != 2)
                throw BineditException("Puppet " + std::to_string(id) + " style " + std::to_string(style_index) + ": must have 2 abilities");
            for(auto& c : style.get_child("abilities"))
                style_data.abilities[index++] = c.second.get_value<uint16_t>();
            index = 0;

            // style skills
            if(style.get_child("style_skills").size() != 11)
                throw BineditException("Puppet " + std::to_string(id) + " style " + std::to_string(style_index) + ": must have 11 style skills");
            for(auto& c : style.get_child("style_skills"))
                style_data.style_skills[index++] = c.second.get_value<uint16_t>();
            index = 0;

            // level 70 skills
            if(style.get_child("lvl70_skills").size() != 8)
                throw BineditException("Puppet " + std::to_string(id) + " style " + std::to_string(style_index) + ": must have 8 level 70 skills");
            for(auto& c : style.get_child("lvl70_skills"))
                style_data.lv70_skills[index++] = c.second.get_value<uint16_t>();

            // skillcards
            if(style.get_child("compatibility").size() > 128)
                throw BineditException("Puppet " + std::to_string(id) + ": Too many skillcards!");
            std::vector<unsigned int> skillcards;
            skillcards.reserve(128);
            memset(style_data.skill_compat_table, 0, sizeof(style_data.skill_compat_table));
            for(auto& c : style.get_child("compatibility"))
                skillcards.push_back(c.second.get_value<unsigned int>());
            for(auto i : skillcards)
            {
                if(i > 512 || i < 385)
                    throw BineditException("Puppet " + std::to_string(id) + " style " + std::to_string(style_index) + ": invalid compatibility value: " + std::to_string(i) + "\nAcceptable values are 385-512");

                auto j = i - 385;
                auto k = j % 8;
                j /= 8;

                style_data.skill_compat_table[j] |= 1 << k;
            }
        }

        puppet.write(&file[offset]);
    }

    if(!write_file(data.wstring(), file.get(), sz))
        throw BineditException("Failed to write to file: " + data.string());
}

static void convert_mad(const Path& in, const Path& out)
{
    std::size_t sz;
    auto file = read_file(in.wstring(), sz);
    if(!file || (sz < 121))
        throw BineditException("Error reading file: " + in.string());

    libtpdp::MADData data(file.get());
    boost::property_tree::ptree tree;

    if(memchr(data.location_name, 0, 32) == NULL)
        throw BineditException("Missing null-terminator at offset: 0x1F");

    tree.put("location_name", sjis_to_utf8(std::string(data.location_name))); // needs to be UTF-8 or the parser complains when reading it back

    for(auto i : data.tilesets)
        tree.add("tilesets.", i);

    tree.put("weather", data.weather);
    tree.put("overworld_theme", data.overworld_theme);
    tree.put("battle_background", data.battle_background);
    tree.put("forbid_bike", data.forbid_bike);
    tree.put("encounter_type", data.encounter_type);
    tree.put("unknown", data.unknown);

    if(data.weather > 9)
    {
        ScopedConsoleColorMT color(COLOR_WARN);
        std::wcerr << L"Warning: " << in << L"\nUnknown weather value: " << (unsigned int)data.weather << std::endl;
    }
    if(data.encounter_type > 1)
    {
        ScopedConsoleColorMT color(COLOR_WARN);
        std::wcerr << L"Warning: " << in << L"\nUnknown encounter_type value: " << (unsigned int)data.encounter_type << std::endl;
    }
    if(data.forbid_bike > 1)
    {
        ScopedConsoleColorMT color(COLOR_WARN);
        std::wcerr << L"Warning: " << in << L"\nUnknown forbid_bike value: " << (unsigned int)data.forbid_bike << std::endl;
    }

    for(auto i = 0; i < 10; ++i)
    {
        if(i < 5)
        {
            libtpdp::MADEncounter special_encounter(data, i, true);
            boost::property_tree::ptree special_node;
            special_node.put("id", special_encounter.id);
            special_node.put("level", special_encounter.level);
            special_node.put("style", special_encounter.style);
            special_node.put("weight", special_encounter.weight);

            tree.add_child("special_encounters.", special_node);
        }

        libtpdp::MADEncounter encounter(data, i, false);
        boost::property_tree::ptree node;
        node.put("id", encounter.id);
        node.put("level", encounter.level);
        node.put("style", encounter.style);
        node.put("weight", encounter.weight);

        tree.add_child("normal_encounters.", node);
    }

    save_as_utf8(out, tree);
}

static void patch_mad(const Path& data, const Path& json)
{
    boost::property_tree::ptree tree;
    read_as_utf8(json, tree);

    std::size_t sz;
    auto file = read_file(data.wstring(), sz);
    if(!file || (sz < 121))
        throw BineditException("Error reading file: " + data.string());

    libtpdp::MADData mad(file.get());
    std::string location_name = utf8_to_sjis(tree.get<std::string>("location_name"));
    if(location_name.size() >= 32)
        throw BineditException("Location name too long! must be less than 32 bytes");
    //memset(mad.location_name, 0, sizeof(mad.location_name));
    memcpy(mad.location_name, location_name.data(), location_name.size());
    mad.location_name[location_name.size()] = 0;
    mad.clear_encounters();

    mad.weather = tree.get<uint8_t>("weather");
    mad.overworld_theme = tree.get<uint8_t>("overworld_theme");
    mad.battle_background = tree.get<uint8_t>("battle_background");
    mad.forbid_bike = tree.get<uint8_t>("forbid_bike");
    mad.encounter_type = tree.get<uint8_t>("encounter_type");
    mad.unknown = tree.get<uint8_t>("unknown");

    if(tree.get_child("special_encounters").size() > 5)
        throw BineditException("Too many special encounters! Max is 5");
    if(tree.get_child("normal_encounters").size() > 10)
        throw BineditException("Too many normal encounters! Max is 10");

    int index = 0;
    for(auto& it : tree.get_child("special_encounters"))
    {
        libtpdp::MADEncounter encounter;
        encounter.index = index;
        encounter.id = it.second.get<uint16_t>("id");
        encounter.level = it.second.get<uint8_t>("level");
        encounter.style = it.second.get<uint8_t>("style");
        encounter.weight = it.second.get<uint8_t>("weight");

        if(encounter.level > 100)
        {
            ScopedConsoleColorMT color(COLOR_WARN);
            std::wcerr << L"Warning: " << json << L"\nPuppet level greater than 100!" << std::endl;
        }
        if(encounter.style > 3)
            throw BineditException("Puppet style greater than 3!");

        encounter.write(mad, index++, true);
    }

    index = 0;
    for(auto& it : tree.get_child("normal_encounters"))
    {
        libtpdp::MADEncounter encounter;
        encounter.index = index;
        encounter.id = it.second.get<uint16_t>("id");
        encounter.level = it.second.get<uint8_t>("level");
        encounter.style = it.second.get<uint8_t>("style");
        encounter.weight = it.second.get<uint8_t>("weight");

        if(encounter.level > 100)
        {
            ScopedConsoleColorMT color(COLOR_WARN);
            std::wcerr << L"Warning: " << json << L"\nPuppet level greater than 100!" << std::endl;
        }
        if(encounter.style > 3)
            throw BineditException("Puppet style greater than 3!");

        encounter.write(mad, index++, false);
    }

    if(tree.get_child("tilesets").size() != 4)
        throw BineditException("Must have 4 tilesets!");

    index = 0;
    for(auto& it : tree.get_child("tilesets"))
        mad.tilesets[index++] = it.second.get_value<uint16_t>();

    mad.write(file.get());

    if(!write_file(data.wstring(), file.get(), sz))
        throw BineditException("Error writing to file: " + data.string());
}

static void convert_dod(const Path& in, const Path& out, const void *rand_data)
{
    std::size_t sz;
    auto file = read_file(in.wstring(), sz);
    if(!file || (sz < 0x42A))
        throw BineditException("Error reading file: " + in.string());

    boost::property_tree::ptree tree;

    if(memchr(file.get(), 0, 32) == NULL)
        throw BineditException("Missing null-terminator at offset: 0x1F");
    if(memchr(&file[0x3AA], 0, 128) == NULL)
        throw BineditException("Missing null-terminator at offset: 0x429");

    std::string trainer_name(file.get());
    std::string trainer_title(&file[0x3AA]);

    tree.put("trainer_name", sjis_to_utf8(trainer_name));
    tree.put("trainer_title", sjis_to_utf8(trainer_title));
    tree.put("portrait_id", read_le16(&file[0x20]));
    tree.put("start_bgm", (uint8_t)file[0x22]);
    tree.put("battle_bgm", (uint8_t)file[0x23]);
    tree.put("victory_bgm", (uint8_t)file[0x24]);

    tree.put("intro_text_id", read_le16(&file[0x25]));
    tree.put("end_text_id", read_le16(&file[0x27]));

    tree.put("defeat_text_id", read_le16(&file[0x29]));
    tree.put("ai_difficulty", (uint8_t)file[0x2B]);

    unsigned char *buf = (unsigned char*)&file[0x2C];
    for(auto i = 0; i < 6; ++i)
    {
        boost::property_tree::ptree node;

        // Hack to detect blank puppets
        __m128i zero = _mm_setzero_si128();
        for(unsigned int j = 0; j < libtpdp::PUPPET_SIZE_BOX;)
        {
            if((libtpdp::PUPPET_SIZE_BOX - j) >= 16)
            {
                auto temp = _mm_loadu_si128((__m128i*)&buf[(i * libtpdp::PUPPET_SIZE_BOX) + j]);
                if((uint16_t)_mm_movemask_epi8(_mm_cmpeq_epi8(zero, temp)) != 0xffff)
                {
                    libtpdp::decrypt_puppet(&buf[i * libtpdp::PUPPET_SIZE_BOX], rand_data);
                    break;
                }
                j += 16;
            }
            else
            {
                if(buf[(i * libtpdp::PUPPET_SIZE_BOX) + j] != 0)
                {
                    libtpdp::decrypt_puppet(&buf[i * libtpdp::PUPPET_SIZE_BOX], rand_data);
                    break;
                }
                ++j;
            }
        }
        libtpdp::Puppet puppet(&buf[i * libtpdp::PUPPET_SIZE_BOX], false);

        node.put("nickname", utf_narrow(puppet.puppet_nickname()));
        node.put("id", puppet.puppet_id);
        node.put("style", puppet.style_index);
        node.put("ability", puppet.ability_index);
        node.put("costume", puppet.costume_index);
        node.put("experience", puppet.exp);
        node.put("mark", utf_narrow(libtpdp::puppet_mark_string(puppet.mark)));
        node.put("held_item", puppet.held_item_id);
        node.put("heart_mark", puppet.has_heart_mark());

        for(auto j : puppet.skills)
            node.add("skills.", j);

        for(auto j : puppet.ivs)
            node.add("ivs.", j);

        for(auto j : puppet.evs)
            node.add("evs.", j);

        tree.add_child("puppets.", node);
    }

    save_as_utf8(out, tree);
}

static void patch_dod(const Path& data, const Path& json, const void *rand_data)
{
    boost::property_tree::ptree tree;
    read_as_utf8(json, tree);

    std::size_t sz;
    auto file = read_file(data.wstring(), sz);
    if(!file || (sz != 1066))
        throw BineditException("Error reading file: " + data.string());

    auto trainer_name = utf8_to_sjis(tree.get<std::string>("trainer_name"));
    auto trainer_title = utf8_to_sjis(tree.get<std::string>("trainer_title"));
    auto portrait_id = tree.get<uint16_t>("portrait_id");
    auto start_bgm = tree.get<uint8_t>("start_bgm");
    auto battle_bgm = tree.get<uint8_t>("battle_bgm");
    auto victory_bgm = tree.get<uint8_t>("victory_bgm");
    write_le16(&file[0x20], portrait_id);
    file[0x22] = start_bgm;
    file[0x23] = battle_bgm;
    file[0x24] = victory_bgm;

    auto intro_text_id = tree.get<uint16_t>("intro_text_id");
    auto end_text_id = tree.get<uint16_t>("end_text_id");
    write_le16(&file[0x25], intro_text_id);
    write_le16(&file[0x27], end_text_id);

    auto defeat_text_id = tree.get<uint16_t>("defeat_text_id");
    auto ai_difficulty = tree.get<uint8_t>("ai_difficulty");
    write_le16(&file[0x29], defeat_text_id);
    file[0x2B] = ai_difficulty;

    if(trainer_name.size() >= 32)
        throw BineditException("Trainer name must be less than 32 bytes!");
    if(trainer_title.size() >= 128)
        throw BineditException("Trainer title must be less than 128 bytes!");

    //memset(file.get(), 0, 32);
    //memset(&file[0x3AA], 0, 128);
    memcpy(file.get(), trainer_name.data(), trainer_name.size());
    memcpy(&file[0x3AA], trainer_title.data(), trainer_title.size());
    file[0x3AA + trainer_title.size()] = 0;
    file[trainer_name.size()] = 0;

    std::size_t pos = 0;
    auto buf = &file[0x2C];
    for(auto& it : tree.get_child("puppets"))
    {
        auto& node = it.second;
        if(pos >= 6)
            throw BineditException("Too many puppets!");

        // Hack to detect blank puppets
        __m128i zero = _mm_setzero_si128();
        for(unsigned int i = 0; i < libtpdp::PUPPET_SIZE_BOX;)
        {
            if((libtpdp::PUPPET_SIZE_BOX - i) >= 16)
            {
                auto temp = _mm_loadu_si128((__m128i*)&buf[(pos * libtpdp::PUPPET_SIZE_BOX) + i]);
                if((uint16_t)_mm_movemask_epi8(_mm_cmpeq_epi8(zero, temp)) != 0xffff)
                {
                    libtpdp::decrypt_puppet(&buf[pos * libtpdp::PUPPET_SIZE_BOX], rand_data);
                    break;
                }
                i += 16;
            }
            else
            {
                if(buf[(pos * libtpdp::PUPPET_SIZE_BOX) + i] != 0)
                {
                    libtpdp::decrypt_puppet(&buf[pos * libtpdp::PUPPET_SIZE_BOX], rand_data);
                    break;
                }
                ++i;
            }
        }
        libtpdp::Puppet puppet(&buf[pos * libtpdp::PUPPET_SIZE_BOX], false);

        puppet.set_puppet_nickname(utf_widen(node.get<std::string>("nickname")));
        puppet.puppet_id = node.get<uint16_t>("id");
        puppet.style_index = node.get<uint8_t>("style");
        puppet.ability_index = node.get<uint8_t>("ability");
        puppet.costume_index = node.get<uint8_t>("costume");
        puppet.exp = node.get<uint32_t>("experience");
        puppet.mark = (uint8_t)mark_to_uint(node.get<std::string>("mark"));
        puppet.held_item_id = node.get<uint16_t>("held_item");
        puppet.set_heart_mark(node.get<bool>("heart_mark"));

        if(puppet.mark == -1)
            throw BineditException("Invalid puppet mark: " + node.get<std::string>("mark"));

        if(node.get_child("evs").size() != 6)
            throw BineditException("Puppet must have 6 evs!");
        if(node.get_child("ivs").size() != 6)
            throw BineditException("Puppet must have 6 ivs!");
        if(node.get_child("skills").size() != 4)
            throw BineditException("Puppet must have 4 skills!");

        auto index = 0;
        for(auto& child : node.get_child("evs"))
            puppet.evs[index++] = child.second.get_value<uint8_t>();
        index = 0;
        for(auto& child : node.get_child("ivs"))
            puppet.ivs[index++] = child.second.get_value<uint8_t>();
        index = 0;
        for(auto& child : node.get_child("skills"))
            puppet.skills[index++] = child.second.get_value<uint16_t>();

        puppet.write(&buf[pos * libtpdp::PUPPET_SIZE_BOX], false);
        libtpdp::encrypt_puppet(&buf[pos * libtpdp::PUPPET_SIZE_BOX], rand_data);
        ++pos;
    }

    if(!write_file(data.wstring(), file.get(), sz))
        throw BineditException("Failed to write to file: " + data.string());
}

static void convert_skills(const Path& in, const Path& out)
{
    std::size_t sz;
    auto file = read_file(in.wstring(), sz);
    if(!file || sz != 0x1DC00)
        throw BineditException("Error reading file: " + in.string());

    {
        ScopedConsoleLock lock;
        std::wcout << in << L" >> " << out << std::endl;
    }

    boost::property_tree::ptree tree;

    for(std::size_t i = 0; i <= (sz - libtpdp::SKILL_DATA_SIZE); i += libtpdp::SKILL_DATA_SIZE)
    {
        libtpdp::SkillData skill(&file[i]);
        boost::property_tree::ptree node;

        if(memchr(&file[i], 0, 32) == NULL)
            throw BineditException("Missing null-terminator at offset: " + std::to_string(i + 31));
        std::string name(&file[i]);

        node.put("id", i / libtpdp::SKILL_DATA_SIZE);
        node.put("name", sjis_to_utf8(name));
        node.put("element", utf_narrow(libtpdp::element_string(skill.element)));
        node.put("type", utf_narrow(libtpdp::skill_type_string(skill.type)));
        node.put("sp", skill.sp);
        node.put("accuracy", skill.accuracy);
        node.put("power", skill.power);
        node.put("priority", skill.priority);
        node.put("effect_chance", skill.effect_chance);
        node.put("effect_id", skill.effect_id);
        node.put("effect_target", skill.effect_target);
        node.put("ynk_classification", skill.ynk_classification);
        //node.put("ynk_id", skill.ynk_id);

        if(skill.ynk_classification > 2)
        {
            ScopedConsoleColorMT color(COLOR_WARN);
            std::wcerr << L"Warning: unknown skill classification: " << (unsigned int)skill.ynk_classification << std::endl;
            std::wcerr << L"For skill id: " << (i / libtpdp::SKILL_DATA_SIZE) << std::endl;
        }

        tree.add_child("skills.", node);
    }

    save_as_utf8(out, tree);
}

static void patch_skills(const Path& data, const Path& json)
{
    std::size_t sz;
    auto file = read_file(data.wstring(), sz);
    if(!file || sz != 0x1DC00)
        throw BineditException("Error reading file: " + data.string());

    {
        ScopedConsoleLock lock;
        std::wcout << json << L" >> " << data << std::endl;
    }

    boost::property_tree::ptree tree;
    read_as_utf8(json, tree);

    for(auto& it : tree.get_child("skills"))
    {
        auto& node = it.second;
        auto pos = node.get<unsigned int>("id") * libtpdp::SKILL_DATA_SIZE;
        if(pos > (sz - libtpdp::SKILL_DATA_SIZE))
            throw BineditException("Too many skills!");

        libtpdp::SkillData skill(&file[pos]);

        std::string name = utf8_to_sjis(node.get<std::string>("name"));
        if(name.size() >= 32)
            throw BineditException("Name must be less than 32 bytes: " + name);
        //memset(&file[pos], 0, 32);
        memcpy(&file[pos], name.data(), name.size());
        file[pos + name.size()] = 0;

        skill.element = (uint8_t)element_to_uint(node.get<std::string>("element"));
        if(skill.element == -1)
            throw BineditException("Invalid element for skill: " + std::to_string(pos / libtpdp::SKILL_DATA_SIZE));

        skill.type = (uint8_t)skill_type_to_uint(node.get<std::string>("type"));
        if(skill.type == -1)
            throw BineditException("Invalid type for skill: " + std::to_string(pos / libtpdp::SKILL_DATA_SIZE));

        skill.sp = node.get<uint8_t>("sp");
        skill.accuracy = node.get<uint8_t>("accuracy");
        skill.power = node.get<uint8_t>("power");
        skill.priority = node.get<int8_t>("priority");
        skill.effect_chance = node.get<uint8_t>("effect_chance");
        skill.effect_id = node.get<uint16_t>("effect_id");
        skill.effect_target = node.get<uint8_t>("effect_target");
        skill.ynk_classification = node.get<uint8_t>("ynk_classification");
        //skill.ynk_id = node.get<uint16_t>("ynk_id");

        skill.write(&file[pos]);
    }

    if(!write_file(data.wstring(), file.get(), sz))
        throw BineditException("Failed to write to file: " + data.string());
}

static void convert_chip(const Path& in, const Path& out)
{
    std::size_t sz;
    auto file = read_file(in.wstring(), sz);
    if(!file || sz != 6656)
        throw BineditException("Error reading file: " + in.string());

    libtpdp::ChipData chp(file.get());
    boost::property_tree::ptree tree;

    for(auto i = 0; i < 256; ++i)
    {
        tree.add("index_map.", chp[i]);
    }

    save_as_utf8(out, tree);
}

static void convert_fmf(const Path& in, const Path& out)
{
    libtpdp::FMFData fmf(in.wstring());
    boost::property_tree::ptree tree;

    tree.put("width", fmf.map_width);
    tree.put("height", fmf.map_height);
    tree.put("payload_length", fmf.payload_len);
    tree.put("num_layers", fmf.num_layers);
    tree.put("unknown_1", fmf.unk1);
    tree.put("unknown_2", fmf.unk2);
    tree.put("unknown_3", fmf.unk3);

    const auto layer_sz = fmf.map_width * fmf.map_height * 2;
    for(std::size_t i = 0; i < fmf.num_layers; ++i)
        tree.add("layers.", base64_encode(fmf.get_layer(i), layer_sz));

    save_as_utf8(out, tree);
}

static void patch_fmf(const Path& data, const Path& json)
{
    libtpdp::FMFData fmf(data.wstring());
    boost::property_tree::ptree tree;
    read_as_utf8(json, tree);

    auto width = tree.get<std::size_t>("width");
    auto height = tree.get<std::size_t>("height");
    auto payload_len = tree.get<std::size_t>("payload_length");
    auto num_layers = tree.get<std::size_t>("num_layers");
    auto unk1 = tree.get<uint8_t>("unknown_1");
    auto unk2 = tree.get<uint8_t>("unknown_2");
    auto unk3 = tree.get<uint8_t>("unknown_3");

    if(num_layers != 13)
        throw BineditException("FMF must have 13 layers.");

    if((width * height * num_layers * 2) != payload_len)
        throw BineditException("Invalid payload length.");

    if((width != fmf.map_width) || (height != fmf.map_height))
    {
        fmf.resize(width, height);
    }

    fmf.num_layers = num_layers;
    fmf.unk1 = unk1;
    fmf.unk2 = unk2;
    fmf.unk3 = unk3;

    if(tree.get_child("layers").size() != 13)
        throw BineditException("FMF must have 13 layers.");

    auto i = 0;
    const auto layer_sz = fmf.map_width * fmf.map_height * 2;
    for(auto& it : tree.get_child("layers"))
    {
        auto str = it.second.get_value<std::string>();
        if(str.size() > b64::encoded_size(layer_sz))
            throw BineditException("Layer data exceeds buffer size.");

        auto c = base64_decode(str, fmf.get_layer(i++), layer_sz);
        if(c > layer_sz)
            throw BineditException("Map layer overflow.");
    }

    fmf.write(data.wstring());
}

static void convert_obs(const Path& in, const Path& out)
{
    const libtpdp::OBSData obs(in.wstring());
    boost::property_tree::ptree tree;

    for(std::size_t i = 0; i < obs.num_entries(); ++i)
    {
        const auto entry = obs.get_entry(i);
        boost::property_tree::ptree node;

        // For some reason std::to_string is a million times
        // faster than the built-in conversion???
        node.put("index", std::to_string(i));
        node.put("object_id", std::to_string(entry.object_id));
        node.put("movement_mode", std::to_string(entry.movement_mode));
        node.put("movement_delay", std::to_string(entry.movement_delay));
        node.put("event_arg", std::to_string(entry.event_arg));
        node.put("event_index", std::to_string(entry.event_index));

        for(auto j : entry.flags)
            node.add("flags.", std::to_string(j));

        tree.add_child("entries.", node);
    }

    save_as_utf8(out, tree);
}

static void patch_obs(const Path& data, const Path& json)
{
    libtpdp::OBSData obs(data.wstring());
    boost::property_tree::ptree tree;
    read_as_utf8(json, tree);

    for(auto& it : tree.get_child("entries"))
    {
        auto& node = it.second;
        libtpdp::OBSEntry entry;

        // For some reason std::stoul is a million times
        // faster than the built-in conversion???
        auto index = std::stoull(node.get<std::string>("index"));
        if(index > obs.num_entries())
            throw BineditException("OBS entry out of range.");

        entry.object_id = (uint16_t)std::stoul(node.get<std::string>("object_id"));
        entry.movement_mode = (uint8_t)std::stoul(node.get<std::string>("movement_mode"));
        entry.movement_delay = (uint8_t)std::stoul(node.get<std::string>("movement_delay"));
        entry.event_arg = (uint16_t)std::stoul(node.get<std::string>("event_arg"));
        entry.event_index = (uint16_t)std::stoul(node.get<std::string>("event_index"));

        if(node.get_child("flags").size() != 12)
            throw BineditException("OBS entry must have 12 flags.");

        auto i = 0;
        for(auto& flag : node.get_child("flags"))
            entry.flags[i++] = (uint8_t)std::stoul(flag.second.get_value<std::string>());

        entry.write(obs[index]);
    }

    obs.write(data.wstring());
}

static void write_version_json(const Path& dir)
{
    boost::property_tree::ptree tree;
    tree.put("major", JSON_MAJOR);
    tree.put("minor", JSON_MINOR);
    tree.put("dev_tools", VERSION_STRING);

    save_as_utf8(dir / "version.json", tree);
}

static std::tuple<unsigned int, unsigned int, std::string> read_version_json(const Path& dir)
{
    boost::property_tree::ptree tree;
    read_as_utf8(dir / "version.json", tree);
    return { tree.get<unsigned int>("major"), tree.get<unsigned int>("minor"), tree.get<std::string>("dev_tools") };
}

template<typename Func, typename... ArgTypes>
std::function<void()> make_task(const std::wstring& file, Func func, std::atomic_uint *acc, ArgTypes... args)
{
    return [=]() {
        try
        {
            func(args...);
            if(acc != nullptr)
                acc->fetch_add(1u);
        }
        catch(const std::exception& ex)
        {
            ScopedConsoleColorMT color(COLOR_CRITICAL);
            std::wcerr << L"Error converting file: " << file << std::endl;
            std::wcerr << utf_widen(ex.what()) << std::endl;
        }
        catch(...)
        {
            ScopedConsoleColorMT color(COLOR_CRITICAL);
            std::wcerr << L"Unknown error converting file: " << file << std::endl;
        }
    };
}

bool convert(const Path& input, int threads)
{
    if(!fs::exists(input) || !fs::is_directory(input))
        throw BineditException("Directory does not exist: " + input.string());

    auto rand_path = input / L"gn_dat1.arc/common/EFile.bin";
    std::size_t rand_sz;
    auto rand_data = read_file(rand_path.wstring(), rand_sz);
    if(!rand_data || rand_sz != 65536)
        throw BineditException("Error opening file: " + rand_path.string() + "\nThis file is REQUIRED for converting .dod files.");

    write_version_json(input);

    std::wcout << L"Using up to " << threads << L" concurrent threads" << std::endl;
    ThreadPool pool(threads);

    for(auto& entry : fs::recursive_directory_iterator(input))
    {
        try
        {
            if(!entry.is_regular_file())
                continue;

            auto in = entry.path();
            auto json = Path(in).replace_extension(L".json");

            if(algo::iequals(in.extension().wstring(), L".mad"))
            {
                pool.queue_task(make_task(in.wstring(), convert_mad, &g_count_mad, in, json));
            }
            else if(algo::iequals(in.extension().wstring(), L".dod"))
            {
                pool.queue_task(make_task(in.wstring(), convert_dod, &g_count_dod, in, json, rand_data.get()));
            }
            else if(algo::iequals(in.extension().wstring(), L".chp"))
            {
                pool.queue_task(make_task(in.wstring(), convert_chip, &g_count_chp, in, json));
            }
            else if(algo::iequals(in.extension().wstring(), L".fmf"))
            {
                json.replace_filename(json.stem().wstring() + L"_fmf.json");
                pool.queue_task(make_task(in.wstring(), convert_fmf, &g_count_fmf, in, json));
            }
            else if(algo::iequals(in.extension().wstring(), L".obs"))
            {
                json.replace_filename(json.stem().wstring() + L"_obs.json");
                pool.queue_task(make_task(in.wstring(), convert_obs, &g_count_obs, in, json));
            }
            else if(algo::iequals(in.filename().wstring(), L"DollData.dbs"))
            {
                pool.queue_task(make_task(in.wstring(), convert_nerds, nullptr, in, json));
            }
            else if(algo::iequals(in.filename().wstring(), L"SkillData.sbs"))
            {
                pool.queue_task(make_task(in.wstring(), convert_skills, nullptr, in, json));
            }
        }
        catch(const std::exception& ex)
        {
            ScopedConsoleColorMT color(COLOR_CRITICAL);
            std::wcerr << L"Error converting file: " << entry.path() << std::endl;
            std::wcerr << utf_widen(ex.what()) << std::endl;
        }
    }

    pool.stop();

    std::wcout << L"converted " << g_count_dod.load() << L" .dod files." << std::endl;
    std::wcout << L"converted " << g_count_mad.load() << L" .mad files." << std::endl;
    std::wcout << L"converted " << g_count_chp.load() << L" .chp files." << std::endl;
    std::wcout << L"converted " << g_count_fmf.load() << L" .fmf files." << std::endl;
    std::wcout << L"converted " << g_count_obs.load() << L" .obs files." << std::endl;

    return true;
}

bool patch(const Path& input, int threads)
{
    if(!fs::exists(input) || !fs::is_directory(input))
        throw BineditException("Directory does not exist: " + input.string());

    if(!fs::exists(input / "version.json"))
        throw BineditException("Could not locate version.json. Please run convert.");

    auto ver = read_version_json(input);
    const auto ver_major = std::get<0>(ver);
    const auto ver_minor = std::get<1>(ver);
    if((ver_major != JSON_MAJOR) || (ver_minor != JSON_MINOR))
    {
        ScopedConsoleColorChanger color(COLOR_CRITICAL);

        bool newer = (ver_major > JSON_MAJOR) || (ver_major == JSON_MAJOR && (ver_minor > JSON_MINOR));

        std::wcerr << L"Error: Working directory contains " << (newer ? "a newer" : "an older") << L" JSON dump format." << std::endl;
        std::wcerr << L"Please run convert to update the JSON, or use " << (newer ? "a newer" : "an older") << L" version of TPDP-Dev-Tools." << std::endl;
        return false;
    }

    auto rand_path = input / L"gn_dat1.arc/common/EFile.bin";
    std::size_t rand_sz;
    auto rand_data = read_file(rand_path.wstring(), rand_sz);
    if(!rand_data || rand_sz != 65536)
        throw BineditException("Error opening file: " + rand_path.string() + "\nThis file is REQUIRED for converting .dod files.");

    std::wcout << L"Using up to " << threads << L" concurrent threads" << std::endl;
    ThreadPool pool(threads);

    for(auto& entry : fs::recursive_directory_iterator(input))
    {
        Path json;

        try
        {
            if(!entry.is_regular_file())
                continue;

            auto in = entry.path();
            json = Path(in).replace_extension(L".json");
            if(algo::iequals(in.extension().wstring(), L".mad"))
            {
                if(fs::exists(json) && fs::is_regular_file(json))
                {
                    pool.queue_task(make_task(json.wstring(), patch_mad, &g_count_mad, in, json));
                }
            }
            else if(algo::iequals(in.extension().wstring(), L".fmf"))
            {
                json.replace_filename(json.stem().wstring() + L"_fmf.json");
                if(fs::exists(json) && fs::is_regular_file(json))
                {
                    pool.queue_task(make_task(json.wstring(), patch_fmf, &g_count_fmf, in, json));
                }
            }
            else if(algo::iequals(in.extension().wstring(), L".obs"))
            {
                json.replace_filename(json.stem().wstring() + L"_obs.json");
                if(fs::exists(json) && fs::is_regular_file(json))
                {
                    pool.queue_task(make_task(json.wstring(), patch_obs, &g_count_obs, in, json));
                }
            }
            else if(algo::iequals(in.extension().wstring(), L".dod"))
            {
                if(fs::exists(json) && fs::is_regular_file(json))
                {
                    pool.queue_task(make_task(json.wstring(), patch_dod, &g_count_dod, in, json, rand_data.get()));
                }
            }
            else if(algo::iequals(in.filename().wstring(), L"DollData.dbs"))
            {
                if(fs::exists(json) && fs::is_regular_file(json))
                {
                    pool.queue_task(make_task(json.wstring(), patch_nerds, nullptr, in, json));
                }
            }
            else if(algo::iequals(in.filename().wstring(), L"SkillData.sbs"))
            {
                if(fs::exists(json) && fs::is_regular_file(json))
                {
                    pool.queue_task(make_task(json.wstring(), patch_skills, nullptr, in, json));
                }
            }
        }
        catch(const std::exception& ex)
        {
            ScopedConsoleColorMT color(COLOR_CRITICAL);
            std::wcerr << L"Error: " << json << std::endl;
            std::wcerr << utf_widen(ex.what()) << std::endl;
        }
    }

    pool.stop();

    std::wcout << L"patched " << g_count_dod.load() << L" .dod files." << std::endl;
    std::wcout << L"patched " << g_count_mad.load() << L" .mad files." << std::endl;
    std::wcout << L"patched " << g_count_fmf.load() << L" .fmf files." << std::endl;
    std::wcout << L"patched " << g_count_obs.load() << L" .obs files." << std::endl;

    return true;
}
