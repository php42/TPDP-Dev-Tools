using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Diagnostics;
using System.IO;
using editor.json;
using System.Runtime.Serialization.Json;
using System.Text.RegularExpressions;

// Map tab of MainWindow

namespace editor
{
    public partial class EditorMainWindow : Form
    {
        private void LoadMaps(string dir)
        {
            DirectoryInfo di = new DirectoryInfo(dir);
            if(!di.Exists)
            {
                throw new Exception("Could not locate map folder");
            }

            maps_.Clear();

            try
            {
                var files = di.GetFiles("*.json", SearchOption.AllDirectories);

                foreach(var file in files)
                {
                    DataContractJsonSerializer s = new DataContractJsonSerializer(typeof(MadJson));
                    var fs = file.OpenRead();

                    try
                    {
                        var map = (MadJson)s.ReadObject(fs);
                        map.filepath = file.FullName;

                        Regex rx = new Regex(@"(?<id>\d{3})\.json$", RegexOptions.Compiled | RegexOptions.IgnoreCase);
                        var match = rx.Match(map.filepath);

                        if(match.Success)
                            map.id = int.Parse(match.Groups["id"].Value);
                        else
                            continue;

                        maps_.Add(map);
                    }
                    catch(Exception e)
                    {
                        throw new Exception("Error parsing file: " + file.FullName + "\r\n" + e.Message);
                    }
                    finally
                    {
                        if(fs != null)
                            fs.Close();
                    }
                }
            }
            catch(Exception e)
            {
                throw new Exception("Error iterating map folder: " + e.Message);
            }

            MapListBox.ClearSelected();
            MapListBox.Items.Clear();
            //MapListBox.DataSource = maps_;
            //MapListBox.DisplayMember = "location_name";
            //MapListBox.ValueMember = "location_name";

            MapNameTextBox.Clear();
            MapFilenameTextBox.Clear();
            ClearEncounter();

            foreach(var map in maps_)
            {
                MapListBox.Items.Add(map.location_name);
            }
        }

        private void SaveMaps()
        {
            foreach(var map in maps_)
            {
                try
                {
                    var ser = new DataContractJsonSerializer(typeof(MadJson));
                    var s = new MemoryStream();
                    ser.WriteObject(s, map);
                    File.WriteAllBytes(map.filepath, s.ToArray());
                }
                catch(Exception e)
                {
                    throw new Exception("Error saving map: " + map.filepath + "\r\n" + e.Message);
                }
            }
        }

        private void ClearEncounter()
        {
            MapPuppetComboBox.SelectedIndex = -1;
            MapPuppetComboBox.Items.Clear();
            MapPuppetComboBox.Text = "Puppet";
            MapStyleComboBox.SelectedIndex = -1;
            MapStyleComboBox.Items.Clear();
            MapStyleComboBox.Text = "Style";
            MapLvlSpinCtrl.Value = 0;
            MapWeightSpinCtrl.Value = 0;
        }

        private void RefreshEncounter()
        {
            var index = EncounterComboBox.SelectedIndex;
            var mapindex = MapListBox.SelectedIndex;
            if(index < 0 || mapindex < 0 || mapindex >= maps_.Count)
                return;

            var map = maps_[mapindex];
            MadEncounter encounter;

            if(index < 10 && index < map.normal_encounters.Length)
            {
                encounter = map.normal_encounters[index];
            }
            else if((index - 10) < map.special_encounters.Length)
            {
                encounter = map.special_encounters[index - 10];
            }
            else
            {
                return;
            }

            MapPuppetComboBox.SelectedIndex = -1;
            MapPuppetComboBox.Items.Clear();
            MapPuppetComboBox.Items.Add(new Tuple<string, uint>("None", 0));
            MapPuppetComboBox.DisplayMember = "Item1";
            MapPuppetComboBox.ValueMember = "Item2";
            if(encounter.id == 0)
                MapPuppetComboBox.SelectedIndex = 0;
            MapLvlSpinCtrl.Value = encounter.level;
            MapWeightSpinCtrl.Value = encounter.weight;
            foreach(var it in puppets_)
            {
                var id = it.Key;
                if(id < puppet_names_.Length)
                {
                    MapPuppetComboBox.Items.Add(new Tuple<string, uint>(puppet_names_[id], id));
                    if(encounter.id == id)
                    {
                        MapPuppetComboBox.SelectedIndex = MapPuppetComboBox.FindStringExact(puppet_names_[id]);
                        if(encounter.style < MapStyleComboBox.Items.Count)
                            MapStyleComboBox.SelectedIndex = (int)encounter.style;
                    }
                }
            }
        }

