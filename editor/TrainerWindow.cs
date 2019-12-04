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

// Trainer tab of MainWindow

namespace editor
{
    public partial class EditorMainWindow : Form
    {
        private uint ExpForLevel(uint cost, uint level)
        {
            if(level <= 1)
                return 0;

            uint[] mods = is_ynk_ ? new uint[] { 85, 92, 100, 107, 115 } : new uint[] { 70, 85, 100, 115, 130 };

            uint ret = level * level * level * mods[cost] / 100u;

            return ret;
        }

        private uint LevelFromExp(uint cost, uint exp)
        {
            uint ret = 1;
            while(ExpForLevel(cost, ret + 1) <= exp)
                ++ret;

            if(ret > 100)
                ret = 100;
            return ret;
        }

        private void LoadTrainers(string dir)
        {
            DirectoryInfo di = new DirectoryInfo(dir);
            if(!di.Exists)
            {
                throw new Exception("Could not locate dod folder");
            }

            dods_.Clear();

            try
            {
                var files = di.GetFiles("*.json");

                foreach(var file in files)
                {
                    DataContractJsonSerializer s = new DataContractJsonSerializer(typeof(DodJson));
                    var fs = file.OpenRead();

                    try
                    {
                        var dod = (DodJson)s.ReadObject(fs);
                        dod.filepath = file.FullName;

                        Regex rx = new Regex(@"(?<id>\d{4})\.json$", RegexOptions.Compiled | RegexOptions.IgnoreCase);
                        var match = rx.Match(dod.filepath);

                        if(match.Success)
                            dod.id = int.Parse(match.Groups["id"].Value);
                        else
                            continue;

                        dods_.Add(dod);
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
                throw new Exception("Error iterating dod folder: " + e.Message);
            }

            // Trainers
            TrainerLB.SelectedIndex = -1;
            TrainerLB.Items.Clear();
            foreach(var dod in dods_)
            {
                TrainerLB.Items.Add(dod.trainer_name);
            }

            // Items
            TrainerItemCB.SelectedIndex = -1;
            TrainerItemCB.Items.Clear();
            TrainerItemCB.DisplayMember = "Item1";
            TrainerItemCB.ValueMember = "Item2";
            TrainerItemCB.Items.Add(new Tuple<string, uint>("None", 0));
            foreach(var it in item_names_)
            {
                var id = it.Key;
                var name = it.Value;
                TrainerItemCB.Items.Add(new Tuple<string, uint>(name, id));
            }

            // Skills
            TrainerSkill1CB.SelectedIndex = -1;
            TrainerSkill1CB.Items.Clear();
            TrainerSkill1CB.DisplayMember = "Item1";
            TrainerSkill1CB.ValueMember = "Item2";
            TrainerSkill2CB.SelectedIndex = -1;
            TrainerSkill2CB.Items.Clear();
            TrainerSkill2CB.DisplayMember = "Item1";
            TrainerSkill2CB.ValueMember = "Item2";
            TrainerSkill3CB.SelectedIndex = -1;
            TrainerSkill3CB.Items.Clear();
            TrainerSkill3CB.DisplayMember = "Item1";
            TrainerSkill3CB.ValueMember = "Item2";
            TrainerSkill4CB.SelectedIndex = -1;
            TrainerSkill4CB.Items.Clear();
            TrainerSkill4CB.DisplayMember = "Item1";
            TrainerSkill4CB.ValueMember = "Item2";
            TrainerSkill1CB.Items.Add(new Tuple<string, uint>("None", 0));
            TrainerSkill2CB.Items.Add(new Tuple<string, uint>("None", 0));
            TrainerSkill3CB.Items.Add(new Tuple<string, uint>("None", 0));
            TrainerSkill4CB.Items.Add(new Tuple<string, uint>("None", 0));
            foreach(var it in skill_names_)
            {
                var id = it.Key;
                var name = it.Value;
                TrainerSkill1CB.Items.Add(new Tuple<string, uint>(name, id));
                TrainerSkill2CB.Items.Add(new Tuple<string, uint>(name, id));
                TrainerSkill3CB.Items.Add(new Tuple<string, uint>(name, id));
                TrainerSkill4CB.Items.Add(new Tuple<string, uint>(name, id));
            }

            // Puppets
            TrainerPuppetCB.SelectedIndex = -1;
            TrainerPuppetCB.Items.Clear();
            TrainerPuppetCB.DisplayMember = "Item1";
            TrainerPuppetCB.ValueMember = "Item2";
            TrainerPuppetCB.Items.Add(new Tuple<string, uint>("None", 0));
            foreach(var it in puppets_)
            {
                var puppet = it.Value;
                if(puppet.id >= puppet_names_.Length)
                    continue;
                TrainerPuppetCB.Items.Add(new Tuple<string, uint>(puppet_names_[puppet.id], puppet.id));
            }
        }

        private void SaveTrainers()
        {
            foreach(var dod in dods_)
            {
                try
                {
                    var ser = new DataContractJsonSerializer(typeof(DodJson));
                    var s = new MemoryStream();
                    ser.WriteObject(s, dod);
                    File.WriteAllBytes(dod.filepath, s.ToArray());
                }
                catch(Exception e)
                {
                    throw new Exception("Error saving trainer: " + dod.filepath + "\r\n" + e.Message);
                }
            }
        }

        private void IVSC_ValueChanged(object sender, EventArgs e)
        {
            int statindex;
            if(sender == (object)IV1SC)
                statindex = 0;
            else if(sender == (object)IV2SC)
                statindex = 1;
            else if(sender == (object)IV3SC)
                statindex = 2;
            else if(sender == (object)IV4SC)
                statindex = 3;
            else if(sender == (object)IV5SC)
                statindex = 4;
            else if(sender == (object)IV6SC)
                statindex = 5;
            else
                return;

            var val = ((NumericUpDown)sender).Value;
            var dodindex = TrainerLB.SelectedIndex;
            var slotindex = TrainerSlotCB.SelectedIndex;
            var puppetindex = TrainerPuppetCB.SelectedIndex;
            if(dodindex < 0 || slotindex < 0 || puppetindex < 0)
                return;

            dods_[dodindex].puppets[slotindex].ivs[statindex] = (uint)val;
        }

        private void EVSC_ValueChanged(object sender, EventArgs e)
        {
            int statindex;
            if(sender == (object)EV1SC)
                statindex = 0;
            else if(sender == (object)EV2SC)
                statindex = 1;
            else if(sender == (object)EV3SC)
                statindex = 2;
            else if(sender == (object)EV4SC)
                statindex = 3;
            else if(sender == (object)EV5SC)
                statindex = 4;
            else if(sender == (object)EV6SC)
                statindex = 5;
            else
                return;

            var val = ((NumericUpDown)sender).Value;
            var dodindex = TrainerLB.SelectedIndex;
            var slotindex = TrainerSlotCB.SelectedIndex;
            var puppetindex = TrainerPuppetCB.SelectedIndex;
            if(dodindex < 0 || slotindex < 0 || puppetindex < 0)
                return;

            dods_[dodindex].puppets[slotindex].evs[statindex] = (uint)val;
        }

        private void TrainerLB_SelectedIndexChanged(object sender, EventArgs e)
        {
            var index = TrainerLB.SelectedIndex;
            if(index < 0)
                return;

            var dod = dods_[index];

            TrainerNameTB.Text = dod.trainer_name;
            TrainerTitleTB.Text = dod.trainer_title;
            PortraitIDSC.Value = dod.portrait_id;
            DodFilepathTB.Text = dod.filepath;
            TrainerSlotCB.SelectedIndex = -1;
            TrainerPuppetCB.SelectedIndex = -1;
            TrainerPuppetNickTB.Clear();
            TrainerStyleCB.SelectedIndex = -1;
            TrainerStyleCB.Items.Clear();
            TrainerAbilityCB.SelectedIndex = -1;
            TrainerAbilityCB.Items.Clear();
            TrainerMarkCB.SelectedIndex = -1;
            TrainerCostumeCB.SelectedIndex = -1;
            TrainerItemCB.SelectedIndex = -1;
            TrainerExpSC.Value = 0;

            IntroTextSC.Value = dod.intro_text_id;
            EndTextSC.Value = dod.end_text_id;

            TrainerSkill1CB.SelectedIndex = -1;
            TrainerSkill2CB.SelectedIndex = -1;
            TrainerSkill3CB.SelectedIndex = -1;
            TrainerSkill4CB.SelectedIndex = -1;

            IV1SC.Value = 0;
            IV2SC.Value = 0;
            IV3SC.Value = 0;
            IV4SC.Value = 0;
            IV5SC.Value = 0;
            IV6SC.Value = 0;

            EV1SC.Value = 0;
            EV2SC.Value = 0;
            EV3SC.Value = 0;
            EV4SC.Value = 0;
            EV5SC.Value = 0;
            EV6SC.Value = 0;
        }

        private void TrainerSlotCB_SelectedIndexChanged(object sender, EventArgs e)
        {
            var index = TrainerSlotCB.SelectedIndex;
            var dodindex = TrainerLB.SelectedIndex;
            if(index < 0 || dodindex < 0)
                return;

            var puppet = dods_[dodindex].puppets[index];
            if(puppets_.ContainsKey(puppet.id))
            {
                TrainerPuppetCB.SelectedIndex = TrainerPuppetCB.FindStringExact(puppet_names_[puppet.id]);
            }
            else
            {
                TrainerPuppetCB.SelectedIndex = 0;
                puppet = new PuppetJson();
            }

            TrainerPuppetNickTB.Text = puppet.nickname;
            TrainerStyleCB.SelectedIndex = (puppet.style < 4) ? (int)puppet.style : 0;
            TrainerItemCB.SelectedIndex = TrainerItemCB.FindStringExact(item_names_.ContainsKey(puppet.held_item) ? item_names_[puppet.held_item] : "None");
            TrainerAbilityCB.SelectedIndex = (puppet.ability < 2) ? (int)puppet.ability : 0;
            TrainerExpSC.Value = puppet.experience;
            TrainerMarkCB.SelectedIndex = TrainerMarkCB.FindStringExact(puppet.mark);
            TrainerCostumeCB.SelectedIndex = (puppet.costume < 4) ? (int)puppet.costume : 0;

            TrainerSkill1CB.SelectedIndex = TrainerSkill1CB.FindStringExact(skill_names_.ContainsKey(puppet.skills[0]) ? skill_names_[puppet.skills[0]] : "None");
            TrainerSkill2CB.SelectedIndex = TrainerSkill2CB.FindStringExact(skill_names_.ContainsKey(puppet.skills[1]) ? skill_names_[puppet.skills[1]] : "None");
            TrainerSkill3CB.SelectedIndex = TrainerSkill3CB.FindStringExact(skill_names_.ContainsKey(puppet.skills[2]) ? skill_names_[puppet.skills[2]] : "None");
            TrainerSkill4CB.SelectedIndex = TrainerSkill4CB.FindStringExact(skill_names_.ContainsKey(puppet.skills[3]) ? skill_names_[puppet.skills[3]] : "None");

            IV1SC.Value = puppet.ivs[0];
            IV2SC.Value = puppet.ivs[1];
            IV3SC.Value = puppet.ivs[2];
            IV4SC.Value = puppet.ivs[3];
            IV5SC.Value = puppet.ivs[4];
            IV6SC.Value = puppet.ivs[5];

            EV1SC.Value = puppet.evs[0];
            EV2SC.Value = puppet.evs[1];
            EV3SC.Value = puppet.evs[2];
            EV4SC.Value = puppet.evs[3];
            EV5SC.Value = puppet.evs[4];
            EV6SC.Value = puppet.evs[5];
        }

        private void TrainerPuppetCB_SelectedIndexChanged(object sender, EventArgs e)
        {
            var index = TrainerPuppetCB.SelectedIndex;
            var slotindex = TrainerSlotCB.SelectedIndex;
            var dodindex = TrainerLB.SelectedIndex;
            if(index < 0 || slotindex < 0 || dodindex < 0)
                return;

            var id = ((Tuple<string, uint>)TrainerPuppetCB.SelectedItem).Item2;
            DollData data = puppets_.ContainsKey(id) ? puppets_[id] : new DollData();

            TrainerStyleCB.SelectedIndex = -1;
            TrainerStyleCB.Items.Clear();
            foreach(var style in data.styles)
                TrainerStyleCB.Items.Add(style.type);

            TrainerAbilityCB.SelectedIndex = -1;
            TrainerAbilityCB.Items.Clear();

            dods_[dodindex].puppets[slotindex].id = id;
        }

        private void TrainerStyleCB_SelectedIndexChanged(object sender, EventArgs e)
        {
            var index = TrainerStyleCB.SelectedIndex;
            var puppetindex = TrainerPuppetCB.SelectedIndex;
            var slotindex = TrainerSlotCB.SelectedIndex;
            var dodindex = TrainerLB.SelectedIndex;
            if(index < 0 || puppetindex < 0 || slotindex < 0 || dodindex < 0)
                return;

            var id = dods_[dodindex].puppets[slotindex].id;
            DollData data;
            if(puppetindex > 0 && puppets_.ContainsKey(id))
                data = puppets_[id];
            else
                data = new DollData();

            TrainerAbilityCB.SelectedIndex = -1;
            TrainerAbilityCB.Items.Clear();
            foreach(var ability in data.styles[index].abilities)
                TrainerAbilityCB.Items.Add(ability_names_.ContainsKey(ability) ? ability_names_[ability] : "None");

            dods_[dodindex].puppets[slotindex].style = (uint)index;
        }

        private void TrainerSkill_SelectedIndexChanged(object sender, EventArgs e)
        {
            int skillindex;
            if(sender == (object)TrainerSkill1CB)
                skillindex = 0;
            else if(sender == (object)TrainerSkill2CB)
                skillindex = 1;
            else if(sender == (object)TrainerSkill3CB)
                skillindex = 2;
            else if(sender == (object)TrainerSkill4CB)
                skillindex = 3;
            else
                return;

            var index = ((ComboBox)sender).SelectedIndex;
            var slotindex = TrainerSlotCB.SelectedIndex;
            var dodindex = TrainerLB.SelectedIndex;
            if(index < 0 || slotindex < 0 || dodindex < 0)
                return;

            var id = ((Tuple<string, uint>)((ComboBox)sender).SelectedItem).Item2;
            dods_[dodindex].puppets[slotindex].skills[skillindex] = id;
        }

        private void TrainerNameTB_TextChanged(object sender, EventArgs e)
        {
            var dodindex = TrainerLB.SelectedIndex;
            if(dodindex < 0)
                return;

            dods_[dodindex].trainer_name = TrainerNameTB.Text;
            TrainerLB.Items[dodindex] = TrainerNameTB.Text;
        }

        private void TrainerTitleTB_TextChanged(object sender, EventArgs e)
        {
            var dodindex = TrainerLB.SelectedIndex;
            if(dodindex < 0)
                return;

            dods_[dodindex].trainer_title = TrainerTitleTB.Text;
        }

        private void TrainerAbilityCB_SelectedIndexChanged(object sender, EventArgs e)
        {
            var index = TrainerAbilityCB.SelectedIndex;
            var puppetindex = TrainerPuppetCB.SelectedIndex;
            var slotindex = TrainerSlotCB.SelectedIndex;
            var dodindex = TrainerLB.SelectedIndex;
            if(index < 0 || puppetindex < 0 || slotindex < 0 || dodindex < 0)
                return;

            dods_[dodindex].puppets[slotindex].ability = (uint)index;
        }

        private void TrainerExpSC_ValueChanged(object sender, EventArgs e)
        {
            var value = TrainerExpSC.Value;
            var puppetindex = TrainerPuppetCB.SelectedIndex;
            var slotindex = TrainerSlotCB.SelectedIndex;
            var dodindex = TrainerLB.SelectedIndex;
            if(puppetindex < 0 || slotindex < 0 || dodindex < 0)
                return;

            dods_[dodindex].puppets[slotindex].experience = (uint)value;

            var id = ((Tuple<string, uint>)TrainerPuppetCB.SelectedItem).Item2;
            DollData data = puppets_.ContainsKey(id) ? puppets_[id] : new DollData();

            TrainerLevelSC.ValueChanged -= TrainerLevelSC_ValueChanged;
            TrainerLevelSC.Value = LevelFromExp(data.cost, (uint)value);
            TrainerLevelSC.ValueChanged += TrainerLevelSC_ValueChanged;
        }

        private void TrainerLevelSC_ValueChanged(object sender, EventArgs e)
        {
            var value = TrainerLevelSC.Value;
            var puppetindex = TrainerPuppetCB.SelectedIndex;
            var slotindex = TrainerSlotCB.SelectedIndex;
            var dodindex = TrainerLB.SelectedIndex;
            if(puppetindex < 0 || slotindex < 0 || dodindex < 0)
                return;

            var id = ((Tuple<string, uint>)TrainerPuppetCB.SelectedItem).Item2;
            DollData data = puppets_.ContainsKey(id) ? puppets_[id] : new DollData();

            TrainerExpSC.Value = ExpForLevel(data.cost, (uint)value);
        }

        private void TrainerMarkCB_SelectedIndexChanged(object sender, EventArgs e)
        {
            var index = TrainerMarkCB.SelectedIndex;
            var puppetindex = TrainerPuppetCB.SelectedIndex;
            var slotindex = TrainerSlotCB.SelectedIndex;
            var dodindex = TrainerLB.SelectedIndex;
            if(index < 0 || puppetindex < 0 || slotindex < 0 || dodindex < 0)
                return;

            dods_[dodindex].puppets[slotindex].mark = (string)TrainerMarkCB.SelectedItem;
        }

        private void TrainerCostumeCB_SelectedIndexChanged(object sender, EventArgs e)
        {
            var index = TrainerCostumeCB.SelectedIndex;
            var puppetindex = TrainerPuppetCB.SelectedIndex;
            var slotindex = TrainerSlotCB.SelectedIndex;
            var dodindex = TrainerLB.SelectedIndex;
            if(index < 0 || puppetindex < 0 || slotindex < 0 || dodindex < 0)
                return;

            dods_[dodindex].puppets[slotindex].costume = (uint)index;
        }

        private void TrainerItemCB_SelectedIndexChanged(object sender, EventArgs e)
        {
            var index = TrainerItemCB.SelectedIndex;
            var puppetindex = TrainerPuppetCB.SelectedIndex;
            var slotindex = TrainerSlotCB.SelectedIndex;
            var dodindex = TrainerLB.SelectedIndex;
            if(index < 0 || puppetindex < 0 || slotindex < 0 || dodindex < 0)
                return;

            var id = ((Tuple<string, uint>)TrainerItemCB.SelectedItem).Item2;
            dods_[dodindex].puppets[slotindex].held_item = id;
        }

        private void TrainerPuppetNickTB_TextChanged(object sender, EventArgs e)
        {
            var puppetindex = TrainerPuppetCB.SelectedIndex;
            var slotindex = TrainerSlotCB.SelectedIndex;
            var dodindex = TrainerLB.SelectedIndex;
            if(puppetindex < 0 || slotindex < 0 || dodindex < 0)
                return;

            dods_[dodindex].puppets[slotindex].nickname = TrainerPuppetNickTB.Text;
        }

        private void IntroTextSC_ValueChanged(object sender, EventArgs e)
        {
            var dodindex = TrainerLB.SelectedIndex;
            if(dodindex < 0)
                return;

            dods_[dodindex].intro_text_id = (uint)IntroTextSC.Value;
        }

        private void EndTextSC_ValueChanged(object sender, EventArgs e)
        {
            var dodindex = TrainerLB.SelectedIndex;
            if(dodindex < 0)
                return;

            dods_[dodindex].end_text_id = (uint)EndTextSC.Value;
        }

        private void PortraitIDSC_ValueChanged(object sender, EventArgs e)
        {
            var dodindex = TrainerLB.SelectedIndex;
            if(dodindex < 0)
                return;

            dods_[dodindex].portrait_id = (uint)PortraitIDSC.Value;
        }
    }
}
