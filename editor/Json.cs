using System.Runtime.Serialization;

namespace editor.json
{
    [DataContract]
    class DodJson
    {

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

// shush
#pragma warning disable 0649
    [DataContract]
    class MadJson
    {
        [DataMember]
        public string location_name;

        [DataMember]
        public MadEncounter[] special_encounters;

        [DataMember]
        public MadEncounter[] normal_encounters;

        public string filepath;
        public int id;
    }

    [DataContract]
    class StyleData
    {
        [DataMember]
        public string type;

        [DataMember]
        public string element1;

        [DataMember]
        public string element2;

        [DataMember]
        public uint lvl100_skill;

        [DataMember]
        public uint[] abilities;

        [DataMember]
        public uint[] base_stats;

        [DataMember]
        public uint[] style_skills;

        [DataMember]
        public uint[] lvl70_skills;

        [DataMember]
        public uint[] compatibility;
    }

    [DataContract]
    class DollData
    {
        [DataMember]
        public uint id;

        [DataMember]
        public uint cost;

        [DataMember]
        public uint[] base_skills;

        [DataMember]
        public uint[] item_drop_table;

        [DataMember]
        public StyleData[] styles;
    }
#pragma warning restore 0649

    [DataContract]
    class DollDataJson
    {
        [DataMember]
        public DollData[] puppets;
    }
}