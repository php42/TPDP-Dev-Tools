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
using System.Threading;
using System.Threading.Tasks;

// Rental team tab of MainWindow
// FIXME: this is a lazy copy/paste of TrainerWindow.cs

namespace editor
{
    public partial class EditorMainWindow : Form
    {
        private Mutex pts_mtx = new Mutex();

        private void LoadPts(string filepath, int id)
        {
            DataContractJsonSerializer ser = new DataContractJsonSerializer(typeof(PtsJson));
            var buf = File.ReadAllBytes(filepath);
            var s = new MemoryStream(buf);

            var pts = (PtsJson)ser.ReadObject(s);
            pts.filepath = filepath;
            pts.id = id;

            pts_mtx.WaitOne();

            try
            {
                rentals_.Add(pts);
            }
            finally
            {
                pts_mtx.ReleaseMutex();
            }
        }

        private void LoadRentals(string dir)
        {
            DirectoryInfo di = new DirectoryInfo(dir);
            if(!di.Exists)
            {
                throw new Exception("Could not locate rental team folder");
            }

            rentals_.Clear();

            try
            {
                var files = di.GetFiles("*.json");
                Regex rx = new Regex(@"(?<id>\d{3})\.json$", RegexOptions.Compiled | RegexOptions.IgnoreCase);

                Parallel.ForEach(files, file => {
                    var match = rx.Match(file.FullName);

                    if(match.Success)
                        LoadPts(file.FullName, int.Parse(match.Groups["id"].Value));
                });
            }
            catch(Exception e)
            {
                throw new Exception("Error iterating rental team folder: " + e.Message);
            }

            rentals_.Sort((PtsJson x, PtsJson y) => { return x.id.CompareTo(y.id); });

            // Rentals
            RentalLB.SelectedIndex = -1;
            RentalLB.Items.Clear();
            foreach(var pts in rentals_)
            {
                RentalLB.Items.Add(pts.id.ToString("D3"));
            }

            // Items
            RentalItemCB.SelectedIndex = -1;
            RentalItemCB.Items.Clear();
            RentalItemCB.DisplayMember = "Item1";
            RentalItemCB.ValueMember = "Item2";
            RentalItemCB.Items.Add(new Tuple<string, uint>("None", 0));
            foreach(var it in item_names_)
            {
                var id = it.Key;
                var name = it.Value;
                RentalItemCB.Items.Add(new Tuple<string, uint>(name, id));
            }

            // Skills
            RentalSkill1CB.SelectedIndex = -1;
            RentalSkill1CB.Items.Clear();
            RentalSkill1CB.DisplayMember = "Item1";
            RentalSkill1CB.ValueMember = "Item2";
            RentalSkill2CB.SelectedIndex = -1;
            RentalSkill2CB.Items.Clear();
            RentalSkill2CB.DisplayMember = "Item1";
            RentalSkill2CB.ValueMember = "Item2";
            RentalSkill3CB.SelectedIndex = -1;
            RentalSkill3CB.Items.Clear();
            RentalSkill3CB.DisplayMember = "Item1";
            RentalSkill3CB.ValueMember = "Item2";
            RentalSkill4CB.SelectedIndex = -1;
            RentalSkill4CB.Items.Clear();
            RentalSkill4CB.DisplayMember = "Item1";
            RentalSkill4CB.ValueMember = "Item2";
            RentalSkill1CB.Items.Add(new Tuple<string, uint>("None", 0));
            RentalSkill2CB.Items.Add(new Tuple<string, uint>("None", 0));
            RentalSkill3CB.Items.Add(new Tuple<string, uint>("None", 0));
            RentalSkill4CB.Items.Add(new Tuple<string, uint>("None", 0));
            foreach(var it in skill_names_)
            {
                var id = it.Key;
                var name = it.Value;
                RentalSkill1CB.Items.Add(new Tuple<string, uint>(name, id));
                RentalSkill2CB.Items.Add(new Tuple<string, uint>(name, id));
                RentalSkill3CB.Items.Add(new Tuple<string, uint>(name, id));
                RentalSkill4CB.Items.Add(new Tuple<string, uint>(name, id));
            }

            // Puppets
            RentalPuppetCB.SelectedIndex = -1;
            RentalPuppetCB.Items.Clear();
            RentalPuppetCB.DisplayMember = "Item1";
            RentalPuppetCB.ValueMember = "Item2";
            RentalPuppetCB.Items.Add(new Tuple<string, uint>("None", 0));
            foreach(var it in puppets_)
            {
                var puppet = it.Value;
                if(puppet.id >= puppet_names_.Length)
                    continue;
                RentalPuppetCB.Items.Add(new Tuple<string, uint>(puppet_names_[puppet.id], puppet.id));
            }
        }

