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

// Skill tab of MainWindow

namespace editor
{
    public partial class EditorMainWindow : Form
    {
        private void LoadSkills(string filepath)
        {
            SkillDataCB.SelectedIndex = -1;
            SkillDataCB.Items.Clear();
            SkillDataCB.DisplayMember = "Item1";
            SkillDataCB.ValueMember = "Item2";
            DataContractJsonSerializer ser = new DataContractJsonSerializer(typeof(SkillJson));
            var buf = File.ReadAllBytes(filepath);
            MemoryStream s = new MemoryStream(buf);
            var tmp = (SkillJson)ser.ReadObject(s);
            foreach(var skill in tmp.skills)
            {
                if(skill_names_.ContainsKey(skill.id))
                {
                    skills_[skill.id] = skill;
                    SkillDataCB.Items.Add(new Tuple<string, uint>(skill_names_[skill.id], skill.id));
                }
            }
        }

        private void SaveSkills(string filepath)
        {
            DataContractJsonSerializer ser = new DataContractJsonSerializer(typeof(SkillJson));
            var buf = File.ReadAllBytes(filepath);
            MemoryStream s = new MemoryStream(buf);
            var tmp = (SkillJson)ser.ReadObject(s);
            for(var i = 0; i < tmp.skills.Length; ++i)
            {
                if(skills_.ContainsKey(tmp.skills[i].id))
                {
                    tmp.skills[i] = skills_[tmp.skills[i].id];
                }
            }
            s = new MemoryStream();
            ser.WriteObject(s, tmp);
            File.WriteAllBytes(filepath, s.ToArray());
        }

        private void SkillDataCB_SelectedIndexChanged(object sender, EventArgs e)
        {
            var index = SkillDataCB.SelectedIndex;
            if(index < 0)
                return;

            var id = ((Tuple<string, uint>)SkillDataCB.Items[index]).Item2;

            if(skill_names_[id] != skills_[id].name)
            {
                if(MessageBox.Show(this, "Embedded name does not match entry in SkillData.csv\nOverwrite?", "Warning", MessageBoxButtons.YesNo, MessageBoxIcon.Warning) == DialogResult.Yes)
                    skills_[id].name = skill_names_[id];
            }

            var skill = skills_[id];

            SkillDataIDSC.Value = id;
            SkillDataTB.Text = skill_names_[id];
            SkillDescTB.Text = skill_descs_[id];
            SkillDataElementCB.SelectedIndex = SkillDataElementCB.FindStringExact(skill.element);
            SkillDataTypeCB.SelectedIndex = SkillDataTypeCB.FindStringExact(skill.type);
            SkillDataSPSC.Value = skill.sp;
            SkillDataAccSC.Value = skill.accuracy;
            SkillDataPowerSC.Value = skill.power;
            SkillDataPrioSC.Value = skill.priority;
            SkillDataEffectChanceSC.Value = skill.effect_chance;
            SkillDataEffectIDSC.Value = skill.effect_id;
            SkillDataEffectTargetSC.Value = skill.effect_target;
            SkillDataClassCB.SelectedIndex = (int)skill.ynk_classification;
        }

        private void SkillDataChanged(object sender, EventArgs e)
        {
            var index = SkillDataCB.SelectedIndex;
            if(index < 0)
                return;

            var id = ((Tuple<string, uint>)SkillDataCB.Items[index]).Item2;
            var skill = skills_[id];

            // spaghetti
            if(sender == (object)SkillDataTB)                   // skill name
            {
                var name = SkillDataTB.Text;
                skills_[id].name = name;
                skill_names_[id] = name;

                SkillDataCB.SelectedIndexChanged -= SkillDataChanged;
                SkillDataCB.Items[index] = new Tuple<string, uint>(name, id);
                SkillDataCB.SelectedIndexChanged += SkillDataChanged;
            }
            else if(sender == (object)SkillDescTB)
            {
                skill_descs_[id] = SkillDescTB.Text;
            }
            else if(sender == (object)SkillDataElementCB)       // skill element
            {
                if(SkillDataElementCB.SelectedIndex < 0)
                    return;
                var element = SkillDataElementCB.Text;
                skills_[id].element = element;
            }
            else if(sender == (object)SkillDataTypeCB)          // skill type
            {
                if(SkillDataTypeCB.SelectedIndex < 0)
                    return;
                var type = SkillDataTypeCB.Text;
                skills_[id].type = type;
            }
            else if(sender == (object)SkillDataSPSC)            // skill sp
            {
                var val = SkillDataSPSC.Value;
                skills_[id].sp = (uint)val;
            }
            else if(sender == (object)SkillDataAccSC)           // skill accuracy
            {
                var val = SkillDataAccSC.Value;
                skills_[id].accuracy = (uint)val;
            }
            else if(sender == (object)SkillDataPowerSC)         // skill power
            {
                var val = SkillDataPowerSC.Value;
                skills_[id].power = (uint)val;
            }
            else if(sender == (object)SkillDataPrioSC)          // skill priority
            {
                var val = SkillDataPrioSC.Value;
                skills_[id].priority = (int)val;
            }
            else if(sender == (object)SkillDataEffectChanceSC)  // effect chance
            {
                var val = SkillDataEffectChanceSC.Value;
                skills_[id].effect_chance = (uint)val;
            }
            else if(sender == (object)SkillDataEffectIDSC)      // effect id
            {
                var val = SkillDataEffectIDSC.Value;
                skills_[id].effect_id = (uint)val;
            }
            else if(sender == (object)SkillDataEffectTargetSC)  // effect target
            {
                var val = SkillDataEffectTargetSC.Value;
                skills_[id].effect_target = (uint)val;
            }
            else if(sender == (object)SkillDataClassCB)         // skill classification
            {
                var val = SkillDataClassCB.SelectedIndex;
                if(val < 0)
                    return;
                skills_[id].ynk_classification = (uint)val;
            }
        }

        private void NewSkillBT_Click(object sender, EventArgs e)
        {
            uint id = 0;
            using(var dialog = new NewIDDialog("New Skill", 1023))
            {
                if(dialog.ShowDialog() != DialogResult.OK)
                    return;
                id = (uint)dialog.ID;
            }

            if(skills_.ContainsKey(id))
            {
                ErrMsg("ID already in use!");
                return;
            }

            var str = "Skill" + id.ToString();
            skill_names_[id] = str;
            skill_descs_[id] = str;
            skills_[id] = new SkillData { id = id, name = str };

            var index = SkillDataCB.Items.Add(new Tuple<string, uint>(str, id));
            SkillDataCB.SelectedIndex = index;
        }
    }
}
