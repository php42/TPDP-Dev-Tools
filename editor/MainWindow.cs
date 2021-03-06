﻿using System;
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

// Files tab and data members of MainWindow

namespace editor
{
    public partial class EditorMainWindow : Form
    {
        private Process proc_;
        private delegate void ConOutDelegate(string msg);
        private bool is_ynk_ = false;
        private List<MadJson> maps_ = new List<MadJson>();
        private List<DodJson> dods_ = new List<DodJson>();
        private Dictionary<uint, DollData> puppets_ = new Dictionary<uint, DollData>();
        private Dictionary<uint, SkillData> skills_ = new Dictionary<uint, SkillData>();
        private Dictionary<uint, string> ability_names_ = new Dictionary<uint, string>();
        private Dictionary<uint, string> skill_names_ = new Dictionary<uint, string>();
        private Dictionary<uint, string> skill_descs_ = new Dictionary<uint, string>();
        private Dictionary<uint, string> item_names_ = new Dictionary<uint, string>();
        private Dictionary<uint, string> skillcard_names_ = new Dictionary<uint, string>();
        private string[] puppet_names_ = new string[1] { "" };
        private string[] map_names_ = new string[1] { "" };
        private string working_dir_;
        private SettingsJson cfg_ = new SettingsJson();
        private byte[] event_flags_;
        private byte[] obj_flags_;
        private byte[] puppet_flags_;
        private Dictionary<uint, string> bgm_data_ = new Dictionary<uint, string>();

        private void Reset()
        {
            is_ynk_ = false;
            maps_ = new List<MadJson>();
            dods_ = new List<DodJson>();
            puppets_ = new Dictionary<uint, DollData>();
            skills_ = new Dictionary<uint, SkillData>();
            ability_names_ = new Dictionary<uint, string>();
            skill_names_ = new Dictionary<uint, string>();
            skill_descs_ = new Dictionary<uint, string>();
            item_names_ = new Dictionary<uint, string>();
            skillcard_names_ = new Dictionary<uint, string>();
            puppet_names_ = new string[1] { "" };
            map_names_ = new string[1] { "" };
            working_dir_ = null;
            event_flags_ = null;
            obj_flags_ = null;
            puppet_flags_ = null;
            bgm_data_ = new Dictionary<uint, string>();
        }

        public EditorMainWindow()
        {
            InitializeComponent();

            InitDesign();

            try
            {
                // detect game folder location
                var appdata_path = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData);
                var path = appdata_path + "/FocasLens/幻想人形演舞-ユメノカケラ-/install.ini";
                if(!File.Exists(path))
                {
                    path = appdata_path + "/FocasLens/幻想人形演舞/gn_enbu.ini";
                }

                var str = File.ReadAllText(path, Encoding.GetEncoding(932));
                if(String.IsNullOrEmpty(str))
                    return;
                var pos = str.IndexOf("InstallPath=");
                if(pos < 0)
                    return;
                pos += "InstallPath=".Length;
                var endpos = str.IndexOf("\r\n", pos);
                if(endpos < 0)
                    endpos = str.Length;
                GameDirTextBox.Text = str.Substring(pos, endpos - pos);
            }
            catch
            {
                GameDirTextBox.Text = "";
            }

            ReadCfg();

            if(!string.IsNullOrEmpty(cfg_.game_dir))
                GameDirTextBox.Text = cfg_.game_dir;
            if(!string.IsNullOrEmpty(cfg_.working_dir))
                WorkingDirTextBox.Text = cfg_.working_dir;
            if(!cfg_.map_popup)
                TabControl.SelectedIndexChanged -= TabControl_SelectedIndexChanged;
        }

        private void ReadCfg()
        {
            try
            {
                var buf = File.ReadAllBytes("config.json");
                var ser = new DataContractJsonSerializer(typeof(SettingsJson));
                var s = new MemoryStream(buf);
                cfg_ = (SettingsJson)ser.ReadObject(s);
            }
            catch
            {
                cfg_ = new SettingsJson();
            }
        }