        private static void WritePts(PtsJson pts)
        {
            try
            {
                var ser = new DataContractJsonSerializer(typeof(PtsJson));
                var s = new MemoryStream();
                ser.WriteObject(s, pts);
                File.WriteAllBytes(pts.filepath, s.ToArray());
            }
            catch(Exception e)
            {
                throw new Exception("Error saving rental team: " + pts.filepath + "\r\n" + e.Message);
            }
        }

        private void SaveRentals()
        {
            Parallel.ForEach(rentals_, pts => WritePts(pts));

            /*
            foreach(var pts in rentals_)
            {
                WritePts(pts);
            }
            */
        }

        private void RentalIVSC_ValueChanged(object sender, EventArgs e)
        {
            int statindex;
            if(sender == (object)RentalIV1SC)
                statindex = 0;
            else if(sender == (object)RentalIV2SC)
                statindex = 1;
            else if(sender == (object)RentalIV3SC)
                statindex = 2;
            else if(sender == (object)RentalIV4SC)
                statindex = 3;
            else if(sender == (object)RentalIV5SC)
                statindex = 4;
            else if(sender == (object)RentalIV6SC)
                statindex = 5;
            else
                return;

            var val = ((NumericUpDown)sender).Value;
            var ptsindex = RentalLB.SelectedIndex;
            var slotindex = RentalSlotCB.SelectedIndex;
            var puppetindex = RentalPuppetCB.SelectedIndex;
            if(ptsindex < 0 || slotindex < 0 || puppetindex < 0)
                return;

            rentals_[ptsindex].puppets[slotindex].ivs[statindex] = (uint)val;
        }

        private void RentalEVSC_ValueChanged(object sender, EventArgs e)
        {
            int statindex;
            if(sender == (object)RentalEV1SC)
                statindex = 0;
            else if(sender == (object)RentalEV2SC)
                statindex = 1;
            else if(sender == (object)RentalEV3SC)
                statindex = 2;
            else if(sender == (object)RentalEV4SC)
                statindex = 3;
            else if(sender == (object)RentalEV5SC)
                statindex = 4;
            else if(sender == (object)RentalEV6SC)
                statindex = 5;
            else
                return;

            var val = ((NumericUpDown)sender).Value;
            var ptsindex = RentalLB.SelectedIndex;
            var slotindex = RentalSlotCB.SelectedIndex;
            var puppetindex = RentalPuppetCB.SelectedIndex;
            if(ptsindex < 0 || slotindex < 0 || puppetindex < 0)
                return;

            rentals_[ptsindex].puppets[slotindex].evs[statindex] = (uint)val;
        }

        private void RentalLB_SelectedIndexChanged(object sender, EventArgs e)
        {
            var index = RentalLB.SelectedIndex;
            if(index < 0)
                return;

            var pts = rentals_[index];

            PtsFilepathTB.Text = pts.filepath;
            RentalSlotCB.SelectedIndex = -1;
            RentalPuppetCB.SelectedIndex = -1;
            RentalPuppetNickTB.Clear();
            RentalStyleCB.SelectedIndex = -1;
            RentalStyleCB.Items.Clear();
            RentalAbilityCB.SelectedIndex = -1;
            RentalAbilityCB.Items.Clear();
            RentalMarkCB.SelectedIndex = -1;
            RentalCostumeCB.SelectedIndex = -1;
            RentalItemCB.SelectedIndex = -1;
            RentalExpSC.Value = 0;
            RentalHeartMarkCB.Checked = false;

            RentalSkill1CB.SelectedIndex = -1;
            RentalSkill2CB.SelectedIndex = -1;
            RentalSkill3CB.SelectedIndex = -1;
            RentalSkill4CB.SelectedIndex = -1;

            RentalIV1SC.Value = 0;
            RentalIV2SC.Value = 0;
            RentalIV3SC.Value = 0;
            RentalIV4SC.Value = 0;
            RentalIV5SC.Value = 0;
            RentalIV6SC.Value = 0;

            RentalEV1SC.Value = 0;
            RentalEV2SC.Value = 0;
            RentalEV3SC.Value = 0;
            RentalEV4SC.Value = 0;
            RentalEV5SC.Value = 0;
            RentalEV6SC.Value = 0;

            RentalLevelSC.Value = 0;
        }

