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

#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <set>

namespace libtpdp
{

constexpr unsigned int SKILL_DATA_SIZE = 0x77;
constexpr unsigned int STYLE_DATA_SIZE = 0x65;
constexpr unsigned int PUPPET_DATA_SIZE = 0x1F1;

typedef std::vector<std::wstring> CSVEntry;

enum PuppetStyleType
{
	STYLE_NONE = 0,
	STYLE_NORMAL,
	STYLE_POWER,
	STYLE_DEFENSE,
	STYLE_ASSIST,
	STYLE_SPEED,
	STYLE_EXTRA,
    STYLE_MAX
};

enum ElementType
{
	ELEMENT_NONE = 0,
	ELEMENT_VOID,
	ELEMENT_FIRE,
	ELEMENT_WATER,
	ELEMENT_NATURE,
	ELEMENT_EARTH,
	ELEMENT_STEEL,
	ELEMENT_WIND,
	ELEMENT_ELECTRIC,
	ELEMENT_LIGHT,
	ELEMENT_DARK,
	ELEMENT_NETHER,
	ELEMENT_POISON,
	ELEMENT_FIGHTING,
	ELEMENT_ILLUSION,
	ELEMENT_SOUND,
	ELEMENT_DREAM,
	ELEMENT_WARPED,
    ELEMENT_MAX
};

enum SkillType
{
	SKILL_TYPE_FOCUS = 0,
	SKILL_TYPE_SPREAD,
	SKILL_TYPE_STATUS,
    SKILL_TYPE_MAX
};

/* base data for skills */
class SkillData
{
public:
	//char name[32];									/* for reference, name field is 32 bytes long */
	uint8_t element, power, accuracy, sp;
	int8_t priority;
	uint8_t type;									    /* focus, spread, status */
    uint8_t zero;
    uint16_t effect_id;
    uint8_t effect_chance, effect_target;               /* best guess. chance seems to be ignored on status effect skills. target = 0 for self, 1 for opponent */
    uint8_t effect_type;                                /* YnK only */
    uint16_t ynk_id;                                    /* YnK only */
	//uint8_t unknown_0x2e[73];							/* for reference, 73 bytes of padding or who knows what */

	SkillData() : element(0), power(0), accuracy(0), sp(0), priority(0), type(0), zero(0),
                  effect_id(0), effect_chance(0), effect_target(0), effect_type(0), ynk_id(0) {}
	SkillData(const void *data) {read(data);}

	void read(const void *data);
    void write(void *data);
};

/* base data for an individual style of a puppet */
class StyleData
{
public:
	uint8_t style_type, element1, element2;	/* style_type = normal, power, defense, etc */
	uint8_t base_stats[6];
	uint16_t abilities[2];
	uint16_t style_skills[11];				/* lv. 0, 0, 0, 0, 30, 36, 42, 49, 56, 63, 70 */
    uint16_t lv100_skill;                   /* skill learned at level 100 */
	uint8_t skill_compat_table[16];			/* bitfield of 128 boolean values indicating ability to learn skill cards */
	uint16_t lv70_skills[8];				/* extra skills at level 70 */

    //std::set<unsigned int> skillset;        /* set of all skills ids this puppet can learn by levelling (used internally, not present in game data) */

    StyleData();
	StyleData(const void *data) {read(data);}

	void read(const void *data);
    void write(void *data);
    std::wstring style_string() const;
};

/* base data for a puppet (including styles) */
class PuppetData
{
public:
	//char name[32];
	uint8_t cost;                   /* exp cost modifier */
	uint16_t base_skills[5];        /* lvl 7, 10, 14, 19, 24 */
	uint16_t item_drop_table[4];	/* items dropped when defeated in the wild */
	uint16_t id;                    /* id isn't actually parsed, but is indicated by its position in the file */
	StyleData styles[4];

	PuppetData();
	PuppetData(const void *data) {read(data);}

	void read(const void *data);
    void write(void *data);

    int level_to_learn(unsigned int style_index, unsigned int skill_id) const;  /* level required to learn a skill, returns -1 if puppet cannot learn the skill by levelling */
	int max_style_index() const;
};

/* base data for items */
class ItemData
{
public:
    std::wstring name, description;	/* description may contain escape sequences like "\n" */
    int id, type, price;			/* type = junk, consumable, etc. type 255 = unimplemented (id 0 (nothing) also uses type 255) */
    bool combat;					/* can be used in battle (healing items etc.) */
    bool common;					/* item is considered to be common (?) */
    bool can_discard;				/* player is able to discard this item from their inventory (?) */
    bool held;						/* this item can be held by puppets */
    bool reincarnation;				/* this item is used for reincarnation */
    int skill_id;					/* (skill cards) id of the skill this item teaches */

