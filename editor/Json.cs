using System.Runtime.Serialization;

namespace editor.json
{
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

        // shush
#pragma warning disable CS0649
        [DataMember]
        public MadEncounter[] special_encounters;

        [DataMember]
        public MadEncounter[] normal_encounters;
#pragma warning restore CS0649

        public string filepath;
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

        [DataMember]
        public uint ynk_id = 0;

    }

    [DataContract]
    class SkillJson
    {
        // shush
#pragma warning disable CS0649
        [DataMember]
        public SkillData[] skills;
#pragma warning restore CS0649
    }
}