        private void RefreshRentalPuppet()
        {
            var index = RentalSlotCB.SelectedIndex;
            var ptsindex = RentalLB.SelectedIndex;
            if(index < 0 || ptsindex < 0)
                return;

            var puppet = rentals_[ptsindex].puppets[index];
            if(puppets_.ContainsKey(puppet.id))
            {
                RentalPuppetCB.SelectedIndex = RentalPuppetCB.FindStringExact(puppet_names_[puppet.id]);
            }
            else
            {
                RentalPuppetCB.SelectedIndex = 0;
                puppet = new PuppetJson();
            }

            RentalPuppetNickTB.Text = puppet.nickname;
            RentalStyleCB.SelectedIndex = (puppet.style < 4) ? (int)puppet.style : 0;
            RentalItemCB.SelectedIndex = RentalItemCB.FindStringExact(item_names_.ContainsKey(puppet.held_item) ? item_names_[puppet.held_item] : "None");
            RentalAbilityCB.SelectedIndex = (puppet.ability < 2) ? (int)puppet.ability : 0;
            RentalExpSC.Value = puppet.experience;
            RentalMarkCB.SelectedIndex = RentalMarkCB.FindStringExact(puppet.mark);
            RentalCostumeCB.SelectedIndex = (puppet.costume < 4) ? (int)puppet.costume : 0;

            RentalHeartMarkCB.Checked = puppet.heart_mark;

            // FIXME: problematic if any skills have the same name
            RentalSkill1CB.SelectedIndex = RentalSkill1CB.FindStringExact(skill_names_.ContainsKey(puppet.skills[0]) ? skill_names_[puppet.skills[0]] : "None");
            RentalSkill2CB.SelectedIndex = RentalSkill2CB.FindStringExact(skill_names_.ContainsKey(puppet.skills[1]) ? skill_names_[puppet.skills[1]] : "None");
            RentalSkill3CB.SelectedIndex = RentalSkill3CB.FindStringExact(skill_names_.ContainsKey(puppet.skills[2]) ? skill_names_[puppet.skills[2]] : "None");
            RentalSkill4CB.SelectedIndex = RentalSkill4CB.FindStringExact(skill_names_.ContainsKey(puppet.skills[3]) ? skill_names_[puppet.skills[3]] : "None");

            RentalIV1SC.Value = puppet.ivs[0];
            RentalIV2SC.Value = puppet.ivs[1];
            RentalIV3SC.Value = puppet.ivs[2];
            RentalIV4SC.Value = puppet.ivs[3];
            RentalIV5SC.Value = puppet.ivs[4];
            RentalIV6SC.Value = puppet.ivs[5];

            RentalEV1SC.Value = puppet.evs[0];
            RentalEV2SC.Value = puppet.evs[1];
            RentalEV3SC.Value = puppet.evs[2];
            RentalEV4SC.Value = puppet.evs[3];
            RentalEV5SC.Value = puppet.evs[4];
            RentalEV6SC.Value = puppet.evs[5];
        }

        private void RentalSlotCB_SelectedIndexChanged(object sender, EventArgs e)
        {
            RefreshRentalPuppet();
        }