    ItemData() : id(0), type(255), price(0), combat(false), common(false), can_discard(false), held(false), reincarnation(false), skill_id(0) {}
    ItemData(const CSVEntry& data, bool ynk) { parse(data, ynk); }

    bool parse(const CSVEntry& data, bool ynk);

    /* returns true if item is valid and usable in-game */
    inline bool is_valid() const { return (type < 255); }
};

class MADData;

/* describes an encounter entry from a MAD file */
class MADEncounter
{
public:
	int index;
	uint16_t id;
	uint8_t level;
	uint8_t style;
	uint8_t weight;
    
    /* weight determines encounter rate relative to other puppets in the same grass in the same area.
     * encounter percentage is calculated by the weight of the given puppet divided by the sum of weights */

	MADEncounter() {};
	MADEncounter(int index, uint16_t id, uint8_t level, uint8_t style, uint8_t weight) : index(index), id(id), level(level), style(style), weight(weight) {}
	MADEncounter(const MADData& data, int index, bool special) { read(data, index, special); };

	/* read the encounter at the given index from the supplied MAD file, 'special' toggles between normal and blue grass encounters */
	void read(const MADData& data, int index, bool special);

	/* save this encounter to the given index in the supplied MAD file, 'special' toggles between normal and blue grass encounters */
	void write(MADData& data, int index, bool special);
};

/* data from .mad files. details wild puppet encounters for a specific location
 * table indices correlate to the same index in other tables for the same grass type */
class MADData
{
public:
    /* encounters in normal grass */
    uint16_t puppet_ids[10];            /* IDs of puppets found in normal grass */
    uint8_t puppet_levels[10];          /* Average levels of puppets found in normal grass */
    uint8_t puppet_styles[10];          /* Style of puppets in normal grass (index into puppet style table, see PuppetData) */
    uint8_t puppet_ratios[10];          /* Encouter rate weightings of puppets in normal grass */

    /* encounters in blue grass */
    uint16_t special_puppet_ids[5];     /* IDs of puppets found in blue grass */
    uint8_t special_puppet_levels[5];   /* Average levels of puppets found in blue grass */
    uint8_t special_puppet_styles[5];   /* Style of puppets in blue grass (index into puppet style table, see PuppetData) */
    uint8_t special_puppet_ratios[5];   /* Encouter rate weightings of puppets in blue grass */

    /* null-terminated shift-jis string identifying the location */
    char location_name[32];

    MADData() {}
    MADData(const void *data) { read(data); }

    void read(const void *data);
    void write(void *data);
	void clear_encounters();
};

/* parses csv files and splits each field into a separate string */
class CSVFile
{
private:
    std::vector<CSVEntry> value_map_;

public:
    CSVFile() {}
    CSVFile(const void *data, std::size_t len) { parse(data, len); }

    bool parse(const void *data, std::size_t len);
    const CSVEntry& get_line(int id) const { return value_map_[id]; }
    CSVEntry& get_line(int id) { return value_map_[id]; }
    const CSVEntry& operator[](int id) const { return value_map_[id]; }
    CSVEntry& operator[](int id) { return value_map_[id]; }
    void clear() { value_map_.clear(); }

    const std::vector<CSVEntry>& data() const { return value_map_; }
    std::vector<CSVEntry>& data() { return value_map_; }
    std::vector<CSVEntry>::const_iterator begin() const { return value_map_.cbegin(); }
    std::vector<CSVEntry>::const_iterator end() const { return value_map_.cend(); }

    auto& front() { return value_map_.front(); }
    const auto& front() const { return value_map_.front(); }
    auto& back() { return value_map_.back(); }
    const auto& back() const { return value_map_.back(); }

    std::size_t num_lines() const { return value_map_.size(); }
    std::size_t num_fields() const { return value_map_.empty() ? 0 : value_map_[0].size(); }
    const std::wstring& get_field(unsigned int line, unsigned int field) const { return value_map_[line][field]; }

    std::string to_string() const;
};

std::wstring element_string(unsigned int element);
std::wstring style_string(unsigned int style);
std::wstring skill_type_string(unsigned int type);

unsigned int level_from_exp(unsigned int cost, unsigned int exp, bool ynk);
unsigned int exp_for_level(unsigned int cost, unsigned int level, bool ynk);

}
