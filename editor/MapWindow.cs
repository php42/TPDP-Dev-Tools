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
// TODO: Clean up this disaster

namespace editor
{
    public partial class EditorMainWindow : Form
    {
        private List<FmfJson> fmfs_ = new List<FmfJson>();
        private List<ObsJson> obss_ = new List<ObsJson>();
        private FmfJson fmf_data_;
        private ObsJson obs_data_;
        private byte[][] map_layers_ = new byte[13][];
        private int selected_map_;

        private void LoadMad(string filepath, int id)
        {
            DataContractJsonSerializer ser = new DataContractJsonSerializer(typeof(MadJson));
            var buf = File.ReadAllBytes(filepath);
            var s = new MemoryStream(buf);
            var map = (MadJson)ser.ReadObject(s);

            map.id = id;
            map.filepath = filepath;

            maps_.Add(map);
        }

        private void LoadFmf(string filepath, int id)
        {
            DataContractJsonSerializer ser = new DataContractJsonSerializer(typeof(FmfJson));
            var buf = File.ReadAllBytes(filepath);
            var s = new MemoryStream(buf);
            var fmf = (FmfJson)ser.ReadObject(s);

            fmf.id = id;
            fmf.filepath = filepath;

            fmfs_.Add(fmf);
        }

        private void LoadObs(string filepath, int id)
        {
            DataContractJsonSerializer ser = new DataContractJsonSerializer(typeof(ObsJson));
            var buf = File.ReadAllBytes(filepath);
            var s = new MemoryStream(buf);
            var obs = (ObsJson)ser.ReadObject(s);

            obs.id = id;
            obs.filepath = filepath;

            obss_.Add(obs);
        }

        private void LoadMaps(string dir)
        {
            DirectoryInfo di = new DirectoryInfo(dir);
            if(!di.Exists)
            {
                throw new Exception("Could not locate map folder");
            }

            maps_.Clear();
            fmfs_.Clear();
            obss_.Clear();

            try
            {
                var files = di.GetFiles("*.json", SearchOption.AllDirectories);

                foreach(var file in files)
                {
                    Regex rx = new Regex(@"(?<id>\d{3})\.json$", RegexOptions.Compiled | RegexOptions.IgnoreCase);
                    var match = rx.Match(file.Name);

                    if(match.Success)
                    {
                        var fmf = file.FullName.Replace(".json", "_fmf.json");
                        var obs = file.FullName.Replace(".json", "_obs.json");
                        if(!File.Exists(fmf) || !File.Exists(obs))
                            continue;

                        var id = int.Parse(match.Groups["id"].Value);
                        LoadMad(file.FullName, id);
                        LoadFmf(fmf, id);
                        LoadObs(obs, id);
                    }
                }
            }
            catch(Exception e)
            {
                throw new Exception("Error iterating map folder: " + e.Message);
            }

            MapListBox.ClearSelected();
            MapListBox.Items.Clear();

            MapNameTextBox.Clear();
            MapFilenameTextBox.Clear();
            ClearEncounter();

            fmf_data_ = null;
            obs_data_ = null;
            map_layers_ = new byte[13][];
            selected_map_ = -1;

            foreach(var map in maps_)
            {
                MapListBox.Items.Add(map.location_name);
            }
        }

        private void SaveMAD(MadJson mad)
        {
            try
            {
                var ser = new DataContractJsonSerializer(typeof(MadJson));
                var s = new MemoryStream();
                ser.WriteObject(s, mad);
                File.WriteAllBytes(mad.filepath, s.ToArray());
            }
            catch(Exception e)
            {
                throw new Exception("Error saving mad: " + mad.filepath + "\r\n" + e.Message);
            }
        }

        private void SaveFMF(FmfJson fmf)
        {
            try
            {
                var ser = new DataContractJsonSerializer(typeof(FmfJson));
                var s = new MemoryStream();
                ser.WriteObject(s, fmf);
                File.WriteAllBytes(fmf.filepath, s.ToArray());
            }
            catch(Exception e)
            {
                throw new Exception("Error saving fmf: " + fmf.filepath + "\r\n" + e.Message);
            }
        }

        private void SaveOBS(ObsJson obs)
        {
            try
            {
                var ser = new DataContractJsonSerializer(typeof(ObsJson));
                var s = new MemoryStream();
                ser.WriteObject(s, obs);
                File.WriteAllBytes(obs.filepath, s.ToArray());
            }
            catch(Exception e)
            {
                throw new Exception("Error saving obs: " + obs.filepath + "\r\n" + e.Message);
            }
        }