        private void RentalPuppetCB_SelectedIndexChanged(object sender, EventArgs e)
        {
            var index = RentalPuppetCB.SelectedIndex;
            var slotindex = RentalSlotCB.SelectedIndex;
            var ptsindex = RentalLB.SelectedIndex;
            if(index < 0 || slotindex < 0 || ptsindex < 0)
                return;

            var id = ((Tuple<string, uint>)RentalPuppetCB.SelectedItem).Item2;
            DollData data = puppets_.ContainsKey(id) ? puppets_[id] : new DollData();

            RentalStyleCB.SelectedIndex = -1;
            RentalStyleCB.Items.Clear();
            foreach(var style in data.styles)
                RentalStyleCB.Items.Add(style.type);

            RentalAbilityCB.SelectedIndex = -1;
            RentalAbilityCB.Items.Clear();

            var puppet = rentals_[ptsindex].puppets[slotindex];

            RentalLevelSC.ValueChanged -= RentalLevelSC_ValueChanged;
            RentalLevelSC.Value = LevelFromExp(data.cost, puppet.experience);
            RentalLevelSC.ValueChanged += RentalLevelSC_ValueChanged;

            if(string.IsNullOrEmpty(puppet.nickname) || (puppet.nickname == puppet_names_[puppet.id]))
                RentalPuppetNickTB.Text = (id > 0) ? puppet_names_[id] : "";

            rentals_[ptsindex].puppets[slotindex].id = id;
        }

        private void RentalStyleCB_SelectedIndexChanged(object sender, EventArgs e)
        {
            var index = RentalStyleCB.SelectedIndex;
            var puppetindex = RentalPuppetCB.SelectedIndex;
            var slotindex = RentalSlotCB.SelectedIndex;
            var ptsindex = RentalLB.SelectedIndex;
            if(index < 0 || puppetindex < 0 || slotindex < 0 || ptsindex < 0)
                return;

            var id = rentals_[ptsindex].puppets[slotindex].id;
            DollData data;
            if(puppetindex > 0 && puppets_.ContainsKey(id))
                data = puppets_[id];
            else
                data = new DollData();

            RentalAbilityCB.SelectedIndex = -1;
            RentalAbilityCB.Items.Clear();
            foreach(var ability in data.styles[index].abilities)
                RentalAbilityCB.Items.Add(ability_names_.ContainsKey(ability) ? ability_names_[ability] : "None");

            rentals_[ptsindex].puppets[slotindex].style = (uint)index;
        }

        private void RentalSkill_SelectedIndexChanged(object sender, EventArgs e)
        {
            int skillindex;
            if(sender == (object)RentalSkill1CB)
                skillindex = 0;
            else if(sender == (object)RentalSkill2CB)
                skillindex = 1;
            else if(sender == (object)RentalSkill3CB)
                skillindex = 2;
            else if(sender == (object)RentalSkill4CB)
                skillindex = 3;
            else
                return;

            var index = ((ComboBox)sender).SelectedIndex;
            var slotindex = RentalSlotCB.SelectedIndex;
            var ptsindex = RentalLB.SelectedIndex;
            if(index < 0 || slotindex < 0 || ptsindex < 0)
                return;

            var id = ((Tuple<string, uint>)((ComboBox)sender).SelectedItem).Item2;
            rentals_[ptsindex].puppets[slotindex].skills[skillindex] = id;
        }

        private void RentalAbilityCB_SelectedIndexChanged(object sender, EventArgs e)
        {
            var index = RentalAbilityCB.SelectedIndex;
            var puppetindex = RentalPuppetCB.SelectedIndex;
            var slotindex = RentalSlotCB.SelectedIndex;
            var ptsindex = RentalLB.SelectedIndex;
            if(index < 0 || puppetindex < 0 || slotindex < 0 || ptsindex < 0)
                return;

            rentals_[ptsindex].puppets[slotindex].ability = (uint)index;
        }

        private void RentalExpSC_ValueChanged(object sender, EventArgs e)
        {
            var value = RentalExpSC.Value;
            var puppetindex = RentalPuppetCB.SelectedIndex;
            var slotindex = RentalSlotCB.SelectedIndex;
            var ptsindex = RentalLB.SelectedIndex;
            if(puppetindex < 0 || slotindex < 0 || ptsindex < 0)
                return;

            rentals_[ptsindex].puppets[slotindex].experience = (uint)value;

            var id = ((Tuple<string, uint>)RentalPuppetCB.SelectedItem).Item2;
            DollData data = puppets_.ContainsKey(id) ? puppets_[id] : new DollData();

            RentalLevelSC.ValueChanged -= RentalLevelSC_ValueChanged;
            RentalLevelSC.Value = LevelFromExp(data.cost, (uint)value);
            RentalLevelSC.ValueChanged += RentalLevelSC_ValueChanged;
        }

