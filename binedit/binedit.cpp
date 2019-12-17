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
#include <libtpdp.h>
#include "binedit.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <chrono>
#include <thread>
#include <future>
#include <functional>
#include <stdexcept>
#include <utility>

namespace algo = boost::algorithm;
namespace fs = std::filesystem;
namespace b64 = boost::beast::detail::base64;

constexpr unsigned int JSON_MAJOR = 1;
constexpr unsigned int JSON_MINOR = 0;

class WorkerError : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

/* synchronization for console access (scoped ownership, recursive) */
class ScopedConsoleLock
{
private:
    static std::recursive_mutex mtx_;
    std::lock_guard<std::recursive_mutex> lock_;

public:
    ScopedConsoleLock() : lock_(mtx_) {}
    ScopedConsoleLock(const ScopedConsoleLock&) = delete;
    ScopedConsoleLock& operator=(const ScopedConsoleLock&) = delete;
};

std::recursive_mutex ScopedConsoleLock::mtx_;

/* scoped ownership of the console + change console text color (color reverted at end of life) */
class ScopedConsoleColorChangerThreadsafe : public ScopedConsoleLock, public ScopedConsoleColorChanger // C++ inheritance rules guarantee ScopedConsoleLock to be constructed first and destroyed last
{
    using ScopedConsoleColorChanger::ScopedConsoleColorChanger;
};

