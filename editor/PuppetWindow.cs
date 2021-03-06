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

// Puppet tab of MainWindow

namespace editor
{
    public partial class EditorMainWindow : Form
    {
        private void LoadPuppets()
        {
            // Puppets
            PuppetLB.ClearSelected();
            PuppetLB.Items.Clear();
            PuppetLB.DisplayMember = "Item1";
            PuppetLB.ValueMember = "Item2";
            foreach(var it in puppets_)
            {
                var puppet = it.Value;
                if(puppet.id >= puppet_names_.Length)
                    continue;
                PuppetLB.Items.Add(new Tuple<string, uint>(puppet_names_[puppet.id], puppet.id));
            }

            // Abilities
            PuppetAbility1CB.SelectedIndex = -1;
            PuppetAbility2CB.SelectedIndex = -1;
            PuppetAbility1CB.Items.Clear();
            PuppetAbility2CB.Items.Clear();
            PuppetAbility1CB.DisplayMember = "Item1";
            PuppetAbility1CB.ValueMember = "Item2";
            PuppetAbility2CB.DisplayMember = "Item1";
            PuppetAbility2CB.ValueMember = "Item2";
            PuppetAbility1CB.Items.Add(new Tuple<string, uint>("None", 0));
            PuppetAbility2CB.Items.Add(new Tuple<string, uint>("None", 0));
            foreach(var it in ability_names_)
            {
                var id = it.Key;
                var name = it.Value;
                PuppetAbility1CB.Items.Add(new Tuple<string, uint>(name, id));
                PuppetAbility2CB.Items.Add(new Tuple<string, uint>(name, id));
            }

            // Items
            Item1CB.SelectedIndex = -1;
            Item1CB.Items.Clear();
            Item1CB.DisplayMember = "Item1";
            Item1CB.ValueMember = "Item2";
            Item2CB.SelectedIndex = -1;
            Item2CB.Items.Clear();
            Item2CB.DisplayMember = "Item1";
            Item2CB.ValueMember = "Item2";
            Item3CB.SelectedIndex = -1;
            Item3CB.Items.Clear();
            Item3CB.DisplayMember = "Item1";
            Item3CB.ValueMember = "Item2";
            Item4CB.SelectedIndex = -1;
            Item4CB.Items.Clear();
            Item4CB.DisplayMember = "Item1";
            Item4CB.ValueMember = "Item2";
            Item1CB.Items.Add(new Tuple<string, uint>("None", 0));
            Item2CB.Items.Add(new Tuple<string, uint>("None", 0));
            Item3CB.Items.Add(new Tuple<string, uint>("None", 0));
            Item4CB.Items.Add(new Tuple<string, uint>("None", 0));
            foreach(var it in item_names_)
            {
                var id = it.Key;
                var name = it.Value;
                Item1CB.Items.Add(new Tuple<string, uint>(name, id));
                Item2CB.Items.Add(new Tuple<string, uint>(name, id));
                Item3CB.Items.Add(new Tuple<string, uint>(name, id));
                Item4CB.Items.Add(new Tuple<string, uint>(name, id));
            }

            // Skillcards
            SkillCardLB.Items.Clear();
            SkillCardLB.DisplayMember = "Item1";
            SkillCardLB.ValueMember = "Item2";
            foreach(var it in skillcard_names_)
            {
                var id = it.Key;
                var name = it.Value;
                SkillCardLB.Items.Add(new Tuple<string, uint>(name, id));
            }
        }

        private void PuppetLB_SelectedIndexChanged(object sender, EventArgs e)
        {
            var index = PuppetLB.SelectedIndex;
            if(index < 0)
                return;

            var id = ((Tuple<string, uint>)PuppetLB.Items[index]).Item2;
            var puppet = puppets_[id];

            PuppetIDSC.Value = id;
            PuppetCostSC.Value = puppet.cost;
            PuppetdexIndexSC.Value = puppet.puppetdex_index;

            PuppetStyleCB.SelectedIndex = -1;
            PuppetStyleTypeCB.SelectedIndex = -1;
            PuppetAbility1CB.SelectedIndex = -1;
            PuppetAbility2CB.SelectedIndex = -1;
            PuppetElement1CB.SelectedIndex = -1;
            PuppetElement2CB.SelectedIndex = -1;
            for(var i = 0; i < 128; ++i)
                SkillCardLB.SetItemChecked(i, false);
            StatHPSC.Value = 0;
            StatFASC.Value = 0;
            StatFDSC.Value = 0;
            StatSASC.Value = 0;
            StatSDSC.Value = 0;
            StatSPDSC.Value = 0;

            var itemid = puppet.item_drop_table[0];
            if(item_names_.ContainsKey(itemid))
                Item1CB.SelectedIndex = Item1CB.FindStringExact(item_names_[itemid]);
            else
                Item1CB.SelectedIndex = 0;

            itemid = puppet.item_drop_table[1];
            if(item_names_.ContainsKey(itemid))
                Item2CB.SelectedIndex = Item2CB.FindStringExact(item_names_[itemid]);
            else
                Item2CB.SelectedIndex = 0;

            itemid = puppet.item_drop_table[2];
            if(item_names_.ContainsKey(itemid))
                Item3CB.SelectedIndex = Item3CB.FindStringExact(item_names_[itemid]);
            else
                Item3CB.SelectedIndex = 0;

            itemid = puppet.item_drop_table[3];
            if(item_names_.ContainsKey(itemid))
                Item4CB.SelectedIndex = Item4CB.FindStringExact(item_names_[itemid]);
            else
                Item4CB.SelectedIndex = 0;
        }