        private void SaveMaps()
        {
            if(selected_map_ >= 0)
            {
                for(var i = 0; i < 13; ++i)
                    fmf_data_.layers[i] = Convert.ToBase64String(map_layers_[i]);

                fmfs_[selected_map_] = fmf_data_;
                obss_[selected_map_] = obs_data_;
            }

            foreach(var mad in maps_)
            {
                SaveMAD(mad);
            }

            foreach(var fmf in fmfs_)
            {
                SaveFMF(fmf);
            }

            foreach(var obs in obss_)
            {
                SaveOBS(obs);
            }
        }

        private void SelectMap(int index)
        {
            if((index < 0) || (index >= maps_.Count))
                return;

            if(selected_map_ >= 0)
            {
                for(var i = 0; i < 13; ++i)
                    fmf_data_.layers[i] = Convert.ToBase64String(map_layers_[i]);

                fmfs_[selected_map_] = fmf_data_;
                obss_[selected_map_] = obs_data_;
            }

            selected_map_ = index;
            fmf_data_ = fmfs_[index];
            obs_data_ = obss_[index];

            MapListBox.SelectedIndexChanged -= MapListBox_SelectedIndexChanged;
            MapDesignCB.SelectedIndexChanged -= MapDesignCB_SelectedIndexChanged;
            EventMapCB.SelectedIndexChanged -= EventMapCB_SelectedIndexChanged;
            MapListBox.SelectedIndex = index;
            MapDesignCB.SelectedIndex = index;
            EventMapCB.SelectedIndex = index;
            MapListBox.SelectedIndexChanged += MapListBox_SelectedIndexChanged;
            MapDesignCB.SelectedIndexChanged += MapDesignCB_SelectedIndexChanged;
            EventMapCB.SelectedIndexChanged += EventMapCB_SelectedIndexChanged;

            RefreshMad();
            RefreshFmf();
            RefreshEvent();
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

        private void RefreshMad()
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
                MapBackgroundSC.Value = map.battle_background;
                MapMusicSC.Value = map.overworld_theme;
                MapWeatherCB.SelectedIndex = (int)map.weather;
                MapEncounterTypeCB.SelectedIndex = (int)map.encounter_type;
                ForbidBikeCB.Checked = map.forbid_bike > 0;
                RefreshEncounter();
            }
        }

        private void MapListBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            var index = MapListBox.SelectedIndex;
            if(index >= 0)
                SelectMap(index);
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

            uint total = 0;
            if(encounterindex < 10 && encounterindex < maps_[mapindex].normal_encounters.Length)
            {
                maps_[mapindex].normal_encounters[encounterindex].weight = (uint)val;

                for(var i = 0; i < 10; ++i)
                {
                    if(maps_[mapindex].normal_encounters[i].id != 0)
                        total += maps_[mapindex].normal_encounters[i].weight;
                }
            }
            else if((encounterindex - 10) < maps_[mapindex].special_encounters.Length)
            {
                maps_[mapindex].special_encounters[encounterindex - 10].weight = (uint)val;

                for(var i = 0; i < 5; ++i)
                {
                    if(maps_[mapindex].special_encounters[i].id != 0)
                        total += maps_[mapindex].special_encounters[i].weight;
                }
            }

            if(total > 0)
            {
                var p = ((double)val / (double)total) * 100.0;
                string str = "(" + p.ToString("0.##") + "%)";
                MapPercentLabel.Text = str;
            }
            else
            {
                MapPercentLabel.Text = "(0%)";
            }
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

        private void MapBackgroundSC_ValueChanged(object sender, EventArgs e)
        {
            var mapindex = MapListBox.SelectedIndex;
            if(mapindex < 0)
                return;

            maps_[mapindex].battle_background = (uint)MapBackgroundSC.Value;
        }

        private void MapMusicSC_ValueChanged(object sender, EventArgs e)
        {
            var mapindex = MapListBox.SelectedIndex;
            if(mapindex < 0)
                return;

            maps_[mapindex].overworld_theme = (uint)MapMusicSC.Value;
        }

        private void MapWeatherCB_SelectedIndexChanged(object sender, EventArgs e)
        {
            var mapindex = MapListBox.SelectedIndex;
            var index = MapWeatherCB.SelectedIndex;
            if(mapindex < 0 || index < 0)
                return;

            maps_[mapindex].weather = (uint)index;
        }

        private void MapEncounterTypeCB_SelectedIndexChanged(object sender, EventArgs e)
        {
            var mapindex = MapListBox.SelectedIndex;
            var index = MapEncounterTypeCB.SelectedIndex;
            if(mapindex < 0 || index < 0)
                return;

            maps_[mapindex].encounter_type = (uint)index;
        }

        private void ForbidBikeCB_CheckedChanged(object sender, EventArgs e)
        {
            var mapindex = MapListBox.SelectedIndex;
            if(mapindex < 0)
                return;

            maps_[mapindex].forbid_bike = (ForbidBikeCB.Checked ? 1u : 0u);
        }