        private void WriteCfg()
        {
            try
            {
                var ser = new DataContractJsonSerializer(typeof(SettingsJson));
                var s = new MemoryStream();
                ser.WriteObject(s, cfg_);
                File.WriteAllBytes("config.json", s.ToArray());
            }
            catch
            {
                // ...
            }
        }

        private void ErrMsg(string msg)
        {
            MessageBox.Show(this, msg, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }

        private void EnableConsoleButtons(bool enable)
        {
            ExtractButton.Enabled = enable;
            ConvertButton.Enabled = enable;
            LoadButton.Enabled = enable;
            ApplyButton.Enabled = enable;
            RepackButton.Enabled = enable;
            DiffButton.Enabled = enable;
        }

        private void ConsoleOutputHandler(object proc, DataReceivedEventArgs data)
        {
            if(!String.IsNullOrEmpty(data.Data))
            {
                if(ConsoleOutput.InvokeRequired)
                    ConsoleOutput.Invoke((ConOutDelegate)(msg => ConsoleOutput.AppendText(msg)), new object[] { data.Data + "\r\n" });
                else
                    ConsoleOutput.AppendText(data.Data + "\r\n");
            }
        }

        private void AppendConsoleErr(string msg)
        {
            ConsoleOutput.SelectionStart = ConsoleOutput.TextLength;
            ConsoleOutput.SelectionLength = 0;
            ConsoleOutput.SelectionColor = Color.Red;
            ConsoleOutput.AppendText(msg);
            ConsoleOutput.SelectionColor = ConsoleOutput.ForeColor;
        }

        private void ConsoleErrHandler(object proc, DataReceivedEventArgs data)
        {
            if(!String.IsNullOrEmpty(data.Data))
            {
                if(ConsoleOutput.InvokeRequired)
                {
                    ConsoleOutput.Invoke(new ConOutDelegate(AppendConsoleErr), new object[] { data.Data + "\r\n" });
                }
                else
                {
                    AppendConsoleErr(data.Data + "\r\n");
                }
            }
        }

        private void OnConsoleAppExit(Object source, EventArgs e)
        {
            if(proc_.HasExited)
            {
                var ecode = proc_.ExitCode;
                if((ecode != 0) && (ecode != 1))
                    AppendConsoleErr("Failed with exit code: " + ((uint)ecode).ToString("X"));
            }

            proc_.Close();
            proc_ = null;
            EnableConsoleButtons(true);
        }

        private bool RunConsoleCmd(string app, string args)
        {
            if(proc_ != null)
            {
                ErrMsg("Please wait for the current operation to complete.");
                return false;
            }

            if(!File.Exists(app))
            {
                ErrMsg("Could not find " + app + ", please make sure it is in the same folder as this program");
                return false;
            }

            ConsoleOutput.Clear();
            ConsoleOutput.ForeColor = Color.Black;
            ConsoleOutput.AppendText(app + " " + args + "\r\n");

            proc_ = new Process();
            proc_.StartInfo.FileName = app;
            proc_.StartInfo.Arguments = args;
            proc_.StartInfo.UseShellExecute = false;
            proc_.StartInfo.RedirectStandardOutput = true;
            proc_.StartInfo.RedirectStandardError = true;
            proc_.StartInfo.CreateNoWindow = true;
            proc_.StartInfo.StandardErrorEncoding = Encoding.UTF8;
            proc_.StartInfo.StandardOutputEncoding = Encoding.UTF8;
            proc_.EnableRaisingEvents = true;
            proc_.OutputDataReceived += new DataReceivedEventHandler(ConsoleOutputHandler);
            proc_.ErrorDataReceived += new DataReceivedEventHandler(ConsoleErrHandler);
            proc_.Exited += new EventHandler(OnConsoleAppExit);
            proc_.SynchronizingObject = this;

            try
            {
                if(!proc_.Start())
                    throw new Exception("Failed to run progam: " + app);
                proc_.BeginOutputReadLine();
                proc_.BeginErrorReadLine();
            }
            catch(Exception ex)
            {
                ErrMsg(ex.Message);
                proc_.Close();
                proc_ = null;
                return false;
            }

            return true;
        }

        private void GameDirButton_Click(object sender, EventArgs e)
        {
            var result = GameDirBrowser.ShowDialog();
            
            if(result == DialogResult.OK)
            {
                GameDirTextBox.Text = GameDirBrowser.SelectedPath;
            }
        }

        private void WorkingDirButton_Click(object sender, EventArgs e)
        {
            var result = WorkingDirBrowser.ShowDialog();

            if(result == DialogResult.OK)
            {
                WorkingDirTextBox.Text = WorkingDirBrowser.SelectedPath;
            }
        }

        private void ExtractButton_Click(object sender, EventArgs e)
        {
            string app = "diffgen.exe";

            GameDirTextBox.Text = GameDirTextBox.Text.TrimEnd("/\\".ToCharArray()); // Remove trailing slashes
            if(!Directory.Exists(GameDirTextBox.Text))
            {
                ErrMsg("Invalid game folder path");
                return;
            }

            WorkingDirTextBox.Text = WorkingDirTextBox.Text.TrimEnd("/\\".ToCharArray()); // Remove trailing slashes
            if(!Directory.Exists(WorkingDirTextBox.Text))
            {
                ErrMsg("Invalid working directory path");
                return;
            }

            String args = "-i \"" + GameDirTextBox.Text + "\" -o \"" + WorkingDirTextBox.Text + "\" --extract";

            if(RunConsoleCmd(app, args))
                EnableConsoleButtons(false);

            cfg_.game_dir = GameDirTextBox.Text;
            cfg_.working_dir = WorkingDirTextBox.Text;
            WriteCfg();
        }

        private void ConvertButton_Click(object sender, EventArgs e)
        {
            string app = "binedit.exe";

            WorkingDirTextBox.Text = WorkingDirTextBox.Text.TrimEnd("/\\".ToCharArray()); // Remove trailing slashes
            if(!Directory.Exists(WorkingDirTextBox.Text))
            {
                ErrMsg("Invalid working directory path");
                return;
            }

            String args = "-i \"" + WorkingDirTextBox.Text + "\" --convert";

            if(RunConsoleCmd(app, args))
                EnableConsoleButtons(false);

            cfg_.game_dir = GameDirTextBox.Text;
            cfg_.working_dir = WorkingDirTextBox.Text;
            WriteCfg();
        }

        private void ApplyButton_Click(object sender, EventArgs e)
        {
            WorkingDirTextBox.Text = WorkingDirTextBox.Text.TrimEnd("/\\".ToCharArray()); // Remove trailing slashes
            if(!Directory.Exists(WorkingDirTextBox.Text))
            {
                ErrMsg("Invalid working directory path");
                return;
            }

            if(working_dir_ == null)
            {
                ErrMsg("Please load a working directory first.");
                return;
            }

            if(working_dir_ != WorkingDirTextBox.Text)
            {
                if(MessageBox.Show(this, "Working directory has changed since the data was loaded, this probably won't work as expected.\r\nContinue?", "Warning", MessageBoxButtons.YesNo, MessageBoxIcon.Warning) != DialogResult.Yes)
                    return;
                working_dir_ = WorkingDirTextBox.Text;
            }

            string app = "binedit.exe";
            var wkdir = working_dir_;

            string pflags = wkdir + (is_ynk_ ? "/gn_dat6.arc/dollImage/img/DollMainGLoadFlag.bin" : "/gn_dat3.arc/doll/img/DollMainGLoadFlag.bin");
            string oflags = wkdir + (is_ynk_ ? "/gn_dat5.arc/map/obj/ObjGLoadFlag.bin" : "/gn_dat3.arc/map/obj/ObjGLoadFlag.bin");
            string eflags = wkdir + (is_ynk_ ? "/gn_dat5.arc/map/obj/ObjFirstActivity.bin" : "/gn_dat3.arc/map/obj/ObjFirstActivity.bin");

            try
            {
                File.WriteAllBytes(pflags, puppet_flags_);
                File.WriteAllBytes(eflags, event_flags_);
                File.WriteAllBytes(oflags, obj_flags_);
            }
            catch(Exception ex)
            {
                ErrMsg("Error: " + ex.Message);
                return;
            }

            /*
            // Save puppet names
            string puppetnames = wkdir + (is_ynk_ ? "/gn_dat5.arc/name/DollName.csv" : "/gn_dat3.arc/name/DollName.csv");
            try
            {
                File.WriteAllLines(puppetnames, puppet_names_, Encoding.GetEncoding(932));
            }
            catch(Exception ex)
            {
                ErrMsg("Error writing file: " + puppetnames + "\r\n" + ex.Message);
                return;
            }
            */

            // Save map names
            string mapnames = wkdir + (is_ynk_ ? "/gn_dat5.arc/name/MapName.csv" : "/gn_dat3.arc/name/MapName.csv");
            try
            {
                File.WriteAllLines(mapnames, map_names_, Encoding.GetEncoding(932));
            }
            catch(Exception ex)
            {
                ErrMsg("Error writing file: " + mapnames + "\r\n" + ex.Message);
                return;
            }

            // Save puppets
            string dolldata = wkdir + (is_ynk_ ? "/gn_dat6.arc/doll/DollData.json" : "/gn_dat3.arc/doll/DollData.json");
            try
            {
                DataContractJsonSerializer ser = new DataContractJsonSerializer(typeof(DollDataJson));
                MemoryStream s = new MemoryStream();
                DollDataJson j = new DollDataJson();
                j.puppets = new DollData[puppets_.Count];
                var i = 0;
                foreach(var it in puppets_)
                {
                    j.puppets[i++] = it.Value;
                }
                ser.WriteObject(s, j);
                File.WriteAllBytes(dolldata, s.ToArray());
            }
            catch(Exception ex)
            {
                ErrMsg("Error writing file: " + dolldata + "\r\n" + ex.Message);
                return;
            }

            // Save .mad files
            //string mapdir = wkdir + (is_ynk_ ? "/gn_dat5.arc/map/data" : "/gn_dat3.arc/map/data");
            try
            {
                SaveMaps();
            }
            catch(Exception ex)
            {
                ErrMsg("Failed to save maps: " + ex.Message);
                return;
            }

            // Save Trainers
            try
            {
                SaveTrainers();
            }
            catch(Exception ex)
            {
                ErrMsg("Failed to save trainers: " + ex.Message);
                return;
            }

            // Save Skills
            try
            {
                string skills = wkdir + (is_ynk_ ? "/gn_dat6.arc/doll/SkillData.json" : "/gn_dat3.arc/doll/skill/SkillData.json");
                SaveSkills(skills);
            }
            catch(Exception ex)
            {
                ErrMsg("Failed to save skills: " + ex.Message);
                return;
            }

            // Save Skill names and descriptions
            string skl = wkdir + (is_ynk_ ? "/gn_dat6.arc/doll/SkillData.csv" : "/gn_dat3.arc/doll/skill/SkillData.csv");
            try
            {
                var skills = File.ReadAllLines(skl, Encoding.GetEncoding(932));
                bool dirty = false;
                for(var i = 0; i < skills.Length; ++i)
                {
                    var fields = skills[i].Split(new char[] { ',' }, is_ynk_ ? 4 : 2);
                    if(fields.Length < (is_ynk_ ? 4 : 2))
                        continue;

                    uint id;
                    try
                    {
                        id = is_ynk_ ? uint.Parse(fields[0]) : (uint)i;
                    }
                    catch
                    {
                        continue;
                    }

                    if(id == 0 || !skills_.ContainsKey(id))
                        continue;

                    var name_index = is_ynk_ ? 1 : 0;
                    var desc_index = is_ynk_ ? 3 : 1;
                    var name = skill_names_[id];
                    var desc = skill_descs_[id];

                    if((fields[name_index] != name) || (fields[desc_index] != desc))
                    {
                        fields[name_index] = name;
                        fields[desc_index] = desc;
                        skills[i] = string.Join(",", fields);
                        dirty = true;
                    }
                }

                if(dirty)
                    File.WriteAllLines(skl, skills, Encoding.GetEncoding(932));
            }
            catch(Exception ex)
            {
                ErrMsg("Error reading file: " + skl + "\r\n" + ex.Message);
                return;
            }

            String args = "-i \"" + wkdir + "\" --patch";

            if(RunConsoleCmd(app, args))
                EnableConsoleButtons(false);

            cfg_.game_dir = GameDirTextBox.Text;
            cfg_.working_dir = WorkingDirTextBox.Text;
            WriteCfg();
        }

        private void RepackButton_Click(object sender, EventArgs e)
        {
            string app = "diffgen.exe";

            GameDirTextBox.Text = GameDirTextBox.Text.TrimEnd("/\\".ToCharArray()); // Remove trailing slashes
            if(!Directory.Exists(GameDirTextBox.Text))
            {
                ErrMsg("Invalid game folder path");
                return;
            }

            WorkingDirTextBox.Text = WorkingDirTextBox.Text.TrimEnd("/\\".ToCharArray()); // Remove trailing slashes
            if(!Directory.Exists(WorkingDirTextBox.Text))
            {
                ErrMsg("Invalid working directory path");
                return;
            }

            String args = "-i \"" + GameDirTextBox.Text + "\" -o \"" + WorkingDirTextBox.Text + "\" --repack";

            if(RunConsoleCmd(app, args))
                EnableConsoleButtons(false);

            cfg_.game_dir = GameDirTextBox.Text;
            cfg_.working_dir = WorkingDirTextBox.Text;
            WriteCfg();
        }

        private void DiffButton_Click(object sender, EventArgs e)
        {
            string app = "diffgen.exe";

            WorkingDirTextBox.Text = WorkingDirTextBox.Text.TrimEnd("/\\".ToCharArray()); // Remove trailing slashes
            if(!Directory.Exists(WorkingDirTextBox.Text))
            {
                ErrMsg("Invalid working directory path");
                return;
            }

            string path, dir;
            using(var dialog = new PatchDialog(GameDirTextBox.Text))
            {
                if(dialog.ShowDialog() != DialogResult.OK)
                    return;
                path = dialog.PatchPath.TrimEnd("/\\".ToCharArray());
                dir = dialog.DirPath.TrimEnd("/\\".ToCharArray());
            }

            if(String.IsNullOrEmpty(path))
            {
                ErrMsg("Invalid diff file path");
                return;
            }

            String args = "-i \"" + dir + "\" -o \"" + WorkingDirTextBox.Text + "\" --diff=\"" + path + "\"";

            if(RunConsoleCmd(app, args))
                EnableConsoleButtons(false);

            cfg_.working_dir = WorkingDirTextBox.Text;
            WriteCfg();
        }

        private void LoadButton_Click(object sender, EventArgs e)
        {
            ConsoleOutput.Clear();
            ConsoleOutput.AppendText("Loading...\r\n");
            ConsoleOutput.Refresh();
            Reset();

            WorkingDirTextBox.Text = WorkingDirTextBox.Text.TrimEnd("/\\".ToCharArray()); // Remove trailing slashes

            // Find DollData.json and determine if this is YnK or base TPDP data
            string wkdir = WorkingDirTextBox.Text;
            string dolldata = wkdir + "/gn_dat6.arc/doll/DollData.json";
            is_ynk_ = true;
            if(!File.Exists(dolldata))
            {
                dolldata = wkdir + "/gn_dat3.arc/doll/DollData.json";
                is_ynk_ = false;
                if(!File.Exists(dolldata))
                {
                    ErrMsg("Could not locate DollData.json");
                    ConsoleOutput.Clear();
                    return;
                }
            }

            string versiondata = wkdir + "/version.json";
            if(!File.Exists(versiondata))
            {
                ErrMsg("Seems like you're using an old JSON dump from an alpha build.\r\nPlease run convert to update the JSON.");
                ConsoleOutput.Clear();
                return;
            }

            try
            {
                DataContractJsonSerializer ser = new DataContractJsonSerializer(typeof(VersionJson));
                var buf = File.ReadAllBytes(versiondata);
                MemoryStream s = new MemoryStream(buf);
                var ver = (VersionJson)ser.ReadObject(s);
                if(!ver.VersionMatches())
                {
                    var major = VersionJson.VersionMajor;
                    var minor = VersionJson.VersionMinor;
                    bool newer = (ver.major > major) || (ver.major == major && (ver.minor > minor));
                    ErrMsg("Current JSON dump is for " + (newer ? "a newer" : "an older") + " version of TPDP-Dev-Tools.\r\nPlease run convert to update the JSON.");
                    ConsoleOutput.Clear();
                    return;
                }
            }
            catch(Exception ex)
            {
                ErrMsg("Error reading file: " + versiondata + "\r\n" + ex.Message);
                return;
            }

            working_dir_ = wkdir;

            string pflags = wkdir + (is_ynk_ ? "/gn_dat6.arc/dollImage/img/DollMainGLoadFlag.bin" : "/gn_dat3.arc/doll/img/DollMainGLoadFlag.bin");
            string oflags = wkdir + (is_ynk_ ? "/gn_dat5.arc/map/obj/ObjGLoadFlag.bin" : "/gn_dat3.arc/map/obj/ObjGLoadFlag.bin");
            string eflags = wkdir + (is_ynk_ ? "/gn_dat5.arc/map/obj/ObjFirstActivity.bin" : "/gn_dat3.arc/map/obj/ObjFirstActivity.bin");
            string bgm = wkdir + (is_ynk_ ? "/gn_dat4.arc/bgm/BgmData.csv" : "/gn_dat2.arc/sound/bgm/BgmData.csv");
            string[] bgmdata;
            try
            {
                puppet_flags_ = File.ReadAllBytes(pflags);
                obj_flags_ = File.ReadAllBytes(oflags);
                event_flags_ = File.ReadAllBytes(eflags);
                bgmdata = File.ReadAllLines(bgm, Encoding.GetEncoding(932));
            }
            catch(Exception ex)
            {
                ErrMsg("Error: " + ex.Message);
                Reset();
                return;
            }

            foreach(var line in bgmdata)
            {
                var fields = line.Split(',');
                uint id;
                try
                {
                    id = uint.Parse(fields[0]);
                }
                catch
                {
                    continue;
                }
                var name = fields[1] + ".ogg";

                if(!File.Exists(wkdir + (is_ynk_ ? "/gn_dat4.arc/bgm/" : "/gn_dat2.arc/sound/bgm/") + name))
                    continue;

                bgm_data_[id] = id.ToString() + ": " + name;
            }
            bgm_data_[0] = "None";

            // Parse puppets
            puppets_.Clear();
            try
            {
                DataContractJsonSerializer ser = new DataContractJsonSerializer(typeof(DollDataJson));
                var buf = File.ReadAllBytes(dolldata);
                MemoryStream s = new MemoryStream(buf);
                var tmp = (DollDataJson)ser.ReadObject(s);
                foreach(var puppet in tmp.puppets)
                {
                    puppets_[puppet.id] = puppet;
                }
            }
            catch(Exception ex)
            {
                ErrMsg("Error parsing file: " + dolldata + "\r\n" + ex.Message);
                return;
            }

            // Parse puppet names
            string puppetnames = wkdir + (is_ynk_ ? "/gn_dat5.arc/name/DollName.csv" : "/gn_dat3.arc/name/DollName.csv");
            try
            {
                puppet_names_ = File.ReadAllLines(puppetnames, Encoding.GetEncoding(932));
            }
            catch(Exception ex)
            {
                ErrMsg("Error reading file: " + puppetnames + "\r\n" + ex.Message);
                return;
            }

            // Parse map names
            string mapnames = wkdir + (is_ynk_ ? "/gn_dat5.arc/name/MapName.csv" : "/gn_dat3.arc/name/MapName.csv");
            try
            {
                map_names_ = File.ReadAllLines(mapnames, Encoding.GetEncoding(932));
            }
            catch(Exception ex)
            {
                ErrMsg("Error reading file: " + mapnames + "\r\n" + ex.Message);
                return;
            }

            // Parse .mad files
            string mapdir = wkdir + (is_ynk_ ? "/gn_dat5.arc/map/data" : "/gn_dat3.arc/map/data");
            try
            {
                LoadMaps(mapdir);
            }
            catch(Exception ex)
            {
                ErrMsg("Failed to load maps: " + ex.Message);
                Reset();
                return;
            }

            // Parse ability names
            ability_names_.Clear();
            string abl = wkdir + (is_ynk_ ? "/gn_dat6.arc/doll/AbilityData.csv" : "/gn_dat3.arc/doll/ability/AbilityData.csv");
            try
            {
                var abilities = File.ReadAllLines(abl, Encoding.GetEncoding(932));
                foreach(var ability in abilities)
                {
                    var fields = ability.Split(',');
                    if(fields.Length < 2)
                        continue;
                    var id = uint.Parse(fields[0]);
                    var name = fields[1];

                    if(id == 0 || string.IsNullOrEmpty(name))
                        continue;

                    if(name.Contains("NULL") || name.Contains("ＡＢＬ"))
                        continue;

                    ability_names_[id] = name;
                }
            }
            catch(Exception ex)
            {
                ErrMsg("Error reading file: " + abl + "\r\n" + ex.Message);
                return;
            }

            // Parse skill names
            skill_names_.Clear();
            skill_descs_.Clear();
            string skl = wkdir + (is_ynk_ ? "/gn_dat6.arc/doll/SkillData.csv" : "/gn_dat3.arc/doll/skill/SkillData.csv");
            try
            {
                var skills = File.ReadAllLines(skl, Encoding.GetEncoding(932));
                uint count = 0;
                foreach(var skill in skills)
                {
                    var fields = skill.Split(new char[] {','}, is_ynk_ ? 4 : 2);
                    if(fields.Length < (is_ynk_ ? 4 : 2))
                        continue;

                    uint id;
                    try
                    {
                        id = is_ynk_ ? uint.Parse(fields[0]) : count++;
                    }
                    catch
                    {
                        continue;
                    }

                    var name = is_ynk_ ? fields[1] : fields[0];
                    var desc = is_ynk_ ? fields[3] : fields[1];

                    if(id == 0 || string.IsNullOrEmpty(name))
                        continue;

                    if(name.Contains("ＳＫＩＬＬ０") || name.Contains("ＳＫＩＬＬ１"))
                        continue;

                    skill_names_[id] = name;
                    skill_descs_[id] = desc;
                }
            }
            catch(Exception ex)
            {
                ErrMsg("Error reading file: " + skl + "\r\n" + ex.Message);
                return;
            }

            // Parse item names
            item_names_.Clear();
            skillcard_names_.Clear();
            string itm = wkdir + (is_ynk_ ? "/gn_dat6.arc/item/ItemData.csv" : "/gn_dat3.arc/item/ItemData.csv");
            try
            {
                var items = File.ReadAllLines(itm, Encoding.GetEncoding(932));
                foreach(var item in items)
                {
                    var fields = item.Split(',');
                    if(fields.Length < 2)
                        continue;
                    uint id;
                    try
                    {
                        id = uint.Parse(fields[0]);
                    }
                    catch
                    {
                        continue;
                    }
                    var name = fields[1];

                    if(id == 0 || string.IsNullOrEmpty(name))
                        continue;

                    if(name.StartsWith("Item"))
                        continue;

                    // Skillcards
                    if(id >= 385 && id <= 512)
                    {
                        /*var index = (is_ynk_ ? 11 : 10);
                        if(fields.Length <= index)
                            throw new Exception("ItemData.csv has too few fields!");
                        var scname = fields[index];*/
                        var skillid = uint.Parse(fields[9]);

                        if((skillid > 0) && (skillid < 1023))
                        {
                            skillcard_names_[id] = skill_names_[skillid];
                            /*if(!skill_names_[skillid].Equals(scname, StringComparison.OrdinalIgnoreCase))
                            {
                                string msg = "Note: Skillcard name mismatch! Item \"" + scname + "\" actually teaches \"" + skill_names_[skillid] + "\"\r\n";
                                msg += skill_names_[skillid] + " will be displayed in the editor.\r\n";
                                ConsoleOutput.AppendText(msg);
                            }*/
                        }
                        else
                        {
                            //skillcard_names_[id] = scname;
                            skillcard_names_[id] = name;
                        }
                    }

                    item_names_[id] = name;
                }
            }
            catch(Exception ex)
            {
                ErrMsg("Error reading file: " + itm + "\r\n" + ex.Message);
                return;
            }

            // Populate Puppets tab
            try
            {
                LoadPuppets();
            }
            catch(Exception ex)
            {
                ErrMsg("Failed to load puppet data: " + ex.Message);
                Reset();
                return;
            }

            // Populate Trainers tab
            try
            {
                string dods = wkdir + (is_ynk_ ? "/gn_dat5.arc/script/dollOperator" : "/gn_dat3.arc/script/dollOperator");
                LoadTrainers(dods);
            }
            catch(Exception ex)
            {
                ErrMsg("Failed to load trainer data: " + ex.Message);
                Reset();
                return;
            }

            // Populate Skills tab
            try
            {
                string skills = wkdir + (is_ynk_ ? "/gn_dat6.arc/doll/SkillData.json" : "/gn_dat3.arc/doll/skill/SkillData.json");
                LoadSkills(skills);
            }
            catch(Exception ex)
            {
                ErrMsg("Failed to load skill data: " + ex.Message);
                Reset();
                return;
            }

            // Populate Design tab
            try
            {
                LoadDesign();
            }
            catch(Exception ex)
            {
                ErrMsg("Error initializing level designer: " + ex.Message);
                Reset();
                return;
            }

            // Populate Event tab
            try
            {
                LoadEvents();
            }
            catch(Exception ex)
            {
                ErrMsg("Error initializing event editor: " + ex.Message);
                Reset();
                return;
            }

            ConsoleOutput.AppendText("Done.\r\n");

            cfg_.game_dir = GameDirTextBox.Text;
            cfg_.working_dir = WorkingDirTextBox.Text;
            WriteCfg();
        }

        private void TabControl_SelectedIndexChanged(object sender, EventArgs e)
        {
            if(TabControl.SelectedIndex == 5)
            {
                string msg = "Right click to erase.\r\nCtrl + left click to select brush from active layer.\r\nShift + drag left to copy.\r\nAlt + left click to paste.\r\n\r\n";
                msg += "Please see \"MAP EDITOR README.txt\" in the docs folder for more information.";

                MessageBox.Show(this, msg, "Info", MessageBoxButtons.OK, MessageBoxIcon.Information);
                TabControl.SelectedIndexChanged -= TabControl_SelectedIndexChanged;
                cfg_.map_popup = false;
                WriteCfg();
            }
        }

        private void TabControl_KeyDown(object sender, KeyEventArgs e)
        {
            if(!map_display_.Focused && !map_panel_.Focused)
                return;

            if(e.KeyCode == Keys.Z)
            {
                if(e.Modifiers == Keys.Control)
                    Undo();
                else if(e.Modifiers == (Keys.Control | Keys.Shift))
                    Redo();
                return;
            }

            if((e.KeyCode == Keys.G) && (e.Modifiers == Keys.None))
            {
                MapGridCB.Checked = !MapGridCB.Checked;
                return;
            }
            if((e.KeyCode == Keys.L) && (e.Modifiers == Keys.None))
            {
                DesignLabelCB.Checked = !DesignLabelCB.Checked;
                return;
            }

            int index;
            switch(e.KeyCode)
            {
                case Keys.D1:
                    index = 0;
                    break;
                case Keys.D2:
                    index = 1;
                    break;
                case Keys.D3:
                    index = 2;
                    break;
                case Keys.D4:
                    index = 3;
                    break;
                case Keys.D5:
                    index = 4;
                    break;
                case Keys.D6:
                    index = 5;
                    break;
                case Keys.D7:
                    index = 6;
                    break;
                case Keys.D8:
                    index = 7;
                    break;
                default:
                    return;
            }

            if(e.Modifiers == Keys.None)
            {
                BrushLayerCB.SelectedIndex = index;
            }
            else if(e.Modifiers == Keys.Control)
            {
                bool c = LayerVisibiltyCB.CheckedIndices.Contains(index);
                LayerVisibiltyCB.SetItemChecked(index, !c);
            }
        }
    }
}
