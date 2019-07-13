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

            // Skills
            BaseSkillLvlCB.SelectedIndex = -1;
            BaseSkillCB.SelectedIndex = -1;
            BaseSkillCB.Items.Clear();
            BaseSkillCB.DisplayMember = "Item1";
            BaseSkillCB.ValueMember = "Item2";
            StyleSkillLvlCB.SelectedIndex = -1;
            StyleSkillCB.SelectedIndex = -1;
            StyleSkillCB.Items.Clear();
            StyleSkillCB.DisplayMember = "Item1";
            StyleSkillCB.ValueMember = "Item2";
            BaseSkillCB.Items.Add(new Tuple<string, uint>("None", 0));
            StyleSkillCB.Items.Add(new Tuple<string, uint>("None", 0));
            foreach(var it in skill_names_)
            {
                var id = it.Key;
                var name = it.Value;
                BaseSkillCB.Items.Add(new Tuple<string, uint>(name, id));
                StyleSkillCB.Items.Add(new Tuple<string, uint>(name, id));
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

            PuppetStyleCB.SelectedIndex = -1;
            PuppetStyleTypeCB.SelectedIndex = -1;
            PuppetAbility1CB.SelectedIndex = -1;
            PuppetAbility2CB.SelectedIndex = -1;
            PuppetElement1CB.SelectedIndex = -1;
            PuppetElement2CB.SelectedIndex = -1;
            BaseSkillLvlCB.SelectedIndex = -1;
            BaseSkillCB.SelectedIndex = -1;
            StyleSkillLvlCB.SelectedIndex = -1;
            StyleSkillCB.SelectedIndex = -1;
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

            StyleSkillLvlCB.SelectedIndex = -1;
            StyleSkillCB.SelectedIndex = -1;

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

        private void BaseSkillLvlCB_SelectedIndexChanged(object sender, EventArgs e)
        {
            var index = BaseSkillLvlCB.SelectedIndex;
            var puppetindex = PuppetLB.SelectedIndex;
            if(index < 0 || puppetindex < 0)
                return;

            var id = ((Tuple<string, uint>)PuppetLB.SelectedItem).Item2;
            var puppet = puppets_[id];
            var skillid = puppet.base_skills[index];

            if(skill_names_.ContainsKey(skillid))
                BaseSkillCB.SelectedIndex = BaseSkillCB.FindStringExact(skill_names_[skillid]);
            else
                BaseSkillCB.SelectedIndex = 0;
        }

        private void StyleSkillLvlCB_SelectedIndexChanged(object sender, EventArgs e)
        {
            var index = StyleSkillLvlCB.SelectedIndex;
            var puppetindex = PuppetLB.SelectedIndex;
            var styleindex = PuppetStyleCB.SelectedIndex;
            if(index < 0 || puppetindex < 0 || styleindex < 0)
                return;

            var id = ((Tuple<string, uint>)PuppetLB.SelectedItem).Item2;
            var style = puppets_[id].styles[styleindex];
            uint skillid;

            if(index < 11)
                skillid = style.style_skills[index];
            else if(index < 19)
                skillid = style.lvl70_skills[index - 11];
            else
                skillid = style.lvl100_skill;

            if(skill_names_.ContainsKey(skillid))
                StyleSkillCB.SelectedIndex = StyleSkillCB.FindStringExact(skill_names_[skillid]);
            else
                StyleSkillCB.SelectedIndex = 0;
        }

        private void BaseSkillCB_SelectedIndexChanged(object sender, EventArgs e)
        {
            var index = BaseSkillCB.SelectedIndex;
            var lvlindex = BaseSkillLvlCB.SelectedIndex;
            var puppetindex = PuppetLB.SelectedIndex;
            if(index < 0 || puppetindex < 0 || lvlindex < 0)
                return;

            var id = ((Tuple<string, uint>)PuppetLB.SelectedItem).Item2;
            var skillid = ((Tuple<string, uint>)BaseSkillCB.SelectedItem).Item2;
            puppets_[id].base_skills[lvlindex] = skillid;
        }

        private void StyleSkillCB_SelectedIndexChanged(object sender, EventArgs e)
        {
            var index = StyleSkillCB.SelectedIndex;
            var lvlindex = StyleSkillLvlCB.SelectedIndex;
            var puppetindex = PuppetLB.SelectedIndex;
            var styleindex = PuppetStyleCB.SelectedIndex;
            if(index < 0 || puppetindex < 0 || styleindex < 0 || lvlindex < 0)
                return;

            var id = ((Tuple<string, uint>)PuppetLB.SelectedItem).Item2;
            var skillid = ((Tuple<string, uint>)StyleSkillCB.SelectedItem).Item2;

            if(lvlindex < 11)
                puppets_[id].styles[styleindex].style_skills[lvlindex] = skillid;
            else if(lvlindex < 19)
                puppets_[id].styles[styleindex].lvl70_skills[lvlindex - 11] = skillid;
            else
                puppets_[id].styles[styleindex].lvl100_skill = skillid;
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
            foreach(var it in puppets_)
            {
                if(it.Key > id)
                    id = it.Key;
            }

            ++id;
            if(id >= puppet_names_.Length)
            {
                ErrMsg("No free puppet IDs!\r\nAdd more names to DollName.csv or manually edit DollData.json");
                return;
            }

            var puppet = new DollData() { id = id };
            puppets_[id] = puppet;
            PuppetLB.Items.Add(new Tuple<string, uint>(puppet_names_[id], puppet.id));
            PuppetLB.SelectedIndex = PuppetLB.FindStringExact(puppet_names_[id]);
        }
    }
}