        private void RentalLevelSC_ValueChanged(object sender, EventArgs e)
        {
            var value = RentalLevelSC.Value;
            var puppetindex = RentalPuppetCB.SelectedIndex;
            var slotindex = RentalSlotCB.SelectedIndex;
            var ptsindex = RentalLB.SelectedIndex;
            if(puppetindex < 0 || slotindex < 0 || ptsindex < 0)
                return;

            var id = ((Tuple<string, uint>)RentalPuppetCB.SelectedItem).Item2;
            DollData data = puppets_.ContainsKey(id) ? puppets_[id] : new DollData();

            RentalExpSC.Value = ExpForLevel(data.cost, (uint)value);
        }

        private void RentalMarkCB_SelectedIndexChanged(object sender, EventArgs e)
        {
            var index = RentalMarkCB.SelectedIndex;
            var puppetindex = RentalPuppetCB.SelectedIndex;
            var slotindex = RentalSlotCB.SelectedIndex;
            var ptsindex = RentalLB.SelectedIndex;
            if(index < 0 || puppetindex < 0 || slotindex < 0 || ptsindex < 0)
                return;

            rentals_[ptsindex].puppets[slotindex].mark = (string)RentalMarkCB.SelectedItem;
        }

        private void RentalCostumeCB_SelectedIndexChanged(object sender, EventArgs e)
        {
            var index = RentalCostumeCB.SelectedIndex;
            var puppetindex = RentalPuppetCB.SelectedIndex;
            var slotindex = RentalSlotCB.SelectedIndex;
            var ptsindex = RentalLB.SelectedIndex;
            if(index < 0 || puppetindex < 0 || slotindex < 0 || ptsindex < 0)
                return;

            rentals_[ptsindex].puppets[slotindex].costume = (uint)index;
        }

        private void RentalItemCB_SelectedIndexChanged(object sender, EventArgs e)
        {
            var index = RentalItemCB.SelectedIndex;
            var puppetindex = RentalPuppetCB.SelectedIndex;
            var slotindex = RentalSlotCB.SelectedIndex;
            var ptsindex = RentalLB.SelectedIndex;
            if(index < 0 || puppetindex < 0 || slotindex < 0 || ptsindex < 0)
                return;

            var id = ((Tuple<string, uint>)RentalItemCB.SelectedItem).Item2;
            rentals_[ptsindex].puppets[slotindex].held_item = id;
        }

        private void RentalPuppetNickTB_TextChanged(object sender, EventArgs e)
        {
            var puppetindex = RentalPuppetCB.SelectedIndex;
            var slotindex = RentalSlotCB.SelectedIndex;
            var ptsindex = RentalLB.SelectedIndex;
            if(puppetindex < 0 || slotindex < 0 || ptsindex < 0)
                return;

            rentals_[ptsindex].puppets[slotindex].nickname = RentalPuppetNickTB.Text;
        }

        private void RentalHeartMarkCB_CheckedChanged(object sender, EventArgs e)
        {
            var puppetindex = RentalPuppetCB.SelectedIndex;
            var slotindex = RentalSlotCB.SelectedIndex;
            var ptsindex = RentalLB.SelectedIndex;
            if(puppetindex < 0 || slotindex < 0 || ptsindex < 0)
                return;

            rentals_[ptsindex].puppets[slotindex].heart_mark = RentalHeartMarkCB.Checked;
        }

        private void RentalAllSBT_Click(object sender, EventArgs e)
        {
            RentalIV1SC.Value = 15;
            RentalIV2SC.Value = 15;
            RentalIV3SC.Value = 15;
            RentalIV4SC.Value = 15;
            RentalIV5SC.Value = 15;
            RentalIV6SC.Value = 15;
        }

        private void RentalAllEBT_Click(object sender, EventArgs e)
        {
            RentalIV1SC.Value = 0;
            RentalIV2SC.Value = 0;
            RentalIV3SC.Value = 0;
            RentalIV4SC.Value = 0;
            RentalIV5SC.Value = 0;
            RentalIV6SC.Value = 0;
        }