typedef ScopedConsoleColorChangerThreadsafe ScopedConsoleColorMT;

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
        std::cerr << "Error writing to file: " << out.string() << std::endl;
        std::cerr << ex.what() << std::endl;
    }
    catch(const std::exception& ex)
    {
        ScopedConsoleColorMT color(COLOR_WARN);
        std::cerr << "Error writing to file: " << out.string() << std::endl;
        std::cerr << ex.what() << std::endl;
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
        std::cout << in.string() << " >> " << out.string() << std::endl;
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
        std::cout << json.string() << " >> " << data.string() << std::endl;
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
            throw BineditException("Invalid cost value: " + std::to_string(cost) + "\r\nAcceptable values are 0-4.");

        puppet.cost = cost;

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
                    throw BineditException("Puppet " + std::to_string(id) + " style " + std::to_string(style_index) + ": invalid compatibility value: " + std::to_string(i) + "\r\nAcceptable values are 385-512");

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
        std::cerr << "Warning: " << in.string() << "\r\nUnknown weather value: " << (unsigned int)data.weather << std::endl;
    }
    if(data.encounter_type > 1)
    {
        ScopedConsoleColorMT color(COLOR_WARN);
        std::cerr << "Warning: " << in.string() << "\r\nUnknown encounter_type value: " << (unsigned int)data.encounter_type << std::endl;
    }
    if(data.forbid_bike > 1)
    {
        ScopedConsoleColorMT color(COLOR_WARN);
        std::cerr << "Warning: " << in.string() << "\r\nUnknown forbid_bike value: " << (unsigned int)data.forbid_bike << std::endl;
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
            std::cerr << "Warning: " << json.string() << "\r\nPuppet level greater than 100!" << std::endl;
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
            std::cerr << "Warning: " << json.string() << "\r\nPuppet level greater than 100!" << std::endl;
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

    tree.put("intro_text_id", read_le16(&file[0x25]));
    tree.put("end_text_id", read_le16(&file[0x27]));

    unsigned char *buf = (unsigned char*)&file[0x2C];
    for(auto i = 0; i < 6; ++i)
    {
        boost::property_tree::ptree node;
        libtpdp::decrypt_puppet(&buf[i * libtpdp::PUPPET_SIZE_BOX], rand_data);
        libtpdp::Puppet puppet(&buf[i * libtpdp::PUPPET_SIZE_BOX], false);

        node.put("nickname", utf_narrow(puppet.puppet_nickname()));
        node.put("id", puppet.puppet_id);
        node.put("style", puppet.style_index);
        node.put("ability", puppet.ability_index);
        node.put("costume", puppet.costume_index);
        node.put("experience", puppet.exp);
        node.put("mark", utf_narrow(libtpdp::puppet_mark_string(puppet.mark)));
        node.put("held_item", puppet.held_item_id);

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
    write_le16(&file[0x20], portrait_id);

    auto intro_text_id = tree.get<uint16_t>("intro_text_id");
    auto end_text_id = tree.get<uint16_t>("end_text_id");
    write_le16(&file[0x25], intro_text_id);
    write_le16(&file[0x27], end_text_id);

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

        libtpdp::decrypt_puppet(&buf[pos * libtpdp::PUPPET_SIZE_BOX], rand_data);
        libtpdp::Puppet puppet(&buf[pos * libtpdp::PUPPET_SIZE_BOX], false);

        puppet.set_puppet_nickname(utf_widen(node.get<std::string>("nickname")));
        puppet.puppet_id = node.get<uint16_t>("id");
        puppet.style_index = node.get<uint8_t>("style");
        puppet.ability_index = node.get<uint8_t>("ability");
        puppet.costume_index = node.get<uint8_t>("costume");
        puppet.exp = node.get<uint32_t>("experience");
        puppet.mark = (uint8_t)mark_to_uint(node.get<std::string>("mark"));
        puppet.held_item_id = node.get<uint16_t>("held_item");

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
        std::cout << in.string() << " >> " << out.string() << std::endl;
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
            std::cerr << "Warning: unknown skill classification: " << (unsigned int)skill.ynk_classification << std::endl;
            std::cerr << "For skill id: " << (i / libtpdp::SKILL_DATA_SIZE) << std::endl;
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
        std::cout << json.string() << " >> " << data.string() << std::endl;
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
    for(auto i = 0; i < fmf.num_layers; ++i)
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
std::function<void()> make_worker(const std::string& file, Func func, ArgTypes... args)
{
    return [=]() {
        try
        {
            func(args...);
        }
        catch(const std::exception& ex)
        {
            throw WorkerError("Error converting file: " + file + "\r\n" + ex.what());
        }
    };
}

bool convert(const Path& input)
{
    if(!fs::exists(input) || !fs::is_directory(input))
        throw BineditException("Directory does not exist: " + input.string());

    auto rand_path = input / L"gn_dat1.arc/common/EFile.bin";
    std::size_t rand_sz;
    auto rand_data = read_file(rand_path.wstring(), rand_sz);
    if(!rand_data || rand_sz != 65536)
        throw BineditException("Error opening file: " + rand_path.string() + "\r\nThis file is REQUIRED for converting .dod files.");

    write_version_json(input);

    std::vector<std::future<void>> futures;
    auto max_threads = std::thread::hardware_concurrency();
    if(max_threads < 1)
        max_threads = 1;

    std::cout << "Using up to " << max_threads << " concurrent threads" << std::endl;

    auto count_mad = 0;
    auto count_dod = 0;
    auto count_chp = 0;
    auto count_fmf = 0;
    auto count_obs = 0;
    for(auto& entry : fs::recursive_directory_iterator(input))
    {
        if(!entry.is_regular_file())
            continue;

        try
        {
            auto in = entry.path();
            if(algo::iequals(in.extension().wstring(), L".mad"))
            {
                auto json = in;
                json.replace_extension(L".json");
                //convert_mad(in, json);
                ++count_mad;
                futures.push_back(std::async(std::launch::async, make_worker(in.string(), convert_mad, in, json)));
            }
            else if(algo::iequals(in.extension().wstring(), L".dod"))
            {
                auto json = in;
                json.replace_extension(L".json");
                //convert_dod(in, json, rand_data.get());
                ++count_dod;
                futures.push_back(std::async(std::launch::async, make_worker(in.string(), convert_dod, in, json, rand_data.get())));
            }
            else if(algo::iequals(in.extension().wstring(), L".chp"))
            {
                auto json = in;
                json.replace_extension(L".json");
                //convert_chip(in, json);
                ++count_chp;
                futures.push_back(std::async(std::launch::async, make_worker(in.string(), convert_chip, in, json)));
            }
            else if(algo::iequals(in.extension().wstring(), L".fmf"))
            {
                auto json = in;
                json.replace_filename(json.stem().wstring() + L"_fmf.json");
                //convert_fmf(in, json);
                ++count_fmf;
                futures.push_back(std::async(std::launch::async, make_worker(in.string(), convert_fmf, in, json)));
            }
            else if(algo::iequals(in.extension().wstring(), L".obs"))
            {
                auto json = in;
                json.replace_filename(json.stem().wstring() + L"_obs.json");
                //convert_obs(in, json);
                ++count_obs;
                futures.push_back(std::async(std::launch::async, make_worker(in.string(), convert_obs, in, json)));
            }
            else if(algo::iequals(in.filename().wstring(), L"DollData.dbs"))
            {
                auto json = in;
                json.replace_extension(L".json");
                //convert_nerds(in, json);
                futures.push_back(std::async(std::launch::async, make_worker(in.string(), convert_nerds, in, json)));
            }
            else if(algo::iequals(in.filename().wstring(), L"SkillData.sbs"))
            {
                auto json = in;
                json.replace_extension(L".json");
                //convert_skills(in, json);
                futures.push_back(std::async(std::launch::async, make_worker(in.string(), convert_skills, in, json)));
            }
        }
        catch(const std::exception& ex)
        {
            ScopedConsoleColorMT color(COLOR_CRITICAL);
            std::cerr << "Error converting file: " << entry.path().string() << std::endl;
            std::cerr << ex.what() << std::endl;
        }

        for(;;)
        {
            for(auto it = futures.begin(); it != futures.end();)
            {
                try
                {
                    if(it->wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
                    {
                        it->get();
                        it = futures.erase(it);
                    }
                    else
                    {
                        ++it;
                    }
                }
                catch(const WorkerError& ex)
                {
                    ScopedConsoleColorMT color(COLOR_CRITICAL);
                    std::cerr << ex.what() << std::endl;
                    it = futures.erase(it);
                }
            }
            if(futures.size() >= max_threads)
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            else
                break;
        }
    }

    while(!futures.empty())
    {
        try
        {
            if(futures.back().valid())
                futures.back().get();
        }
        catch(const WorkerError& ex)
        {
            ScopedConsoleColorMT color(COLOR_CRITICAL);
            std::cerr << ex.what() << std::endl;
        }
        futures.pop_back();
    }

    std::cout << "converted " << count_dod << " .dod files." << std::endl;
    std::cout << "converted " << count_mad << " .mad files." << std::endl;
    std::cout << "converted " << count_chp << " .chp files." << std::endl;
    std::cout << "converted " << count_fmf << " .fmf files." << std::endl;
    std::cout << "converted " << count_obs << " .obs files." << std::endl;

    return true;
}

bool patch(const Path& input)
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

        std::cerr << "Error: Working directory contains " << (newer ? "a newer" : "an older") << " JSON dump format." << std::endl;
        std::cerr << "Please run convert to update the JSON, or use " << (newer ? "a newer" : "an older") << " version of TPDP-Dev-Tools." << std::endl;
        return false;
    }

    auto rand_path = input / L"gn_dat1.arc/common/EFile.bin";
    std::size_t rand_sz;
    auto rand_data = read_file(rand_path.wstring(), rand_sz);
    if(!rand_data || rand_sz != 65536)
        throw BineditException("Error opening file: " + rand_path.string() + "\r\nThis file is REQUIRED for converting .dod files.");

    std::vector<std::future<void>> futures;
    auto max_threads = std::thread::hardware_concurrency();
    if(max_threads < 1)
        max_threads = 1;

    std::cout << "Using up to " << max_threads << " concurrent threads" << std::endl;

    auto count_mad = 0;
    auto count_dod = 0;
    auto count_fmf = 0;
    auto count_obs = 0;
    for(auto& entry : fs::recursive_directory_iterator(input))
    {
        if(!entry.is_regular_file())
            continue;

        Path json;
        try
        {
            auto in = entry.path();
            if(algo::iequals(in.extension().wstring(), L".mad"))
            {
                json = in;
                json.replace_extension(L".json");
                if(fs::exists(json) && fs::is_regular_file(json))
                {
                    //patch_mad(in, json);
                    ++count_mad;
                    futures.push_back(std::async(std::launch::async, make_worker(json.string(), patch_mad, in, json)));
                }
            }
            else if(algo::iequals(in.extension().wstring(), L".fmf"))
            {
                json = in;
                json.replace_filename(json.stem().wstring() + L"_fmf.json");
                if(fs::exists(json) && fs::is_regular_file(json))
                {
                    //patch_fmf(in, json);
                    ++count_fmf;
                    futures.push_back(std::async(std::launch::async, make_worker(json.string(), patch_fmf, in, json)));
                }
            }
            else if(algo::iequals(in.extension().wstring(), L".obs"))
            {
                json = in;
                json.replace_filename(json.stem().wstring() + L"_obs.json");
                if(fs::exists(json) && fs::is_regular_file(json))
                {
                    //patch_obs(in, json);
                    ++count_obs;
                    futures.push_back(std::async(std::launch::async, make_worker(json.string(), patch_obs, in, json)));
                }
            }
            else if(algo::iequals(in.extension().wstring(), L".dod"))
            {
                json = in;
                json.replace_extension(L".json");
                if(fs::exists(json) && fs::is_regular_file(json))
                {
                    //patch_dod(in, json, rand_data.get());
                    ++count_dod;
                    futures.push_back(std::async(std::launch::async, make_worker(json.string(), patch_dod, in, json, rand_data.get())));
                }
            }
            else if(algo::iequals(in.filename().wstring(), L"DollData.dbs"))
            {
                json = in;
                json.replace_extension(L".json");
                if(fs::exists(json) && fs::is_regular_file(json))
                {
                    //patch_nerds(in, json);
                    futures.push_back(std::async(std::launch::async, make_worker(json.string(), patch_nerds, in, json)));
                }
            }
            else if(algo::iequals(in.filename().wstring(), L"SkillData.sbs"))
            {
                json = in;
                json.replace_extension(L".json");
                if(fs::exists(json) && fs::is_regular_file(json))
                {
                    //patch_skills(in, json);
                    futures.push_back(std::async(std::launch::async, make_worker(json.string(), patch_skills, in, json)));
                }
            }
        }
        catch(const std::exception& ex)
        {
            ScopedConsoleColorMT color(COLOR_CRITICAL);
            std::cerr << "Error: " << json.string() << std::endl;
            std::cerr << ex.what() << std::endl;
        }

        for(;;)
        {
            for(auto it = futures.begin(); it != futures.end();)
            {
                try
                {
                    if(it->wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
                    {
                        it->get();
                        it = futures.erase(it);
                    }
                    else
                    {
                        ++it;
                    }
                }
                catch(const WorkerError & ex)
                {
                    ScopedConsoleColorMT color(COLOR_CRITICAL);
                    std::cerr << ex.what() << std::endl;
                    it = futures.erase(it);
                }
            }
            if(futures.size() >= max_threads)
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            else
                break;
        }
    }

    while(!futures.empty())
    {
        try
        {
            if(futures.back().valid())
                futures.back().get();
        }
        catch(const WorkerError & ex)
        {
            ScopedConsoleColorMT color(COLOR_CRITICAL);
            std::cerr << ex.what() << std::endl;
        }
        futures.pop_back();
    }

    std::cout << "patched " << count_dod << " .dod files." << std::endl;
    std::cout << "patched " << count_mad << " .mad files." << std::endl;
    std::cout << "patched " << count_fmf << " .fmf files." << std::endl;
    std::cout << "patched " << count_obs << " .obs files." << std::endl;

    return true;
}