        private void MapListBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            var index = MapListBox.SelectedIndex;
            if(index >= 0 && index < maps_.Count)
            {
                var map = maps_[index];
                MapNameTextBox.Text = map.location_name;
                MapFilenameTextBox.Text = map.filepath;
                if(map.id > 0 && map.id < map_names_.Length)
                    MapDispNameBox.Text = map_names_[map.id];
                else
                    MapDispNameBox.Text = "";
                RefreshEncounter();
            }
        }

        private void EncounterComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            RefreshEncounter();
        }

        private void MapPuppetComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            var index = MapPuppetComboBox.SelectedIndex;
            if(index < 0)
                return;

            MapStyleComboBox.SelectedIndex = -1;
            MapStyleComboBox.Items.Clear();
            MapStyleComboBox.Text = "Style";
            var val = (Tuple<string, uint>)MapPuppetComboBox.SelectedItem;
            uint id = val.Item2;

            if(puppets_.ContainsKey(id))
            {
                foreach(var style in puppets_[id].styles)
                {
                    MapStyleComboBox.Items.Add(style.type);
                }
            }

            var mapindex = MapListBox.SelectedIndex;
            var encounterindex = EncounterComboBox.SelectedIndex;
            if(mapindex >= 0 && encounterindex >= 0)
            {
                if(encounterindex < 10 && encounterindex < maps_[mapindex].normal_encounters.Length)
                    maps_[mapindex].normal_encounters[encounterindex].id = id;
                else if((encounterindex - 10) < maps_[mapindex].special_encounters.Length)
                    maps_[mapindex].special_encounters[encounterindex - 10].id = id;
            }
        }

        private void MapStyleComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            var index = MapStyleComboBox.SelectedIndex;
            var mapindex = MapListBox.SelectedIndex;
            var encounterindex = EncounterComboBox.SelectedIndex;
            var puppetindex = MapPuppetComboBox.SelectedIndex;
            if(index < 0 || mapindex < 0 || encounterindex < 0 || puppetindex < 0)
                return;

            if(encounterindex < 10 && encounterindex < maps_[mapindex].normal_encounters.Length)
                maps_[mapindex].normal_encounters[encounterindex].style = (uint)index;
            else if((encounterindex - 10) < maps_[mapindex].special_encounters.Length)
                maps_[mapindex].special_encounters[encounterindex - 10].style = (uint)index;
        }

        private void MapLvlSpinCtrl_ValueChanged(object sender, EventArgs e)
        {
            var val = MapLvlSpinCtrl.Value;
            var mapindex = MapListBox.SelectedIndex;
            var encounterindex = EncounterComboBox.SelectedIndex;
            if(val < 0 || mapindex < 0 || encounterindex < 0)
                return;

            if(encounterindex < 10 && encounterindex < maps_[mapindex].normal_encounters.Length)
                maps_[mapindex].normal_encounters[encounterindex].level = (uint)val;
            else if((encounterindex - 10) < maps_[mapindex].special_encounters.Length)
                maps_[mapindex].special_encounters[encounterindex - 10].level = (uint)val;
        }

        private void MapWeightSpinCtrl_ValueChanged(object sender, EventArgs e)
        {
            var val = MapWeightSpinCtrl.Value;
            var mapindex = MapListBox.SelectedIndex;
            var encounterindex = EncounterComboBox.SelectedIndex;
            if(val < 0 || mapindex < 0 || encounterindex < 0)
                return;

            if(encounterindex < 10 && encounterindex < maps_[mapindex].normal_encounters.Length)
                maps_[mapindex].normal_encounters[encounterindex].weight = (uint)val;
            else if((encounterindex - 10) < maps_[mapindex].special_encounters.Length)
                maps_[mapindex].special_encounters[encounterindex - 10].weight = (uint)val;
        }

        private void MapNameTextBox_TextChanged(object sender, EventArgs e)
        {
            var mapindex = MapListBox.SelectedIndex;
            if(!MapNameTextBox.Focused || mapindex < 0)
                return;

            maps_[mapindex].location_name = MapNameTextBox.Text;
            MapListBox.Items[mapindex] = MapNameTextBox.Text;
        }

        private void MapDispNameBox_TextChanged(object sender, EventArgs e)
        {
            var mapindex = MapListBox.SelectedIndex;
            if(!MapDispNameBox.Focused || mapindex < 0)
                return;

            var id = maps_[mapindex].id;
            if(id > 0 && id < map_names_.Length)
                map_names_[id] = MapDispNameBox.Text;
        }
    }
}