        private void RentalAll64BT_Click(object sender, EventArgs e)
        {
            RentalEV1SC.Value = 64;
            RentalEV2SC.Value = 64;
            RentalEV3SC.Value = 64;
            RentalEV4SC.Value = 64;
            RentalEV5SC.Value = 64;
            RentalEV6SC.Value = 64;
        }

        private void RentalAll0BT_Click(object sender, EventArgs e)
        {
            RentalEV1SC.Value = 0;
            RentalEV2SC.Value = 0;
            RentalEV3SC.Value = 0;
            RentalEV4SC.Value = 0;
            RentalEV5SC.Value = 0;
            RentalEV6SC.Value = 0;
        }

        private void NewRentalBT_Click(object sender, EventArgs e)
        {
            if(string.IsNullOrEmpty(working_dir_) || rentals_.Count == 0)
                return;

            if(!is_ynk_)
            {
                ErrMsg("Base TPDP not supported.");
                return;
            }

            int id = 0;
            using(var dialog = new NewIDDialog("New Team", 999))
            {
                if(dialog.ShowDialog() != DialogResult.OK)
                    return;
                id = dialog.ID;
            }

            var path = working_dir_ + "/gn_dat5.arc/script/party/" + id.ToString("D3") + ".pts";
            if(File.Exists(path))
            {
                ErrMsg("Team ID already exists!");
                return;
            }

            try
            {
                byte[] buf = new byte[894];
                File.WriteAllBytes(path, buf);
            }
            catch(Exception ex)
            {
                ErrMsg("Failed to write to file: " + path + "\r\n" + ex.Message);
                return;
            }

            var pts = new PtsJson();
            pts.filepath = path.Replace(".pts", ".json");
            pts.id = id;
            rentals_.Add(pts);
            RentalLB.Items.Add(pts.id.ToString("D3"));
            RentalLB.SelectedIndex = RentalLB.Items.Count - 1;
            WritePts(pts);
        }

        private void RentalEquipBT_Click(object sender, EventArgs e)
        {
            var slotindex = RentalSlotCB.SelectedIndex;
            var ptsindex = RentalLB.SelectedIndex;
            if(slotindex < 0 || ptsindex < 0)
                return;

            var puppet = rentals_[ptsindex].puppets[slotindex];
            if(puppet.id == 0)
                return;

            var data = puppets_[puppet.id];
            var style = data.styles[puppet.style];

            var moves = new List<uint>();

            var lvl = LevelFromExp(data.cost, puppet.experience);
            foreach(var it in skills_)
            {
                var id = it.Key;
                var lvlreq = data.LevelToLearn(puppet.style, id);
                if((lvlreq >= 0) && (lvl >= lvlreq))
                    moves.Add(id);
            }
            var tmp = new List<uint>(moves);
            foreach(var i in tmp)
            {
                var skill1 = skills_[i];
                if(skill1.type == "Status")
                    continue;
                foreach(var j in moves)
                {
                    var skill2 = skills_[j];
                    if(skill2.type == "Status")
                        continue;
                    if((skill2.element == skill1.element) && (skill2.type == skill1.type) && (skill2.priority == skill1.priority) && (skill2.power > skill1.power))
                    {
                        moves.Remove(i);
                        break;
                    }
                }
            }

            bool is_stab(uint id)
            {
                return ((skills_[id].element == style.element1) || (skills_[id].element == style.element2)) && (skills_[id].type != "Status");
            }
            int comp(uint l, uint r)
            {
                bool lstab = is_stab(l);
                bool rstab = is_stab(r);
                var lreq = data.LevelToLearn(puppet.style, l);
                var rreq = data.LevelToLearn(puppet.style, r);
                if(!lstab && rstab)
                    return 1;
                if(lstab && !rstab)
                    return -1;
                if(lreq < rreq)
                    return 1;
                if(lreq > rreq)
                    return -1;
                return 0;
            }
            moves.Sort(comp);
            for(var i = 0; i < 4; ++i)
                rentals_[ptsindex].puppets[slotindex].skills[i] = (i < moves.Count) ? moves[i] : 0u;
            RefreshRentalPuppet();
        }
    }
}