        private void PuppetStyleCB_SelectedIndexChanged(object sender, EventArgs e)
        {
            var index = PuppetStyleCB.SelectedIndex;
            var puppetindex = PuppetLB.SelectedIndex;
            if(index < 0 || puppetindex < 0)
                return;

            var puppetid = ((Tuple<string, uint>)PuppetLB.Items[puppetindex]).Item2;
            var puppet = puppets_[puppetid];
            var style = puppet.styles[index];

            PuppetStyleTypeCB.SelectedIndex = PuppetStyleTypeCB.FindStringExact(style.type);
            PuppetElement1CB.SelectedIndex = PuppetElement1CB.FindStringExact(style.element1);
            PuppetElement2CB.SelectedIndex = PuppetElement2CB.FindStringExact(style.element2);
            var ability = ability_names_.ContainsKey(style.abilities[0]) ? ability_names_[style.abilities[0]] : "None";
            PuppetAbility1CB.SelectedIndex = PuppetAbility1CB.FindStringExact(ability);
            ability = ability_names_.ContainsKey(style.abilities[1]) ? ability_names_[style.abilities[1]] : "None";
            PuppetAbility2CB.SelectedIndex = PuppetAbility2CB.FindStringExact(ability);

            SkillCardLB.ItemCheck -= SkillCardLB_ItemCheck;
            for(var i = 0; i < 128; ++i)
                SkillCardLB.SetItemChecked(i, false);
            foreach(var id in style.compatibility)
            {
                if(id < 385 || id > 512)
                    continue;
                var i = id - 385;
                SkillCardLB.SetItemChecked((int)i, true);
            }
            SkillCardLB.ItemCheck += SkillCardLB_ItemCheck;

            StatHPSC.Value = style.base_stats[0];
            StatFASC.Value = style.base_stats[1];
            StatFDSC.Value = style.base_stats[2];
            StatSASC.Value = style.base_stats[3];
            StatSDSC.Value = style.base_stats[4];
            StatSPDSC.Value = style.base_stats[5];
        }

        private void PuppetCostSC_ValueChanged(object sender, EventArgs e)
        {
            var index = PuppetLB.SelectedIndex;
            if(index < 0)
                return;

            var id = ((Tuple<string, uint>)PuppetLB.SelectedItem).Item2;
            puppets_[id].cost = (uint)PuppetCostSC.Value;
        }

        private void PuppetdexIndexSC_ValueChanged(object sender, EventArgs e)
        {
            var index = PuppetLB.SelectedIndex;
            if(index < 0)
                return;

            var id = ((Tuple<string, uint>)PuppetLB.SelectedItem).Item2;
            puppets_[id].puppetdex_index = (uint)PuppetdexIndexSC.Value;
        }

        private void PuppetElement1CB_SelectedIndexChanged(object sender, EventArgs e)
        {
            var index = PuppetElement1CB.SelectedIndex;
            var puppetindex = PuppetLB.SelectedIndex;
            var styleindex = PuppetStyleCB.SelectedIndex;
            if(index < 0 || puppetindex < 0 || styleindex < 0)
                return;

            var id = ((Tuple<string, uint>)PuppetLB.SelectedItem).Item2;
            puppets_[id].styles[styleindex].element1 = (string)PuppetElement1CB.SelectedItem;
        }

        private void PuppetElement2CB_SelectedIndexChanged(object sender, EventArgs e)
        {
            var index = PuppetElement2CB.SelectedIndex;
            var puppetindex = PuppetLB.SelectedIndex;
            var styleindex = PuppetStyleCB.SelectedIndex;
            if(index < 0 || puppetindex < 0 || styleindex < 0)
                return;

            var id = ((Tuple<string, uint>)PuppetLB.SelectedItem).Item2;
            puppets_[id].styles[styleindex].element2 = (string)PuppetElement2CB.SelectedItem;
        }

