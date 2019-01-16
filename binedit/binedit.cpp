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
#include "../common/textconvert.h"
#include "../common/filesystem.h"
#include "../common/console.h"
#include <libtpdp.h>
#include "binedit.h"
#include <iostream>
#include <sstream>
#include <vector>

namespace algo = boost::algorithm;
namespace fs = std::filesystem;

static void save_as_utf8(const Path& out, boost::property_tree::ptree& tree)
{
    /* we now need to do some f***ery to append a UTF-8 BOM to the front of the file as a safety measure
     * to try to force text editors into UTF-8 mode */
    std::stringbuf buf("\xEF\xBB\xBF"); // UTF-8 BOM
    std::ostream stream(&buf);
    stream.seekp(0, stream.end);
    boost::property_tree::write_json(stream, tree);
    auto outfile = buf.str();
    if(!write_file(out.wstring(), outfile.data(), outfile.size()))
    {
        ScopedConsoleColorChanger color(COLOR_WARN);
        std::cerr << "Error writing to file: " << out.string() << std::endl;
    }
}

static void read_as_utf8(const Path& in, boost::property_tree::ptree& tree)
{
    /* this is a wrapper to avoid using boost::property_tree::json_parser for I/O because
     * it doesn't have wstring methods */
    std::size_t sz;
    std::string str;
    auto file = read_file(in.wstring(), sz);
    if(!file)
        throw BineditException("Error reading file: " + in.string());
    str.assign(file.get(), sz);
    file.reset();

    std::stringbuf buf(std::move(str));
    std::istream stream(&buf);
    boost::property_tree::read_json(stream, tree);
}

static unsigned int element_to_uint(const std::string& name)
{
    auto tmp = utf_widen(name); // converting text form to integer representation
    for(unsigned int i = libtpdp::ELEMENT_NONE; i < libtpdp::ELEMENT_MAX; ++i)
        if(algo::iequals(tmp, libtpdp::element_string(i)))
            return i;

    return -1;
}

static unsigned int style_to_uint(const std::string& name)
{
    auto tmp = utf_widen(name); // converting text form to integer representation
    for(unsigned int i = libtpdp::STYLE_NONE; i < libtpdp::STYLE_MAX; ++i)
        if(algo::iequals(tmp, libtpdp::style_string(i)))
            return i;

    return -1;
}

static unsigned int skill_type_to_uint(const std::string& name)
{
    auto tmp = utf_widen(name); // converting text form to integer representation
    for(unsigned int i = libtpdp::SKILL_TYPE_FOCUS; i < libtpdp::SKILL_TYPE_MAX; ++i)
        if(algo::iequals(tmp, libtpdp::skill_type_string(i)))
            return i;

    return -1;
}

static unsigned int mark_to_uint(const std::string& name)
{
    auto tmp = utf_widen(name); // converting text form to integer representation
    for(unsigned int i = libtpdp::MARK_NONE; i < libtpdp::MARK_MAX; ++i)
        if(algo::iequals(tmp, libtpdp::puppet_mark_string(i)))
            return i;

    return -1;
}

/* convert the DollData.dbs file containing the definitions
 * of all the puppets to json */
