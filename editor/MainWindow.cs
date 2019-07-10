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

// Files tab and data members of MainWindow

namespace editor
{
    public partial class EditorMainWindow : Form
    {
        private Process proc_;
        private delegate void ConOutDelegate(string msg);
        private bool is_ynk_ = false;
        private List<MadJson> maps_ = new List<MadJson>();
        private Dictionary<uint, DollData> puppets_ = new Dictionary<uint, DollData>();
        private Dictionary<uint, string> ability_names_ = new Dictionary<uint, string>();
        private Dictionary<uint, string> skill_names_ = new Dictionary<uint, string>();
        private Dictionary<uint, string> item_names_ = new Dictionary<uint, string>();
        private Dictionary<uint, string> skillcard_names_ = new Dictionary<uint, string>();
        private string[] puppet_names_ = new string[1] { "" };
        private string[] map_names_ = new string[1] { "" };
        private string working_dir_;

        private void Reset()
        {
            is_ynk_ = false;
            maps_ = new List<MadJson>();
            puppets_ = new Dictionary<uint, DollData>();
            puppet_names_ = new string[1] { "" };
            map_names_ = new string[1] { "" };
            working_dir_ = null;
        }

        public EditorMainWindow()
        {
            InitializeComponent();

            // detect game folder location
            var appdata_path = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData);
            var path = appdata_path + "/FocasLens/幻想人形演舞-ユメノカケラ-/install.ini";
            if(!File.Exists(path))
            {
                path = appdata_path + "/FocasLens/幻想人形演舞/gn_enbu.ini";
                if(!File.Exists(path))
                    return;
            }

            try
            {
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
        }

        private void ErrMsg(string msg)
        {
            MessageBox.Show(msg, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
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
            proc_.EnableRaisingEvents = true;
            proc_.OutputDataReceived += new DataReceivedEventHandler(ConsoleOutputHandler);
            proc_.ErrorDataReceived += new DataReceivedEventHandler(ConsoleErrHandler);
            proc_.Exited += new EventHandler(OnConsoleAppExit);
            proc_.SynchronizingObject = this;

            try
            {
                proc_.Start();
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

            if(!Directory.Exists(GameDirTextBox.Text))
            {
                ErrMsg("Invalid game folder path");
                return;
            }

            if(!Directory.Exists(WorkingDirTextBox.Text))
            {
                ErrMsg("Invalid working directory path");
                return;
            }

            String args = "-i \"" + GameDirTextBox.Text + "\" -o \"" + WorkingDirTextBox.Text + "\" --extract";

            if(RunConsoleCmd(app, args))
                EnableConsoleButtons(false);
        }

        private void ConvertButton_Click(object sender, EventArgs e)
        {
            string app = "binedit.exe";

            if(!Directory.Exists(WorkingDirTextBox.Text))
            {
                ErrMsg("Invalid working directory path");
                return;
            }

            String args = "-i \"" + WorkingDirTextBox.Text + "\" --convert";

            if(RunConsoleCmd(app, args))
                EnableConsoleButtons(false);
        }

        private void ApplyButton_Click(object sender, EventArgs e)
        {
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
                if(MessageBox.Show("Working directory has changed since the data was loaded, this probably won't work as expected.\r\nContinue?", "Warning", MessageBoxButtons.YesNo, MessageBoxIcon.Warning) != DialogResult.Yes)
                    return;
                working_dir_ = WorkingDirTextBox.Text;
            }

            string app = "binedit.exe";
            var wkdir = working_dir_;

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

            String args = "-i \"" + wkdir + "\" --patch";

            if(RunConsoleCmd(app, args))
                EnableConsoleButtons(false);
        }

        private void RepackButton_Click(object sender, EventArgs e)
        {
            string app = "diffgen.exe";

            if(!Directory.Exists(GameDirTextBox.Text))
            {
                ErrMsg("Invalid game folder path");
                return;
            }

            if(!Directory.Exists(WorkingDirTextBox.Text))
            {
                ErrMsg("Invalid working directory path");
                return;
            }

            String args = "-i \"" + GameDirTextBox.Text + "\" -o \"" + WorkingDirTextBox.Text + "\" --repack";

            if(RunConsoleCmd(app, args))
                EnableConsoleButtons(false);
        }

        private void DiffButton_Click(object sender, EventArgs e)
        {
            string app = "diffgen.exe";

            if(!Directory.Exists(GameDirTextBox.Text))
            {
                ErrMsg("Invalid game folder path");
                return;
            }

            if(!Directory.Exists(WorkingDirTextBox.Text))
            {
                ErrMsg("Invalid working directory path");
                return;
            }

            if(DiffFileDialog.ShowDialog() != DialogResult.OK)
                return;

            var path = DiffFileDialog.FileName;
            if(String.IsNullOrEmpty(path))
            {
                ErrMsg("Invalid diff file path");
                return;
            }

            String args = "-i \"" + GameDirTextBox.Text + "\" -o \"" + WorkingDirTextBox.Text + "\" --diff=\"" + path + "\"";

            if(RunConsoleCmd(app, args))
                EnableConsoleButtons(false);
        }

        private void LoadButton_Click(object sender, EventArgs e)
        {
            ConsoleOutput.Clear();
            ConsoleOutput.AppendText("Loading...\r\n");
            ConsoleOutput.Refresh();

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
                    return;
                }
            }

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
            string skl = wkdir + (is_ynk_ ? "/gn_dat6.arc/doll/SkillData.csv" : "/gn_dat3.arc/doll/skill/SkillData.csv");
            try
            {
                var skills = File.ReadAllLines(skl, Encoding.GetEncoding(932));
                uint count = 0;
                foreach(var skill in skills)
                {
                    var fields = skill.Split(',');
                    if(fields.Length < 2)
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
                    var name = fields[1];

                    if(id == 0 || string.IsNullOrEmpty(name))
                        continue;

                    if(name.Contains("ＳＫＩＬＬ０") || name.Contains("ＳＫＩＬＬ１"))
                        continue;

                    skill_names_[id] = name;
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
                        var index = (is_ynk_ ? 11 : 10);
                        if(fields.Length <= index)
                            throw new Exception("ItemData.csv has too few fields!");
                        skillcard_names_[id] = fields[index];
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
                ErrMsg("Failed to load puppets: " + ex.Message);
                Reset();
                return;
            }

            ConsoleOutput.AppendText("Done.\r\n");
            working_dir_ = wkdir;
        }
    }
}