        private void PuppetAbility1CB_SelectedIndexChanged(object sender, EventArgs e)
        {
            var index = PuppetAbility1CB.SelectedIndex;
            var puppetindex = PuppetLB.SelectedIndex;
            var styleindex = PuppetStyleCB.SelectedIndex;
            if(index < 0 || puppetindex < 0 || styleindex < 0)
                return;

            var id = ((Tuple<string, uint>)PuppetLB.SelectedItem).Item2;
            puppets_[id].styles[styleindex].abilities[0] = ((Tuple<string, uint>)PuppetAbility1CB.SelectedItem).Item2;
        }

        private void PuppetAbility2CB_SelectedIndexChanged(object sender, EventArgs e)
        {
            var index = PuppetAbility2CB.SelectedIndex;
            var puppetindex = PuppetLB.SelectedIndex;
            var styleindex = PuppetStyleCB.SelectedIndex;
            if(index < 0 || puppetindex < 0 || styleindex < 0)
                return;

            var id = ((Tuple<string, uint>)PuppetLB.SelectedItem).Item2;
            puppets_[id].styles[styleindex].abilities[1] = ((Tuple<string, uint>)PuppetAbility2CB.SelectedItem).Item2;
        }

        private void PuppetStyleTypeCB_SelectedIndexChanged(object sender, EventArgs e)
        {
            var index = PuppetStyleTypeCB.SelectedIndex;
            var puppetindex = PuppetLB.SelectedIndex;
            var styleindex = PuppetStyleCB.SelectedIndex;
            if(index < 0 || puppetindex < 0 || styleindex < 0)
                return;

            var id = ((Tuple<string, uint>)PuppetLB.SelectedItem).Item2;
            puppets_[id].styles[styleindex].type = (string)PuppetStyleTypeCB.SelectedItem;
        }

        private void StatSC_ValueChanged(object sender, EventArgs e)
        {
            int statindex;
            if(sender == (object)StatHPSC)
                statindex = 0;
            else if(sender == (object)StatFASC)
                statindex = 1;
            else if(sender == (object)StatFDSC)
                statindex = 2;
            else if(sender == (object)StatSASC)
                statindex = 3;
            else if(sender == (object)StatSDSC)
                statindex = 4;
            else if(sender == (object)StatSPDSC)
                statindex = 5;
            else
                return;

            var val = ((NumericUpDown)sender).Value;
            var puppetindex = PuppetLB.SelectedIndex;
            var styleindex = PuppetStyleCB.SelectedIndex;
            if(puppetindex < 0 || styleindex < 0)
                return;

            var id = ((Tuple<string, uint>)PuppetLB.SelectedItem).Item2;
            puppets_[id].styles[styleindex].base_stats[statindex] = (uint)val;

            var bst = StatHPSC.Value + StatFASC.Value + StatFDSC.Value + StatSASC.Value + StatSDSC.Value + StatSPDSC.Value;
            PuppetBSTLabel.Text = "BST: " + bst.ToString();
        }

        private void ItemCB_SelectedIndexChanged(object sender, EventArgs e)
        {
            int itemindex;
            if(sender == (object)Item1CB)
                itemindex = 0;
            else if(sender == (object)Item2CB)
                itemindex = 1;
            else if(sender == (object)Item3CB)
                itemindex = 2;
            else if(sender == (object)Item4CB)
                itemindex = 3;
            else
                return;

            var index = ((ComboBox)sender).SelectedIndex;
            var puppetindex = PuppetLB.SelectedIndex;
            if(index < 0 || puppetindex < 0)
                return;

            var id = ((Tuple<string, uint>)PuppetLB.SelectedItem).Item2;
            puppets_[id].item_drop_table[itemindex] = ((Tuple<string, uint>)((ComboBox)sender).SelectedItem).Item2;
        }

        private void SkillCardLB_ItemCheck(object sender, ItemCheckEventArgs e)
        {
            var puppetindex = PuppetLB.SelectedIndex;
            var styleindex = PuppetStyleCB.SelectedIndex;
            if(puppetindex < 0 || styleindex < 0)
                return;

            var tmp = SkillCardLB.CheckedItems;
            var items = new List<uint>();
            foreach(var it in tmp)
                items.Add(((Tuple<string, uint>)it).Item2);

            var itemid = ((Tuple<string, uint>)SkillCardLB.Items[e.Index]).Item2;
            if(e.NewValue == CheckState.Checked)
                items.Add(itemid);
            else
                items.RemoveAll(x => x == itemid);

            var ids = new uint[items.Count];
            for(var i = 0; i < items.Count; ++i)
            {
                ids[i] = items[i];
            }

            var id = ((Tuple<string, uint>)PuppetLB.SelectedItem).Item2;
            puppets_[id].styles[styleindex].compatibility = ids;
        }