        // Avert thy gaze
        private void NewMapBT_Click(object sender, EventArgs e)
        {
            if((maps_.Count == 0) || string.IsNullOrEmpty(working_dir_))
            {
                ErrMsg("No data loaded!");
                return;
            }

            using(var dialog = new NewMapDialog(map_names_.Length - 1))
            {
                if(dialog.ShowDialog() == DialogResult.OK)
                {
                    var madjson = new MadJson();
                    var fmfjson = new FmfJson();
                    var obsjson = new ObsJson();
                    var id = dialog.MapID;
                    var name = dialog.MapName;
                    var w = dialog.MapW;
                    var h = dialog.MapH;
                    var layer_sz = w * h * 2;

                    var idstr = id.ToString("D3");
                    var dir = is_ynk_ ? @"\gn_dat5.arc\map\data\" : @"\gn_dat3.arc\map\data\";
                    dir = working_dir_ + dir + idstr + @"\";

                    madjson.id = id;
                    madjson.filepath = dir + idstr + ".json";
                    var madpath = dir + idstr + ".mad";
                    fmfjson.id = id;
                    fmfjson.filepath = dir + idstr + "_fmf.json";
                    var fmfpath = dir + idstr + ".fmf";
                    obsjson.id = id;
                    obsjson.filepath = dir + idstr + "_obs.json";
                    var obspath = dir + idstr + ".obs";

                    if(File.Exists(madpath))
                    {
                        ErrMsg("File already exists: " + madpath);
                        return;
                    }
                    if(File.Exists(fmfpath))
                    {
                        ErrMsg("File already exists: " + fmfpath);
                        return;
                    }
                    if(File.Exists(obspath))
                    {
                        ErrMsg("File already exists: " + obspath);
                        return;
                    }
                    if(!Directory.Exists(dir))
                    {
                        try
                        {
                            Directory.CreateDirectory(dir);
                        }
                        catch
                        {
                            ErrMsg("Failed to create directory: " + dir);
                            return;
                        }
                    }

                    // FMF
                    fmfjson.width = (uint)w;
                    fmfjson.height = (uint)h;
                    fmfjson.num_layers = 13;
                    fmfjson.payload_length = (uint)layer_sz * 13;
                    fmfjson.unknown_1 = 0x20;
                    fmfjson.unknown_2 = 0x20;
                    fmfjson.unknown_3 = 0x10;

                    var fmf_sz = fmfjson.payload_length + 20;
                    var fmfdata = new byte[fmf_sz];
                    Array.Copy(new byte[] { (byte)'F', (byte)'M', (byte)'F', (byte)'_' }, fmfdata, 4);
                    Array.Copy(BitConverter.GetBytes(fmfjson.payload_length), 0, fmfdata, 4, 4);
                    Array.Copy(BitConverter.GetBytes(fmfjson.width), 0, fmfdata, 8, 4);
                    Array.Copy(BitConverter.GetBytes(fmfjson.height), 0, fmfdata, 0x0C, 4);
                    fmfdata[0x10] = 0x20;
                    fmfdata[0x11] = 0x20;
                    fmfdata[0x12] = 13;
                    fmfdata[0x13] = 0x10;
                    for(var i = 0; i < 13; ++i)
                    {
                        fmfjson.layers[i] = Convert.ToBase64String(fmfdata, 20, layer_sz);
                    }
                    File.WriteAllBytes(fmfpath, fmfdata);
                    SaveFMF(fmfjson);

                    // OBS
                    obsjson.entries = new ObsEntry[1024];
                    for(var i = 0; i < 1024; ++i)
                    {
                        var entry = new ObsEntry();
                        entry.index = (uint)i;
                        entry.event_index = (uint)i;
                        entry.movement_delay = 255;
                        entry.flags[0] = 1;
                        entry.flags[1] = 1;
                        obsjson.entries[i] = entry;
                    }
                    File.WriteAllBytes(obspath, new byte[1024 * 20]);
                    SaveOBS(obsjson);

                    // MAD
                    madjson.location_name = name;
                    for(var i = 0; i < 10; ++i)
                    {
                        madjson.normal_encounters[i] = new MadEncounter();
                        if(i < 5)
                            madjson.special_encounters[i] = new MadEncounter();
                    }
                    File.WriteAllBytes(madpath, new byte[0x8B]);
                    SaveMAD(madjson);

                    if(id < map_names_.Length)
                        map_names_[id] = name;
                    maps_.Add(madjson);
                    fmfs_.Add(fmfjson);
                    obss_.Add(obsjson);

                    MapListBox.Items.Add(name);
                    MapDesignCB.Items.Add(name);
                    EventMapCB.Items.Add(name);

                    SelectMap(maps_.Count - 1);
                }
            }
        }
    }
}