static void convert_nerds(const Path& in, const Path& out)
{
    std::size_t sz;
    auto file = read_file(in.wstring(), sz);

    if(file == nullptr)
        throw BineditException("failed to open file: " + in.string());

    std::cout << in.string() << " >> " << out.string() << std::endl;

    boost::property_tree::ptree tree;

    for(std::size_t pos = 0; (sz - pos) >= libtpdp::PUPPET_DATA_SIZE; pos += libtpdp::PUPPET_DATA_SIZE)
    {
        libtpdp::PuppetData data(&file[pos]);
        data.id = (uint16_t)(pos / libtpdp::PUPPET_DATA_SIZE);

        if(data.styles[0].style_type == 0) // not a real puppet
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

    std::cout << json.string() << " >> " << data.string() << std::endl;

    for(auto& it : tree.get_child("puppets")) // iterate puppets
    {
        auto& node = it.second;
        auto id = node.get<unsigned int>("id");

        // puppet id
        auto offset = (id * libtpdp::PUPPET_DATA_SIZE);
        if(id >= (sz / libtpdp::PUPPET_DATA_SIZE))
            throw BineditException("Puppet ID too large: " + std::to_string(id));

        libtpdp::PuppetData puppet(&file[offset]);
        puppet.id = id;

        // puppet cost
        auto cost = node.get<unsigned int>("cost");
        if(cost > 4)
            throw BineditException("Invalid cost value: " + std::to_string(cost) + "\r\nAcceptable values are 0-4.");

        puppet.cost = cost;

        auto index = 0;

        // puppet base skills
        if(node.get_child("base_skills").size() != 5)
            throw BineditException("puppet " + std::to_string(id) + " must have 5 base skills");
        for(auto& it : node.get_child("base_skills"))
            puppet.base_skills[index++] = it.second.get_value<unsigned int>();
        index = 0;

        // item drops
        if(node.get_child("item_drop_table").size() != 4)
            throw BineditException("puppet " + std::to_string(id) + " must have 4 drop items");
        for(auto& it : node.get_child("item_drop_table"))
            puppet.item_drop_table[index++] = it.second.get_value<unsigned int>();
        index = 0;

        // styles
        if(node.get_child("styles").size() != 4)
            throw BineditException("puppet " + std::to_string(id) + " must have 4 styles");
        auto style_index = 0;
        for(auto& it : node.get_child("styles"))
        {
            auto& style = it.second;
            auto& style_data = puppet.styles[style_index++];

            // style type
            auto name = style.get<std::string>("type");
            style_data.style_type = style_to_uint(name);
            if(style_data.style_type == -1)
                throw BineditException("Invalid style type: " + name + " for puppet: " + std::to_string(id));

            // element 1
            name = style.get<std::string>("element1");
            style_data.element1 = element_to_uint(name);
            if(style_data.element1 == -1)
                throw BineditException("Invalid element: " + name + " for puppet: " + std::to_string(id));

            // element 2
            name = style.get<std::string>("element2");
            style_data.element2 = element_to_uint(name);
            if(style_data.element2 == -1)
                throw BineditException("Invalid element: " + name + " for puppet: " + std::to_string(id));

            // level 100 skill
            style_data.lv100_skill = style.get<unsigned int>("lvl100_skill");

            // stats
            if(style.get_child("base_stats").size() != 6)
                throw BineditException("Puppet " + std::to_string(id) + " style " + std::to_string(style_index) + ": must have 6 base stats");
            for(auto& it : style.get_child("base_stats"))
                style_data.base_stats[index++] = it.second.get_value<unsigned int>();
            index = 0;

            // abilities
            if(style.get_child("abilities").size() != 2)
                throw BineditException("Puppet " + std::to_string(id) + " style " + std::to_string(style_index) + ": must have 2 abilities");
            for(auto& it : style.get_child("abilities"))
                style_data.abilities[index++] = it.second.get_value<unsigned int>();
            index = 0;

            // style skills
            if(style.get_child("style_skills").size() != 11)
                throw BineditException("Puppet " + std::to_string(id) + " style " + std::to_string(style_index) + ": must have 11 style skills");
            for(auto& it : style.get_child("style_skills"))
                style_data.style_skills[index++] = it.second.get_value<unsigned int>();
            index = 0;

            // level 70 skills
            if(style.get_child("lvl70_skills").size() != 8)
                throw BineditException("Puppet " + std::to_string(id) + " style " + std::to_string(style_index) + ": must have 8 level 70 skills");
            for(auto& it : style.get_child("lvl70_skills"))
                style_data.lv70_skills[index++] = it.second.get_value<unsigned int>();

            // skillcards
            if(style.get_child("compatibility").size() > 128)
                throw BineditException("Puppet " + std::to_string(id) + ": Too many skillcards!");
            std::vector<unsigned int> skillcards;
            skillcards.reserve(128);
            memset(style_data.skill_compat_table, 0, sizeof(style_data.skill_compat_table));
            for(auto& it : style.get_child("compatibility"))
                skillcards.push_back(it.second.get_value<unsigned int>());
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

    if(tree.get_child("special_encounters").size() > 5)
        throw BineditException("Too many special encounters! Max is 5");
    if(tree.get_child("normal_encounters").size() > 10)
        throw BineditException("Too many normal encounters! Max is 10");

    int index = 0;
    for(auto& it : tree.get_child("special_encounters"))
    {
        libtpdp::MADEncounter encounter;
        encounter.index = index;
        encounter.id = it.second.get<unsigned int>("id");
        encounter.level = it.second.get<unsigned int>("level");
        encounter.style = it.second.get<unsigned int>("style");
        encounter.weight = it.second.get<unsigned int>("weight");

        if(encounter.level > 100)
        {
            ScopedConsoleColorChanger color(COLOR_WARN);
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
        encounter.id = it.second.get<unsigned int>("id");
        encounter.level = it.second.get<unsigned int>("level");
        encounter.style = it.second.get<unsigned int>("style");
        encounter.weight = it.second.get<unsigned int>("weight");

        if(encounter.level > 100)
        {
            ScopedConsoleColorChanger color(COLOR_WARN);
            std::cerr << "Warning: " << json.string() << "\r\nPuppet level greater than 100!" << std::endl;
        }
        if(encounter.style > 3)
            throw BineditException("Puppet style greater than 3!");

        encounter.write(mad, index++, false);
    }

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

        for(auto i : puppet.skills)
            node.add("skills.", i);

        for(auto i : puppet.ivs)
            node.add("ivs.", i);

        for(auto i : puppet.evs)
            node.add("evs.", i);

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
        puppet.puppet_id = node.get<unsigned int>("id");
        puppet.style_index = node.get<unsigned int>("style");
        puppet.ability_index = node.get<unsigned int>("ability");
        puppet.costume_index = node.get<unsigned int>("costume");
        puppet.exp = node.get<unsigned int>("experience");
        puppet.mark = mark_to_uint(node.get<std::string>("mark"));
        puppet.held_item_id = node.get<unsigned int>("held_item");

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
            puppet.evs[index++] = child.second.get_value<unsigned int>();
        index = 0;
        for(auto& child : node.get_child("ivs"))
            puppet.ivs[index++] = child.second.get_value<unsigned int>();
        index = 0;
        for(auto& child : node.get_child("skills"))
            puppet.skills[index++] = child.second.get_value<unsigned int>();

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

    std::cout << in.string() << " >> " << out.string() << std::endl;

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
        node.put("ynk_effect_type", skill.effect_type);
        node.put("ynk_id", skill.ynk_id);

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

    std::cout << json.string() << " >> " << data.string() << std::endl;

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

        skill.element = element_to_uint(node.get<std::string>("element"));
        if(skill.element == -1)
            throw BineditException("Invalid element for skill: " + std::to_string(pos / libtpdp::SKILL_DATA_SIZE));

        skill.type = skill_type_to_uint(node.get<std::string>("type"));
        if(skill.type == -1)
            throw BineditException("Invalid type for skill: " + std::to_string(pos / libtpdp::SKILL_DATA_SIZE));

        skill.sp = node.get<unsigned int>("sp");
        skill.accuracy = node.get<unsigned int>("accuracy");
        skill.power = node.get<unsigned int>("power");
        skill.priority = node.get<int>("priority");
        skill.effect_chance = node.get<unsigned int>("effect_chance");
        skill.effect_id = node.get<unsigned int>("effect_id");
        skill.effect_target = node.get<unsigned int>("effect_target");
        skill.effect_type = node.get<unsigned int>("ynk_effect_type");
        skill.ynk_id = node.get<unsigned int>("ynk_id");

        skill.write(&file[pos]);
    }

    if(!write_file(data.wstring(), file.get(), sz))
        throw BineditException("Failed to write to file: " + data.string());
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

    auto count_mad = 0;
    auto count_dod = 0;
    for(auto& entry : fs::recursive_directory_iterator(input))
    {
        if(!entry.is_regular_file())
            continue;

        try
        {
            if(algo::iequals(entry.path().extension().wstring(), L".mad"))
            {
                auto json = entry.path();
                json.replace_extension(L".json");
                convert_mad(entry.path(), json);
                ++count_mad;
            }
            else if(algo::iequals(entry.path().extension().wstring(), L".dod"))
            {
                auto json = entry.path();
                json.replace_extension(L".json");
                convert_dod(entry.path(), json, rand_data.get());
                ++count_dod;
            }
            else if(algo::iequals(entry.path().filename().wstring(), L"DollData.dbs"))
            {
                auto json = entry.path();
                json.replace_extension(L".json");
                convert_nerds(entry.path(), json);
            }
            else if(algo::iequals(entry.path().filename().wstring(), L"SkillData.sbs"))
            {
                auto json = entry.path();
                json.replace_extension(L".json");
                convert_skills(entry.path(), json);
            }
        }
        catch(const std::exception& ex)
        {
            ScopedConsoleColorChanger color(COLOR_CRITICAL);
            std::cerr << "Error converting file: " << entry.path().string() << std::endl;
            std::cerr << ex.what() << std::endl;
            return false;
        }
    }

    std::cout << "converted " << count_dod << " .dod files." << std::endl;
    std::cout << "converted " << count_mad << " .mad files." << std::endl;

    return true;
}

bool patch(const Path& input)
{
    if(!fs::exists(input) || !fs::is_directory(input))
        throw BineditException("Directory does not exist: " + input.string());

    auto rand_path = input / L"gn_dat1.arc/common/EFile.bin";
    std::size_t rand_sz;
    auto rand_data = read_file(rand_path.wstring(), rand_sz);
    if(!rand_data || rand_sz != 65536)
        throw BineditException("Error opening file: " + rand_path.string() + "\r\nThis file is REQUIRED for converting .dod files.");

    auto count_mad = 0;
    auto count_dod = 0;
    for(auto& entry : fs::recursive_directory_iterator(input))
    {
        if(!entry.is_regular_file())
            continue;

        Path json;
        try
        {
            if(algo::iequals(entry.path().extension().wstring(), L".mad"))
            {
                json = entry.path();
                json.replace_extension(L".json");
                if(fs::exists(json) && fs::is_regular_file(json))
                {
                    patch_mad(entry.path(), json);
                    ++count_mad;
                }
            }
            else if(algo::iequals(entry.path().extension().wstring(), L".dod"))
            {
                json = entry.path();
                json.replace_extension(L".json");
                if(fs::exists(json) && fs::is_regular_file(json))
                {
                    patch_dod(entry.path(), json, rand_data.get());
                    ++count_dod;
                }
            }
            else if(algo::iequals(entry.path().filename().wstring(), L"DollData.dbs"))
            {
                json = entry.path();
                json.replace_extension(L".json");
                if(fs::exists(json) && fs::is_regular_file(json))
                    patch_nerds(entry.path(), json);
            }
            else if(algo::iequals(entry.path().filename().wstring(), L"SkillData.sbs"))
            {
                json = entry.path();
                json.replace_extension(L".json");
                if(fs::exists(json) && fs::is_regular_file(json))
                    patch_skills(entry.path(), json);
            }
        }
        catch(const std::exception& ex)
        {
            ScopedConsoleColorChanger color(COLOR_CRITICAL);
            std::cerr << "Error: " << json.string() << std::endl;
            std::cerr << ex.what() << std::endl;
            return false;
        }
    }

    std::cout << "patched " << count_dod << " .dod files." << std::endl;
    std::cout << "patched " << count_mad << " .mad files." << std::endl;

    return true;
}