        private void NewPuppetButton_Click(object sender, EventArgs e)
        {
            if(string.IsNullOrEmpty(working_dir_) || puppets_.Count == 0)
            {
                ErrMsg("No data loaded!");
                return;
            }

            uint id = 0;
            using(var dialog = new NewIDDialog("New Puppet", 511))
            {
                if(dialog.ShowDialog() != DialogResult.OK)
                    return;
                id = (uint)dialog.ID;
            }

            if(puppets_.ContainsKey(id))
            {
                ErrMsg("Puppet ID in use! Select a different ID.");
                return;
            }

            if((id >= puppet_names_.Length) || (id >= 512))
            {
                var min = Math.Min(puppet_names_.Length, 512);
                ErrMsg("Puppet ID too large! ID must be less than " + min.ToString() + ".");
                return;
            }

            uint puppetdex_pos = 0;
            foreach(var i in puppets_)
                if(i.Value.puppetdex_index > puppetdex_pos)
                    puppetdex_pos = i.Value.puppetdex_index;

            if(++puppetdex_pos > 511)
                puppetdex_pos = 0;

            var puppet = new DollData() { id = id, puppetdex_index = puppetdex_pos };
            puppets_[id] = puppet;
            PuppetLB.Items.Add(new Tuple<string, uint>(puppet_names_[id], puppet.id));
            PuppetLB.SelectedIndex = PuppetLB.FindStringExact(puppet_names_[id]);
            obj_flags_[id] = 1;
            if(is_ynk_)
                obj_flags_[id + 400] = 1;
            for(var i = 0; i < (is_ynk_ ? 4 : 3); ++i)
            {
                puppet_flags_[(id * 4) + i] = 1;
            }
        }

        private void DumpPuppets()
        {
            if(string.IsNullOrEmpty(working_dir_) || puppets_.Count == 0)
            {
                ErrMsg("No data loaded!");
                return;
            }

            if(ExportPuppetDialog.ShowDialog() != DialogResult.OK)
                return;

            var filepath = ExportPuppetDialog.FileName;
            if(string.IsNullOrEmpty(filepath))
            {
                ErrMsg("Invalid filepath!");
                return;
            }

            string[] style_levels = { "0", "0", "0", "0", "30", "36", "42", "49", "56", "63", "70" };
            string[] base_levels = { "7", "10", "14", "19", "24" };
            string[] costs = { "80", "90", "100", "110", "120" };
            string[] stats = { "\tHP: ", "\tFo.Atk: ", "\tFo.Def: ", "\tSp.Atk: ", "\tSp.Def: ", "\tSpeed: " };

            string output = "";
            foreach(var it in puppets_)
            {
                var puppet = it.Value;
                var id = it.Key;

                for(uint i = 0; i < 4; ++i)
                {
                    var style = puppet.styles[i];
                    if(style.type.Equals("None", StringComparison.OrdinalIgnoreCase))
                        continue;

                    string typestring = "(" + style.element1;
                    if(!style.element2.Equals("None", StringComparison.OrdinalIgnoreCase))
                        typestring += "/" + style.element2;
                    typestring += ")";

                    output += style.type + " " + puppet_names_[id] + " " + typestring + " " + costs[puppet.cost] + " Cost\r\n";
                    uint bst = 0;
                    for(uint j = 0; j < 6; ++j)
                    {
                        output += stats[j] + style.base_stats[j].ToString() + "\r\n";
                        bst += style.base_stats[j];
                    }
                    output += "\tBST: " + bst.ToString() + "\r\n";

                    output += "\r\n\tAbilities:\r\n";
                    foreach(var ability in style.abilities)
                    {
                        if(ability != 0)
                            output += "\t\t" + ability_names_[ability] + "\r\n";
                    }

                    output += "\r\n\tSkills:\r\n";
                    if(i != 0) // inherit normal form starting skills
                    {
                        for(uint j = 0; j < 4; ++j)
                        {
                            var skillid = puppet.styles[0].style_skills[j];
                            if(skillid != 0)
                                output += "\t\tLvl 0: " + skill_names_[skillid] + "\r\n";
                        }
                    }

                    for(uint j = 0; j < 4; ++j) // starting skills
                    {
                        var skillid = style.style_skills[j];
                        if(skillid != 0)
                            output += "\t\tLvl 0: " + skill_names_[skillid] + "\r\n";
                    }

                    for(uint j = 0; j < 5; ++j) // base skills
                    {
                        var skillid = puppet.base_skills[j];
                        if(skillid != 0)
                            output += "\t\tLvl " + base_levels[j] + ": " + skill_names_[skillid] + "\r\n";
                    }

                    for(uint j = 4; j < 11; ++j) // style skills
                    {
                        var skillid = style.style_skills[j];
                        if(skillid != 0)
                            output += "\t\tLvl " + style_levels[j] + ": " + skill_names_[skillid] + "\r\n";
                    }

                    for(uint j = 0; j < 8; ++j) // extra lvl 70 skills
                    {
                        var skillid = style.lvl70_skills[j];
                        if(skillid != 0)
                            output += "\t\tLvl 70: " + skill_names_[skillid] + "\r\n";
                    }

                    if(style.lvl100_skill != 0) // lvl 100 skill
                    {
                        output += "\t\tLvl 100: " + skill_names_[style.lvl100_skill] + "\r\n";
                    }

                    output += "\r\n\tSkill Cards:\r\n";
                    var skillcards = new SortedSet<uint>();
                    if(i != 0) // inherit skillcards from normal form
                    {
                        foreach(var scid in puppet.styles[0].compatibility)
                            skillcards.Add(scid);
                    }
                    foreach(var scid in style.compatibility)
                        skillcards.Add(scid);

                    foreach(var scid in skillcards)
                        output += "\t\t" + skillcard_names_[scid] + "\r\n";
                    output += "\r\n";

                    try
                    {
                        File.WriteAllText(filepath, output);
                    }
                    catch(Exception ex)
                    {
                        ErrMsg("Failed to write to file: " + filepath + "\r\n" + ex.Message);
                        return;
                    }
                }
            }
        }

