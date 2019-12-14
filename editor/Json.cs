using System.Runtime.Serialization;

namespace editor.json
{
    // Compiler can't detect members being set
    // by the JSON serializer and complains a lot
#pragma warning disable CS0649

    [DataContract]
    class VersionJson
    {
        [DataMember]
        public uint major = 0;

        [DataMember]
        public uint minor = 0;

        [DataMember]
        public string dev_tools = "Unknown";
    }

    [DataContract]
    class ChipJson
    {
        [DataMember]
        public uint[] index_map = new uint[256];
    }

    [DataContract]
    class PuppetJson
    {
        [DataMember]
        public string nickname = "";

        [DataMember]
        public uint id;

        [DataMember]
        public uint style;

        [DataMember]
        public uint ability;

        [DataMember]
        public uint costume;

        [DataMember]
        public uint experience;

        [DataMember]
        public string mark = "None";

        [DataMember]
        public uint held_item;

        [DataMember]
        public uint[] skills = new uint[4];

        [DataMember]
        public uint[] ivs = new uint[6];

        [DataMember]
        public uint[] evs = new uint[6];
    }

    [DataContract]
    class DodJson
    {
        [DataMember]
        public string trainer_name = "";

        [DataMember]
        public string trainer_title = "";

        [DataMember]
        public uint portrait_id;

        [DataMember]
        public uint intro_text_id;

        [DataMember]
        public uint end_text_id;

        [DataMember]
        public PuppetJson[] puppets = { new PuppetJson(), new PuppetJson(), new PuppetJson(), new PuppetJson(), new PuppetJson(), new PuppetJson() };

        public string filepath = "";
        public int id;
    }

    [DataContract]
    class MadEncounter
    {
        [DataMember]
        public uint id;

        [DataMember]
        public uint level;

        [DataMember]
        public uint style;

        [DataMember]
        public uint weight;
    }

    [DataContract]
    class MadJson
    {
        [DataMember]
        public string location_name;

        [DataMember]
        public uint[] tilesets = new uint[4];

        [DataMember]
        public uint weather = 0;

        [DataMember]
        public uint overworld_theme = 0;

        [DataMember]
        public uint battle_background = 0;

        [DataMember]
        public uint forbid_bike = 0;

        [DataMember]
        public uint encounter_type = 0;

        [DataMember]
        public uint unknown = 0;

        [DataMember]
        public MadEncounter[] special_encounters = new MadEncounter[5];

        [DataMember]
        public MadEncounter[] normal_encounters = new MadEncounter[10];

        public string filepath;
        public int id;
    }

    [DataContract]
    class FmfJson
    {
        [DataMember]
        public uint width;

        [DataMember]
        public uint height;

        [DataMember]
        public uint payload_length;

        [DataMember]
        public uint num_layers;

        [DataMember]
        public uint unknown_1;

        [DataMember]
        public uint unknown_2;

        [DataMember]
        public uint unknown_3;

        [DataMember]
        public string[] layers = new string[13];

        public string filepath = "";
        public int id;
    }

    [DataContract]
    class ObsEntry
    {
        [DataMember]
        public uint index;

        [DataMember]
        public uint object_id;

        [DataMember]
        public uint movement_mode;

        [DataMember]
        public uint movement_delay;

        [DataMember]
        public uint event_arg;

        [DataMember]
        public uint event_index;

        [DataMember]
        public uint[] flags = new uint[12];
    }

    [DataContract]
    class ObsJson
    {
        [DataMember]
        public ObsEntry[] entries;

        public string filepath = "";
        public int id;
    }

    [DataContract]
    class StyleData
    {
        [DataMember]
        public string type = "None";

        [DataMember]
        public string element1 = "None";

        [DataMember]
        public string element2 = "None";

        [DataMember]
        public uint lvl100_skill = 0;

        [DataMember]
        public uint[] abilities = new uint[2];

        [DataMember]
        public uint[] base_stats = new uint[6];

        [DataMember]
        public uint[] style_skills = new uint[11];

        [DataMember]
        public uint[] lvl70_skills = new uint[8];

        [DataMember]
        public uint[] compatibility = new uint[0];
    }

    [DataContract]
    class DollData
    {
        [DataMember]
        public uint id = 0;

        [DataMember]
        public uint cost = 0;

        [DataMember]
        public uint[] base_skills = new uint[5];

        [DataMember]
        public uint[] item_drop_table = new uint[4];

        [DataMember]
        public StyleData[] styles = { new StyleData(), new StyleData(), new StyleData(), new StyleData() };
    }

    [DataContract]
    class DollDataJson
    {
        [DataMember]
        public DollData[] puppets;
    }

    [DataContract]
    class SkillData
    {
        [DataMember]
        public uint id = 0;

        [DataMember]
        public string name = "Null";

        [DataMember]
        public string element = "None";

        [DataMember]
        public string type = "Focus";

        [DataMember]
        public uint sp = 0;

        [DataMember]
        public uint accuracy = 0;

        [DataMember]
        public uint power = 0;

        [DataMember]
        public int priority = 0;

        [DataMember]
        public uint effect_chance = 0;

        [DataMember]
        public uint effect_id = 0;

        [DataMember]
        public uint effect_target = 0;

        [DataMember]
        public uint ynk_effect_type = 0;

        //[DataMember]
        //public uint ynk_id = 0;

    }

    [DataContract]
    class SkillJson
    {
        [DataMember]
        public SkillData[] skills;
    }

    [DataContract]
    class SettingsJson
    {
        [DataMember]
        public string game_dir = "";

        [DataMember]
        public string working_dir = "";

        [DataMember]
        public bool map_popup = true;
    }

#pragma warning restore CS0649
}