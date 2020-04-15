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
using System.Threading.Tasks;

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

        // Don't look
        private Dictionary<int, string> battle_bgs_ = new Dictionary<int, string> {
            {0,"out11.png" },
            {1,"out21.png" },
            {2,"in1.png" },
            {3,"mori1.png" },
            {4,"dokutsu1.png" },
            {5,"sangaku1.png" },
            {6,"hakurei1.png" },
            {7,"muenzuka1.png" },
            {8,"mizuumi1.png" },
            {9,"korinodokutsu1.png" },
            {10,"komakan1.png" },
            {11,"tosyokan1.png" },
            {12,"myorenji1.png" },
            {13,"daisibyo1.png" },
            {14,"sinreibyo1.png" },
            {15,"chikurin1.png" },
            {16,"in1.png" },
            {18,"genbunosawa1.png" },
            {19,"sanzunokawa1.png" },
            {20,"tengunosato1.png" },
            {21,"moriyajinja1.png" },
            {23,"haison1.png" },
            {24,"haikyo1.png" },
            {25,"haiyokan1.png" },
            {27,"chiireiden1.png" },
            {28,"syakunetsu1.png" },
            {29,"kanketsusen1.png" },
            {30,"suzuran1.png" },
            {31,"taiyo1.png" },
            {32,"makai1.png" },
            {33,"pandemonium1.png" },
            {36,"tenkai1.png" },
            {38,"saisi1.png" },
            {39,"hitozato1.png" },
            {40,"myorenjiIn1.png" },
            {41,"boti1.png" },
            {42,"pandemonium20.png" },
            {43,"yousitu1.png" },
            {44,"hyosetu1.png" },
            {45,"genmukai1.png" },
            {46,"yumenosekai0.png" },
            {47,"nitori1.png" },
            {48,"kisinjo1.png" },
            {49,"kisinjogyaku1.png" },
            {50,"sinnreibyonaibu1.png" },
            {51,"saigyoayakasi1.png" },
            {52,"hakugyokuro1.png" },
            {53,"hakugyokurokaidan0.png" },
            {54,"jigoku1.png" },
            {55,"butokai1.png" },
            {56,"butokaichitei1.png" },
            {57,"mugenkan1.png" },
            {58,"tsukinomiyako1.png" },
            {59,"mugensekai1.png" },
            {60,"mugensekai20.png" }
        };

        private void LoadMad(string filepath, int id)
        {
            DataContractJsonSerializer ser = new DataContractJsonSerializer(typeof(MadJson));
            var buf = File.ReadAllBytes(filepath);
            var s = new MemoryStream(buf);
            var map = (MadJson)ser.ReadObject(s);

            map.id = id;
            map.filepath = filepath;

            if(map.normal_encounters.Length < 10)
            {
                var tmp = new MadEncounter[10];
                for(var i = 0; i < map.normal_encounters.Length; ++i)
                    tmp[i] = map.normal_encounters[i];
                map.normal_encounters = tmp;
            }
            if(map.special_encounters.Length < 5)
            {
                var tmp = new MadEncounter[5];
                for(var i = 0; i < map.special_encounters.Length; ++i)
                    tmp[i] = map.special_encounters[i];
                map.special_encounters = tmp;
            }

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

            MapMusicCB.SelectedIndex = -1;
            MapMusicCB.Items.Clear();
            MapMusicCB.DisplayMember = "Item1";
            MapMusicCB.ValueMember = "Item2";
            foreach(var bgm in bgm_data_)
            {
                MapMusicCB.Items.Add(new Tuple<string, uint>(bgm.Value, bgm.Key));
            }
        }

        private static void SaveMAD(MadJson mad)
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

        private static void SaveFMF(FmfJson fmf)
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

        private static void SaveOBS(ObsJson obs)
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

            Parallel.ForEach(maps_, mad => SaveMAD(mad));
            Parallel.ForEach(fmfs_, fmf => SaveFMF(fmf));
            Parallel.ForEach(obss_, obs => SaveOBS(obs));

            /*
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
            */
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

            if(index < 10)
            {
                encounter = map.normal_encounters[index];
            }
            else
            {
                encounter = map.special_encounters[index - 10];
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
                if(bgm_data_.ContainsKey(map.overworld_theme))
                    MapMusicCB.SelectedIndex = MapMusicCB.FindStringExact(bgm_data_[map.overworld_theme]);
                else
                {
                    MapMusicCB.SelectedIndex = -1;
                    ErrMsg("Unknown music value: " + map.overworld_theme.ToString());
                }
                MapWeatherCB.SelectedIndex = (int)map.weather;
                MapEncounterTypeCB.SelectedIndex = (int)map.encounter_type;
                ForbidBikeCB.Checked = map.forbid_bike > 0;
                MapIDSC.Value = map.id;
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
                bool special = encounterindex >= 10;
                if(special)
                    encounterindex -= 10;

                uint oldid;
                if(special)
                {
                    oldid = maps_[mapindex].special_encounters[encounterindex].id;
                    if(oldid != id || id == 0)
                        maps_[mapindex].special_encounters[encounterindex].style = 0;
                    maps_[mapindex].special_encounters[encounterindex].id = id;
                }
                else
                {
                    oldid = maps_[mapindex].normal_encounters[encounterindex].id;
                    if(oldid != id || id == 0)
                        maps_[mapindex].normal_encounters[encounterindex].style = 0;
                    maps_[mapindex].normal_encounters[encounterindex].id = id;
                }

                if(id == 0)
                {
                    MapLvlSpinCtrl.Value = 0;
                    MapWeightSpinCtrl.Value = 0;
                }
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

            if(encounterindex < 10)
                maps_[mapindex].normal_encounters[encounterindex].style = (uint)index;
            else
                maps_[mapindex].special_encounters[encounterindex - 10].style = (uint)index;
        }

        private void MapLvlSpinCtrl_ValueChanged(object sender, EventArgs e)
        {
            var val = MapLvlSpinCtrl.Value;
            var mapindex = MapListBox.SelectedIndex;
            var encounterindex = EncounterComboBox.SelectedIndex;
            if(val < 0 || mapindex < 0 || encounterindex < 0)
                return;

            if(encounterindex < 10)
                maps_[mapindex].normal_encounters[encounterindex].level = (uint)val;
            else
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
            if(encounterindex < 10)
            {
                maps_[mapindex].normal_encounters[encounterindex].weight = (uint)val;

                for(var i = 0; i < 10; ++i)
                {
                    if(maps_[mapindex].normal_encounters[i].id != 0)
                        total += maps_[mapindex].normal_encounters[i].weight;
                }
            }
            else
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

            var id = (uint)MapBackgroundSC.Value;
            maps_[mapindex].battle_background = id;

            if(battle_bgs_.ContainsKey((int)id))
            {
                try
                {
                    var path = working_dir_ + "/gn_dat1.arc/battle/locationBG/" + battle_bgs_[(int)id];
                    BattleBGPB.Image = new Bitmap(path);
                }
                catch
                {
                    BattleBGPB.Image = null;
                }
            }
            else
            {
                BattleBGPB.Image = null;
            }
        }

        private void MapMusicCB_SelectedIndexChanged(object sender, EventArgs e)
        {
            var mapindex = MapListBox.SelectedIndex;
            var index = MapMusicCB.SelectedIndex;
            if(index < 0 || mapindex < 0)
                return;

            var val = (Tuple<string, uint>)MapMusicCB.SelectedItem;
            var id = val.Item2;
            maps_[mapindex].overworld_theme = id;
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

                    for(var i = 0; i < 128; ++i)
                        event_flags_[(id * 128) + i] = 0xFF;

                    SelectMap(maps_.Count - 1);
                }
            }
        }
    }
}