        private void ExportPuppetButton_Click(object sender, EventArgs e)
        {
            try
            {
                DumpPuppets();
            }
            catch(Exception ex)
            {
                ErrMsg("Error exporting puppet stats: " + ex.Message);
            }
        }

        private void PuppetMovesetBT_Click(object sender, EventArgs e)
        {
            var puppetindex = PuppetLB.SelectedIndex;
            var styleindex = PuppetStyleCB.SelectedIndex;

            if(puppetindex < 0)
            {
                ErrMsg("No puppet selected.");
                return;
            }

            var base_skills = new List<uint>();
            var style_skills = new List<uint>();

            var id = ((Tuple<string, uint>)PuppetLB.SelectedItem).Item2;
            var puppet = puppets_[id];

            base_skills.AddRange(puppet.base_skills);

            if(styleindex >= 0)
            {
                style_skills.AddRange(puppet.styles[styleindex].style_skills);
                style_skills.AddRange(puppet.styles[styleindex].lvl70_skills);
                style_skills.Add(puppet.styles[styleindex].lvl100_skill);
            }
            else
            {
                for(var i = 0; i < 20; ++i)
                    style_skills.Add(0);
            }

            using(var dlg = new SkillEditDialog(skill_names_, style_skills, base_skills))
            {
                dlg.ShowDialog(this);

                for(var i = 0; i < dlg.BaseSkills.Count; ++i)
                {
                    puppets_[id].base_skills[i] = dlg.BaseSkills[i];
                }

                if(styleindex >= 0)
                {
                    for(var i = 0; i < dlg.StyleSkills.Count; ++i)
                    {
                        if(i < 11)
                            puppets_[id].styles[styleindex].style_skills[i] = dlg.StyleSkills[i];
                        else if(i < 19)
                            puppets_[id].styles[styleindex].lvl70_skills[i - 11] = dlg.StyleSkills[i];
                        else
                            puppets_[id].styles[styleindex].lvl100_skill = dlg.StyleSkills[i];
                    }
                }
            }
        }

        private void PuppetSearchBT_Click(object sender, EventArgs e)
        {
            uint id = 0;
            using(var dialog = new NewIDDialog("Find Puppet by ID", 511))
            {
                if(dialog.ShowDialog() != DialogResult.OK)
                    return;
                id = (uint)dialog.ID;
            }


            for(var i = 0; i < PuppetLB.Items.Count; ++i)
            {
                if(((Tuple<string, uint>)PuppetLB.Items[i]).Item2 == id)
                {
                    PuppetLB.SelectedIndex = i;
                    return;
                }
            }

            ErrMsg("ID not found.");
        }
    }
}
